CC = gcc
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic
PYTHON ?= python3

.PHONY: all test test-udp clean
all: telecom_uplink

debug:
	$(CC) -g -std=c99 -o uplink_debug ./telecom_uplink.c 

telecom_uplink: telecom_uplink.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c99 $(LDFLAGS) -o $@ $< $(LDLIBS)

test: telecom_uplink
	$(PYTHON) test_uplink.py

test-udp: telecom_uplink
	$(PYTHON) test_uplink.py --udp

clean:
	$(RM) telecom_uplink
