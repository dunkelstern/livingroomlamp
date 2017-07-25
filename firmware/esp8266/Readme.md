# Firmware for wifi-part of the living room lamp

This uses the [simplehttp library](https://github.com/dunkelstern/simplehttp-esp8266) to implement a web-server and builds a JSON API on top of it.

The HTTP library is contained in the `shttp` subfolder. The only custom part is in the `lamp` folder.

## HTTP API

TODO: Document this

## Building

Some pointers:

1. Get the ESP8266 compiler: https://github.com/pfalcon/esp-open-sdk
2. Get the SDK
3. Unpack SDK somewhere
4. Set up the environment
    ```bash
    export PATH="/path/to/compiler/xtensa-lx106-elf/bin:$PATH"
    export SDK_PATH="$HOME/esp-sdk/ESP8266_RTOS_SDK"
    export BIN_PATH="$HOME/esp-sdk/build"

    export CFLAGS="-I${SDK_PATH}/include -I${SDK_PATH}/extra_include $CFLAGS"
    export LDFLAGS="-L${SDK_PATH}/lib $LDFLAGS"
    ```
5. Switch to the base directory and run `make`
6. Grab the firmware image from `$BIN_PATH`
7. Flash with `esptool`:
    ```bash
    esptool.py --baud 115200 -p /dev/tty.usbserial-A1017UCG write_flash 0x00000 eagle.flash.bin 0x20000 eagle.irom0text.bin
    ```
    You will have to modify the tty device to suit your setup.

## Legal

License: 3 Clause BSD (see LICENSE-BSD.txt)