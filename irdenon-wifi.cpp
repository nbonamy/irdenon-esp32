#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "irdenon-wifi.h"
#include "irdenon-led.h"
#include "config.h"

int wifiConnected = 0;
int timeout = 3*60000;

void onWiFiDisconnect(WiFiEvent_t event, WiFiEventInfo_t info)
{
  if (wifiConnected == 1) {
    Serial.println("[WIFI] Disconnected! Reconnecting");
    wifiConnected = 0;
    ledOn();
  }
}

void onWiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
  // log
  Serial.println("");
  Serial.print("[WIFI] Connected with IP address: ");
  Serial.println(IPAddress(info.got_ip.ip_info.ip.addr));

  // mdns
  #ifdef MDNS_HOST
    if (MDNS.begin(MDNS_HOST)) {
      Serial.print("[MDNS] Responder started: ");
      Serial.print(MDNS_HOST);
      Serial.println(".local");
    } else {
      Serial.println("[MDNS] Responder failed");
    }
  #endif

  // led
  ledOff();

}

void connectWifi()
{
  // reset
  Serial.println("[WIFI] Resetting");
  wifiConnected = 0;
  WiFi.disconnect(false, true);
  //WiFi.setAutoReconnect(false);
  WiFi.setSleep(false);
  WiFi.mode(WIFI_STA);

  // log
  Serial.print("[WIFI] MAC Address: ");
  Serial.println(WiFi.macAddress());

  // configure static stuff
  #ifdef WIFI_IP
    Serial.print("[WIFI] Configuring static IP: ");
    Serial.println(WIFI_IP);
    WiFi.config(WIFI_IP, WIFI_GW, WIFI_MK);
  #endif

  // log
  Serial.print("[WIFI] Connecting to SSID: ");
  Serial.println(WIFI_SSID);
  Serial.print("[WIFI] Status: ");

  // some events
  WiFi.onEvent(onWiFiDisconnect, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.onEvent(onWiFiDisconnect, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_LOST_IP);
  WiFi.onEvent(onWiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);

  // do it
  unsigned long start = millis();
  WiFi.begin(WIFI_SSID, WIFI_PSK);
  //WiFi.setTxPower(WIFI_POWER_8_5dBm);
  while (true) {

    // get status
    uint8_t status = WiFi.waitForConnectResult();
    if (status == WL_CONNECTED) {
      //Serial.println("C");
      break;
    }

    // log
    switch(status) {
      case WL_NO_SSID_AVAIL:
        Serial.print("X");
        break;
      case WL_CONNECT_FAILED:
        Serial.print("F");
        break;
      case WL_CONNECTION_LOST:
        Serial.print("L");
        break;
      case WL_DISCONNECTED:
        Serial.print("D");
        break;
      default:
        Serial.print(".");
        break;
    }

    // expired?
    if (millis() - start > timeout) {
      Serial.println("");
      Serial.println("[WIFI] Waited too long. Restarting ESP32");
      WiFi.disconnect();
      ESP.restart();
    }

    // wait a bit
    delay(500);
  }

  // connected!
  wifiConnected = 1;

}
