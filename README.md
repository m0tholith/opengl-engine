# opengl-engine
## cloning and building
only tested for linux
### dependencies
- opengl 3.3
- glfw
- cglm
- assimp
### build and run
```bash
git clone https://github.com/Motholith/opengl-engine
cd opengl-engine
cmake CMakeLists.txt
cmake --build .
```
### build and run (debug)
this enables the `-g` option when running the compiler
```bash
cmake CMakeLists.txt -D CMAKE_BUILD_TYPE=DEBUG
cmake --build .
```
## nix-shell
if you use nix you can use the `shell.nix` file to enter a devshell with all the required dependencies:
```bash
nix-shell
```
if you use `nix-direnv` then you probably don't need me to tell you that the devshell will automatically be enabled
## future goals maybe (in no particular order)
- [X] model animations
    - [X] step "interpolation"
    - [X] linear interpolation
    - [X] smoothstep interpolation
    - [X] skeletal animation
- [ ] lighting
- [ ] lua bindings
