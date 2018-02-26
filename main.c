
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <assert.h>

struct state {
    unsigned  width;
    unsigned  height;
    char     *pixels;
    unsigned  stride;
};

void draw_thing(struct state* state)
{
    unsigned  width = state->width;
    unsigned  height = state->height;
    char     *pixels = state->pixels;
    unsigned  stride = state->stride;
    unsigned x, y;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            char* line = pixels + y * stride;
            char* pixel = line + x * 3;
            char* red = pixel + 0;
            char* green = pixel + 1;
            char* blue = pixel + 2;

            /* Draw background. */
            *blue = (x % (width / 8) == 0 || y % (width / 8) == 0) * 128;

            unsigned cx = width / 2;
            unsigned cy = height / 2;
            int dx = x - cx;
            int dy = y - cy;

            if (dx < 0) { dx *= -1; }
            if (dy < 0) { dy *= -1; }

            assert (dx >= 0);
            assert (dy >= 0);

            /* Draw square. */
            *green = ((dx < 32) && (dy < 32)) * 128;

            /* Draw circle. */
            /* FIXME Replace glibc sqrt. */
            *red = (sqrt(dx*dx + dy*dy) < 64) * 255;
        }
    }
}

int run(void)
{
    SDL_Window   *window = NULL;
    SDL_Event     event;
    SDL_Renderer *renderer = NULL;
    SDL_Texture  *texture = NULL;
    unsigned      run = 1;
    unsigned      window_width = 256;
    unsigned      window_height = 256;
    unsigned      old_window_width = 0;
    unsigned      old_window_height = 0;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        /* TODO Log error. */
        return 1;
    }

    /* FIXME Window should not float? */
    window = SDL_CreateWindow("soft-render",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            window_width,
            window_height,
            SDL_WINDOW_RESIZABLE);

    if (!window) {
        /* TODO Log error. */
        return 2;
    }

    renderer = SDL_CreateRenderer(
            window,
            -1,
            SDL_RENDERER_ACCELERATED);

    if (!renderer) {
        /* TODO Log error. */
        return 3;
    }

    while (run) {

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                run = 0;
                break;
            case SDL_WINDOWEVENT:
                switch (event.window.event) {
                case SDL_WINDOWEVENT_CLOSE:
                    run = 0;
                    break;
                case SDL_WINDOWEVENT_RESIZED:
                    /* Signals resize, different from old_window_* */
                    window_width = event.window.data1;
                    window_height = event.window.data2;
                    break;
                default:
                    /* FIXME Handle other window events. */
                    break;
                }
            default:
                /* FIXME Handle other events. */
                break;
            }
        }

        /* Reallocate pixel data and texture on resize. */
        if (window_width != old_window_width ||
                window_height != old_window_height) {

            printf("resize %ux%u\n", window_width, window_height);

            if (texture) {
                SDL_DestroyTexture(texture); /* FIXME Check for error. */
                texture = NULL;
            }

            texture = SDL_CreateTexture(
                    renderer,
                    SDL_PIXELFORMAT_RGB24,
                    SDL_TEXTUREACCESS_STREAMING,
                    window_width,
                    window_height);

            if (!texture) {
                /* FIXME report error. */
                return 3;
            }

            old_window_width = window_width;
            old_window_height = window_height;

        }

        {
            char *pixels = NULL;
            int   pitch = 0;
            struct state state;
            {
                /* Lock texture for writing. */
                int r = SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);
                if (r) {
                    fprintf(stderr, "SDL_LockTexture: %s\n", SDL_GetError());
                    return 1;
                }
                if (!pixels) {
                    assert(pixels);
                    return 1;
                }
                if (pitch < 0) {
                    assert(pitch >= 0);
                    return 3;
                }
            }

            state.width = window_width;
            state.height = window_height;
            state.stride = pitch;
            state.pixels = pixels;

            draw_thing(&state);

            /* Unlock texture, no writes after this. */
            SDL_UnlockTexture(texture);
        }

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        SDL_GL_SwapWindow(window);
        SDL_Delay(100);
    }

    /* Cleanup before main return. */
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

int main(void)
{
    return run();
}

