CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic

.PHONY: all clean
all: telecom_downlink

telecom_downlink: telecom_downlink.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c99 $(LDFLAGS) -o $@ $< $(LDLIBS)

clean:
	$(RM) telecom_downlink
