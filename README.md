# ESP8266_stuff
This branch has been made in order to support the other ESP chips
we have that don't work with the newest SDK. Currently they
only work with SDK v0.9.5.

Our code uses some API functions which where not available in this SDK,
so we had to modify it to work with the older SDK.

# INCOMPATIBILITIES:
*	wifi_set_event_handler_cb is not available in the older SDK, so we had
	to implement our own limited event handling.

*	system_init_done_cb is not available in the older SDK, so we simply
	call init_done at the end of user_init.
