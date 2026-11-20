#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "../libretro.h"
#include "../video_audio.h"

/* Testa a callback real do loader sem depender de uma ROM ou DLL externa. */
extern bool retro_environment_cb(unsigned cmd, void *data);

/* Substitutos das callbacks SDL usadas por libretro_core.c. */
float proporcao_video = 0.0f;
int formato_de_cor = SDL_PIXELFORMAT_RGB555;

void retro_video_refresh_cb(const void *data, unsigned width,
                            unsigned height, size_t pitch) {
    (void)data; (void)width; (void)height; (void)pitch;
}
void retro_audio_sample_cb(int16_t left, int16_t right) {
    (void)left; (void)right;
}
size_t retro_audio_sample_batch_cb(const int16_t *data, size_t frames) {
    (void)data;
    return frames;
}
void retro_input_poll_cb(void) {}
int16_t retro_input_state_cb(unsigned port, unsigned device,
                             unsigned index, unsigned id) {
    (void)port; (void)device; (void)index; (void)id;
    return 0;
}

int main(void) {
    bool can_dupe = false;
    /* O Gambatte so aceita carregar ROMs quando esta consulta retorna
       true e preenche a variavel booleana com true. */
    assert(retro_environment_cb(RETRO_ENVIRONMENT_GET_CAN_DUPE,
                                &can_dupe));
    assert(can_dupe);
    assert(!retro_environment_cb(RETRO_ENVIRONMENT_GET_CAN_DUPE, NULL));

    enum retro_pixel_format format = RETRO_PIXEL_FORMAT_RGB565;
    assert(retro_environment_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT,
                                &format));
    assert(formato_de_cor == SDL_PIXELFORMAT_RGB565);
    assert(!retro_environment_cb(999999U, &can_dupe));

    puts("B1: Gambatte GET_CAN_DUPE e callbacks de video OK");
    return 0;
}
