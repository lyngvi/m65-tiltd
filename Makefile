SENDRAW=send-raw
SENDRAW_OBJECTS=send-raw.o replay.o util.o

MONITOR=monitor
MONITOR_OBJECTS=monitor.o util.o

ALL_TARGETS=$(SENDRAW) $(MONITOR)
ALL_OBJECTS=$(SENDRAW_OBJECTS) $(MONITOR_OBJECTS)

CFLAGS:=$(shell pkg-config --cflags libusb-1.0)
LIBS:=$(shell pkg-config --libs libusb-1.0)

all: $(ALL_TARGETS)

$(SENDRAW): $(SENDRAW_OBJECTS)
	gcc -o $(SENDRAW) $(SENDRAW_OBJECTS) $(LIBS)

$(MONITOR): $(MONITOR_OBJECTS)
	gcc -o $(MONITOR) $(MONITOR_OBJECTS) $(LIBS)

clean:
	rm -f $(ALL_TARGETS) $(ALL_OBJECTS) replay.c

%.o: %.c
	gcc -c $(CFLAGS) $^

replay.c: replay.py
	./replay.py > replay.c