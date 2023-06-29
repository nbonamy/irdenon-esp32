#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "credentials.h"
#include "ircodes.h"
//#include "nvs_flash.h"

// #include <WiFiClient.h>
// #include <HTTPClient.h>
// #include <PubSubClient.h>
// const char* MQTT_BROKER = "192.168.178.200";
// WiFiClient espClient;
// PubSubClient client(espClient);

IRsend irsend(4); // D2
WebServer server(80);
int wifiConnected = 0;

// const char index_html[] PROGMEM = R"rawliteral(
// <!DOCTYPE html>
// <html>
// <head>
//   <title>Denon IR</title>
//   <style>
//     button { font-size: 4em; margin: 11px; width: 45%; height: 256px; }
//     .btnRed { background-color: #4C0000; color: white; }
//     .btnGreen { background-color: #004C00; color: white; }
//     .btnBlue { background-color: #00004C; color: white; }
//     .btnViolet { background-color: #4C004C; color: white; width: 30%; }
//   </style>
//   <script src="http://code.jquery.com/jquery-1.11.0.min.js"></script>
//   <script type="text/javascript">
//     function sendVal(newVal){
//       $.ajax({
//         url: "/api/do?action="+newVal,
//         type: "GET"
//       });
//     }
//   </script>
// </head>
// <body>
//   <button onclick="sendVal(this.value)" value="PWON" class="btnRed">PWON</button>
//   <button onclick="sendVal(this.value)" value="PWOFF" class="btnGreen">PWOFF</button>
//   <br>
//   <button onclick="sendVal(this.value)" value="VOLDOWN">VOLDOWN</button>
//   <button onclick="sendVal(this.value)" value="VOLUP">VOLUP</button>
//   <br>
//   <button onclick="sendVal(this.value)" value="DVD" class="btnViolet">DVD</button>
//   <button onclick="sendVal(this.value)" value="TV" class="btnViolet">TV</button>
//   <button onclick="sendVal(this.value)" value="VCR" class="btnViolet">VCR</button>
//   <br>
//   <button onclick="sendVal(this.value)" value="MUTE">MUTE</button>
//   <button onclick="sendVal(this.value)" value="NIGHT">NIGHT</button>
// </body>
// </html>
// )rawliteral";

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

/*void mqtt(char* topic, byte* payload, unsigned int length) {
    Serial.print("Received mqtt [");
    Serial.print(topic);
    Serial.print("] ");
    char msg[length+1];
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
        msg[i] = (char)payload[i];
    }
    Serial.println();

    msg[length] = '\0';
    Serial.println(msg);

    if(strcmp(msg,"on")==0){
        send(PWON);
    }
    else if(strcmp(msg,"off")==0){
        send(PWOFF);
    }
    else if(strcmp(msg,"mute")==0){
        Serial.println("IR mute");
        send(MUTE);
    }
    else if(strcmp(msg,"volup")==0){
        Serial.println("IR volup");
        sendMulti(DENON_VOL_REPEAT,VOLUP);
    }
    else if(strcmp(msg,"voldown")==0){
        Serial.println("IR voldown");
        sendMulti(DENON_VOL_REPEAT,VOLDOWN);
    }
    else if(strcmp(msg,"dvd")==0){
        Serial.println("IR dvd");
        sendMulti(DENON_VOL_REPEAT,DVD);
    }
    else if(strcmp(msg,"tv")==0){
        Serial.println("IR tv");
        sendMulti(DENON_VOL_REPEAT,TV);
    }
    else if(strcmp(msg,"vcr")==0){
        Serial.println("IR vcr");
        sendMulti(DENON_VOL_REPEAT,VCR);
    }
    else if(strcmp(msg,"night")==0){
        Serial.println("IR night");
        sendMulti(DENON_VOL_REPEAT,NIGHT);
    }
}*/

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
  // clear all stuff
  Serial.println("");
  //ESP_ERROR_CHECK(nvs_flash_erase());
  //nvs_flash_init();

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

  // setup mqtt
  // client.setServer(MQTT_BROKER, 1883);
  // client.setCallback(mqtt);
  // Serial.print("mqtt broker: ");
  // Serial.println(MQTT_BROKER);

  // configure server
  Serial.println("[HTTP] Starting webserver");
  server.on("/api/do", handleApiDo);
  server.onNotFound([]() {
    if (!handleFileRead(server.uri())) {
      server.send(404, "text/plain", "404 Not Found");
    }
  });  server.begin();
}

/*void reconnect() {
    while (!client.connected()) {
        Serial.println("Reconnecting MQTT...");
        if (!client.connect("ESP-Denon")) {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" retrying in 5 seconds");
            delay(5000);
        }
    }
    client.subscribe("/home/denon");
    Serial.println("MQTT Connected...");
}*/

void loop()
{
  // Serial.print(".");
  server.handleClient();
}
