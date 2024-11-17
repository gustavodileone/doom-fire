#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define APP_NAME "Doom - Fire Algorithm"

#define ICCCM_DELETE_WINDOW "WM_DELETE_WINDOW"

#define DEFAULT_WIDTH 666
#define DEFAULT_HEIGHT 666
#define FRAMES_PER_SECOND 66

typedef struct App {
    Display* dp;
    Window w;
    GC gc;
    Pixmap pm;
    int screen, width, height;

    Atom delete_window;
} App;

typedef struct Doom {
    unsigned long palette[36];
    short *frame;
    size_t frame_length;
} Doom;

unsigned long rgb(int r, int g, int b) {
    return b + (g << 8) + (r << 16);
}

void sleep_by_fps(int fps) {
    struct timespec time;
    time.tv_sec = 0;
    time.tv_nsec = 1000000000 / fps;

    nanosleep(&time, NULL);
}

void fill_doom(Doom* doom, int width) {
    int last_index = (sizeof(doom->palette) / sizeof(doom->palette[0])) - 1;
    for(int i = 0; i < doom->frame_length; i++) {
        doom->frame[i] = i >= (doom->frame_length - width) ? last_index : 0;
    }
}

Doom init_doom(App app) {
    Doom doom = {
        palette: { rgb(7, 7, 7), rgb(31, 7, 7), rgb(47, 15, 7), rgb(71, 15, 7), rgb(87, 23, 7), rgb(103, 31, 7), rgb(119, 31, 7), rgb(143, 39, 7), rgb(159, 47, 7), rgb(175, 63, 7), rgb(191, 71, 7), rgb(199, 71, 7), rgb(223, 79, 7), rgb(223, 87, 7), rgb(223, 87, 7), rgb(215, 95, 7), rgb(215, 103, 15), rgb(207, 111, 15), rgb(207, 119, 15), rgb(207, 127, 15), rgb(207, 135, 23), rgb(199, 135, 23), rgb(199, 143, 23), rgb(199, 151, 31), rgb(191, 159, 31), rgb(191, 159, 31), rgb(191, 167, 39), rgb(191, 167, 39), rgb(191, 175, 47), rgb(183, 175, 47), rgb(183, 183, 47), rgb(183, 183, 55), rgb(207, 207, 111), rgb(223, 223, 159), rgb(239, 239, 199), rgb(255, 255, 255) },
        frame_length: app.width * app.height,
        frame: NULL,
    };

    doom.frame = malloc(doom.frame_length * sizeof(doom.frame[0]));
    if(doom.frame == NULL) {
        fprintf(stderr, "Couldn't initialize memory for the frame.\n");
        exit(EXIT_FAILURE);
    }

    fill_doom(&doom, app.width);

    return doom;
}

void render_doom(App app, Doom doom) {
    int four_halves = app.width / 4;

    for(int y = 0; y < app.height; y++) {
        for(int x = 0; x < four_halves; x++) {
            int i = x + (y * app.width);

            int bellow = i + app.width;
            if(bellow >= doom.frame_length) continue;

            int decayed = doom.frame[bellow] - (rand() % 2);
            doom.frame[i] = decayed > 0 ? decayed : 0;

            XSetForeground(app.dp, app.gc, doom.palette[doom.frame[i]]);
            XDrawPoint(app.dp, app.pm, app.gc, x, y);
        }
    }

    for(int i = 0; i < app.width; i = i + four_halves) {
        XCopyArea(app.dp, app.pm, app.pm, app.gc, 0, 0, app.width, app.height, i, 0);
    }

    XCopyArea(app.dp, app.pm, app.w, app.gc, 0, 0, app.width, app.height, 0, 0);
}

void resize_doom(Doom* doom, int width, int height) {
    int length = width * height;

    doom->frame = realloc(doom->frame, length * sizeof(doom->frame[0]));
    if(doom->frame == NULL) {
        fprintf(stderr, "Couldn't allocate memory for the frame.");
        exit(EXIT_FAILURE);
    }

    doom->frame_length = length;

    fill_doom(doom, width);
}

void resize_app(App* app, int width, int height) {
    app->width = width;
    app->height = height;

    XFreePixmap(app->dp, app->pm);
    app->pm = XCreatePixmap(app->dp, app->w, width, height, DefaultDepth(app->dp, app->screen));
}

App init_app() {
    App app = {0};
    app.width = DEFAULT_WIDTH;
    app.height = DEFAULT_HEIGHT;

    Display* dp = XOpenDisplay(NULL);
    
    app.dp = dp;
    app.screen = DefaultScreen(dp);

    app.w = XCreateSimpleWindow(dp, DefaultRootWindow(dp), 0, 0, app.width, app.height, 0, 0, BlackPixel(dp, app.screen));
    app.gc = XCreateGC(dp, app.w, 0, NULL);
    app.pm = XCreatePixmap(dp, app.w, app.width, app.height, DefaultDepth(dp, app.screen));

    XSetStandardProperties(dp, app.w, APP_NAME, NULL, None, NULL, 0, NULL);

    XSelectInput(dp, app.w, StructureNotifyMask);
    XMapWindow(dp, app.w);

    Atom wm_delete_window = XInternAtom(dp, ICCCM_DELETE_WINDOW, False);
    XSetWMProtocols(dp, app.w, &wm_delete_window, 1);

    app.delete_window = wm_delete_window;

    return app;
}

void destroy_app(App app) {
    XCloseDisplay(app.dp);
}

int main() {
    srand(time(NULL));

    App app = init_app();
    Doom doom = init_doom(app);

    int exit_program = 0;
    while(!exit_program) {
        while(XPending(app.dp)) {
            XEvent ev;
            XNextEvent(app.dp, &ev);

            switch (ev.type) {
                case MapNotify:
                    XWindowAttributes wattr;
                    XGetWindowAttributes(app.dp, app.w, &wattr);
                    if(wattr.width == app.width && wattr.height == app.height) break;

                    resize_app(&app, wattr.width, wattr.height);
                    resize_doom(&doom, app.width, app.height);
                break;

                case ConfigureNotify:
                    if(ev.xconfigure.width == app.width && ev.xconfigure.height == app.height) break;
                    resize_app(&app, ev.xconfigure.width, ev.xconfigure.height);
                    resize_doom(&doom, app.width, app.height);
                break;

                case ClientMessage:
                    if((Atom) ev.xclient.data.l[0] == app.delete_window) exit_program = 1;
                break;
            }
        }

        render_doom(app, doom);
        sleep_by_fps(FRAMES_PER_SECOND);
    }

    destroy_app(app);
    return EXIT_SUCCESS;
}