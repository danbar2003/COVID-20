def safe_cast(val, to_type, default=None):
    try:
        return to_type(val)
    except (ValueError, TypeError):
        return default


def show_options():
    print("\n\n\n[+] 1 - scan the network\n"
          "[+] 2 - show zombies\n"
          "[+] 3 - get zombie tree\n"
          "[+] 4 - command\n")
