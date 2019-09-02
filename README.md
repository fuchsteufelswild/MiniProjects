# MiniProjects
Simple programs with one spesific goal
All applications are written for linux os

- MapReduce (MapReduce implementation in C language with many processes) { Command Line Arguments: (number of mappers) Mapper Reducer < input.txt }

- Mining Simulator (Basic mining simulator implementation using multithreads with 4 types of actors namely; Miners (Extracts mines) , Transporters (Takes ores from mines to smelters and foragers), Smelters (Produces copper and iron ingots), Foragers(Produces coal ingots))

Input Spesifications: <br>
{ <br>
1st line Nm -> number of mines <br>
next Nm lines -> (Waiting and production interval) (Capacity) (OreType {Iron: 0, Copper: 1, Coal: 2}) (Total amount of extraction) <br>
next line Nt -> Number of transporters <br>
next Nt lines -> (Load time of a transporter) <br>
next line Ns -> Number of smelters <br>
next Ns lines -> (Production interval) (Capacity) (OreType {Iron: 0, Copper: 1}) <br>
next line Nf -> Number of foundries <br> 
next Nf lines -> (Production interval) (Capacity) <br>
} <br>

- Simple Ext2 File System (Ext2 file system implementation with minimal features; initialization, copying files and directories with arbitrary size, and creation) 
{ Command Line Arguments: image source (target inode or directory) }
