#include <math.h>
#include <stdbool.h>

extern int max_iters;
extern int bailout2;

extern int box_height;
extern int box_width;
extern int min_box_size;

extern float **iters;
extern int height;
extern int width;
extern float scale;
extern float center_y;
extern float center_x;

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

void calc_border(int y1, int x1, int y2, int x2) {
    for (int x = x1+1; x <= x2; x++) {
        int y = y2;
        iters[y][x] = calc_iters(y,x);
    }
    for (int y = y1+1; y < y2; y++) {
        int x = x2;
        iters[y][x] = calc_iters(y,x);
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

void calc_frame(int y1, int x1, int y2, int x2) {
    // calculating the top and left borders of the whole frame because the calc_box function
    // only calculates the right and bottom borders
    for (int y = y1; y <= y2; y++) { iters[y][x1] = calc_iters(y,x1); } // calculating left border
    for (int x = x1+1; x <= x2; x++) { iters[x][y1] = calc_iters(y1,x); } // calculating top border
    
    for (int y = y1; y < y2; y+=box_height) {
        for (int x = x1; x < x2; x+=box_width) {
            int end_y = min(y+box_height, y2);
            int end_x = min(x+box_width, x2);
            calc_border(y, x, end_y, end_x);
            calc_box(y, x, end_y, end_x);
        }
    }
}
