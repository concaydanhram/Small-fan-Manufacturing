
import json
import os

import paho.mqtt.client as mqtt
from dotenv import load_dotenv
from influxdb_client_3 import InfluxDBClient3, Point

load_dotenv()

MQTT_BROKER = os.getenv("MQTT_BROKER", "127.0.0.1")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))
MQTT_TOPIC = os.getenv("MQTT_TOPIC", "fan_data")

INFLUXDB_HOST = os.getenv(
    "INFLUXDB_HOST", "http://127.0.0.1:8181"
)
INFLUXDB_DATABASE = os.getenv(
    "INFLUXDB_DATABASE", "fan_monitor"
)
INFLUXDB_TOKEN = os.getenv("INFLUXDB_TOKEN")

if not INFLUXDB_TOKEN:
    raise RuntimeError(
        "Missing INFLUXDB_TOKEN. Check your .env file."
    )

# Connect to InfluxDB 3 Core
db = InfluxDBClient3(
    host=INFLUXDB_HOST,
    database=INFLUXDB_DATABASE,
    token=INFLUXDB_TOKEN,
)


def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code.is_failure:
        print(f"[MQTT] Connection failed: {reason_code}")
        return

    print(f"[MQTT] Connected to {MQTT_BROKER}:{MQTT_PORT}")
    client.subscribe(MQTT_TOPIC, qos=0)
    print(f"[MQTT] Subscribed to topic: {MQTT_TOPIC}")


def on_message(client, userdata, msg):
    try:
        # Decode the JSON message published by ESP32
        payload = json.loads(msg.payload.decode("utf-8"))

        device = str(payload["dev"])
        freq = float(payload["freq"])
        amp = float(payload["amp"])
        current = float(payload["current"])

        # Create one time-series point
        point = (
            Point("fan_telemetry")
            .tag("dev", device)
            .field("freq", freq)
            .field("amp", amp)
            .field("current", current)
        )

        # Write the point to InfluxDB
        db.write(point)

        print(
            f"[DB] Saved | dev={device}, "
            f"freq={freq}, amp={amp}, current={current}"
        )

    except (UnicodeDecodeError, json.JSONDecodeError, KeyError, ValueError) as exc:
        print(f"[DATA ERROR] Invalid message: {exc}")
        print(f"Payload: {msg.payload!r}")

    except Exception as exc:
        print(f"[DB ERROR] Failed to save message: {exc}")


client = mqtt.Client(
    mqtt.CallbackAPIVersion.VERSION2,
    client_id="prj3-python-server",
)

client.on_connect = on_connect
client.on_message = on_message

try:
    print(f"Connecting to MQTT broker {MQTT_BROKER}:{MQTT_PORT}...")
    client.connect(MQTT_BROKER, MQTT_PORT, keepalive=60)
    client.loop_forever()

except KeyboardInterrupt:
    print("\nStopping server...")

except Exception as exc:
    print(f"[SERVER ERROR] {exc}")

finally:
    client.disconnect()
    db.close()
    print("Server stopped.")
