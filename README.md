# raylib_module_editor_c

# License: MIT

# Status:
- Prototyping builds.
- network test.
- unstable code
- subject to change code builds.

# Programing Languages:
- c language ( c99 )
- cmake ( for config build )
- lua ?

# Tools:
- Cmake 3.5
- Msys2 64

# Information:
  This project will have sample files for raylib with other libraries to run examples.

  To able to create raylib editor. Which required some features from other examples to get part of it working. To able to refine and learn from it.
  
  Idea base on minecraft and other ideas base game for voxel in c program language. The models will be base on blocks and non block mesh to keep thing simple. It low poloygon style.

# Flecs:
  Entity Component System. I keep it simple. Anything can be entity.

  Basically of the create entity is default number index which does not start with 0 index. 0 is reserve for empty entity. As it has made many entities from 1-540 are created. Think of premade entity run in background like update, start up, relation of entities and so on. To able to run a simple loop.

  You can think of node tree as loop through the relateship logic system loop.


# Goals:
- [ ] Need to add examples and docs.
- [ ] raygui to handle entities type of data.
- [x] Physics 3d using the ODE simple.
- [x] Network test using enet simple.
- [ ] raygui sample tests.

## Editor:
- [ ] menu
- [ ] file system
- [ ] json
- [ ] object
  - [ ] add
  - [ ] remove
  - [ ] position
  - [ ] rotation
  - [ ] scale
  - [ ] select

# Tests:
- [x] Physics Ode 3D simple test
- [x] enet simple test
- [x] transform 3d hierarchy

# Flecs Samples:
- [ ] main_editor_test.c
- [ ] main_gui_test.c

- [x] main_camera_picking.c
  - simple ray cast detect cube
- [x] main_camera_test.c
- [x] main_enet_test.c
  - server tested
    - [x] add clients
    - [x] sent all clients to message pack
    - [x] board cast to message pack
  - client tested
    - [x] sent server to message pack
    - [x] when close it sent disconnect
- [ ] main_transform_3d_hierarchy_test.c
- [x] main_ode_test.c
    - [ ] cube
        - [x] simple 
        - [x] reset
        - [x] sync 
        - [x] render
        - [ ] add 
        - [ ] remove 
- [ ] main_camera_test.c
    - [ ] test camera base movement
    - [ ] input system
- [ ] main_lua_test.c


# Design:
  Note this is work in progress.

  This required a lot of thinking to make it module style design for raylib and other libraries will be added to make easy or hard to connect correctly. Which it will break api or module design and conflicts as well.

  By using the flecs to handle entities and raylib render and other things.

# bugs:
- sync object with ODE (Open Dynamics Engine) physics 3d.

# Credits:
- https://www.raylib.com/
- https://www.flecs.dev
- https://github.com/SanderMertens/flecs
- https://raylibtech.itch.io/rguilayout

