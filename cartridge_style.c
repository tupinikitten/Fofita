#include "cartridge_style.h"
#include <stdio.h>
#include <math.h>

static const CartridgeColor palette[] = {
    {200,61,85}, {232,137,162}, {217,181,108}, {122,157,120},
    {106,133,184}, {139,108,177}, {229,225,216}, {69,70,77},
    {103,175,168}, {199,123,69}, {167,160,200}, {167,183,198}
};

const CartridgeColor *cartridge_style_palette(size_t *count) {
    if (count) *count = sizeof(palette) / sizeof(palette[0]);
    return palette;
}


CartridgeColor cartridge_style_color_from_hsv(float hue, float saturation, float value) {
    CartridgeColor out = {0, 0, 0};
    if (saturation < 0.0f) saturation = 0.0f;
    if (saturation > 1.0f) saturation = 1.0f;
    if (value < 0.0f) value = 0.0f;
    if (value > 1.0f) value = 1.0f;
    while (hue < 0.0f) hue += 360.0f;
    while (hue >= 360.0f) hue -= 360.0f;
    float c = value * saturation;
    float x = c * (1.0f - fabsf(fmodf(hue / 60.0f, 2.0f) - 1.0f));
    float m = value - c;
    float r = 0.0f, g = 0.0f, b = 0.0f;
    if (hue < 60.0f) { r=c; g=x; }
    else if (hue < 120.0f) { r=x; g=c; }
    else if (hue < 180.0f) { g=c; b=x; }
    else if (hue < 240.0f) { g=x; b=c; }
    else if (hue < 300.0f) { r=x; b=c; }
    else { r=c; b=x; }
    out.r = (uint8_t)((r + m) * 255.0f + 0.5f);
    out.g = (uint8_t)((g + m) * 255.0f + 0.5f);
    out.b = (uint8_t)((b + m) * 255.0f + 0.5f);
    return out;
}

size_t cartridge_style_load_favorites(CartridgeColor *out, size_t capacity, const char *file_path) {
    if (!out || capacity == 0 || !file_path) return 0;
    FILE *file = fopen(file_path, "r");
    if (!file) return 0;
    size_t count = 0;
    unsigned int r, g, b;
    while (count < capacity && fscanf(file, "%u %u %u", &r, &g, &b) == 3) {
        if (r <= 255 && g <= 255 && b <= 255) {
            out[count].r = (uint8_t)r;
            out[count].g = (uint8_t)g;
            out[count].b = (uint8_t)b;
            count++;
        }
    }
    fclose(file);
    return count;
}

int cartridge_style_add_favorite(CartridgeColor color, const char *file_path) {
    if (!file_path) return 0;
    CartridgeColor existing[64];
    size_t count = cartridge_style_load_favorites(existing, 64, file_path);
    if (count >= 64) return 0;
    for (size_t i = 0; i < count; ++i)
        if (existing[i].r == color.r && existing[i].g == color.g && existing[i].b == color.b) return 1;
    FILE *file = fopen(file_path, "a");
    if (!file) return 0;
    int ok = fprintf(file, "%u %u %u\n", color.r, color.g, color.b) > 0;
    fclose(file);
    return ok;
}

int cartridge_style_remove_favorite(size_t index, const char *file_path) {
    if (!file_path) return 0;
    CartridgeColor colors[64];
    size_t count = cartridge_style_load_favorites(colors, 64, file_path);
    if (index >= count) return 0;
    FILE *file = fopen(file_path, "w");
    if (!file) return 0;
    for (size_t i = 0; i < count; ++i) {
        if (i == index) continue;
        fprintf(file, "%u %u %u\n", colors[i].r, colors[i].g, colors[i].b);
    }
    fclose(file);
    return 1;
}
