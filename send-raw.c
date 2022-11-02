#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include "stuff.h"

#include <libusb-1.0/libusb.h>

// Responses were on ep 84
// They were prefixed by URB headers
//    "\x1b\x00\x50\x50\xa5\x65\x8f\xce\xff\xff\x00\x00\x00\x00\x09\x00"
//    "\x00\x01\x00\x02\x00\x04\x01\x40\x00\x00\x00"
// but these are not meaningful.
// Reference: m65pro-tilt-controls-2.pcapng commands, where we start iCue
static uint8_t cmd1763[PACKET_SIZE] = "\x08\x02\x11";         // Response 1765: 00 02 00 1c 1b
static uint8_t cmd1767[PACKET_SIZE] = "\x08\x02\x12";         // Response 1769: 00 02 00 a6 1b
static uint8_t cmd1771[PACKET_SIZE] = "\x08\x02\x59";         // Response 1773: 00 02 05 a4 6a 00 20
static uint8_t cmd1775[PACKET_SIZE] = "\x08\x02\x5a";         // Response 1777: 00 02 05 a4 6a 00 20
static uint8_t cmd1779[PACKET_SIZE] = "\x08\x02\x5b";         // Response 1781: 00 02 05 a4 6a 00 20
static uint8_t cmd1783[PACKET_SIZE] = "\x08\x02\x03";         // Response 1785: 00 02 00 01
static uint8_t cmd1787[PACKET_SIZE] = "\x08\x01\x03\x00\x02"; // Response 1789: 00 01 00 01

static uint8_t cmd1791[PACKET_SIZE] = "\x08\x02\x13";         // Response 1793: 00 02 00 05 06 7e
static uint8_t cmd1795[PACKET_SIZE] = "\x08\x02\x03";         // Response 1797: 00 02 00 02
static uint8_t cmd1799[PACKET_SIZE] = "\x08\x02\x5f";         // Response 1801: 00 02 00 02
static uint8_t cmd1803[PACKET_SIZE] = "\x08\x02\x74";         // Response 1805: 00 02 00 05
static uint8_t cmd1807[PACKET_SIZE] = "\x08\x02\x01";         // Response 1809: 00 02 00 04
static uint8_t cmd1811[PACKET_SIZE] = "\x08\x02\x03";         // Response 1813: 00 02 00 02

static uint8_t cmd1815[PACKET_SIZE] = "\x08\x02\x13";         // Response 1817: 00 02 00 05 06 7e
static uint8_t cmd1819[PACKET_SIZE] = "\x08\x02\x14";         // Response 1821: 00 02 00 00 01 1b
static uint8_t cmd1823[PACKET_SIZE] = "\x08\x0d\x00\x05";     // Response 1825: 00 0d 00 00 01 1b
static uint8_t cmd1827[PACKET_SIZE] = "\x08\x09";             // Response 1829: 00 09 00 08 0e 08
static uint8_t cmd1831[PACKET_SIZE] = "\x08\x08";             // Response 1833: 00 08 00 51 9e de 46 66 ee 45 d0

static uint8_t cmd1835[PACKET_SIZE] = "\x08\x05\x01";         // Response 1837: 00 05 00 51 9e de 46 66 ee 45 d0
static uint8_t cmd1839[PACKET_SIZE] = "\x08\x02\x96";         // Response 1841: 00 02 00 05 00 00 00 66 ee 45 d0
static uint8_t cmd1843[PACKET_SIZE] = "\x08\x0d\x00\x27";     // Response 1845: 00 0d 01 05 00 00 00 66 ee 45 d0
static uint8_t cmd1847[PACKET_SIZE] = "\x08\x02\x4b";         // Response 1849: 00 02 00 04 00 00 00 66 ee 45 d0
static uint8_t cmd1851[PACKET_SIZE] = "\x08\x02\x66";         // Response 1853: 00 02 00 05 00 00 00 66 ee 45 d0
static uint8_t cmd1855[PACKET_SIZE] = "\x08\x02\x67";         // Response 1857: 00 02 00 05 00 00 00 66 ee 45 d0
static uint8_t cmd1859[PACKET_SIZE] = "\x08\x02\x68";         // Response 1861: 00 02 05 a4 6a 00 20 66 ee 45 d0
static uint8_t cmd1863[PACKET_SIZE] = "\x08\x0d\x00\x05";     // Response 1865: 00 0d 00 a4 6a 00 20 66 ee 45 d0
static uint8_t cmd1867[PACKET_SIZE] = "\x08\x09";             // Response 1869: 00 09 00 08 0e 08 00 00 00 45 d0
static uint8_t cmd1871[PACKET_SIZE] = "\x08\x08";             // Response 1873: 00 08 00 51 9e de 46 66 ee 45 d0

