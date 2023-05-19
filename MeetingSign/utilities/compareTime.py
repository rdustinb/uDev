from datetime import datetime
# Custom Config.py file with stuff in it
import config

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

if currentTime > start_time_today_string and currentTime < end_time_today_string:
    print("Fetch iCloud calendar stuff.")
else:
    print("Off hours, not checking...")
