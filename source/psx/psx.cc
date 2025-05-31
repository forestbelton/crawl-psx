#include "libpsx.h"

#include <cassert>
#include <psxgpu.h>

// Length of the ordering table, i.e. the range Z coordinates can have, 0-1 in
// this case. Larger values will allow for more granularity with depth (useful
// when drawing a complex 3D scene) at the expense of RAM usage and performance.
#define OT_LENGTH 2

// Size of the buffer GPU commands and primitives are written to. If the program
// crashes due to too many primitives being drawn, increase this value.
#define BUFFER_LENGTH 8192

#define COLOR(r,g,b) {r,g,b}

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

typedef struct {
    int r;
    int g;
    int b;
} RGB;

static RenderContext ctx;

static void flip_buffers(RenderContext *ctx);

static void draw_text(RenderContext *ctx, int x, int y, int z, const char *text);

void update_psx() {
    // Draw some text in front of the square (Z = 0, primitives with higher
    // Z indices are drawn first).
    for (int y = 0; y < PSX_TEXT_LINES; y++) {
        draw_text(&ctx, 0, y * 8 + 8, 0, &psx_text_buffer[y][0]);
    }

    flip_buffers(&ctx);
}

void init_psx() {
    // Initialize the GPU and load the default font texture provided by
    // PSn00bSDK at (960, 0) in VRAM.
    ResetGraph(0);
    FntLoad(960, 0);

    clrscr();

    // Place the two framebuffers vertically in VRAM.
    constexpr int w = PSX_SCREEN_WIDTH_PX;
    constexpr int h = PSX_SCREEN_HEIGHT_PX;
    SetDefDrawEnv(&ctx.buffers[0].draw_env, 0, 0, w, h);
    SetDefDispEnv(&ctx.buffers[0].disp_env, 0, 0, w, h);
    SetDefDrawEnv(&ctx.buffers[1].draw_env, 0, h, w, h);
    SetDefDispEnv(&ctx.buffers[1].disp_env, 0, h, w, h);

    // Set the default background color and enable auto-clearing.
    constexpr RGB bg_color = COLOR(63, 0, 127);
    setRGB0(&ctx.buffers[0].draw_env, bg_color.r, bg_color.g, bg_color.b);
    setRGB0(&ctx.buffers[1].draw_env, bg_color.r, bg_color.g, bg_color.b);
    ctx.buffers[0].draw_env.isbg = 1;
    ctx.buffers[1].draw_env.isbg = 1;

    // Initialize the first buffer and clear its OT so that it can be used for
    // drawing.
    ctx.active_buffer = 0;
    ctx.next_packet = ctx.buffers[0].buffer;
    ClearOTagR(ctx.buffers[0].ot, OT_LENGTH);

    // Turn on the video output.
    SetDispMask(1);
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
    ctx->next_packet    = disp_buffer->buffer;
    ClearOTagR(disp_buffer->ot, OT_LENGTH);
}

// A simple helper for drawing text using PSn00bSDK's debug font API. Note that
// FntSort() requires the debug font texture to be uploaded to VRAM beforehand
// by calling FntLoad().
static void draw_text(RenderContext *ctx, const int x, const int y, const int z, const char *text) {
    RenderBuffer *buffer = &(ctx->buffers[ctx->active_buffer]);

    ctx->next_packet = static_cast<uint8_t *>(
        FntSort(&(buffer->ot[z]), ctx->next_packet, x, y, text));

    assert(ctx->next_packet <= &(buffer->buffer[BUFFER_LENGTH]));
}
