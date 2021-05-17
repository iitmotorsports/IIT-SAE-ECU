@page XMacros X Macros

# X Macros

This Page helps explain how [X Macros](https://www.geeksforgeeks.org/x-macros-in-c/) are used in this project.

There are resources linked that can also explain their usage however this page hopes to explain their usage in this project.

The basic idea behind X Macros in this project is that, using the C preprocessor, we can essentially
generate code by just using definitions.


@section XMacros_Creating_an_X_Macro Creating an X Macro


In this project, X Macros have two parts, the defining part, and the generating part.


@subsection XMacros_Defining Defining


The defining part mostly occurs in `.def` files, the X macro definition can look as such.

``` C
#define NAME_OF_X_MACRO \
X(45, "Some String")    \
X(67, "Another string") \
X(32, "sTRiNg")
```

If you are inexperienced with macros, know that the `\` at the end of each line just means that this is a multi-line macro.

This means the final macro actually will look like this.

``` C
#define NAME_OF_X_MACRO X(45, "Some String") X(67, "Another string") X(32, "sTRiNg")
```

@warning When making an X Macro multiline, you need to make sure that the last line does not
have a `\`, as the `\` tells the preprocessor that there is another line it should append
to the same define, just take note of that on the example `NAME_OF_X_MACRO`.

Each line after the define must call a non existent macro simply called `X`.

Technically it does not have to be called `X` but that is what this project uses.

Note that each line gives the same number of arguments and those arguments also are of the same data type respectively.

Using the right generating techniques, the number of arguments and their data types would
not matter, however this project does do that.


@subsection XMacros_Generating Generating


Using `NAME_OF_X_MACRO`, we can interact with it's calls to `X` in many different ways.

We know that the first parameter of each macro is an integer, and the second a string.
Say we want to log each of these macros. Using Log_t it would look as such.

``` C
void printAll(){
#define X(number, string) Log.i("ID", string, number);
    NAME_OF_X_MACRO
#undef X
}
```

Note that we first define `X`, call the `NAME_OF_X_MACRO` macro, and then undefine `X` with `#undef`.

In the preprocessor, `NAME_OF_X_MACRO` will expand as follows.

``` C
void printAll(){
#define X(number, string) Log.i("ID", string, number);
    X(45, "Some String") 
    X(67, "Another string")
    X(32, "sTRiNg")
#undef X
}
```

Because the macro `X` exists after we have defined it, each line will now expand to whatever X is as such.

``` C
void printAll(){
#define X(number, string) Log.i("ID", string, number);
    Log.i("ID", "Some String", 45);
    Log.i("ID", "Another string", 67);
    Log.i("ID", "sTRiNg", 32);
#undef X
}
```

So essentially, after the preprocessor, the final code before compilation will look like this.
``` C
void printAll(){
    Log.i("ID", "Some String", 45);
    Log.i("ID", "Another string", 67);
    Log.i("ID", "sTRiNg", 32);
}
```

And, as you can hopefully see, this means we can generate a lot of code using just some input parameters.


@section XMacros_Caveats Caveats


The following sections discuss generation techniques used in this project that may be confusing to some.


@subsection XMacros_XMacro_Counter X Macro Counter


One technique that is used throughout this project that may be confusing is this method of generation.

``` C
#define X(...) ,
static const int LOG_COUNT = PP_NARG_MO(NAME_OF_X_MACRO);
#undef X
```

Using our previous definition of `NAME_OF_X_MACRO`, `LOG_COUNT` will expand to 3.

This is done by using the macro `PP_NARG_MO`, which simply counts how many parameters are passed to it.
This macro is defined in PPHelp.h.

However, to the preprocessor, each parameter technically does not need to actually exists, simply
putting a `,` means that there is a parameter.

Therefore, the next expansion of `LOG_COUNT` would look as such

``` C
#define X(...) ,
static const int LOG_COUNT = PP_NARG_MO(,,,);
#undef X
```

This tells the preprocessor that it should pass 4 arguments and `PP_NARG_MO` counts this. Also note, however, that we have
three calls to `X`, not 4. There will always be one extra, so `PP_NARG_MO` ensures that it removes one each time this happens.

Note however of a limitation, `PP_NARG_MO` technically is not *counting*, the only thing one must know is that `PP_NARG_MO`
currently has a limit of `64` parameters. This can be increased however I am too lazy and I also have not ran into any issues with this limit.


@subsection XMacros_Concatenated_Calls Concatenated Calls


Mainly used in Pins.cpp, this generation method uses [macro concatenation](https://gcc.gnu.org/onlinedocs/gcc-3.0.1/cpp_3.html#SEC18)
to *build* the name of a function to then call it.

This example is part of the source code for Pins::getPinValue()

```
0   #define __READPIN_DIGITALINPUT(PIN) \
1       }                               \
2       else if (GPIO_Pin == PIN) {     \
3           return digitalReadFast(PIN);
4   
5   #define __READPIN_ANALOGINPUT(PIN) \
6       }                              \
7       else if (GPIO_Pin == PIN) {    \
8           return analogRead(PIN);    \
9   
10  #define __READPIN_DIGITALOUTPUT(PIN)
11  #define __READPIN_ANALOGOUTPUT(PIN)
12  
13  ...
14  
15  int getPinValue(uint8_t GPIO_Pin) {
16      if (GPIO_Pin >= 100) {
17          return getCanPinValue(GPIO_Pin);
18  #define X(pin, Type, IO, init) __READPIN_##Type##IO(pin);
19          ECU_PINS
20  #undef X
21      } else {
22  #ifdef CONF_ECU_DEBUG
23          Log.d(ID, "No pin defined", GPIO_Pin);
24  #endif
25          return 0;
26      }
27  }
```

Note that the `...` just means there is other code in between the parts we are looking at that we do not care about for now.

The main definition to take note of is `ECU_PINS` on line `19`.

the X Macro definition on line `18` takes in a pin number, a type, whether this pin is
input or output, and the initial value of the pin.

An example call to `X` is as follows

``` C
X(45, DIGITAL, INPUT, NIL)
```

Pin 45 will be read as digital input, with no initial value as it is input.

Out current definition of `X` at line `18` would interpret this as follows

``` C
    // Using X(pin, Type, IO, init) __READPIN_##Type##IO(pin);
    // Input X(45, DIGITAL, INPUT, NIL)
    __READPIN_DIGITALINPUT(45);
```

Which, looking at line `0`, would be expand to 

``` C
    } else if (GPIO_Pin == PIN) {     
        return digitalReadFast(PIN);
```

Where Pins::getPinValue(), after the preprocessor, would end up looking like this

``` C
int getPinValue(uint8_t GPIO_Pin) {
    if (GPIO_Pin >= 100) {
        return getCanPinValue(GPIO_Pin);
    } else if (GPIO_Pin == PIN) {     
        return digitalReadFast(PIN);
    } else {
        return 0;
    }
}
```

If our call to `X` was instead this

``` C
X(10, ANALOG, OUTPUT, 0)
```

It would expand to `__READPIN_ANALOGOUTPUT(10)`

However, because we do not read from output pins, `__READPIN_ANALOGOUTPUT()` is an
empty macro, meaning nothing is done inside Pins::getPinValue()