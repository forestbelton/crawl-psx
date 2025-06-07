/*
 * PSn00bSDK controller polling example (SPI driver)
 * (C) 2021 spicyjpeg - MPL licensed
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <psxetc.h>
#include <psxapi.h>
#include <psxpad.h>
#include <hwregs_c.h>

#include "psx/input.h"

/* Internal structures and globals */

typedef struct {
    uint8_t tx_buff[SPI_BUFF_LEN];
    uint8_t rx_buff[SPI_BUFF_LEN];
    uint32_t tx_len, rx_len, port;
    SPI_Callback callback;
} SPI_Context;

static volatile SPI_Context _context;
static volatile SPI_Request *_current_req;
static volatile SPI_Callback _default_cb;

static volatile uint8_t pad_buff[2][34];
static volatile size_t pad_buff_len[2];
static volatile uint32_t pad_config_attempt[2] = {0, 0};

/* Request queue management */

static void _spi_create_poll_req() {
    const auto req = reinterpret_cast<volatile PadRequest *>(_context.tx_buff);

    req->addr = 0x01;
    req->cmd = PAD_CMD_READ;
    req->tap_mode = 0x00; // 0x01 to enable extended multitap response
    req->motor_l = 0x00;
    req->motor_r = 0x00;

    _context.tx_len = 4;
    _context.rx_len = 0;
    _context.port ^= 1;
    _context.callback = _default_cb;
}

static void _spi_next_req() {
    // Copy the contents of the first request in the queue into the TX buffer.
    memcpy(
        const_cast<uint8_t *>(_context.tx_buff),
        const_cast<uint8_t *>(_current_req->data),
        _current_req->len
    );

    _context.tx_len = _current_req->len;
    _context.rx_len = 0;
    _context.port = _current_req->port;
    _context.callback = _current_req->callback;

    // Pop the first request from the queue by deallocating it and adjusting
    // the pointer to the first queue item.
    SPI_Request *next = _current_req->next;

    delete _current_req;
    _current_req = next;
}

/* Interrupt handlers */

static void _spi_poll_irq_handler() {
    // Fetch the last response byte, which wasn't followed by a pulse on /ACK,
    // from the RX FIFO.
    if (SIO_STAT(0) & 0x0002)
        _context.rx_buff[_context.rx_len - 1] = static_cast<uint8_t>(SIO_DATA(0));

    if (_context.callback)
        _context.callback(_context.port, _context.rx_buff, _context.rx_len);

    // If the request queue is empty, create a pad polling request.
    if (_current_req)
        _spi_next_req();
    else
        _spi_create_poll_req();

    // Prepare the SPI port by clearing any pending IRQ, pulling /CS high and
    // enabling the /ACK IRQ. In order to communicate with controllers, /CS has
    // to be driven low again for about 20 us before sending the first byte.
    // TODO: these delays can be probably tweaked for better performance
    SIO_CTRL(0) = 0x0010;
    for (uint32_t i = 0; i < 1000; i++)
        __asm__ volatile("");

    SIO_CTRL(0) = 0x1003 | (_context.port << 13);
    for (uint32_t i = 0; i < 2000; i++)
        __asm__ volatile("");

    // Send the first byte indicating which device to address. If the matching
    // device is connected, it will reply by triggering the /ACK IRQ.
    SIO_DATA(0) = _context.tx_buff[0];
}

static void _spi_ack_irq_handler() {
    // Wait until /ACK is pulled up by the controller before sending the next
    // byte. According to nocash docs, this has to be done before resetting the
    // IRQ.
    while (SIO_STAT(0) & 0x0080)
        __asm__ volatile("");

    // Keep /CS pulled low and acknowledge the IRQ (bit 4) to ensure it can be
    // triggered again.
    SIO_CTRL(0) = 0x1013 | (_context.port << 13);

    if (!_context.rx_len) {
        // We just sent the first address byte. Obviously the response we
        // received was read from an open bus, so the SPI port's internal FIFO
        // must be flushed (by performing dummy reads) to ensure we are only
        // going to read valid data from now on.
        SIO_DATA(0);
    } else if (_context.rx_len <= SPI_BUFF_LEN) {
        // If this is not the first byte, put it in the RX buffer.
        _context.rx_buff[_context.rx_len - 1] = (uint8_t) SIO_DATA(0);
    }

    // Send the next byte, or a null byte if there is no more data to send and
    // we're just reading a response.
    _context.rx_len++;
    if (_context.rx_len < _context.tx_len)
        SIO_DATA(0) = (uint32_t) _context.tx_buff[_context.rx_len];
    else
        SIO_DATA(0) = 0x00;
}

/* Public API */

SPI_Request *SPI_CreateRequest() {
    const auto req = new SPI_Request;

    req->len = 0;
    req->port = 0;
    req->callback = nullptr;
    req->next = nullptr;

    // Find the last queued request by traversing the linked list and append a
    // pointer to the new request.
    if (!_current_req) {
        _current_req = req;
    } else {
        volatile SPI_Request *volatile last = _current_req;
        while (last->next)
            last = last->next;

        last->next = req;
    }

    return req;
}

void SPI_SetPollRate(const uint32_t value) {
    TIMER_CTRL(2) = 0x0258; // CLK/8 input, IRQ on reload, disable one-shot IRQ

    if (value < 65)
        TIMER_RELOAD(2) = 0xffff;
    else
        TIMER_RELOAD(2) = (F_CPU / 8) / value;
}

