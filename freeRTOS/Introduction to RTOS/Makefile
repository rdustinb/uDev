THISCHAPTERPATH=Chapter8_hw
#THISBOARD=esp32:esp32:adafruit_feather_esp32s2
#THISPORT=/dev/cu.usbmodem101
THISBOARD=esp32:esp32:esp32da
THISPORT=/dev/cu.usbserial-0001


all: compile upload

compile:
	clear ; arduino-cli compile -v -b ${THISBOARD} ${THISCHAPTERPATH}

upload:
	arduino-cli upload -v -p ${THISPORT} -b ${THISBOARD} ${THISCHAPTERPATH}