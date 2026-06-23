/*
 * Motorhome GPS NMEA bridge  -  WT32-ETH01 (ESP32 + LAN8720 wired Ethernet)
 *
 * Reads NMEA 0183 from a NEO-7M GPS on UART2 at 9600 baud and pushes each
 * sentence over the wired LAN to the Peplink B One 5G, which listens as the
 * "GPS Receiver for Raw NMEA 0183 Network Stream".
 *
 * Transport defaults to UDP (the one documented-working B One 5G case used UDP).
 * Set USE_TCP to 1 to send as a TCP client instead (auto-reconnects on drop).
 *
 * Arduino IDE + ESP32 board support. Compiles on core 2.x and 3.x (the
 * ETH.begin call is branched on version). Board: "WT32-ETH01 Ethernet Module",
 * or "ESP32 Dev Module" if that entry is not in your list.
 */

#include <Arduino.h>
#include <ETH.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>

// ---------- user settings ----------
#define USE_TCP        0              // 0 = UDP (default), 1 = TCP client
#define PEPLINK_IP     "192.168.50.1" // your Peplink LAN IP (the listener); 192.168.50.1 is the Peplink default
#define PEPLINK_PORT   10110          // matches the Peplink NMEA stream port

// GPS on UART2. Pins are the WT32-ETH01 right-side header:
//   board silk "RXD" = GPIO5  -> ESP receive  (wire GPS TXD here, the one that matters)
//   board silk "TXD" = GPIO17 -> ESP transmit (wire GPS RXD here, optional, config only)
#define GPS_RX_PIN     5
#define GPS_TX_PIN     17
#define GPS_BAUD       9600
// ------------------------------------

// WT32-ETH01 LAN8720 wiring (leave as-is for this board)
#define ETH_PHY        ETH_PHY_LAN8720
#define ETH_ADDR       1
#define ETH_POWER_PIN  16
#define ETH_MDC_PIN    23
#define ETH_MDIO_PIN   18
#define ETH_CLK        ETH_CLOCK_GPIO0_IN

WiFiUDP    udp;
WiFiClient tcp;
unsigned long lastTcpTry = 0;
unsigned long lastStatus = 0;
unsigned long sentCount  = 0;

char line[128];
int  lineLen = 0;

void forwardSentence(const char *buf, int len) {
#if USE_TCP
  if (!tcp.connected()) {
    if (millis() - lastTcpTry > 2000) {
      lastTcpTry = millis();
      tcp.connect(PEPLINK_IP, PEPLINK_PORT);
    }
    if (!tcp.connected()) return;          // still down, drop this sentence
  }
  tcp.write((const uint8_t *)buf, len);
#else
  udp.beginPacket(PEPLINK_IP, PEPLINK_PORT);
  udp.write((const uint8_t *)buf, len);
  udp.endPacket();
#endif
  sentCount++;
  Serial.write((const uint8_t *)buf, len);  // echo to USB serial for bench debug
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("Motorhome GPS NMEA bridge starting");

  Serial2.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ETH.begin(ETH_PHY, ETH_ADDR, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_POWER_PIN, ETH_CLK);
#else
  ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_PHY, ETH_CLK);
#endif

  Serial.print("Bringing up Ethernet (DHCP)");
  unsigned long t0 = millis();
  while ((uint32_t)ETH.localIP() == 0 && millis() - t0 < 20000) {
    Serial.print('.');
    delay(500);
  }
  Serial.println();

  if ((uint32_t)ETH.localIP() == 0) {
    Serial.println("No IP yet. Check cable/link; it will keep retrying below.");
  } else {
    Serial.print("Link up. IP ");
    Serial.print(ETH.localIP());
    Serial.print("  MAC ");
    Serial.println(ETH.macAddress());
    Serial.print("Reserve this MAC on the Peplink, whitelist its IP. Sending to ");
    Serial.print(PEPLINK_IP); Serial.print(':'); Serial.print(PEPLINK_PORT);
    Serial.println(USE_TCP ? " over TCP" : " over UDP");
  }
}

void loop() {
  // pull NMEA from the GPS one line at a time, forward complete sentences
  while (Serial2.available()) {
    char c = Serial2.read();
    if (lineLen < (int)sizeof(line) - 1) {
      line[lineLen++] = c;
    } else {
      lineLen = 0;                    // overflow guard, drop the runt
    }
    if (c == '\n') {
      line[lineLen] = '\0';
      if (line[0] == '$') {           // forward valid NMEA only
        forwardSentence(line, lineLen);
      }
      lineLen = 0;
    }
  }

  // heartbeat every 5s so the bench shows life even before a fix exists
  if (millis() - lastStatus > 5000) {
    lastStatus = millis();
    Serial.print("[status] ip ");
    Serial.print(ETH.localIP());
    Serial.print("  link ");
    Serial.print(ETH.linkUp() ? "up" : "down");
    Serial.print("  sentences forwarded ");
    Serial.println(sentCount);
  }
}
