#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef struct Program {
    Display* dp;
    Window w;
    int screen, x, y, width, height;
    GC gc;
} Program;

typedef struct DoomFire {
    unsigned long palette[36];
    unsigned long *area;
    int area_size;
} DoomFire;

unsigned long RGB(int r, int g, int b) {
    return b + (g << 8) + (r << 16);
}

DoomFire init_doom_fire(int width, int height) {
    DoomFire f;
    f.palette[0] = RGB(0, 0, 0);
    f.palette[1] = RGB(31, 7, 7);
    f.palette[2] = RGB(47, 15, 7);
    f.palette[3] = RGB(71, 15, 7);
    f.palette[4] = RGB(87, 23, 7);
    f.palette[5] = RGB(103, 31, 7);
    f.palette[6] = RGB(119, 31, 7);
    f.palette[7] = RGB(143, 39, 7);
    f.palette[8] = RGB(159, 47, 7);
    f.palette[9] = RGB(175, 63, 7);
    f.palette[10] = RGB(191, 71, 7);
    f.palette[11] = RGB(199, 71, 7);
    f.palette[12] = RGB(223, 79, 7);
    f.palette[13] = RGB(223, 87, 7);
    f.palette[14] = RGB(223, 87, 7);
    f.palette[15] = RGB(215, 95, 7);
    f.palette[16] = RGB(215, 103, 15);
    f.palette[17] = RGB(207, 111, 15);
    f.palette[18] = RGB(207, 119, 15);
    f.palette[19] = RGB(207, 127, 15);
    f.palette[20] = RGB(207, 135, 23);
    f.palette[21] = RGB(199, 135, 23);
    f.palette[22] = RGB(199, 143, 23);
    f.palette[23] = RGB(199, 151, 31);
    f.palette[24] = RGB(191, 159, 31);
    f.palette[25] = RGB(191, 159, 31);
    f.palette[26] = RGB(191, 167, 39);
    f.palette[27] = RGB(191, 167, 39);
    f.palette[28] = RGB(191, 175, 47);
    f.palette[29] = RGB(183, 175, 47);
    f.palette[30] = RGB(183, 183, 47);
    f.palette[31] = RGB(183, 183, 55);
    f.palette[32] = RGB(207, 207, 111);
    f.palette[33] = RGB(223, 223, 159);
    f.palette[34] = RGB(239, 239, 199);
    f.palette[35] = RGB(255, 255, 255);

    f.area_size = width * height;
    f.area = malloc(sizeof(unsigned long) * f.area_size);
    for(int i = 0; i < f.area_size; i++) {
        f.area[i] = i >= (f.area_size - width) ? 35 : 0;
    }

    return f;
}

void render_doom_fire(DoomFire* f, Program* p) {
    for(int y = 0; y < p->height; y++) {
        for(int x = 0; x < p->width; x++) {
            int current_index = x + (p->width * y);
            int bellow_index = current_index + p->width;
            if(bellow_index >= p->width * p->height)
                continue;

            int decay = f->area[bellow_index] - (rand() % 3);
            f->area[current_index] = decay >= 0 ? decay : 0;

            XFreeGC(p->dp, p->gc);
            p->gc = XCreateGC(p->dp, p->w, 0, NULL);
            XSetForeground(p->dp, p->gc, f->palette[f->area[current_index]]);
            XDrawPoint(p->dp, p->w, p->gc, x, y);
            XFlush(p->dp);
        }
    }
}

void update_program_attr(Program* p, XWindowAttributes* attr) {
    p->width = attr->width;
    p->height = attr->height;
    p->x = attr->x;
    p->y = attr->y;
}

Program init_program() {
    Program p;
    p.dp = XOpenDisplay(NULL);
    assert(p.dp);

    p.screen = XDefaultScreen(p.dp);

    p.x = 0, p.y = 0;
    p.width = 500, p.height = 500;

    p.w = XCreateSimpleWindow(p.dp, XDefaultRootWindow(p.dp), p.x, p.y, p.width, p.height, 0, BlackPixel(p.dp, p.screen), BlackPixel(p.dp, p.screen));
    XSetStandardProperties(p.dp, p.w, "Olá mundo", NULL, None, NULL, 0, NULL);
    XSelectInput(p.dp, p.w, StructureNotifyMask | KeyPressMask);
    XMapWindow(p.dp, p.w);

    p.gc = XCreateGC(p.dp, p.w, 0, NULL);
    XSetForeground(p.dp, p.gc, RGB(255, 0, 0));

    return p;
}

void close_program(Program p) {
    XFreeGC(p.dp, p.gc);
    XDestroyWindow(p.dp, p.w);
    XCloseDisplay(p.dp);

    exit(EXIT_SUCCESS);
}

int main() {
    srand(time(NULL));
    Program p = init_program();

    XEvent e;
    while(1) {
        XNextEvent(p.dp, &e);
        if(e.type == MapNotify) break;
    }

    XWindowAttributes attr;
    XGetWindowAttributes(p.dp, p.w, &attr);
    update_program_attr(&p, &attr);

    p.width = 250;
    p.height = 250;

    DoomFire f = init_doom_fire(p.width, p.height);

    while(1) {
        // XNextEvent(p.dp, &e);
        // if(e.xkey.keycode == 'q') close_program(p);

        render_doom_fire(&f, &p);
    }

    return EXIT_SUCCESS;
}