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

extern uint32_t font_tim_raw[];
static RECT font_tim_prect;
static RECT font_tim_crect;
static uint32_t font_tim_mode;

static void flip_buffers(RenderContext *ctx);

static void draw_buffer_char(int x, int y, char ch);

template <typename T>
T *new_primitive(const int z) {
    RenderBuffer *buffer = &ctx.buffers[ctx.active_buffer];
    uint8_t *prim = ctx.next_packet;

    addPrim(&buffer->ot[z], prim);
    ctx.next_packet += sizeof (T);
    assert(ctx.next_packet <= &buffer->buffer[BUFFER_LENGTH]);

    return reinterpret_cast<T*>(prim);
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

static void draw_buffer_char(const int x, const int y, const char ch) {
    if (ch <= 0x20 || ch > 0x7f) {
        return;
    }

    const auto sprt = new_primitive<SPRT_8>(0);

    const uint8_t ch_u = ((ch - 0x20) % 16) * 8;
    const uint8_t ch_v = ((ch - 0x20) / 16) * 8;

    setSprt8(sprt);
    setShadeTex(sprt, 1);
    setXY0(sprt, x * 8, y * 8 + 8);
    setUV0(sprt, ch_u, ch_v);
    setClut(sprt, font_tim_crect.x, font_tim_crect.y);
}

void update_psx() {
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
    constexpr RGB bg_color = COLOR(0x2f, 0x36, 0x40);
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
