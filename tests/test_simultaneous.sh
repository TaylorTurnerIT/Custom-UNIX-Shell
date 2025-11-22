#!/usr/bin/env bash
# Test: two simultaneous sleeps where one is backgrounded. Total runtime should be ~2s
set -euo pipefail
printf "sleep 2 &\nsleep 2\necho done\n" > /tmp/wish_sim.txt
start=$(date +%s)
./wish /tmp/wish_sim.txt > /tmp/wish_sim_out.txt 2>&1
end=$(date +%s)
elapsed=$((end-start))
echo "Elapsed: ${elapsed}s"
cat /tmp/wish_sim_out.txt
if [ ${elapsed} -le 1 ] || [ ${elapsed} -ge 4 ]; then
  echo "[FAIL] expected ~2s elapsed"
  exit 2
fi
echo "[PASS] simultaneous test"
