
// Broadcast the names of victims of police brutality since 2015
// Based on https://github.com/markszabo/FakeBeaconESP8266/
// and the work by Moritz Metz for the death of John Perry Barlow

// ESP32 code lifted from https://github.com/Jeija/esp32-80211-tx/

// Data from Mapping Police Violence for the deaths occuring between 2015 & 2019
// https://mappingpoliceviolence.org/
// Data from wikipedia for the deaths occuring in 2020
// https://en.wikipedia.org/wiki/Lists_of_killings_by_law_enforcement_officers_in_the_United_States 

/* ----------------- */

// Broadcast the Declaration of Independence of Cyberspace as SSIDs
// based on https://github.com/markszabo/FakeBeaconESP8266/
// Forked by Moritz Metz, 2018-02-08
// Works on an ESP8266 (i.e. Wemos D1 Mini)
// USE ONLY FOR TESTING AND AT YOUR OWN RISK :)
// Independence of Cyberspace by John Perry Barlow R.I.P. 2018-02-07
// Text: https://www.eff.org/cyberspace-independence
// Video: https://vimeo.com/111576518
// Wikipedia: https://en.wikipedia.org/wiki/A_Declaration_of_the_Independence_of_Cyberspace

/*
 * Fake beacon frames for ESP8266 using the Arduino IDE
 * Compiled with Arduino 1.6.9 and esp8266 2.1.0, but other versions should work too
 *
 * Based on the WiFiBeaconJam by kripthor (https://github.com/kripthor/WiFiBeaconJam)
 *
 * More info: http://nomartini-noparty.blogspot.com/2016/07/esp8266-and-beacon-frames.html
 */

/* ----------------- */

#include <Arduino.h>

#if defined(ESP8266)
  #include <ESP8266WiFi.h> //more about beacon frames https://mrncciew.com/2014/10/08/802-11-mgmt-beacon-frame/
  extern "C" {
    #include "user_interface.h"
  }
#elif defined(ESP32)
  #include <WiFi.h>
  #include "esp_wifi.h"
  /*
  * This is the (currently unofficial) 802.11 raw frame TX API,
  * defined in esp32-wifi-lib's libnet80211.a/ieee80211_output.o
  *
  * This declaration is all you need for using esp_wifi_80211_tx in your own application.
  */
  esp_err_t esp_wifi_80211_tx(wifi_interface_t ifx, const void *buffer, int len, bool en_sys_seq);

  uint8_t beacon_raw[] = {
    0x80, 0x00,							// 0-1: Frame Control
    0x00, 0x00,							// 2-3: Duration
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,				// 4-9: Destination address (broadcast)
    0xba, 0xde, 0xaf, 0xfe, 0x00, 0x06,				// 10-15: Source address
    0xba, 0xde, 0xaf, 0xfe, 0x00, 0x06,				// 16-21: BSSID
    0x00, 0x00,							// 22-23: Sequence / fragment number
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,			// 24-31: Timestamp (GETS OVERWRITTEN TO 0 BY HARDWARE)
    0x64, 0x00,							// 32-33: Beacon interval
    0x31, 0x04,							// 34-35: Capability info
    0x00, 0x00, /* FILL CONTENT HERE */				// 36-38: SSID parameter set, 0x00:length:content
    0x01, 0x08, 0x82, 0x84,	0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24,	// 39-48: Supported rates
    0x03, 0x01, 0x01,						// 49-51: DS Parameter set, current channel 1 (= 0x01),
    0x05, 0x04, 0x01, 0x02, 0x00, 0x00,				// 52-57: Traffic Indication Map
  };

  #define BEACON_SSID_OFFSET 38
  #define SRCADDR_OFFSET 10
  #define BSSID_OFFSET 16
  #define SEQNUM_OFFSET 22
#else
  #error Code not made for anything else than an ESP8266 or an ESP32
#endif

#include "data.h"

