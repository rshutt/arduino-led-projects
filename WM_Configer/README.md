# WM_Configer

This module wraps up autoconfigure portal and dupe reset detection to allow the user to manage ESP8266's wherein the portal comes up if no config exists or double reset has happened.  

Drop these files into `~/Documents/arduino/libraries/WM_Configer` and include with `#include <WM_Configer.h>`

Then in your main `.ino` file, use the following:

Global scope - `class WM_Configer * myWMConfiger;`

In setup() after Serial init - 
```
  myWMConfiger = new WM_Configer();

  myWMConfiger->config_tree();
```

In loop() near top -
```
  myWMConfiger->dodrdloop();
```

# Warning - ESP_WiFiManager...

[This](https://github.com/khoih-prog/ESP_WiFiManager/blob/master/README.md#howto-fix-multiple-definitions-linker-error) is some irritating crap, but I had to replace the `src` dir with `src_cpp` contents per the howto.  What a mess...


