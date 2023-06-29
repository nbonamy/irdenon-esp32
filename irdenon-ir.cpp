#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include "irdenon-ir.h"

IRsend irsend(4); // D2
DynamicJsonDocument irCodes(32768);

void sendMulti(JsonArray data, uint16_t count);

void initIr()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  irsend.begin();
}

void loadIr()
{
  // check it exists
  File file = SPIFFS.open("/ircodes.json", FILE_READ);
  if (file.isDirectory()) {
    Serial.println("[ IR ] ircodes.json not found");
    file.close();
    return;
  }

  // load json
  DeserializationError error = deserializeJson(irCodes, file);
  if (error) {
    Serial.print("[ IR ] Error while parsing ircodes.json: ");
    Serial.println(error.c_str());
  }

  // done with file
  file.close();

  // log
  Serial.print("[ IR ] ");
  Serial.print(irCodes["commands"].size());
  Serial.println(" commands loaded");
}

bool sendIr(String commandId)
{
  // check it exists
  if (!irCodes["commands"].containsKey(commandId)) {
    Serial.println("[ IR ] No IR code for " + commandId);
    return false;
  }

  // get the command
  int repeat = 1;
  JsonObject command = irCodes["commands"][commandId];
  JsonArray data = command["data"].as<JsonArray>();
  if (command.containsKey("repeat")) {
    repeat = command["repeat"];
  }
  sendMulti(data, repeat);

  // done
  return true;
}

void send(uint16_t buffer[])
{
  digitalWrite(LED_BUILTIN, LOW);
  irsend.sendRaw(buffer, 95, 38);
  digitalWrite(LED_BUILTIN, HIGH);
}

void sendMulti(JsonArray data, uint16_t count)
{
  // log
  Serial.print("[ IR ] Sending ");
  Serial.print(data.size());
  Serial.print(" bytes ");
  Serial.print(count);
  Serial.println(" time(s)");

  // convert
  uint16_t buffer[data.size()];
  for (int i=0; i<data.size(); i++) {
    buffer[i] = data[i].as<uint16_t>();
  }

  // do it
  for (int i = 0; i < count; i++) {
    send(buffer);
  }
}