#define TOTAL_LINES (sizeof(data) / sizeof(char *))

#if defined(ESP8266)
void sendBeacon(const char* ssid) {
  // Randomize channel //
  byte channel = random(1,12);

  wifi_set_channel(channel);

  uint8_t packet[128] = { 0x80, 0x00, //Frame Control
                      0x00, 0x00, //Duration
              /*4*/   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, //Destination address
              /*10*/  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, //Source address - overwritten later
              /*16*/  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, //BSSID - overwritten to the same as the source address
              /*22*/  0xc0, 0x6c, //Seq-ctl
              //Frame body starts here
              /*24*/  0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00, 0x00, 0x00, //timestamp - the number of microseconds the AP has been active
              /*32*/  0xFF, 0x00, //Beacon interval
              /*34*/  0x01, 0x04, //Capability info
              /* SSID */
              /*36*/  0x00
              };

  int ssidLen = strlen(ssid);
  packet[37] = ssidLen;

  for(int i = 0; i < ssidLen; i++) {
    packet[38+i] = ssid[i];
  }

  uint8_t postSSID[13] = {0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, //supported rate
                      0x03, 0x01, 0x04 /*DSSS (Current Channel)*/ };

  for(int i = 0; i < 12; i++) {
    packet[38 + ssidLen + i] = postSSID[i];
  }

  packet[50 + ssidLen] = channel;

  // Randomize SRC MAC
  packet[10] = packet[16] = random(256);
  packet[11] = packet[17] = random(256);
  packet[12] = packet[18] = random(256);
  packet[13] = packet[19] = random(256);
  packet[14] = packet[20] = random(256);
  packet[15] = packet[21] = random(256);

  int packetSize = 51 + ssidLen;

  wifi_send_pkt_freedom(packet, packetSize, 0);
  wifi_send_pkt_freedom(packet, packetSize, 0);
  wifi_send_pkt_freedom(packet, packetSize, 0);

  delay(1);
}
#elif defined(ESP32)
void sendBeacon(const char* ssid) {
  // Randomize channel //
  uint8_t beacon[200];
  memcpy(beacon, beacon_raw, BEACON_SSID_OFFSET - 1);
  beacon[BEACON_SSID_OFFSET - 1] = strlen(ssid);
  memcpy(&beacon[BEACON_SSID_OFFSET], ssid, strlen(ssid));
  memcpy(&beacon[BEACON_SSID_OFFSET + strlen(ssid)], &beacon_raw[BEACON_SSID_OFFSET], sizeof(beacon_raw) - BEACON_SSID_OFFSET);

  // Last byte of source address / BSSID will be line number - emulate multiple APs broadcasting one song line each
  beacon[SRCADDR_OFFSET + 5] = 0;
  beacon[BSSID_OFFSET + 5] = 0;

  esp_wifi_80211_tx(WIFI_IF_AP, beacon, sizeof(beacon_raw) + strlen(ssid), false);
}
#endif

void DoIoC() {
  for (uint i = 0; i < TOTAL_LINES; i++) {
    sendBeacon(data[i]);
  }
}

void setup() {
  delay(500);
  #if defined(ESP8266)
    wifi_set_opmode(STATION_MODE);
    wifi_promiscuous_enable(1);
  #elif defined(ESP32)
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_AP);

    wifi_config_t ap_config = {};

    //Assign ssid & password strings
    strcpy((char*)ap_config.ap.ssid, "ssid");
    strcpy((char*)ap_config.ap.password, "password");
    ap_config.ap.ssid_len = 0;
    ap_config.ap.channel = 1;
    ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    ap_config.ap.ssid_hidden = 1;
    ap_config.ap.max_connection = 4;
    ap_config.ap.beacon_interval = 60000;

    esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    esp_wifi_start();
    esp_wifi_set_ps(WIFI_PS_NONE);
  #endif
}

void loop() {
  DoIoC();
}
