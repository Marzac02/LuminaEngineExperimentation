<div align="center">

<img src="https://github.com/user-attachments/assets/552b8ca0-ebca-4876-9c6a-df38c468d41e" width="120"/>

# Lumina Game Engine

**A modern, high-performance game engine built with Vulkan**

[![License](https://img.shields.io/github/license/mrdrelliot/lumina)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows-blue)](https://github.com/mrdrelliot/lumina)
[![C++](https://img.shields.io/badge/C++-23-blue)](https://github.com/mrdrelliot/lumina)
[![Vulkan](https://img.shields.io/badge/Vulkan-renderer-red)](https://www.vulkan.org/)
[![Discord](https://img.shields.io/discord/1193738186892005387?label=Discord&logo=discord)](https://discord.gg/xQSB7CRzQE)

[Blog](https://www.dr-elliot.com) • [Discord](https://discord.gg/xQSB7CRzQE) • [Documentation](#-documentation)

</div>

---

## About

Lumina is a modern C++ game engine designed for learning and experimentation with real-world engine architecture. Built from the ground up with Vulkan, it demonstrates professional engine design patterns including reflection systems, ECS architecture, and advanced rendering techniques.

**Perfect for:**
- Learning modern game engine architecture
- Experimenting with Vulkan rendering techniques
- Building prototypes with a clean, modular codebase
- Understanding how engines like Unreal and Godot work under the hood

> **Note:** Lumina is an educational project in active development. APIs may change, and some features are experimental. If you encounter build issues, please reach out on [Discord](https://discord.gg/xQSB7CRzQE) for assistance.

---

## Key Features

### **Advanced Rendering**
- **Vulkan-powered renderer** with automatic resource tracking and barrier placement
- **Forward+ rendering pipeline** with clustered lighting for efficient multi-light scenes
- **PBR materials** with full GLTF/GLB support

### **Modern Architecture**
- **Entity Component System (ECS)** using EnTT for high-performance gameplay code
- **Reflection system** for automatic serialization and editor integration
- **Modular design** with clean separation of concerns

### **Professional Editor**
- **ImGui-based editor** with real-time scene manipulation
- **Visual hierarchy** for easy entity management
- **Component inspector** with automatic UI generation via reflection

### **Performance First**
- **Multi-threaded task system** with EnkiTS
- **Custom memory allocators** using RPMalloc for optimal performance
- **Built-in profiling** with Tracy integration

### **Lua Scripting**

Full ECS access from Lua - Create systems, query entities, modify components
Hot-reloadable scripts - Iterate on gameplay without recompiling
Automatic binding generation - C++ components instantly available in Lua through reflection
Performance profiling - Built-in Lua script profiling with Tracy

---

## 📸 Gallery

<div align="center">

<img src="https://github.com/user-attachments/assets/a6a6a5bb-034e-4423-a25b-2c4dcad6bbc6" alt=Project Creator width=800  />
<img src="https://github.com/user-attachments/assets/a6b973ba-851e-4732-b30b-eb0bf14b08e1" alt="Scene Editor" width="800"/>
<img src="https://github.com/user-attachments/assets/944c2569-a969-42b9-b0e6-88050fb5037c" alt="Clustered Lighting" width="800"/>
<img src="https://github.com/user-attachments/assets/b8717096-e8e9-437b-af18-01502ed821b9" alt="PBR Materials" width="800"/>

</div>

<details>
<summary>📷 View More Screenshots</summary>

<img src="https://github.com/user-attachments/assets/8c81055c-f46a-447d-a79c-31b51fded805" alt="Editor Overview"/>
<img src="https://github.com/user-attachments/assets/6b1dc6a7-ffb3-416b-93ad-d130695e810e" alt="Component Inspector"/>
<img src="https://github.com/user-attachments/assets/9974246c-4bc0-4975-b489-5846f1551c74" alt="Scene Hierarchy"/>
<img src="https://github.com/user-attachments/assets/a5659962-8c9b-4bf7-9730-6ebc079b42fd" alt="Material Editor"/>
<img src="https://github.com/user-attachments/assets/9ccd1e52-bb64-44c8-bd8c-66c1f1545253" alt="Lighting System"/>

</details>

---

## Quick Start

### What You Need
- **Windows 10/11** (64-bit)
- **Visual Studio 2022 or 2019** with MSVC v143 toolset (17.8+)
- **Python 3.8+** added to your system PATH

### Installation Steps

1. **Clone the repository**
   ```bash
   git clone https://github.com/mrdrelliot/lumina.git
   cd lumina
   ```

2. **Run the setup script**
   ```bash
   python Setup.py
   ```
   - This downloads and extracts all dependencies automatically
   - If the download fails, manually download [External.7z](https://www.dropbox.com/scl/fi/suigjbqj75pzcpxcqm6hv/External.7z?rlkey=ebu8kiw4gswtvj5mclg6wa1lu&st=vd98mrab&dl=0) and extract it to the `lumina` folder

3. **Open the solution**
   - Open `Lumina.sln` in Visual Studio

4. **Build and run**
   - Select `Development` configuration
   - Press F5 or click Build → Run

5. **Start developing**
   - Open the `Sandbox` project to experiment
   - Or run `Tools/ProjectConfigurator.py` to create a new project

### Troubleshooting

- **Missing v143 toolset?** Install it via Visual Studio Installer → Individual Components → MSVC v143 Build Tools
- **"Cannot find .generated.h" error?** Build again - Visual Studio sometimes needs a second pass to detect new files
- **Python not found?** Ensure Python is added to PATH during installation (check "Add Python to PATH" when installing)
- **Build fails?** [Submit an issue](https://github.com/mrdrelliot/LuminaEngine/issues) or contact me on Discord

**Note:** The `LUMINA_DIR` environment variable is set automatically during build. If needed, set it manually:
```bash
setx LUMINA_DIR "C:\path\to\lumina"
```

---


## Supported Asset Formats

| Format | Support | Notes |
|--------|---------|-------|
| **GLTF** | Full | Recommended format |
| **GLB** | Full | Binary GLTF |
| **PNG/JPG** | Full | Textures via STB_Image |

### Free Asset Resources

- [Khronos GLTF Samples](https://github.com/KhronosGroup/glTF-Sample-Assets)
- [Kenney 3D Assets](https://kenney.nl/assets?q=3d)
- [Flightradar24 Models](https://github.com/Flightradar24/fr24-3d-models)

---

## Technology Stack

<table>
<tr>
<td valign="top" width="50%">

### Core Systems
- **GLFW** - Window & input management
- **EnTT** - Entity Component System
- **EnkiTS** - Multi-threaded task scheduler
- **EASTL** - High-performance STL replacement
- **RPMalloc** - Custom memory allocator

</td>
<td valign="top" width="50%">

### Rendering
- **Vulkan** - Graphics API
- **VMA** - Vulkan Memory Allocator
- **Volk** - Vulkan Loader
- **VkBootstrap** - Vulkan initialization
- **SPIRV-Reflect** - Shader reflection
- **GLM** - Math library

</td>
</tr>
<tr>
<td valign="top" width="50%">

### Tools & Utilities
- **ImGui** - Editor UI framework
- **Tracy** - Performance profiler
- **SPDLog** - Fast logging
- **NlohmannJson** - JSON serialization
- **XXHash** - Fast hashing

</td>
<td valign="top" width="50%">

### Content Pipeline
- **FastGLTF** - GLTF 2.0 parser
- **STB_Image** - Image loading
- **Reflection** - Custom C++ reflection

</td>
</tr>
</table>

---

## 📖 Documentation

### Coding Standards

Lumina follows a consistent naming convention:

| Prefix | Usage | Example |
|--------|-------|---------|
| `F` | Internal engine types (non-reflected) | `FRenderer`, `FTexture` |
| `C` | Reflected classes | `CTransform`, `CMeshRenderer` |
| `S` | Reflected structs | `SVertex`, `SMaterial` |

**General Rules:**
- ✅ PascalCase for all identifiers
- ✅ Tabs for indentation
- ✅ Braces on new lines
- ✅ Descriptive variable names

See [CONTRIBUTING.md](CONTRIBUTING.md) for complete guidelines.

### Architecture Overview

```
Game Loop → ECS Update → Render Graph → Vulkan Commands → Present
              ↓              ↓              ↓
          Reflection    Scene Graph    Resource Manager
```

---

## Roadmap

### Current Focus
- **Refactoring to dynamic libraries** - Better modularity and faster iteration
- **Scene batched rendering** - Improved draw call efficiency
- **Documentation expansion** - API docs and tutorials

### Upcoming Features
- **Multi-threaded renderer** - Parallel command buffer generation
- **Plugin system** - Hot-reloadable game code
- **Animation system** - Skeletal animation support
- **Physics integration** - Rigid body dynamics
- **Audio system** - 3D spatial audio

### Future Platforms
- **macOS support** - Metal backend
- **Linux support** - Native Vulkan

---

## Contributing

Contributions are welcome! Whether it's bug fixes, features, or documentation improvements.

### How to Contribute
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes following the [coding standards](CONTRIBUTING.md)
4. Add tests if applicable
5. Commit with clear messages (`git commit -m 'Add amazing feature'`)
6. Push to your branch (`git push origin feature/amazing-feature`)
7. Open a Pull Request

**Requirements:**
- ✅ Clean, well-documented code
- ✅ Follow existing architecture patterns
- ✅ Include tests where appropriate
- ✅ Update documentation as needed

---

## 🙏 Acknowledgments

Lumina is inspired by and learns from these excellent open-source engines:

- [**Spartan Engine**](https://github.com/PanosK92/SpartanEngine) - Feature-rich Vulkan engine
- [**Kohi Game Engine**](https://kohiengine.com/) - Educational engine series
- [**ezEngine**](https://ezengine.net/) - Professional open-source engine
- [**GoDot**](https://godotengine.org/) - AAA quality open source C++ game engine.
- [**Unreal Engine**](https://www.unrealengine.com/) - Does it need an introduction?

Special thanks to the entire game engine development community for sharing knowledge and resources.

---

## 📄 License

Lumina is licensed under the [Apache 2.0 License](LICENSE).

```
Copyright 2024 Dr. Elliot

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```

---

## Connect

- **Blog**: [dr-elliot.com](https://www.dr-elliot.com)
- **Discord**: [Join our community](https://discord.gg/xQSB7CRzQE)
- **GitHub**: [mrdrelliot/lumina](https://github.com/mrdrelliot/lumina)

---

<div align="center">

**Made with ❤️ for the game development community**

⭐ Star this repo if you find it useful!

</div>
