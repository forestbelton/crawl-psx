#ifndef PSX_INPUT_H
#define PSX_INPUT_H

#include <cstddef>
#include <psxpad.h>

// Maximum request/response length (34 bytes for pads, 140 for memory cards).
// Must be a multiple of 4 to avoid memory alignment issues.
//#define SPI_BUFF_LEN 36
#define SPI_BUFF_LEN 140

typedef void (*SPI_Callback)(uint32_t port, const volatile uint8_t *buff, size_t rx_len);

typedef struct SPI_Request {
    union {
        uint8_t data[SPI_BUFF_LEN];
        PadRequest pad_req;
        MemCardRequest mcd_req;
    };

    uint32_t len, port;
    SPI_Callback callback;
    SPI_Request *next;
} SPI_Request;

/**
 * @brief Installs the SPI and timer 2 interrupt handlers and starts the poll
 * timer. By default, the polling rate is set to 250 Hz (125 Hz per port),
 * however it can be changed at any time by calling SPI_SetPollRate().
 *
 * The provided callback (if any) is called to report the result of poll
 * requests, which are issued automatically when no other request is in the
 * queue. Passing NULL as callback does not disable auto-polling.
 *
 * @param callback
 */
void SPI_Init(SPI_Callback callback);

/**
 * @brief Allocates a new request object and adds it to the request queue. The
 * object must be populated afterward by setting the length, callback and
 * filling in the TX data buffer.
 */
SPI_Request *SPI_CreateRequest();

/**
 * @brief Changes the controller polling rate. The lowest supported rate is 65
 * Hz (requests sent every 1/65th of a second, each port polled at 32.5 Hz when
 * no request is pending).
 *
 * @param value
 */
void SPI_SetPollRate(uint32_t value);

#endif
