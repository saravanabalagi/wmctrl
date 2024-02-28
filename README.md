
# wmctrl
A command line tool to interact with an EWMH/NetWM compatible X Window Manager.

## Installation

To compile and install the program, run the following commands:

    ./configure
    make
    (become root)
    make install

After installation you may run "wmctrl -h" to view the documentation.

## Features Added:

The version here contains various enhancements that people around the world (yes, including me 😉) had added in their own divergent versions. The additions include these new command-line options/actions:

* -k toggle: Toggle show desktop
* -Y: iconify (Vadim Ushakov)
* -S: sort window list in stacking order (Vadim Ushakov)
* -j: list current desktop (Kevin Der)
* -r -y: like -e but reactivate after the move (Chris Piro)
* -E: get-title (Dan Corson)
* -z: lower window (Dan Corson)

The program is based on the [EWMH specification](https://specifications.freedesktop.org/wm-spec/wm-spec-latest.html). Please note that wmctrl only works with window managers which implement
this specification. You can find the docs for the original wmctrl [here](http://tripie.sweb.cz/utils/wmctrl/).

## Licence

Please refer to [COPYING](COPYING)
