
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <GL/glew.h>
#include <time.h>
#include <assert.h>

#include "prof.h"

struct drawstate {
    unsigned  width;
    unsigned  height;
    unsigned char     *pixels;
    unsigned  stride;
};

struct v3 {
	float x, y, z;
};

struct color {
	unsigned char r;
	unsigned char g;
	unsigned char b;
};

float dot(struct v3 a, struct v3 b)
{
	return (a.x * b.x)
		+ (a.y * b.y)
		+ (a.z * b.z);
}

struct v3 sub(struct v3 a, struct v3 b)
{
	struct v3 c;
	c.x = a.x - b.x;
	c.y = a.y - b.y;
	c.z = a.z - b.z;
	return c;
}

struct v3 add(struct v3 a, struct v3 b)
{
	struct v3 c;
	c.x = a.x + b.x;
	c.y = a.y + b.y;
	c.z = a.z + b.z;
	return c;
}

struct sphere {
	struct v3 p;
	unsigned r;
	struct color c;
};

struct ray {
	struct v3 d;
};

/* Wrapper around a pixel from the canvas. */
struct canvas_pixel {
	unsigned w, h;
	int x, y;
	struct color c;
};

struct f2 {
	float f[2];
};

struct profblock profblocks[PROFNAME_LAST];
FILE            *prof_file;

void prof_start(enum profname name)
{
	struct profblock *pb = profblocks + name;
	assert(!pb->active);

	pb->active = 1;
	pb->count_current_frame++;
	pb->count_total++;

	pb->current_clock_start = clock();
}

void prof_end(enum profname name)
{
	struct profblock *pb = profblocks + name;
	assert(pb->active);

	pb->active = 0;
	pb->current_clock_end = clock();

	clock_t clock_delta = pb->current_clock_end - pb->current_clock_start;
	float delta_seconds = ((float)clock_delta) / CLOCKS_PER_SEC;

	pb->clock_current_frame += delta_seconds;
	pb->clock_total += delta_seconds;
}

void prof_frame_reset(void)
{
	for (int i = 0; i < PROFNAME_LAST; i++) {
		struct profblock *pb = profblocks + i;
		assert(!pb->active);
		pb->clock_last_frame = pb->clock_current_frame;
		pb->count_last_frame = pb->count_current_frame;
		pb->clock_current_frame = 0;
		pb->count_current_frame = 0;
	}
}

void prof_clear(enum profname name)
{
	struct profblock *pb = profblocks + name;
	pb->active = 0;
	pb->clock_current_frame = 0;
	pb->clock_last_frame = 0;
	pb->clock_total = 0;
	pb->count_current_frame = 0;
	pb->clock_last_frame = 0;
	pb->count_total = 0;
}

/* Intersection calculation between a ray and a sphere. */
struct f2 intersec_ray_sphere(struct ray ray, struct sphere sphere)
{
	struct f2 f2;

	struct v3 o = {0, 0, 0};
	struct v3 oc = sub(o, sphere.p);

	float k1 = dot(ray.d, ray.d);
	float k2 = 2 * dot(oc, ray.d);
	float k3 = dot(oc, oc) - sphere.r * sphere.r;

	float d = k2 * k2 - 4 * k1 * k3;

	if (d < 0) {
		f2.f[0] = INFINITY;
		f2.f[1] = -INFINITY;
	} else {
		f2.f[0] = (-k2 + sqrt(d)) / (2*k1);
		f2.f[1] = (-k2 - sqrt(d)) / (2*k1);
	}

	return f2;
}

unsigned in_range(float f, float f_min, float f_max)
{
	return f >= f_min && f < f_max;
}

/* Trace a ray against a sphere, get color from sphere. */
struct color trace_ray(struct ray ray, struct sphere *spheres, unsigned count)
{
	float t_min = 1;
	float t_max = INFINITY;
	unsigned index;
	float t_closest = INFINITY;
	unsigned index_closest = count;

	for (index = 0; index < count; index++) {
		struct f2 t = intersec_ray_sphere(ray, spheres[index]);
		unsigned t0_valid = in_range(t.f[0], t_min, t_max) && t.f[0] < t_closest;
		unsigned t1_valid = in_range(t.f[1], t_min, t_max) && t.f[1] < t_closest;
		if (t0_valid) {
			t_closest = t.f[0];
			index_closest = index;
		} else if(t1_valid) {
			t_closest = t.f[1];
			index_closest = index;
		}
	}

