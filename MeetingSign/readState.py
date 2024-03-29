#!/opt/homebrew/bin/python3

from serial.tools import list_ports
from serial import Serial
import configparser
import os
import time
import sys

# Used to Colorize the state data
from sty import fg, bg, ef, rs, Style, RgbFg

DEBUG = False

LEDPWMSTEPS = 16

RESET = '\033[m'

# Branch if this respose is to be colorless or not...
try:
    if sys.argv[1] == "NOCOLORS":
        withColors = False
    else:
        withColors = True
except:
    withColors = True

def get_color_escape(vals):
    r = int(255*int(vals[0])/LEDPWMSTEPS)
    g = int(255*int(vals[1])/LEDPWMSTEPS)
    b = int(255*int(vals[2])/LEDPWMSTEPS)
    if withColors:
        if r == 0 and g == 0 and b == 0:
            return '\033[38;2;{};{};{}m.\033[m'.format(r, g, b)
        else:
            return '\033[38;2;{};{};{}mO\033[m'.format(r, g, b)
    else:
        if r == 0 and g == 0 and b == 0:
            return '.'
        else:
            return 'O'

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
    if DEBUG: print("I am attempting to read device: %s..."%(thisDevice))

try:
    # Setup and Write to the Serial Device
    serialIF = Serial(thisDevice, config.get('DEFAULT', 'arduinoSerialBaud'), timeout=0.5)
    
    # Send a read command, will get back 64 bytes
    serialIF.write('R.\r\n'.encode('raw_unicode_escape'))
    
    # Get the response...
    thisResponse = serialIF.readline().decode('UTF-8').split(",")
    if int(thisResponse[-1]) == 0:
        print(get_color_escape(thisResponse[ 0: 3]) +       
              get_color_escape(thisResponse[ 3: 6]) +       
              get_color_escape(thisResponse[ 6: 9]) +       
              get_color_escape(thisResponse[ 9:12]) +       
              get_color_escape(thisResponse[12:15]) +       
              get_color_escape(thisResponse[15:18]) +       
              get_color_escape(thisResponse[18:21]) +       
              get_color_escape(thisResponse[21:24]))
    else:
        print("Read of state failed.")
    
    # Close the IF
    serialIF.close()
except Exception as e:
    print(e)
    print("Reading the meeting sign through the USB interface failed...")
