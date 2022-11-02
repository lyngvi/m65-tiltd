#include <stdint.h>
#include <stdlib.h>
#include "stuff.h"
static uint8_t cmd01787[PACKET_SIZE] = "\x08\x01\x03\x00\x02";
static uint8_t cmd01823[PACKET_SIZE] = "\x08\x0d\x00\x05";
static uint8_t cmd01827[PACKET_SIZE] = "\x08\x09";
static uint8_t cmd01831[PACKET_SIZE] = "\x08\x08";
static uint8_t cmd01835[PACKET_SIZE] = "\x08\x05\x01";
static uint8_t cmd01843[PACKET_SIZE] = "\x08\x0d\x00\x27";
static uint8_t cmd01863[PACKET_SIZE] = "\x08\x0d\x00\x05";
static uint8_t cmd01867[PACKET_SIZE] = "\x08\x09";
static uint8_t cmd01871[PACKET_SIZE] = "\x08\x08";
static uint8_t cmd01875[PACKET_SIZE] = "\x08\x05\x01";
static uint8_t cmd01879[PACKET_SIZE] = "\x08\x0d\x00\x24";
static uint8_t cmd01883[PACKET_SIZE] = "\x08\x09";
static uint8_t cmd01887[PACKET_SIZE] = "\x08\x08";
static uint8_t cmd01891[PACKET_SIZE] = "\x08\x05\x01";
static uint8_t cmd01931[PACKET_SIZE] = "\x09\x01\x03\x00\x02";
static uint8_t cmd01967[PACKET_SIZE] = "\x09\x0d\x00\x05";
static uint8_t cmd01971[PACKET_SIZE] = "\x09\x09";
static uint8_t cmd01975[PACKET_SIZE] = "\x09\x08";
static uint8_t cmd01979[PACKET_SIZE] = "\x09\x05\x01";
static uint8_t cmd01987[PACKET_SIZE] = "\x09\x0d\x00\x27";
static uint8_t cmd01991[PACKET_SIZE] = "\x09\x05\x01";
static uint8_t cmd01995[PACKET_SIZE] = "\x09\x0d\x00\x27";
static uint8_t cmd01999[PACKET_SIZE] = "\x08\x0d\x00\x24";
static uint8_t cmd02000[PACKET_SIZE] = "\x09\x0d\x00\x05";
static uint8_t cmd02005[PACKET_SIZE] = "\x08\x09";
static uint8_t cmd02009[PACKET_SIZE] = "\x09\x09";
static uint8_t cmd02012[PACKET_SIZE] = "\x08\x08";
static uint8_t cmd02017[PACKET_SIZE] = "\x08\x05\x01";
static uint8_t cmd02020[PACKET_SIZE] = "\x09\x08";
static uint8_t cmd02032[PACKET_SIZE] = "\x09\x05\x01";
static uint8_t cmd02055[PACKET_SIZE] = "\x09\x0d\x00\x02";
static uint8_t cmd02059[PACKET_SIZE] = "\x09\x06\x00\x08\x00\x00\x00\x01\x01\x01\x00\x00\x00\x00\x01";
static uint8_t cmd02063[PACKET_SIZE] = "\x09\x05\x01";
static uint8_t cmd02107[PACKET_SIZE] = "\x09\x01\xa8";
static uint8_t cmd02137[PACKET_SIZE] = "\x09\x01\xbd\x00\x01";
static uint8_t cmd02157[PACKET_SIZE] = "\x09\x01\xbe\x00\x01";
static uint8_t cmd02177[PACKET_SIZE] = "\x09\x01\xbb";
static uint8_t cmd02197[PACKET_SIZE] = "\x09\x01\xbc";
static uint8_t cmd02217[PACKET_SIZE] = "\x09\x01\x07";
static uint8_t cmd02229[PACKET_SIZE] = "\x09\x0d\x00\x01";
static uint8_t cmd02237[PACKET_SIZE] = "\x09\x0d\x01\x02";
static uint8_t cmd02241[PACKET_SIZE] = "\x09\x06\x01\x08\x00\x00\x00\x01\x01\x01\x00\x00\x00\x00\x01";
static uint8_t cmd02245[PACKET_SIZE] = "\x09\x05\x01\x01";
static uint8_t cmd02253[PACKET_SIZE] = "\x09\x01\xa8";
static uint8_t cmd02277[PACKET_SIZE] = "\x09\x01\xbd\x00\x01";
static uint8_t cmd02297[PACKET_SIZE] = "\x09\x01\xbe\x00\x01";
static uint8_t cmd02317[PACKET_SIZE] = "\x09\x01\xbb";
static uint8_t cmd02337[PACKET_SIZE] = "\x09\x01\xbc";
static uint8_t cmd02357[PACKET_SIZE] = "\x09\x01\x07";
static uint8_t cmd02361[PACKET_SIZE] = "\x09\x01\x21\x00\xb8\x0b";
static uint8_t cmd02365[PACKET_SIZE] = "\x09\x01\x22\x00\xb8\x0b";
static uint8_t cmd02373[PACKET_SIZE] = "\x09\x01\x02\x00\xf4\x01";
static uint8_t cmd02421[PACKET_SIZE] = "\x09\x01\xba";
static uint8_t cmd02425[PACKET_SIZE] = "\x09\x01\xb7";
static uint8_t cmd02429[PACKET_SIZE] = "\x09\x01\xb8";
static uint8_t cmd02433[PACKET_SIZE] = "\x09\x01\xba\x00\x14";
static uint8_t cmd02437[PACKET_SIZE] = "\x09\x01\xb7\x00\x14";
static uint8_t cmd02441[PACKET_SIZE] = "\x09\x01\xb8\x00\x14";
static uint8_t cmd02441_a[PACKET_SIZE] = "\x09\x01\xb9\x00\x14";
static uint8_t cmd06519[PACKET_SIZE] = "\x08\x12";
static uint8_t cmd06563[PACKET_SIZE] = "\x09\x12";
static uint8_t cmd06857[PACKET_SIZE] = "\x08\x12";
static uint8_t cmd06861[PACKET_SIZE] = "\x09\x12";
static uint8_t cmd08889[PACKET_SIZE] = "\x08\x12";
static uint8_t cmd08893[PACKET_SIZE] = "\x09\x12";
static uint8_t cmd10459[PACKET_SIZE] = "\x08\x12";
static uint8_t cmd10463[PACKET_SIZE] = "\x09\x12";
static uint8_t cmd11017[PACKET_SIZE] = "\x09\x01\xbd";
static uint8_t cmd11039[PACKET_SIZE] = "\x09\x01\xbe";
static uint8_t cmd11059[PACKET_SIZE] = "\x09\x01\xbb";
static uint8_t cmd11083[PACKET_SIZE] = "\x09\x01\xbc";
static uint8_t cmd11107[PACKET_SIZE] = "\x09\x01\xa8\x00\x01";
static uint8_t cmd12337[PACKET_SIZE] = "\x09\x01\xbd\x00\x01";
static uint8_t cmd12361[PACKET_SIZE] = "\x09\x01\xbe\x00\x01";
static uint8_t cmd12337_a[PACKET_SIZE] = "\x09\x01\xbc\x00\x01";
static uint8_t cmd12361_a[PACKET_SIZE] = "\x09\x01\xbb\x00\x01";
static uint8_t cmd12381[PACKET_SIZE] = "\x09\x01\xa8";
uint8_t* const replaySequence[] = {
    cmd01787,
    cmd01823,
    cmd01827,
    cmd01831,
    cmd01835,
    cmd01843,
    cmd01863,
    cmd01867,
    cmd01871,
    cmd01875,
    cmd01879,
    cmd01883,
    cmd01887,
    cmd01891,
    cmd01931,
    cmd01967,
    cmd01971,
    cmd01975,
    cmd01979,
    cmd01987,
    cmd01991,
    cmd01995,
    cmd01999,
    cmd02000,
    cmd02005,
    cmd02009,
    cmd02012,
    cmd02017,
    cmd02020,
    cmd02032,
    cmd02055,
    cmd02059,
    cmd02063,
    cmd02107,
    cmd02137,
    cmd02157,
    cmd02177,
    cmd02197,
    cmd02217,
    cmd02229,
    cmd02237,
    cmd02241,
    cmd02245,
    cmd02253,
    cmd02277,
    cmd02297,
    cmd02317,
    cmd02337,
    cmd02357,
    cmd02361,
    cmd02365,
    cmd02373,
    cmd02421,
    cmd02425,
    cmd02429,
    cmd02433,
    cmd02437,
    cmd02441,
    cmd02441_a,
    cmd06519,
    cmd06563,
    cmd06857,
    cmd06861,
    cmd08889,
    cmd08893,
    cmd10459,
    cmd10463,
    cmd11017,
    cmd11039,
    cmd11059,
    cmd11083,
    cmd11107,
    cmd12337,
    cmd12337_a,
    cmd12361,
    cmd12361_a,
    cmd12381,
    NULL
};
