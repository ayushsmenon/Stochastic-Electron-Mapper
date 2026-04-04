import glfw
from OpenGL.GL import *
from OpenGL.GLU import *
import numpy as np
import scipy.special as spec
from math import pi, cos, sin, factorial
import imgui
from imgui.integrations.glfw import GlfwRenderer
import time
from PIL import Image 

TOTAL_PARTICLES = 500000

class OrbitalState:
    def __init__(self):
        self.n, self.l, self.m, self.Z = 3, 2, 0, 1
        self.brightness = 0.6
        self.rev_speed = 1.0  
        self.pts = np.zeros((TOTAL_PARTICLES, 3), dtype=np.float32)
        self.target_pts = np.zeros((TOTAL_PARTICLES, 3), dtype=np.float32)
        self.cols = np.zeros((TOTAL_PARTICLES, 3), dtype=np.float32)
        self.morph_factor = 1.0

    def get_color(self, intensity, zoom_dist):
        adaptive_br = self.brightness * (130.0 / zoom_dist)**1.3
        val = np.clip(intensity * adaptive_br, 0, 1)
        r = np.clip(0.1 + val * 3.0, 0, 1)
        g = np.clip((val - 0.65) * 5.0, 0, 1) if val > 0.65 else 0.0
        b = np.clip(0.3 + (val - 0.65) * 2.5, 0, 1) if val > 0.65 else np.clip(0.1 + val * 0.4, 0, 1)
        return [r, g, b]

    def compute(self, zoom_dist):
        res, r_max = 2000, (self.n**2 / self.Z) * 15.0
        r_vals = np.linspace(0, r_max, res)
        rho = (2.0 * self.Z * r_vals) / self.n
        norm = np.sqrt(((2.0 * self.Z) / self.n)**3 * factorial(self.n - self.l - 1) / (2.0 * self.n * factorial(self.n + self.l)))
        R = norm * np.exp(-rho / 2.0) * (rho**self.l) * spec.assoc_laguerre(rho, self.n - self.l - 1, 2 * self.l + 1)
        
        r_cdf = np.cumsum((r_vals**2) * (R**2)); r_cdf /= r_cdf[-1]
        t_vals = np.linspace(0, pi, res)
        t_cdf = np.cumsum(np.sin(t_vals) * (spec.lpmv(abs(self.m), self.l, np.cos(t_vals))**2)); t_cdf /= t_cdf[-1]
        
        u_r, u_t, u_p = np.random.random(TOTAL_PARTICLES), np.random.random(TOTAL_PARTICLES), np.random.random(TOTAL_PARTICLES) * 2 * pi
        r, theta = np.interp(u_r, r_cdf, r_vals), np.interp(u_t, t_cdf, t_vals)
        
        self.target_pts[:, 0] = r * np.sin(theta) * np.cos(u_p)
        self.target_pts[:, 1] = r * np.cos(theta)
        self.target_pts[:, 2] = r * np.sin(theta) * np.sin(u_p)
        
        r_vec = np.linalg.norm(self.target_pts, axis=1)
        ints = np.clip(2.5 / (r_vec / (self.n / self.Z) + 2.0), 0.1, 1.0)
        self.cols = np.array([self.get_color(i, zoom_dist) for i in ints], dtype=np.float32)
        self.morph_factor = 0.0

class Camera:
    def __init__(self):
        self.dist, self.rot_x, self.rot_y = 130.0, 30.0, 45.0
        self.last_mouse = (0, 0)
        self.dragging = False

orbital = OrbitalState()
cam = Camera()

def scroll_callback(window, xoffset, yoffset):
    if not imgui.get_io().want_capture_mouse:
        cam.dist = np.clip(cam.dist - yoffset * 10.0, 5.0, 2500.0)

