#!/opt/homebrew/bin/python3

from datetime import datetime
from serial.tools import list_ports
from serial import Serial
import sys
import config

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
    with open("config.py", "r") as fh_r:
        for thisLine in fh_r:
            if thisLine.find("manual") != -1:
                thisConfig.append("manual=True\n")
            else:
                thisConfig.append(str(thisLine))
else:
    with open("config.py", "r") as fh_r:
        for thisLine in fh_r:
            if thisLine.find("manual") != -1:
                thisConfig.append("manual=False\n")
            else:
                thisConfig.append(str(thisLine))

# Store the new manual configuration
with open("config.py", "w") as fh_w:
    for thisLine in thisConfig:
        fh_w.write(thisLine)

# Update the sign manually
if ENABLE_SERIAL_UPDATE:
    # Get the Serial USB Device
    port = list(list_ports.comports())
    for p in port:
      if(p.usb_description() == 'USB Serial'):
        thisDevice = p.device
    
    try:
        # Setup and Write to the Serial Device
        serialIF = Serial(thisDevice, config.arduinoSerialBaud, timeout=0.5)
    
        # For this update, set the LED based on the calendar events checked above...
        if ledOn:
            serialIF.write('M0016000002160100041603000600080007000801050000160305001601160016.\r\n'.encode('raw_unicode_escape'))
        else:
            serialIF.write('A000000.\r\n'.encode('raw_unicode_escape'))

        # Get the response...
        if int(serialIF.read(2).decode('utf-8')) != 0:
            print('uController Failure response.')
            raise ValueError('uController Failure response.')
        #print(int(serialIF.read(2).decode('utf-8')) == 0)
    
        # Close the IF
        serialIF.close()

        # Print that the update finished successfully
        print("Last updated: %s"%(datetime.now()))
    except:
        print("Updating the meeting sign through the USB interface failed...")
