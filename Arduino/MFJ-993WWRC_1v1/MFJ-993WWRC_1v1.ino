/*
  MFJ-993BRT_WWRC_1v1.ino

  Wireless remote control via webinterface of a MFJ-993BRT.
  DHCP activated.

  Copyright (C) 2022 Thorsten Godau DL9SEC

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.

  ===

  Using D1-mini (ESP8266 Module) @ 160MHz.
 
  Pin   Function        Arduino    Usage
  ===============================================
  TX    TXD,IO          GPIO1      Serial
  RX    RXD, IO         GPIO3      Serial
  A0    Analog in       A0         Unused
  D0    IO              GPIO16     TUNE (O)
  D1    IO, SCL           GPIO5      ANT (O)
  D2    IO, SDA           GPIO4      TUNING LED (I)
  D3    IO, 10k Pull-up GPIO0      SWR LED (I)
  D4    IO, BUILTIN_LED GPIO2      WLAN (O)
  D5    IO, SCK           GPIO14     C-UP (O) 
  D6    IO, MISO          GPIO12     C-DN (O)
  D7    IO, MOSI          GPIO13     L-UP (O)
  D8    IO, SS            GPIO15     L-DN (O)
  G     GND               -
  5V    5V              –          5V supply
  3V3   3.3V            -
  RST   Reset         -

  OTA update
  ==========

  Sketch OTA update works fine with a set OTA password:

    :
    ArduinoOTA.setPassword("YourOTApassphrase");
    :

  Sketch data (@SPIFFS) OTA update DOESN'T work with an enabled OTA password!
  To update the sketch data with "ESP8266 Sketch Data Upload", comment out the line above,
  upload the sketch, then upload the sketch data, re-activate the line above and upload the
  sketch again.

*/

#include <ESP8266WiFi.h>          // LGPL-2.1-or-later, (c) 2011-2014 Arduino, modified by Ivan Grokhotkov, December 2014
#include <ESPAsyncTCP.h>          // LGPL-2.1-or-later, (c) 2016 Hristo Gochkov
#include <ESPAsyncWebServer.h>    // LGPL-2.1-or-later, (c) 2016 Hristo Gochkov
#include <FS.h>                   // LGPL-2.1-or-later, (c) 2015 Ivan Grokhotkov
#include <ezOutput.h>             // BSD-3-Clause, (c) 2019, ArduinoGetStarted.com          
#include <ArduinoOTA.h>           // LGPL-2.1-or-later, (c) 2018 Juraj Andrassy

// Firmware version
#define FW_Version          "1.1"         // Firmware version

// WiFi credentials
#define STASSID             "YourSSID"
#define STAPSK              "YourPassPhrase"

// Serial baudrates
#define BAUD_LOGGER         115200

// Digital pins
#define DO_LED              2             // Diag LED (internal)
#define DO_CUP              14            // Button C-UP
#define DO_CDN              12            // Button C-DN
#define DO_LUP              13            // Button L-UP
#define DO_LDN              15            // Button L-DN
#define DO_TUNE             16            // Button TUNE
#define DO_ANT              5             // Button ANT

#define DI_LEDRD            4             // TUNING LED (red)
#define DI_LEDGN            0             // SWR LED (green)


const char* ssid            = STASSID;
const char* password        = STAPSK;

uint8_t    u8isTuning       = 0;          // State of the tuning LED
uint8_t    u8SWRok          = 0;          // State of the SWR LED

// String to hold the complete outgoing JSON data
// {"u8isTuning":0,"u8SWRok":0,"esp_FW":"x.x","esp_MAC":"xx:xx:xx:xx:xx:xx"}

String     strJSONString    = "{\"u8isTuning\":0,\"u8SWRok\":0,\"esp_FW\":\"x.x\",\"esp_MAC\":\"xx:xx:xx:xx:xx:xx\"}";

IPAddress  ipESPIP;

// Hostname
String  myHostname = "mfj-993wwrc";

AsyncWebServer server(80);  // Create AsyncWebServer object on port 80

ezOutput    rcCUP(DO_CUP);      // Remote control output for C-UP
ezOutput    rcCDN(DO_CDN);      // Remote control output for C-DN
ezOutput    rcLUP(DO_LUP);      // Remote control output for L-UP
ezOutput    rcLDN(DO_LDN);      // Remote control output for L-DN
ezOutput    rcTUNE(DO_TUNE);    // Remote control output for TUNE
ezOutput    rcANT(DO_ANT);      // Remote control output for ANT

