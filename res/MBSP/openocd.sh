#!/bin/bash

openocd  -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "adapter speed 100" -c "program mbsp.elf verify reset exit"

