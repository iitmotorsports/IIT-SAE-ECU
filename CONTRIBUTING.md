# How To Contribute

## Before you begin

This repository is for students that are part of the SAE club at Illinois Tech.

If you are here then that probably means you have joined the club and are interested in programming the microcontrollers that the car will use.

This side of the software sub-team works closely with different electrical sub systems of the car. This includes things like the Motor controllers, Battery Management System (BMS), and the peripherals like the steering wheel and pedals.

Another major part of this project communicates with an Android phone app, which is currently being used as a dashboard for the car.
If you would like to take a look at the Android app instead, look for the appropriate repository on the [Illinois Tech Motorsports](https://github.com/Illinois-Tech-Motorsports) organization page on github.

The main part of this project is written in C++ and is setup to only compile on Windows. 
Various scripts used by the compilation process are written in python 3.

This project is best worked on when using [VSCode](https://code.visualstudio.com/) as all the tasks and formatting is taken care of through local settings.

Before continuing, be sure to follow the steps for setting up for compilation and running VSCode tasks in the [README](README.md).

## Familiarizing Yourself

If you are having trouble with anything here, first double check you read it correctly, and if that doesn't work, ask whoever is leading software, or someone that you think might know, to help you.

## The Environment

The following assumes you are using VSCode.

Here are some things that you should do which should help you familiarize yourself with functions, extensions, and tasks that are often used and to ensure that everything is working.

Although you already should have from the [README](README.md) setup, make sure you have also cloned the [TeensyToolchain](https://github.com/LeHuman/TeensyToolchain) repo, unziped it, and put it next to this project folder.

1. By default, The top left icon in VSCode shows the open editors and then files in your workspace. The one below that is source control.

    ![The source control tab](https://raw.githubusercontent.com/Illinois-Tech-Motorsports/IIT-SAE-ECU/master/Images/Contributing/VSIcons.png)

    We will not get into how [git](https://git-scm.com/) works, you should learn about that elsewhere. Assuming you understand Git, know that this is where you can commit changes to the repository without using a terminal.
   
   On the bottom taskbar in VSCode, towards the far left, there is a similar smaller icon that which should look like ![master](https://raw.githubusercontent.com/Illinois-Tech-Motorsports/IIT-SAE-ECU/master/Images/Contributing/master.png) , this is your current branch.
   
   For right now, it may be best that you work on a separate branch and create merge requests as you work on the repository. You can click on ![master](https://raw.githubusercontent.com/Illinois-Tech-Motorsports/IIT-SAE-ECU/master/Images/Contributing/master.png) which will show a popup to create a branch.

2. Run these [tasks](https://code.visualstudio.com/Docs/editor/tasks)* in VSCode
    * `Build`: Compile project
      * A Terminal should open up running various commands and ending with the statement `Task Succeeded ✔`
    * `Clean`: Clean up build files
      * Same result as the `Build` task
    * If you have access to two teensies** connected
      * Using the [settings.json](.vscode/settings.json) file, change the entires that are currently defined as `FRONT_TEENSY_PORT` and `BACK_TEENSY_PORT` to match the connect Teensy COM PORT. In windows, these COM PORTs show up in the [Device Manager](https://support.microsoft.com/en-us/windows/open-device-manager-a7f2db46-faaf-24f0-8b7b-9e4a6032fc8c). There should only be one port per Teensy. These ports should look like `COMx` where `x` is a number.

        ![What the COM PORTs might look like](https://raw.githubusercontent.com/Illinois-Tech-Motorsports/IIT-SAE-ECU/master/Images/Contributing/COMPORTS.png)
      * `Shebang Both`***: Run all tasks required to program both connected Teensies ( for the prompts, select `FRONT_TEENSY_PORT`, and `HEX` )
        * You should see the terminal printing log information from the front Teensy
3. Open the documentation file under the path `docs/index.html` on your computer, this should open up in your browser.
   * This will be useful later

\* By default, pressing the combo `Ctrl + lShift + P` then typing and selecting `Tasks: Run Task` should give you a list of all the tasks listed in the [tasks.json](.vscode/tasks.json) file. Alternatively, installing the [Tasks](https://marketplace.visualstudio.com/items?itemName=actboy168.tasks) extension shows commonly used tasks on the bottom task bar in VSCode.

![How the bottom task bar should look like](https://raw.githubusercontent.com/Illinois-Tech-Motorsports/IIT-SAE-ECU/master/Images/Contributing/Tasks.png)

These tasks are used to help automate development.

\** The project currently only supports the [Teensy 3.6](https://www.pjrc.com/store/teensy36.html), and will soon be updated to only use the [Teensy 4.0](https://www.pjrc.com/store/teensy40.html), make sure you use the correct board. You can also, instead, switch to an older version of the repository if you have an older board. The repository should have each season of SAE tagged.

\*** If you only have one Teensy, only setup the front one and instead run `Shebang Front`.

## The Code

Now that you have taken a look at what you will be *using* to program, lets look at *how* you should program.

### Coding Conventions

As stated before, this project mainly uses C++ and Python.

Both languages can be auto formatted by running the default combo of `lShift + lAlt + F`, otherwise pressing the combo `Ctrl + lShift + P` then typing and selecting `Format Document` should do the same. Note, however, that the style for python is less strict than for C++, the following only concerns when programming in C++.

 - No new lines after an open bracket, this reduces the need for vertical screen space. Also, try reducing used vertical space wherever else you can.
  ```C++
    If (true) { // Wrong ✖

        return;
    }

    If (true) { // Correct ✔
        return;
    }
  ```
 - Favor readability. Limit the use of one liners. This also helps when debugging problems.
  ```C++
    // Wrong ✖
    return object.command(otherObj.eval(input / 126f) * 255);

    // Correct ✔
    float fnlEval = otherObj.eval(input / 126f);
    return object.command(fnlEval * 255);
  ```
 - [X Macros](docs/html/_x_macros.html) should be defined in a `.def` file within the same library folder.
 - [Header guards](https://www.learncpp.com/cpp-tutorial/header-guards/) should follow the format `__ECU_X_Y__` where `X` is the file name and `Y` is the file extension ( as this can be used with `.h`, `.hpp`, and `.def` files), the entire string should also be in capitals.

Before committing changes, be sure you have at least auto formatted your file. You should, however, follow these guidelines as you are working on files.

### Modularity

A big idea behind this project is modularity. This means that if we want to program some new functionality it should be in the form of a library.

Each library should only focus on doing one thing and doing it well. They should not be hardcoded to fit our specific case, and should be as general as they can be. These libraies are stored in the `libraries` folder ( As expected ).
This library folder also stores third party libraries, those that we have taken from other repositories. Regardless, they work in the same way.

One caveat, however, is that some of the libraries do break this rule somewhat. For example, some libraries have it hardcoded to behave one way if they are on the front teensy and different otherwise. The file [ECUGlobalConfig.h](libraries/ECUGlobalConfig/ECUGlobalConfig.h) denotes these *hardcoded* values, in the sense that the libraries are directly dependent on our situation. That situation being that we are currently using two Teensies which behave diffrently. This issue will, *hopefully*, be soon dealt with.

### Python Scripting

The python portion of this project is somewhat neglected. As long as it seemed to work we just left it alone. This is especially true for the [Pre_Build.py](Pre_Build.py) script. Not much is to be said beyond this, feel free to look into the scripts and improve upon them as you see fit.

### Documentation

[Doxygen](https://www.doxygen.nl/index.html) is a program that is used to help generate documentation for this project. All of it's settings are in the [Doxyfile](docs/Doxyfile), refer to the [Doxygen](https://www.doxygen.nl/index.html) website for more info.

In regards to the libraries that we have written, all of their headers have been documented using [Doxygen](https://www.doxygen.nl/index.html). The extension [Doxygen Extensions](https://marketplace.visualstudio.com/items?itemName=Isaias.doxygen-pack) exist for VSCode. Use this extension to help easily create comments.

Very quickly, the way Doxygen works is by reading specifically formatted comments. You can look at examples throughout the project, but we will look at the file [State.h](libraries/State/State.h).

Note that towards the top there is a multiline comment, this gives Doxygen info about the file itself.
``` C++
/**
 * @file State.h
 * @author IR
 * @brief State library
 * @version 0.1
 * @date 2020-11-11
 * 
 * @copyright Copyright (c) 2020
 * ...
 */
```
The `@` symbol is used to denote what information you are giving, this is more important for functions.

Looking at the `notify` and `getLastState` function...

``` C++
    /**
     * @brief Send a code to the next state
     * 
     * @param notify code to send
     */
    void notify(int notify);

    /**
     * @brief Get a pointer of the last state
     * @note If the state machine has just started, this will return a nullptr
     * @return State_t* pointer of last state
     */
    State_t *getLastState();
```

`@brief` gives a brief description of the function
`@note` gives us the option is give an optional extra note
`@param` Tells us what the first parameter of the function should be
`@return` Tells us what this function should return

The extension should help with auto generating these symbols, just begin creating a multiline comment above a function and it should generate it for you.

If you wish to remove a part of code from documentation surround the code with the following comments. This is mostly used to omit the actual source files from documentation, as currently only header files are used with Doxygen.
``` C++
    // @cond
    int dontDocumentMe[4];

    void dontDocumentThis(void);

    void orThis(void);
    // @endcond
```

You do not have to compile and commit the documentation each time you add to it. However, to whomever ends up owning the repository next, make sure to update it every now and then.

## Further Reading

You will need to look through the documentation to get a more detailed description of what all the libraries do, as you should not work on something that you do not understand. This documentation attempts to explain a lot of how these underlying libraries work. Of course, as this project is, *hopefully*, expanded on, feel free to improve this documentation.

As a reminder, you can see the documentation by opening the file `docs/index.html`, which should open up in your browser.

I strongly recommend at least learning how logging works, as logging is used basically throughout this entire project, this is under `Classes > Class List > Log_t > Detailed Description`. Also, take a look at the `Pre_Build.py` file under the `See also` part, this is also a major part of how logging works.

Other commonly used libraries are the Canbus and Pins libs. These deal with communicating over Canbus and interacting the the GPIO pins respectively. Regardless, as you work on this project, hopefully you find this documentation to be helpful.

At this point ( especially if you have access to an Teensy ), attempt to first make some basic changes with the libraries. like use the logging library in the [Front::run](src/Front.cpp) function and make it print something out, or use the pin library to turn the in-built led on and off. Of course, don't actually commit these changes to the repository.

Actually, you can completely undo changes to a file in the source tab in VSCode before you commit. This is in the source control tab, in case you needed to know that.

![Discard changes to a file](https://raw.githubusercontent.com/Illinois-Tech-Motorsports/IIT-SAE-ECU/master/Images/Contributing/discard.png)

You can also look at all of the source files for examples on everything. But just as a quick example for logging.

The statements
```C++
    LOG_TAG BruhID = "Bruh";
    Log.w(BruhID, "Moment", 2);
```
Should print in the terminal ( while monitoring the teensy over serial )
> [Bruh] [WARN]  Moment 2

## Moving Forward

This repository was initially created with the intent that it would be used for multiple years after it's initial creation. Regardless, feel free to not use this repository and start something new. Use a new platform, use a new board, framework, whatever. Creating this repository from scratch taught us a lot as we are, after all, just students. In any case, this repository should work for the following years and its design should make it easy to add new functionality, so long as the teensy allows it.
