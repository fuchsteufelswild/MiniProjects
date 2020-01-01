import socket
import threading
import sys
from utils import *


class Intermediate:
    def __init__(self, _ip, _port):
        src_receive_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # Receive messages from the source 
        src_receive_socket.bind((_ip, _port)) # Bind to its port, ip

        self.ack_receive_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # Receive ack messages from dest

        listen_src_thread = threading.Thread(target=self.listen_for_messages, args=(src_receive_socket, )) # Listen on source thread
        listen_dst_thread = threading.Thread(target=self.listen_for_ack, args=(self.ack_receive_socket, )) # Listen on destination thread

        self.is_packet_ready = threading.Semaphore(0)

        self.sender_addr = None

        listen_src_thread.start()
        listen_dst_thread.start()

        listen_src_thread.join()
        listen_dst_thread.join()

    def listen_for_messages(self, int_socket):
        first_time = True
        while True:
            recv_size = 0
            total_data_recv = bytearray()
            while(recv_size < payload_packet_size):
                data, addr = int_socket.recvfrom(payload_packet_size)
                if(first_time):
                    self.sender_addr = addr
                    first_time = False
                recv_size += len(data)
                total_data_recv.extend(data)

            if(not is_packet_corrupted(total_data_recv)):
                
                path_id = string_to_int(total_data_recv[seq_number_size + 1:seq_number_size + 2]) - 48
                dest_addr_ip = " "
                dest_addr_port = 0

                if(path_id == 0):
                    dest_addr_ip = path0[2][0]
                    dest_addr_port = path0[2][1]
                if(path_id == 1):
                    dest_addr_ip = path1[2][0]
                    dest_addr_port = path1[2][1]
                if(path_id == 2):
                    dest_addr_ip = path2[2][0]
                    dest_addr_port = path2[2][1]


                self.ack_receive_socket.sendto(total_data_recv, (dest_addr_ip, dest_addr_port))
                if(string_to_int(total_data_recv[seq_number_size:seq_number_size + 1]) == 49):
                    for i in range(0, 10):
                        self.ack_receive_socket.sendto(total_data_recv, (dest_addr_ip, dest_addr_port))
                    print("Tranmission finished")
                    break

    def listen_for_ack(self, int_socket):
        while True:
            recv_size = 0
            total_data_recv = bytearray()
            while(recv_size < ack_packet_size):
                data, addr = int_socket.recvfrom(ack_packet_size)
                recv_size += len(data)
                total_data_recv.extend(data)
            # print(total_data_recv)
            if(not is_ack_packet_corrupted(total_data_recv)):
                int_socket.sendto(total_data_recv, (self.sender_addr[0], self.sender_addr[1]))
                print(string_to_int(total_data_recv[seq_number_size:seq_number_size + 1]))

                if(string_to_int(total_data_recv[seq_number_size:seq_number_size + 1]) == 51):
                    for i in range(0, 10):
                        int_socket.sendto(total_data_recv, (self.sender_addr[0], self.sender_addr[1]))
                    print("Tranmission finished")
                    break


i_ip = sys.argv[1]
p = sys.argv[2]

i = Intermediate(i_ip, string_to_int(p))
