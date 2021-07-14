# Sensorify-embedded

## Welcome

You need to define the following variables:
```
const char* ssid     = "FBI Survaillance Van #420";
const char* password = "supersecretwifipassword";

const char* host       = "wut.example.com";
const uint port        = 80;
const char* privateKey = "supersecretapikey";
const char* schoolName = "Fiktivna Škola 37";
const char* deviceName = "Sensei 23";
```
either in `config.h` or above `setup()` while removing `#include "config.h"`