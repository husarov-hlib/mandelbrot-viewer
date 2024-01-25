#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>

#include "mandelbrot.h"
#include "color.h"

// possible double evalutation problems
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

int height = 0;
int width = 0;

float scale = 0;
float center_y = 0;
float center_x = 0;

int max_iters = 512;
int box_height = 32;
int box_width = 32;
int min_box_size = 4;
int bailout2 = 1<<16; // bailout radius squared

float **iters = NULL;

void draw_frame(FILE *fp, float **colormap) {
    fprintf(fp, "P3\n%d %d 255\n", width, height);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float i = iters[y][x];
            int less = floor(i);
            int more = min(ceil(i), max_iters-1);
            float frac = fmod(i, 1.0F);
//            printf("%d %d %d ",
//                    lin_int(colormap[less][0], colormap[more][0], frac),
//                    lin_int(colormap[less][1], colormap[more][1], frac),
//                    lin_int(colormap[less][2], colormap[more][2], frac));
            fprintf(fp, "%d %d %d ",
                    lin_int(colormap[less][0], colormap[more][0], frac),
                    lin_int(colormap[less][1], colormap[more][1], frac),
                    lin_int(colormap[less][2], colormap[more][2], frac));
        }
    }
}

int main() {
    height = 3000;
    width = 3000;
    center_y = 0;
    center_x = -0.55;
    scale = 0.001;

    iters = (float **) malloc(sizeof(float *) * height);
    for (int y = 0; y < height; y++) {
        iters[y] = (float *) malloc(sizeof(float) * width);
    }

    calc_frame(0, 0, height-1, width-1, iters);

    float **colormap = gen_colormap(iters);

    FILE *fp;
    fp = fopen("image.pbm", "w");
    draw_frame(fp, colormap);
    fclose(fp);

    return 0;
}
