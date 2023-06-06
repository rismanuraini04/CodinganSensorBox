#include <ArduinoJson.h>
#include <Arduino.h>
#include <EEPROM.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <HX711.h>
#define BTN 4
#define DT1 33
#define SCK1 32

const char *MQTT_SERVER = "192.168.28.149";
const char *MQTT_USERNAME = "risma";
const char *MQTT_PASSWORD = "1234";
unsigned long sendtimestamp = 0;
unsigned long displayTimeStamp = 0;
int interval = 5000;
int intervalDisplay = 2000;
int lastWeight1 = 0;
float calibration_factor1 = 191.50;

String dataEEPROM = "";
String deviceId = "";

WiFiManager wm;
WiFiClient espClient;
PubSubClient client(espClient);
LiquidCrystal_I2C lcd(0x27, 16, 2);
HX711 scale1(DT1, SCK1);

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String listenTo = "reminder-" + deviceId;
  String mqttTopiq = String(topic);
  const char *topicChar = listenTo.c_str();
  if (mqttTopiq == listenTo)
  {
    Serial.println("Buzzer bunyi");
  }
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("esp", MQTT_USERNAME, MQTT_PASSWORD))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      String topic = "reminder-" + deviceId;
      const char *topicChar = topic.c_str();
      client.subscribe(topicChar);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

String readFromEEPROM(int addrOffset)
{
  int newStrLen = EEPROM.read(addrOffset);
  char data[newStrLen + 1];
  for (int i = 0; i < newStrLen; i++)
  {
    data[i] = EEPROM.read(addrOffset + 1 + i);
  }
  data[newStrLen] = '\0';
  return String(data);
}

void writeToEEPROM(int addrOffset, const String &strToWrite)
{
  int len = strToWrite.length();
  Serial.println("Store new data to EEPROM " + strToWrite);
  EEPROM.write(addrOffset, len);
  for (int i = 0; i < len; i++)
  {
    EEPROM.write(addrOffset + 1 + i, strToWrite[i]);
    EEPROM.commit();
  }
}

void setupWiFi()
{
  bool res;

  res = wm.autoConnect("SmartMedicineBox", "password"); // password protected ap

  if (!res)
  {
    Serial.println("Failed to connect");
    // ESP.restart();
  }
  else
  {
    // if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }
}

void setup()
{
  // put your setup code here, to run once:
  lcd.init();
  Serial.begin(9600);
  EEPROM.begin(512);
  scale1.set_scale(calibration_factor1);
  scale1.tare();

  pinMode(BTN, INPUT_PULLDOWN);

  setupWiFi();

  dataEEPROM = readFromEEPROM(1);

  if (dataEEPROM == "")
  {
    Serial.println("Data Kosong"); // requestkeserver ip 8000
    HTTPClient http;

    http.begin("http://192.168.28.149:8000/api/v1/smartbox/generateid");
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST("");

    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    String responsebody = http.getString();
    Serial.print("HTTP Body");
    Serial.println(responsebody);
    StaticJsonDocument<512> doc;

    DeserializationError error = deserializeJson(doc, responsebody);

    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    String uniqcode = doc["box"]["uniqCode"];
    Serial.println(uniqcode);

    writeToEEPROM((1), uniqcode);
    deviceId = uniqcode;
  }

  if (dataEEPROM != "")
  {
    Serial.print("ID adalah ");
    Serial.println(dataEEPROM);
    deviceId = dataEEPROM;
  }

  lcd.backlight();

  client.setServer(MQTT_SERVER, 1883);
  client.setCallback(callback);
}

void loop()
{
  // put your main code here, to run repeatedly:

  int weight1 = scale1.get_units();

  if (digitalRead(BTN) == HIGH)
  {
    wm.resetSettings();
    ESP.restart();
  }

  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  if (millis() - sendtimestamp > interval)
  {
    sendtimestamp = millis();
    Serial.print("Mengirim pesan");
    String topic = "/smartbox/update/weight/" + deviceId;
    const char *topicChar = topic.c_str();
    int berat1 = scale1.get_units();
    String berat1str = String(berat1);
    String payload = "{\"id\":\"" + deviceId + "\",\"Box 1\":\"" + berat1str + "\",\"Box 2\":\"8\"}";
    const char *payloadChar = payload.c_str();
    client.publish(topicChar, payloadChar);
    Serial.println(payload);
  }

  if (millis() - displayTimeStamp > intervalDisplay)
  {
    displayTimeStamp = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ID: " + deviceId);
    String displayBerat1 = "B1:" + String(weight1);
    lcd.setCursor(0, 1);
    lcd.print(displayBerat1);
  }
}