def main():
    if not glfw.init(): return
    win = glfw.create_window(1280, 720, "Schrödinger's Atomic Model", None, None)
    glfw.make_context_current(win)
    
    imgui.create_context(); impl = GlfwRenderer(win)
    glfw.set_scroll_callback(win, scroll_callback)

    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE); glEnable(GL_POINT_SMOOTH)
    orbital.compute(cam.dist); orbital.pts = np.copy(orbital.target_pts)
    last_zoom = cam.dist

    while not glfw.window_should_close(win):
        glfw.poll_events(); impl.process_inputs(); imgui.new_frame()
        
        if orbital.morph_factor < 1.0:
            orbital.morph_factor += 0.05
            orbital.pts += (orbital.target_pts - orbital.pts) * 0.12

        # UI Panel
        imgui.begin("Controls")
        c_n, orbital.n = imgui.slider_int("n ", orbital.n, 1, 6)
        c_l, orbital.l = imgui.slider_int("l ", orbital.l, 0, orbital.n - 1)
        c_m, orbital.m = imgui.slider_int("m ", orbital.m, -orbital.l, orbital.l)
        c_z, orbital.Z = imgui.slider_int("Z ", orbital.Z, 1, 10)
        
        imgui.separator()
        _, orbital.rev_speed = imgui.slider_float("Rev Speed", orbital.rev_speed, 0.0, 5.0)
        c_b, orbital.brightness = imgui.slider_float("Glow", orbital.brightness, 0.1, 1.5)
        
        if any([c_n, c_l, c_m, c_z]): 
            orbital.l = min(orbital.l, orbital.n - 1)
            orbital.m = max(-orbital.l, min(orbital.m, orbital.l))
            orbital.compute(cam.dist)
        
        # Live color update
        if c_b or abs(last_zoom - cam.dist) > 1.0:
            r_vec = np.linalg.norm(orbital.pts, axis=1)
            ints = np.clip(2.5 / (r_vec / (orbital.n / orbital.Z) + 2.0), 0.1, 1.0)
            orbital.cols = np.array([orbital.get_color(i, cam.dist) for i in ints], dtype=np.float32)
            last_zoom = cam.dist

        if imgui.button("Save image"):
            w, h = glfw.get_framebuffer_size(win)
            glPixelStorei(GL_PACK_ALIGNMENT, 1)
            data = glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE)
            Image.frombytes("RGB", (w, h), data).transpose(Image.FLIP_TOP_BOTTOM).save(f"atom_{int(time.time())}.png")

        imgui.end()

        if not imgui.get_io().want_capture_mouse:
            x, y = glfw.get_cursor_pos(win)
            if glfw.get_mouse_button(win, 0):
                if cam.dragging:
                    cam.rot_y += (x - cam.last_mouse[0]) * 0.2
                    cam.rot_x += (y - cam.last_mouse[1]) * 0.2
                cam.dragging, cam.last_mouse = True, (x, y)
            else: cam.dragging = False

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluPerspective(45, 1280/720, 0.1, 5000.0)
        glMatrixMode(GL_MODELVIEW); glLoadIdentity(); glTranslatef(0, 0, -cam.dist)
        glRotatef(cam.rot_x, 1, 0, 0); glRotatef(cam.rot_y, 0, 1, 0)

        glRotatef(glfw.get_time() * 20.0 * orbital.rev_speed, 0, 1, 0)

        glPointSize(max(0.4, 1.2 * (130.0 / cam.dist)))
        glEnableClientState(GL_VERTEX_ARRAY); glEnableClientState(GL_COLOR_ARRAY)
        glVertexPointer(3, GL_FLOAT, 0, orbital.pts); glColorPointer(3, GL_FLOAT, 0, orbital.cols)
        glDrawArrays(GL_POINTS, 0, TOTAL_PARTICLES)
        
        glPointSize(10.0); glBegin(GL_POINTS); glColor3f(1, 1, 1); glVertex3f(0, 0, 0); glEnd()
        
        imgui.render(); impl.render(imgui.get_draw_data()); glfw.swap_buffers(win)
    
    impl.shutdown(); glfw.terminate()

if __name__ == "__main__": main()

