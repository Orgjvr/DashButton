# DashButton
This is a IOT button which sends a MQTT message with the VCC voltage when pressed. Optimised for battery use.

I run it on an ESP-01 (actually an ESP8266 with the first form factor). It needs minimal connections, and works great from a 3.7V LiPo battery with an inline diode to drop the voltage a bit to ensure the safety of the ESP8266. 

I have multiple of these buttons in my home, and a cheap no-name 18650 battery last more than a year. Of course if you are going to do thousands of button presses, the battery will need replacement sooner. I do have some stats about the amount of button presses, but I need to analyse it, as the original version of the code, went into an infinite loop if it could not connect to either the Wifi, or the MQTT server. This depleted the batteries in no amount of time :) The newer version, will limit the amount of attempted connections, and if it could not connect, go back to sleep.

Todo:
  Add some photos of the development button.
  Get info of amount of button presses.
  
