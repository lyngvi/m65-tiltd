ENABLETILT=enable-tilt
ENABLETILT_OBJECTS=enable-tilt.o replay.o util.o

MONITOR=monitor
MONITOR_OBJECTS=monitor.o util.o

ALL_TARGETS=$(ENABLETILT) $(MONITOR)
ALL_OBJECTS=$(ENABLETILT_OBJECTS) $(MONITOR_OBJECTS)

CFLAGS:=$(shell pkg-config --cflags libusb-1.0)
LIBS:=$(shell pkg-config --libs libusb-1.0)

all: $(ALL_TARGETS)

$(ENABLETILT): $(ENABLETILT_OBJECTS)
	gcc -o $(ENABLETILT) $(ENABLETILT_OBJECTS) $(LIBS)

$(MONITOR): $(MONITOR_OBJECTS)
	gcc -o $(MONITOR) $(MONITOR_OBJECTS) $(LIBS)

clean:
	rm -f $(ALL_TARGETS) $(ALL_OBJECTS) replay.c

%.o: %.c
	gcc -c $(CFLAGS) $^

replay.c: replay.py
	./replay.py > replay.c