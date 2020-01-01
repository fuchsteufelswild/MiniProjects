import hashlib

default_window_size = 8 # Initial window size
default_encoding_type = "utf-8" # Default encoding type 

default_timer = 0.01

####
'''
Header form for sending packets
Seq#|FINFLAG|
PATHID|DESTID|Data|CHECKSUM
'''
header_size = 8 # Size of the header
seq_number_size = 5 # Seq number
flags_size = 1 # Size of all flags syn, fin
path_id_size = 1 # Size of the path array id
dest_id_size = 1 # Size of the destination array id
packet_id_size = 4 # Size of the packet id
checksum_size = 32 # Size of the checksum
####
####
'''
Header format for ack messages
Seq#|PATHID|DESTID|Checksum|
'''

total_file_size = 5000000
payload_size = 500
payload_packet_size = header_size + payload_size + checksum_size
number_of_packets = int(total_file_size / payload_size)

ack_packet_size = seq_number_size + path_id_size + dest_id_size + checksum_size

path0 = [('127.0.0.1', 12346), ('10.10.2.1', 25212), ('10.10.5.2', 12349)] # Shortest path experiment 1 
path1 = [('127.0.0.1', 12346), ('10.10.1.2', 25211), ('10.10.4.2', 12349)] # Experiment 2 first path 
path2 = [('127.0.0.1', 12346), ('10.10.3.2', 25213), ('10.10.7.1', 25210)] # Experiment 2 second path 

def generate_checksum(packet):
    md5 = hashlib.md5()
    md5.update(packet)

    return bytearray(md5.hexdigest(), default_encoding_type)


def is_packet_corrupted(packet):
    for i in range(seq_number_size):
        if(ord(packet[i:i+1]) < ord('0') or ord(packet[i:i+1]) > ord('9')):
            return True
    return generate_checksum(packet[:header_size + payload_size]) != packet[header_size + payload_size:]

def is_ack_packet_corrupted(packet):
    for i in range(seq_number_size):
        if(ord(packet[i:i+1]) < ord('0') or ord(packet[i:i+1]) > ord('9')):
            return True
    return generate_checksum(packet[:seq_number_size + path_id_size + dest_id_size]) != packet[seq_number_size + path_id_size + dest_id_size:]

def string_to_int(string):
    res = 0
    string = string[::-1]
    for i in range(len(string)):
        res += int(string[i]) * pow(10, i)
    return res
