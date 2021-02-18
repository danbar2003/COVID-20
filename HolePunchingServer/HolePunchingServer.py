import socket
import threading
import struct
import schedule

PORT = 33014
SERIALIZATION_FORMAT = 'iBLLBBHLH'

clients = []

def main():
    udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp_sock.bind(('', PORT))

    while True:
        while len(clients) < 2:
            clients.append(udp_sock.recvfrom()[1])
    
        udp_sock.sendto(str(clients[1]), client[0])
        udp_sock.sendto(str(clients[0]), client[1])

    

if __name__ == '__main__':
    main()



