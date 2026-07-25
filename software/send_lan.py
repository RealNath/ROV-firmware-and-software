import socket
import time
from data_source import get_data

HOST = '192.168.1.177'
PORT = 8888

# AF_INET = IPv4, SOCK_DGRAM = UDP protocol
client = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
# client.connect((HOST, PORT))

while True:
    data = get_data()
    # Format string without '\n' because UDP sends complete packets, not streams
    message = ','.join(map(str, data)) 

    # CORRECT UDP METHOD: sendto() explicitly targets the IP and Port
    client.sendto(message.encode('utf-8'), (HOST, PORT))
    print("Sent:", data)

    time.sleep(0.05)