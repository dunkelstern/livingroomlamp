# iPhone app for living room lamp

This is the ultra simple app for the living room lamp.

Known bugs:

- The color wheel is a subtractive color wheel, it should be a additive wheel -.-
- The app rotates but the content does not fit on the screen in landscape format, have to disable roation
- The IP of the ESP module is hardcoded, needs a setting or MDNS support on ESP module
- The parameters are reset on app start so they do not reflect the current lamp state (while there is a API endpoint for fetching the parameters)
- The API endpoint was to slow, so the App lags a bit
- The current color is not indicated on the color wheel

## Legal

License: 3 Clause BSD (see LICENSE-BSD.txt)