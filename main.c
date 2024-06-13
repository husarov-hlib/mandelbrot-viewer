#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <getopt.h>
#include <assert.h>
#include <unistd.h>

#include <SDL2/SDL.h>

#include "color.h"

#define EQ_THRESHOLD 0.0001

int max_iters = 1024;
int bailout2 = 1<<16; // bailout radius squared

float *iters;
bool *touched; // pixels queued or already calculated
int *queue, *queue_head, *queue_tail;

int height = 0;
int width = 0;

double scale = 0; // size, i.e. width or height of pixel
double center_y = 0;
double center_x = 0;

extern unsigned int colormap[2048];

void calc_iters(int index) {
    assert(index >= 0);
    assert(index < width*height);

    int x = index % width;
    int y = index / width;

    double cy = scale * (double) (y - height/2) + center_y;
    double cx = scale * (double) (x - width/2) + center_x;
    double zy = 0;
    double zx = 0;
    double zx2 = 0;
    double zy2 = 0;

    double old_x;
    double old_y;

    int i = 0;
    while ((i < max_iters) && (zx2 + zy2 <= bailout2)) {
        if (i % 32 == 0) {
            old_x = zx;
            old_y = zy;
        }

        zy = 2 * zx * zy + cy;
        zx = zx2 - zy2 + cx;
        zx2 = zx * zx;
        zy2 = zy * zy;

        if (fabs(old_x - zx) < EQ_THRESHOLD && fabs(old_y - zy) < EQ_THRESHOLD)
            return;

        i++;
    }
    
    if (i != max_iters) {
        assert(fmax(0.0, (double) i - log2f(log2f(zx2+zy2)) + 2.0F) <= max_iters);
        iters[index] = fmax(0.0, (double) i - log2f(log2f(zx2+zy2)) + 2.0F);
    } else {
        iters[index] = i;
    }
}

void print_help() {
    printf("  -?, --help            shows this help text\n");
    printf("\n");
    printf("required:\n");
    printf("  -w, --width=NUMBER    width of the generated image\n");
    printf("  -h, --height=NUMBER   height of the generated image\n");
    printf("  -x=FLOAT              the x coordinate(real part) of the center\n");
    printf("  -y=FLOAT              the y coordinate(imaginary part) of the center\n");
    printf("  -s, --scale=FLOAT     width and height of a single pixel\n");
    //printf("  -f, --filename=STRING filename to which the generated image will be saved\n");
}

void parse_arguments(int argc, char **argv, char **filename) {
    bool width_set = false;
    bool height_set = false;
    bool center_x_set = false;
    bool center_y_set = false;
    bool scale_set = false;
    //bool filename_set = false;

    struct option long_options[] = {
        {"help",    no_argument,        NULL,   '?' },
        {"width",   required_argument,  NULL,   'w' },
        {"height",  required_argument,  NULL,   'h' },
        {"scale",   required_argument,  NULL,   's' },
        {"filename",required_argument,  NULL,   'f' },
        {NULL,      0,                  NULL,   0   }
    };

    bool parsing_error = false;
    int opt;
    while ((opt = getopt_long(argc, argv, "w:h:x:y:s:f:?", long_options, NULL)) != -1) {
        switch(opt) {
            char *endptr;
            case 'w':
                width = strtol(optarg, &endptr, 10);
                if (endptr == optarg) { printf("%s is not a number\n", optarg); parsing_error = true; }
                width_set = true;
                break;
            case 'h':
                height = strtol(optarg, NULL, 10);
                if (endptr == optarg) { printf("%s is not a number\n", optarg); parsing_error = true; }
                height_set = true;
                break;
            case 'x':
                center_x = strtof(optarg, NULL);
                if (endptr == optarg) { printf("%s is not a number\n", optarg); parsing_error = true; }
                center_x_set = true;
                break;
            case 'y':
                center_y = strtof(optarg, NULL);
                if (endptr == optarg) { printf("%s is not a number\n", optarg); parsing_error = true; }
                center_y_set = true;
                break;
            case 's':
                scale = strtof(optarg, NULL);
                if (endptr == optarg) { printf("%s is not a number\n", optarg); parsing_error = true; }
                scale_set = true;
                break;
//            case 'f':
//                *filename = optarg;
//                filename_set = true;
//                break;
            case '?':
                print_help();
                exit(1);
                break;
        }
    }

    if (!width_set) { printf("--width missing\n"); parsing_error = true; }
    if (!height_set) { printf("--height missing\n"); parsing_error = true; }
    if (!scale_set) { printf("--scale missing\n"); parsing_error = true; }
    if (!center_x_set) { printf("-x missing\n"); parsing_error = true; }
    if (!center_y_set) { printf("-y missing\n"); parsing_error = true; }
//    if (!filename_set) { printf("--filename missing\n"); parsing_error = true; }
    
    if (parsing_error) {
        print_help();
        exit(1);
    }
}

