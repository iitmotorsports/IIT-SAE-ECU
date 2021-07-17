SAE at IIT ECU Programming
===========================
 
Hardware
-------

- [Teensy 3.6](https://www.pjrc.com/store/teensy36.html) x 2
- [A Windows machine](https://www.microsoft.com/en-us/windows) (Preferably Win 10)
- [A Formula Hybrid Car](https://bicyclewarehouse.com/collections/hybrid-bikes) (Not necessary for testing)
- [A Compatible CAN transceiver](https://www.amazon.com/SN65HVD230-CAN-Board-Communication-Development/dp/B00KM6XMXO) x 2

Setup
-----

NOTE: This repo used [TeensyTemplate](https://github.com/LeHuman/TeensyTemplate) as a starting point

Download [TeensyToolchain](https://github.com/LeHuman/TeensyToolchain), unzip, and put it next to this folder

Rename the downloaded folder from `TeensyToolchain-master` to `TeensyToolchain`

Alternatively, you can modify `TOOLCHAIN_OFFSET` inside `.vscode/settings.json` in order to tell the project where the toolchain is. The path must be relative.

Install the Latest Release version of [CMake](https://cmake.org/download/)

As of today, that would be version 3.18.4

Inside `.vscode/tasks.json`, modify the options for `FRONT_TEENSY_PORT` and `BACK_TEENSY_PORT` for the ports that the project should use to connect to your teensy.

You can also modify the defaults for other tasks.

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
* `Monitor`: Monitor teensy over a CLI, select the appropriate options when running, defaults to `HEX` mode

By default, `Ctrl + Shift + B` (Run Build Task) should run all of the above in order except for `Clean` and `Hard Clean`

Alternatively, install the [Tasks](https://marketplace.visualstudio.com/items?itemName=actboy168.tasks) extension so that tasks can instead be on the status bar. Already configured.

Documentation
-------------

This project uses [Doxygen](https://www.doxygen.nl/index.html) to auto generate documentation.

[Doxygen Extensions](https://marketplace.visualstudio.com/items?itemName=Isaias.doxygen-pack) exist for VSCode, to easily compile and view inside VSCode.

`index.html` can be found inside the docs folder or you can also view the ( somewhat broken ) Github Page for this repo

New Members
-----------

For new members that are just joining, please refer to the [CONTRIBUTING](CONTRIBUTING.md) file.