#!/opt/homebrew/bin/python3

from datetime import datetime
from serial.tools import list_ports
from serial import Serial
import sys
import configparser
import os

# For updating the configuation file:
configFile = os.path.join(os.path.dirname(os.path.realpath(__file__)), "config.ini")

ENABLE_SERIAL_UPDATE=True

try:
    if sys.argv[1] == "On":
        ledOn = True
    else:
        ledOn = False
except:
    print('No argument given to manual sign update, exiting.')
    ledOn = True
    sys.exit()

# Update the config to block automatic sign updates...
thisConfig = list()
if ledOn:
    with open(configFile, "r") as fh_r:
        for thisLine in fh_r:
            if thisLine.find("manual") != -1:
                thisConfig.append("manual = True\n")
            else:
                thisConfig.append(str(thisLine))
else:
    with open(configFile, "r") as fh_r:
        for thisLine in fh_r:
            if thisLine.find("manual") != -1:
                thisConfig.append("manual = False\n")
            else:
                thisConfig.append(str(thisLine))

# Store the new manual configuration
with open(configFile, "w") as fh_w:
    for thisLine in thisConfig:
        fh_w.write(thisLine)

# Read in the current configuration
config = configparser.ConfigParser()
config.read(configFile)

# Update the sign manually
if ENABLE_SERIAL_UPDATE:
    # Get the Serial USB Device
    port = list(list_ports.comports())
    for p in port:
      if(p.usb_description() == 'USB Serial'):
        thisDevice = p.device
        print("I am attempting to update device: %s..."%(thisDevice))
    
    try:
        # Setup and Write to the Serial Device
        serialIF = Serial(thisDevice, config.get('DEFAULT', 'arduinoSerialBaud'), timeout=0.5)
    
        # For this update, set the LED based on the calendar events checked above...
        if ledOn:
            serialIF.write('M0016000002160100041603000600080007000801050000160305001601160016.\r\n'.encode('raw_unicode_escape'))
        else:
            serialIF.write('A000000.\r\n'.encode('raw_unicode_escape'))

        # Get the response...
        if int(serialIF.read(2).decode('utf-8')) != 0:
            print('uController Failure response.')
            raise ValueError('uController Failure response.')
    
        # Close the IF
        serialIF.close()

        # Print that the update finished successfully
        print("Last updated: %s"%(datetime.now()))
    except Exception as e:
        print(e)
        print("Updating the meeting sign through the USB interface failed...")
