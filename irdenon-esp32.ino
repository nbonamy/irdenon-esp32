#include <Arduino.h>
#include <SPIFFS.h>
#include "irdenon-ir.h"
#include "irdenon-led.h"
#include "irdenon-http.h"
#include "irdenon-wifi.h"

void setup()
{
  // serial
  Serial.begin(460800);
  Serial.println("");
  Serial.println("[MAIN] Starting");

  // start spiffs
  SPIFFS.begin(true);

  // ir stuff
  initIr();
  loadIr();

  // connect to wifi
  connectWifi();

  // led
  initLed();
  ledOff();

  // start server
  startHttpServer();
}

void loop()
{
  server.handleClient();
  // if (Serial.available()) {
  //   String command = Serial.readStringUntil('\n');
  //   Serial.println("[MAIN] Received command: " + command);
  //   if (command == "reboot") ESP.restart();
  // }
}
