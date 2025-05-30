#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <cstdarg>
#include <psxgpu.h>
#include "libpsx.h"

// Length of the ordering table, i.e. the range Z coordinates can have, 0-15 in
// this case. Larger values will allow for more granularity with depth (useful
// when drawing a complex 3D scene) at the expense of RAM usage and performance.
#define OT_LENGTH 16

// Size of the buffer GPU commands and primitives are written to. If the program
// crashes due to too many primitives being drawn, increase this value.
#define BUFFER_LENGTH 8192

// NCurses return codes
#define OK 0
#define ERR (-1)

/* Framebuffer/display list class */

typedef struct {
	DISPENV disp_env;
	DRAWENV draw_env;

	uint32_t ot[OT_LENGTH];
	uint8_t  buffer[BUFFER_LENGTH];
} RenderBuffer;

typedef struct {
	RenderBuffer buffers[2];
	uint8_t      *next_packet;
	int          active_buffer;
} RenderContext;

// Set up our rendering context.
RenderContext ctx;

void setup_context(RenderContext *ctx, int w, int h, int r, int g, int b) {
	// Place the two framebuffers vertically in VRAM.
	SetDefDrawEnv(&(ctx->buffers[0].draw_env), 0, 0, w, h);
	SetDefDispEnv(&(ctx->buffers[0].disp_env), 0, 0, w, h);
	SetDefDrawEnv(&(ctx->buffers[1].draw_env), 0, h, w, h);
	SetDefDispEnv(&(ctx->buffers[1].disp_env), 0, h, w, h);

	// Set the default background color and enable auto-clearing.
	setRGB0(&(ctx->buffers[0].draw_env), r, g, b);
	setRGB0(&(ctx->buffers[1].draw_env), r, g, b);
	ctx->buffers[0].draw_env.isbg = 1;
	ctx->buffers[1].draw_env.isbg = 1;

	// Initialize the first buffer and clear its OT so that it can be used for
	// drawing.
	ctx->active_buffer = 0;
	ctx->next_packet   = ctx->buffers[0].buffer;
	ClearOTagR(ctx->buffers[0].ot, OT_LENGTH);

	// Turn on the video output.
	SetDispMask(1);
}

void flip_buffers(RenderContext *ctx) {
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
	ctx->next_packet    = disp_buffer->buffer;
	ClearOTagR(disp_buffer->ot, OT_LENGTH);
}

void *new_primitive(RenderContext *ctx, int z, size_t size) {
	// Place the primitive after all previously allocated primitives, then
	// insert it into the OT and bump the allocation pointer.
	RenderBuffer *buffer = &(ctx->buffers[ctx->active_buffer]);
	uint8_t      *prim   = ctx->next_packet;

	addPrim(&(buffer->ot[z]), prim);
	ctx->next_packet += size;

	// Make sure we haven't yet run out of space for future primitives.
	assert(ctx->next_packet <= &(buffer->buffer[BUFFER_LENGTH]));

	return (void *) prim;
}

// A simple helper for drawing text using PSn00bSDK's debug font API. Note that
// FntSort() requires the debug font texture to be uploaded to VRAM beforehand
// by calling FntLoad().
void draw_text(RenderContext *ctx, int x, int y, int z, const char *text) {
	RenderBuffer *buffer = &(ctx->buffers[ctx->active_buffer]);

	ctx->next_packet = (uint8_t *)
		FntSort(&(buffer->ot[z]), ctx->next_packet, x, y, text);

	assert(ctx->next_packet <= &(buffer->buffer[BUFFER_LENGTH]));
}

/* Main */

#define SCREEN_XRES 320
#define SCREEN_YRES 240

static int x = 0, y = 0, dx = 1, dy = 1;

static int cursor_x = 0;
static int cursor_y = 0;

#define LINES (240 / 8 - 2)
#define COLS 80

static char text_buffer[LINES][COLS + 1];

void update_psx() {
	// Draw some text in front of the square (Z = 0, primitives with higher
	// Z indices are drawn first).
	for (int y = 0; y < LINES; y++) {
		draw_text(&ctx, 8, y * 8 + 8, 0, text_buffer[y]);
	}

	flip_buffers(&ctx);
}

void init_psx() {
// Initialize the GPU and load the default font texture provided by
	// PSn00bSDK at (960, 0) in VRAM.
	ResetGraph(0);
	FntLoad(960, 0);

	for (auto & row : text_buffer) {
		for (char & cell : row) {
			cell = ' ';
		}
		row[COLS - 1] = 0;
	}

	setup_context(&ctx, SCREEN_XRES, SCREEN_YRES, 63, 0, 127);
}

void delay(int ms) {
    // TODO(forest)
}

void exit(int code) {
    // TODO(forest)
    for (;;);
}

// UI
void putch(unsigned char chr) {
	switch (chr) {
		case '\n':
			cursor_x = 0;
			cursor_y++;
			if (cursor_y == LINES) {
				cursor_y--;
			}
			break;
		default:
			text_buffer[cursor_y][cursor_x++] = chr;
			break;
	}

	if (cursor_x == COLS) {
		if (cursor_y != LINES - 1) {
			cursor_y++;
			cursor_x = 0;
		} else {
			cursor_x--;
		}
	}
	// TODO: Do I need to handle newline, backspace, etc?
}

void gotoxy(int x, int y) {
	if (x < 0 || x > COLS || y < 0 || y > LINES) {
		return;
	}
	cursor_x = x;
	cursor_y = y;
}

void textcolor(int col) {
}

void textbackground(int col) {
}

int wherex() {
    return cursor_x;
}

int wherey() {
    return cursor_y;
}

void cprintf(const char *format,...) {
	static char buffer[2048];          // One full screen if no control seq...

	va_list argp;
	va_start(argp, format);
	vsprintf(buffer, format, argp);
	va_end(argp);

	for (int i = 0; buffer[i] != 0; i++) {
		putch(buffer[i]);
	}
}
