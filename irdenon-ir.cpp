#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include "irdenon-ir.h"

#define PROTOCOL_RAW "raw"
#define PROTOCOL_DENON "denon"
#define PROTOCOL_PANASONIC64 "panasonic64"
#define PROTOCOL_KASEIKYO "kaseikyo"
#define PROTOCOL_DEFAULT PROTOCOL_RAW

IRsend irsend(4); // D2
DynamicJsonDocument irCodes(32768);

void sendMulti(String protocal, JsonArray data, uint16_t repeat);

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
  String protocol = PROTOCOL_DEFAULT;
  JsonObject command = irCodes["commands"][commandId];
  JsonArray data = command["data"].as<JsonArray>();
  if (command.containsKey("repeat")) {
    repeat = command["repeat"];
  }
  if (command.containsKey("protocol")) {
    protocol = command["protocol"].as<String>();
  }

  // now send
  sendMulti(protocol, data, repeat);

  // done
  return true;
}

void sendRaw(JsonArray data, uint16_t repeat)
{
  // convert
  uint16_t buffer[data.size()];
  for (int i=0; i<data.size(); i++) {
    buffer[i] = data[i].as<uint16_t>();
  }

  for (int i = 0; i < repeat; i++) {
    irsend.sendRaw(buffer, data.size(), 38);
  }
}

uint64_t dataToInt64(JsonArray data)
{
  uint64_t res = 0;
  for (int i=0; i<data.size(); i++) {
    uint64_t byte = data[i].as<uint64_t>();
    res += byte << (7-i)*8;
  }
  return res;
}

void sendMulti(String protocol, JsonArray data, uint16_t repeat)
{
  // log
  Serial.print("[ IR ] Sending " + protocol + ": ");
  Serial.print(data.size());
  Serial.print(" bytes / ");
  Serial.print(data.size() * 8);
  Serial.print(" bits / ");
  Serial.print(repeat);
  Serial.println(" time(s)");

  // led
  digitalWrite(LED_BUILTIN, LOW);

  // do it
  if (protocol == PROTOCOL_RAW) {
    sendRaw(data, repeat);
  } else if (protocol == PROTOCOL_DENON) {
    irsend.sendDenon(dataToInt64(data), data.size()*8, repeat-1);
  } else if (protocol == PROTOCOL_PANASONIC64 || protocol == PROTOCOL_KASEIKYO) {
    irsend.sendPanasonic64(dataToInt64(data), data.size()*8, repeat-1);
  }
  
  // led
  digitalWrite(LED_BUILTIN, HIGH);
}
