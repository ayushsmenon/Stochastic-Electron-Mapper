#define _USE_MATH_DEFINES
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl2.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <algorithm>
#include <fstream>
#include <ctime>
#include <cstring>
#include <array>

const int TOTAL_PARTICLES = 500000;

double factorial(int n) {
    double res = 1.0;
    for (int i = 2; i <= n; ++i) res *= i;
    return res;
}

double interp(double x, const std::vector<double>& xp, const std::vector<double>& fp) {
    auto it = std::lower_bound(xp.begin(), xp.end(), x);
    if (it == xp.begin()) return fp.front();
    if (it == xp.end()) return fp.back();
    size_t i = std::distance(xp.begin(), it);
    double t = (x - xp[i - 1]) / (xp[i] - xp[i - 1]);
    return fp[i - 1] + t * (fp[i] - fp[i - 1]);
}

struct Camera {
    double dist = 130.0, rot_x = 30.0, rot_y = 45.0;
    double last_mouse_x = 0, last_mouse_y = 0;
    bool dragging = false;
} cam;

struct OrbitalState {
    int n = 3, l = 2, m = 0, Z = 1;
    float brightness = 0.6f;
    float rev_speed = 1.0f;
    float morph_factor = 1.0f;
    
    std::vector<float> pts;
    std::vector<float> target_pts;
    std::vector<float> cols;

    OrbitalState() {
        pts.resize(TOTAL_PARTICLES * 3, 0.0f);
        target_pts.resize(TOTAL_PARTICLES * 3, 0.0f);
        cols.resize(TOTAL_PARTICLES * 3, 0.0f);
    }

    std::array<float, 3> get_color(float intensity, double zoom_dist) {
        float adaptive_br = brightness * std::pow(130.0f / (float)zoom_dist, 1.3f);
        float val = std::clamp(intensity * adaptive_br, 0.0f, 1.0f);
        float r = std::clamp(0.1f + val * 3.0f, 0.0f, 1.0f);
        float g = val > 0.65f ? std::clamp((val - 0.65f) * 5.0f, 0.0f, 1.0f) : 0.0f;
        float b = val > 0.65f ? std::clamp(0.3f + (val - 0.65f) * 2.5f, 0.0f, 1.0f) : std::clamp(0.1f + val * 0.4f, 0.0f, 1.0f);
        return {r, g, b};
    }

    void compute(double zoom_dist) {
        int res = 2000;
        double r_max = (std::pow(n, 2) / Z) * 15.0;
        
        std::vector<double> r_vals(res), t_vals(res);
        std::vector<double> r_cdf(res), t_cdf(res);

        double norm = std::sqrt(std::pow(2.0 * Z / n, 3) * factorial(n - l - 1) / (2.0 * n * factorial(n + l)));
        double r_cdf_sum = 0.0, t_cdf_sum = 0.0;

        for (int i = 0; i < res; ++i) {
            double r = r_max * i / (res - 1);
            r_vals[i] = r;
            double rho = (2.0 * Z * r) / n;
            
            // Laguerre polynomial
            double R = norm * std::exp(-rho / 2.0) * std::pow(rho, l) * std::assoc_laguerre(n - l - 1, 2 * l + 1, rho);
            r_cdf_sum += r * r * R * R;
            r_cdf[i] = r_cdf_sum;

            double t = M_PI * i / (res - 1);
            t_vals[i] = t;
            
            // Associated Legendre polynomial
            double P = std::assoc_legendre(l, std::abs(m), std::cos(t));
            t_cdf_sum += std::sin(t) * P * P;
            t_cdf[i] = t_cdf_sum;
        }

        for (int i = 0; i < res; ++i) {
            r_cdf[i] /= r_cdf_sum;
            t_cdf[i] /= t_cdf_sum;
        }

        std::mt19937 gen(1337); 
        std::uniform_real_distribution<double> dist_u(0.0, 1.0);
        std::uniform_real_distribution<double> dist_p(0.0, 2.0 * M_PI);

        for (int i = 0; i < TOTAL_PARTICLES; ++i) {
            double u_r = dist_u(gen);
            double u_t = dist_u(gen);
            double u_p = dist_p(gen);

            double r = interp(u_r, r_cdf, r_vals);
            double theta = interp(u_t, t_cdf, t_vals);

            target_pts[i * 3 + 0] = r * std::sin(theta) * std::cos(u_p);
            target_pts[i * 3 + 1] = r * std::cos(theta);
            target_pts[i * 3 + 2] = r * std::sin(theta) * std::sin(u_p);

            float r_vec = static_cast<float>(r);
            float intensity = std::clamp(2.5f / (r_vec / ((float)n / Z) + 2.0f), 0.1f, 1.0f);
            auto c = get_color(intensity, zoom_dist);
            cols[i * 3 + 0] = c[0];
            cols[i * 3 + 1] = c[1];
            cols[i * 3 + 2] = c[2];
        }
        morph_factor = 0.0f;
    }
} orbital;


void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureMouse) {
        cam.dist = std::clamp(cam.dist - yoffset * 10.0, 5.0, 2500.0);
    }
}

