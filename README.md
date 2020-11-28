# libnetthermprint

## What it is?
libnetthermprint is a simple library which changes your thermal printer to networked thermal printer. Server will be provided soon!

# Usage
Import the library and libescpos into your project and declare functions with weak attribute.
Below is an example code which you can use with ESP8266 Arduino:
```c++
NetThermPrint netprint;
void loop() {
  ArduinoOTA.handle();
  int packetSize = UDP.parsePacket();
  if (packetSize) {
    char *packet = new char[packetSize];
    int len = UDP.read(packet, packetSize);
    if (len > 0) {
      netprint.processData(packet);
    }
    delete[] packet;
  }
  netprint.timerLoop();
}
void NetThermPrint_sendUdpResponse(const char *data, size_t len) {
  UDP.beginPacket(UDP.remoteIP(), UDP.remotePort());
  UDP.write(data, len);
  UDP.endPacket();
}

void NetThermPrint_sendUdp(const char *data, size_t len) {
  UDP.beginPacket(serverIp, port);
  UDP.write(data, len);
  UDP.endPacket();
}

time_t NetThermPrint_getTime() { return millis(); }

void NetThermPrint_systemReset() { ESP.restart(); }
```