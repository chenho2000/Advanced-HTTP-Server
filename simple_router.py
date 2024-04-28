import socket
import os
from message import *
import pickle
import time


def send(message, ip):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((ip, 3000))
        s.sendall(pickle.dumps(message))


""" broadcast info to all ip addresses in {ips} but {curr}
"""
def boardcast_info(curr, ips, info):        
    for i in ips:
        if i == curr:
            continue
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((i, 3000))
            s.sendall(pickle.dumps(info))
            # print("send out !!\n")
            # print(i, info)


def print_forwarding_table(curr_routers, forwarding_table):
    print("curr router:")
    print(curr_routers)
    print("forwarding table: ")
    print(forwarding_table)


"""broadcast to current router's all adjacent routers and end systems
"""
def set(curr, table, curr_routers, external):
    boardcast_info(curr, curr_routers.keys(), table)
    boardcast_info(curr, external.keys(), table)

"""new_table is going to be broadcasted by other routers
old_table is this router's forwarding table
"""
def set_forwarding_table(new_table, old_table):
    for i in new_table.keys():
        if i == "forwarding_table":
            continue
        if i not in old_table.keys() or old_table[i][2] > new_table[i][2]:         # if encounter new router or routers with smaller hop number, update
            old_table[i] = new_table[i]


def update(curr, neighbour, neighbour_routers, neighbour_external):
    for i in neighbour.keys():
        if i == "forwarding_table":
            continue
        if i not in curr.keys() or curr[i][2] > neighbour[i][2] + 1:
            curr[i] = [neighbour_routers, neighbour_external, neighbour[i][2] + 1]


