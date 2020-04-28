__libpandabsp__ is a collection of custom tools and libraries I have developed for the Panda3D game engine.

The name comes from the original goal of the project -- to add native support for Half-Life BSP levels into Panda. Over time, the project has grown to support lots more than just BSP levels.

## Features
* Custom BSP format based on the Half-Life 1 BSP format. Modified to work natively with Panda and to support new features based on the Source Engine BSP format, such as HDR lightmaps, radiosity normal mapping, cubemap reflection probes, irradiance probes, precomputed vertex lighting for props, etc.
* Custom tools for compiling said BSP levels. These tools are a modification of [Vluzacn's Zoner's Half-Life Tools](http://www.zhlt.info/).
* Support for loading, rendering, querying, and operating on the BSP levels in a Panda application. Makes use of the PVS for culling. Automatically applies lights, irradiance probes, and cubemap reflection probes from the level to dynamic models.

WIP readme
