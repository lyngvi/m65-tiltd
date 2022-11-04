#include <linux/uinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include "stuff.h"

#include <libusb-1.0/libusb.h>

#define CORSAIR_KICK_THE_MOUSE_S (24) // for some reason, iCue does this, so whatever, we can too
#define CORSAIR_COMMAND_SUCCESSFUL_1 "\x01\x01"

static int g_Noisy = 0;
static int g_Term = 0;

enum ControlIndex {
    CITiltDown,
    CITiltUp,
    CITiltLeft,
    CITiltRight,
    CIDPIDown,
    CIDPIUp,
    CIMetaPlaceholder
};

enum ButtonIndex {
    BILeft,
    BIRight,
    BICenter,
    BIForward,
    BIBack,
    BIDPIUp,
    BIDPIDown,
};

// keycodes to ControlIndex
static int g_TiltMap[] = {
    KEY_KP2,
    KEY_KP8,
    KEY_KP4,
    KEY_KP6,
    KEY_KP1,
    KEY_KP7,
    KEY_LEFTMETA
};

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

static uint8_t cmd_maintain_1[PACKET_SIZE] = "\x08\x12";
static uint8_t cmd_maintain_2[PACKET_SIZE] = "\x09\x12";
static const struct command_response_pair maintenanceSequence[] = {
    { sizeof(cmd_maintain_1), cmd_maintain_1, 4, "\x00\x12\x00\x04" },
    { sizeof(cmd_maintain_2), cmd_maintain_2, 2, "\x01\x12" },
    { 0, NULL, 0, NULL },
};

static void honorable_discharge(int signum) { g_Term = 1; }

