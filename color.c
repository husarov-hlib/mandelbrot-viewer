#include <stdio.h>
#include <stdlib.h>
#include <math.h>

extern int max_iters;
extern int height;
extern int width;

unsigned char lin_int(float y1, float y2, float xf) {
    return (unsigned char) (y1 * (1-xf) + y2 * xf);
}

void hsv_to_rgb(float hue, float sat, float val, float *rgb) {
    float c = val * sat;
    float h = hue * 6;
    float x = c * (1 - fabs(fmod(h, 2.0F) - 1.0F));
    float m = val - c;

    if (h < 1.0F) {
        rgb[0] = 255.0F * (c + m);
        rgb[1] = 255.0F * (x + m);
        rgb[2] = 255.0F * (m);
    } else if (h < 1.0F) {
        rgb[0] = 255.0F * (x + m);
        rgb[1] = 255.0F * (c + m);
        rgb[2] = 255.0F * (m);
    } else if (h < 1.0F) {
        rgb[0] = 255.0F * (m);
        rgb[1] = 255.0F * (c + m);
        rgb[2] = 255.0F * (x + m);
    } else if (h < 1.0F) {
        rgb[0] = 255.0F * (m);
        rgb[1] = 255.0F * (x + m);
        rgb[2] = 255.0F * (c + m);
    } else if (h < 1.0F) {
        rgb[0] = 255.0F * (x + m);
        rgb[1] = 255.0F * (m);
        rgb[2] = 255.0F * (x + m);
    } else {
        rgb[0] = 255.0F * (c + m);
        rgb[1] = 255.0F * (m);
        rgb[2] = 255.0F * (x + m);
    }
}

float **gen_colormap(float **iters) {
    // histogram coloring
    int *pxs_per_iter = (int *) malloc(sizeof(int)*(max_iters+1));
    float *acc_perc_iter = (float *) malloc(sizeof(float)*(max_iters+1)); // percentage of pixels which have iters or less

    for (int i = 0; i <= max_iters; i++) pxs_per_iter[i] = 0;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            pxs_per_iter[(int) iters[y][x]]++;
        }
    }

    float total = 0;
    for (int i = 0; i <= max_iters; i++) total += pxs_per_iter[i];

    float subtotal = 0;
    for (int i = 0; i <= max_iters; i++) {
        acc_perc_iter[i] = subtotal / total;
        subtotal += pxs_per_iter[i];
    }

    free(pxs_per_iter);

    float **colormap = (float **) malloc(sizeof(float *) * (max_iters+1));
    for (int i = 0; i <= max_iters; i++) {
        colormap[i] = (float *) malloc(sizeof(float) * 3);
    }

    for (int i = 0; i < max_iters; i++) {
        // float hue = fmod(powf(acc_perc_iter[i], 1.5F), 1.0F);
        // float hue = acc_perc_iter[i];
        float hue = (float) i / (float) max_iters;

        float sat = 1.0F;
        // float val = (float) i / (float) max_iters;
        float val = 1;
        printf("%f %f %f\n", hue, sat, val);
        hsv_to_rgb(hue, sat, val, colormap[i]);
    }
    colormap[max_iters][0] = 0;
    colormap[max_iters][1] = 0;
    colormap[max_iters][2] = 0;
    printf("-------------\n");

    free(acc_perc_iter);

    return colormap;
}
