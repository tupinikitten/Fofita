#include "input_bindings.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

static const SDL_Scancode default_keys[INPUT_BIND_COUNT] = {
    SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT,
    SDL_SCANCODE_X, SDL_SCANCODE_Z, SDL_SCANCODE_RETURN, SDL_SCANCODE_RSHIFT
};

static const SDL_GameControllerButton default_buttons[INPUT_BIND_COUNT] = {
    SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN,
    SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
    SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_B,
    SDL_CONTROLLER_BUTTON_START, SDL_CONTROLLER_BUTTON_BACK
};

static SDL_Scancode keys[INPUT_BIND_COUNT] = {
    SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT,
    SDL_SCANCODE_X, SDL_SCANCODE_Z, SDL_SCANCODE_RETURN, SDL_SCANCODE_RSHIFT
};
static SDL_GameControllerButton buttons[INPUT_BIND_COUNT] = {
    SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN,
    SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
    SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_B,
    SDL_CONTROLLER_BUTTON_START, SDL_CONTROLLER_BUTTON_BACK
};

static bool valid_action(InputBindAction a) {
    return a >= 0 && a < INPUT_BIND_COUNT;
}
static bool valid_keyboard(SDL_Scancode k) {
    return k > SDL_SCANCODE_UNKNOWN && k < SDL_NUM_SCANCODES &&
           k != SDL_SCANCODE_ESCAPE && k != SDL_SCANCODE_SPACE &&
           k != SDL_SCANCODE_F5 && k != SDL_SCANCODE_F6 &&
           k != SDL_SCANCODE_F8;
}
static bool valid_gamepad(SDL_GameControllerButton b) {
    return b >= SDL_CONTROLLER_BUTTON_A && b < SDL_CONTROLLER_BUTTON_MAX &&
           b != SDL_CONTROLLER_BUTTON_GUIDE;
}

void input_bindings_reset(void) {
    memcpy(keys,default_keys,sizeof(keys));
    memcpy(buttons,default_buttons,sizeof(buttons));
}

SDL_Scancode input_bindings_keyboard(InputBindAction action) {
    return valid_action(action) ? keys[action] : SDL_SCANCODE_UNKNOWN;
}
SDL_GameControllerButton input_bindings_gamepad(InputBindAction action) {
    return valid_action(action) ? buttons[action] : SDL_CONTROLLER_BUTTON_INVALID;
}

bool input_bindings_set_keyboard(InputBindAction action, SDL_Scancode key) {
    if(!valid_action(action) || !valid_keyboard(key)) return false;
    SDL_Scancode previous=keys[action];
    for(int i=0; i<INPUT_BIND_COUNT; ++i)
        if(i != (int)action && keys[i] == key) {
            keys[i]=previous;
            break;
        }
    keys[action]=key;
    return true;
}

bool input_bindings_set_gamepad(InputBindAction action, SDL_GameControllerButton button) {
    if(!valid_action(action) || !valid_gamepad(button)) return false;
    SDL_GameControllerButton previous=buttons[action];
    for(int i=0; i<INPUT_BIND_COUNT; ++i)
        if(i != (int)action && buttons[i] == button) {
            buttons[i]=previous;
            break;
        }
    buttons[action]=button;
    return true;
}

static bool valid_map(const SDL_Scancode k[INPUT_BIND_COUNT],
                      const SDL_GameControllerButton b[INPUT_BIND_COUNT]) {
    for(int i=0;i<INPUT_BIND_COUNT;++i) {
        if(!valid_keyboard(k[i]) || !valid_gamepad(b[i])) return false;
        for(int j=0;j<i;++j)
            if(k[i]==k[j] || b[i]==b[j]) return false;
    }
    return true;
}

