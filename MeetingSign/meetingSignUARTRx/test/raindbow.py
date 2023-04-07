import config
import time
from serial.tools import list_ports
from serial import Serial

# Get the Serial USB Device
port = list(list_ports.comports())
for p in port:
  if(p.usb_description() == 'USB Serial'):
    thisDevice = p.device
    
# Setup and Write to the Serial Device
serialIF = Serial(thisDevice, config.arduinoSerialBaud, timeout=0.5)

# Red
serialIF.write('S00160000.\r\n'.encode('raw_unicode_escape'))
time.sleep(1)
# Orange
serialIF.write('S02160100.\r\n'.encode('raw_unicode_escape'))
time.sleep(1)
# Yellow
serialIF.write('S04160300.\r\n'.encode('raw_unicode_escape'))
time.sleep(1)
# Green
serialIF.write('S06000800.\r\n'.encode('raw_unicode_escape'))
time.sleep(1)
# Aquamarine
serialIF.write('S07000801.\r\n'.encode('raw_unicode_escape'))
time.sleep(1)
# Blue
serialIF.write('S05000016.\r\n'.encode('raw_unicode_escape'))
time.sleep(1)
# Purple
serialIF.write('S03050016.\r\n'.encode('raw_unicode_escape'))
time.sleep(1)
# Violet
serialIF.write('S01160016.\r\n'.encode('raw_unicode_escape'))
time.sleep(1)

# Close the IF
serialIF.close()
