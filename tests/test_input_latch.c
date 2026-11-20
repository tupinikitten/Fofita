#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* O runner Windows pode exibir uma caixa de dialogo quando assert falha.
   Encerrar diretamente para deixar o erro visivel no log do Actions. */
#undef assert
#define assert(cond) do { \
    if(!(cond)) { \
        fprintf(stderr,"Input latch FAIL %s:%d: %s\\n",__FILE__,__LINE__,#cond); \
        exit(1); \
    } \
} while(0)
#include <string.h>
#include "../input_latch.h"

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    SDL_Event e={0};
    input_latch_reset();

    /* Toque rapido entre frames: DOWN e UP na mesma varredura de eventos.
       O game ainda deve enxergar um quadro de DOWN. */
    e.type=SDL_KEYDOWN;
    e.key.keysym.scancode=SDL_SCANCODE_DOWN;
    input_latch_event(&e);
    e.type=SDL_KEYUP;
    input_latch_event(&e);
    input_latch_begin_frame();
    assert(input_latch_key(SDL_SCANCODE_DOWN));
    assert(!input_latch_key(SDL_SCANCODE_UP));
    input_latch_begin_frame();
    assert(!input_latch_key(SDL_SCANCODE_DOWN));

    /* A/B/START preservam teclas remapeadas: o latch usa scancode fisico. */
    e.type=SDL_KEYDOWN;
    e.key.keysym.scancode=SDL_SCANCODE_X;
    e.key.repeat=0;
    input_latch_event(&e);
    e.key.keysym.scancode=SDL_SCANCODE_RETURN;
    input_latch_event(&e);
    input_latch_begin_frame();
    assert(input_latch_key(SDL_SCANCODE_X));
    assert(input_latch_key(SDL_SCANCODE_RETURN));

    /* KEYDOWN repetido nao injeta novos apertos indevidos. */
    e.key.repeat=1;
    input_latch_event(&e);
    input_latch_begin_frame();
    assert(!input_latch_key(SDL_SCANCODE_RETURN));

    /* Botao de gamepad tambem nao desaparece se soltar rapidamente. */
    memset(&e,0,sizeof(e));
    e.type=SDL_CONTROLLERBUTTONDOWN;
    e.cbutton.button=SDL_CONTROLLER_BUTTON_A;
    e.cbutton.which=123;
    input_latch_event(&e);
    e.type=SDL_CONTROLLERBUTTONUP;
    input_latch_event(&e);
    input_latch_begin_frame();
    assert(input_latch_button(123,SDL_CONTROLLER_BUTTON_A));
    assert(!input_latch_button(456,SDL_CONTROLLER_BUTTON_A));
    input_latch_begin_frame();
    assert(!input_latch_button(123,SDL_CONTROLLER_BUTTON_A));

    /* Perder foco cancela pendencias (evita movimentos fantasma). */
    e.type=SDL_KEYDOWN;
    e.key.keysym.scancode=SDL_SCANCODE_LEFT;
    e.key.repeat=0;
    input_latch_event(&e);
    e.type=SDL_WINDOWEVENT;
    e.window.event=SDL_WINDOWEVENT_FOCUS_LOST;
    input_latch_event(&e);
    input_latch_begin_frame();
    assert(!input_latch_key(SDL_SCANCODE_LEFT));
    assert(!input_latch_key(SDL_SCANCODE_UNKNOWN));

    puts("Input: toques curtos preservados, sem repeticoes ou ghosting");
    return 0;
}
