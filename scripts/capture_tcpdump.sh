#!/bin/bash
# capture loopback pcap for port 12345
OUT=${1:-capture_loop.pcap}
sudo tcpdump -i lo port 12345 -w $OUT
# press Ctrl+C to stop capture
