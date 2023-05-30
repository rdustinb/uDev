#!/bin/bash

# Unload the Agent so that the directed call of the script doesn't collide with
# an outstanding request and bork everything...
launchctl unload ~/Library/LaunchAgents/meetingSign.plist

# Get rid of the logfile...
rm -fr ~/Library/LaunchAgents/meetingSign.log

# Run the script soas to reauthenticate if needed...
python3 fetchWorkCalEvents.py

# Reload the Agent for automated checks...
launchctl load ~/Library/LaunchAgents/meetingSign.plist
