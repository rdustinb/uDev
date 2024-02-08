#!/opt/homebrew/bin/python3

from pyicloud import PyiCloudService
from pyicloud.exceptions import PyiCloudFailedLoginException
from datetime import datetime,date,timedelta
from serial.tools import list_ports
from serial import Serial
import sys
import time
import config
import os

DEBUG=False
ENABLE_SERIAL_UPDATE=True
ENABLE_CALENDAR_FETCH=False

# Check the manual configuration, if enabled, don't change it
if config.manual:
    print("Meeting sign was manually enabled, quitting automatic script...")
    sys.exit()

# Check if the configuration has quiet times
try:
    # Get the Start time from config.py
    start_time_today_string = datetime.strptime("%s %s"%(
        datetime.today().strftime('%Y-%m-%d'),
        datetime.strptime(config.startTime, '%H:%M').strftime('%H:%M')
        ), '%Y-%m-%d %H:%M')
    
    # Get the End time from config.py
    end_time_today_string = datetime.strptime("%s %s"%(
        datetime.today().strftime('%Y-%m-%d'),
        datetime.strptime(config.endTime, '%H:%M').strftime('%H:%M')
        ), '%Y-%m-%d %H:%M')

    currentTime = datetime.now()

    # Compare Config quiet times
    if currentTime > start_time_today_string and currentTime < end_time_today_string:
        # Disabled by default, no need for else here...
        ENABLE_CALENDAR_FETCH=True
    
except:
    print("Configured quiet time stamps don't make sense, defaulting to fetch...")
    # If the time in config doesn't make sense, just enable the fetching...
    ENABLE_CALENDAR_FETCH=True

# Fetch Calendar Events and Update the Sign if enabled...
if ENABLE_CALENDAR_FETCH:
    try:
        api = PyiCloudService(config.myIcloudEmail, config.myIcloudPassword)
    except PyiCloudFailedLoginException as e:
        # When the authentication token is expired...
        print("Reauthentication of the iCloud account is needed...")
        os.system("icloud --username=%s"%config.myIcloudEmail)
        api = PyiCloudService(config.myIcloudEmail, config.myIcloudPassword)
        print("Continuing of the calendar fetching...")
    
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
        if DEBUG: print("%s, %s, %s"%(nextEventStart, nextEventEnd, currentTime))
        if(currentTime > nextEventStart and currentTime < nextEventEnd):
            print("We are in a calendar event, enabling LED, breaking loop!")
            # If we've found one instance, stop scanning the events
            ledOn = True
            break
else:
    ledOn = False
    print("Quiet Time, disabling the sign...")
    
if ENABLE_SERIAL_UPDATE:
    # Get the Serial USB Device
    port = list(list_ports.comports())
    for p in port:
      if(p.usb_description() == 'USB Serial'):
        thisDevice = p.device
        print("I am attempting to update device: %s..."%(thisDevice))
    
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
    
        # Close the IF
        serialIF.close()

        # Print that the update finished successfully
        print("Last updated: %s"%(datetime.now()))
    except Exception as e:
        print(e)
        print("Updating the meeting sign through the USB interface failed...")
