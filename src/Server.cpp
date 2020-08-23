/*************************************************** 
    Copyright (C) 2016  Steffen Ochs

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
    HISTORY: Please refer Github History
    
 ****************************************************/

// Beispiele:
// https://github.com/spacehuhn/wifi_ducky/blob/master/esp8266_wifi_duck/esp8266_wifi_duck.ino
// WebSocketClient: https://github.com/Links2004/arduinoWebSockets/issues/119

#include "Server.h"
#include "WebHandler.h"
#include "Cloud.h"
#include "system/SystemBase.h"
#include "DbgPrint.h"
#include "Settings.h"
#include "RecoveryMode.h"
#include <SPIFFS.h>

// include html files
#include "webui/index.html.gz.h"
#include "webui/fwupdate.html.gz.h"
#include "webui/displayupdate.html.gz.h"
#include "webui/restart.html.gz.h"

#define DEFAULT_PASSWORD "admin"

const char *WServer::username = "admin";
String WServer::password = DEFAULT_PASSWORD;
boolean WServer::requireAuth = true;

WServer::WServer()
{
}

void WServer::init()
{
  loadConfig();
  webServer = new AsyncWebServer(80);
  webServer->addHandler(&nanoWebHandler);

  webServer->on("/help", HTTP_GET, [](AsyncWebServerRequest *request) {
             request->redirect("https://github.com/WLANThermo-nano/WLANThermo_nano_Software/blob/master/README.md");
           })
      .setFilter(ON_STA_FILTER);

  webServer->on("/info", [](AsyncWebServerRequest *request) {
    size_t usedBytes;
    size_t totalBytes;
    usedBytes = SPIFFS.usedBytes();
    totalBytes = SPIFFS.totalBytes();
    //TODO: print wifi SSIDs
    /*for (int i = 0; i < wifi.savedlen; i++) {
        ssidstr += " ";
        ssidstr += String(i+1);
        ssidstr += ": "; 
        ssidstr += wifi.savedssid[i];
    }*/

    String info = "spiffs: " + String(usedBytes) + " | " + String(totalBytes) + "\n" + "heap: " + String(ESP.getFreeHeap()) + "\n" + "sn: " + gSystem->getSerialNumber() + "\n" + "pn: " + gSystem->item.read(ItemNvsKeys::kItem) + "\n";
    if (gSystem->battery != NULL)
    {
      info += "batlimit: " + String(gSystem->battery->min) + " | " + String(gSystem->battery->max) + "\n" + "bat: " + String(gSystem->battery->adcvoltage) + " | " + String(gSystem->battery->voltage) + " | " + String(gSystem->battery->simc) + "\n" + "batstat: " + String(gSystem->battery->getPowerModeInt()) + " | " + String(gSystem->battery->setreference) + "\n";
    }
    info += "ssid: " + WiFi.SSID() + "\n" + "wifimode: " + String(WiFi.getMode()) + "\n" + "mac:" + String(gSystem->wlan.getMacAddress()) + "\n" + "iS: " + String(gSystem->getFlashSize());
    request->send(200, "", info);
  });

  webServer->on("/setbattmin", [](AsyncWebServerRequest *request) {
    if (gSystem->battery)
    {
      gSystem->battery->min -= 100;
      gSystem->battery->saveConfig();
    }
    request->send(200, TEXTPLAIN, "Done");
  });

  webServer->on("/settestmode", [](AsyncWebServerRequest *request) {
    CloudConfig cloudConfig = gSystem->cloud.getConfig();
    cloudConfig.interval = 3;
    gSystem->cloud.setConfig(cloudConfig);
    request->send(200, TEXTPLAIN, "3 Sekunden");
  });

  webServer->on("/stop", [](AsyncWebServerRequest *request) {
    for (uint8_t i = 0u; i < gSystem->pitmasters.count(); i++)
    {
      Pitmaster *pm = gSystem->pitmasters[i];

      if (pm != NULL)
        pm->setType(pm_off);
    }
    gSystem->pitmasters.saveConfig();
    request->send(200, TEXTPLAIN, "Stop pitmaster");
  });

  webServer->on("/clientlog", [](AsyncWebServerRequest *request) {
    Cloud::clientlog = true;
    gSystem->otaUpdate.resetUpdateInfo();
    gSystem->otaUpdate.askUpdateInfo();
    request->send(200, TEXTPLAIN, "aktiviert");
  });

  webServer->on("/restart", [](AsyncWebServerRequest *request) {
             AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", restart_html_gz, sizeof(restart_html_gz));
             response->addHeader("Content-Disposition", "inline; filename=\"index.html\"");
             response->addHeader("Content-Encoding", "gzip");
             request->send(response);
             gSystem->restart();
           })
      .setFilter(ON_STA_FILTER);

  webServer->on("/ping", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, TEXTPLAIN, WiFi.localIP().toString().c_str());
  });

  webServer->on("/newtoken", [](AsyncWebServerRequest *request) {
    request->send(200, TEXTPLAIN, gSystem->cloud.newToken());
    gSystem->cloud.saveConfig();
  });

  webServer->on("/rr", [](AsyncWebServerRequest *request) {
    String response = "\nCPU0: " + gSystem->getResetReason(0);
    response += "\nCPU1: " + gSystem->getResetReason(1);
    response += "\nResetCounter: " + String(RecoveryMode::getResetCounter());
    request->send(200, TEXTPLAIN, response);
  });

  // to avoid multiple requests to ESP
  webServer->on("/", [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html_gz, sizeof(index_html_gz));
    response->addHeader("Content-Disposition", "inline; filename=\"index.html\"");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  // 404 NOT found: called when the url is not defined here
  webServer->onNotFound([](AsyncWebServerRequest *request) {
    request->send(404);
  });

  webServer->begin();
  IPRINTPLN("HTTP server started");
}

void WServer::saveConfig()
{
  DynamicJsonBuffer jsonBuffer(Settings::jsonBufferSize);
  JsonObject &json = jsonBuffer.createObject();
  json["password"] = password;
  json["auth"] = requireAuth;
  Settings::write(kServer, json);
}

void WServer::loadConfig()
{
  DynamicJsonBuffer jsonBuffer(Settings::jsonBufferSize);
  JsonObject &json = Settings::read(kServer, &jsonBuffer);

  if (json.success())
  {

    if (json.containsKey("password"))
      this->password = json["password"].asString();
    if (json.containsKey("auth"))
      this->requireAuth = json["auth"].as<boolean>();
  }
}

String WServer::getUsername()
{
  return username;
}

String WServer::getPassword()
{
  return password;
}

void WServer::setPassword(String newPassword)
{
  password = newPassword;
}

WServer gWebServer;