bool input_bindings_load_file(const char *path) {
    if(!path || !path[0]) return false;
    FILE *f=fopen(path,"rb");
    if(!f) return false;
    char line[128];
    SDL_Scancode read_keys[INPUT_BIND_COUNT]={0};
    SDL_GameControllerButton read_buttons[INPUT_BIND_COUNT];
    for(int i=0;i<INPUT_BIND_COUNT;++i)
        read_buttons[i]=SDL_CONTROLLER_BUTTON_INVALID;
    unsigned seen_keys=0,seen_buttons=0;
    bool ok=false;
    while(fgets(line,sizeof(line),f)) {
        unsigned idx=0;
        int value=0;
        char tail=0;
        if(sscanf(line,"key_%u=%d %c",&idx,&value,&tail)==2) {
            if(idx>=INPUT_BIND_COUNT || (seen_keys&(1u<<idx)) ||
               value<=0 || value>=SDL_NUM_SCANCODES)
                goto end;
            read_keys[idx]=(SDL_Scancode)value;
            seen_keys|=1u<<idx;
        } else if(sscanf(line,"pad_%u=%d %c",&idx,&value,&tail)==2) {
            if(idx>=INPUT_BIND_COUNT || (seen_buttons&(1u<<idx)) ||
               value<0 || value>=SDL_CONTROLLER_BUTTON_MAX)
                goto end;
            read_buttons[idx]=(SDL_GameControllerButton)value;
            seen_buttons|=1u<<idx;
        } else {
            /* Uma configuracao desconhecida nao deve substituir controles. */
            goto end;
        }
    }
    if(ferror(f)) goto end;
    if(seen_keys != (1u<<INPUT_BIND_COUNT)-1 ||
       seen_buttons != (1u<<INPUT_BIND_COUNT)-1 ||
       !valid_map(read_keys,read_buttons))
        goto end;
    memcpy(keys,read_keys,sizeof(keys));
    memcpy(buttons,read_buttons,sizeof(buttons));
    ok=true;
end:
    fclose(f);
    return ok;
}

static bool replace_file(const char *tmp, const char *dest) {
#ifdef _WIN32
    return MoveFileExA(tmp,dest,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
    return rename(tmp,dest)==0;
#endif
}

bool input_bindings_save_file(const char *path) {
    if(!path || !path[0] || !valid_map(keys,buttons)) return false;
    char temp[1024];
    int n=snprintf(temp,sizeof(temp),"%s.tmp",path);
    if(n<0 || (size_t)n>=sizeof(temp)) return false;
    FILE *f=fopen(temp,"wb");
    if(!f) return false;
    bool ok=true;
    for(int i=0;i<INPUT_BIND_COUNT;++i) {
        if(fprintf(f,"key_%d=%d\n",i,(int)keys[i])<0 ||
           fprintf(f,"pad_%d=%d\n",i,(int)buttons[i])<0) {
            ok=false;
            break;
        }
    }
    if(ok && fflush(f)!=0) ok=false;
    if(fclose(f)!=0) ok=false;
    if(!ok) { remove(temp); return false; }
    if(!replace_file(temp,path)) { remove(temp); return false; }
    return true;
}

static bool config_path(char *out,size_t cap) {
    if(!out || cap<1) return false;
    char *dir=SDL_GetPrefPath("TupiniKitten","Fofita");
    if(!dir) return false;
    int n=snprintf(out,cap,"%s%s",dir,"fofita_controles.cfg");
    SDL_free(dir);
    return n>=0 && (size_t)n<cap;
}

void input_bindings_initialize(void) {
    input_bindings_reset();
    char path[1024];
    if(config_path(path,sizeof(path))) {
        FILE *f=fopen(path,"rb");
        if(!f) return;
        fclose(f);
        if(!input_bindings_load_file(path))
            fprintf(stderr,"Aviso: configuracao de controles invalida; usando padrao.\n");
    }
}

bool input_bindings_save(void) {
    char path[1024];
    return config_path(path,sizeof(path)) && input_bindings_save_file(path);
}
