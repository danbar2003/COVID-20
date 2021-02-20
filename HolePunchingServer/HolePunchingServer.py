import socket
import threading
import struct
import schedule

PORT = 3301
SERIALIZATION_FORMAT = '=iBLLBBH' + 2 * 'LH'

clients = []

"""
enum PACK_TYPE type;
	/*-------commnad-------*/
	BYTE act;
	ULONG32 gateway_ip;
	ULONG32 victim_ip;
	BYTE gateway_mac[6];
	BYTE victim_mac[6];
	/*---------------------*/

	/*--p2p-communication--*/
	union
	{
		uint16_t id; // commands 
		uint16_t num_of_hosts; // p2p establishing
	} numerics ;

	adr dst_peer, private_peer;
	/*---------------------*/
"""


def main():
    udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp_sock.bind(('', PORT))

    while len(clients) < 2:
        data, adr = udp_sock.recvfrom(1024)
        print(struct.unpack(SERIALIZATION_FORMAT, data))
        clients.append(adr)


if __name__ == '__main__':
    main()
