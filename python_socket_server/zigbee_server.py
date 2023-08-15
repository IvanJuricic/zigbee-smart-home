import socket
import sys
import signal
import datetime
import threading

# Server settings
host = "0.0.0.0"  # Listen on all available interfaces
port = 12345  # Choose a suitable port

log_file = open('./log_file.log', 'a')
original_stdout = sys.stdout  # Save a reference to the original standard output

# Redirect standard output to the log file
sys.stdout = log_file

# List to hold active client sockets
active_clients = []

# Define a function to handle the SIGINT signal (Ctrl+C)
def handle_sigint(signum, frame):
    # Restore the original standard output
    sys.stdout = original_stdout
    log_file.close()  # Close the log fil
    client_socket.close()
    print(f"\n[{datetime.datetime.now()}] Shutting down...")
    sys.exit(0)

signal.signal(signal.SIGINT, handle_sigint)

def handle_client(client_socket, client_address):
    try:
        print(f"[{datetime.datetime.now()}] Connected by: {client_address}")
        active_clients.append(client_socket)
        while True:
            data = client_socket.recv(1024).decode()
            if not data:
                print(f"[{datetime.datetime.now()}] Client {client_address} closed the connection")
                active_clients.remove(client_socket)
                break
            print(f"[{datetime.datetime.now()}] Received data from {client_address}: {data}")
            # Process the request and send a response
            response = "Hello from the server!"
            client_socket.sendall(response.encode())
            
            # Flush the log output to the file after each communication
            sys.stdout.flush()
    except Exception as e:
        print(f"[{datetime.datetime.now()}] Error: {e}")
        sys.stdout.flush()  # Flush the log in case of an error
    finally:
        client_socket.close()

try:
    # Create a socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    # Bind the socket to the server address
    server_socket.bind((host, port))

    # Listen for incoming connections
    server_socket.listen(5)
    print(f"[{datetime.datetime.now()}] Waiting for connections...")
    

    while True:
        client_socket, client_address = server_socket.accept()

        # Create a new thread to handle the client connection
        client_thread = threading.Thread(target=handle_client, args=(client_socket, client_address))
        client_thread.start()

finally:
    # Close active client sockets
    for client_socket in active_clients:
        client_socket.close()

    # Restore the original standard output
    sys.stdout = original_stdout
    log_file.close()  # Close the log file
