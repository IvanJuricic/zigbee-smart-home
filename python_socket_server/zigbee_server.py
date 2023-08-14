import socket

# Server settings
host = "0.0.0.0"  # Listen on all available interfaces
port = 12345  # Choose a suitable port

# Create a socket
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Bind the socket to the server address
server_socket.bind((host, port))

# Listen for incoming connections
server_socket.listen(1)
print("Waiting for a connection...")

while True:
    # Wait for a connection
    client_socket, client_address = server_socket.accept()
    print(f"Connected by: {client_address}")

    try:
        # Receive data from the client
        data = client_socket.recv(1024).decode()

        # Process the request and send a response
        response = "Hello from the server!"
        client_socket.sendall(response.encode())
    finally:
        # Clean up the connection
        client_socket.close()
