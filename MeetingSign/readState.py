#!/opt/homebrew/bin/python3

from serial.tools import list_ports
from serial import Serial
import configparser
import os
import time

# For updating the configuation file:
configFile = os.path.join(os.path.dirname(os.path.realpath(__file__)), "config.ini")

# Read in the current configuration
config = configparser.ConfigParser()
config.read(configFile)

# Get the Serial USB Device
port = list(list_ports.comports())
for p in port:
  if(p.usb_description() == 'USB Serial'):
    thisDevice = p.device
    print("I am attempting to read device: %s..."%(thisDevice))

try:
    # Setup and Write to the Serial Device
    serialIF = Serial(thisDevice, config.get('DEFAULT', 'arduinoSerialBaud'), timeout=0.5)

    # Send a read command, will get back 64 bytes
    serialIF.write('R.\r\n'.encode('raw_unicode_escape'))

    # Get the response...
    thisResponse = serialIF.readline().decode('UTF-8').split(",")[:-1]
    print(thisResponse)
    print(len(thisResponse))

    # Close the IF
    serialIF.close()
except Exception as e:
    print(e)
    print("Reading the meeting sign through the USB interface failed...")

# Decode what the colors are?
