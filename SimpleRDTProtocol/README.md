<b>Simple RDT</b>

Simple program to send file from a source to a specific destination with given fixed path.
UDP is used and Reliable Data Transmission is achieved via additions. Utilized pipelining and multi-homing.


Can be run with two different options.

First: Sending data through one path to target destination
Second: Sending data through two path to target destination

We will call them first and second experiments.

R2 is the intermediate router in single path. R1-R3 is the intermediate routers in the second experiment.

The path data is stored in the utils.py in the lists of path0, path1, and path2. Change the content of the arrays before running the scripts.
path0=[(source_ip, source_port), (r2_ip, r2_port), (destination_ip, destination_port)]
path1=[(source_ip, source_port), (r1_ip, r1_port), (destination_ip, destination_port)]
path2=[(source_ip, source_port), (r3_ip, r3_port), (destination_ip, destination_port)]

Note that in path0 r2_ip is the interface in the link between source and r2 and the destination_ip is the ip of the interface between r2 and destination.
It follows similiar setup in the path1 and path2.

After paths are configured.

On the destination node run: 0 for 1st experiment 1 for 2nd experiment
python3 dest.py 0 
python3 dest.py 1

On the source node run: 0 for 1st experiment 1 for 2nd experiment
python3 src.py 0 
python3 src.py 1

On the intermediate routers run:
python3 intermediate.py the_ip_of_the_interface_to_source the_port_in_the_path_lists

Wait for it to finish. When it finishes the destination client will print the transmission time.



In order to test the results more clearly we can add some delay and loss to links using bash scripts.
For more realistic results use the bash scripts available under BashScripts directory:

Before conducting the 1st experiment:
For 5% loss 3ms delay:
Run Experiment1_configureDS_part0.sh on destination 
Run Experiment1_configureDS_part0.sh on source 
Run Experiment1_configureR_part0.sh on r2 

For 15% loss 3ms delay:
Run Experiment1_configureDS_part1.sh on destination 
Run Experiment1_configureDS_part1.sh on source 
Run Experiment1_configureR_part1.sh on r2

For 38% loss 3ms delay:
Run Experiment1_configureDS_part2.sh on destination 
Run Experiment1_configureDS_part2.sh on source 
Run Experiment1_configureR_part2.sh on r2


Before conducting the 2nd experiment:
For 5% loss 3ms delay:
Run Experiment2_configureDS_part0.sh on destination 
Run Experiment2_configureDS_part0.sh on source 
Run Experiment2_configureR_part0.sh on r1
Run Experiment2_configureR_part0.sh on r3

For 15% loss 3ms delay:
Run Experiment2_configureDS_part1.sh on destination 
Run Experiment2_configureDS_part1.sh on source 
Run Experiment2_configureR_part1.sh on r1
Run Experiment2_configureR_part1.sh on r3

For 38% loss 3ms delay:
Run Experiment2_configureDS_part2.sh on destination 
Run Experiment2_configureDS_part2.sh on source 
Run Experiment2_configureR_part2.sh on r1
Run Experiment2_configureR_part2.sh on r3
