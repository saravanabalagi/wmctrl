
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

The program is based on the [EWMH specification](http://www.freedesktop.org/standards/wm-spec). Please note that wmctrl only works with window managers which implement
this specification. You can find the docs for the original wmctrl [here](http://tripie.sweb.cz/utils/wmctrl/)

## Licence

Copyright (c) 2019 Saravanabalagi Ramachandran

- Not available for commercial use
- Absolutely no warranty provided

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
