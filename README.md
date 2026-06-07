## Project Overview
This project simulates the electron probability density of a hydrogen atom by numerically solving the time-independent Schrödinger equation and rendering the results as a real-time 3D point cloud.

Coordinate points are computed to visualize s, p, d, and f orbitals.

---

## Mathematical Concepts
- Uses radial wavefunctions based on Laguerre polynomials  
- Angular distribution computed using spherical harmonics  
- Probability density is sampled to generate particle positions  

---

## Sampling and Rendering
- Random sampling is used to generate large-scale point distributions  
- Points are converted from spherical to Cartesian coordinates  
- OpenGL is used to render the point cloud efficiently  

---

## Visualization
- Color is mapped based on particle density  
- Rotation and scaling are used to provide 3D view  



## References

### Physics & Mathematics
- D. J. Griffiths, *Introduction to Quantum Mechanics*  
- E. Schrödinger, “An Undulatory Theory of the Mechanics of Atoms and Molecules”  
- Wolfram MathWorld – Spherical Harmonics  

### Computer Graphics
- LearnOpenGL – Coordinate Systems  
- Lighthouse3D – GLSL and spherical coordinates tutorials  

---
---

## Acknowledgments
* **[Dear ImGui](https://github.com/ocornut/imgui)**: Created by Omar Cornut and contributors. It provided the lightweight and intuitive graphical user interface necessary for real-time parameter tuning.
* **[GLFW](https://www.glfw.org/)**: A multi-platform library for OpenGL, OpenGL ES, Vulkan, window management, and input, which served as the backbone for our windowing and user interaction layer.

---

## Notes
- Performance depends on system capability due to high particle count  
- Lower particle counts can be used for smoother performance  

---

# Setup & Compilation (C++)

## Build Command (Windows / MinGW)

Run the following command in the project root directory to compile the optimized simulation:

```bash
g++ -std=c++17 -O3 main.cpp \imgui/imgui.cpp \imgui/imgui_draw.cpp \imgui/imgui_tables.cpp \imgui/imgui_widgets.cpp \imgui/backends/imgui_impl_glfw.cpp \imgui/backends/imgui_impl_opengl2.cpp \-I./imgui \-I./imgui/backends \-I./glfw/include \-L./glfw/lib-mingw-w64 \-lglfw3 -lopengl32 -lgdi32 \-o atomic_sim.exe
```

---

# Running the Simulation

Once the project is successfully compiled, run the executable from the project root directory.

## Windows (PowerShell / Command Prompt)

```bash
.\atomic_sim.exe
```
