#include <assert.h>
#include <stdio.h>
#include "../input_bindings.h"

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    const char *path="b2-test-controls.cfg";
    input_bindings_reset();

    assert(input_bindings_keyboard(INPUT_BIND_A)==SDL_SCANCODE_X);
    assert(input_bindings_keyboard(INPUT_BIND_B)==SDL_SCANCODE_Z);
    assert(input_bindings_gamepad(INPUT_BIND_SELECT)==SDL_CONTROLLER_BUTTON_BACK);
    assert(!input_bindings_set_keyboard(INPUT_BIND_A,SDL_SCANCODE_ESCAPE));
    assert(!input_bindings_set_keyboard(INPUT_BIND_A,SDL_SCANCODE_SPACE));
    assert(!input_bindings_set_keyboard(INPUT_BIND_A,SDL_SCANCODE_F5));
    assert(!input_bindings_set_keyboard(INPUT_BIND_A,SDL_SCANCODE_F6));
    assert(!input_bindings_set_keyboard(INPUT_BIND_A,SDL_SCANCODE_F8));
    assert(!input_bindings_set_gamepad(INPUT_BIND_B,SDL_CONTROLLER_BUTTON_GUIDE));
    assert(!input_bindings_set_keyboard((InputBindAction)99,SDL_SCANCODE_A));

    /* Usar uma tecla ocupada troca as duas: nunca perde o botao B. */
    assert(input_bindings_set_keyboard(INPUT_BIND_A,SDL_SCANCODE_Z));
    assert(input_bindings_keyboard(INPUT_BIND_A)==SDL_SCANCODE_Z);
    assert(input_bindings_keyboard(INPUT_BIND_B)==SDL_SCANCODE_X);
    assert(input_bindings_set_gamepad(INPUT_BIND_A,SDL_CONTROLLER_BUTTON_B));
    assert(input_bindings_gamepad(INPUT_BIND_A)==SDL_CONTROLLER_BUTTON_B);
    assert(input_bindings_gamepad(INPUT_BIND_B)==SDL_CONTROLLER_BUTTON_A);
    assert(input_bindings_save_file(path));

    input_bindings_reset();
    assert(input_bindings_keyboard(INPUT_BIND_A)==SDL_SCANCODE_X);
    assert(input_bindings_load_file(path));
    assert(input_bindings_keyboard(INPUT_BIND_A)==SDL_SCANCODE_Z);
    assert(input_bindings_gamepad(INPUT_BIND_B)==SDL_CONTROLLER_BUTTON_A);

    /* Arquivo incorreto nao modifica controles que ja funcionam. */
    FILE *f=fopen(path,"wb"); assert(f);
    assert(fputs("key_0=99999\n",f)>=0); assert(fclose(f)==0);
    assert(!input_bindings_load_file(path));
    assert(input_bindings_keyboard(INPUT_BIND_A)==SDL_SCANCODE_Z);
    assert(remove(path)==0);
    input_bindings_reset();
    assert(input_bindings_keyboard(INPUT_BIND_A)==SDL_SCANCODE_X);
    puts("Remapeamento: default, troca, persistencia e protecao OK");
    return 0;
}
