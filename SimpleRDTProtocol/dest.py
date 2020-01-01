import socket
import threading
import hashlib
import sys
import time
from utils import  *

class Receiver:
    def __init__(self, port0, port1, port2, host0, host1, host2):
        self.port_n0 = port0
        self.port_n1 = port1 #port numbers
        self.port_n2 = port2

        self.host_n0 = host0
        self.host_n1 = host1
        self.host_n2 = host2

        self.expected_seq = 0
        self.expected_lock = threading.Lock() # lock to synchronize incoming packets from two different ports.
        self.receive_array = bytearray() # this is where incoming packets will be stored

        self.is_finished = False

        self.get_time_milliseconds = lambda: int(round(time.time() * 1000))
        self.current_time = 0
        self.lock_is_completed = threading.Semaphore(0)
        self.who_finished = -1
        self.finished_time = 0

    #listening for the corresponding port number wrt GBN
    def listen_for(self, number, port_number, host):
        s=socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.bind((host, port_number))
        while not self.is_finished:#keep receiving until the arrival of the last packet
            recv_size = 0
            total_data_recv = bytearray()

            send_to_target = None
            while(recv_size < payload_packet_size):
                data, addr = s.recvfrom(payload_packet_size)
                send_to_target = addr
                recv_size += len(data)
                total_data_recv.extend(data)
            if(not is_packet_corrupted(total_data_recv)): # packet is error free , accepting the packet
                seq_number = string_to_int(total_data_recv[0:seq_number_size].decode())
                rec_flag=0
                self.expected_lock.acquire()
                
                if(string_to_int(total_data_recv[seq_number_size:seq_number_size + 1]) == 49): # fin flag is set , terminating the loop
                    
                    self.who_finished = number
                    self.lock_is_completed.release()
                    ack_str =  bytearray(str(self.expected_seq).zfill(seq_number_size), default_encoding_type)

                    ack_str += bytearray('3'.zfill(1), default_encoding_type) #packet to be sent is in form of Seq+PATHID +DESTID
                    ack_str += bytearray('0'.zfill(1), default_encoding_type)
                    ack_str += generate_checksum(ack_str) # packet to be sent is in form of Seq+PATHID +DESTID +Checksum
                    for i in range(0, 10):
                        s.sendto(ack_str, (send_to_target[0], send_to_target[1]))
                    self.is_finished = True
                if(seq_number==self.expected_seq): # we got the correct seq add it to the recv array
                    
                    self.receive_array.extend(total_data_recv[header_size:header_size + payload_size])
                    self.expected_seq += 1
                    rec_flag = 1
                    if(self.expected_seq >= number_of_packets):
                        if(self.finished_time == 0):
                            self.finished_time = self.get_time_milliseconds() - self.current_time
                    if(self.expected_seq >= number_of_packets + 1):
                        if(self.finished_time == 0):
                            self.finished_time = self.get_time_milliseconds() - self.current_time
                        self.is_finished = True
                    
                self.expected_lock.release()
                if(rec_flag == 1): # the expected packet is received send ack
                    ack_str =  total_data_recv[:seq_number_size]
                    ack_str += total_data_recv[seq_number_size + 1:seq_number_size + 2] #packet to be sent is in form of Seq+PATHID +DESTID
                    ack_str += bytearray(str(0).zfill(1), default_encoding_type)
                    ack_str += generate_checksum(ack_str) # packet to be sent is in form of Seq+PATHID +DESTID +Checksum
                    s.sendto(ack_str, (send_to_target[0], send_to_target[1]))
                else:
                    self.expected_lock.acquire()
                    ack_str =  bytearray(str(self.expected_seq - 1).zfill(seq_number_size), default_encoding_type)
                    self.expected_lock.release()

                    ack_str += total_data_recv[seq_number_size + 1:seq_number_size + 2] #packet to be sent is in form of Seq+PATHID +DESTID
                    ack_str += bytearray(str(0).zfill(1), default_encoding_type)
                    ack_str += generate_checksum(ack_str) # packet to be sent is in form of Seq+PATHID +DESTID +Checksum
                    s.sendto(ack_str, (send_to_target[0], send_to_target[1]))
        print("Transmission finished")



    #this is the main function which will start listening for the other two ports
    def start_receiving(self, exp_type):

        if(exp_type == '1'):
            left_connection=threading.Thread(target=self.listen_for, args=(0, self.port_n1, self.host_n1, ))
            right_connection=threading.Thread(target=self.listen_for, args=(1, self.port_n2, self.host_n2, ))

            self.current_time = self.get_time_milliseconds()

            left_connection.start()
            right_connection.start()

            self.lock_is_completed.acquire()

            if(self.who_finished == 0):
                left_connection.join()
                right_connection.join(5)
            else:
                right_connection.join()
                left_connection.join(5)
        else:
            connection = threading.Thread(target=self.listen_for, args=(0, self.port_n0, self.host_n0))
            self.current_time = self.get_time_milliseconds()
            connection.start()
            connection.join()

exp_type = sys.argv[1]

r=Receiver(path0[2][1], path1[2][1], path2[2][1], path0[2][0], path1[2][0], path2[2][0])
r.start_receiving(exp_type)


print("Transmission took " + str(r.finished_time) + " milliseconds")
sys.exit()
