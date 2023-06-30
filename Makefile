
install:
	arduino-cli core install esp32:esp32
	arduino-cli lib install IRremoteESP8266
	arduino-cli lib install ArduinoJson

build:
	arduino-cli compile -b esp32:esp32:esp32 -v

upload:
	arduino-cli upload -b esp32:esp32:esp32 -p /dev/ttyUSB0 -v