// Drops GLU dependency
void set_perspective(double fovY, double aspect, double zNear, double zFar) {
    double fH = std::tan(fovY / 360 * M_PI) * zNear;
    double fW = fH * aspect;
    glFrustum(-fW, fW, -fH, fH, zNear, zFar);
}

void save_image_ppm(int width, int height) {
    std::vector<unsigned char> pixels(width * height * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // Flip vertically
    std::vector<unsigned char> flipped(width * height * 3);
    for (int y = 0; y < height; ++y) {
        std::memcpy(&flipped[y * width * 3], &pixels[(height - 1 - y) * width * 3], width * 3);
    }

    std::string filename = "atom_" + std::to_string(std::time(nullptr)) + ".ppm";
    std::ofstream out(filename, std::ios::binary);
    out << "P6\n" << width << " " << height << "\n255\n";
    out.write(reinterpret_cast<char*>(flipped.data()), flipped.size());
}

int main() {
    if (!glfwInit()) return -1;
    GLFWwindow* win = glfwCreateWindow(1280, 720, "Schrodinger's Atomic Model", NULL, NULL);
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1); // Enable vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL2_Init();
    
    glfwSetScrollCallback(win, scroll_callback);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_POINT_SMOOTH);

    orbital.compute(cam.dist);
    orbital.pts = orbital.target_pts;
    double last_zoom = cam.dist;

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (orbital.morph_factor < 1.0f) {
            orbital.morph_factor += 0.05f;
            for (size_t i = 0; i < orbital.pts.size(); ++i) {
                orbital.pts[i] += (orbital.target_pts[i] - orbital.pts[i]) * 0.12f;
            }
        }

        ImGui::Begin("Controls");
        bool c_n = ImGui::SliderInt("n ", &orbital.n, 1, 6);
        bool c_l = ImGui::SliderInt("l ", &orbital.l, 0, orbital.n - 1);
        bool c_m = ImGui::SliderInt("m ", &orbital.m, -orbital.l, orbital.l);
        bool c_z = ImGui::SliderInt("Z ", &orbital.Z, 1, 10);
        
        ImGui::Separator();
        ImGui::SliderFloat("Rev Speed", &orbital.rev_speed, 0.0f, 5.0f);
        bool c_b = ImGui::SliderFloat("Glow", &orbital.brightness, 0.1f, 1.5f);

        if (c_n || c_l || c_m || c_z) {
            orbital.l = std::min(orbital.l, orbital.n - 1);
            orbital.m = std::max(-orbital.l, std::min(orbital.m, orbital.l));
            orbital.compute(cam.dist);
        }

        if (c_b || std::abs(last_zoom - cam.dist) > 1.0) {
            for (int i = 0; i < TOTAL_PARTICLES; ++i) {
                float rx = orbital.pts[i*3], ry = orbital.pts[i*3+1], rz = orbital.pts[i*3+2];
                float r_vec = std::sqrt(rx*rx + ry*ry + rz*rz);
                float intensity = std::clamp(2.5f / (r_vec / ((float)orbital.n / orbital.Z) + 2.0f), 0.1f, 1.0f);
                auto c = orbital.get_color(intensity, cam.dist);
                orbital.cols[i*3+0] = c[0]; orbital.cols[i*3+1] = c[1]; orbital.cols[i*3+2] = c[2];
            }
            last_zoom = cam.dist;
        }

        if (ImGui::Button("Save image")) {
            int w, h;
            glfwGetFramebufferSize(win, &w, &h);
            save_image_ppm(w, h);
        }
        ImGui::End();

        ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureMouse) {
            double x, y;
            glfwGetCursorPos(win, &x, &y);
            if (glfwGetMouseButton(win, 0) == GLFW_PRESS) {
                if (cam.dragging) {
                    cam.rot_y += (x - cam.last_mouse_x) * 0.2;
                    cam.rot_x += (y - cam.last_mouse_y) * 0.2;
                }
                cam.dragging = true;
                cam.last_mouse_x = x;
                cam.last_mouse_y = y;
            } else {
                cam.dragging = false;
            }
        }

        int display_w, display_h;
        glfwGetFramebufferSize(win, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        set_perspective(45.0, (double)display_w / display_h, 0.1, 5000.0);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(0.0f, 0.0f, -cam.dist);
        glRotatef(cam.rot_x, 1.0f, 0.0f, 0.0f);
        glRotatef(cam.rot_y, 0.0f, 1.0f, 0.0f);
        glRotatef(glfwGetTime() * 20.0 * orbital.rev_speed, 0.0f, 1.0f, 0.0f);

        glPointSize(std::max(0.4, 1.2 * (130.0 / cam.dist)));
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, orbital.pts.data());
        glColorPointer(3, GL_FLOAT, 0, orbital.cols.data());
        glDrawArrays(GL_POINTS, 0, TOTAL_PARTICLES);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);

        glPointSize(10.0f);
        glBegin(GL_POINTS);
        glColor3f(1.0f, 1.0f, 1.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);
        glEnd();

        ImGui::Render();
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(win);
    }

    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}