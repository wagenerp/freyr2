# freyr2 LED animation framework
## Summary

Designed to be used as a centralized animation service providing data to any number of LED devices via a number of *egress modules* (network or otherwise).

* Modular design. The core framework provides just enough features to manage LED animations, egress modules and custom application modules to be instantiated at run time. 
  * Static / run-time dynamic loading. Chosen at compile-time, either build a monolithic binary with all modules included or load (and re-load!) modules at run-time, enabling animation development while the animator runs!
* ESP-32 firmware. Utilizing static modules, the entire framework can run compile on ESP-32 (C++20 support required) to provide a standalone animation solution.
  * Web-based interface. Connect any WiFi and web browser capable device for an instant user interface to your portable LED device.
* Runtime command system. (Re)Configure the entire setup, control and switch animations at run-time
* mqtt command module: Interface with any freyr2 instance on your network, including interface descriptions for autocompletion of commands.


## Building and running freyr2 on linux (animation service)

Freyr2 builds using 
  * cmake (3.5 or higher)
  * C++20 / C23
  * python 3 (3.9 or higher)
  * alpha4 library (submodule!)

Modules may bring additional requirements. Currently:
  * `mod_mqtt`: Requires mosquittopp

The core framework has no dependencies other than alpha4 which is provided as source-level inclusion via:

    git submodule update --init

Then build freyr the usual way:

    mkdir -p build; cd build
    cmake ../
    make 

If working with dynamic modules (default), freyr by default looks for modules in the `modules` directory. At least one module named `mod_bootstrap.so` must be available as it is loaded automatically. Without this there is no way to load any other modules ore interface with freyr other than to terminate by signal.

Having built freyr2, you can run it with an example configuration like this:

    ./freyr -l ../examples/terminal.cfg
   


### Compile options

* Static linkage. If you wish to include all modules in a single monolithic binary, dispable dynamic modules with `-DOPT_DYNAMIC=OFF`
* Gprof output. Profiling can be enabled simply with `-DOPT_GPROF=ON`
* Module selection. For each module (animation, egress, etc.) an option is created with `MODULE_` prefix and all caps (e.g. `mod_coordinates.cpp` yields `MODULE_MOD_COORDINATES`). Disable any module you wish to exclude with `-DMODULE_<NAME>=OFF`
      



## Building freyr2 for esp-32 (standalone device)

You can compile freyr2 as part of a firmware for the popular esp-32 microcontroller. A sample firmware is included in `firmware/esp32-freyr/`.
To build you need
  * A working ESP-IDF installation
    * Version 5 using GCC 11 should work fine.
    * By the time this was written EDP-IDF 5 was not released yet, I instead cloned master branch on commit 9b8c558e63d95b491355a22e69f247834e899b47 and built the compiler from tag esp-2022r1-dev (commit 27482258d5b061e0899fb665630a354c238980b1).
  * python 3 (3.9 or higher)
    * If you wish to use the web interface, additional python packages are required:
      * rjsmin, rcsmin, xmltodict, htmlmin
      * Or just fiddle with `webif/armcomp/generate.py` to remove these modules.

Not all modules are supported by ESP32 and have been disabled:
  * `mod_input_stdin`, `egress_console`, `egress_upsilon-striped`, `mod_mqtt`

During compilation a web interface is minimized and compressed, you can find this under `webif/armcomp/` and adjust to your own design. The firmware itself is configured by adjusting the `firmware/esp32-freyr/main/device.cpp`.

To build the firmware follow the normal esp-idf cmake build flow. Having sourced your esp-idf files, run:

    cd firmware/esp32-freyr/; mkdir -p build; cd build
    cmake .. 
    make 
    make flash


## Modification and contribution

### Core framework overview
There is a single main file (`src/main/freyr.cpp`) initializing the core framework, running main loop and handling command-line arguments as well as instantiating the bootstrap module.