// FYI we're explicitly allowed to resubmit the transfer in the callback if we have to
static void LIBUSB_CALL interrupt_transact_completion(struct libusb_transfer *transfer)
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
static int do_interrupt_transact(struct libusb_context* ctx,
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

    libusb_fill_interrupt_transfer(out, dev, LIBUSB_ENDPOINT_OUT | ep, d_send,  send_bytes, interrupt_transact_completion, &out_status, 0);
    libusb_fill_interrupt_transfer(inp, dev, LIBUSB_ENDPOINT_IN  | ep, d_recv, *recv_bytes, interrupt_transact_completion, &inp_status, 0);
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

static int execute_sequence(libusb_context* ctx, libusb_device_handle* m65ctrl, const struct command_response_pair* sequence)
{
    int r, s, k;
    for (s = 0; sequence[s].command != NULL; ++s) {
        uint8_t response[PACKET_SIZE] = {};
        int responseSize = sizeof(response);
        if (g_Noisy)
            print_hex("Command", sequence[s].command, sequence[s].command_size);
        if ( (r = do_interrupt_transact(ctx, m65ctrl, CORSAIR_SLIPSTREAM_CONTROL_ENDPOINT,
                sequence[s].command, sequence[s].command_size, response, &responseSize)) < 0)
            return r;
        if (g_Noisy)
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

static int emit_uinp(int fd, int type, int code, int val)
{
    struct input_event ie = {};
    ie.type = type;
    ie.code = code;
    ie.value = val;
    return write(fd, &ie, sizeof(ie));
}

static void handle_tilt(int uinpfd, int index)
{
    emit_uinp(uinpfd, EV_KEY, g_TiltMap[CIMetaPlaceholder], 1);
    emit_uinp(uinpfd, EV_KEY, g_TiltMap[index], 1);
    emit_uinp(uinpfd, EV_SYN, SYN_REPORT, 0);
    emit_uinp(uinpfd, EV_KEY, g_TiltMap[index], 0);
    emit_uinp(uinpfd, EV_KEY, g_TiltMap[CIMetaPlaceholder], 0);
    emit_uinp(uinpfd, EV_SYN, SYN_REPORT, 0);
    if (g_Noisy)
        printf("Tilt! %d\n", index);
}

// FYI we're explicitly allowed to resubmit the transfer in the callback if we have to
static void LIBUSB_CALL tilt_io_completion(struct libusb_transfer *transfer)
{
    if (transfer->status == LIBUSB_TRANSFER_COMPLETED) {
        int r;
        if (g_Noisy)
            print_hex("received: ", transfer->buffer, transfer->actual_length);
        int uinpfd = (int) (intptr_t) transfer->user_data;
        if (transfer->actual_length >= 5 && memcmp(transfer->buffer, "\x01\x09\x00\x01", 4) == 0) {
            int tilt_index = CITiltDown + transfer->buffer[4];
            handle_tilt(uinpfd, tilt_index);
        } else if (transfer->actual_length >= 3 && memcmp(transfer->buffer, "\x01\x02", 2) == 0) {
            int button_mask = transfer->buffer[2];
            if (button_mask & (1 << BIDPIUp))
                handle_tilt(uinpfd, CIDPIUp);
            if (button_mask & (1 << BIDPIDown))
                handle_tilt(uinpfd, CIDPIDown);
        }
        if ( (r = libusb_submit_transfer(transfer)) < 0) {
            fprintf(stderr, "libusb_submit_transfer: %s\n", libusb_strerror(r));
            g_Term = 2;
        }
        return;
    }
    if (transfer->status != LIBUSB_TRANSFER_CANCELLED)
        fprintf(stderr, "libusb_transfer status: %d\n", transfer->status);
    g_Term = 2;
    libusb_free_transfer(transfer);
}

int run_tilt(libusb_context* ctx, libusb_device_handle* m65, int uinpfd)
{
    int r = 0, s;
    struct libusb_transfer* inp = NULL;
    int inp_status = LIBUSB_BOGUS_INCOMPLETE;
    uint8_t recv_buffer[PACKET_SIZE];
    struct timeval t0;

    if ((inp = libusb_alloc_transfer(0)) == NULL) {
        fprintf(stderr, "libusb_alloc_transfer: %s\n", strerror(errno));
        return -1;
    }

    // We actually get a lot of mouse traffic - button clicks, etc - but we're only concerned
    // with the tilt patterns.
    libusb_fill_interrupt_transfer(inp, m65,
        LIBUSB_ENDPOINT_IN | CORSAIR_SLIPSTREAM_TILT_ENDPOINT, recv_buffer, sizeof(recv_buffer),
        tilt_io_completion, NULL, 0);

    inp->user_data = (void*) (intptr_t) uinpfd;
    if ( (r = libusb_submit_transfer(inp)) < 0) {
        fprintf(stderr, "libusb_submit_transfer: %s\n", libusb_strerror(r));
        return r;
    }

    if ( (r = execute_sequence(ctx, m65, initSequence)) < 0)
        goto bail;

    gettimeofday(&t0, NULL);
    while (!g_Term) {
        struct timeval timeout;
        time_until(&timeout, &t0);
        if (timeout.tv_sec == 0 && timeout.tv_usec == 0) {
            if ( (r = execute_sequence(ctx, m65, maintenanceSequence)) < 0)
                break;
            t0.tv_sec += CORSAIR_KICK_THE_MOUSE_S;
            continue;
        }
        if ( (r = libusb_handle_events_timeout_completed(ctx, &timeout, NULL)) < 0) {
            fprintf(stderr, "libusb_handle_events: %s\n", libusb_strerror(r));
            break;
        }
    }

bail:
    if (inp != NULL && (s = libusb_cancel_transfer(inp)) < 0) {
        fprintf(stderr, "libusb_cancel_transfer: %s\n", libusb_strerror(s));
        if (r == 0)
            r = s;
    }
    // should be freed by the callback
    return r;
}

int main(int argc, char *argv[])
{
    libusb_device_handle* m65 = NULL;
    libusb_context* ctx = NULL;
    int control_detached = 0, tilt_detached = 0;
    int r, s, k;
    int uinpfd = -1;

    // ensure we can cleanup
    signal(SIGINT, honorable_discharge);
    signal(SIGTERM, honorable_discharge);

    if ( (uinpfd = open("/dev/uinput", O_WRONLY)) == -1) {
        fprintf(stderr, "could not open /dev/uinput: %s\n", strerror(errno));
        return -1;
    }
    if (ioctl(uinpfd, UI_SET_EVBIT, EV_KEY) < 0) {
        fprintf(stderr, "ioctl(UI_SET_EVBIT): %s\n", strerror(errno));
        r = -1;
        goto bail;
    }
    {
        // We have to declare our supported keys
        for (k = 0; k < sizeof(g_TiltMap) / sizeof(g_TiltMap[0]); k++) {
            if (ioctl(uinpfd, UI_SET_KEYBIT, g_TiltMap[k]) < 0) {
                fprintf(stderr, "ioctl(UI_SET_KEYBIT %d): %s\n", g_TiltMap[k], strerror(errno));
                r = -1;
                goto bail;
            }
        }
    }

    {
        struct uinput_setup usetup = {};
        usetup.id.bustype = BUS_USB; // this is taken straight from kernel docs
        usetup.id.vendor = 0x1234; /* sample vendor */
        usetup.id.product = 0x5678;
        snprintf(usetup.name, sizeof(usetup.name), "m65-tilt-keypress");
        if (ioctl(uinpfd, UI_DEV_SETUP, &usetup) < 0) {
            fprintf(stderr, "ioctl(UI_DEV_SETUP): %s\n", strerror(errno));
            r = -1;
            goto bail;
        }
        if (ioctl(uinpfd, UI_DEV_CREATE) < 0) {
            fprintf(stderr, "ioctl(UI_DEV_CREATE): %s\n", strerror(errno));
            r = -1;
            goto bail;
        }
    }

    if ( (r = libusb_init(&ctx)) < 0) {
        fprintf(stderr, "libusb_init: %s\n", libusb_strerror(r));
        goto bail;
    }

    // libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG, NULL);
    if ( (m65 = libusb_open_device_with_vid_pid(ctx, USB_VENDOR_CORSAIR, USB_MODEL_CORSAIR_SLIPSTREAM_WIRELESS)) == NULL) {
        fprintf(stderr, "Could not open %04x:%04x: %s\n",
            USB_VENDOR_CORSAIR, USB_MODEL_CORSAIR_SLIPSTREAM_WIRELESS,
            strerror(errno));
        r = -1;
        goto bail;
    }

    if (libusb_has_capability(LIBUSB_CAP_SUPPORTS_DETACH_KERNEL_DRIVER)) {
        control_detached = libusb_kernel_driver_active(m65, CORSAIR_SLIPSTREAM_CONTROL_INTERFACE);
        if (control_detached && (r = libusb_detach_kernel_driver(m65, CORSAIR_SLIPSTREAM_CONTROL_INTERFACE)) < 0) {
            fprintf(stderr, "libusb_detach_kernel_driver: %s\n", libusb_strerror(r));
            r = 0;
            control_detached = 0;
        }

        tilt_detached = libusb_kernel_driver_active(m65, CORSAIR_SLIPSTREAM_TILT_INTERFACE);
        if (tilt_detached && (r = libusb_detach_kernel_driver(m65, CORSAIR_SLIPSTREAM_TILT_INTERFACE)) < 0) {
            fprintf(stderr, "libusb_detach_kernel_driver: %s\n", libusb_strerror(r));
            r = 0;
            tilt_detached = 0;
        }
    }

    if ( (r = libusb_claim_interface(m65, CORSAIR_SLIPSTREAM_CONTROL_INTERFACE)) < 0) {
        fprintf(stderr, "libusb_claim_interface(control): %s\n", libusb_strerror(r));
        goto bail;
    }

    if ( (r = libusb_claim_interface(m65, CORSAIR_SLIPSTREAM_TILT_INTERFACE)) < 0) {
        fprintf(stderr, "libusb_claim_interface(tilt): %s\n", libusb_strerror(r));
        goto bail;
    }

    r = run_tilt(ctx, m65, uinpfd);

bail:
    if (m65 != NULL) {
        if ( (s = libusb_release_interface(m65, CORSAIR_SLIPSTREAM_CONTROL_INTERFACE)) < 0)
            fprintf(stderr, "libusb_release_interface(control): %s\n", libusb_strerror(s));
        if (control_detached && (s = libusb_attach_kernel_driver(m65, CORSAIR_SLIPSTREAM_CONTROL_INTERFACE)) < 0)
            fprintf(stderr, "libusb_attach_kernel_driver(control): %s\n", libusb_strerror(s));

        if ( (s = libusb_release_interface(m65, CORSAIR_SLIPSTREAM_TILT_INTERFACE)) < 0)
            fprintf(stderr, "libusb_release_interface(tilt): %s\n", libusb_strerror(s));
        if (tilt_detached && (s = libusb_attach_kernel_driver(m65, CORSAIR_SLIPSTREAM_TILT_INTERFACE)) < 0)
            fprintf(stderr, "libusb_attach_kernel_driver(tilt): %s\n", libusb_strerror(s));

        libusb_close(m65);
        m65 = NULL;
    }

    if (uinpfd != -1) {
        ioctl(uinpfd, UI_DEV_DESTROY);
        close(uinpfd);
        uinpfd = -1;

    }

    libusb_exit(ctx);
    return (r != 0) ? r :
        (g_Term != 1) ? -1 :
        0;
}