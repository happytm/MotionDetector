
## Motion sensor fully based on the ESP32.

#### Based on excellent work by paoloinverse at: https://github.com/paoloinverse/bistatic_interference_radar_esp
- His library was modified to remove serial debug & serial parameter setting via serial to replace with settings via web interface.

#### TODO: https://neuton.ai/news/projects/75-tabular-data-vs-computer-vision-detecting-room-occupancy.html
           - https://github.com/happytm/arduino-room-occupancy
           
#### To play with the sensor:
  - Simply copy motionDetector folder to your Arduino library folder and compile motionDetector.ino file in Arduino IDE.
  - Upload data folder to ESP32 device.
  - Connect to AP called "ESP" and point your browser to 192.168.4.1.
  - Enter your WiFi settings first (ESP32 will reboot) and refresh the browser then enter motion sensor settings at the web interface.
  - Enjoy the graph build up with motion sensor data over time.
   
Do you really need to connect your ESP32 SoC to external movement detector sensors such as infrared PIR, ultrasonic or dedicated radar sensors, when the ESP32 itself can work as a passive radar *without any additional hardware* ?


Use the ESP32 wifi 802.11 radio as a bistatic radar based on multipath interference. Will detect human intruders when they cross the path between the ESP32 and other access points or wifi clients or freely move inside rooms / buildings at a reasonable distance from the ESP32.

Refer to the included .ino example, the basic usage is pretty simple. Feel free to experiment, this code works pretty well inside buildings. 

You will notice the strongest variations are when an intruder crosses the line-of-sight path between the ESP32 and the other access point or client used as transmitter. However, appreciable variations will be detected when indirect / reflected paths are crossed!

The example code outputs the signal over at built in web interface. Web interface also allows to set WiFi & motion sensor settings.
Most of the code is fully automated and preconfigured, although the core parameters can be modified by editing the .h file. 

Commented example plot follows:

![example_plot_01](https://user-images.githubusercontent.com/62485162/146927658-b540635e-16f6-4b56-b713-32469a1c8256.png)

This sensor is possible by a relatively complex digital filter entirely built in integer math, with a low computational expense.

Please note the code is mostly self-configuring and can autonomously take care of the common problems and failures typically encountered in a wifi based infrastructure, incuding faults affecting the nearby access points and stations, depending on the ESP32 operating mode. 

That being said, feel free to mess with the library internal parameters (such as buffer and filter sizes): if you find anything interesting and worth of notice, I'd be pleased to discuss it with you. 
