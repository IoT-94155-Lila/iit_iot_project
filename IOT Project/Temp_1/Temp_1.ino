#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <HTTPClient.h>

/* -------- WiFi Credentials -------- */
const char* ssid = "TP-Link_51E3";
const char* password = "59223962";

/* -------- MQTT Broker -------- */
const char* mqtt_server = "192.168.0.194";
const int mqtt_port = 1883;
const char* mqtt_topic = "env/data";

/* -------- ThingSpeak -------- */
String thingSpeakAPIKey = "H6BOUJJ1CLT0Q327";   // WRITE API KEY

/* -------- DHT Sensor -------- */
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

/* -------- MQ-2 Sensor -------- */
#define MQ2_PIN 34

WiFiClient espClient;
PubSubClient client(espClient);

/* -------- WiFi Connect -------- */
void setup_wifi() {
  Serial.print("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

/* -------- MQTT Connect -------- */
void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32_Client_01")) {
      Serial.println("Connected");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying...");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // Sensor init
  dht.begin();

  // ESP32 ADC fix
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  delay(2000);   // DHT11 delay

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int gasValue = analogRead(MQ2_PIN);

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("DHT Read Error");
    return;
  }

  /* -------- Print Values (DEBUG) -------- */
  Serial.println("Sensor Readings:");
  Serial.print("Temperature: "); Serial.println(temperature);
  Serial.print("Humidity: "); Serial.println(humidity);
  Serial.print("Gas: "); Serial.println(gasValue);

  /* -------- MQTT JSON Payload -------- */
  String mqttPayload = "{";
  mqttPayload += "\"temperature\":";
  mqttPayload += temperature;
  mqttPayload += ",";
  mqttPayload += "\"humidity\":";
  mqttPayload += humidity;
  mqttPayload += ",";
  mqttPayload += "\"gas\":";
  mqttPayload += gasValue;
  mqttPayload += "}";

  client.publish(mqtt_topic, mqttPayload.c_str());
  Serial.println(" MQTT Published");

  /* -------- ThingSpeak Upload -------- */
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String url = "http://api.thingspeak.com/update?api_key=" + thingSpeakAPIKey +
                 "&field1=" + String(temperature) +
                 "&field2=" + String(humidity) +
                 "&field3=" + String(gasValue);

    Serial.println("ThingSpeak URL:");
    Serial.println(url);

    http.begin(url);
    int httpCode = http.GET();

    Serial.print("ThingSpeak HTTP Code: ");
    Serial.println(httpCode);

    http.end();
  }

  // ThingSpeak rate limit
  delay(2000);
}