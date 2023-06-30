
all: build upload

install:
	arduino-cli core install esp32:esp32
	arduino-cli lib install IRremoteESP8266
	arduino-cli lib install ArduinoJson

build:
	arduino-cli compile -b esp32:esp32:esp32da -v

upload:
	arduino-cli upload -b esp32:esp32:esp32da:UploadSpeed=460800 -p /dev/ttyUSB0 -v

monitor:
	@stty -F /dev/ttyUSB0 460800
	tail -f /dev/ttyUSB0
	#arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=460800
