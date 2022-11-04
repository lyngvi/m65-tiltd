#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include "stuff.h"

#include <libusb-1.0/libusb.h>

#define CORSAIR_KICK_THE_MOUSE_S (24) // for some reason, iCue does this, so whatever, we can too
#define CORSAIR_COMMAND_SUCCESSFUL_1 "\x01\x01"

static int noisy = 0;

static uint8_t cmd1931[PACKET_SIZE] = "\x09\x01\x03\x00\x02"; // Response 1933: 01 01. This appears to be required to stop getting error codes from attempts to use below.
static uint8_t cmd_set_angle_0[PACKET_SIZE] = "\x09\x01\xb7\x00\x14"; // tilt angles - 0x14 actually maps to 20 degrees, believe it or not
static uint8_t cmd_set_angle_1[PACKET_SIZE] = "\x09\x01\xb8\x00\x14";
static uint8_t cmd_set_angle_2[PACKET_SIZE] = "\x09\x01\xb9\x00\x14";
static uint8_t cmd_set_angle_3[PACKET_SIZE] = "\x09\x01\xba\x00\x14";
static uint8_t cmd_enable_0[PACKET_SIZE]    = "\x09\x01\xbb\x00\x01"; // guessing - it's probably an enable
static uint8_t cmd_enable_1[PACKET_SIZE]    = "\x09\x01\xbc\x00\x01";
static uint8_t cmd_enable_2[PACKET_SIZE]    = "\x09\x01\xbd\x00\x01";
static uint8_t cmd_enable_3[PACKET_SIZE]    = "\x09\x01\xbe\x00\x01";

struct command_response_pair {
    int command_size; // total command size
    /* const */ uint8_t* command;
    int response_size; // minimum response size - we only check to this value
    const uint8_t* expected_response;
};

static const struct command_response_pair initSequence[] = {
    { sizeof(cmd1931),         cmd1931,         2, CORSAIR_COMMAND_SUCCESSFUL_1 }, // seems to enable things
    { sizeof(cmd_set_angle_0), cmd_set_angle_0, 2, CORSAIR_COMMAND_SUCCESSFUL_1 },
    { sizeof(cmd_set_angle_1), cmd_set_angle_1, 2, CORSAIR_COMMAND_SUCCESSFUL_1 },
    { sizeof(cmd_set_angle_2), cmd_set_angle_2, 2, CORSAIR_COMMAND_SUCCESSFUL_1 },
    { sizeof(cmd_set_angle_3), cmd_set_angle_3, 2, CORSAIR_COMMAND_SUCCESSFUL_1 },
    { sizeof(cmd_enable_0),    cmd_enable_0,    2, CORSAIR_COMMAND_SUCCESSFUL_1 },
    { sizeof(cmd_enable_1),    cmd_enable_1,    2, CORSAIR_COMMAND_SUCCESSFUL_1 },
    { sizeof(cmd_enable_2),    cmd_enable_2,    2, CORSAIR_COMMAND_SUCCESSFUL_1 },
    { sizeof(cmd_enable_3),    cmd_enable_3,    2, CORSAIR_COMMAND_SUCCESSFUL_1 },
    { 0, NULL, 0, NULL }
};

static uint8_t cmd_maintain_2[PACKET_SIZE] = "\x09\x12";
static const struct command_response_pair maintenanceSequence[] = {
    { sizeof(cmd_maintain_2), cmd_maintain_2, 2, "\x01\x12" },
    { 0, NULL, 0, NULL },
};

static int g_Term = 0;

void honorable_discharge(int signum) { g_Term = 1; }

// FYI we're explicitly allowed to resubmit the transfer in the callback if we have to
void LIBUSB_CALL io_completion(struct libusb_transfer *transfer)
{
    int* sent = (int*) transfer->user_data;
    if (transfer->status == LIBUSB_TRANSFER_COMPLETED) {
        (*sent) = transfer->actual_length;
        return;
    }
    fprintf(stderr, "libusb_transfer status: %d\n", transfer->status);
    (*sent) = LIBUSB_ERROR_IO; /* I guess, sure */
}

