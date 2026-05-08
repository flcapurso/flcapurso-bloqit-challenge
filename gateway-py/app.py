"""
MQTT Gateway: Bridge between cloud (MQTT) and QR scanner container (Unix socket).
Relays commands from cloud to QR reader, publishes responses back to MQTT.
"""

import paho.mqtt.client as mqtt
import json
import time
import os
import socket

# Configuration from environment variables
SOCKET_PATH  = os.getenv('SOCKET_PATH', "/tmp/qr_socket.sock")

MQTT_HOST    = os.getenv("MQTT_HOST", "test.mosquitto.org")
MQTT_PORT    = int(os.getenv("MQTT_PORT", 1883))
MQTT_CAFILE  = os.getenv("MQTT_CERT", "")
TOPIC_SUB    = os.getenv("MQTT_TOPIC_CMD", "from_cloud/command")
TOPIC_PUB    = os.getenv("MQTT_TOPIC_EVENT", "from_device/events")

FIRST_RECONNECT_MAX_ATTEMPTS = 1

mqtt_client = None
qr_container_socket = None

# MQTT Connection callback
def on_connect(mqtt_client, userdata, flags, rc):
    """Handle successful connection to broker."""
    if rc == 0:
        print(f"[INFO] Connected to {MQTT_HOST}:{MQTT_PORT}")
        mqtt_client.subscribe(TOPIC_SUB)
        print(f"[INFO] Subscribed to {TOPIC_SUB}")
    else:
        print(f"[ERROR] Connection failed with code {rc}")

# MQTT Subscribe callback
def on_subscribe(mqtt_client, userdata, mid, reason_code_list, properties):
    """Handle subscription confirmation from broker."""
    if reason_code_list[0].is_failure:
        print(f"[ERROR] Broker rejected you subscription: {reason_code_list[0]}")
    else:
        print(f"[INFO] Broker granted the following QoS: {reason_code_list[0].value}")

# MQTT Disconnect callback
def on_disconnect(mqtt_client, userdata, rc):
    """Handle unexpected disconnection."""
    print(f"[WARN] Disconnected (rc={rc}), will attempt reconnect...")

# MQTT Message receive callback
def on_message(mqtt_client, userdata, msg):
    """Parse incoming command and relay to QR scanner container."""
    payload = msg.payload.decode()
    print(f"[RECEIVED] topic={msg.topic} payload={payload}")

    try:
        data = json.loads(payload)

        if "command" in data:
            command = data["command"]
            print(f"[COMMAND] {command}")

            # Append timeout parameter if present (e.g., "START 5000")
            if command.lower() == "start" and "timeout" in data:
                command += " " + str(data["timeout"])

            try:
                global qr_container_socket
                qr_container_socket.sendall(command.encode())
            except Exception as e:
                print("Send error:", e)

    except json.JSONDecodeError:
        print("[ERROR] Payload is not valid JSON")

# MQTT Publish callback
def on_publish(mqtt_client, userdata, mid):
    """Confirm message published to broker."""
    print(f"[INFO] Message published (mid={mid})")


def main():
    """Initialize MQTT client, connect to broker, and relay messages to/from QR container."""
    mqtt_client = mqtt.Client(client_id="gateway-py-client")

    # Register MQTT callback handlers
    mqtt_client.on_connect    = on_connect
    mqtt_client.on_disconnect = on_disconnect
    mqtt_client.on_message    = on_message
    mqtt_client.on_publish    = on_publish

    # Configure TLS if certificate provided
    if MQTT_CAFILE:
        mqtt_client.tls_set(ca_certs=MQTT_CAFILE)

    # Set reconnection backoff strategy
    mqtt_client.reconnect_delay_set(min_delay=1, max_delay=30)

    # Connect to MQTT broker with retry
    print(f"[INFO] Connecting to {MQTT_HOST}:{MQTT_PORT}...")
    for attempt in range(FIRST_RECONNECT_MAX_ATTEMPTS):
        try:
            error = mqtt_client.connect(MQTT_HOST, MQTT_PORT, keepalive=60)
        
        except Exception as e:
            print(f"[INFO] ATTEMPT #{attempt} - Error connecting to MQTT broker: ", e)
            error = -1
            time.sleep(2)

    if error:
        print(mqtt_client.is_connected())
        print(f"[ERROR] Connection to {MQTT_HOST}:{MQTT_PORT} failed. Exiting program")
        exit(-1)

    # Start non-blocking event loop for MQTT
    mqtt_client.loop_start()

    # Connect to QR scanner container and relay messages
    global qr_container_socket
    qr_container_socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)

    while True: 
        # Attempt Unix socket connection to QR container
        try:
            qr_container_socket.connect(SOCKET_PATH)
            print(f"[INFO] Connected to {SOCKET_PATH}")
        except socket.error as e:
            print(f"[ERROR] Connection to {SOCKET_PATH} failed: {e}")
            time.sleep(2)
            continue

        # Forward responses from QR container to MQTT broker
        while True:
            try:
                response = qr_container_socket.recv(1024)
                if not response:
                    print("[ERROR] QR-container socket disconnected.")
                    break

                # Publish response to MQTT topic
                print(f'[PUBLISH] "{TOPIC_PUB}" -> {response.decode()}')
                mqtt_client.publish(TOPIC_PUB, response)

            except Exception as e:
                print("Receive error:", e)
                break
    
    print(f"[ERROR] Connection to Container A failed. Exiting program")
    exit(-1)

if __name__ == "__main__":
    main()