The core framework in turn provides features to all modules (animations, egress modules, application modules) via api headers (C interface only!):
  * `src/core/animation_api.h`: Defines entry points of *animation* modules and low-level animation handling functions.
  * `src/core/basemodule_api.h`: Low-level access to module system.
  * `src/core/egress_api.h`: Defines entry points of *egress* modules and low-level egress module handling.
    * The egress API also notifies all interested parties of changes to the configured LEDs via *hooks*.
  * `src/core/frame_api.h`: Access to the actual LED data to be filled by animations and emitted by egress modules.
  * `src/core/module.h`: Defines entry points to all application modules and the command / hook handling API.


Main loop execution is split into two sections: Animation rendering and synchronization.
During animation rendering, any number of threads (currently only one is supported) execute the `iterate` method for any visible animations, operating on the raw buffer provided by `frame_raw_anim()` (`frame_api.h`). Any other core API must not be used from within the `iterate` methods.

Synchronization handles 
  * Each application module's `flush` method. 
  * Application of global filters (the `applyFilters` hook)
  * Transmission of LED data via corresponding egress modules.
  * Regulating delay to achieve a stable frame rate.

An application module may access API functions *only* during synchronization - i.e. their `init`, `deinit` and `flush` methods. In particular, modules listening for external input asynchronously (e.g. `mod_input_stdin.cpp` or `mod_mqtt.cpp`) must buffer this input and apply it in their `flush` methods.

Finally, *egress* modules are expected never to call any core API other than accessing LED data via `frame_raw_egress()`.

### Existing modules

* `mod_bootstrap`: Always the first module to be loaded, provides commands for module instantiation and basic features. Without this, no configuration commands are available.
* `mod_coordinates`: Provides spatial data for every LED in existence (follows egress module init/deinit to allocate buffers). This can be configured via `coordinate_set` command and accessed from animations by `coordinates_raw_anim()`. Any animation with the `-s` suffix uses coordinates (spatial animations) and thus this module *must* be loaded before these animations are used.
* `mod_grouping`: Maintains lists of named LED groups for addressing them e.g. when assigning animations. LEDs are grouped via `group_add` by addressing them via their egress module's instance name, offset and count.
* `mod_input_stdin`: Read standard input line by line and interpret them as commands (as if they were provided as part of a configuration file via `-l` command-line argument).
* `mod_mqtt`: Interface with an MQTT server, listening for command inputs and providing description on available commands. This is where modules' and commands' `describe` entry points are used - these make use of the "UNified Interface Co-ordination Notation" (UnICOrN)
* `mod_streams`: Some egress modules require additional information on the LEDs (e.g. color channel ordering and color depth). These are provided in a run-length encoding fashion via `mod_streams`.
* `egress_console`: Outputs a rectangular grid of LEDs via 24 bit ansi color codes on the current terminal. Can be redirected into a file (e.g. another terminal's input file descriptor via procfs).
* `egress_dummy`: Dummy output, not actually displaying LEDs.
* `egress_upsilon-striped`: Transmits LED data via UDP using the striped upsilon stream transfer protocol (e.g. used by the upsilon FPGA design (coming soon)). This requires streams to be set up for all LEDs it handles.
* `mod_display`: Provides the `display` and (and other) commands used for controlling what animations are displayed. Supports overlayed animation tiers and blending of animations.
* `mod_filter_brightness`: Hooks into `applyFilters` to provide a per-pixel brightness scale.
* `mod_filter_overlay`: Hooks into `applyFilters` to provide an alpha-blended overlay for each pixel.

The following are special modules loaded and maintained by the `mod_display` module to achieve and expose its animation blending effect.
* `blend_fade`: Perform uniform alpha blending between two animations.
* `blend_wipe`: Send a plane through 3d space (requires `mod_coordinates`) transitioning between two animations



### Creating a new animation
To get started on a new animation, you can simply copy an existing one (e.g. `anim_rainbow-s.c`) and its entry in the `src/modules/CMakeLists.txt` list) and recompile. The animation name is derived from source file without extension and `anim_` prefix (which is mandatory!).
You can then view your animation by issuing command (e.g. by adjusting an example configuration or typing it to stdin)
   
    display <your-animation> on all

If you make changes to the animation and wish to re-load it, you can have the (dynamic) module unloaded and re-loaded:

    float all
    display <your-animation> on all

Simply follow patterns laid out in existing animations and everything should work fine. In particular, do not call any core API functions other than `frame_raw_anim`.
