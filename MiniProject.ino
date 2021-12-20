#include <ESP8266WiFi.h>
#include "SinricPro.h"
#include "SinricProSwitch.h"
#include <map>

#define WIFI_SSID         "HOmemade"    
#define WIFI_PASS         "air584410"

#define APP_KEY           "f0df49ff-6c4a-4636-a766-19b029971778"      
#define APP_SECRET        "54db9a76-3672-45e3-a6b3-ef9dff145436-88dcbde0-6469-4e95-9196-15433decf1b1" 
  
#define Device_1  "61bc94b5695fce5c874d20d1"
#define Device_2  "61bc9550695fce5c874d2134"
#define Device_3  "61bc95890df86e5c8fe8de74"

#define wifiLed   16   //D0
#define RelayPin1 5  //D1
#define RelayPin2 4  //D2
#define RelayPin3 14 //D5
#define RelayPin4 12 //D6
#define SwitchPin1 10  //SD3
#define SwitchPin2 0   //D3 
#define SwitchPin3 13  //D7
#define SwitchPin4 3   //RX



#define BAUD_RATE   9600

#define DEBOUNCE_TIME 250

typedef struct {      // struct for the std::map below
  int relayPIN;
  int flipSwitchPIN;
} deviceConfig_t;


std::map<String, deviceConfig_t> devices = {
    //{deviceId, {relayPIN,  flipSwitchPIN}}
    {Device_1, {  RelayPin1, SwitchPin1 }},
    {Device_2, {  RelayPin2, SwitchPin2 }},
    {Device_3, {  RelayPin3, SwitchPin3 }}    
};

typedef struct {      // struct for the std::map below
  String deviceId;
  bool lastFlipSwitchState;
  unsigned long lastFlipSwitchChange;
} flipSwitchConfig_t;

std::map<int, flipSwitchConfig_t> flipSwitches;    

void setupRelays() { 
  for (auto &device : devices) {           
    int relayPIN = device.second.relayPIN; 
    pinMode(relayPIN, OUTPUT);             
    digitalWrite(relayPIN, HIGH);
  }
}

void setupFlipSwitches() {
  for (auto &device : devices)  {                     
    flipSwitchConfig_t flipSwitchConfig;              

    flipSwitchConfig.deviceId = device.first;         
    flipSwitchConfig.lastFlipSwitchChange = 0;        
    flipSwitchConfig.lastFlipSwitchState = true;     

    int flipSwitchPIN = device.second.flipSwitchPIN;  

    flipSwitches[flipSwitchPIN] = flipSwitchConfig;   
    pinMode(flipSwitchPIN, INPUT_PULLUP);             
  }
}

bool onPowerState(String deviceId, bool &state)
{
  Serial.printf("%s: %s\r\n", deviceId.c_str(), state ? "on" : "off");
  int relayPIN = devices[deviceId].relayPIN; 
  digitalWrite(relayPIN, !state);            
  return true;
}

void handleFlipSwitches() {
  unsigned long actualMillis = millis();                                          
  for (auto &flipSwitch : flipSwitches) {                                         
    unsigned long lastFlipSwitchChange = flipSwitch.second.lastFlipSwitchChange;  

    if (actualMillis - lastFlipSwitchChange > DEBOUNCE_TIME) {                    
      int flipSwitchPIN = flipSwitch.first;                                       
      bool lastFlipSwitchState = flipSwitch.second.lastFlipSwitchState;           
      bool flipSwitchState = digitalRead(flipSwitchPIN);                          
      if (flipSwitchState != lastFlipSwitchState) {                               
        flipSwitch.second.lastFlipSwitchState = flipSwitchState;                  
      }
    }
  }
}

void setupWiFi()
{
  Serial.printf("\r\n[Wifi]: Connecting");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.printf(".");
    delay(250);
  }
  digitalWrite(wifiLed, LOW);
  Serial.printf("connected!\r\n[WiFi]: IP-Address is %s\r\n", WiFi.localIP().toString().c_str());
}

void setupSinricPro()
{
  for (auto &device : devices)
  {
    const char *deviceId = device.first.c_str();
    SinricProSwitch &mySwitch = SinricPro[deviceId];
    mySwitch.onPowerState(onPowerState);
  }

  SinricPro.begin(APP_KEY, APP_SECRET);
  SinricPro.restoreDeviceStates(true);
}

void setup()
{
  Serial.begin(BAUD_RATE);

  pinMode(wifiLed, OUTPUT);
  digitalWrite(wifiLed, HIGH);

  setupRelays();
  setupFlipSwitches();
  setupWiFi();
  setupSinricPro();
}

void loop()
{
  SinricPro.handle();
  handleFlipSwitches();
}
