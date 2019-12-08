/*
  Battery Button V1
  2018/06/02
  by OrgJvR

  This code uses a state machine during the loop.
  Minimum requirements:
    1. A runnable ESP (check the minimum connections)
    2. A button between RST and GND. This will wake ESP and it will send a MQTT message.
  Optional requirements:
    1. A button to force ESP into setup mode (Need to define enableOTA)
    2. A LED to indicate status (Need to define USE_LED). Dont use GPIO0!

  ToDo:
    1. AP mode to setup Wifi and MQTT parameters.
    2. Maybe go to AP mode after 10 seconds if it cannot connect to wifi.
*/
//#define enableOTA // You need at least 1MB flash for OTA to work. If used, go to http://bb-webupdate.local in web browser.
//#define DEBUG // This cannot be used with enableOTA if SETUP_BUTTON=3!
//#define USE_LED

//**********************************************************************
// INCLUDE
//**********************************************************************
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClient.h>

// If you enable the define below, you need a file called private.h 
//    with the same lines as the ELSE below. This will not be uploaded to git.
#define hasPrivateCredentials 

#ifdef hasPrivateCredentials
  #include "private.h"
#else
  const char* ssid = "ssid";
  const char* password = "wifi_password";
  // MQTT
  const char* mqtt_server = "192.168.10.5";
  const char* mqtt_user = "mqttusername";
  const char* mqtt_pass = "mqttpassword";
  const int mqtt_port = 1883;
#endif


#ifdef enableOTA
  #include <ESP8266WebServer.h>
  #include <ESP8266mDNS.h>
  #include <ESP8266HTTPUpdateServer.h>
#endif

void setup_wifi();
void reconnect();
void getUID(char cMac[]) ;
void getInfo();

//**********************************************************************
// DEFINE
//**********************************************************************
#ifdef USE_LED
  //#define LED 2               // GPIO2 - Onboard LED op die ESP12
  #define LED 1               // GPIO1 - Onboard LED op die ESP01
#endif

#ifdef enableOTA
  // Button for Webupdate. Dont use GPIO0!
  //#define SETUP_BUTTON 12     // Use GPIO12 on ESP-12
  #define SETUP_BUTTON 3     // Use TX pin for Setup Button on ESP-01
#endif

//**********************************************************************
// PARAMETER
//**********************************************************************
// Wifi
#ifdef DEBUG
  unsigned long previousMillis = 0;   // will store last time LED was updated
  long sendtime = 0;                  // sendtime
#endif
#ifdef enableOTA
  const char* host = "bb-webupdate";
  const char* update_path = "/firmware";
  const char* update_username = "admin";
  const char* update_password = "BigButt";
#endif
char mqtt_clientId[10];
char outTopic[25] = "/BatteryButton/";

// Hardware
#ifdef USE_LED
  #ifndef DEBUG
    unsigned long previousMillis = 0;   // will store last time LED was updated
  #endif
  int ledState = HIGH;                // ledState used to set the LED
  const long interval = 500;          // interval at which to blink (milliseconds)
#endif
char msg[10];                       // message for mqtt publish
#ifdef enableOTA
  boolean mSetupButton = LOW;         // flag Setup Button
  int state = 0;                      // Stratup state for Statemachine
#else
  int state = 1;                      // Startup state for Statemachine
#endif
//**********************************************************************
// SETUP
//**********************************************************************
ADC_MODE (ADC_VCC);                 // VCC Read

#ifdef enableOTA
  ESP8266WebServer httpServer(80);
  ESP8266HTTPUpdateServer httpUpdater;
#endif

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  #ifdef enableOTA
    pinMode(SETUP_BUTTON, INPUT_PULLUP);    // Webupdate Button
  #endif
  #ifdef USE_LED
    pinMode(LED, OUTPUT);                   // Onboard LED as Output
  #endif
  #ifdef DEBUG
    sendtime = millis();
    //  Serial.begin(115200);
    Serial.begin(74880);
  //Print Info about module;
  //getInfo();
  #endif

  
  // Setup Wifi
  setup_wifi();

  // Get mac
  #ifdef DEBUG
    Serial.println("getUID(mqtt_clientId):");
  #endif
  getUID(mqtt_clientId);
  #ifdef DEBUG
    Serial.print("mqtt_clientId:");
    Serial.println(mqtt_clientId);
  #endif
  strcat(outTopic,mqtt_clientId);
  
  // Setup MQTT
  client.setServer(mqtt_server, mqtt_port);

  // Setup httpUpdater
#ifdef enableOTA
  MDNS.begin(host);
  //httpUpdater.setup(&httpServer);
  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  httpServer.begin();
  MDNS.addService("http", "tcp", 80);
#endif
}