void setup()
{
  // Init pins
  pinMode(DO_LED,   OUTPUT);
        
  pinMode(DI_LEDRD, INPUT_PULLUP);
  pinMode(DI_LEDGN, INPUT_PULLUP);

  rcCUP.low();
  rcCDN.low();
  rcLUP.low();
  rcLDN.low();
  rcTUNE.low();
  rcANT.high();

  digitalWrite(DO_LED, 1);  // Internal LED off

  // Init logger serial
  Serial.begin(BAUD_LOGGER);
  Serial.print("\n\nMFJ-993BRT Wireless Web Remote Control v"); Serial.print(FW_Version); Serial.println(" ready.");
  Serial.println(ESP.getFullVersion());

  // Init SPIFFS
  if(!SPIFFS.begin())
  {
    Serial.println("SPIFFS->An error has occurred while mounting SPIFFS");
    return;
  }

  // Set custom hostname
  WiFi.hostname(myHostname.c_str());

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print("WiFi->Connecting to "); Serial.print(STASSID); Serial.println("...");
  }
  
  digitalWrite(DO_LED, 0);  // Internal LED on

  // Print ESP8266 Local IP Address
  ipESPIP = WiFi.localIP();
  Serial.print("WiFi->IP: "); Serial.print(ipESPIP); Serial.println("...");

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    digitalWrite(DO_LED, 1);
    request->send(SPIFFS, "/index.html", "text/html");
    digitalWrite(DO_LED, 0);
  });

  // Route to load DejaVuSansMono.woff file
  server.on("/DejaVuSansMono.woff", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    digitalWrite(DO_LED, 1);
    request->send(SPIFFS, "/DejaVuSansMono.woff", "font/woff");
    digitalWrite(DO_LED, 0);
  });
  
  // Route to load stylesheet.css file
  server.on("/stylesheet.css", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    digitalWrite(DO_LED, 1);
    request->send(SPIFFS, "/stylesheet.css", "text/css");
    digitalWrite(DO_LED, 0);
  });

  // Route to load jshelper.js file
  server.on("/jshelper.js", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    digitalWrite(DO_LED, 1);
    request->send(SPIFFS, "/jshelper.js", "text/javascript");
    digitalWrite(DO_LED, 0);
  });

  // Route to load favicon.ico file
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    digitalWrite(DO_LED, 1);
    request->send(SPIFFS, "/favicon.ico", "image/x-icon");
    digitalWrite(DO_LED, 0);
  });
  
  // Route to load JSON data
  server.on("/jsondata", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    digitalWrite(DO_LED, 1);
    request->send_P(200, "application/json", strJSONString.c_str());
    digitalWrite(DO_LED, 0);
  });

  // Route to load the LICENSE text file
  server.on("/legal", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    digitalWrite(DO_LED, 1);
    request->send(SPIFFS, "/LEGAL", "text/plain");
    digitalWrite(DO_LED, 0);
  });

  // Button standard actions

  // Route to action "C-UP"
  server.on("/cup", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    digitalWrite(DO_LED, 1);
    Serial.println("WebServer->C-UP clicked...");  // Debug output
    
    if (rcCUP.getState() == LOW)
    {
      // Press C-UP for 250 milliseconds
      rcCUP.pulse(250, 0);
    }

    request->send(SPIFFS, "/index.html", "text/html");
    digitalWrite(DO_LED, 0);    
  });

  // Route to action "L-UP"
  server.on("/lup", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    digitalWrite(DO_LED, 1);
    Serial.println("WebServer->L-UP clicked...");  // Debug output
    
    if (rcLUP.getState() == LOW)
    {
      // Press L-UP for 250 milliseconds
      rcLUP.pulse(250, 0);
    }

    request->send(SPIFFS, "/index.html", "text/html");
    digitalWrite(DO_LED, 0);    
  });

  // Route to action "C-DN"
  server.on("/cdn", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    digitalWrite(DO_LED, 1);
    Serial.println("WebServer->C-DN clicked...");  // Debug output
    
    if (rcCDN.getState() == LOW)
    {
      // Press C-DN for 250 milliseconds
      rcCDN.pulse(250, 0);
    }

    request->send(SPIFFS, "/index.html", "text/html");
    digitalWrite(DO_LED, 0);    
  });

  // Route to action "L-DN"
  server.on("/ldn", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    digitalWrite(DO_LED, 1);
    Serial.println("WebServer->L-DN clicked...");  // Debug output
    
    if (rcLDN.getState() == LOW)
    {
      // Press L-DN for 250 milliseconds
      rcLDN.pulse(250);
    }

    request->send(SPIFFS, "/index.html", "text/html");
    digitalWrite(DO_LED, 0);    
  });

  // Route to action "TOGGLEC"
  server.on("/togglec", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    digitalWrite(DO_LED, 1);
    Serial.println("WebServer->TOGGLEC clicked...");  // Debug output
    
    if (rcCUP.getState() == LOW && rcCDN.getState() == LOW)
    {
      // Press C-UP & C-DN for 500 milliseconds
      rcCUP.pulse(500);
      rcCDN.pulse(500);
    }

    request->send(SPIFFS, "/index.html", "text/html");
    digitalWrite(DO_LED, 0);
  });

  // Route to action "TUNE"
  server.on("/tune", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    digitalWrite(DO_LED, 1);
    Serial.println("WebServer->TUNE clicked...");  // Debug output
    
    if (rcTUNE.getState() == LOW)
    {
      // Press TUNE for 1000 milliseconds
      rcTUNE.pulse(1000);
    }

    request->send(SPIFFS, "/index.html", "text/html");
    digitalWrite(DO_LED, 0);
  });

  // Button special/combo actions

  // Route to action "Factory Default Reset"
  server.on("/fdreset", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    digitalWrite(DO_LED, 1);
    Serial.println("WebServer->Facory Default Reset...");  // Debug output

    if (rcTUNE.getState() == LOW && rcLUP.getState() == LOW)
    {
      // Press TUNE & L-UP for 3000 milliseconds
      rcTUNE.pulse(3000);
      rcLUP.pulse(3000);
    }

    request->send(SPIFFS, "/index.html", "text/html");
    digitalWrite(DO_LED, 0);
  });

  // Route to action "Delete Antenna Memory"
  server.on("/damem", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    digitalWrite(DO_LED, 1);
    Serial.println("WebServer->Delete Antenna Memory...");  // Debug output

    if (rcTUNE.getState() == LOW && rcANT.getState() == HIGH)
    {
      // Press TUNE & ANT for 3000 milliseconds
      rcTUNE.pulse(3000);
      rcANT.pulse(3000);
    }

    request->send(SPIFFS, "/index.html", "text/html");
    digitalWrite(DO_LED, 0);    
  });

  // Route to action "Total Reset"
  server.on("/treset", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    digitalWrite(DO_LED, 1);
    Serial.println("WebServer->Total Reset...");  // Debug output

    if (rcTUNE.getState() == LOW && rcCUP.getState() == LOW && rcLUP.getState() == LOW)
    {
      // Press TUNE & CUP & LUP for 3000 milliseconds
      rcTUNE.pulse(3000);
      rcCUP.pulse(3000);
      rcLUP.pulse(3000);
    }
    
    request->send(SPIFFS, "/index.html", "text/html");
    digitalWrite(DO_LED, 0);   
  });

  // Start server
  server.begin();

  // ArduinoOTA setup
  ArduinoOTA.setHostname(myHostname.c_str());
  ArduinoOTA.setPort(8266);
  //ArduinoOTA.setPassword("YourOTApassphrase");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("ArduinoOTA->Start updating " + type);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nArduinoOTA->End");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("ArduinoOTA->Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("ArduinoOTA->Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("ArduinoOTA->Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("ArduinoOTA->Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("ArduinoOTA->Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("ArduinoOTA->Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("ArduinoOTA->End Failed");
    }
  });
  
  ArduinoOTA.begin();  

  // Reserve 100 bytes for strJSONString
  strJSONString.reserve(100);
}
 