"""curr is one of the router's interfaces
case 1: source and destination are under same switch, doesn't go through router
case 2: same router, different interfaces
case 3: receive from an end system, deliver to an external router
case 4: receive from an external router, deliver to an end system
case 5: receive from an external router, deliver to an external router
"""
def receive(curr, curr_routers, external, forwarding_table):
    # receive from an internal interface
    if curr not in external:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s2:
            s2.bind((curr, 3000))
            s2.listen()
            while(1):
                conn, addr = s2.accept()
                with conn:
                    print(f"{curr} Connected by {addr}")
                    # print_forwarding_table(
                    #             curr_routers, forwarding_table)
                    while True:
                        data = conn.recv(1024)
                        if not data:
                            break
                        message = pickle.loads(data)
                        if type(message) == dict:          # if message is of dictionary type, then it is either a forwarding table or an internal interface
                            if "forwarding_table" in message.keys():
                                set_forwarding_table(message, forwarding_table)
                            else:
                                curr_routers = message
                            break
                        print(f"Received {message.m}")

                        # new end system

                        if message.dest_ip == "255.255.255.255":
                            curr_routers[curr].append(addr[0])
                            forwarding_table[addr[0]] = [
                                curr_routers, external, 0]          #?????
                            set(curr, curr_routers, curr_routers, external)         # broadcast internal interface and forwarding table
                            set(curr, forwarding_table, curr_routers, external)
                            # print("updated here: \n")
                            # print_forwarding_table(
                            #     curr_routers, forwarding_table)
                            break

                        brek = False
                        # it's a message
                        # if host it's in current router
                        for k in curr_routers.keys():
                            if message.dest_ip in curr_routers[k]:                      # case 2
                                # if not current interface
                                if k != curr:
                                    # print("send(message, curr_routers[k])", curr_routers[k])
                                    # print("\n")
                                    send(message, k)
                                # if it's in current interface
                                else:
                                    message.ttl -= 1
                                    # if ttl < 0
                                    if message.ttl < -1:
                                        brek = True
                                        break
                                    # print("send(message, message.dest_ip)", message.dest_ip)
                                    # print("\n")
                                    send(message, message.dest_ip)
                                    brek = True

                        if brek:
                            break
                        # in forwarding table
                        if message.dest_ip in forwarding_table.keys():
                            # find which interface connect to that router
                            for h in external.keys():                                   # case 3
                                for p in forwarding_table[message.dest_ip][1].keys():
                                    if external[h] == p:
                                        # print(curr, h)
                                        # print("\n")
                                        send(message, h)
                            break
                        # not in forwarding table, need to check for path
                        # print("let the discover begin\n\n")
                        message.discover = True
                        message.path.append([curr_routers, external])
                        for k in external.keys():       # send to every external interface to explore the destination
                            # print(curr, k)
                            # print("\n")
                            send(message, k)
                        break

    # receive from an external interface
    else:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s3:
            s3.bind((curr, 3000))
            s3.listen()
            pid = os.fork()
            if pid == 0:
                curr_time = time.time()
                while(1):
                    if time.time() - curr_time  > 30:
                        curr_time = time.time()
                        send("advertise", curr)
            while(1):
                conn, addr = s3.accept()
                with conn:
                    print(f"{curr} Connected by {addr}")
                    # print_forwarding_table(
                    #             curr_routers, forwarding_table)
                    while True:
                        data = conn.recv(1024)
                        if not data:
                            break
                        message = pickle.loads(data)
                        if message == "advertise":                  # since we r using RIP, advertise the forwarding table and other info every 30 secs
                            send([forwarding_table, curr_routers,
                                 external], external[curr])
                            break
                        if type(message) == dict:
                            # print("dict is \n\n")
                            # print(message)
                            if "forwarding_table" in message.keys():
                                set_forwarding_table(message, forwarding_table)
                            else:
                                curr_routers = message
                            # print_forwarding_table(
                            #     curr_routers, forwarding_table)
                            break
                        elif type(message) == list:                  # upon receiving other router's forwarding table
                            update(forwarding_table,
                                   message[0], message[1], message[2])
                            set(curr, forwarding_table, curr_routers, external)     # flooding to all after updating
                            # print("\n\n")
                            # print(forwarding_table)
                            # print("\n\n")
                            break

                        # if by forwarding table, we know where to go
                        if message.discover == False:
                            # in the interface directly link to current interface
                            if external[curr] in forwarding_table[message.dest_ip][1].keys():
                                # print("send(message, external[curr])", external[curr])
                                # print("\n")
                                send(message, external[curr])
                            else:
                                in_router = False

                                # if dest is in this router
                                for i in curr_routers.keys():                       # case 4
                                    if message.dest_ip in curr_routers[i]:
                                        # print("send(message, i)", i)
                                        # print("\n")
                                        send(message, i)
                                        in_router = True

                                if in_router:
                                    break

                                # send to other interface
                                for i in external.keys():                           # case 5
                                    if external[i] in forwarding_table[message.dest_ip][1].keys():
                                        message.ttl -= 1
                                        if message.ttl == -1:
                                            break
                                        # print("send(message, i)", i)
                                        # print("\n")
                                        send(message, i)

                        else:                               # if we do not know where to go
                            if message.internal:            # True only if message orginate from this router
                                message.internal = False
                                # print(curr, external[curr])
                                send(message, external[curr])
                                break

                            # print("show path")
                            # print(message.path)
                            # print("\n")
                            # if it's a discover message from src to dst
                            if message.discover and message.forward:
                                message.hops += 1
                                if message.hops > 15:
                                    break
                                if message.source_ip not in forwarding_table.keys():
                                    forwarding_table[message.source_ip] = message.path[-1].copy()
                                    forwarding_table[message.source_ip].append(
                                        message.hops)
                                elif forwarding_table[message.source_ip][2] > message.hops:
                                    forwarding_table[message.source_ip] = message.path[-1].copy()
                                    forwarding_table[message.source_ip].append(
                                        message.hops)

                            # if it's a discover message from dst back to src
                            elif message.discover and not message.forward:
                                message.hops += 1
                                if message.hops > 15:
                                    break
                                back = message.path.pop()
                                if message.dest_ip not in forwarding_table.keys():
                                    forwarding_table[message.dest_ip] = back.copy(
                                    )
                                    forwarding_table[message.dest_ip].append(
                                        message.hops)
                                elif forwarding_table[message.dest_ip][2] > message.hops:
                                    forwarding_table[message.dest_ip] = back.copy(
                                    )
                                    forwarding_table[message.dest_ip].append(
                                        message.hops)

                            # update the whole router
                            set(curr, forwarding_table, curr_routers, external)

                            # if it's in this router
                            in_curr = False
                            if message.forward:
                                message.path.append([curr_routers, external])
                                for i in curr_routers.keys():
                                    if message.dest_ip in curr_routers[i]:
                                        in_curr = True
                                        # print("send(message, i)", i)
                                        # print("\n")
                                        send(message, i)

                            # need to start backward update
                            if in_curr:
                                message.forward = False
                                message.hops = 0
                                message.ttl = 1107
                                if len(message.path) < 2:
                                    break
                                # print("send(message, external[curr])", external[curr])
                                # print("\n")
                                send(message, external[curr])

                            # if it's not in this router, pass to the next one!

                            if message.forward:
                                # print("show path")
                                # print(message.path)
                                # print("\n")
                                for j in external.keys():
                                    if curr == j:
                                        continue
                                    else:
                                        # print("send(message, external[j])", external[j])
                                        # print("\n")
                                        send(message, external[j])

                            if not message.forward:
                                # print("show path")
                                # print(message.path)
                                # print("\n")
                                if len(message.path) < 2:
                                    break
                                send_to = message.path[-2]
                                for q in send_to[1].keys():
                                    if send_to[1][q] == curr:
                                        # print("send(message, q)", q)
                                        # print("\n")
                                        send(message, q)
                        break


if __name__ == '__main__':
    """three key information for each router:
    curr_routers: <inner interface: [responsible end systems]>
    external: <outer interface: corresponding external interface>
    forwarding table: <dst ip: [,ttl]
    """


    # curr_routers = dict()
    # forwarding_table = {"forwarding_table": []}
    # external = dict()
    forwarding_table = {"forwarding_table": []}
    curr_routers = dict()      
    case = input("case")

    # TEST CASE 3 USE THIS:
    # if case == "1":
    #     curr_routers = {"10.0.1.1": [], "10.0.2.1": []}
    #     external = {}


    # TEST test1.py USE THIS
    # if case == "1":
    #     curr_routers = {"10.0.0.1": []}
    #     external = {"10.100.0.1": "10.100.0.2"}       
    # elif case == "2":
    #     curr_routers = {"10.1.0.1": []}
    #     external = {"10.100.0.2": "10.100.0.1"}
        

    for i in curr_routers.keys():
        pid = os.fork()
        if pid == 0:
            receive(i, curr_routers, external, forwarding_table)

    for i in external:
        pid = os.fork()
        if pid == 0:
            receive(i, curr_routers, external, forwarding_table)
