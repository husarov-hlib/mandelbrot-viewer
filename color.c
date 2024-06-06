#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "color.h"

extern int max_iters;
extern int height;
extern int width;

#define COLORMAP_SIZE 2048

typedef struct {
    float x;
    rgb color;
} grad_point;

rgb colormap[COLORMAP_SIZE];
grad_point grad_points[5] = {
    {0.0,       {0,   7,   100}},
    {0.16,      {32,  107, 203}},
    {0.42,      {237, 255, 255}},
    {0.6425,    {255, 170, 0}},
    {0.8575,    {0,   2,   0}},
};

int grad_size = 5;

rgb max_iter_color = {0,0,0};

void gen_colormap() {
    float ds[grad_size][3]; // rates of change of each color channel
    float dx;
    for (int i = 0; i < grad_size; i++) {
        grad_point p1 = grad_points[i % grad_size];
        grad_point p2 = grad_points[(i+1) % grad_size];
        dx = p2.x - p1.x;
        if (dx < 0.0) dx += 1;
        assert(dx >= 0);

        for (int k = 0; k < 3; k++) {
            ds[i][k] = (p2.color[k] - p1.color[k]) / dx;
        }
    }

    int p_i = 0; // index of the gradient point from which the current piece
                 // of the colormap is interpolated
    for (int i = 0; i < COLORMAP_SIZE; i++) {
        float x = (float) i / (float) COLORMAP_SIZE;

        if (p_i < grad_size-1 && x > grad_points[p_i+1].x) p_i++;
        assert(p_i < grad_size);

        grad_point p = grad_points[p_i]; // the gradient point from which the
                                         // current piece of colormap is interpolated
        for (int k = 0; k < 3; k++) {
            colormap[i][k] = p.color[k] + (x - p.x) * ds[p_i][k];
        }
    }
}

int mod(int num, int base) { return num < 0 ? base + (num%base) : num % base; }

void iters_to_rgb(float iters, rgb color) {
    if (iters == max_iters) {
        color[0] = max_iter_color[0];
        color[1] = max_iter_color[1];
        color[2] = max_iter_color[2];
        return;
    }

    float log = log2f(fmax(0.125, iters));
    int index = mod( ((log * 0.13 - 0.2) * COLORMAP_SIZE), COLORMAP_SIZE );

    assert(index >= 0);
    assert(index < COLORMAP_SIZE);

    color[0] = colormap[index][0];
    color[1] = colormap[index][1];
    color[2] = colormap[index][2];
}