static uint8_t cmd1875[PACKET_SIZE] = "\x08\x05\x01";         // Response 1877: 00 05 00 51 9e de 46 66 ee 45 d0
static uint8_t cmd1879[PACKET_SIZE] = "\x08\x0d\x00\x24";     // Response 1881: 00 0d 00 51 9e de 46 66 ee 45 d0
static uint8_t cmd1883[PACKET_SIZE] = "\x08\x09";             // Response 1885: 00 09 00 03 0e 03 00 00 00 45 d0
static uint8_t cmd1887[PACKET_SIZE] = "\x08\x08";             // Response 1889: 00 08 00 21
static uint8_t cmd1891[PACKET_SIZE] = "\x08\x05\x01";         // Response 1893: 00 05 00 21
static uint8_t cmd1895[PACKET_SIZE] = "\x08\x02\x73";         // Response 1897: 00 02
static uint8_t cmd1899[PACKET_SIZE] = "\x08\x02\x96";         // Response 1901: 00 02 00 05
static uint8_t cmd1903[PACKET_SIZE] = "\x08\x02\x36";         // Response 1905: 00 02 00 02
static uint8_t cmd1907[PACKET_SIZE] = "\x09\x02\x11";         // Response 1909: 01 02 00 1c 1b
static uint8_t cmd1911[PACKET_SIZE] = "\x09\x02\x12";         // Response 1913: 01 02 00 b5 1b
static uint8_t cmd1915[PACKET_SIZE] = "\x09\x02\x59";         // Response 1917: 01 02 05 40
static uint8_t cmd1919[PACKET_SIZE] = "\x09\x02\x5a";         // Response 1921: 01 02 05 40
static uint8_t cmd1923[PACKET_SIZE] = "\x09\x02\x5b";         // Response 1925: 01 02 05 40
static uint8_t cmd1927[PACKET_SIZE] = "\x09\x02\x03";         // Response 1929: 01 02 00 01
static uint8_t cmd1931[PACKET_SIZE] = "\x09\x01\x03\x00\x02"; // Response 1933: 01 01           // This appears to be required to stop getting error codes from attempts to change config
static uint8_t cmd1935[PACKET_SIZE] = "\x09\x02\x13";         // Response 1937: 01 02 00 05 07
static uint8_t cmd1939[PACKET_SIZE] = "\x09\x02\x03";         // Response 1941: 01 02 00 02
static uint8_t cmd1943[PACKET_SIZE] = "\x09\x02\x5f";         // Response 1945: 01 02 00 02
static uint8_t cmd1947[PACKET_SIZE] = "\x09\x02\x74";         // Response 1949: 01 02 00 07
static uint8_t cmd1951[PACKET_SIZE] = "\x09\x02\x01";         // Response 1953: 01 02 00 04
static uint8_t cmd1955[PACKET_SIZE] = "\x09\x02\x03";         // Response 1957: 01 02 00 02
static uint8_t cmd1959[PACKET_SIZE] = "\x09\x02\x13";         // Response 1961: 01 02 00 05 07 2a
static uint8_t cmd1963[PACKET_SIZE] = "\x09\x02\x14";         // Response 1965: 01 02 00 00 05 0e
static uint8_t cmd1967[PACKET_SIZE] = "\x09\x0d\x00\x05";     // Response 1969: 01 0d
static uint8_t cmd1971[PACKET_SIZE] = "\x09\x09";             // Response 1973: 01 09 00 08 0e 08
static uint8_t cmd1975[PACKET_SIZE] = "\x09\x08";             // Response 1977: 01 08 00 51 9e de 46 66 ee 45 d0
static uint8_t cmd1979[PACKET_SIZE] = "\x09\x05\x01";         // Response 1981: 01 05
static uint8_t cmd1983[PACKET_SIZE] = "\x09\x02\x96";         // Response 1985: 01 02 00 05
static uint8_t cmd1987[PACKET_SIZE] = "\x09\x0d\x00\x27";     // Response 1989: 01 0d 03
static uint8_t cmd1991[PACKET_SIZE] = "\x09\x05\x01";         // Response 1993: 01 05
static uint8_t cmd1995[PACKET_SIZE] = "\x09\x0d\x00\x27";     // Response 1997: 01 0d 03
static uint8_t cmd1999[PACKET_SIZE] = "\x08\x0d\x00\x24";     // Response 2003: 00 0d 00 02            ?
static uint8_t cmd2000[PACKET_SIZE] = "\x09\x0d\x00\x05";     // Response 200....?
static uint8_t cmd2005[PACKET_SIZE] = "\x08\x09";             // Response 2007: 01 0d are we out of sync?
static uint8_t cmd2009[PACKET_SIZE] = "\x09\x09";             // Response 2010: 00 09 00 03 0e 03
static uint8_t cmd2012[PACKET_SIZE] = "\x08\x08";             // Response 2015: 00 08 00 21


