# SDLRenderer3

This example takes from `example_sdlgpu3`. We need to generate bindings for SDLRenderer3 backend because they are not
native to `cimgui`. Then you can add the compiled library for linking in your application.

For the generation phase from cmake you need LuaJIT to be present.

## Building

From the build directory of your choice:

`cmake path_to_example_sdlgpu3`

Then simply run:

`make`

