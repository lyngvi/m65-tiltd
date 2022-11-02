#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include "stuff.h"

#include <libusb-1.0/libusb.h>

int g_term = 0;

// FYI we're explicitly allowed to resubmit the transfer in the callback if we have to
void LIBUSB_CALL io_completion(struct libusb_transfer *transfer)
{
    if (transfer->status == LIBUSB_TRANSFER_COMPLETED) {
        print_hex("received: ", transfer->buffer, transfer->actual_length);
        libusb_submit_transfer(transfer);
        return;
    }
    g_term = 1;
    fprintf(stderr, "libusb_transfer status: %d\n", transfer->status);
}

int do_monitor(struct libusb_context* ctx, struct libusb_device_handle* dev, int ep)
{
    struct libusb_transfer* inp = NULL;
    int r, s;
    int inp_status = LIBUSB_BOGUS_INCOMPLETE;
    uint8_t recv_buffer[PACKET_SIZE];
    
    if ((inp = libusb_alloc_transfer(0)) == NULL) {
        fprintf(stderr, "libusb_alloc_transfer: %s\n", strerror(errno));
        r = -1;
        goto bail;
    }

    libusb_fill_interrupt_transfer(inp, dev,
        LIBUSB_ENDPOINT_IN | ep, recv_buffer, sizeof(recv_buffer),
        io_completion, NULL, 0);


    if ( (r = libusb_submit_transfer(inp)) < 0) {
        fprintf(stderr, "libusb_submit_transfer: %s\n", libusb_strerror(r));
        goto bail;
    }
    while (!g_term) {
        struct timeval tv = { 0, 0 };
        if ( (r = libusb_handle_events_timeout(ctx, &tv)) < 0) {
            fprintf(stderr, "libusb_handle_events: %s\n", libusb_strerror(r));
            break;
        }
    }
bail:
    if (inp != NULL)
        libusb_free_transfer(inp);
    return 0;
}

void do_term(int signum)
{
    g_term = 1;
}

int main(int argc, char *argv[])
{
    libusb_device_handle* dev_handle = NULL;
    libusb_device* dev;
    libusb_context* ctx = NULL;
    int detached = 0, r, s;

    // ensure we can cleanup
    signal(SIGINT, do_term);
    signal(SIGTERM, do_term);

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
    if (detached && (r = libusb_detach_kernel_driver(dev_handle, CORSAIR_SLIPSTREAM_TILT_INTERFACE)) < 0) {
        fprintf(stderr, "libusb_detach_kernel_driver: %s\n", libusb_strerror(r));
        r = 0;
        detached = 0;
    }

    if ( (r = libusb_claim_interface(dev_handle, CORSAIR_SLIPSTREAM_TILT_INTERFACE)) < 0) {
        fprintf(stderr, "libusb_claim_interface: %s\n", libusb_strerror(r));
        goto bail;
    }

    do_monitor(ctx, dev_handle, CORSAIR_SLIPSTREAM_TILT_ENDPOINT);

bail:
    if ( (s = libusb_release_interface(dev_handle, CORSAIR_SLIPSTREAM_TILT_INTERFACE)) < 0)
        fprintf(stderr, "libusb_release_interface: %s\n", libusb_strerror(s));
    if (detached && (s = libusb_attach_kernel_driver(dev_handle, CORSAIR_SLIPSTREAM_TILT_INTERFACE)) < 0)
        fprintf(stderr, "libusb_attach_kernel_driver: %s\n", libusb_strerror(s));
    libusb_close(dev_handle);
bail2:
    libusb_exit(ctx);
    return r;
}