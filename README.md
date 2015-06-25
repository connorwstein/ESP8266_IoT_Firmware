# ESP8266_stuff

The project currently works with SDK version 1.1.0 or later.

There was a problem with the wifi_handler in the SDK version 1.0.1,
where the EVENT_STAMODE_GOT_IP event does not actually set the ip
immediately. This caused issues when trying to connect to the MQTT broker
subsequently.


REQUIREMENTS:
We use the esp_mqtt library to connect to an MQTT broker.
The library is available on github at http://github.com/tuanpmt/esp_mqtt.

Our project links against the library. To do so, first build esp_mqtt by
cloning the esp_mqtt repo and following the README.md instructions there.

Then go into the build/mqtt/ directory, and run:

$ xtensa-lx106-elf-ar cru libmqtt.a \*.o

Move the libmqtt.a file into the lib/ directory of our project folder.
