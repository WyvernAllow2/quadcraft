# Quadcraft
A Minecraft clone written in C using OpenGL.

## Build instructions
### Prerequisites
* [git](https://git-scm.com/)
* [CMake](https://cmake.org/)
* Any C compiler with C99 support (clang, gcc, MSVC etc.)

### Build steps
```shell
# 1. Clone the repository and all submodules
git clone --recursive https://github.com/WyvernAllow2/quadcraft.git
cd quadcraft

# 2. Configure and build with CMake
cmake -S . -B build
cmake --build build
```

_After building, `quadcraft.exe` will be found somewhere in the `build/` directory, depending on the build tool used._

**NOTE:** When running `quadcraft.exe` ensure that `res/` exists in the working directory of the executable, otherwise assets will fail to load.

## Dependencies
**NOTE:** All dependencies are included as git submodules in `deps/`

* [glad](https://github.com/Dav1dde/glad) - OpenGL loader
* [glfw](https://github.com/glfw/glfw) - Windowing
* [stb_image](https://github.com/nothings/stb) - Image loading

## License
See [LICENSE.txt](LICENSE.txt)