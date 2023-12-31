
# default values
PORT = /dev/ttyUSB0
SPEED = 460800

# some stuff are platform dependent
SYSTEM = $(shell sh -c "uname | tr '[:upper:]' '[:lower:]'")
ifeq ($(SYSTEM),darwin)
	PORT = $(shell sh -c "ls /dev/cu.usbserial-*")
endif

all: build upload

install:
	arduino-cli core install esp32:esp32
	arduino-cli lib install IRremote
	arduino-cli lib install ArduinoJson

build:
	arduino-cli compile -b esp32:esp32:esp32da -v

clean:
	arduino-cli compile --clean -b esp32:esp32:esp32da -v

upload:
	-killall -q arduino-cli
	arduino-cli upload -b esp32:esp32:esp32da:UploadSpeed=${SPEED} -p ${PORT} -v
	make monitor

monitor:
	arduino-cli monitor -p ${PORT} -c baudrate=${SPEED}
