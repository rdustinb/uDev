= uController UART Commands =

There are two packet commands and structures which can be used to update the LEDs driven from the microcontroller.

'All' command, which sends one set of RGB color values that the microcontroller assigns to all the RGB LEDs.

'Multiple' command, which allows individual LEDs to have unique RGB values defined in one packet. This command structure allows for as many or as few LEDs to be altered at a time.

== Command Fields ==

All - A<RRGGBB>.\r\n

Multiple - M<LLRRGGBB><LLRRGGBB><...><LLRRGGBB>.\r\n

Where LL is the LED index number assigned the immediately-following RGB values.
Where RR is the red value, GG is the green value, and BB is the blue value.
Values have a range of 0 (the LED leg is driven with a PWM of 0%) to 16 (the LED leg is driven with a PWM of 100%).

== Command Examples ==

The following command will set all the LEDs to an Aquamarine color.

`serialIF.write('A010816.\r\n'.encode('raw_unicode_escape'))`

The following command will set an array of 8 RGB LEDs to have each color of the rainbow.

`serialIF.write('M0016000002160100041603000600080007000801050000160305001601160016.\r\n'.encode('raw_unicode_escape'))`

= macOS Scripts =

== Files ==

fetchWorkCalEvents.py - This is the main script which polls a particular iCloud calendar for events and updates the Arduino uController sign.
config.py.example - This is the configuration file which contains local information about your iCloud account and which calendar to use and such. Rename this to config.py in the same folder as fetchWorkCalEvents.py.
meetingSign.plist.example - This is the Launch Agent registration file, which should be filled out for your system and renamed to meetingSign.plist. Once it is filled out, it should be copied to: 

`~/Library/LaunchAgents/`

and loaded in to the LaunchD system with the command: 

`$ launchctl load ~/Library/LaunchAgents/meetingSign.plist`

The Launch Agent, once loaded, will persist through reboots, but maybe not full OS updates.

== iCloud Authentication ==

Let me first say that it appears like the authentication "resets" every time a mac is rebooted. So if you have 2FA setup on your account, you will have to rerun the authentication before the script can get access to your calendar. The best way to do this is to unload the Launch Agent, then running $ icloud --username=your.email.account@icloud.com, to get the system to repull the authentication token from iCloud. Making sure to unload the Launch Agent FIRST will prevent the Lanch Agent from stomping on your terminal command.

I am looking at how to better handle this...

