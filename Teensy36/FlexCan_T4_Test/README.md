Teensy 3.X Project Template
===========================

Purpose
-------

An easy starting point for a Teensy 3.X project which might not fit inside the
arduino build environment.

Modified to be used under [WSL](https://docs.microsoft.com/en-us/windows/wsl/install-win10) and [VSCode](https://code.visualstudio.com/)

Note that this will not work under [VSCodium](https://vscodium.com/) as Microsoft wont let us for [now](https://code.visualstudio.com/docs/remote/faq#_why-arent-the-remote-development-extensions-or-their-components-open-source) :/

Reasons to Use
--------------

- You need to modify the teensy core
- You don't love Java IDE's
- You love Make
- Because

Setup
-----

Install the Teensy udev rule: `sudo cp tools/49-teensy.rules /etc/udev/rules.d/`

Then unplug your Teensy and plug it back in.

### VSCode Specific

Install and enable ( or enable per workspace ) the [Remote Extension Pack](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.vscode-remote-extensionpack)

When using with WSL and VSCode cd into the folder and then run `code .`

Also run `sudo chmod 777 -R folder-name` where `folder-name` is the name of the entire folder, I was running into permission issues with WSL

Set the baudrate and port that should be used to upload/monitor in `.vscode/settings.json`

This template *should* still work on pure linux, however, the current CLI tools will not

Using
-----

1. Put your code in `src/main.cpp`
2. Put any libraries you need in `libraries`
3. Set the TEENSY variable in `Makefile` according to your teensy version
4. Build your code ```make```
5. Upload your code ```make upload```

### VSCode Tasks

* `Clean`: Run `make clean`
* `Build`: Run `make build`
* `Stop Monitor`: Run a CMD command to stop the monitor ( VS does not close it automatically )
* `Reboot`: Connect to teensy at 134 Baud to reboot it
* `Upload`: Run `make upload` with WSL Mode enabled
* `Monitor`: Run monitor program to read serial data

By default, `Ctrl + Shift + B` (Run Build Task) should run all of the above in order except for `Clean`

Make Targets
------------

- `make` alias for `make hex`
- `make build` compiles everything and produces a .elf
- `make upload` uploads the hex file to a teensy board
- `make clean` clean up build files

Where everything came from
--------------------------

- The `teensy` sub-folder is taken from a [Teensyduino](http://www.pjrc.com/teensy/td_download.html) installation from the arduino install directory `hardware/teensy`
- The `tools` sub-folder is also taken from the same [Teensyduino](http://www.pjrc.com/teensy/td_download.html) directory
- The `src/main.cpp` file is moved, modified from `teensy/avr/cores/teensy3/main.cpp`
- The `Makefile` file is moved, modified from `teensy/avr/cores/teensy3/Makefile`
- The `49-teensy.rules` file is taken from [PJRC's udev rules](http://www.pjrc.com/teensy/49-teensy.rules)
- The `ComMonitor.exe` file is from [ComMonitor-CLI](https://github.com/LeHuman/ComMonitor-CLI)
- The `teensy_loader_cli.exe` file is the [Teensy Loader Command Line](https://www.pjrc.com/teensy/loader_cli.html) compiled for windows

Modifications to `Makefile` include
- Add support for arduino libraries
- Change tools directory
- Calculate target name from current directory
- Prettify rule output
- Do not upload by default, only build
- Other modifications which might have broken that arduino part
- Usage of the teensy cli loader
