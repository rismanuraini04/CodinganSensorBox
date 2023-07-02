#include <ArduinoJson.h>
#include <Arduino.h>
// #include <EEPROM.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <HX711.h>
#include <Preferences.h>
#define BTN 4
#define DT1 33
#define SCK1 32
#define DT2 25
#define SCK2 26
#define BUZZER_PIN 15
#define PIR1_PIN 14
#define PIR2_PIN 27

const char *MQTT_SERVER = "103.139.192.253";
const char *MQTT_USERNAME = "skripsimqtt";
const char *MQTT_PASSWORD = "@YZ7rqrLIDJ^!Qrz";
unsigned long sendtimestamp = 0;
unsigned long displayTimeStamp = 0;
int interval = 5000;
int intervalDisplay = 2000;
float calibration_factor1 = 191.50;
float calibration_factor2 = 190.00;
bool buzzerTriggered1 = false;
bool buzzerTriggered2 = false;
bool medicineTaken1 = false;
bool medicineTaken2 = false;
bool medicineNotTaken1 = false;
bool medicineNotTaken2 = false;
bool isFirstMovementPir1 = false;
bool isSecondMovementPir1 = false;
bool isFirstMovementPir2 = false;
bool isSecondMovementPir2 = false;
unsigned long buzzerStartTime1;
unsigned long buzzerStartTime2;
unsigned long firstMovementTimeStamp1;
unsigned long firstMovementTimeStamp2;

String dataPreferences = "";
String deviceId = "";
String schedule = "";

WiFiManager wm;
WiFiClient espClient;
PubSubClient client(espClient);
LiquidCrystal_I2C lcd(0x27, 16, 2);
HX711 scale1(DT1, SCK1);
HX711 scale2(DT2, SCK2);
Preferences preferences;

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String payloadStr = "";
  String listenTo = "reminder-" + deviceId;
  String mqttTopiq = String(topic);
  const char *topicChar = listenTo.c_str();

  for (int i = 0; i < length; i++)
  {
    payloadStr += (char)payload[i];
    // Serial.print((char)payload[i]);
  }
  // Serial.println();
  Serial.println(payloadStr);

  if (mqttTopiq == listenTo + "-Box 1")
  {
    schedule = payloadStr;
    buzzerTriggered1 = true;
    buzzerStartTime1 = millis();
    Serial.println("Buzzer bunyi");
    digitalWrite(BUZZER_PIN, HIGH); // Buzzer ON
    delay(300);                     // Delay
    digitalWrite(BUZZER_PIN, LOW);  // Buzzer OFF
    delay(300);                     // Delay
    digitalWrite(BUZZER_PIN, HIGH); // Buzzer ON
    delay(300);                     // Delay
    digitalWrite(BUZZER_PIN, LOW);  // Buzzer OFF
    delay(300);                     // Delay
    digitalWrite(BUZZER_PIN, HIGH); // Buzzer ON
    delay(300);                     // Delay
    digitalWrite(BUZZER_PIN, LOW);  // Buzzer OFF
    delay(300);                     // Delay
  }

  if (mqttTopiq == listenTo + "-Box 2")
  {
    schedule = payloadStr;
    buzzerTriggered2 = true;
    buzzerStartTime2 = millis();
    Serial.println("Buzzer bunyi");
    digitalWrite(BUZZER_PIN, HIGH); // Buzzer ON
    delay(300);                     // Delay
    digitalWrite(BUZZER_PIN, LOW);  // Buzzer OFF
    delay(300);                     // Delay
    digitalWrite(BUZZER_PIN, HIGH); // Buzzer ON
    delay(300);                     // Delay
    digitalWrite(BUZZER_PIN, LOW);  // Buzzer OFF
    delay(300);                     // Delay
    digitalWrite(BUZZER_PIN, HIGH); // Buzzer ON
    delay(300);                     // Delay
    digitalWrite(BUZZER_PIN, LOW);  // Buzzer OFF
    delay(300);
  }
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("medicine", MQTT_USERNAME, MQTT_PASSWORD))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      String topic1 = "reminder-" + deviceId + "-Box 1";
      const char *topic1Char = topic1.c_str();
      String topic2 = "reminder-" + deviceId + "-Box 2";
      const char *topic2Char = topic2.c_str();
      client.subscribe(topic1Char);
      client.subscribe(topic2Char);
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

String readFromPreferences(const char *key)
{
  return preferences.getString(key, "");
}

void writeToPreferences(const char *key, const String &value)
{
  preferences.putString(key, value);
  preferences.end();
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
  preferences.begin("my-preferences");
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PIR1_PIN, INPUT);
  pinMode(PIR2_PIN, INPUT);

  scale1.set_scale(calibration_factor1);
  scale1.tare();

  scale2.set_scale(calibration_factor2);
  scale2.tare();

  pinMode(BTN, INPUT_PULLDOWN);

  setupWiFi();

  dataPreferences = readFromPreferences("deviceId");

  if (dataPreferences == "")
  {
    Serial.println("Data Kosong"); // requestkeserver ip 8000
    HTTPClient http;

    http.begin("http://103.139.192.253:8400/api/v1/smartbox/generateid");
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

    writeToPreferences("deviceId", uniqcode);
    deviceId = uniqcode;
  }

  if (dataPreferences != "")
  {
    Serial.println("data tidak kosong");
    Serial.print("ID adalah ");
    Serial.println(dataPreferences);
    deviceId = dataPreferences;
  }

  lcd.backlight();

  client.setServer(MQTT_SERVER, 1883);
  client.setCallback(callback);
}

