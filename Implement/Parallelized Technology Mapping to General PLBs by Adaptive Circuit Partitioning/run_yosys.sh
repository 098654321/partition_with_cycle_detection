#!/bin/bash

set -euo pipefail

# 参数检查
if [ "$#" -ne 3 ]; then
    echo "用法: $0 <verilog_path> <top> <output_edif>"
    exit 1
fi

VERILOG_PATH="$1"
TOP="$2"
OUTPUT_EDIF="$3"

# 结果与日志目录（含空格路径需加引号）
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROFILE_DIR="$SCRIPT_DIR/yosys_profile"
mkdir -p "$PROFILE_DIR/logs"
CSV_FILE="$PROFILE_DIR/profile.csv"
JSON_FILE="$PROFILE_DIR/current.json"

# 初始化 CSV
echo "step,label,real_s,user_s,sys_s,peak_bytes,maxrss_bytes" > "$CSV_FILE"

# 工具函数：运行单步并记录 /usr/bin/time -l 输出
run_step() {
  local step="$1"; shift
  local label="$1"; shift
  local yosys_cmd="$1"; shift
  local time_file="$PROFILE_DIR/logs/${step}_${label}.time"
  local log_file="$PROFILE_DIR/logs/${step}_${label}.log"

  # 执行并记录时间/内存，stderr 重定向到 time_file，stdout 到 log_file
  /usr/bin/time -l yosys -q -t -d -l "$log_file" -p "$yosys_cmd" 2> "$time_file"

  # 解析时间与峰值内存（macOS /usr/bin/time -l 输出格式）
  local real user sys peak maxrss
  real=$(awk '/real/{print $1}' "$time_file" | tail -n1)
  user=$(awk '/user/{print $1}' "$time_file" | tail -n1)
  sys=$(awk '/sys/{print $1}' "$time_file" | tail -n1)
  peak=$(awk '/peak memory footprint/{print $1}' "$time_file" | tail -n1)
  maxrss=$(awk '/maximum resident set size/{print $1}' "$time_file" | tail -n1)

  echo "$step,$label,$real,$user,$sys,$peak,$maxrss" >> "$CSV_FILE"
}

# 工具函数：仅统计 pass 的时间与内存（排除 read_json / write_json）
run_step_pass() {
  local step="$1"; shift
  local label="$1"; shift
  local pass_cmd="$1"; shift

  local base_time_file="$PROFILE_DIR/logs/${step}_${label}_baseline.time"
  local base_log_file="$PROFILE_DIR/logs/${step}_${label}_baseline.log"
  local time_file="$PROFILE_DIR/logs/${step}_${label}.time"
  local log_file="$PROFILE_DIR/logs/${step}_${label}.log"

  # 基线：只读写设计，不执行 pass
  /usr/bin/time -l yosys -q -t -d -l "$base_log_file" -p "read_json \"$JSON_FILE\"; write_json \"$JSON_FILE\"" 2> "$base_time_file"

  # 总运行：读入 + pass + 写回
  /usr/bin/time -l yosys -q -t -d -l "$log_file" -p "read_json \"$JSON_FILE\"; $pass_cmd; write_json \"$JSON_FILE\"" 2> "$time_file"

  # 解析时间与内存
  local base_real base_user base_sys base_peak base_maxrss
  local real user sys peak maxrss

  base_real=$(awk '/real/{print $1}' "$base_time_file" | tail -n1)
  base_user=$(awk '/user/{print $1}' "$base_time_file" | tail -n1)
  base_sys=$(awk '/sys/{print $1}' "$base_time_file" | tail -n1)
  base_peak=$(awk '/peak memory footprint/{print $1}' "$base_time_file" | tail -n1)
  base_maxrss=$(awk '/maximum resident set size/{print $1}' "$base_time_file" | tail -n1)

  real=$(awk '/real/{print $1}' "$time_file" | tail -n1)
  user=$(awk '/user/{print $1}' "$time_file" | tail -n1)
  sys=$(awk '/sys/{print $1}' "$time_file" | tail -n1)
  peak=$(awk '/peak memory footprint/{print $1}' "$time_file" | tail -n1)
  maxrss=$(awk '/maximum resident set size/{print $1}' "$time_file" | tail -n1)

  # 计算 pass 估计值（时间作差；内存取增量近似）
  # 注意：峰值内存非线性，增量仅为近似估计
  python3 - "$step" "$label" "$CSV_FILE" \
    "${base_real:-0}" "${base_user:-0}" "${base_sys:-0}" "${base_peak:-0}" "${base_maxrss:-0}" \
    "${real:-0}" "${user:-0}" "${sys:-0}" "${peak:-0}" "${maxrss:-0}" <<'PY'
import sys
step,label,csv = sys.argv[1], sys.argv[2], sys.argv[3]
br,bu,bs,bp,bm,r,u,s,p,m = [float(x or 0.0) for x in sys.argv[4:14]]
real = max(0.0, r - br)
user = max(0.0, u - bu)
sys_t = max(0.0, s - bs)
peak = max(0.0, p - bp)
maxrss = max(0.0, m - bm)
with open(csv, 'a') as f:
    f.write(f"{step},{label},{real:.6f},{user:.6f},{sys_t:.6f},{int(peak)},{int(maxrss)}\n")
PY
}

# 第 0 步：读取 verilog，层次化，并写出初始 IL
run_step "00" "read_and_hierarchy" \
  "read_verilog \"$VERILOG_PATH\"; hierarchy -check -top $TOP; write_json \"$JSON_FILE\""

# 逐步运行各 pass，每步先读入 IL，再写回 IL 以保持设计状态
run_step_pass "01" "proc"               "proc"
run_step_pass "02" "opt_expr"           "opt_expr"
run_step_pass "03" "opt_clean"          "opt_clean"
run_step_pass "04" "check"              "check"
run_step_pass "05" "opt_nodffe_nosdff"  "opt -nodffe -nosdff"
run_step_pass "06" "fsm"                "fsm"
run_step_pass "07" "opt"                "opt"
run_step_pass "08" "wreduce"            "wreduce"
run_step_pass "09" "peepopt"            "peepopt"
run_step_pass "10" "opt_clean"          "opt_clean"
run_step_pass "11" "alumacc"            "alumacc"
run_step_pass "12" "share"              "share"
run_step_pass "13" "opt"                "opt"
run_step_pass "14" "memory_nomap"       "memory -nomap"
run_step_pass "15" "opt_clean"          "opt_clean"
run_step_pass "16" "opt_fast_full"      "opt -fast -full"
run_step_pass "17" "memory_map"         "memory_map"
run_step_pass "18" "opt_full"           "opt -full"
run_step_pass "19" "techmap"            "techmap"
run_step_pass "20" "opt_fast"           "opt -fast"
run_step_pass "21" "abc_fast"           "abc -fast"
run_step_pass "22" "opt_fast"           "opt -fast"
run_step_pass "23" "hierarchy_check"    "hierarchy -check"
run_step_pass "24" "stat"               "stat"
run_step_pass "25" "check"              "check"
run_step_pass "26" "write_edif"         "write_edif \"$OUTPUT_EDIF\""

echo "统计完成：请查看 \"$CSV_FILE\" 与 \"$PROFILE_DIR/logs\""
