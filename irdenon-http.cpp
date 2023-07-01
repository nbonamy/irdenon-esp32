#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include "irdenon-http.h"
#include "irdenon-ir.h"

WebServer server(80);
File fsUploadFile;
bool isIrFileUploaded;

bool handleFileRead(String path);
void handleFileUpload();
void handleApiSend();

void startHttpServer() {

  Serial.println("[HTTP] Starting webserver");
  server.on("/api/reboot", [] () {
    Serial.println("[HTTP] Reboot command received");
    server.send(200, "text/plain", "");
    WiFi.disconnect();
    ESP.restart();
  });
  server.on("/api/send", handleApiSend);
  server.on("/api/upload", HTTP_POST, []() {
    server.send(200, "text/plain", "");
  }, handleFileUpload);
  server.onNotFound([]() {
    if (!handleFileRead(server.uri())) {
      server.send(404, "text/plain", "404 Not Found");
    }
  }); 
  server.begin();

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
  File file = SPIFFS.open(path, FILE_READ);
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
  File file = SPIFFS.open(path, FILE_READ);
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
    Serial.print("[HTTP] Upload started: ");
    Serial.println(filename);

    // start
    fsUploadFile = SPIFFS.open(filename, FILE_WRITE);

    // ir file?
    isIrFileUploaded = (filename == "/ircodes.json");

  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile) {
      fsUploadFile.write(upload.buf, upload.currentSize);
    }
  } else if (upload.status == UPLOAD_FILE_END) {

    // close
    if (fsUploadFile) {
      fsUploadFile.close();
    }

    // log
    Serial.print("[HTTP] Upload completed. Bytes: ");
    Serial.println(upload.totalSize);

    // reload ir
    if (isIrFileUploaded) {
      loadIr();
    }
  }
}

void handleApiSend()
{
  // check command
  String btn = server.arg("command");
  Serial.print("[HTTP] Command received: ");
  Serial.println(btn);

  // process
  if (sendIr(btn)) {
    server.send(200, getContentType("dummy.json"), "{ \"status\": \"ok\" }");
  } else {
    server.send(404, "text/plain", "404 Not Found");
  }

}

