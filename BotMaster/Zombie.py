class Zombie:

    def __init__(self, adr):
        self.adr = adr  # (str_ip, n_port)
        self.branches = []  # other zombies
        self.hosts = {}  # ip : mac

    def equals(self, zombie):
        if self.adr == zombie.adr:
            return True
        return False

    def add_zombie(self, zombie):
        for z in self.branches:
            if zombie.equals(z):
                return
        self.branches.append(zombie)

    def has_next(self, zombie):
        for z in self.branches:
            if zombie.equals(z):
                return zombie

    def show_hosts(self):
        for key in self.hosts:
            print(key, ':', self.hosts[key])
        print('\n')
