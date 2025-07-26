#!/bin/bash

tshark -i enx021234567890 -w capture_$(date +%Y-%m-%d_%H-%M-%S).pcap > /dev/null 2>&1 &

#echo "$(date +%Y-%m-%d_%H-%M-%S)"
# enx021234567890