void SPI_Init(SPI_Callback callback) {
    // Disable the BIOS timer handler (which for some stupid reason is enabled
    // by default, even though it does nothing) and set up custom interrupt
    // handlers.
    EnterCriticalSection();
    ChangeClearRCnt(2, 0);
    InterruptCallback(IRQ_TIMER2, &_spi_poll_irq_handler);
    InterruptCallback(IRQ_SIO0, &_spi_ack_irq_handler);
    ExitCriticalSection();

    SIO_CTRL(0) = 0x0040; // Reset all registers
    SIO_MODE(0) = 0x000d; // 1x multiplier, 8 data bits, no parity
    SIO_BAUD(0) = 0x0088; // 250000 bps

    SPI_SetPollRate(250);
    _current_req = nullptr;
    _default_cb = callback;
}

// Just a wrapper around SPI_CreateRequest(). This does not send the command
// immediately but adds it to the driver's request queue.
void send_pad_cmd(
    const uint32_t port,
    const PadCommand cmd,
    const uint8_t arg1,
    const uint8_t arg2,
    const SPI_Callback callback
) {
    SPI_Request *req = SPI_CreateRequest();

    req->len = 9;
    req->port = port;
    req->callback = callback;
    req->pad_req.addr = 0x01;
    req->pad_req.cmd = cmd;
    req->pad_req.tap_mode = 0x00;
    req->pad_req.motor_r = arg1;
    req->pad_req.motor_l = arg2;

    // The padding bytes must be 0xff when unlocking vibration motors.
    memset(
        req->pad_req.dummy,
        (cmd == PAD_CMD_REQUEST_CONFIG) ? 0xff : 0x00,
        4
    );
}

// This callback determines whether a pad that identified as digital is
// actually a DualShock in digital mode by checking if it started identifying
// as CONFIG_MODE after receiving a configuration command. Calls to printf()
// had to be commented out due to them being too slow.
void dualshock_init_cb(const uint32_t port, const volatile uint8_t *buff, const size_t rx_len) {
    if (const auto pad = reinterpret_cast<const volatile PadResponse *>(buff);
        (rx_len < 2) ||
        (pad->prefix != 0x5a) ||
        (pad->type != PAD_ID_CONFIG_MODE)
    ) {
        //printf("no, pad is digital-only (len = %d)\n", rx_len);

        pad_config_attempt[port]++;
        return;
    }

    //printf("yes, forcing analog mode (len = %d)\n", rx_len);

    // Issue further commands to force analog mode on, unlock rumble (not used
    // in this example) and enable longer responses containing button pressure
    // readings.
    // TODO: find out if passing 0x03 instead of 0x02 in PAD_CMD_SET_ANALOG
    // locks the analog button, as emulated by DuckStation...
    // https://gist.github.com/scanlime/5042071
    send_pad_cmd(port, PAD_CMD_CONFIG_MODE, 0x01, 0x00, nullptr);
    send_pad_cmd(port, PAD_CMD_SET_ANALOG, 0x01, 0x02, nullptr);
    send_pad_cmd(port, PAD_CMD_INIT_PRESSURE, 0x00, 0x00, nullptr); // Ignored by DualShock 1
    send_pad_cmd(port, PAD_CMD_REQUEST_CONFIG, 0x00, 0x01, nullptr);
    send_pad_cmd(port, PAD_CMD_RESPONSE_CONFIG, 0xff, 0xff, nullptr); // Ignored by DualShock 1
    send_pad_cmd(port, PAD_CMD_CONFIG_MODE, 0x00, 0x00, nullptr);
}

// This function is called by the pad timer ISR each time a pad is polled and a
// response (even an invalid/incomplete one) is received.
void poll_cb(const uint32_t port, const volatile uint8_t *buff, const size_t rx_len) {
    // Copy the response to a persistent buffer so it can be accessed from the
    // main loop and displayed on screen.
    pad_buff_len[port] = rx_len;
    if (rx_len)
        memcpy((void *) pad_buff[port], (void *) buff, rx_len);

    // If this pad identifies as a digital pad, attempt to put it into analog
    // mode up to 3 times by entering configuration mode. Once the attempt
    // counter exceeds the threshold, it will be treated as digital-only. The
    // attempt counter is reset when the controller is unplugged or stops
    // returning digital pad responses.
    // NOTE: according to nocash docs, there is a hardware bug in DualShock
    // controllers that causes the prefix byte (normally 0x5a) to turn into
    // 0x00 if the analog button is pressed after config commands have been
    // used.
    if (const auto pad = reinterpret_cast<volatile const PadResponse *>(buff);
        rx_len &&
        ((pad->prefix == 0x5a) || !(pad->prefix)) &&
        (pad->type == PAD_ID_DIGITAL)
    ) {
        if (pad_config_attempt[port] < 3) {
            /*printf(
                "Detecting if pad %d supports config mode: attempt %d... ",
                port + 1,
                pad_config_attempt[port] + 1
            );*/

            // The pad only identifies as CONFIG_MODE after at least another
            // command is sent.
            send_pad_cmd(port, PAD_CMD_CONFIG_MODE, 0x01, 0x00, nullptr);
            send_pad_cmd(port, PAD_CMD_READ, 0x00, 0x00, &dualshock_init_cb);
        }
    } else {
        pad_config_attempt[port] = 0;
    }
}

int read_pad(uint32_t &btn) {
    if (!pad_buff_len[0]) {
        return 0;
    }

    const auto pad = reinterpret_cast<volatile PadResponse*>(&pad_buff[0]);
    btn = ~pad->btn;
    return 1;
}
