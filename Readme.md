# My Livingroom lamp

This is the git repo for my living room lamp.

**Electronic Components:**

- 12 High power 1W LEDs for bright ilumination
- 104 NeoPixels (SK6812) with RGB and Warm White LEDs
- 15 Low power 4 mm Cold white LEDs as an emergency light
- Arduino Nano for PWM and NeoPixel timing
- ESP8266 for WIFI functionality (will be upgraded to provide a HomeKit device in future)
- 5V 50W Meanwell power supply
- Lots and lots of wire

For wiring schematics see the `schematics` subfolder.

**Mechanical Components:**

- 60mm by 600mm by 0.75mm aluminium or steel sheet (to make the Hexagon shape and act as a heat sink for the LEDs)
- Some aluminium extrusions to fix everything together
- 500mm by 500mm by 4mm acrylic sheet as the diffusion and anti glare shield (bottom of the lamp, the only part that is visible directly)
- A lot of nuts and bolts

For CAD drawings (no 3D printed parts this time as the lamp will get quite warm to the point PLA would soften) see the `mechanical` subfolder.

**Software Components:**

- Arduino sketch (see `arduino` subfolder)
- ESP8266 firmware, based on FreeRTOS and my HTTP library (see `esp8266` subfolder)

For firmware look no further as into the `firmware` subfolder.

## Legal

License: 3 Clause BSD (see LICENSE-BSD.txt)