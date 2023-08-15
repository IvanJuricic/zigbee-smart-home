import socket
import sys
import signal

# Server settings
host = "0.0.0.0"  # Listen on all available interfaces
port = 12345  # Choose a suitable port

log_file = open('./log_file.log', 'a')
original_stdout = sys.stdout  # Save a reference to the original standard output

# Redirect standard output to the log file
sys.stdout = log_file

# Define a function to handle the SIGINT signal (Ctrl+C)
def handle_sigint(signum, frame):
    # Restore the original standard output
    sys.stdout = original_stdout
    log_file.close()  # Close the log fil
    print("\nShutting down...")
    sys.exit(0)

signal.signal(signal.SIGINT, handle_sigint)

try:
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
            print(f"Received data => {data}")
            # Process the request and send a response
            response = "Hello from the server!"
            client_socket.sendall(response.encode())
        finally:
            # Clean up the connection
            client_socket.close()
        
        # Flush the log output to the file after each connection
        sys.stdout.flush()

finally:
    # Restore the original standard output
    sys.stdout = original_stdout
    log_file.close()  # Close the log file
