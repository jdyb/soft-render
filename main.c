
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <assert.h>

struct drawstate {
    unsigned  width;
    unsigned  height;
    char     *pixels;
    unsigned  stride;
};

void draw_thing(struct drawstate* state)
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

void draw_grid(struct drawstate* state)
{
    unsigned x, y;
    for (y = 0; y < state->height; y++) {
        for (x = 0; x < state->width; x++) {
            char* line = state->pixels + y * state->stride;
            char* pixel = line + x * 3;
            char* red = pixel + 0;
            char* green = pixel + 1;
            char* blue = pixel + 2;

            *red = 0;
            *green = 0;
            *blue = (x % (state->width / 8) == 0 ||
                    y % (state->width / 8) == 0) * 128;
        }
    }
}

struct sdlstate {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    SDL_Texture  *texture;
    unsigned      window_width;
    unsigned      window_height;
    unsigned      old_window_width;
    unsigned      old_window_height;
};

typedef void (*drawfunction)(struct drawstate*);

const static drawfunction functions[] = {
    draw_thing,
    draw_grid
};

struct controlstate {
    unsigned  active_function;
    unsigned  run;
    SDL_Event event;
};

int run(void)
{
	struct sdlstate sdlstate;
	struct controlstate cstate;

	memset(&sdlstate, 0, sizeof(sdlstate));
	memset(&cstate, 0, sizeof(cstate));

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        /* TODO Log error. */
        return 1;
    }

    cstate.run = 1;
    sdlstate.window_width = 256;
    sdlstate.window_height = 256;

    /* FIXME Window should not float? */
    sdlstate.window = SDL_CreateWindow("soft-render",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            sdlstate.window_width,
            sdlstate.window_height,
            SDL_WINDOW_RESIZABLE);

    if (!sdlstate.window) {
        /* TODO Log error. */
        return 2;
    }

    sdlstate.renderer = SDL_CreateRenderer(
            sdlstate.window,
            -1,
            SDL_RENDERER_ACCELERATED);

    if (!sdlstate.renderer) {
        /* TODO Log error. */
        return 3;
    }

    while (cstate.run) {

        while (SDL_PollEvent(&cstate.event)) {
            switch (cstate.event.type) {
            case SDL_QUIT:
                cstate.run = 0;
                break;
            case SDL_WINDOWEVENT:
                switch (cstate.event.window.event) {
                case SDL_WINDOWEVENT_CLOSE:
                    cstate.run = 0;
                    break;
                case SDL_WINDOWEVENT_RESIZED:
                    /* Signals resize, different from old_window_* */
                    sdlstate.window_width = cstate.event.window.data1;
                    sdlstate.window_height = cstate.event.window.data2;
                    break;
                default:
                    /* FIXME Handle other window events. */
                    break;
                }
            case SDL_KEYDOWN:
                {
                    unsigned count = sizeof(functions) / sizeof(functions[0]);
                    cstate.active_function = (cstate.active_function + 1) % count;
                    printf("function %u / %u\n", cstate.active_function, count);
                }
                break;
            default:
                /* FIXME Handle other events. */
                break;
            }
        }

        /* Reallocate pixel data and texture on resize. */
        if (sdlstate.window_width != sdlstate.old_window_width ||
                sdlstate.window_height != sdlstate.old_window_height) {

            printf("resize %ux%u\n", sdlstate.window_width, sdlstate.window_height);

            if (sdlstate.texture) {
                SDL_DestroyTexture(sdlstate.texture); /* FIXME Check for error. */
                sdlstate.texture = NULL;
            }

            sdlstate.texture = SDL_CreateTexture(
                    sdlstate.renderer,
                    SDL_PIXELFORMAT_RGB24,
                    SDL_TEXTUREACCESS_STREAMING,
                    sdlstate.window_width,
                    sdlstate.window_height);

            if (!sdlstate.texture) {
                /* FIXME report error. */
                return 3;
            }

            sdlstate.old_window_width = sdlstate.window_width;
            sdlstate.old_window_height = sdlstate.window_height;

        }

        {
            char *pixels = NULL;
            int   pitch = 0;
            struct drawstate state;
            {
                /* Lock texture for writing. */
                int r = SDL_LockTexture(sdlstate.texture, NULL, (void**)&pixels, &pitch);
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

            state.width = sdlstate.window_width;
            state.height = sdlstate.window_height;
            state.stride = pitch;
            state.pixels = pixels;

            functions[cstate.active_function](&state);

            /* Unlock texture, no writes after this. */
            SDL_UnlockTexture(sdlstate.texture);
        }

        SDL_RenderClear(sdlstate.renderer);
        SDL_RenderCopy(sdlstate.renderer, sdlstate.texture, NULL, NULL);
        SDL_RenderPresent(sdlstate.renderer);

        SDL_GL_SwapWindow(sdlstate.window);
        SDL_Delay(100);
    }

    /* Cleanup before main return. */
    SDL_DestroyTexture(sdlstate.texture);
    SDL_DestroyRenderer(sdlstate.renderer);
    SDL_DestroyWindow(sdlstate.window);
    SDL_Quit();

    return 0;
}

int main(void)
{
    return run();
}

