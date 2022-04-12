#include <ESP8266WiFi.h>

#define STACK_PROTECTOR 512
#define MAX_SRV_CLIENTS 2

WiFiClient serverClients[MAX_SRV_CLIENTS];
WiFiServer server(8888);

//IPAddress staticIP(10, 10, 10, 89);
//IPAddress gateway(10, 10, 10, 1);
//IPAddress subnet(255, 255, 255, 0);

void setup() {
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.begin("XXX", "Password");
//  WiFi.config(staticIP, gateway, subnet);
  pinMode(2, OUTPUT);

  CheckWifi();

  server.begin();
  server.setNoDelay(true);
}

void CheckWifi() {
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(2, HIGH);
    delay(200);
  }
  digitalWrite(2, LOW);
}

void loop() {
  CheckWifi();

  if (server.hasClient()) {
    // find free/disconnected spot
    for (int i = 0; i < MAX_SRV_CLIENTS; i++)
      if (!serverClients[i]) {  // equivalent to !serverClients[i].connected()
        serverClients[i] = server.available();
        break;
      }
  }

  for (int i = 0; i < MAX_SRV_CLIENTS; i++) {
    while (serverClients[i].available() && Serial.availableForWrite() > 0) {      
      if(digitalRead(2) != HIGH) {
        digitalWrite(2, HIGH);
//        delay(1);
        delayMicroseconds(5);
      }
      Serial.print(serverClients[i].read());
    }
  
    if(digitalRead(2) != LOW) {
//      delay(1);
      delayMicroseconds(5);
      digitalWrite(2, LOW);      
    }
  }

  // determine maximum output size "fair TCP use"
  // client.availableForWrite() returns 0 when !client.connected()
  int maxToTcp = 0;
  for (int i = 0; i < MAX_SRV_CLIENTS; i++) {
    if (serverClients[i]) {
      int afw = serverClients[i].availableForWrite();
      if (afw) {
        if (!maxToTcp) {
          maxToTcp = afw;
        } else {
          maxToTcp = std::min(maxToTcp, afw);
        }
      } 
    }
  }

  // check UART for data
  size_t len = std::min(Serial.available(), maxToTcp);
  len = std::min(len, (size_t)STACK_PROTECTOR);
  if (len) {
    uint8_t sbuf[len];
    int serial_got = Serial.readBytes(sbuf, len);
    // push UART data to all connected telnet clients
    for (int i = 0; i < MAX_SRV_CLIENTS; i++)
      // if client.availableForWrite() was 0 (congested)
      // and increased since then,
      // ensure write space is sufficient:
      if (serverClients[i].availableForWrite() >= serial_got) {
        serverClients[i].flush();
        size_t tcp_sent = serverClients[i].write(sbuf, serial_got);
      }
  }

}