void loop()
{
  unsigned long currentTime = millis();

  int weight1 = scale1.get_units();
  int weight2 = scale2.get_units();
  int previousWeight1 = scale1.get_units(); // Berat sebelumnya
  int currentWeight1 = scale1.get_units();  // Berat saat ini
  int pir1State = digitalRead(PIR1_PIN);
  int pir2State = digitalRead(PIR2_PIN);

  // PIR 1

  if (buzzerTriggered1 && isFirstMovementPir1 == false && !medicineTaken1 && pir1State == HIGH && (millis() - buzzerStartTime1 <= 120000))
  {
    // buzzerTriggered = true;
    Serial.println("Gerakan pertama PIR 1 terdeteksi");
    isFirstMovementPir1 = true;
    firstMovementTimeStamp1 = millis();
    delay(1000);
  }

  if (buzzerTriggered1 == true && isFirstMovementPir1 == true && pir1State == HIGH && (millis() - firstMovementTimeStamp1 >= 5000))
  {
    Serial.println("Gerakan kedua PIR 1 terdeteksi");
    Serial.println("Obat telah diminum");
    Serial.print("Mengirim pesan");
    String topic = "/smartbox/update/history/" + deviceId;
    const char *topicChar = topic.c_str();
    String payload = "{\"id\":\"" + deviceId + "\",\"box\":\"" + "Box 1" + "\"}";
    const char *payloadChar = payload.c_str();
    client.publish(topicChar, payloadChar);
    Serial.println(payload);

    medicineTaken1 = true; // Setel variabel status menjadi true karena obat telah diminum
    buzzerTriggered1 = false;
    isFirstMovementPir1 = false;
    // Mengatur variabel medicineTaken kembali ke false setelah tindakan selesai
    medicineTaken1 = false;
  }

  if (buzzerTriggered1 && !medicineTaken1 && isFirstMovementPir1 == false && (millis() - buzzerStartTime1 > 120000))
  {
    Serial.println("OBAT TIDAK DIMINUM");
    delay(1000);
    // Setel variabel status obat tidak diminum menjadi true
    medicineNotTaken1 = true;
    buzzerTriggered1 = false;
    String topic = "/smartbox/update/not-taken/history/" + deviceId;
    const char *topicChar = topic.c_str();
    String payload = "{\"id\":\"" + deviceId + "\",\"box\":\"" + "Box 1" + "\",\"schedule\":\"" + schedule + "\"}";
    const char *payloadChar = payload.c_str();
    client.publish(topicChar, payloadChar);
    Serial.println(payload);
    schedule = "";

    medicineNotTaken1 = false;
  }

  // PIR 2

  if (buzzerTriggered2 && isFirstMovementPir2 == false && !medicineTaken2 && pir2State == HIGH && (millis() - buzzerStartTime2 <= 120000))
  {
    // buzzerTriggered = true;
    Serial.println("Gerakan pertama PIR 2 terdeteksi");
    isFirstMovementPir2 = true;
    firstMovementTimeStamp2 = millis();
    delay(1000);
  }

  if (buzzerTriggered2 == true && isFirstMovementPir2 == true && pir2State == HIGH && (millis() - firstMovementTimeStamp2 >= 5000))
  {
    Serial.println("Gerakan kedua PIR 2 terdeteksi");
    Serial.println("Obat telah diminum");
    Serial.print("Mengirim pesan");
    String topic = "/smartbox/update/history/" + deviceId;
    const char *topicChar = topic.c_str();
    String payload = "{\"id\":\"" + deviceId + "\",\"box\":\"" + "Box 2" + "\"}";
    const char *payloadChar = payload.c_str();
    client.publish(topicChar, payloadChar);
    Serial.println(payload);

    medicineTaken2 = true; // Setel variabel status menjadi true karena obat telah diminum
    buzzerTriggered2 = false;
    isFirstMovementPir2 = false;
    // Mengatur variabel medicineTaken kembali ke false setelah tindakan selesai
    medicineTaken2 = false;
  }

  if (buzzerTriggered2 && !medicineTaken2 && isFirstMovementPir2 == false && (millis() - buzzerStartTime2 > 120000))
  {
    Serial.println("OBAT TIDAK DIMINUM");
    delay(1000);
    // Setel variabel status obat tidak diminum menjadi true
    medicineNotTaken2 = true;
    buzzerTriggered2 = false;
    String topic = "/smartbox/update/not-taken/history/" + deviceId;
    const char *topicChar = topic.c_str();
    String payload = "{\"id\":\"" + deviceId + "\",\"box\":\"" + "Box 2" + "\",\"schedule\":\"" + schedule + "\"}";
    const char *payloadChar = payload.c_str();
    client.publish(topicChar, payloadChar);
    Serial.println(payload);
    schedule = "";

    medicineNotTaken2 = false;
  }

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
    int berat2 = scale2.get_units();
    String berat2str = String(berat2);
    String payload = "{\"id\":\"" + deviceId + "\",\"Box 1\":\"" + berat1str + "\",\"Box 2\":\"" + berat2str + "\"}";
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
    String displayBerat2 = "B2:" + String(weight2);
    lcd.setCursor(0, 1);
    lcd.print(displayBerat1);
    lcd.setCursor(6, 1);
    lcd.print(displayBerat2);
  }
}
