#!/bin/bash

# 参数检查
if [ "$#" -ne 2 ]; then
    echo "用法: $0 <verilog_path> <output_json>"
    exit 1
fi

VERILOG_PATH="$1"
OUTPUT_JSON="$2"

yosys -p "
    read_verilog $VERILOG_PATH;
    hierarchy -top fifo;
    proc             
    opt_clean
    write_json $OUTPUT_JSON
    show -format png -prefix demo fifo
"


# hierarchy -top top 用于
# 1. 识别设计中的顶层模块（top module）
# 2. 同一模块用不同参数产生变体时， hierarchy 会对每个参数组合做专用解析
# 3. 数组式实例会被展开成多个独立实例，便于后续优化
# 4. 多驱动线按 AND/OR 语义归约为确定的组合网
# 5. 检查顶层是否唯一、子模块是否存在、接口是否匹配，必要时报错或修复。
# 6. ……

# write_json 用于将读取的 verilog 以 json 格式输出，输出内容见 help write_json