void setup_wifi() {
  int WaitTime = 0;
  int loop_cnt = 0;
  delay(10);
  // We start by connecting to a WiFi network
  #ifdef DEBUG
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
  #endif
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED && loop_cnt < 50) {
    delay(50);
    WaitTime+=50;
    #ifdef DEBUG
      WaitTime+=450;
      delay(450); //delay a further 450ms to not flood the user with info
      Serial.print(".");
    #endif
    /*if (WaitTime>15000) {
      //After 15 seconds start the AP
      WiFi.disconnect();
      WiFi.softAP("softAP","123456789000", 1, false);
    }*/
    loop_cnt++;
  }

  #ifdef DEBUG
    Serial.println("");
    Serial.println("WiFi connected if valid ip address below");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  #endif
}

void reconnect() {
  int loop_cnt = 0;
  // Loop until we're reconnected
  while (!client.connected() && loop_cnt < 10) {
    #ifdef DEBUG
      Serial.println("Attempting MQTT connection...");
    #endif
    // Client ID connected
    if (client.connect(mqtt_clientId, mqtt_user, mqtt_pass)) {
      #ifdef DEBUG
        Serial.print(mqtt_clientId);
        Serial.println(" connected");
      #endif
      // Once connected, publish an announcement...
      client.publish("outTopic", "connected");
    } else {
      #ifdef DEBUG
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
      #endif
      // Wait 5 seconds before retrying
      delay(500);
    }
    loop_cnt++;
  }
}

void getUID(char cMac[]) {
  byte mac[6];
  WiFi.macAddress(mac);
  for (int i = 3; i < 6; i++)
  {
        byte nib1 = (mac[i] >> 4) & 0x0F;
        byte nib2 = (mac[i] >> 0) & 0x0F;
        cMac[(i-3)*2+0] = nib1  < 0xA ? '0' + nib1  : 'A' + nib1  - 0xA;
        cMac[(i-3)*2+1] = nib2  < 0xA ? '0' + nib2  : 'A' + nib2  - 0xA;
  }
  cMac[6] = '\0';
}

void getInfo() {
  #ifdef DEBUG
    Serial.println();
    Serial.print("Chip ID: ");
    Serial.println(ESP.getFlashChipId());
    Serial.print("Chip Real Size: ");
    Serial.println(ESP.getFlashChipRealSize());
    Serial.print("Chip Size: ");
    Serial.println(ESP.getFlashChipSize());
    Serial.print("Chip Speed: ");
    Serial.println(ESP.getFlashChipSpeed());
    Serial.print("Chip Mode: ");
    Serial.println(ESP.getFlashChipMode());
  #endif
}


//**********************************************************************
// LOOP
//**********************************************************************
void loop() {

  #ifdef enableOTA
    // HTTPServer Handle Client
    httpServer.handleClient();
  #endif

  // MQTT Client
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  #ifdef DEBUG
    // LED blink when ESP is in Upload Mode
    unsigned long currentMillis = millis();

    #ifdef USE_LED
      if (currentMillis - previousMillis >= interval) {
        // save the last time you blinked the LED
        previousMillis = currentMillis;

        // if the LED is off turn it on and vice-versa:
        if ((ledState == LOW) && (mSetupButton = HIGH)) {
          ledState = HIGH;
        } else {
          ledState = LOW;
        }
        // set the LED with the ledState of the variable:
        digitalWrite(LED, ledState);
      }
    #endif
  #endif

  // Read the VCC from Battery
  float vcc = ESP.getVcc() / 1000.0;
  vcc = vcc - 0.12;     // correction value from VCC

  // StateMachine for send the telegram to MQTT
  switch (state) {
    case 0:    // SETUP
      #ifdef enableOTA
        if (digitalRead(SETUP_BUTTON) == LOW) {
          mSetupButton = HIGH;
          #ifdef DEBUG
            Serial.println("Webupdate started ...");
            Serial.printf("Open http://%s.local%s in your browser and login with username '%s' and password '%s'\n", host, update_path, update_username, update_password);
          #endif
          state = 0;
        } else {
          state = 1;
        }
        // This cause state machine to always stay in state 0 if it previously was in state 0 (Until reset).
        if (mSetupButton == HIGH) {
          state = 0;
        }
      #endif
      break;
    case 1:    // Send MQTT message
      dtostrf(vcc, sizeof(vcc), 2, msg);
      client.publish(outTopic, msg);
      #ifdef DEBUG
        Serial.print("VCC: ");
        Serial.print(msg);
        Serial.println("V");
      #endif
      state = 2;
      break;
    case 2:    // Debug and go to Deepsleep
      #ifdef DEBUG
        sendtime = millis() - sendtime;
        Serial.print("Sendtime: ");
        Serial.print(sendtime);
        Serial.println("ms");
        Serial.print("Good Night ...");
      #endif
      ESP.deepSleep(0, WAKE_RFCAL);
      break;
  }
  delay(100);  // delay in between reads for stability
}
