#include "libpsx.h"

#include <cassert>
#include <psxgpu.h>
#include <string.h>

#include "enum.h"

// Length of the ordering table, i.e. the range Z coordinates can have, 0-1 in
// this case. Larger values will allow for more granularity with depth (useful
// when drawing a complex 3D scene) at the expense of RAM usage and performance.
#define OT_LENGTH 2

// Size of the buffer GPU commands and primitives are written to. If the program
// crashes due to too many primitives being drawn, increase this value.
#define BUFFER_LENGTH 32768

typedef struct {
    DISPENV disp_env;
    DRAWENV draw_env;

    uint32_t ot[OT_LENGTH];
    uint8_t buffer[BUFFER_LENGTH];
} RenderBuffer;

typedef struct {
    RenderBuffer buffers[2];
    uint8_t *next_packet;
    int active_buffer;
} RenderContext;

static RenderContext ctx;

extern uint32_t font_tim_raw[];
static RECT font_tim_prect;
static RECT font_tim_crect;
static uint32_t font_tim_mode;

static void flip_buffers(RenderContext *ctx);

static void draw_buffer_char(int x, int y, const psx_text_cell &cell);

template<typename T>
T *new_primitive(const int z) {
    RenderBuffer *buffer = &ctx.buffers[ctx.active_buffer];
    uint8_t *prim = ctx.next_packet;

    addPrim(&buffer->ot[z], prim);
    ctx.next_packet += sizeof(T);
	assert(ctx.next_packet - buffer->buffer < BUFFER_LENGTH);

    return reinterpret_cast<T *>(prim);
}

static void draw_text_buffer() {
    for (int yi = 0; yi < PSX_TEXT_LINES; yi++) {
        for (int xi = 0; xi < PSX_TEXT_COLS; xi++) {
            draw_buffer_char(xi, yi, psx_text_buffer[yi][xi]);
        }
    }

    const auto tpri = new_primitive<DR_TPAGE>(0);
    const auto tpage = getTPage(font_tim_mode & 0b11, 0, font_tim_prect.x, font_tim_prect.y);
    setDrawTPage(tpri, 0, 0, tpage);
}

static void draw_buffer_char(const int x, const int y, const psx_text_cell &cell) {
    if (cell.ch <= 0x20 || cell.ch > 0x7f) {
        return;
    }

    const auto sprt = new_primitive<SPRT_8>(0);

    const uint8_t ch_u = (cell.ch - 0x20) % 16 * 8;
    const uint8_t ch_v = (cell.ch - 0x20) / 16 * 8;

    setSprt8(sprt);
    setShadeTex(sprt, 1);
    setXY0(sprt, x * 8, y * 8 + 8);
    setUV0(sprt, ch_u, ch_v);

	assert(cell.fg >= 0 && cell.fg <= 0xf);
    setClut(sprt, font_tim_crect.x, font_tim_crect.y + cell.fg);
}

static volatile uint8_t  pad_buff[2][34];
static volatile size_t   pad_buff_len[2];
static volatile uint32_t pad_config_attempt[2] = { 0, 0 };

// Just a wrapper around SPI_CreateRequest(). This does not send the command
// immediately but adds it to the driver's request queue.
void send_pad_cmd(
	const uint32_t     port,
	const PadCommand   cmd,
	const uint8_t      arg1,
	const uint8_t      arg2,
	const SPI_Callback callback
) {
	SPI_Request *req = SPI_CreateRequest();

	req->len              = 9;
	req->port             = port;
	req->callback         = callback;
	req->pad_req.addr     = 0x01;
	req->pad_req.cmd      = cmd;
	req->pad_req.tap_mode = 0x00;
	req->pad_req.motor_r  = arg1;
	req->pad_req.motor_l  = arg2;

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
	PadResponse *pad = (PadResponse *) buff;

	if (
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
	send_pad_cmd(port, PAD_CMD_CONFIG_MODE,     0x01, 0x00, nullptr);
	send_pad_cmd(port, PAD_CMD_SET_ANALOG,      0x01, 0x02, nullptr);
	send_pad_cmd(port, PAD_CMD_INIT_PRESSURE,   0x00, 0x00, nullptr); // Ignored by DualShock 1
	send_pad_cmd(port, PAD_CMD_REQUEST_CONFIG,  0x00, 0x01, nullptr);
	send_pad_cmd(port, PAD_CMD_RESPONSE_CONFIG, 0xff, 0xff, nullptr); // Ignored by DualShock 1
	send_pad_cmd(port, PAD_CMD_CONFIG_MODE,     0x00, 0x00, nullptr);
}

// This function is called by the pad timer ISR each time a pad is polled and a
// response (even an invalid/incomplete one) is received.
void poll_cb(const uint32_t port, const volatile uint8_t *buff, const size_t rx_len) {
	// Copy the response to a persistent buffer so it can be accessed from the
	// main loop and displayed on screen.
	pad_buff_len[port] = rx_len;
	if (rx_len)
		memcpy((void *) pad_buff[port], (void *) buff, rx_len);

	PadResponse *pad = (PadResponse *) buff;

	// If this pad identifies as a digital pad, attempt to put it into analog
	// mode up to 3 times by entering configuration mode. Once the attempt
	// counter exceeds the threshold, it will be treated as digital-only. The
	// attempt counter is reset when the controller is unplugged or stops
	// returning digital pad responses.
	// NOTE: according to nocash docs, there is a hardware bug in DualShock
	// controllers that causes the prefix byte (normally 0x5a) to turn into
	// 0x00 if the analog button is pressed after config commands have been
	// used.
	if (
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
			send_pad_cmd(port, PAD_CMD_READ,        0x00, 0x00, &dualshock_init_cb);
		}

	} else {
		pad_config_attempt[port] = 0;
	}
}