static uint8_t cmd2032[PACKET_SIZE] = "\x09\x05\x01";         // Response 2037: 01 05
static uint8_t cmd2055[PACKET_SIZE] = "\x09\x0d\x00\x02";     // Response 2057: 01 0d                   // this enables cmd2059 to work?
static uint8_t cmd2059[PACKET_SIZE] = "\x09\x06\x00\x08\x00\x00\x00\x01\x01\x01\x00\x00\x00\x00\x01"; // Response 2061: 01 06

static uint8_t cmd2107[PACKET_SIZE] = "\x09\x01\xa8";         // Response 2112: 01 01               ?
static uint8_t cmd2137[PACKET_SIZE] = "\x09\x01\xbd\x00\x01"; // Response 2139: 01 01
static uint8_t cmd2157[PACKET_SIZE] = "\x09\x01\xbe\x00\x01"; // Response 2159: 01 01
static uint8_t cmd2177[PACKET_SIZE] = "\x09\x01\xbb";         // Response 2179: 01 01
static uint8_t cmd2197[PACKET_SIZE] = "\x09\x01\xbc";         // Response 2199: 01 01
static uint8_t cmd2217[PACKET_SIZE] = "\x09\x01\x07";         // Response 2219: 01 01

static uint8_t cmd2253[PACKET_SIZE] = "\x09\x01\xa8";         // Response 2255: 01 01
static uint8_t cmd2277[PACKET_SIZE] = "\x09\x01\xbd\x00\x01"; // Response 2279: 01 01
static uint8_t cmd2297[PACKET_SIZE] = "\x09\x01\xbe\x00\x01"; // Response 2299: 01 01
static uint8_t cmd2317[PACKET_SIZE] = "\x09\x01\xbb";         // Response 2319: 01 01
static uint8_t cmd2337[PACKET_SIZE] = "\x09\x01\xbc";         // Response 2339: 01 01
static uint8_t cmd2357[PACKET_SIZE] = "\x09\x01\x07";         // Response 2359: 01 01

static uint8_t cmd2361[PACKET_SIZE] = "\x09\x01\x21\x00\xb8"; // Response 2363: 01 01
static uint8_t cmd2365[PACKET_SIZE] = "\x09\x01\x21\x00\xb8"; // Response 2367: 01 01 // this appears to have borked my horizontal dpi
static uint8_t cmd2373[PACKET_SIZE] = "\x09\x01\x02\x00\xf4\x01"; // Response 2375: 01 01 

