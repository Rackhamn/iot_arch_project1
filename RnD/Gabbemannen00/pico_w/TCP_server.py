import socket #import socket module in order to use TCP/IP communication
import time
# Serverconfiguration
SERVER_IP = "192.168.1.128" # Pico W:s IP-adress (change if needed)
SERVER_PORT = 5000 # Portnumber of Server
BUFFER_SIZE = 1024 # Max amount of data to recieve in bytes in each receivement

# Create a TCP-socket
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Bind the socket to the given IP-adress and portnumber
server_socket.bind((SERVER_IP, SERVER_PORT))

# Start to listen for incomming connections(1 means we accept a connection)
server_socket.listen(1)

print("Looking for a connection...")

# Accept an incomming connection from client
client_socket, client_adress = server_socket.accept()
print(f"Connection from client adress: {client_adress}")

try:
    # continue to receive messages until the client shuts the connection
    while True:
        # receive data from the client
        data = client_socket.recv(BUFFER_SIZE)
        
        # check if no data was received after the connection was shut down
        if not data:
            print("")
            print("No more data was received from client.")
            break # exit the loop when client is disconnected
        
        # Decode the received bytes to a string
        received_message = data.decode()
        
        # print the received message in the terminal
        print(f"Received data: {received_message}")
        
        # Send the same message back to the client (echo)
        client_socket.send(received_message.encode())
        print(f"Sent back: {received_message}")
     
except Exception as e:
    # handle unexpected failures during communication
    print(f"Error: {e}")

finally:
    # shut the connection to the client when everything is done
    client_socket.close()
    print("Connection closed.")
