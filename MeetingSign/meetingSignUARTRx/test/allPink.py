import config
from serial.tools import list_ports
from serial import Serial

# Get the Serial USB Device
port = list(list_ports.comports())
for p in port:
  if(p.usb_description() == 'USB Serial'):
    thisDevice = p.device
    
# Setup and Write to the Serial Device
serialIF = Serial(thisDevice, config.arduinoSerialBaud, timeout=0.5)

serialIF.write('A320102.\r\n'.encode('raw_unicode_escape'))
    
# Close the IF
serialIF.close()
