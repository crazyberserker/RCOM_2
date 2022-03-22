CC = gcc
CFLAGS = -Wall

APPLICATION_DIR = application/
PROTOCOL_DIR = protocol/
CABLE_DIR = cable/

TX_SERIAL_PORT = /dev/ttyS10
RX_SERIAL_PORT = /dev/ttyS11

TX_FILE = penguin.gif
RX_FILE = penguin-received.gif

all: application_main cable_app

application_main: $(APPLICATION_DIR)/main.c $(PROTOCOL_DIR)/*.c
	$(CC) $(CFLAGS) -o $@ $^ -I$(PROTOCOL_DIR)

cable_app: $(CABLE_DIR)/cable.c
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: run_tx
run_tx: application_main
	./application_main $(TX_SERIAL_PORT) tx $(TX_FILE)

.PHONY: run_rx
run_rx: application_main
	./application_main $(RX_SERIAL_PORT) rx $(RX_FILE)

.PHONY: run_cable
run_cable: cable_app
	./cable_app

.PHONY: check_files
check_files:
	diff -s $(TX_FILE) $(RX_FILE) || exit 0

.PHONY: clean
clean:
	rm application_main cable_app || exit 0
