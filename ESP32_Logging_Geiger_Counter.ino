/***********************************
***          BOARD SETUP         ***
***********************************/

// Follow these steps to prepare your Arduino environment for programming the ESP32

// Use board: "ESP32 Dev Module" (with all defaults)


/***********************************
***        INTERNET SETUP        ***
***********************************/

// Follow these steps to set up a free online data-logging and charting endpoint:-

// 1. Visit https://iotplotter.com/
// 2. Sign up for a new account and verify your email address.
// 3. Click "Create New Feed" and give it a name (e.g. "Nuclear Radiation counts in (your city) (your country)"
// 4. Optionally enter a description (e.g. "This is the output from a Geiger Counter measuring radiation particles. It counts them, and every 1 minute uploads the count.  See https://github.com/gitcnd/ESP32_Logging_Geiger_Counter ")
// 5. Record your API key below:
#define IOT_API_ID "put your ID here"
// 6. Click "Manage Feed" and then "Keys" and record your API key below:
#define IOT_APIKEY "put your key here"


/***********************************
***       YOUR WIFI SETUP        ***
***********************************/

// Follow these steps to get your wifi access point name and password into your ESP32

#define WIFI_SSID "Your 2.4ghz WiFi SSID here"
#define WIFI_PASSWORD "your wifi password here"

/*******************************************************/
// Code is below - no need to change anyhing after here
/*******************************************************/



#include "SerialID.h"	// So we know what code and version is running inside our MCUs (outputs this over serial at boot)
SerialIDset("\n#\tv1.01 " __FILE__ "\t" __DATE__ " " __TIME__);

/***********************************
***           LED Init.          ***
***********************************/

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif
int tog=0;
int ledon=10;    // Can lengthen this (e.g. 200) to indicate error states
int ledoff=1000;  // Can shorten this (e.g. 200) to indicate errors
unsigned long timer=millis();

/***********************************
***           WiFi Init.         ***
***********************************/


#include <WiFi.h>
#include <mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>


// #include "ESP8266WiFi.h"
#include <HTTPClient.h>

HTTPClient http;
const char* host = "91.103.1.85"; // geekbox - change this to whatever IP address you want to use
const int  port = 80;
const int  watchdog = 5000;
const int  onemin = 60000;
unsigned long previousMillis = millis(); 
unsigned long previousMillis2 = millis(); 
int myloop=0;
int numberOfNetworks=0;

uint64_t uploaded16=0;

volatile uint64_t count16=0;
volatile uint32_t last16=0;
volatile uint32_t now16=0;
volatile uint32_t delta16=0;

#define COUNTBUFSZ 20000
#define MAXPTR COUNTBUFSZ / sizeof(uint32_t) - 1
volatile uint32_t tickbuf[MAXPTR+1];
volatile int ptr16=0;

void IRAM_ATTR ISR16() {
    now16=micros();
    count16++;
    delta16=now16-last16;
    last16=now16;
    if(ptr16<MAXPTR) {
      tickbuf[ptr16++]=delta16;
      ptr16--; // skip buffering every tick in thie release
    }
}


/* ************************************* setup() ************************************* */

void setup() {

/***********************************
***      Serial Output SetUp     ***
***********************************/
  
  SerialIDshow(115200); // starts Serial & prints version etc.
  Serial.setDebugOutput(true);



/***********************************
***           LED SetUp          ***
***********************************/
  pinMode(LED_BUILTIN,OUTPUT);



/***********************************
***           WiFi SetUp         ***
***********************************/
  numberOfNetworks = WiFi.scanNetworks();
//  int cell=0; //int dy=0;
 
  for(int i =0; i<numberOfNetworks; i++){
      Serial.print("Network name: "); 
//      if(WiFi.SSID(i)==MOBILE_SSID)cell++;
      Serial.println(WiFi.SSID(i));
      Serial.print("Signal strength: ");
      Serial.println(WiFi.RSSI(i));
      Serial.println("-----------------------");
  }

//  if(cell) { // prioritize connecting to my mobile cell, so we can use this temp connection to set the device wifi settings later
//    const char* ssid = MOBILE_SSID; // "CryptoPhoto Blocks Phishing";
//    const char* password = MOBILE_PASSWORD;
//    WiFi.begin(ssid, password);
//  } else {
    const char* ssid = WIFI_SSID; // "Bandido Nation Weapons HQ";
    const char* password = WIFI_PASSWORD;
    WiFi.begin(ssid, password);
//  }
  
  int timeout=600; Serial.print("Connecting");
  while ((WiFi.status() != WL_CONNECTED)&&(timeout>0)) {
    delay(100);
    Serial.print(".");
    timeout--;
  }
 
  Serial.println(WiFi.localIP());
  


// OTA Setup

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.print("OTA Ready; IP address: ");
  Serial.println(WiFi.localIP());

  // Geiger counter ISR
  pinMode(16,INPUT);
  attachInterrupt(16, ISR16, RISING);

} // setup



char buf[21];
char *print64(uint64_t n) {
  char *str = &buf[sizeof(buf) - 1];
  *str = '\0';
  do {
    uint64_t m = n;
    n /= 10;
    *--str = m - 10*n + '0';
  } while (n);
  return str;
}

void iotLog() {
    if(WiFi.status()== WL_CONNECTED){
      WiFiClient client;
      HTTPClient http;
      uint64_t prev16;
    
      // Your Domain name with URL path or IP address with path
      http.begin(client, "http://iotplotter.com/api/v2/feed/" + String(IOT_API_ID) );

      // Specify content-type header
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      http.addHeader("api-key", IOT_APIKEY );
      // Data to send with HTTP POST
      prev16=count16; // Get an instantaneous copy of the counter
      String httpRequestData = "{\"data\":{\"Particle_Counts_Per_Minute\":[{\"value\":";
      httpRequestData += print64(prev16-uploaded16);
      uploaded16=prev16; // Remember how much we have sent so far
      httpRequestData +="}]}}";
      // Send HTTP POST request
      int httpResponseCode = http.POST(httpRequestData);
      
      Serial.print("iotplotter HTTP Response code: ");
      Serial.println(httpResponseCode);
        
      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }  
} // iotLog



void loop() {

  ArduinoOTA.handle(); // Checks if a new sketch is about to be uploaded "Over The Air" (OTA) = via Wifi.

  // Blink the LED so we know we are alive
  if(timer<millis()) {
    tog++; digitalWrite(LED_BUILTIN, tog&1);
    timer=millis();
    if(tog&1) { timer+=ledon; } else {timer+=ledoff;} // NB: LED is wired backwards (low=on)
  }

  if( previousMillis2+onemin < millis()) {
    previousMillis2 = millis(); 
    iotLog();
  }

  delay(10); // 0.01 seconds sleep

} // loop
