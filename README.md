# libpandabsp
`libpandabsp` is an extension library for the [Panda3D](https://www.github.com/panda3d/panda3d "Panda3D GitHub Page") game engine. It implements a whole host of features, ready to be used in your Panda3D game!

The library is written fully in C++ and most of the classes are published to Python via Panda3D's `interrogate` tool.

Since I have just published it, the repository is currently not buildable/useable by itself yet.

# Dependencies
* [Panda3D SDK](https://panda3d.org)
* [Intel Embree SDK](http://embree.org)
* [keyvalue-parser](https://github.com/lachbr/keyvalue-parser)
* [libpandabsp-compilers](https://github.com/lachbr/libpandabsp-compilers) `common` library

# Features

* Shader system
  * Each shader implements a class that configures a variation of itself for each `RenderState` that uses the shader
  * `RenderStates` and material file parameters influence/configure shader variations
  * Various provided shaders (Model PBR, Lightmapped PBR, etc)
* Material system
  * Material files configure a shader's parameters
  * Can be applied to any node or `RenderState`
* Post Processing system
  * Bloom and HDR tonemapping filters provided
* Custom BSP level system
  * Derived from Half-Life BSP format
  * New features based on Source Engine BSP format
  * Construct level with brush geometry
  * BSP tree computed from brushes
  * Ray tracing against and querying the BSP tree
  * Precomputed visibility structure for optimized rendering
  * Entities (point and brush)
  * Precomputed HDR radiosity lightmaps on brush surfaces
  * Lightmaps packed into single texture when loaded
  * Radiosity normal mapping on normal-mapped brush surfaces
  * Precomputed directional ambient lighting probes for dynamic models
  * Static prop models with precomputed vertex lighting
  * Dynamic lighting and cascaded shadow mapping for the sun on models and brushes
  * Pre-rendered HDR cubemap reflection probes
  * Transitioning entities between levels
  * Client and server side views of levels
  * Collision meshes for Bullet or built-in Panda collision
  * Decal tracing and clipping onto brush surfaces
  * Dynamic models lit with four closest light entities, closest ambient probe, closest reflection probe
* Abstraction layer to Intel's Embree ray tracing library
* Panda3D's `Audio3DManager` rewritten in C++
* Glow effect with an occlusion query

WIP readme
