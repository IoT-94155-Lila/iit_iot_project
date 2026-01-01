import paho.mqtt.client as mqtt
import mysql.connector
import requests
import math   

# ---------------- ThingSpeak ----------------
API_KEY = "KQYWBZQS2WERU396"
print(f"API Key OK: {API_KEY}")

# ---------------- MySQL Connection ----------------
db = mysql.connector.connect(
    host="localhost",
    user="root",
    password="root",
    database="env_db"
)
cursor = db.cursor()

print("Waiting for ESP32...")

# ---------------- MQTT Callback ----------------
def on_message(client, userdata, msg):
    payload = msg.payload.decode()
    print(f"ESP32 SENT: {payload}")

    # ---------- Step 1: Parse data safely ----------
    try:
        data = payload.split(',')
        temp = float(data[0])
        hum  = float(data[1])
        gas  = int(data[2])
    except Exception as e:
        print("Parsing error, skipping message\n")
        return

    # ---------- Step 2: Block NaN values ----------
    if math.isnan(temp) or math.isnan(hum):
        print("\n")
        return

    # ---------- Step 3: Insert into MySQL ----------
    try:
        cursor.execute(
            "INSERT INTO sensor_data (temperature, humidity, gas) VALUES (%s, %s, %s)",
            (temp, hum, gas)
        )
        db.commit()
    except Exception as e:
        print("MySQL error:", e, "\n")
        return

    # ---------- Step 4: Upload to ThingSpeak ----------
    try:
        url = (
            f"https://api.thingspeak.com/update?"
            f"api_key={API_KEY}&field1={temp}&field2={hum}&field3={gas}"
        )
        response = requests.get(url)
        print(f"ThingSpeak: {response.text} (Status: {response.status_code})")
    except Exception as e:
        print("ThingSpeak error:", e)

    print(f"SAVED: {temp}°C {hum}% {gas}ppm\n")

# ---------------- MQTT Setup ----------------
client = mqtt.Client()  # Deprecation warning is safe to ignore
client.on_message = on_message

client.connect("broker.emqx.io", 1883, 60)
client.subscribe("env/data")

client.loop_forever()
