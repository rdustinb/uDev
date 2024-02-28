#!/opt/homebrew/bin/python3

from serial.tools import list_ports
from serial import Serial
import config

ENABLE_SERIAL_READ=True

# Update the sign manually
if ENABLE_SERIAL_READ:
    # Get the Serial USB Device
    port = list(list_ports.comports())
    for p in port:
      if(p.usb_description() == 'USB Serial'):
        thisDevice = p.device
        print("I am attempting to read device: %s..."%(thisDevice))
    
    try:
        # Setup and Write to the Serial Device
        serialIF = Serial(thisDevice, config.arduinoSerialBaud, timeout=0.5)
    
        # Send a read command, will get back 64 bytes
        serialIF.write('R.\r\n'.encode('raw_unicode_escape'))

        # Get the response...
        thisResponse = list()
        for thisByte in range(34):
            thisResponse.append(serialIF.read().decode('utf-8'))
        print(thisResponse[:-2])
    
        # Close the IF
        serialIF.close()
    except Exception as e:
        print(e)
        print("Reading the meeting sign through the USB interface failed...")
