import enum
import struct
import sys
from PIL import Image

FONT_PNG_PATH = "source/psx/img/omelette-thin-hvshadow.png"
FONT_TIM_PATH = FONT_PNG_PATH.replace(".png", ".tim")

TEXTURE_VRAM_X = 640
TEXTURE_VRAM_Y = 0

CLUT_VRAM_X = 0
CLUT_VRAM_Y = 480


class Color(enum.Enum):
    BLACK = 0
    BLUE = 1
    GREEN = 2
    CYAN = 3
    RED = 4
    MAGENTA = 5
    BROWN = 6
    LIGHT_GRAY = 7
    GRAY = 8
    BRIGHT_BLUE = 9
    BRIGHT_GREEN = 10
    BRIGHT_CYAN = 11
    BRIGHT_RED = 12
    BRIGHT_MAGENTA = 13
    YELLOW = 14
    WHITE = 15


COLOR_TO_RGB: dict[Color, tuple[int, int, int]] = {
    Color.BLACK: (0x2f, 0x36, 0x40),
    Color.BLUE: (0x00, 0x97, 0xe6),
    Color.GREEN: (0x44, 0xbd, 0x32),
    Color.CYAN: (0x12, 0x89, 0xa7),
    Color.RED: (0xc2, 0x36, 0x16),
    Color.MAGENTA: (0xb5, 0x34, 0x71),
    Color.BROWN: (0xcd, 0x61, 0x33),
    Color.LIGHT_GRAY: (0xdc, 0xdd, 0xe1),
    Color.GRAY: (0x71, 0x80, 0x93),
    Color.BRIGHT_BLUE: (0x00, 0xa8, 0xff),
    Color.BRIGHT_GREEN: (0x4c, 0xd1, 0x37),
    Color.BRIGHT_CYAN: (0x12, 0xcb, 0xc4),
    Color.BRIGHT_RED: (0xe8, 0x41, 0x18),
    Color.BRIGHT_MAGENTA: (0xed, 0x4c, 0x67),
    Color.YELLOW: (0xfb, 0xc5, 0x31),
    Color.WHITE: (0xf5, 0xf6, 0xfa),
}


def to_tim_color(rgb: tuple[int, int, int]) -> bytes:
    r = (rgb[0] >> 3) & 0x1f
    g = (rgb[1] >> 3) & 0x1f
    b = (rgb[2] >> 3) & 0x1f
    return struct.pack("<H", (b << 10) | (g << 5) | r)


def get_clut_index(rgba: tuple[int, int, int, int]) -> int:
    r, g, b, a = rgba
    index = None
    if a == 0:
        index = 0
    elif r == 255 and g == 255 and b == 255:
        index = 1
    elif r == 0 and g == 0 and b == 0:
        index = 2
    if index is None:
        raise Exception(f"unsupported color {rgba}")
    return index

def main() -> None:
    with open(FONT_TIM_PATH, "wb") as f:
        f.write(struct.pack("<I", 0x10))  # ID
        f.write(struct.pack("<I", 1 << 3))  # Flags: 4-bit CLUT + Has CLUT section

        # Write CLUTs
        bnum = 4 + 2 + 2 + 2 + 2 + len(Color) * 2 * 16
        clut_width = 16
        clut_height = len(Color)
        f.write(struct.pack("<III", bnum, (CLUT_VRAM_Y << 16) | CLUT_VRAM_X, (clut_height << 16) | clut_width))

        for color in Color:
            clut = bytearray(32)
            clut[0], clut[1] = 0x00, 0x00  # Transparent color
            clut[2], clut[3] = to_tim_color(COLOR_TO_RGB[color])  # Foreground color
            clut[4], clut[5] = 0x80, 0x00  # Black outline
            f.write(clut)

        # Write image header
        im = Image.open(FONT_PNG_PATH).convert("RGBA")
        assert im.width % 4 == 0

        im_width = im.width // 4
        im_height = im.height
        assert im_width <= 0xffff and im_height <= 0xffff

        bnum = 4 + 2 + 2 + 2 + 2 + im_width * im_height * 2
        f.write(struct.pack("<III", bnum, (TEXTURE_VRAM_Y << 16) | TEXTURE_VRAM_X, (im_height << 16) | im_width))

        for y in range(im_height):
            for x in range(im_width):
                pix0, pix1, pix2, pix3 = [get_clut_index(im.getpixel((x * 4 + i, y))) for i in range(4)]
                f.write(struct.pack("<H", pix3 << 12 | pix2 << 8 | pix1 << 4 | pix0))


if __name__ == "__main__":
    main()
    sys.exit(0)
