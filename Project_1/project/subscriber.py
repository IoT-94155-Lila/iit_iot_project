import paho.mqtt.client as mqtt
import mysql.connector
import requests
from datetime import datetime

API_KEY = "1QTM8EMHWR5GN5LV"
print(f" API Key OK: {API_KEY}")

db = mysql.connector.connect(host="localhost", user="root", password="root", database="env_db")
cursor = db.cursor()

def on_message(client, userdata, msg):
    print(f" ESP32 SENT: {msg.payload.decode()}")
    
    data = msg.payload.decode().split(',')
    temp, hum, gas = float(data[0]), float(data[1]), int(data[2])
    
    # MySQL
    cursor.execute("INSERT INTO SENSORS_DATA(temperature, humidity, gas) VALUES (%s,%s,%s)", 
                   (temp, hum, gas))
    db.commit()
    
    # ThingSpeak
    url = f"https://api.thingspeak.com/update?api_key={API_KEY}&field1={temp}&field2={hum}&field3={gas}"
    response = requests.get(url)
    print(f" ThingSpeak: {response.text} (Status: {response.status_code})")
    
    print(f" SAVED: {temp}°C {hum}% {gas}ppm\n")

client = mqtt.Client()
client.on_message = on_message
client.connect("broker.emqx.io", 1883, 60)
client.subscribe("env/data")
print(" Waiting for ESP32...")
client.loop_forever()