	if (index_closest < count) {
		return spheres[index_closest].c;
	} else {
		struct color black = {0, 0, 0};
		return black;
	}
}

void trace_sphere_pixel(struct canvas_pixel *pixel)
{
	float vw = 1; /* Viewport x size (width). */
	float vh = 1; /* Viewport y size (height) */
	float d = 1; /* Viewport z distance from camera. */

	struct ray ray;
	struct sphere spheres[3];
	struct color red = {255, 0, 0};
	struct color green = {0, 255, 0};
	struct color blue = {0, 0, 255};

	/* Spheres to check for intersection. */
	spheres[0].p.x =  0;
	spheres[0].p.y = -1;
	spheres[0].p.z =  3;
	spheres[0].r = 1;
	spheres[0].c = red;

	spheres[1].p.x =  2;
	spheres[1].p.y =  0;
	spheres[1].p.z =  4;
	spheres[1].r = 1;
	spheres[1].c = blue;

	spheres[2].p.x = -2;
	spheres[2].p.y =  0;
	spheres[2].p.z =  4;
	spheres[2].r = 1;
	spheres[2].c = green;

	/* Compute ray direction. */
	ray.d.x = pixel->x * (vw / pixel->w);
	ray.d.y = pixel->y * (vh / pixel->h);
	ray.d.z = d;

	/* Finally trace ray against sphere, get color from sphere. */
	pixel->c = trace_ray(ray, spheres, 3);
}

/* Wrapper for pixel address computations. */
void set_canvas_pixel(
		struct drawstate* state,
		struct color c,
		unsigned x,
		unsigned y)
{
	unsigned char *pixels = state->pixels;
	unsigned char* line = pixels + y * state->stride;
	unsigned char* pixel = line + x * 3;
	unsigned char* red = pixel + 0;
	unsigned char* green = pixel + 1;
	unsigned char* blue = pixel + 2;
	*red = c.r;
	*green = c.g;
	*blue = c.b;
}

/* Draw a 'sphere' using a simplistic raytracer. */
void draw(struct drawstate *state)
{
	unsigned  width = state->width;
	unsigned  height = state->height;
	unsigned  x, y;
	struct canvas_pixel p;
	p.w = state->width;
	p.h = state->height;
	prof_start(PROFNAME_DRAW);
	/* FIXME lift these loops and select a per-pixel function instead
	 * of loop function in functions table. */
	for (y = 0; y < height; y++) {
		p.y = -y + height / 2;
		for (x = 0; x < width; x++) {
			p.x = x - width / 2;
			trace_sphere_pixel(&p);
			set_canvas_pixel(state, p.c, x, y);
		}
	}
	prof_end(PROFNAME_DRAW);

	unsigned pixel_count = height * width;
	profblocks[PROFNAME_PIXEL].count_current_frame = pixel_count;
	profblocks[PROFNAME_PIXEL].count_last_frame = pixel_count;
	profblocks[PROFNAME_PIXEL].count_total += pixel_count;
	profblocks[PROFNAME_PIXEL].clock_current_frame =
		profblocks[PROFNAME_DRAW].clock_current_frame / pixel_count;
	profblocks[PROFNAME_PIXEL].clock_current_frame =
		profblocks[PROFNAME_DRAW].clock_last_frame / pixel_count;
	profblocks[PROFNAME_PIXEL].clock_total +=
		profblocks[PROFNAME_PIXEL].clock_current_frame;

}


struct sdlstate {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    SDL_Texture  *texture;
    TTF_Font     *font;
    SDL_Surface  *text_surface;
    SDL_Texture  *text_texture;
    SDL_Rect      text_rect;
    char          text_lines[64][512];
    unsigned      line_count;
    unsigned      window_width;
    unsigned      window_height;
    unsigned      old_window_width;
    unsigned      old_window_height;
};

struct controlstate {
    unsigned  active_function;
    unsigned  run;
    SDL_Event event;
    unsigned mouse_x, mouse_y;
    unsigned mouse_active;
};