uint32_t last_btn;

void read_pad() {
	if (!pad_buff_len[0]) {
		return;
	}

	const auto pad = (PadResponse *) pad_buff[0];
	const uint32_t btn = ~pad->btn;

	for (int i = 0; i < 32; ++i) {
		const int mask = (1 << i);
		int cmd = CMD_NO_CMD;

		if ((btn & mask) && !(last_btn & mask)) {
			switch (mask) {
				case PAD_LEFT:
					cmd = CMD_MOVE_LEFT;
					break;

				case PAD_UP:
					cmd = CMD_MOVE_UP;
					break;

				case PAD_RIGHT:
					cmd = CMD_MOVE_RIGHT;
					break;

				case PAD_DOWN:
					cmd = CMD_MOVE_DOWN;
					break;

				case PAD_CROSS:
					cmd = '\n';
					break;

				default:;
			}
		}

		if (cmd != CMD_NO_CMD) {
			set_input_cmd(cmd);
			break;
		}
	}
	last_btn = btn;
}

void update_psx() {
	read_pad();

    draw_text_buffer();
    flip_buffers(&ctx);
}

void init_psx() {
    // Initialize the GPU.
    ResetGraph(0);

    // Load font into VRAM.
    TIM_IMAGE font_tim;
    GetTimInfo(&font_tim_raw[0], &font_tim);

    font_tim_mode = font_tim.mode;
    font_tim_crect = *font_tim.crect;
    font_tim_prect = *font_tim.prect;

    LoadImage(font_tim.prect, font_tim.paddr);
    if (font_tim.mode & 8) {
        LoadImage(font_tim.crect, font_tim.caddr);
    }

    clrscr();

    // Place the two framebuffers vertically in VRAM.
    constexpr int w = PSX_SCREEN_WIDTH_PX;
    constexpr int h = PSX_SCREEN_HEIGHT_PX;
    SetDefDrawEnv(&ctx.buffers[0].draw_env, 0, 0, w, h);
    SetDefDispEnv(&ctx.buffers[0].disp_env, 0, 0, w, h);
    SetDefDrawEnv(&ctx.buffers[1].draw_env, 0, h, w, h);
    SetDefDispEnv(&ctx.buffers[1].disp_env, 0, h, w, h);

    // Set the default background color and enable auto-clearing.
    constexpr auto r = 0x2f;
	constexpr auto g = 0x36;
	constexpr auto b = 0x40;
    setRGB0(&ctx.buffers[0].draw_env, r, g, b);
    setRGB0(&ctx.buffers[1].draw_env, r, g, b);
    ctx.buffers[0].draw_env.isbg = 1;
    ctx.buffers[1].draw_env.isbg = 1;

    // Initialize the first buffer and clear its OT so that it can be used for
    // drawing.
    ctx.active_buffer = 0;
    ctx.next_packet = ctx.buffers[0].buffer;
    ClearOTagR(ctx.buffers[0].ot, OT_LENGTH);

    // Turn on the video output.
    SetDispMask(1);

	// Initialize PAD input.
	// NB: This MUST happen after all of the above. For some reason.
	SPI_Init(&poll_cb);
}

static void flip_buffers(RenderContext *ctx) {
    // Wait for the GPU to finish drawing, then wait for vblank in order to
    // prevent screen tearing.
    DrawSync(0);
    VSync(0);

    RenderBuffer *draw_buffer = &(ctx->buffers[ctx->active_buffer]);
    RenderBuffer *disp_buffer = &(ctx->buffers[ctx->active_buffer ^ 1]);

    // Display the framebuffer the GPU has just finished drawing and start
    // rendering the display list that was filled up in the main loop.
    PutDispEnv(&(disp_buffer->disp_env));
    DrawOTagEnv(&(draw_buffer->ot[OT_LENGTH - 1]), &(draw_buffer->draw_env));

    // Switch over to the next buffer, clear it and reset the packet allocation
    // pointer.
    ctx->active_buffer ^= 1;
    ctx->next_packet = disp_buffer->buffer;
    ClearOTagR(disp_buffer->ot, OT_LENGTH);
}
