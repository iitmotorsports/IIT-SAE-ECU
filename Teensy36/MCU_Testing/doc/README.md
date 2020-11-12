SAE ECU Programming
===========================
 
Hardware
-------

- [Teensy 3.6](https://www.pjrc.com/store/teensy36.html) x 2
- [A Windoes machine](https://www.microsoft.com/en-us/windows) (Preferably Win 10)
- [A Formula Hybrid Car](https://bicyclewarehouse.com/collections/hybrid-bikes) (Not necessary for testing)
- [A Compatible CAN transceiver](https://www.amazon.com/SN65HVD230-CAN-Board-Communication-Development/dp/B00KM6XMXO) x 2

Setup
-----

Install the Latest Release version of [CMake](https://cmake.org/download/)

As of today, that would be version 3.18.4

Inside `.vscode/tasks.json`, modify the options for `TEENSY_USB_PORTNAME` for the ports that the project should use to connect to your teensy. ( Set to COM6 by default )

You can also modify the defaults for other tasks.

Incase you wish to place the project folder elsewhere, you can modify `TOOLCHAIN_OFFSET` inside `.vscode/settings.json` in order to tell the project where the toolchain is. The path must be relative.
This path must also be modified inside the first line of `CmakeLists.txt`.

Using
-----

Put your code in `src/main.cpp`
Put any libraries you need in `libraries`

You can also modify the set values and compile flags inside `CmakeLists.txt` if you know what you are doing.
They are mostly under the function calls `add_compile_definitions` and `add_compile_options` towards the beginning.

### VSCode Tasks

* `Clean`: Clean up build files
* `Hard Clean`: Clean up entire CMake project
* `Build`: Compile project
* `Upload`: Upload compiled binary to teensy
* `Monitor`: Monitor teensy over a CLI, select the appropriate options when running

By default, `Ctrl + Shift + B` (Run Build Task) should run all of the above in order except for `Clean` and `Hard Clean`

Alternatively, install the [Tasks](https://marketplace.visualstudio.com/items?itemName=actboy168.tasks) extension so that tasks can instead be on the status bar. Already configured.

Where everything came from
--------------------------

### ARM Toolchain
- The folders with the names `arm-none-eabi`, `bin`, and `lib` all come from the arduino install directory `hardware/tools/arm`
### Teensy Core
- The `teensy` sub-folder is taken from a [Teensyduino](http://www.pjrc.com/teensy/td_download.html) installation from the arduino install directory `hardware/teensy`
### Tools
- `ComMonitor.exe` is from [ComMonitor-CLI](https://github.com/LeHuman/ComMonitor-CLI)
  - More info about usage on it's repository
- `teensy_loader_cli.exe` is the [Teensy Loader Command Line](https://www.pjrc.com/teensy/loader_cli.html) compiled for Windows
- `ninja.exe` is the [Ninja Build System](https://github.com/ninja-build/ninja) binary for Windows