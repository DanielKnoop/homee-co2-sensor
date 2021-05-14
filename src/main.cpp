#include <Arduino.h>
#include "virtualHomee.hpp"
#include <SoftwareSerial.h>

#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

virtualHomee vhih;
nodeAttributes* co2;
nodeAttributes* temperature;
SoftwareSerial co2Serial(D2, D1);

void setup() {
  co2Serial.begin(9600);
  Serial.begin(115200);

  WiFi.begin(pSSID, pWLANPASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  node* mhz19b = new node(10, 3002, "CO2 Sensor");
  co2 = mhz19b->AddAttributes(new nodeAttributes(20));
  co2->setMinimumValue(400);
  co2->setMaximumValue(5000);
  co2->setEditable(0);
  co2->setUnit("ppm");

  temperature = mhz19b->AddAttributes(new nodeAttributes(5));
  temperature->setMinimumValue(-40);
  temperature->setMaximumValue(60);
  temperature->setEditable(0);
  temperature->setUnit("°C");

  vhih.addNode(mhz19b);
  vhih.start();
}

byte getCheckSum(byte *packet) {
  byte i;
  byte checksum = 0;
  for (i = 1; i < 8; i++) {
    checksum += packet[i];
  }
  checksum = 0xff - checksum;
  checksum += 1;
  return checksum;
}

bool readSensor(int *ppm, int *temperature){
  byte cmd[9] = {0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79};
  byte response[9]; 
  co2Serial.write(cmd, 9);
  memset(response, 0, 9);
  unsigned long waitMillis = millis();
  while (co2Serial.available() == 0) {
    yield();
    if(millis() - waitMillis > 2000)
    {
      Serial.println("Keine Antwort vom Sensor erhalten");
      return false;
    }
  }
  co2Serial.readBytes(response, 9);
  byte check = getCheckSum(response);
  
  if (response[8] != check) {
    Serial.println("Fehler in der Übertragung!");
    return false;
  }
 
  *ppm = (static_cast<unsigned int>(response[2]) << 8) + response[3];
  unsigned int mhzRespTemp = (unsigned int)response[4];
  *temperature = mhzRespTemp - 40;
  return true;
}

unsigned long previousMillis = 0;
const long interval = 60000; 

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    int ppm = 0;
    int temp = 0;

    if(readSensor(&ppm, &temp))
    {
      Serial.print("PPM: ");
      Serial.print(ppm);
      Serial.print(" Temperature: ");
      Serial.println(temp);
      
      co2->setCurrentValue(ppm);
      temperature->setCurrentValue(temp);

      vhih.updateAttribute(co2);
      vhih.updateAttribute(temperature);
    }
  }
  yield();
}