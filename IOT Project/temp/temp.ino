#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <HTTPClient.h>

/* -------- WiFi Credentials -------- */
const char* ssid = "redmi 9i";
const char* password = "44445555";

/* -------- MQTT Broker -------- */
const char* mqtt_server ="192.168.0.194";
const int mqtt_port = 1883;
const char* mqtt_topic = "env/data";

/* -------- ThingSpeak -------- */
String thingSpeakAPIKey ="H6BOUJJ1CLT0Q327";   // WRITE API KEY

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
  Serial.println("Connecting to WiFi...");
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

    String clientId = "ESP32_Client_";
    clientId += String(random(0xFFFF), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("Connected");
    } else {
      Serial.print("Failed, State = ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  dht.begin();
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  randomSeed(micros());
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  delay(2000); // DHT11 required delay

  float temperature = NAN;
  float humidity = NAN;

  // Retry in case of DHT read failure
  for (int i = 0; i < 5; i++) {
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();
    if (!isnan(temperature) && !isnan(humidity)) break;
    delay(800);
  }

  int gasValue = analogRead(MQ2_PIN);

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("DHT Read Error — Skipping Upload");
    return;
  }

  /* -------- Print Values -------- */
  Serial.println("\nSensor Readings:");
  Serial.printf("Temperature: %.2f\n", temperature);
  Serial.printf("Humidity: %.2f\n", humidity);
  Serial.print("Gas: ");
  Serial.println(gasValue);

  /* -------- MQTT JSON Payload -------- */
  String mqttPayload =
    String("{\"temperature\":") + temperature +
    ",\"humidity\":" + humidity +
    ",\"gas\":" + gasValue + "}";

  client.publish(mqtt_topic, mqttPayload.c_str());
  Serial.println("MQTT Published");

  /* -------- ThingSpeak Upload -------- */
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String url =
      "http://api.thingspeak.com/update?api_key=" + thingSpeakAPIKey +
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

  delay(16000); // ThingSpeak 15-sec rate-limit
}