#!/bin/bash
# spawn_clients.sh - 啟動多個 client 完成簡單請求, 用於驗證 server concurrency
NUM=${1:-10}
PORT=${2:-12345}
for i in $(seq 1 $NUM); do
  # each client sends SYSINFO then QUIT
  (
    echo "SYSINFO"
    sleep 1
    echo "QUIT"
  ) | ./client 127.0.0.1 $PORT -v > client_${i}.log 2>&1 &
done
echo "Spawned $NUM clients. Check client_*.log and server logs."
