#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <IRremote.h>
#include "irdenon-ir.h"
#include "irdenon-led.h"

#define PROTOCOL_RAW "raw"
//#define PROTOCOL_DENON "denon"
#define PROTOCOL_KASEIKYO "denon_kaseikyo"
#define PROTOCOL_DEFAULT PROTOCOL_RAW

#ifndef IR_SEND_PIN
#define IR_SEND_PIN 5
#endif

//IRsend irsend(IR_SEND_PIN);
DynamicJsonDocument irCodes(32768);

void sendMulti(String protocal, JsonArray data, uint repeat);

void initIr()
{
  IrSender.begin(IR_SEND_PIN);
  //irsend.begin();
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

  // led
  ledOn();

  // get the command
  uint repeat = 0;
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

  // led
  ledOff();

  // done
  return true;
}

void sendRaw(JsonArray data, uint repeat)
{
  // log
  Serial.print("[ IR ] Sending raw: ");
  Serial.print(data.size());
  Serial.print(" bytes / ");
  Serial.print(data.size() * 8);
  Serial.print(" bits / ");
  Serial.print(repeat+1);
  Serial.println(" time(s)");

  // convert
  uint16_t buffer[data.size()];
  for (int i=0; i<data.size(); i++) {
    buffer[i] = data[i].as<uint16_t>();
  }

  // send
  for (int i=0; i<repeat+1; i++) {
    IrSender.sendRaw(buffer, data.size(), 38);
  }
}

void sendDenonKaseikyo(JsonArray data, uint repeat)
{
  // get data
  uint16_t address = data[0];
  uint8_t command = data[1];

  // log
  Serial.print("[ IR ] Sending DenonKaseikyo: ");
  Serial.print("address=");
  Serial.print(address);
  Serial.print(", command=");
  Serial.print(command);
  Serial.print(" / ");
  Serial.print(repeat+1);
  Serial.println(" time(s)");

  // send
  IrSender.sendKaseikyo_Denon(address, command, repeat);
}

// uint64_t dataToInt64(JsonArray data)
// {
//   uint64_t res = 0;
//   for (int i=0; i<data.size(); i++) {
//     uint64_t byte = data[i].as<uint64_t>();
//     res += byte << (7-i)*8;
//   }
//   return res;
// }

void sendMulti(String protocol, JsonArray data, uint repeat)
{
  // do it
  if (protocol == PROTOCOL_RAW) {
    sendRaw(data, repeat);
  // } else if (protocol == PROTOCOL_DENON) {
  //   IrSender.sendDenon(dataToInt64(data), data.size()*8, repeat);
  } else if (protocol == PROTOCOL_KASEIKYO) {
    sendDenonKaseikyo(data, repeat);
  }
  
}
