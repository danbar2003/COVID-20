def safe_cast(val, to_type, default=None):
    try:
        return to_type(val)
    except (ValueError, TypeError):
        return default


def show_options():
    print("\n\n\n"
          "[+] 1 - scan the network\n"
          "[+] 2 - show zombies\n"
          "[+] 3 - get zombie's tree\n"
          "[+] 4 - show entire network\n"
          "[+] 5 - command\n"
          "[+] 6 - show ip_macs of host\n")


def find_node(zombie, zombie_lst, flag_lst=None):
    """
    :returns the zombie in the zombie botnet tree
    """

    if not flag_lst:
        flag_lst = []

    # iterate over branch zombies
    for z in zombie_lst:

        # prevent infinite loops
        if z in flag_lst:
            return

        # check if the node
        if z.equals(zombie):
            return z

        # add to flag list
        flag_lst.append(z)

        # recursive check
        node = find_node(zombie, z.branches, flag_lst)

        # if found node simply return
        if node:
            return node


def find_path(dst_zombie, zombie_lst, flag_lst=None):
    """
    finds the next node that should forward the command
    """

    min_height = -1
    temp_node = None

    if not flag_lst:
        flag_lst = []

    for z in zombie_lst:

        if z in flag_lst:
            return

        if z.equals(dst_zombie):
            return z, 0

        flag_lst.append(z)

        node, height = find_path(dst_zombie, z.branches, flag_lst)

        # if found node
        if node:

            # check if path was found before
            if min_height == -1:
                min_height = height  # if there is no path choose the last option
                temp_node = z
            else:
                if height < min_height:
                    temp_node = z
    return temp_node, min_height + 1
