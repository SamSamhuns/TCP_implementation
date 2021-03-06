# TCP implementation

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/3285d3db8d1f4023bd1bc66dcdb265f1)](https://www.codacy.com/app/samhunsadamant/TCP_implementation?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=SamSamhuns/TCP_implementation&amp;utm_campaign=Badge_Grade) [![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

## project2-Teona-Sam
Project II (Task I): Transmission Control Protocol (TCP)
This TCP implementation uses congestion control and ss Threshold to model the transmission control that happens in TCP over internet connections.

Sample Congestion window graph.
<img src="https://raw.githubusercontent.com/SamSamhuns/TCP_implementation/master/Grapher-congestion-window-output.png" width='200%'>

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
rdt_receiver <Port num i.e. 60001> <rcv_file_name>
```
### Terminal node h2:
```
rdt_sender 10.0.0.1  <Same Port num as receiver i.e. 60001> <send_file_name.bin>
```

### Verify the two files are the same and generate a congestion window graph
The Grapher.py script uses the CND.csv file generated by the rdt_sender file.
```
cksum <rcv_file_name> <send_file_name.bin>
python3 Grapher.py
```
