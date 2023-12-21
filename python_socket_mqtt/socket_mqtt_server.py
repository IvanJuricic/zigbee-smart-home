import paho.mqtt.client as mqtt
import asyncio
import websockets

# MQTT broker settings
broker_address = "ivanproxy.ddns.net"
broker_port = 1883

# Dictionary to store topic handlers
topic_handlers = {
    "topic1": None,
    "topic2": None,
    # Add more topics and corresponding handlers as needed
}

# WebSocket server settings
websocket_server_address = "localhost"
websocket_server_port = 8765

# Callback function to handle when a message is received for a specific topic
async def on_message(client, userdata, message):
    topic = message.topic
    payload = message.payload.decode("utf-8")
    print(f"Received message on topic '{topic}': {payload}")

    # Forward the message to WebSocket server
    async with websockets.connect(f"ws://{websocket_server_address}:{websocket_server_port}") as websocket:
        await websocket.send(f"Received message on topic '{topic}': {payload}")
        print("Message forwarded to WebSocket server")

# Create an MQTT client
client = mqtt.Client()

# Set the function to be called when a message is received
client.on_message = on_message

# Connect to the MQTT broker
client.connect(broker_address, broker_port)

# Subscribe to the desired topics and assign handlers
for topic, handler in topic_handlers.items():
    client.subscribe(topic)
    print(f"Subscribed to topic: {topic}")

# Start the MQTT loop
client.loop_start()

# Run the server indefinitely
try:
    while True:
        pass
except KeyboardInterrupt:
    # Disconnect on Ctrl+C
    client.loop_stop()
    client.disconnect()
