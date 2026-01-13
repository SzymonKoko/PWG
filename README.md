# Procedural World Generation Engine

A real-time 3D graphics engine for GPU-accelerated procedural terrain generation.


## 🚀 Overview
This project is a custom-built 3D engine developed in C++20 using OpenGL 4.5.
Its main goal is to generate large-scale realistic procedural worlds in real time using GPU compute shaders,
while maintaining a clean and modular engine architecture.

The engine is being developed as part of my Bachelor`s thesis.

## ✨ Features
- Real-time procedural terrain generation using GPU compute shaders
- Physically Based Rendering (PBR) material system
- Dynamic lighting
- Two work modes (Editor, Play)
- Modular engine architecture with separation of rendering logic and OpenGL backend
- ImGui-based editor and debugging tools

## 🛠️ Technologies
- **Language:** C++20
- **Graphics API:** OpenGL 4.5
- **Shaders:** GLSL (vertex, fragment, compute)
- **GUI:** ImGui
- **Build system:** CMake

## 📸 Demo
▶️ YouTube demo: https://youtube.com/playlist?list=PL-Xeyhsvkx1uo2XyCaSVRS5xcVskqtB-v

## ⚙️ Build & Run
```bash
git clone https://github.com/SzymonKoko/PWG.git
cd PWG
./build.bat
```

## ✨ Future features
- Objects model loading and placing on the terrain
- Chunks and LOD (level of details) system
- Water and hydraulic erosion
- Post processing effects (bloom, HDR...)
- World serialization to file
- First person walking mode