// mimicks the command-response sequence seen on ep 4
int do_interrupt_transact(struct libusb_context* ctx,
        struct libusb_device_handle* dev, int ep,
        uint8_t* d_send, int send_bytes,
        uint8_t* d_recv, int* recv_bytes)
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

    libusb_fill_interrupt_transfer(out, dev, LIBUSB_ENDPOINT_OUT | ep, d_send,  send_bytes, io_completion, &out_status, 0);
    libusb_fill_interrupt_transfer(inp, dev, LIBUSB_ENDPOINT_IN  | ep, d_recv, *recv_bytes, io_completion, &inp_status, 0);
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
    if (out_status < 0)
        fprintf(stderr, "out transfer failed %d\n", out_status);
    if (inp_status < 0)
        fprintf(stderr, "inp transfer failed %d\n", inp_status);

    (*recv_bytes) = inp_status;

bail:
    if (out != NULL)
        libusb_free_transfer(out);
    if (inp != NULL)
        libusb_free_transfer(inp);
    return r;
}

int execute_sequence(libusb_context* ctx, libusb_device_handle* m65ctrl, const struct command_response_pair* sequence)
{
    int r, s, k;
    for (s = 0; sequence[s].command != NULL; ++s) {
        uint8_t response[PACKET_SIZE] = {};
        int responseSize = sizeof(response);
        if (noisy)
            print_hex("Command", sequence[s].command, sequence[s].command_size);
        if ( (r = do_interrupt_transact(ctx, m65ctrl, CORSAIR_SLIPSTREAM_CONTROL_ENDPOINT,
                sequence[s].command, sequence[s].command_size, response, &responseSize)) < 0)
            return r;
        if (noisy)
            print_hex("Response", response, responseSize);
        // We'll just treat these as normal conditions - sloppy, but I don't have set expectations yet
        int pass = (responseSize >= sequence[s].response_size) && memcmp(response, sequence[s].expected_response, sequence[s].response_size) == 0;
        if (!pass) {
            printf("Response mismatch\n");
            print_hex("command", sequence[s].command, sequence[s].command_size);
            print_hex("wanted", sequence[s].expected_response, sequence[s].response_size);
            print_hex("got", response, responseSize);
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    libusb_device_handle* dev_handle = NULL;
    libusb_context* ctx = NULL;
    int detached = 0;
    int r, s, k;

    // ensure we can cleanup
    signal(SIGINT, honorable_discharge);
    signal(SIGTERM, honorable_discharge);

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

    detached = libusb_has_capability(LIBUSB_CAP_SUPPORTS_DETACH_KERNEL_DRIVER) &&
        libusb_kernel_driver_active(dev_handle, CORSAIR_SLIPSTREAM_CONTROL_INTERFACE);

    if (detached && (r = libusb_detach_kernel_driver(dev_handle, CORSAIR_SLIPSTREAM_CONTROL_INTERFACE)) < 0) {
        fprintf(stderr, "libusb_detach_kernel_driver: %s\n", libusb_strerror(r));
        r = 0;
        detached = 0;
    }

    if ( (r = libusb_claim_interface(dev_handle, CORSAIR_SLIPSTREAM_CONTROL_INTERFACE)) < 0) {
        fprintf(stderr, "libusb_claim_interface: %s\n", libusb_strerror(r));
        goto bail;
    }

    if ( (r = execute_sequence(ctx, dev_handle, initSequence)) < 0)
        goto bail;

    while (!g_Term && (r = execute_sequence(ctx, dev_handle, maintenanceSequence)) == 0)
        sleep(CORSAIR_KICK_THE_MOUSE_S); // signal will just set g_Term so error ignored

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