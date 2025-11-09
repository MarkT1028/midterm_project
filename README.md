# 1.切到根目錄
# 2.編譯：make
若要啟用編譯時 debug 訊息（compile-time debug），
執行：
make clean
CFLAGS="-D_COMPILE_DEBUG -Wall -Wextra -fPIC -O2" make
# 3.啟動 server（背景或另外一個終端）：
./server -v
LOG_LEVEL=DEBUG ./server
# 4.在另一個終端啟動 client：
./client 127.0.0.1 12345 -v
 輸入 SYSINFO 或 ECHO Hello 等
 # 5.驗證 10 clients 同時處理：
 ./server -v
 在另一個終端：
./scripts/spawn_clients.sh 10 12345
 等待片刻，再看所有 client_*.log 與 server 的 stderr 輸出
 # 6.用tcpdump風包截取
 sudo tcpdump -i
