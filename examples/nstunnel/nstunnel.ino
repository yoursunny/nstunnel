#include <nstunnel.hpp>
#include <Streaming.h>
#include <WiFi.h>
#include <WiFiUDP.h>

const char* NS_DOMAIN = "nstunnel.example.net";
WiFiUDP udp;
NsTunnel nst(udp);

bool connect() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  int n = WiFi.scanNetworks();
  String ssid;
  int bestRssi = -9999;
  for (int i = 0; i < n; ++i) {
    if (WiFi.encryptionType(i) != WIFI_AUTH_OPEN) {
      continue;
    }
    int rssi = WiFi.RSSI(i);
    if (rssi > bestRssi) {
      ssid = WiFi.SSID(i);
      bestRssi = rssi;
    }
  }
  WiFi.scanDelete();
  if (bestRssi == -9999) {
    Serial.println("Open Network unavailable.");
    return false;
  }

  Serial << "SSID=" << ssid << endl;
  WiFi.begin(ssid.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }
  Serial << "IP=" << WiFi.localIP() << " GW=" << WiFi.gatewayIP() << " DNS=" << WiFi.dnsIP() << endl;
}

void nstRx(uint16_t id, const String& msg)
{
  Serial << "RX " << millis() << " " << id << " " << msg << endl;
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  while (!connect()) {
    delay(1000);
  }
  nst.begin(NS_DOMAIN, WiFi.dnsIP());
  nst.onRx(nstRx);
}

void loop() {
  static int cnt = 0;
  if (++cnt % 100 == 0) {
    uint16_t id = random(0xFFFF);
    String msg(millis());
    Serial << "TX " << millis() << " " << id << " " << msg << endl;
    nst.send(id, msg);
  }
  nst.loop();
  delay(100);
}