static uint8_t cmd2409[PACKET_SIZE] = "\x09\x02\x0e";         // Response 2411: 01 02 00 a0 bb 0d
static uint8_t cmd2413[PACKET_SIZE] = "\x09\x02\x37";         // Response 2415: 01 02 00 e0 93 04
static uint8_t cmd2417[PACKET_SIZE] = "\x09\x02\x79";         // Response 2419: 01 02 00 01

// Tilt configuration - I think. Range 7, 8, a is very strange for 3 out of 4 tilt controls; not sure what the last one it.
// This is the last configuration command sequence observed before we see live tilting so, meh
static uint8_t cmd2421[PACKET_SIZE] = "\x09\x01\xba\x00\x00"; // Response 2423: 01 01
static uint8_t cmd2425[PACKET_SIZE] = "\x09\x01\xb7\x00\x00"; // Response 2427: 01 01
static uint8_t cmd2429[PACKET_SIZE] = "\x09\x01\xb8\x00\x00"; // Response 2431: 01 01
static uint8_t cmd2433[PACKET_SIZE] = "\x09\x01\xba\x00\x14"; // Response 2435: 01 01 // tilt A 20 degrees
static uint8_t cmd2437[PACKET_SIZE] = "\x09\x01\xb7\x00\x14"; // Response 2439: 01 01 // tilt 7 20 degrees
static uint8_t cmd2441[PACKET_SIZE] = "\x09\x01\xb8\x00\x14"; // Response 2443: 01 01 // tilt 8 20 degrees


// These are probably getters for the angles and some kind of enable?
static uint8_t cmd2573[PACKET_SIZE] = "\x09\x06\x00\x06\x00\x00\x00\x00\x00\x00\x5b";
static uint8_t cmd2621[PACKET_SIZE] = "\x09\x02\xb8"; // Response 2623: 01 02 00 14
static uint8_t cmd2625[PACKET_SIZE] = "\x09\x02\xbd"; // Response 2627: 01 02 00 01
static uint8_t cmd2629[PACKET_SIZE] = "\x09\x02\xbe"; // Response 2631: 01 02 00 01
static uint8_t cmd2633[PACKET_SIZE] = "\x09\x02\xbb"; // Response 2635: 01 02
static uint8_t cmd2637[PACKET_SIZE] = "\x09\x02\xbc"; // Response 2635: 01 02
static uint8_t cmd2641[PACKET_SIZE] = "\x09\x02\xb9"; // Response 2643: 01 02 00 14
static uint8_t cmd2645[PACKET_SIZE] = "\x09\x02\xba"; // Response 2647: 01 02 00 14
static uint8_t cmd2649[PACKET_SIZE] = "\x09\x02\xb7"; // Response 2651: 01 02 00 14

// byte 0: 08, 09: different settings pages? lower 3 bits migth be separate from bit3 based on out[0] structure
// byte 1: 01: set; 02: get; 06: ????; 08: ????, 0d: ????
// out[0]: inp[0] == 08 => out[0] == 00; inp[0] == 09 => out[0] == 01
// out[1] == inp[1]
// out[2]: error code?

static uint8_t cmd2701[PACKET_SIZE] = "\x09\x02\xbc"; // Response 2703: 01 02
static uint8_t cmd2705[PACKET_SIZE] = "\x09\x06\x00\x06"; // Response 2707: 01 06
// also: cmd2817 (T=20.33), cmd2821 (T=20.37)
static uint8_t cmd2781[PACKET_SIZE] = "\x09\x06\x00\x06\x00\x00\x00\x00\x00\x00\x7c"; // Response: 01 06
static uint8_t cmd2785[PACKET_SIZE] = "\x09\x06\x00\x06\x00\x00\x00\x00\x00\x00\x70";
static uint8_t cmd2789[PACKET_SIZE] = "\x09\x06\x00\x06\x00\x00\x00\x00\x00\x00\x5b"; // Response 01 06
static uint8_t cmd2793[PACKET_SIZE] = "\x09\x06\x00\x06\x00\x00\x00\x00\x00\x00\x47"; // Response 01 06?
static uint8_t cmd2813[PACKET_SIZE] = "\x09\x06\x00\x06\x00\x00\x00\x00\x00\x00\xbf\x00\xff"; // T = 18.31 Response: 01 06

