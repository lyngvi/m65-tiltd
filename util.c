#include <stdio.h>
#include <stdint.h>

void print_hex(const char* label, const uint8_t* data, int sz)
{
    int k;
    while (sz-- > 0) {
        if (data[sz]) {
            sz++;
            break;
        }
    }
    fprintf(stdout, "%10s:", label);
    for (k = 0; k < sz; ) {
        fprintf(stdout, " %02x", data[k++]);
        if ((k % 16) == 0)
            fprintf(stdout, "\n");
    }
    if ((sz % 16) != 0)
        fprintf(stdout, "\n");
}