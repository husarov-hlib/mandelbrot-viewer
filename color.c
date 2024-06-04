#include <stdio.h>
#include <stdlib.h>
#include <math.h>

extern int max_iters;
extern int height;
extern int width;

unsigned char colormap[16][3] = {
    {66, 30, 15},
    {25, 7, 26},
    {9, 1, 47},
    {4, 4, 73},
    {0, 7, 100},
    {12, 44, 138},
    {24, 82, 177},
    {57, 125, 209},
    {134, 181, 229},
    {211, 236, 248},
    {241, 233, 191},
    {248, 201, 95},
    {255, 170, 0},
    {204, 128, 0},
    {153, 87, 0},
    {106, 52, 3}
};

int colormap_size = 16;

unsigned char max_iter_color[3] = {0,0,0};

unsigned char lin_int(float y1, float y2, float xf) {
    return (unsigned char) (y1 * (1-xf) + y2 * xf);
}

void iters_to_rgb(float iters, unsigned char *rgb) {
    if (iters == max_iters) {
        rgb[0] = max_iter_color[0];
        rgb[1] = max_iter_color[1];
        rgb[2] = max_iter_color[2];
        return;
    }

    float log = log2f(fmax(0.125, iters)) * colormap_size * 0.1;

    int before = (int) floor(log) % colormap_size;
    int after = (int) ceil(log) % colormap_size;
    float frac = log - floor(log);
    rgb[0] = lin_int(colormap[before][0], colormap[after][0], frac);
    rgb[1] = lin_int(colormap[before][1], colormap[after][1], frac);
    rgb[2] = lin_int(colormap[before][2], colormap[after][2], frac);
}
