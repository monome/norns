// SDL keyboard input for desktop builds
// Q/A/Z = Key1/2/3, W/E = Enc1, S/D = Enc2, X/C = Enc3
#ifdef NORNS_DESKTOP
#include <stdio.h>
#include <SDL2/SDL.h>
#include "events.h"
#include "event_types.h"

void sdl_handle_input(void) {
    SDL_Event e;
    union event_data *ev;
    SDL_PumpEvents();
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            ev = event_data_new(EVENT_QUIT);
            event_post(ev);
        } else if (e.type == SDL_KEYDOWN && e.key.repeat == 0) {
            switch (e.key.keysym.sym) {
            case SDLK_q:
                ev = event_data_new(EVENT_KEY);
                ev->key.n = 1; ev->key.val = 1;
                event_post(ev);
                break;
            case SDLK_a:
                ev = event_data_new(EVENT_KEY);
                ev->key.n = 2; ev->key.val = 1;
                event_post(ev);
                break;
            case SDLK_z:
                ev = event_data_new(EVENT_KEY);
                ev->key.n = 3; ev->key.val = 1;
                event_post(ev);
                break;
            case SDLK_w:
                ev = event_data_new(EVENT_ENC);
                ev->enc.n = 1; ev->enc.delta = -8;
                event_post(ev);
                break;
            case SDLK_e:
                ev = event_data_new(EVENT_ENC);
                ev->enc.n = 1; ev->enc.delta = 8;
                event_post(ev);
                break;
            case SDLK_s:
                ev = event_data_new(EVENT_ENC);
                ev->enc.n = 2; ev->enc.delta = -1;
                event_post(ev);
                break;
            case SDLK_d:
                ev = event_data_new(EVENT_ENC);
                ev->enc.n = 2; ev->enc.delta = 1;
                event_post(ev);
                break;
            case SDLK_x:
                ev = event_data_new(EVENT_ENC);
                ev->enc.n = 3; ev->enc.delta = -1;
                event_post(ev);
                break;
            case SDLK_c:
                ev = event_data_new(EVENT_ENC);
                ev->enc.n = 3; ev->enc.delta = 1;
                event_post(ev);
                break;
            }
        } else if (e.type == SDL_KEYUP) {
            switch (e.key.keysym.sym) {
            case SDLK_q:
                ev = event_data_new(EVENT_KEY);
                ev->key.n = 1; ev->key.val = 0;
                event_post(ev);
                break;
            case SDLK_a:
                ev = event_data_new(EVENT_KEY);
                ev->key.n = 2; ev->key.val = 0;
                event_post(ev);
                break;
            case SDLK_z:
                ev = event_data_new(EVENT_KEY);
                ev->key.n = 3; ev->key.val = 0;
                event_post(ev);
                break;
            }
        }
    }
}
#endif