static uint8_t* const initSequence[] = {
//    cmd1787,
//    cmd1823,
//    cmd1827,
//    cmd1831,
//    cmd1835,
//    cmd1843,
//    cmd1863,
//    cmd1867,
// This sequence got 01 01 results from the configuration commands, but didn't succeed in getting everything through
//  cmd1871, cmd1931, cmd2421, cmd2425, cmd2429, cmd2433, cmd2437, cmd2441
//    cmd1871, cmd1875, cmd1883, cmd1931, cmd1967, cmd1971, cmd1975, cmd1987, cmd1991, cmd1995, cmd1999,
//    cmd2000, cmd2005, cmd2009, cmd2012, // some combination of these bring response from cmd2705 to 01 06
//    cmd2107, cmd2137, cmd2157, cmd2177, cmd2197,
//    cmd1871,
    cmd1931, // seems to enable things
//    cmd2005,
//    cmd2009,
//    cmd2012,
    cmd2032,
    cmd2055,
    cmd2059,
//    cmd2217,
//    cmd2253,
//    cmd2317, cmd2337, cmd2357, cmd2373,
//    cmd2417,
//    cmd2421, cmd2425, cmd2429,
//    cmd2433, cmd2437, cmd2441,
//    cmd2573,
//    cmd2701,
//    cmd2705,
//    cmd2781, cmd2785, cmd2789, cmd2793,
//    cmd2813,
    "\x09\x01\xb7\x00\x14\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
    "\x09\x01\xb8\x00\x14\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
    "\x09\x01\xb9\x00\x14\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
    "\x09\x01\xba\x00\x14\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
    "\x09\x01\xbb\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
    "\x09\x01\xbc\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
    "\x09\x01\xbd\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
    "\x09\x01\xbe\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
    NULL
};

extern uint8_t* const replaySequence[];

void nothing(int signum) { }

// FYI we're explicitly allowed to resubmit the transfer in the callback if we have to
void LIBUSB_CALL io_completion(struct libusb_transfer *transfer)
{
    int* error_code = (int*) transfer->user_data;
    if (transfer->status == LIBUSB_TRANSFER_COMPLETED) {
        (*error_code) = 0;
        return;
    }
    fprintf(stderr, "libusb_transfer status: %d\n", transfer->status);
    (*error_code) = -1;
}

// mimicks the command-response sequence seen on ep 4
int do_interrupt_transact(struct libusb_context* ctx,
        struct libusb_device_handle* dev, int ep,
        uint8_t* d_send /* PACKET_SIZE bytes */,
        uint8_t* d_recv /* PACKET_SIZE bytes */)
{
    struct libusb_transfer* out = NULL, *inp = NULL;
    int r, s;
    int out_status = LIBUSB_BOGUS_INCOMPLETE, inp_status = LIBUSB_BOGUS_INCOMPLETE;
    
    if ( (out = libusb_alloc_transfer(0)) == NULL
            || (inp = libusb_alloc_transfer(0)) == NULL) {
        fprintf(stderr, "libusb_alloc_transfer: %s\n", strerror(errno));
        r = -1;
        goto bail;
    }

    libusb_fill_interrupt_transfer(out, dev, LIBUSB_ENDPOINT_OUT | ep, d_send, PACKET_SIZE, io_completion, &out_status, 0);
    libusb_fill_interrupt_transfer(inp, dev, LIBUSB_ENDPOINT_IN  | ep, d_recv, PACKET_SIZE, io_completion, &inp_status, 0);
    if ( (r = libusb_submit_transfer(inp)) < 0) {
        fprintf(stderr, "libusb_submit_transfer(inp): %s\n", libusb_strerror(r));
        goto bail;
    }
    if ( (r = libusb_submit_transfer(out)) < 0) {
        fprintf(stderr, "libusb_submit_transfer(out): %s\n", libusb_strerror(r));
        goto bail;
    }
    while (out_status == LIBUSB_BOGUS_INCOMPLETE || inp_status == LIBUSB_BOGUS_INCOMPLETE) {
        struct timeval tv = { CORSAIR_INTERRUPT_TRANSACT_TIMEOUT_S, 0 };
        if ( (r = libusb_handle_events_timeout(ctx, &tv)) < 0) {
            fprintf(stderr, "libusb_handle_events: %s\n", libusb_strerror(r));
            goto bail;
        }
    }
    if (out_status != 0)
        fprintf(stderr, "out transfer failed %d\n", out_status);
    if (inp_status != 0)
        fprintf(stderr, "inp transfer failed %d\n", inp_status);

bail:
    if (out != NULL)
        libusb_free_transfer(out);
    if (inp != NULL)
        libusb_free_transfer(inp);
    return r;
}

