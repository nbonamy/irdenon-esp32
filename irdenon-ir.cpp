#include <Arduino.h>
#include "irdenon-ir.h"
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include "ircodes.h"

IRsend irsend(4); // D2

void send(uint16_t buf[]);
void sendMulti(uint16_t buf[], uint16_t count);

void initIr()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  irsend.begin();
}

bool sendIr(String command)
{
  if (command == "PWON") send(PWON);
  else if (command == "PWOFF") send(PWOFF);
  else if (command == "VOLUP") sendMulti(VOLUP, DENON_VOL_REPEAT);
  else if (command == "VOLDOWN") sendMulti(VOLDOWN, DENON_VOL_REPEAT);
  else if (command == "DVD") send(DVD);
  else if (command == "TV") send(TV);
  else if (command == "VCR") send(VCR);
  else if (command == "MUTE") send(MUTE);
  else if (command == "NIGHT") send(NIGHT);
  else return false;
  return true;
}

void send(uint16_t buf[])
{
  digitalWrite(LED_BUILTIN, LOW);
  irsend.sendRaw(buf, 95, 38);
  digitalWrite(LED_BUILTIN, HIGH);
}

void sendMulti(uint16_t buf[], uint16_t count)
{
  for (int i = 0; i < count; i++) {
    send(buf);
  }
}
