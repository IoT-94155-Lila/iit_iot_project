#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

#define DHTPIN 4
#define DHTTYPE DHT11
#define MQ2PIN 34

const char* ssid = "TP-Link_51E3_5G";
const char* password = "59223962";
const char* mqtt_server = "broker.emqx.io"; 
const char* mqtt_topic = "env/data";

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  dht.begin();
  Serial.println(" SMART ENVIRONMENT MONITORING SYSTEM");
  
  // WiFi
  WiFi.begin(ssid, password);
  Serial.print("WiFi Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n WiFi Connected: " + WiFi.localIP().toString());
  
  // MQTT
  client.setServer(mqtt_server, 1883);
}

void loop() {
  if (!client.connected()) {
    Serial.println(" MQTT Reconnecting...");
    reconnect();
  }
  client.loop();
  
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int gas = analogRead(MQ2PIN);
  
  Serial.print("Temperature: "); Serial.print(temp); Serial.println(" °C");
  Serial.print("Humidity:    "); Serial.print(hum); Serial.println(" %");
  Serial.print("Gas Level:   "); Serial.println(gas);

  String payload = String(temp) + "," + String(hum) + "," + String(gas);
  if (client.publish(mqtt_topic, payload.c_str())) {
    Serial.println(" SENT TO SERVER: " + payload);
  } else {
    Serial.println(" MQTT SEND FAILED!");
  }
  
  Serial.println(" Next reading in 10 seconds...\n");
  delay(10000);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print(" MQTT Connecting to ");
    Serial.println(mqtt_server);
    if (client.connect("ESP32_Sensor")) {
      Serial.println(" MQTT Connected!");
    } else {
      Serial.print(" Failed! Retrying in 5s...");
      delay(5000);
    }
  }
}