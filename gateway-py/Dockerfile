# Python gateway — Slim
FROM python:3.14-slim

WORKDIR /app

# Install dependencies
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy application code
COPY app.py .

ENV MQTT_HOST=test.mosquitto.org \
    MQTT_PORT=8883 \
    MQTT_TOPIC_CMD=from_cloud/command \
    MQTT_TOPIC_EVENT=from_device/events \
    SOCKET_PATH=/run/qr/qr_socket.sock

CMD ["python", "app.py"]