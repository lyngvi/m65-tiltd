#ifndef STUFF_H
#define STUFF_H

#include <stdint.h>

#define USB_VENDOR_CORSAIR (0x1b1c)
#define USB_MODEL_CORSAIR_SLIPSTREAM_WIRELESS (0x1ba6)
#define CORSAIR_SLIPSTREAM_CONTROL_ENDPOINT (4)
#define CORSAIR_SLIPSTREAM_CONTROL_INTERFACE (1)
#define CORSAIR_SLIPSTREAM_TILT_ENDPOINT (3)
#define CORSAIR_SLIPSTREAM_TILT_INTERFACE (2)
#define PACKET_SIZE (64) // all transit on interface 4 seems to be exactly this size, zero-padded to it
#define LIBUSB_BOGUS_INCOMPLETE (LIBUSB_ERROR_OTHER - 1)
#define CORSAIR_INTERRUPT_TRANSACT_TIMEOUT_S (5)

// in util.c
// neat hex printout of up to sz bytes of data (stopping when the rest are zeroes)
void print_hex(const char* label, const uint8_t* data, int sz);

// stuff outp with the amount of time remaining until target, or 0 if we're there
void time_until(struct timeval* outp, struct timeval* target);

extern const char AppVersion[];

#endif // ndef STUFF_H