int main(int argc, char *argv[])
{
    libusb_device_handle* dev_handle = NULL;
    libusb_device* dev;
    libusb_context* ctx = NULL;
    int detached = 0;
    int r, s, k;
    uint8_t* const* sequence = replaySequence;

    // ensure we can cleanup
    signal(SIGINT, nothing);
    signal(SIGTERM, nothing);

    if ( (r = libusb_init(&ctx)) < 0) {
        fprintf(stderr, "libusb_init: %s\n", libusb_strerror(r));
        return -1;
    }
    // libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG, NULL);
    if ( (dev_handle = libusb_open_device_with_vid_pid(ctx, USB_VENDOR_CORSAIR, USB_MODEL_CORSAIR_SLIPSTREAM_WIRELESS)) == NULL) {
        fprintf(stderr, "Could not open %04x:%04x: %s\n",
            USB_VENDOR_CORSAIR, USB_MODEL_CORSAIR_SLIPSTREAM_WIRELESS,
            strerror(errno));
        r = -1;
        goto bail2;
    }

    detached = libusb_has_capability(LIBUSB_CAP_SUPPORTS_DETACH_KERNEL_DRIVER);
    if (detached && (r = libusb_detach_kernel_driver(dev_handle, CORSAIR_SLIPSTREAM_CONTROL_INTERFACE)) < 0) {
        fprintf(stderr, "libusb_detach_kernel_driver: %s\n", libusb_strerror(r));
        r = 0;
        detached = 0;
    }

    if ( (r = libusb_claim_interface(dev_handle, CORSAIR_SLIPSTREAM_CONTROL_INTERFACE)) < 0) {
        fprintf(stderr, "libusb_claim_interface: %s\n", libusb_strerror(r));
        goto bail;
    }

    for (s = 0; sequence[s] != NULL; ++s) {
        uint8_t response[PACKET_SIZE] = {};
        print_hex("Command", sequence[s], PACKET_SIZE);
        if ( (r = do_interrupt_transact(ctx, dev_handle, CORSAIR_SLIPSTREAM_CONTROL_ENDPOINT, sequence[s], response)) < 0)
            goto bail;
        for (k = PACKET_SIZE; k-- > 0; ) {
            if (response[k] != 0) {
                ++k;
                break;
            }
        }
        print_hex("Response", response, PACKET_SIZE);
    }

    pause();

bail:
    if ( (s = libusb_release_interface(dev_handle, CORSAIR_SLIPSTREAM_CONTROL_INTERFACE)) < 0)
        fprintf(stderr, "libusb_release_interface: %s\n", libusb_strerror(s));
    if (detached && (s = libusb_attach_kernel_driver(dev_handle, CORSAIR_SLIPSTREAM_CONTROL_INTERFACE)) < 0)
        fprintf(stderr, "libusb_attach_kernel_driver: %s\n", libusb_strerror(s));
    libusb_close(dev_handle);
bail2:
    libusb_exit(ctx);
    return r;
}