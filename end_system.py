import socket
import os
from message import *
import pickle


def initialize():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        message = Message("Hello World", end_system_ip, "255.255.255.255", 0)
        s.connect((router_ip, 3000))
        s.sendall(pickle.dumps(message))


def send():
    while(1):
        dest_ip = input("Send to: ")
        message = input("Message: ")
        ttl = int(input("TTL: "))
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            message = Message(message, end_system_ip, dest_ip, ttl)
            s.connect((router_ip, 3000))
            s.sendall(pickle.dumps(message))


def receive():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s2:
        s2.bind((end_system_ip, 3000))
        s2.listen()
        while(1):
            conn, addr = s2.accept()
            with conn:
                print(f"Connected by {addr}")
                while True:
                    data = conn.recv(1024)
                    if not data:
                        break
                    message = pickle.loads(data)
                    print(f"Received {message.m}")


if __name__ == '__main__':
    global end_system_ip
    global router_ip

    # TEST test1.py USE THIS
    # case = input("case")
    # if case == "1":
    #     end_system_ip = "10.0.0.251"
    #     router_ip = "10.0.0.1"
    # elif case == "2":
    #     end_system_ip = "10.1.0.252"
    #     router_ip = "10.1.0.1"

    # TEST case3

    # end_system_ip = input("End System's IP Address: ")
    # router_ip = input("Next Hop Router's IP Address: ")

    initialize()
    pid = os.fork()
    if pid == 0:
        receive()
    else:
        send()
