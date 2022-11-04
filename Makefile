ENABLETILT=m65-tiltd
ENABLETILT_OBJECTS=enable-tilt.o util.o version.o

ALL_TARGETS=$(ENABLETILT)
ALL_OBJECTS=$(ENABLETILT_OBJECTS)

CFLAGS:=$(shell pkg-config --cflags libusb-1.0)
LIBS:=$(shell pkg-config --libs libusb-1.0)

all: $(ALL_TARGETS)

$(ENABLETILT): $(ENABLETILT_OBJECTS)
	gcc -o $(ENABLETILT) $(ENABLETILT_OBJECTS) $(LIBS)

clean:
	rm -f $(ALL_TARGETS) $(ALL_OBJECTS) version.c

%.o: %.c
	gcc -c $(CFLAGS) $^

version.c: .git/HEAD
	echo "const char AppVersion[] = \"$(shell git describe --abbrev=4 --always)\";" > version.c
