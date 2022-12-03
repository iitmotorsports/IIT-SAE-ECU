# SDBC syntax

Version 1.0.0

## Global syntax

- First token on a line indicates what syntax to follow
- Any text after a final `:` in a line is considered the description of that line. If there is no final `:` there is no description
- IDs must be unique to their namespace
- Indents are not necessary but are recommended to format the document appropriately
- The order in which lines are defined is important
- IDs must typically be uppercase with no spaces but underscores allowed. However, text case shall be converted to upper regardless.

## Top level lines

- `NAME`
- `VERSION`
- `NODE`
- `FORMAT`
- `TYPE`

## Name

> NAME "name here"

This shall be the first line in the file. It indicates the name of the SDBC file. It shall match the name of the file, omitting the extension.

## Version

> VERSION 1.0.0

This shall be the second line of the file. It indicates what version of the SDBC standard it follows.
Semantic versioning shall be used with no extensions.

## Nodes

> NODE `node_id` : `optional description`
> > ADDR `can_address_low` - `can_address_high`  
> > GPIO `gpio_pin_id` : `pin#` `[DIGITAL|ANALOG]` `[I|O]` : `optional description`  
> > VIRT `virt_pin_id` : `[DIGITAL|ANALOG]` : `optional description`  
> > MSG `can_msg_address`  `[<<|>>]` `can_msg_id` : `optional description`  
> > > SIG `signal_id` : `"format_name"` `[<<|>>]` `[virt_pin_id|gpio_pin_id]` : `optional description`  

Where `pin#` is an integer.

Nodes are used to organize where CAN messages originate from and to define pins that are in use. Defining pins is not necessary, such as with a motor controller or AMS, where there is no embedded logic. Nodes must be part of the CAN network. SDBC assumes that there is only one network.

### `NODE`

The `NODE` line consists of a unique ID for this node, and the optional description. It is important that the name matches that of the build system's if the node is related to the pins defined i.e. the ECUs.

### Sources

#### `ADDR`

The `ADDR` line optionally defines a CAN bus address space that should be allocated to this node. This means no other nodes can use any addresses within this space. Any sources within this node that use an address outside of this range should not cause error but should show a warning and be added to the address space, where applicable. It shall not be allowed for any source or address space to overlap in address.

The first address is the lower bound while the second is the upper bound.

If no address space is defined, only the refrenced addresses in the node are used.

Addresses can either be in hex (begins with `0x`) or decimal.

#### `GPIO`

The `GPIO` line defines a physical GPIO pin that shall be in use. It consists of a unique ID, the pin number, whether this pin should be treated as `DIGITAL` or `ANALOG`; which changes whether it should be treated as a single bit or multiple, whether this pin is exclusively for input (`I`) or output (`O`), and the optional description.  
NOTE: pin will not be verified if it is necessarily usable as digital or analog, refer to board pin-out to confirm it has digital, analog read and/or write capabilities.

In this context, input means that the physical pin can only be read and output means that the physical pin can only be set.

#### `VIRT`

The `VIRT` line defines a virtual pin that is mainly used to communicate M2M but treated as a pin to simplify the API. It consists of a unique ID, whether this pin should be treated as `DIGITAL` or `ANALOG`; which changes whether it should be treated as a single bit or multiple, and the optional description.

Virtual pins shall be defined on the node of the initial value or whichever node generates the value, as it can technically be defined anywhere in the network.

In this context, input means that the value can only be read and output means that the value can only be set.

Because there is no pin given, the usage of this pin must verify it has a unique or alternative way to modify the value compared to the set GPIO lines.

#### `MSG`

The `MSG` line defines a CAN bus message, where it contains `SIG`s or signals, which are the values within that message. It consists of the can bus address for the message, whether this message is outgoing `<<` or incoming `>>`, the unique id for this message, and the optional description.

Addresses can either be in hex (begins with `0x`) or decimal.

There are can be multiple signals per message, however, it shall be verified at processing whether the sigs are able to fit in the message's 64 bit buffer. The order in which sigs are defined in a message dictates the structure of the buffer. i.e. if a message is defined with an `int`, and two `short`s, in that order, the actual buffer will be sent where the first 32 bits are an `int` and the next two 16 bit blocks are `short`s, where it all totals to 64 bits.

##### `SIG`

The `SIG` line consists of the signal id, the format of this signal, optionally, the id of the value that should be linked to this signal using a value setter `<<` or `>>`, dependent on whether this message is outgoing or incoming, respectively; this keeps the signal up to date with the given source whether it is from a gpio or virt source, and the optional description.

Signals linked with a value setter (`<<` or `>>`) are to be updated in the background at runtime. The header for formatting the incoming values will be generated but the actual logic must be then later defined in a separate source file.

**NOTE:** `GPIO`s can only be set to, at most, an unsigned 32 bit integer value while `VIRT`s are doubles.

**NOTE:** If more than one value setter is used in an incoming message, space **must** be allocated for a bit field, denoting which value setter should actually be updated. The bit field is as large as however many value setters there are and is appended to the end of the message block. This is checked for at parsing.

##### Space optimization

Although not required, effort should be made to optimize the placement of signals within a message.

This can be checked for at parsing.

Values should be optimized to be placed at appropriate integer intervals, where possible. i.e. every 32 bits for 32 bit values, every 16 bits for 16 bit values, or every 8 bits for 8 bit values. This is best achieved by placing values largest to smallest, where applicable.

## Formats

> FORMAT `"format_name"` `"data_type"` (`scale`,`shift`) [`min`,`max`] : `optional description`

The `FORMAT` line consists of the name of the format in quotes, the data type of this format, the scale and shift that should be applied to the final value when decoding a can message signal. The min and max of the final value, if applicable and the optional description

scale, shift, min, and max are allowed to be floats and negative. They are also have defaults that should be used if they are not to be used, as such `(1,0)` or `[0,0]`

## Types

> TYPE `"data_type_name"` `[+|-]` `[0-64]` : `optional description`

The type line consists of the name of a data type, whether this data is signed or unsigned, the number of bits that it uses, and the optional description.

The data type name, if applicable, shall match a defined type or typedef in it's most general sense. i.e. 32 bit values can be either an integer (`int32_t`), unsigned integer (`uint32_t`) or a float (`float`), 1 bit values are booleans (`bool`).

## Embedded compilation

For this section, a 'source' is defined as a `GPIO`, `VIRT`, or `SYNC` line

For use in the embedded build system, here are things that must happen.

Nodes that contain logic (defined sources), will generate defines for those sources that match their id, they are defined based for what node the build is for. Other id's will have a prefix with the id of it's respective owner.

Any sources or `MSG` that have a sync refrenced to them, must output to the network whenever they change.

synced values should be packed into messages as needed, i.e. if a node has two values that are being synced, send both in the same message versus sending them in separate messages.

`MSG`s append their signals to a nodes namespace, not the message name itself.

`MSG`s that are synced must append all of their signals to the receiving node's namespace, as if it were it's own.
