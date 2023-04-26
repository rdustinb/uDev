from pyicloud import PyiCloudService
from datetime import datetime,date,timedelta
from serial.tools import list_ports
from serial import Serial
import time
import config
import os

DEBUG=False
ENABLE_SERIAL_UPDATE=True

#os.system("icloud --username=%s"%config.myIcloudEmail)
api = PyiCloudService(config.myIcloudEmail, config.myIcloudPassword)

ledOn = False # OFF by default...

# Create the calendar object
calendarService = api.calendar

# Fetch all the calendars
#allCals = calendarService.get_calendars(as_objs=True)

# Get the Work Calendar GUID
#workCalGUID = ''
#for thisCal in allCals:
#  if thisCal.title == 'Work':
#    workCalGUID = thisCal.guid
#    break

# Get the events for today
my_from_dt = datetime.fromtimestamp(datetime.timestamp(datetime.today()))
my_to_dt   = datetime.fromtimestamp(datetime.timestamp(datetime.today()))
for thisEvent in calendarService.get_events(from_dt=my_from_dt, to_dt=my_to_dt):
  if(thisEvent['pGuid'] == config.myCalendarpGuid): # workCalGUID):
    # Set the start time to 5 minutes before the actual calendar event
    nextEventStart = datetime(*thisEvent['startDate'][1:6]) - timedelta(minutes=config.startTimeOffset)
    # End time is exactly at the end of the calendar event
    nextEventEnd = datetime(*thisEvent['endDate'][1:6])
    currentTime = datetime.now()
    if DEBUG: print("%s, %s, %s"%(nextEventStart, nextEventEnd, currentTime))
    if(currentTime > nextEventStart and currentTime < nextEventEnd):
        if DEBUG: print("We are in a calendar event, enabling LED, breaking loop!")
        # If we've found one instance, stop scanning the events
        ledOn = True
        break
    #print("I found this event name:")
    #print(thisEvent['title'])
    #print("Which starts at this time:")
    #print("%02d:%02d"%(thisEvent['startDate'][4],thisEvent['startDate'][5]))
    #print("and ends at this time:")
    #print("%02d:%02d"%(thisEvent['endDate'][4],thisEvent['endDate'][5]))

#'startDate': [20230322, 2023, 3, 22, 18, 0, 1080],
#'endDate': [20230322, 2023, 3, 22, 20, 0, 240]

if ENABLE_SERIAL_UPDATE:
    # Get the Serial USB Device
    port = list(list_ports.comports())
    for p in port:
      if(p.usb_description() == 'USB Serial'):
        thisDevice = p.device
    
    # Setup and Write to the Serial Device
    serialIF = Serial(thisDevice, config.arduinoSerialBaud, timeout=0.5)
    
    # For this update, set the LED based on the calendar events checked above...
    if ledOn:
        serialIF.write('A020116.\r\n'.encode('raw_unicode_escape'))
    else:
        serialIF.write('A000000.\r\n'.encode('raw_unicode_escape'))
    
    # Close the IF
    serialIF.close()

