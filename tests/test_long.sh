#!/usr/bin/env bash
# Test: sequential sleeps should accumulate (~6s for 5 + 1)
set -euo pipefail
printf "sleep 5\nsleep 1\necho done\n" > /tmp/wish_long.txt
start=$(date +%s)
./wish /tmp/wish_long.txt > /tmp/wish_long_out.txt 2>&1
end=$(date +%s)
elapsed=$((end-start))
echo "Elapsed: ${elapsed}s"
cat /tmp/wish_long_out.txt
if [ ${elapsed} -lt 5 ] || [ ${elapsed} -gt 8 ]; then
  echo "[FAIL] expected ~6s elapsed"
  exit 2
fi
echo "[PASS] long sleep test"
