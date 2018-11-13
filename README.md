# TCP implementation
## project2-Teona-Sam
Project II (Task I): Transmission Control Protocol (TCP)
This TCP implementation uses congestion control and ss Threshold to model the transmission control that happens in TCP over internet connections.

## Requirements
mininet VM. Can be built from source or a pre-built Mininet VM can be downloaded at http://mininet.org/download/.

## How to Build
Use the make file in the `src` directory to build the executable files.
```
mininet@mininet-vm:~$ make
```

### Run mininet
Start the mininet servers ith different parameters for bandwidth, delay and loss proportions.
```
sudo mn --link tc,bw=10,delay=10ms,loss=2

mininet> xterm h1 h2
```
### Two terminal will popup for h1 and h2; Run sender and receiver on these two terminal

### Terminal node h1:
```
./rdt2.0/obj/rdt_receiver <Port num i.e. 60001> <rcv_file_name>
```
### Terminal node h2:
```
./rdt2.0/obj/rdt_sender 10.0.0.1  <Same Port num as receiver i.e. 60001> <send_file_name.bin>
```

### Verify the two files are the same
```
cksum <rcv_file_name> <send_file_name.bin>
```
