SKETCH=Chapter9_advanced
#BOARD=esp32:esp32:adafruit_feather_esp32s2
#PORT=/dev/cu.usbmodem101
BOARD=esp32:esp32:esp32da
PORT=/dev/cu.usbserial-0001


all: compile upload

compile:
	clear ; arduino-cli compile -v -b ${BOARD} ${SKETCH}

upload:
	arduino-cli upload -v -p ${PORT} -b ${BOARD} ${SKETCH}

new:
	arduino-cli sketch new ${SKETCH}