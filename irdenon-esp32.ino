#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "config.h"
#include "ircodes.h"

IRsend irsend(4); // D2
WebServer server(80);
File fsUploadFile;
int wifiConnected = 0;

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

String methodToString(http_method method) {
  switch (method) {
    case http_method::HTTP_HEAD:
      return "HEAD";
    case http_method::HTTP_GET:
      return "GET";
    case http_method::HTTP_POST:
      return "POST";
    case http_method::HTTP_PUT:
      return "PUT";
    default:
      return "???";
  }
}

String getContentType(String filename) {
  if (server.hasArg("download")) {
    return "application/octet-stream";
  } else if (filename.endsWith(".htm") || filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".json")) {
    return "application/json";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  } else if (filename.endsWith(".gif")) {
    return "image/gif";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else if (filename.endsWith(".xml")) {
    return "text/xml";
  } else if (filename.endsWith(".pdf")) {
    return "application/x-pdf";
  } else if (filename.endsWith(".zip")) {
    return "application/x-zip";
  } else if (filename.endsWith(".gz")) {
    return "application/x-gzip";
  }
  return "text/plain";
}

bool exists(String path){
  bool yes = false;
  File file = SPIFFS.open(path, "r");
  if(!file.isDirectory()){
    yes = true;
  }
  file.close();
  return yes;
}

bool handleFileRead(String path) {
  
  // log
  Serial.print("[HTTP] ");
  Serial.print(methodToString(server.method()));
  Serial.print(" ");
  Serial.println(server.uri());

  // map
  if (path == "/") {
    path = "/index.html";
  }

  // check it exists
  if (!exists(path)) {
    return false;
  }

  // now serve
  File file = SPIFFS.open(path, "r");
  server.streamFile(file, getContentType(path));
  file.close();
  return true;
}

void handleFileUpload() {
  if (server.uri() != "/api/upload") {
    return;
  }
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    
    // get filename
    String filename = upload.filename;
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    // log
    Serial.print("[HTTP] Upload name: ");
    Serial.println(filename);

    // start
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();

  } else if (upload.status == UPLOAD_FILE_WRITE) {
    //Serial.print("handleFileUpload Data: ");
    //Serial.println(upload.currentSize);
    if (fsUploadFile) {
      fsUploadFile.write(upload.buf, upload.currentSize);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {
      fsUploadFile.close();
    }
    Serial.print("[HTTP] Upload completed. Bytes: ");
    Serial.println(upload.totalSize);
  }
}

void handleApiDo()
{
  // check action button
  String btn = server.arg("action");
  Serial.print("[HTTP] Command received: ");
  Serial.println(btn);

  // process
  if (btn == "PWON") send(PWON);
  else if (btn == "PWOFF") send(PWOFF);
  else if (btn == "VOLUP") sendMulti(VOLUP, DENON_VOL_REPEAT);
  else if (btn == "VOLDOWN") sendMulti(VOLDOWN, DENON_VOL_REPEAT);
  else if (btn == "DVD") send(DVD);
  else if (btn == "TV") send(TV);
  else if (btn == "VCR") send(VCR);
  else if (btn == "MUTE") send(MUTE);
  else if (btn == "NIGHT") send(NIGHT);
  else {
    server.send(404, "text/plain", "404 Not Found");
    return;
  }

  // good command
  server.send(200, getContentType("dummy.json"), "{ \"status\": \"ok\" }");

}

void onWiFiDisconnect(WiFiEvent_t event, WiFiEventInfo_t info)
{
  if (wifiConnected == 1) {
    Serial.println("[WiFi] Disconnected! Reconnecting");
  }
}

void onWiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("");
  Serial.print("[WiFi] Connected with IP address: ");
  Serial.println(IPAddress(info.got_ip.ip_info.ip.addr));
}

void connectWifi()
{
  // reset
  Serial.println("[WiFi] Resetting");
  wifiConnected = 0;
  WiFi.disconnect(false, true);
  //WiFi.setAutoReconnect(false);
  WiFi.setSleep(false);
  WiFi.mode(WIFI_STA);

  // log
  Serial.print("[WiFi] MAC Address: ");
  Serial.println(WiFi.macAddress());

  // configure static stuff
  #ifdef WIFI_IP
    Serial.print("[WiFi] Configuring static IP: ");
    Serial.println(WIFI_IP);
    WiFi.config(WIFI_IP, WIFI_GW, WIFI_MK);
  #endif

  // log
  Serial.print("[WiFi] Connecting to SSID: ");
  Serial.println(WIFI_SSID);

  // some events
  WiFi.onEvent(onWiFiDisconnect, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.onEvent(onWiFiDisconnect, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_LOST_IP);
  WiFi.onEvent(onWiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);

  // do it
  unsigned long start = millis();
  WiFi.begin(WIFI_SSID, WIFI_PSK);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
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
    if (millis() - start > 3*60000) {
      Serial.println("");
      Serial.println("[WiFi] Waited too long. Restarting ESP32");
      WiFi.disconnect();
      ESP.restart();
    }

    // wait a bit
    delay(500);
  }

  // connected!
  wifiConnected = 1;

}

void setup()
{
  // clear output
  Serial.println("");

  // serial stuff
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(115200);
  irsend.begin();

  // start spiffs
  SPIFFS.begin(true);

  // connect to wifi
  connectWifi();

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

  // configure server
  Serial.println("[HTTP] Starting webserver");
  server.on("/api/do", handleApiDo);
  server.on("/api/upload", HTTP_POST, []() {
    server.send(200, "text/plain", "");
  }, handleFileUpload);
  server.onNotFound([]() {
    if (!handleFileRead(server.uri())) {
      server.send(404, "text/plain", "404 Not Found");
    }
  });  server.begin();
}

void loop()
{
  server.handleClient();
}