int run(void)
{
	struct sdlstate sdlstate;
	struct controlstate cstate;

	memset(&sdlstate, 0, sizeof(sdlstate));
	memset(&cstate, 0, sizeof(cstate));

	prof_file = fopen("prof", "w");
	if (!prof_file) {
		fputs("Could not open profile file.\n", stderr);
		perror("fopen");
		return 1;
	}

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        /* TODO Log error. */
        return 1;
    }

    /* FIXME Replace SDL_ttf with our own code. */
    if (TTF_Init() == -1) {
	    fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
	    return 2;
    }

    sdlstate.font = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeMonoOblique.ttf", 12);
    if (!sdlstate.font) {
	    fprintf(stderr, "TTF_OpenFont: %s\n", TTF_GetError());
	    return 3;
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
	    prof_start(PROFNAME_LOOP);

	    cstate.mouse_active = 0;

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
		break;
	    case SDL_MOUSEMOTION:
		{
			cstate.mouse_x = cstate.event.motion.x;
			cstate.mouse_y = cstate.event.motion.y;
		}
		break;
	    case SDL_MOUSEBUTTONDOWN:
	    {
		    cstate.mouse_active = 1;
	    }
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
            unsigned char *pixels = NULL;
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

	    draw(&state);


		if (cstate.mouse_active) {
			unsigned char* line = (unsigned char*)pixels + cstate.mouse_y * pitch;
			unsigned char* pixel = line + cstate.mouse_x * 3;
			unsigned char* red = pixel + 0;
			unsigned char* green = pixel + 1;
			unsigned char* blue = pixel + 2;
			printf("pixel %3u %3u color %3u %3u %3u\n",
					cstate.mouse_x,
					cstate.mouse_y,
					*red, *green, *blue);
		}

        	/* Unlock texture, no writes after this. */
        	SDL_UnlockTexture(sdlstate.texture);

        }

        SDL_RenderClear(sdlstate.renderer);

        SDL_RenderCopy(sdlstate.renderer, sdlstate.texture, NULL, NULL);

	sdlstate.line_count = 0;
	for (int i = 0; i < PROFNAME_LAST && i < 20; i++) {
		struct profblock *pb = profblocks + i;
		sprintf(sdlstate.text_lines[i], "%3x %8s %12u %12u %9.1f HZ (%0.5f)",
				i,
				profstr[i],
				pb->count_last_frame,
				pb->count_total,
				1 / pb->clock_last_frame,
				pb->clock_last_frame);
		sdlstate.line_count++;
	}


	for (unsigned i = 0; i < sdlstate.line_count; i++) {
		SDL_Color bg = {0, 0, 0, 0};
		SDL_Color fg = {255, 255, 255, 255};

		if (strlen(sdlstate.text_lines[i]) == 0)
			continue;

		if (sdlstate.text_texture) {
			SDL_DestroyTexture(sdlstate.text_texture);
			sdlstate.text_texture = NULL;
		}

		if (sdlstate.text_surface) {
			SDL_FreeSurface(sdlstate.text_surface);
			sdlstate.text_surface = NULL;
		}

		sdlstate.text_surface = TTF_RenderText(
				sdlstate.font,
				sdlstate.text_lines[i],
				fg,
				bg);

		if (!sdlstate.text_surface) {
			fprintf(stderr, "TTF_RenderText: %s\n", TTF_GetError());
			return 1;
		}

		sdlstate.text_texture =
			SDL_CreateTextureFromSurface(
					sdlstate.renderer,
					sdlstate.text_surface);

		if (!sdlstate.text_texture) {
			fprintf(stderr, "SDL_CreateTextureFromSurface: %s\n",
					SDL_GetError());
			return 1;
		}

		sdlstate.text_rect.h = sdlstate.text_surface->h;
		sdlstate.text_rect.w = sdlstate.text_surface->w;

		sdlstate.text_rect.x = 5;
		sdlstate.text_rect.y = sdlstate.window_height
			- sdlstate.text_surface->h * (i + 1) - 5;

		SDL_RenderCopy(sdlstate.renderer, sdlstate.text_texture, NULL, &(sdlstate.text_rect));
	}

        SDL_RenderPresent(sdlstate.renderer);

        SDL_GL_SwapWindow(sdlstate.window);

	prof_end(PROFNAME_LOOP);

	prof_frame_reset();
    }

    /* FIXME Usa a proper file format for profile data. */
    fwrite(profblocks, sizeof(profblocks[0]), PROFNAME_LAST, prof_file);


    /* Cleanup before main return. */
    TTF_CloseFont(sdlstate.font);
    SDL_DestroyTexture(sdlstate.texture);
    SDL_DestroyRenderer(sdlstate.renderer);
    SDL_DestroyWindow(sdlstate.window);
    TTF_Quit();
    SDL_Quit();
    fclose(prof_file);

    return 0;
}

int main(void)
{
    return run();
}

