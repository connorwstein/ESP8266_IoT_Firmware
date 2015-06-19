# ESP8266_stuff
This branch has been made in order to support the other ESP chips
we have that don't work with the newest SDK. Currently they
only work with SDK v0.9.5.

Our code uses some API functions which where not available in this SDK,
so we had to modify it to work with the older SDK.

INCOMPATIBILITIES:
*	wifi_set_event_handler_cb is not available in the older SDK, so we had
	to implement our own limited event handling.

*	system_init_done_cb is not available in the older SDK, so we simply
	call init_done at the end of user_init.

NOTES:
	We had issues with writing to flash. We couldn't save the device config,
	and we couldn't auto-connect on restart (network settings weren't being
	written to flash). It turns out the problem was that the Amica flasher
	was placed on a defective breadboard, and some pins were getting shorted.
	Also, we needed a newer version of the Node MCU flasher (esptool.py wouldn't
	work because of some different flashing settings).

	We removed the chip from the breadboard, and updated the NodeMCU flasher.
	To upload the firmware, create a shared folder in VirtualBox as follows:

	Devices -> Shared Folders Settings... -> Add shared folder definition.
	Folder path = C:\Users\<user name>\Documents\ESP_firmware
	Click on Auto-Mount.

	In the VM, sudo mkdir /mnt/share. Then type
		sudo umount -t vboxsf ESP_firmware /mnt/share

	You can then copy the bin files from the firmware/ directory to /mnt/share.

	Load these files in the NodeMCU flasher (in the Config tab). Flash them one by one.

	After following these steps, you should be able to get the code working.
