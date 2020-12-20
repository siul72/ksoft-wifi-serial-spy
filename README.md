# ksoft-wifi-serial-spy

## Goals
The goal of the project is to capture RS485 data, publish it over wifi to an MQTT borker
## Architecture
```Mermaid
graph LR; A(RS485 Capture)-->B(MQTT Publish); B-->D(Wifi); D-->E(MQTT Broker);
 ````
## Hardware
WeeMos D1 board family:
 ![WeeMos D1 board](https://i1.wp.com/www.esp8266learning.com/wp-content/uploads/2016/03/Wemos_D1_ESP8266.jpg?w=514)
This board is based on ESP8266 soc for wifi communication.
![enter image description here](https://imgaz3.staticbg.com/thumb/large/upload/2012/lidanpo/SKU108443%20%285%29.JPG.webp)
Max485 chip - ttl to rs485 converter
# Tools and software description
 To build and flash the code to your board use [https://platformio.org/](https://platformio.org/)
 The details of the pin setup is also on the code.
 The functionality works using Tasks in order to execute the code assyncrosly.
 There are 3 Major tasks:
 ## Wifi Task
 The task that handle the connection to the wifi network, currently there are some hardcoded parts, meaning that you need to addujst the SSID and the credentials to connect to the wifi access point.
 ## MQTT Task
 MQTT Task handles the mqtt client connection to the broker, as soon as the wifi is connected
 ## Serial Data Task
 This task waits for data in the serial port and try to send it over MQTT

Further questions contact me:
luis.coelho.720813@gmail.com
