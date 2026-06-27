# 3D Model Viewer

3D Model Viewer renders 3D models using a custom game engine developed by Hatori
Kobo. It serves as a basic demonstration of the 3D capabilities of the
proprietary engine.

[![Demo](demo.webp)](https://youtu.be/VJbMJa1tWf8)

All code has been written from scratch in-house following "handmade" principles,
with minimal use of external libraries and other dependencies. The only
exception to this is [Dear ImGui](https://github.com/ocornut/imgui) for the GUI.
In lieu of even the C standard library, Hatori Kobo maintains its own internal
common library.

While 3D Model Viewer is open-source, the underlying game engine and its various
graphics backends, as well as internal library and platform code, remain
closed-source. See
[Releases](https://github.com/hatori-kobo/3d-model-viewer/releases) to download
and run the application.

## Features

- Physically-based rendering (PBR) with support for base color,
  metallic-roughness, normal, and emissive textures
- Custom renderers for all modern PC graphics APIs (Direct3D 11, Direct3D 12,
  and Vulkan)
- Bindless rendering and custom texture cache (Direct3D 12/Vulkan)
- Rotations represented as quaternions generated from axis-angle
- Support for non-opaque materials (via translucency sorting)
- Support for animation, including skeletal animation (via mesh skinning)
- Support for arbitrary display resolutions, refresh rates, and aspect ratios
- Support for Steam Deck and other Linux-based devices via Proton
- Arcball camera (camera is rotated on a sphere around the model)
- Mouse/keyboard and gamepad controls for model selection, rotation, zoom, etc.
- Immediate-mode GUI for displaying performance metrics, controls, etc.
- Wireframe mode

## Models

The included 3D models are processed from their original glTF 2.0 binary format
(.glb) and packed into a single Hatori Kobo Assets (.hka) file.

### Attributions

- "Abstract Rainbow Translucent Pendant" by riach is licensed under
  [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).
- "Box Animated" by Cesium is licensed under
  [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).
- "Brainstem" by Keith Hunter is licensed under
  [Poser End User License Agreement](https://www.posersoftware.com/documentation/12/Poser_Reference_Manual/Introduction/Installing/Poser_End_User_License_Agreement_(EULA).htm).
- "Corset" by Microsoft is Public Domain.
- "Damaged Helmet" by theblueturtle\_ (model) and ctxwing (rebuild and
  conversion to glTF) is licensed under
  [CC BY-NC 4.0](https://creativecommons.org/licenses/by-nc/4.0/) and
  [CC BY 4.0 International](https://creativecommons.org/licenses/by/4.0/)
  respectively.
- "Fox" by PixelMannen (model), tomkranis (rigging and animation), and
  AsoboStudio and scurest (conversion to glTF) is licensed under
  [CC0 1.0 Universal](https://creativecommons.org/publicdomain/zero/1.0/),
  [CC BY 4.0 International](https://creativecommons.org/licenses/by/4.0/), and
  [CC BY 4.0 International](https://creativecommons.org/licenses/by/4.0/)
  respectively.
- "Ftm" by luyssport is licensed under
  [CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/).
- "PlayStation 1" by Lars Maes is licensed under
  [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).
- "Virtual City" by 3DRT is licensed under
  [3DRT License](https://3drt.com/store/terms-of-use-license.html).
- "Water Bottle" by Microsoft is Public Domain.
