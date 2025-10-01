#!/bin/bash

arm-none-eabi-gdb --nh --nx --eval-command="target extended-remote /dev/serial/by-id/usb-Raspberry_Pi_nomagic_probe_000000000001-if03" mbsp.elf

