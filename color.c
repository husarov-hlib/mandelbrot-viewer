#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "color.h"

extern int max_iters;
extern int height;
extern int width;

#define COLORMAP_SIZE 2048

unsigned int colormap[COLORMAP_SIZE] = {0};
float grad_points[5][4] = {
    {0.0,       0,   7,   100},
    {0.16,      32,  107, 203},
    {0.42,      237, 255, 255},
    {0.6425,    255, 170, 0},
    {0.8575,    0,   2,   0},
};

int grad_size = 5;

rgb max_iter_color = {0,0,0};

void gen_colormap() {
    float ds[grad_size][3]; // rates of change of each color channel
    float dx;
    for (int i = 0; i < grad_size; i++) {
        float *p1 = grad_points[i % grad_size];
        float *p2 = grad_points[(i+1) % grad_size];
        dx = p2[0] - p1[0];
        if (dx < 0.0) dx += 1;
        assert(dx >= 0);

        for (int k = 0; k < 3; k++)
            ds[i][k] = (p2[k+1] - p1[k+1]) / dx;
    }

    int p_i = 0; // index of the gradient point from which the current piece
                 // of the colormap is interpolated
    for (int i = 0; i < COLORMAP_SIZE; i++) {
        float x = (float) i / (float) COLORMAP_SIZE;

        if (p_i < grad_size-1 && x > grad_points[p_i+1][0]) p_i++;
        assert(p_i < grad_size);

        float *p = grad_points[p_i]; // the gradient point from which the
                                         // current piece of colormap is interpolated
        for (int k = 0; k < 3; k++) {
            colormap[i] <<= 8;
            colormap[i] += p[k+1] + (x - p[0]) * ds[p_i][k];
        }
    }
}

int mod(int num, int base) { return num < 0 ? base + (num%base) : num % base; }

unsigned int iters_to_rgb(float iters) {
    if (iters == max_iters)
        return max_iter_color[0] * (2<<18) + max_iter_color[1] * (2<<9) + max_iter_color[2];

    float log = log2f(fmax(0.125, iters));
    int index = mod( ((log * 0.13 - 0.2) * COLORMAP_SIZE), COLORMAP_SIZE );

    assert(index >= 0);
    assert(index < COLORMAP_SIZE);

    return colormap[index];
}
