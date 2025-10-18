# Simple Raytracer / Ray-walker (`raytracer.cpp`)

This folder contains a ray-walking renderer implemented using immediate-mode OpenGL and GLFW. It traces rays from a camera through a 2D scene with circles, quadrilaterals and gravitational bodies that can influence ray paths. The renderer supports multi-threaded ray simulation and simple on-screen editing.

Highlights
- Ray physics include radial collisions, simple reflectivity, color accumulation, and gravitational deflection.
- Interactive: left-click to inspect and edit objects, right-click to remove objects, and UI text input via the console for editing properties.
- Uses multiple threads to compute ray paths.

Build & run
- Requires GLFW and OpenGL. Link with -lglfw -lGL (or -lglfw3 -lopengl32 on Windows).

Example (MSYS2 / mingw-w64 on Windows):

```powershell
g++ "raytracer.cpp" -o "raytracer.exe" -lglfw3 -lopengl32 -lgdi32 -std=c++17 -pthread
.\\raytracer.exe
```

Controls & notes
- The code contains parameters at the top (resolution, ray speed, ray degradation, thread count) — change these to adjust quality and performance.
- Interact with on-screen objects using mouse clicks; follow console prompts when editing objects.
- The program uses immediate-mode GL to draw debug geometry and ray paths.

Suggested improvements
- Add a simple GUI (e.g., Dear ImGui) to remove console input and expose interactive editing controls.
- Replace immediate-mode drawing with a pixel buffer or shader-based renderer for higher performance.
