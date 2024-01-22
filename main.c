#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#define MAX_ITERS 512
#define WIDTH 2000
#define HEIGHT 2000
#define MIN_BOX_SIZE 4
#define START_BOX_HEIGHT 32
#define START_BOX_WIDTH 32
#define BAILOUT2 1<<16 // bailout radius squared

// possible double evalutation problems
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))

float iters_per_pixel[HEIGHT][WIDTH]; // iters per pixel

long int pixels_per_iter[MAX_ITERS];
float hist[MAX_ITERS];
int colormap[MAX_ITERS][3];

float scale = 0.0012;
float center_x = -0.55;
float center_y = 0;

long int total_iters = 0;
long int iters_saved = 0;


float lin_int(float y1, float y2, float xf) {
    return (y1 * (1-xf) + y2 * xf);
}

void gen_colormap() {
    for (int i = 0; i < MAX_ITERS; i++) pixels_per_iter[i] = 0;

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            pixels_per_iter[(int) iters_per_pixel[y][x]]++;
        }
    }

    float total = 0;
    for (int i = 0; i < MAX_ITERS; i++) total += pixels_per_iter[i];

    printf("total: %f\n", total);
    float subtotal = 0;
    for (int i = 0; i < MAX_ITERS; i++) {
        hist[i] = subtotal / total;
        subtotal += pixels_per_iter[i];
    }

    for (int i = 0; i < MAX_ITERS; i++) {
        float lightness = powf(hist[i], 2.2F); // hsv value
        colormap[i][0] = roundf(lightness * 255.0F);
        colormap[i][1] = roundf(lightness * 255.0F);
        colormap[i][2] = roundf(lightness * 255.0F);
    }
}


void calc_border(int y1, int x1, int y2, int x2) {
    for (int x = x1+1; x <= x2; x++) {
        int y = y2;
        iters_per_pixel[y][x] = iters(scale_x(x), scale_y(y));
    }
    for (int y = y1+1; y < y2; y++) {
        int x = x2;
        iters_per_pixel[y][x] = iters(scale_x(x), scale_y(y));
    }
}

void calc_box(int y1, int x1, int y2, int x2, bool p) {
    bool borders_in = true;

    for (int y = y1; borders_in && (y < y2); y++) {
        int x = x1;
        borders_in &= iters_per_pixel[y][x] == MAX_ITERS;
    }
    for (int x = x1; borders_in && (x < x2); x++) {
        int y = y2;
        borders_in &= iters_per_pixel[y][x] == MAX_ITERS;
    }
    for (int y = y2; borders_in && (y > y1); y--) {
        int x = x2;
        borders_in &= iters_per_pixel[y][x] == MAX_ITERS;
    }
    for (int x = x2; borders_in && (x > x1); x--) {
        int y = y1;
        borders_in &= iters_per_pixel[y][x] == MAX_ITERS;
    }

    if (borders_in) {
        iters_saved += (y2-y1-1)*(x2-x1-1)*MAX_ITERS;
        for (int y = y1+1; y < y2; y++) {
            for (int x = x1+1; x < x2; x++) {
                iters_per_pixel[y][x] = MAX_ITERS;
            }
        }
        // if (p) draw_frame();
    } else {
        int w = x2-x1;
        int h = y2-y1;
        if (max(h,w) < 2*MIN_BOX_SIZE) {
            for (int y = y1+1; y < y2; y++) {
                for (int x = x1+1; x < x2; x++) {
                    iters_per_pixel[y][x] = iters(scale_x(x), scale_y(y));
                }
            }
        } else {
            if (w > h) {
                int hx = (x1+x2)/2;
                for (int y = y1+1; y < y2; y++) {
                    iters_per_pixel[y][hx] = iters(scale_x(hx), scale_y(y));
                }
                // if (p) draw_frame();
                calc_box(y1, x1, y2, hx, p);
                calc_box(y1, hx, y2, x2, p);
            } else {
                int hy = (y1+y2)/2;
                for (int x = x1+1; x < x2; x++) {
                    iters_per_pixel[hy][x] = iters(scale_x(x), scale_y(hy));
                }
                // if (p) draw_frame();
                calc_box(y1, x1, hy, x2, p);
                calc_box(hy, x1, y2, x2, p);
            }
        }
    }
}


float iters(float cx, float cy) {
    float zx = 0;
    float zx2 = 0;
    float zy = 0;
    float zy2 = 0;

    int i = 0;
    while ((i < MAX_ITERS) && (zx2 + zy2 <= BAILOUT2)) {
        zy = 2 * zx * zy + cy;
        zx = zx2 - zy2 + cx;
        zx2 = zx * zx;
        zy2 = zy * zy;
        i++;
    }
    total_iters+=i;
    if (i != MAX_ITERS) {
        return (float) i - log2f(log2f(zx2+zy2)) + 2.0F;
    } else {
        return (float) i;
    }
}

float scale_x(float x) {
    return scale * (x - WIDTH/2.0F) + center_x;
}

float scale_y(float y) {
    return scale * (y - HEIGHT/2.0F) + center_y;
}

void draw_frame(FILE *fp) {
    fprintf(fp, "P3\n%d %d 255\n", WIDTH, HEIGHT);
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            float i = iters_per_pixel[y][x]-1;
            float whole;
            float f = modff(i, &whole);
            fprintf(fp, "%d %d %d ",
                    (int) lin_int(colormap[(int) i][0], colormap[(int) i + 1][0], f),
                    (int) lin_int(colormap[(int) i][1], colormap[(int) i + 1][1], f),
                    (int) lin_int(colormap[(int) i][2], colormap[(int) i + 1][2], f));
        }
    }
}

int main() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            iters_per_pixel[y][x] = 0;
        }
    }

    for (int x = 0; x < WIDTH; x++) {
        int y = 0;
        iters_per_pixel[0][x] = iters(scale_x(x), scale_y(y));
    }
    for (int y = 1; y < HEIGHT; y++) {
        int x = 0;
        iters_per_pixel[y][0] = iters(scale_x(x), scale_y(y));
    }

    for (int y = 0; y < HEIGHT-1; y+=START_BOX_HEIGHT) {
        for (int x = 0; x < WIDTH-1; x+=START_BOX_WIDTH) {
            int y2 = min(y+START_BOX_HEIGHT, HEIGHT-1);
            int x2 = min(x+START_BOX_WIDTH, WIDTH-1);
            calc_border(y, x, y2, x2);
            calc_box(y, x, y2, x2, false);
        }
    }

    FILE *fp;
    fp = fopen("image.pbm", "w");
    gen_colormap();
    draw_frame(fp);
    fclose(fp);

    for (int i = 0; i < MAX_ITERS; i++) {
        printf("%d %d %d\n", colormap[i][0], colormap[i][1], colormap[i][2]);
    }
    printf("%ld\n", total_iters);
    printf("saved: %ld\n", iters_saved);
    printf("saved: %.1f%%\n", (float) iters_saved*100.0F / ((float) total_iters + (float) iters_saved));
    return 0;
}
