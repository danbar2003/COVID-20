class Zombie:

    def __init__(self, adr):
        self.adr = adr
        self.branches = []
        self.g_ip = 0
        self.v_ip = 0
        self.g_mac = 0
        self.v_mac = 0

    def equals(self, zombie):
        if self.adr == zombie.adr:
            return True
        return False

    def add_zombie(self, zombie):
        self.branches.append(zombie)
