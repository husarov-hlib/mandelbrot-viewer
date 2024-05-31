#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>

#include "color.h"

int height = 0;
int width = 0;

float scale = 0;
float center_y = 0;
float center_x = 0;

int max_iters = 512;
int bailout2 = 1<<16; // bailout radius squared
int box_size = 1024;
int min_box_size = 4;

float **iters = NULL;

int max(int a, int b) { return (a > b ? a : b); }
int min(int a, int b) { return (a < b ? a : b); }

float calc_iters(int y, int x) {
    float cy = scale * (float) (y - height/2) + center_y;
    float cx = scale * (float) (x - width/2) + center_x;
    float zy = 0;
    float zx = 0;
    float zx2 = 0;
    float zy2 = 0;

    int i = 0;
    while ((i < max_iters) && (zx2 + zy2 <= bailout2)) {
        zy = 2 * zx * zy + cy;
        zx = zx2 - zy2 + cx;
        zx2 = zx * zx;
        zy2 = zy * zy;
        i++;
    }
    
    if (i != max_iters) {
        return (float) i - log2f(log2f(zx2+zy2)) + 2.0F;
    } else {
        return (float) i;
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

int main() {
    height = 2049;
    width = 2049;
    center_y = 0;
    center_x = -0.55;
    scale = 0.001;
//    center_y = 0.63844;
//    center_x = 0.16125;
//    scale = .00001;

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
            for (int i = x; i <= end_x; i++) { iters[end_y][i] = calc_iters(end_y,i); }
            for (int i = y; i < end_y; i++) { iters[i][end_x] = calc_iters(i,end_x); }
            calc_box(y, x, end_y, end_x);
        }
    }

    FILE *fp;
    fp = fopen("image.pbm", "w");
    draw_frame(fp);
    fclose(fp);

    return 0;
}
