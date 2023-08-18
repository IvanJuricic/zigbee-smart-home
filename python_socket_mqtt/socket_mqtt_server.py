import socket
import threading
import time
import paho.mqtt.client as mqtt
import queue

# Socket server settings
HOST = '0.0.0.0'
PORT = 12345

# MQTT broker settings
MQTT_BROKER_IP = '192.168.1.10'
MQTT_BROKER_PORT = 1883

# Create a single queue for inter-thread communication
socket_to_mqtt_queue = queue.Queue(maxsize=1)
mqtt_to_socket_queue = queue.Queue(maxsize=1)

# Create a socket server thread
def socket_server_thread():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind((HOST, PORT))
    server_socket.listen(5)
    print("Socket server listening on {}:{}".format(HOST, PORT))
    
    while True:
        client_socket, client_address = server_socket.accept()
        print("Connected by:", client_address)
        client_thread = threading.Thread(target=handle_client, args=(client_socket,))
        client_thread.start()

def handle_client(client_socket):
    while True:
        data = client_socket.recv(1024).decode()
        if not data:
            break
        print("Received from client:", data)
        # Process the data and send response if needed
        #response = "pozdrav brate"
        if "Toggle!" in data:
            socket_to_mqtt_queue.put(data)
        else:
            continue
            
        response = mqtt_to_socket_queue.get()
        client_socket.sendall(response.encode())

# Create an MQTT client thread
def mqtt_client_thread():
    def on_connect(client, userdata, flags, rc):
        print("Connected to MQTT broker with code:", rc)
    
    def on_message(client, userdata, msg):
        print("Received MQTT message on topic:", msg.topic, "with payload:", msg.payload.decode())
        # Process the MQTT message and send response if needed
        mqtt_to_socket_queue.put(msg.payload.decode())
        

    mqtt_client = mqtt.Client()
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message
    mqtt_client.connect(MQTT_BROKER_IP, MQTT_BROKER_PORT, keepalive=60)
    mqtt_client.loop_start()

    # Subscribe to the topic(s) you're interested in
    mqtt_client.subscribe("test")
    
    while True:
        print("Waiting for client messages")
        message = socket_to_mqtt_queue.get()
        # Process the message and generate a response
        print(f"Received message from client: {message}")
        # Put the response back into the message queue
        #message_queue.put(response)
        mqtt_client.publish("test/topic", message)

if __name__ == '__main__':
    socket_thread = threading.Thread(target=socket_server_thread)
    mqtt_thread = threading.Thread(target=mqtt_client_thread)
    
    socket_thread.start()
    mqtt_thread.start()
    
    socket_thread.join()
    mqtt_thread.join()
