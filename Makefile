CC = gcc
CFLAGS = -Wall -Wextra -fPIC -O2
LDFLAGS = -shared
PREFIX = /usr/local
SRC = src
BUILD = build
LIBNAME = libutils.so

.PHONY: all clean install run_server run_client spawn10

all: $(BUILD)/$(LIBNAME) server client

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/utils.o: $(SRC)/utils.c $(SRC)/utils.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/$(LIBNAME): $(BUILD)/utils.o
	$(CC) $(LDFLAGS) -o $@ $^

server: $(SRC)/server.c $(BUILD)/$(LIBNAME) $(SRC)/utils.h
	$(CC) $(CFLAGS) $(SRC)/server.c -L$(BUILD) -lutils -o server -Wl,-rpath,'$$ORIGIN/$(BUILD)'

client: $(SRC)/client.c $(BUILD)/$(LIBNAME) $(SRC)/utils.h
	$(CC) $(CFLAGS) $(SRC)/client.c -L$(BUILD) -lutils -o client -Wl,-rpath,'$$ORIGIN/$(BUILD)'

clean:
	rm -rf server client $(BUILD)/* build || true

install: all
	sudo cp $(BUILD)/$(LIBNAME) $(PREFIX)/lib/
	sudo ldconfig

# convenience
run_server:
	./server

run_client:
	./client

spawn10:
	./scripts/spawn_clients.sh

