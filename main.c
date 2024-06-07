#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <getopt.h>
#include <assert.h>

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

float **iters = NULL;

int max(int a, int b) { return (a > b ? a : b); }
int min(int a, int b) { return (a < b ? a : b); }

float calc_iters(int y, int x) {
    assert(y >= 0);
    assert(y < height);
    assert(x >= 0);
    assert(x < width);
    assert(iters[y][x] == -1);

    float old_x;
    float old_y;

    float cy = scale * (float) (y - height/2) + center_y;
    float cx = scale * (float) (x - width/2) + center_x;
    float zy = 0;
    float zx = 0;
    float zx2 = 0;
    float zy2 = 0;

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

        if (fabs(old_x - zx) < EQ_THRESHOLD && fabs(old_y - zy) < EQ_THRESHOLD) {
            return max_iters;
        }

        i++;
    }
    
    if (i != max_iters) {
        assert(fmax(0.0, (float) i - log2f(log2f(zx2+zy2)) + 2.0F) <= max_iters);
        return fmax(0.0, (float) i - log2f(log2f(zx2+zy2)) + 2.0F);
    } else {
        return (float) i;
    }
}

void calc_box(int y1, int x1, int y2, int x2) {
    bool borders_in = true;

    for (int y = y1; borders_in && (y < y2); y++) {
        int x = x1;
        borders_in &= iters[y][x] == max_iters;
    }
    for (int x = x1; borders_in && (x < x2); x++) {
        int y = y2;
        borders_in &= iters[y][x] == max_iters;
    }
    for (int y = y2; borders_in && (y > y1); y--) {
        int x = x2;
        borders_in &= iters[y][x] == max_iters;
    }
    for (int x = x2; borders_in && (x > x1); x--) {
        int y = y1;
        borders_in &= iters[y][x] == max_iters;
    }

    if (borders_in) {
        for (int y = y1+1; y < y2; y++) {
            for (int x = x1+1; x < x2; x++) {
                iters[y][x] = max_iters;
            }
        }
    } else {
        int w = x2-x1;
        int h = y2-y1;
        if (max(h,w) < 2*min_box_size) {
            for (int y = y1+1; y < y2; y++) {
                for (int x = x1+1; x < x2; x++) {
                    iters[y][x] = calc_iters(y,x);
                }
            }
        } else {
            if (w > h) {
                int hx = (x1+x2)/2;
                for (int y = y1+1; y < y2; y++) {
                    iters[y][hx] = calc_iters(y,hx);
                }
                calc_box(y1, x1, y2, hx);
                calc_box(y1, hx, y2, x2);
            } else {
                int hy = (y1+y2)/2;
                for (int x = x1+1; x < x2; x++) {
                    iters[hy][x] = calc_iters(hy,x);
                }
                calc_box(y1, x1, hy, x2);
                calc_box(hy, x1, y2, x2);
            }
        }
    }
}

void draw_frame(FILE *fp) {
    fprintf(fp, "P6\n%d %d 255\n", width, height);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float i = iters[y][x];
            unsigned char rgb[3] = {0,0,0};
            iters_to_rgb(i, rgb);
            fwrite(rgb, sizeof(unsigned char), 3, fp);
        }
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

int main(int argc, char **argv) {
    parse_arguments(argc, argv);

    iters = (float **) malloc(sizeof(float *) * height);
    for (int y = 0; y < height; y++) {
        iters[y] = (float *) malloc(sizeof(float) * width);
        for (int x = 0; x < width; x++) {
            iters[y][x] = -1.0;
        }
    }

    for (int y = 0; y < height; y++) { iters[y][0] = calc_iters(y,0); } // calculating left border
    for (int x = 1; x < width; x++) { iters[0][x] = calc_iters(0,x); } // calculating top border
    
    for (int y = 0; y < height-1; y+=box_size) {
        for (int x = 0; x < width-1; x+=box_size) {
            int end_y = min(y+box_size, height-1);
            int end_x = min(x+box_size, width-1);
            for (int i = x+1; i <= end_x; i++) { iters[end_y][i] = calc_iters(end_y,i); }
            for (int i = y+1; i < end_y; i++) { iters[i][end_x] = calc_iters(i,end_x); }
            calc_box(y, x, end_y, end_x);
        }
    }

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            assert(iters[y][x] >= 0.0);
        }
    }

    FILE *fp = fopen(filename, "w");
    gen_colormap();
    draw_frame(fp);
    fclose(fp);

    return 0;
}
