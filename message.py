class Message:
    def __init__(self, message, source_ip, dest_ip, ttl):
        self.m = message
        self.source_ip = source_ip
        self.dest_ip = dest_ip
        self.path = []
        self.ttl = ttl
        self.hops = 0
        self.forward = True
        self.discover = False
        self.internal = True