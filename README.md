# ESP8266_stuff

The project currently works with SDK version 1.1.0 or later.

There was a problem with the wifi_handler in the SDK version 1.0.1,
where the EVENT_STAMODE_GOT_IP event does not actually set the ip
immediately. This caused issues when trying to connect to the MQTT broker
subsequently.
