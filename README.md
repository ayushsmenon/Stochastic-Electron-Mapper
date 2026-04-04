 This project simulates the electron probability density of a Hydrogen atom by numerically solving the time-independent Schrödinger equation and rendering the results as a real-time 3D point cloud.Coordinate points are computed  to visualize $s, p, d,$ and $f$ orbitals.

Mathematical Concept
 Uses radial wavefunctions based on Laguerre polynomials  
 Angular distribution computed using spherical harmonics  
 Probability density is sampled to generate particle positions
Sampling and Rendering
 Random sampling is used to generate points  
 Points are converted from spherical to Cartesian coordinates  
 OpenGL is used to render the point cloud efficiently
Visualization
 Color is mapped based on particle density  
 Rotation and scaling is used to provide a 3D view 
