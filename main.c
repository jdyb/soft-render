
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <GL/glew.h>

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
            {
                /* Lock texture for writing. */
                int r = SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);
                if (r) {
                    fprintf(stderr, "SDL_LockTexture: %s\n", SDL_GetError());
                    return 1;
                }
            }

            /* Soft-render into pixel array. */
            {
                unsigned x, y;
                for (y = 0; y < window_height; y++) {
                    for (x = 0; x < window_width; x++) {
                        char* line = pixels + y * pitch;
                        char* pixel = line + x * 3;
                        char* red = pixel + 0;
                        char* green = pixel + 1;
                        char* blue = pixel + 2;

                        *red = (x > window_width / 2) * 128;
                        *blue = x % 64;
                        *green = y % 64;


                    }
                }
            }
        }

        /* Unlock texture, no writes after this. */
        SDL_UnlockTexture(texture);

        /* TODO Render from texture. */

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

