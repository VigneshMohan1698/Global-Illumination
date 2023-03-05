# Global-Illumination

This is my implementation of Raytraced Global Illumination using Spatiotemporal Variance-Guided Filtering. The raytracer/pathtracer is built using DX12 DXR. 
It supports various shaders like Miss,Hit and Raygen.
After the inital GBuffer pass, Another Raytacer pass is made for the GI. Followed by compute shader passes for Temporal Sampling, Variance Estimation and the Atrous 
filter.

Requirements : 
DX12. RTX Graphics Card.

Controls:
1. Imgui options are available to change settings of the scene.
2. WASD to Move the camera. IJKL to Move the light. 
3. F1-F7 shows of different scenes.
4. 1-9 shows off different GBuffers (Shown on IMGUI window)
5. Different options for water, Raytraced Reflections and more on IMGUI windows.