void loop()
{
  String strTmp;

  ArduinoOTA.handle();

  rcCUP.loop();
  rcCDN.loop();
  rcLUP.loop();
  rcLDN.loop();
  rcTUNE.loop();
  rcANT.loop();

  // Read the LED pins (inverted by open-drain FET)
  if ( 0 == digitalRead(DI_LEDRD) )
  {
      u8isTuning = 1;
  }
  else
  {
    u8isTuning = 0;
  }

  if ( 0 == digitalRead(DI_LEDGN) )
  {
      u8SWRok = 1;
  }
  else
  {
    u8SWRok = 0;
  }

  // Build the JSON string:
  // {"u8isTuning":0,"u8SWRok":0,"esp_FW":"x.x","esp_MAC":"00:00:00:00:00:00"}
  strTmp  = "{\"u8isTuning\":";
  strTmp += String(u8isTuning, DEC);
  strTmp += ",\"u8SWRok\":";
  strTmp += String(u8SWRok, DEC);
  strTmp += ",\"esp_FW\":\"";
  strTmp += FW_Version;
  strTmp += "\",\"esp_MAC\":\"";
  strTmp += WiFi.macAddress();
  strTmp += "\"}";

  if ( strTmp != strJSONString )
  {
      strJSONString = strTmp;             // Copy the temporary input to the handling object
    Serial.println("JSON->" + strJSONString);      // Debug output
  } 
  
}
