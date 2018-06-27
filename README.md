# project2-Teona-Sam
Project II (Task I): Transmission Control Protocol (TCP)

## How to Build

cd ~/github/project2/rdt2.0/src

mininet@mininet-vm:~/github/project2/rdt2.0/src$ make


### run mininet
sudo mn --link tc,bw=10,delay=10ms,loss=2

mininet> xterm h1 h2

### Two terminal will popup for h1 and h2; Run sender and receiver on these two terminal

### terminal node h1:
./rdt2.0/obj/rdt_receiver 60001 FILE_RCVD

### terminal node h2:
./rdt2.0/obj/rdt_sender 10.0.0.1 60001 small_file.bin


### verify the two files
cksum FILE_RCVD small_file.bin
