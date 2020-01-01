import threading
import socket
import sys
import time

from utils import *

class Source:
    def __init__(self):
        self.message_index = [] # Referred packet ids for messages
        self.message_queue = [] # Messages of count of window size
        self.next_seq_number = 0 # Next seq number 
        self.base = 0 # Base of the window
        self.lock_nsn = threading.Lock() # Lock for next_seq_number

        self.is_message_received_in_between = False # Did we receive ack while timer counts
        self.lock_is_message_received = threading.Lock() # Lock for message received variable

        self.is_finished = False # Is connection done

        self.message_lock = threading.Lock() # Lock for filling message queue
        self.gather_lock = threading.Semaphore(0) # Notify to gather messages into array

        self.new_packet_to_send = threading.Condition() # 
        self.waiting_sender = []


    def get_messages(self, file_name):
        with open(file_name, 'rb') as fn:
            send_count = 0 # Which packet we have gathered
            total_data_gathered = 0 # Gathered total data
            while(total_data_gathered < total_file_size): # While we did not gather all
                self.lock_nsn.acquire() # Acquire next 
                self.message_lock.acquire() # Message lock
                if(len(self.message_queue) == default_window_size): # If it is not the first time we came here
                    ind = -1
                    for i in range(8): # Find the index of the last received packet
                        if(self.message_index[i] == self.base):
                            ind = i
                            break
                    del self.message_queue[:ind] # Delete those messages
                    del self.message_index[:ind] # Delete those refer indices

                while len(self.message_queue) < default_window_size: # While we do not have message count of window size
                    self.message_index.append(send_count) # Append referred packet id to index array
                    data = fn.read(payload_size) # Read
                    self.message_queue.append(data) # Add data to list
                    total_data_gathered += payload_size # Add size to gathered
                    send_count += 1 # Increase the packet index

                self.message_lock.release() 
                self.lock_nsn.release() 
                self.gather_lock.acquire() # Wait for the signal
        
    
    # Wait for seconds then check if any ack received in the time being
    # If not received reset the next_seq_number 
    def check_timeout(self):
        while not self.is_finished:
            time.sleep(default_timer) # Wait
            self.lock_is_message_received.acquire() 
            if(not self.is_message_received_in_between): # If any ack received
                self.lock_nsn.acquire()

                self.next_seq_number = self.base # Reset
                while(len(self.waiting_sender) > 0):
                    self.waiting_sender.pop()
                    self.new_packet_to_send.acquire()
                    self.new_packet_to_send.notify() # Inform senders that next seq num reset
                    self.new_packet_to_send.release()
                
                self.lock_nsn.release()
            
            self.is_message_received_in_between = False # Reset
            self.lock_is_message_received.release()

    # Receiver for paths
    def receiver(self, sender_socket):
        while True:

            recv_size = 0
            total_data_recv = bytearray()

            while(recv_size < ack_packet_size):
                ack_data, _ = sender_socket.recvfrom(ack_packet_size) # Receive
                total_data_recv.extend(ack_data)
                recv_size += len(ack_data)
            if(not is_ack_packet_corrupted(total_data_recv)): # Check checksums
                seq_number = string_to_int(total_data_recv[:seq_number_size].decode()) # Convert to string 
                self.lock_is_message_received.acquire() 
                self.lock_nsn.acquire()
                temp_base = self.base
                # If we get acknowledgement for greater value
                if(seq_number >= self.base):
                    self.is_message_received_in_between = True # Message received
                    self.base = seq_number + 1 # Set base
                    temp_base = self.base
                    self.gather_lock.release() # Inform that base is updated
                    while(len(self.waiting_sender) > 0):
                        self.waiting_sender.pop()
                        self.new_packet_to_send.acquire()
                        self.new_packet_to_send.notify() # Inform all senders that base is updated
                        self.new_packet_to_send.release()
                    self.lock_nsn.release()
                    self.lock_is_message_received.release()
                else:
                    self.lock_nsn.release()
                    self.lock_is_message_received.release()
                if(temp_base >= number_of_packets):
                    print("finished receiving")
                    break

            pass        
    
    # Get corresponding message index
    def get_index(self):
        for i in range(8):
            if(self.message_index[i] == self.next_seq_number):
                return i
        return -1 # Default

    # Data sender
    def sender(self, pathNumber, target_ip, target_port):
        sender_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # Open the socket
        is_receiving = False
        receiver_thread = None
        while not self.is_finished:
            self.lock_nsn.acquire()
            self.message_lock.acquire()
            
            seq = self.next_seq_number
            seq_index = self.get_index()
            
            if(self.base == number_of_packets):
                seq_index = 0
                self.is_finished = True
                self.message_lock.release()
                self.lock_nsn.release()
            elif(seq > self.base + default_window_size):
                self.waiting_sender.append(pathNumber) # Append as waiting
                self.message_lock.release()
                self.lock_nsn.release()
                self.new_packet_to_send.acquire()
                self.new_packet_to_send.wait()
                self.new_packet_to_send.release()
            else:
                if(seq_index != -1):
                    self.next_seq_number += 1
                else:
                    self.message_lock.release()
                    self.lock_nsn.release()
                    continue
                self.message_lock.release()
                self.lock_nsn.release()

            data_to_send = bytearray(str(seq).zfill(seq_number_size), default_encoding_type) # Initialize with seq number

            if(self.is_finished):
                data_to_send += bytearray('1'.zfill(1), default_encoding_type) # Add FIN 
            else:
                data_to_send += bytearray('0'.zfill(1), default_encoding_type) # Add FIN 


            if(self.is_finished):
                new_data0 = data_to_send.copy()
                new_data1 = data_to_send.copy()
                new_data2 = data_to_send.copy()

                new_data0 += bytearray(str(0).zfill(1), default_encoding_type) # Add pathid
                new_data1 += bytearray(str(1).zfill(1), default_encoding_type) # Add pathid
                new_data2 += bytearray(str(2).zfill(1), default_encoding_type) # Add pathid

                new_data0 += bytearray('2'.zfill(1), default_encoding_type) # Add destination id
                new_data1 += bytearray('2'.zfill(1), default_encoding_type) # Add destination id
                new_data2 += bytearray('2'.zfill(1), default_encoding_type) # Add destination id

                self.message_lock.acquire()
                new_data0 += self.message_queue[seq_index] # Add payload
                new_data1 += self.message_queue[seq_index] # Add payload
                new_data2 += self.message_queue[seq_index] # Add payload
                self.message_lock.release()

                new_data0 += generate_checksum(new_data0) # Add checksum
                new_data1 += generate_checksum(new_data1) # Add checksum
                new_data2 += generate_checksum(new_data2) # Add checksum

                for i in range(0, 10):
                    sender_socket.sendto(new_data1, (path1[1][0], path1[1][1]))
                    sender_socket.sendto(new_data2, (path2[1][0], path2[1][1]))
                    sender_socket.sendto(new_data0, (path0[1][0], path0[1][1]))

            data_to_send += bytearray(str(pathNumber).zfill(1), default_encoding_type) # Add pathid
            data_to_send += bytearray('2'.zfill(1), default_encoding_type) # Add destination id
            
            
            self.message_lock.acquire()
            data_to_send += self.message_queue[seq_index] # Add payload
            self.message_lock.release()
            
            data_to_send += generate_checksum(data_to_send) # Add checksum

            sender_socket.sendto(data_to_send, (target_ip, target_port))
            if(not is_receiving):
                is_receiving = True
                receiver_thread = threading.Thread(target=self.receiver, args=(sender_socket, )) # Run the receiver thread
                receiver_thread.start()
        receiver_thread.join()
        print("Tranmission finished")

experiment_type = sys.argv[1] # Type of the experiment given in project text
s = Source()


if(experiment_type == '0'):
    data_gather = threading.Thread(target=s.get_messages, args=('input.txt', ))
    data_gather.start()
    while(len(s.message_queue) < default_window_size):
        continue
    path = threading.Thread(target=s.sender, args=(0, path0[1][0], path0[1][1]))
    timeout_thread = threading.Thread(target=s.check_timeout, args=())

    
    path.start()
    timeout_thread.start()

    data_gather.join()
    path.join()
    timeout_thread.join()
    pass
else:
    data_gather = threading.Thread(target=s.get_messages, args=('input.txt', ))
    data_gather.start()
    while(len(s.message_queue) < default_window_size):
        continue
    first_path = threading.Thread(target=s.sender, args=(1, path1[1][0], path1[1][1]))
    second_path = threading.Thread(target=s.sender, args=(2, path2[1][0], path2[1][1]))
    timeout_thread = threading.Thread(target=s.check_timeout, args=())
    
    
    first_path.start()
    second_path.start()
    timeout_thread.start()
    
    data_gather.join()
    first_path.join()
    second_path.join()
    timeout_thread.join()
