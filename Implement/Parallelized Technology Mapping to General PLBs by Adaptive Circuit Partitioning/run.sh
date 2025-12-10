#!/bin/bash

# 参数检查
if [ "$#" -ne 1 ]; then
    echo "用法: $0 <verilog_path>"
    exit 1
fi

VERILOG_PATH="$1"

yosys -p "
    read_verilog $VERILOG_PATH;
    hierarchy -top fifo;
    proc             
    opt_clean
    write_json data/input.json
"

# 生成超图，转换为 KaHyPar 的输入文件
xmake run verilog2kahypar ../data/input.json

# 运行 KaHyPar 进行划分
KaHyPar -h kahypar/run_hmetis.txt -k 3 -e 0.03 -o km1 -m direct -p kahypar/km1_kKaHyPar_sea20.ini -w true

# 生成划分子集对应的 Verilog 文件
cd partitioned_verilog
rm -rf *.v
cd ..
xmake run partition2verilog ../data/input.json ../kahypar/run_hmetis.txt.names ../kahypar/run_hmetis.txt.part3.epsilon0.03.seed-1.KaHyPar ../partitioned_verilog

JOBS=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
cd partitioned_verilog
ls *.v | sed 's/\.v$//' | xargs -I{} -P "$JOBS" sh -c 'yosys -q -p "read_verilog {}.v; hierarchy -top {}; synth; write_verilog {}_synth.v"'
cd ..

