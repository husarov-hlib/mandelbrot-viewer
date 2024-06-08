#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <getopt.h>
#include <assert.h>
#include <string.h>

#include "color.h"

#define EQ_THRESHOLD 0.0001

int height = 0;
int width = 0;

float scale = 0;
float center_y = 0;
float center_x = 0;

int max_iters = 1024;
int bailout2 = 1<<16; // bailout radius squared
int box_size = 1024;
int min_box_size = 4;

char *filename;

float *iters;

int *queue;
int *queue_tail;
bool *needed;

int max(int a, int b) { return (a > b ? a : b); }
int min(int a, int b) { return (a < b ? a : b); }

float calc_iters(int i) {
    assert(i >= 0);
    assert(i < width*height);
    //assert(iters[y][x] == -1);

    int x = i % width;
    int y = i / width;

    float old_x;
    float old_y;

    float cy = scale * (float) (y - height/2) + center_y;
    float cx = scale * (float) (x - width/2) + center_x;
    float zy = 0;
    float zx = 0;
    float zx2 = 0;
    float zy2 = 0;

    int iters = 0;
    while ((iters < max_iters) && (zx2 + zy2 <= bailout2)) {
        if (iters % 32 == 0) {
            old_x = zx;
            old_y = zy;
        }

        zy = 2 * zx * zy + cy;
        zx = zx2 - zy2 + cx;
        zx2 = zx * zx;
        zy2 = zy * zy;

        if (fabs(old_x - zx) < EQ_THRESHOLD && fabs(old_y - zy) < EQ_THRESHOLD) {
            return max_iters;
        }

        iters++;
    }
    
    if (iters != max_iters) {
        assert(fmax(0.0, (float) iters - log2f(log2f(zx2+zy2)) + 2.0F) <= max_iters);
        return fmax(0.0, (float) iters - log2f(log2f(zx2+zy2)) + 2.0F);
    } else {
        return (float) iters;
    }
}

void draw_frame(FILE *fp) {
    fprintf(fp, "P6\n%d %d 255\n", width, height);
    for (int i = 0; i < width*height; i++) {
        unsigned char rgb[3];
        iters_to_rgb(iters[i], rgb);
        fwrite(rgb, sizeof(unsigned char), 3, fp);
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
    printf("  -f, --filename=STRING filename to which the generated image will be saved\n");
}

void parse_arguments(int argc, char **argv) {
    bool width_set = false;
    bool height_set = false;
    bool center_x_set = false;
    bool center_y_set = false;
    bool scale_set = false;
    bool filename_set = false;

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
            case 'f':
                filename = optarg;
                filename_set = true;
                break;
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
    if (!filename_set) { printf("--filename missing\n"); parsing_error = true; }
    
    if (parsing_error) {
        exit(1);
    }
}

void queue_add(int i) {
    if (!needed[i]) return;
    assert(queue_tail < queue + width*height);
    *(queue_tail++) = i;
    needed[i] = false;
}

void scan(int i) {
    int x = i % width;
    int y = i / width;
    iters[i] = calc_iters(i);

    if (iters[i] == max_iters) return;

    if (y >= 1)         queue_add(i-width);
    if (x >= 1)         queue_add(i-1);
    if (x < width-1)    queue_add(i+1);
    if (y < height-1)   queue_add(i+width);
}

int main(int argc, char **argv) {
    parse_arguments(argc, argv);

    iters = malloc(sizeof(float) * width*height);
    for (int i = 0; i < width*height; i++) iters[i] = max_iters;

    queue = malloc(sizeof(int) * width*height);
    int *queue_head = queue;
    queue_tail = queue;
    needed = malloc(sizeof(bool) * width*height); // pixels that need to be calculated, i.e.
                                                  // not yet calculated or queued
    memset(needed, 1, sizeof(bool) * width*height);

    for (int y = 0; y < height; y++) { queue_add(y*width); queue_add((y+1)*width - 1); }
    for (int x = 1; x < width-1; x++) { queue_add(x); queue_add(width*(height-1) + x); }

    while (queue_head <= queue_tail) {
        assert(queue_head < queue + width*height);
        scan(*(queue_head++));
    }

    FILE *fp = fopen(filename, "w");
    gen_colormap();
    draw_frame(fp);
    fclose(fp);

    return 0;
}
