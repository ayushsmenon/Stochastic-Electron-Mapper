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

## Notes
- Performance depends on system capability due to high particle count  
- Lower particle counts can be used for smoother performance  

---
