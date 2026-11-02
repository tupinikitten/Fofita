#ifndef CARTRIDGE_STYLE_H
#define CARTRIDGE_STYLE_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    CART_FINISH_SOLID = 0,
    CART_FINISH_CLEAR = 1,
    CART_FINISH_GLITTER = 2
} CartridgeFinish;

typedef struct {
    uint8_t r, g, b;
} CartridgeColor;

typedef struct {
    CartridgeColor color;
    CartridgeFinish finish;
} CartridgeStyle;

/* Paleta inicial; a interface poderá acrescentar cores personalizadas. */
const CartridgeColor *cartridge_style_palette(size_t *count);
CartridgeColor cartridge_style_color_from_hsv(float hue_degrees, float saturation, float value);
int cartridge_style_add_favorite(CartridgeColor color, const char *file_path);
int cartridge_style_remove_favorite(size_t index, const char *file_path);
size_t cartridge_style_load_favorites(CartridgeColor *out, size_t capacity, const char *file_path);

#endif
