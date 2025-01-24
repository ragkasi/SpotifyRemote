# Spotify Controller

This repository is for the Spotify Remote Controller. It uses an Arduino esp32 microcontroller and Arduino code.


## Features

Features include:

* Volume Control
* Pause/Play Functionality
* Song Info Display

# Spotify Library for ESP32 
This library is a wrapper for the [Spotify Web API](https://developer.spotify.com/documentation/web-api/) and is designed to work with the [ESP32](https://www.espressif.com/en/products/socs/esp32/overview) microcontroller. 

## Dependencies
- [ArduinoJson](https://arduinojson.org/) </br>
- [WiFiClientSecure](https://github.com/espressif/arduino-esp32/tree/master/libraries/NetworkClientSecure) </br>
- [base64](https://github.com/Densaugeo/base64_arduino) </br>
- [WebServer](https://github.com/espressif/arduino-esp32/blob/master/libraries/WebServer/src/WebServer.h) (Optional) </br>

## Installation

To install this project, do the following:

[Youtube Tutorial for the setup](https://www.youtube.com/watch?v=xNjbRq59dlc)</br>
1. Create a new application on the [Spotify Developer Dashboard](https://developer.spotify.com/dashboard/applications) and copy the Client ID and Client Secret into your code. Leave the developer dashboard open as you will need to set the callback url later. </br>

2. Clone the repository
```bash
git clone https://github.com/ragkasi/SpotifyRemote.git
```

3. Open code on Arduino IDE

4. Fill in pw.h.template file with the necessary information obtained from the previous steps, and your wifi connection
