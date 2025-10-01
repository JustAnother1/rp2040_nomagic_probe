#!/bin/bash
arm-none-eabi-gdb -nx --batch \
  -ex 'set verbose' \
  -ex 'file mbsp.elf' \
  -ex 'target extended-remote /dev/serial/by-id/usb-Raspberry_Pi_nomagic_probe_000000000001-if03' \
  -ex 'load' \
  -ex 'compare-sections' \
  -ex 'kill' \
  mbsp.elf