void queue_add(int i, int **tail) {
    // if queued or already calculated don't add to the queue
    if (touched[i]) return;

    **tail = i;
    (*tail)++;
    touched[i] = true;
}

void scan(int i, int **queue_tail) {
    int x = i % width;
    int y = i / width;
    calc_iters(i);

    if (iters[i] == max_iters) return;

    if (y >= 1)         queue_add(i-width, queue_tail);
    if (x >= 1)         queue_add(i-1, queue_tail);
    if (x < width-1)    queue_add(i+1, queue_tail);
    if (y < height-1)   queue_add(i+width, queue_tail);
}

void calc_all_iters() {
    for (int i = 0; i < width*height; i++) iters[i] = max_iters;
    for (int i = 0; i < width*height; i++) touched[i] = false;
    queue_head = queue_tail = queue;

    // calculating borders
    for (int y = 0; y < height; y++) {
        queue_add(y*width, &queue_tail);
        queue_add((y+1)*width - 1, &queue_tail);
    }
    for (int x = 1; x < width-1; x++) {
        queue_add(x, &queue_tail);
        queue_add(width*(height-1) + x, &queue_tail);
    }

    while (queue_head <= queue_tail) { scan(*(queue_head++), &queue_tail); }
}

int main(int argc, char **argv) {
    char *filename;
    parse_arguments(argc, argv, &filename);

    iters = malloc(sizeof(float) * width*height);
    touched = malloc(sizeof(bool) * width*height);
    queue = malloc(sizeof(int) * width*height);

    unsigned int *pixels = malloc(sizeof(unsigned int) * width*height);
    for (int i = 0; i < width*height; i++) pixels[i] = 0;

    gen_colormap();

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *texture = NULL;

    window = SDL_CreateWindow(
            "mandelbrot",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            width,
            height,
            SDL_WINDOW_SHOWN
    );

    renderer = SDL_CreateRenderer(window, -1, 0);

    texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING,
            width,
            height
    );

    calc_all_iters();
    for (int i = 0; i < width*height; i++) pixels[i] = iters_to_rgb(iters[i]);

    SDL_UpdateTexture(texture, NULL, pixels, width * 4);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    bool quit = false;
    SDL_Event event;
    while (!quit) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);

        int dx = 0;
        int dy = 0;
        bool redraw_needed = false;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    quit = true;
                    break;

                case SDL_MOUSEMOTION:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        dx += event.motion.xrel;
                        dy += event.motion.yrel;
                        redraw_needed = true;
                    }

                case SDL_MOUSEWHEEL:
                    if (!isnan(event.wheel.preciseY)) {
                        int x, y;
                        SDL_GetMouseState(&x, &y);

                        double zoom = pow(1.05, event.wheel.preciseY);
                        center_x += (x - (double) width/2) * scale * (1-zoom);
                        center_y += (y - (double) height/2) * scale * (1-zoom);
                        scale *= zoom;
                        redraw_needed = true;
                    }
            }
        }

        SDL_Delay(200);

        if (!redraw_needed) continue;

        center_x -= (double) dx * scale;
        center_y -= (double) dy * scale;

        calc_all_iters();
        for (int i = 0; i < width*height; i++) pixels[i] = iters_to_rgb(iters[i]);

        SDL_UpdateTexture(texture, NULL, pixels, width * 4);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();

    free(iters);
    free(touched);
    free(queue);

    return 0;
}
