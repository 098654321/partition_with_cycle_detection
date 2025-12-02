#!/bin/bash

# 参数检查
if [ "$#" -ne 2 ]; then
    echo "用法: $0 <verilog_path> <output_json>"
    exit 1
fi

VERILOG_PATH="$1"
OUTPUT_EDIF="$2"

yosys -p "
    read_verilog $VERILOG_PATH;
    synth -top fifo
    write_edif $OUTPUT_EDIF    
    show -format png -prefix demo fifo
" > debug.log 2>&1


