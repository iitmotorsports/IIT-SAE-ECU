SAE at IIT ECU Programming
===========================

<img src="https://raw.githubusercontent.com/Illinois-Tech-Motorsports/IIT-SAE-ECU/master/Images/41.png" alt="Teensy 4.1" width="512">

This repository contains the source code that the SAE team at IIT uses for their microcontrollers.

New Members
-----------

For new members that are just joining, please refer to the [CONTRIBUTING](CONTRIBUTING.md) file.

Hardware
-------

- [Teensy 3.6](https://www.pjrc.com/store/teensy36.html) x 2
- [A Windows machine](https://en.sdjmd.com/) (Preferably Win 10)
- [A Formula Hybrid Car](https://www.f1authentics.com/f1-racing-cars-for-sale/) (Not necessary for testing)
- [A Compatible CAN transceiver](https://www.amazon.com/SN65HVD230-CAN-Board-Communication-Development/dp/B00KM6XMXO) x 2

Software
-----

This project assumes you installed the latest version of the following.

- [VSCode](https://code.visualstudio.com/)
  - This template is based around VSCode
- [Python](https://www.python.org/downloads/)
  - At least python 3.10.x
  - Ensure python files are executed by default when opening them in a CLI
- [CMake](https://cmake.org/download/)
  - Make sure to select the `add CMake to path` option when installing
- [Git](https://git-scm.com/download)
  - For version control of this repository

Setup
-----

Clone [TeensyToolchain](https://github.com/LeHuman/TeensyToolchain) alongside this folder.

If it is not named already, rename the cloned/downloaded folder to `TeensyToolchain`  
e.g. `TeensyToolchain-master` -> `TeensyToolchain`

Alternatively, you can modify the `TOOLCHAIN_OFFSET` option inside `.vscode/settings.json` in order to tell the project where the toolchain is. This path is relative to this folder.

Using
-----

Main logic is either in `src/ECUStates.cpp` or `src/Front.cpp`  
Put any libraries / modules in the `libraries` folder

### VSCode Tasks

- `VS setup`: Configure things such as what `COM` ports to use; both for front and back teensy, whether to use the plotter app, and whether to compile for teensy `3.6`, `4.0`, or `4.1`.
- `Clean`: Clean up build files
- `Reset`: Clean up entire CMake project and reconfigure CMake
- `Build`: Compile project and generate binaries
- `Upload`: Upload compiled binary to either front or back teensy
- `Monitor`: Monitor teensy over a CLI, select the appropriate options when running, defaults to `Mapped` mode

By default, `Ctrl + Shift + B` (Run Build Task) should run `Build` then `Upload` for both Teensies and then `Monitor`.

Alternatively, install the [Tasks](https://marketplace.visualstudio.com/items?itemName=actboy168.tasks) extension so that the main tasks appear on the status bar.

There are more tasks but these are the most important ones.

### CMake

You can modify the set values and compile flags inside `CmakeLists.txt` if you know what you are doing.  
They are mostly under the function calls `add_compile_definitions` and `add_compile_options` towards the beginning.

Documentation
-------------

This project uses [Doxygen](https://www.doxygen.nl/index.html) to auto generate documentation.

[Doxygen Extensions](https://marketplace.visualstudio.com/items?itemName=Isaias.doxygen-pack) exist for VSCode, to easily compile and view inside VSCode.

The documentation can be generated locally or you can view the auto generated [Github Page](https://illinois-tech-motorsports.github.io/IIT-SAE-ECU/) for this repo.
