//
//modified by: Eduardo Barajas
//date: 1/25/2018
//
//3350 Spring 2018 Lab-1
//This program demonstrates the use of OpenGL and XWindows
//
//Assignment is to modify this program.
//You will follow along with your instructor.
//
//Elements to be learned in this lab...
// .general animation framework
// .animation loop
// .object definition and movement
// .collision detection
// .mouse/keyboard interaction
// .object constructor
// .coding style
// .defined constants
// .use of static variables
// .dynamic memory allocation
// .simple opengl components
// .git
//
//elements we will add to program...
//   .Game constructor
//   .multiple particles
//   .gravity
//   .collision detection
//   .more objects
//
#include <iostream>
using namespace std;
#include <stdio.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <unistd.h>
#include <cmath>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include <unistd.h>
#include "fonts.h"

const int MAX_PARTICLES = 2000;
const int MAX_BOXES = 5;  
const float GRAVITY = 0.1;

//some structures

struct Vec {
	float x, y, z;
};

struct Shape {
	float width, height;
	float radius;
	Vec center;
};

struct Particle {
	Shape s;
	Vec velocity;
};

class Global {
public:
	int xres, yres;
	Shape box[MAX_BOXES];
	Particle particle[MAX_PARTICLES];
	int n;
	Global() {
		xres = 800;
		yres = 600;
		//define a box shape
		for(int i = 0; i < MAX_BOXES; i++)
		{
			box[i].width = 100;
			box[i].height = 10;
			box[i].center.x = 120 + (i+1)*65;
			box[i].center.y = 500 - (i+1)*60;
		}
		n = 0;
	}
} g;

class X11_wrapper {
private:
	Display *dpy;
	Window win;
	GLXContext glc;
    GC gc;
public:
	~X11_wrapper() {
		XDestroyWindow(dpy, win);
		XCloseDisplay(dpy);
	}
	X11_wrapper() {
		GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
		int w = g.xres, h = g.yres;
		dpy = XOpenDisplay(NULL);
		if (dpy == NULL) {
			cout << "\n\tcannot connect to X server\n" << endl;
			exit(EXIT_FAILURE);
		}
		Window root = DefaultRootWindow(dpy);
		XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
		if (vi == NULL) {
			cout << "\n\tno appropriate visual found\n" << endl;
			exit(EXIT_FAILURE);
		} 
		Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
		XSetWindowAttributes swa;
		swa.colormap = cmap;
		swa.event_mask =
			ExposureMask | KeyPressMask | KeyReleaseMask |
			ButtonPress | ButtonReleaseMask |
			PointerMotionMask |
			StructureNotifyMask | SubstructureNotifyMask;
		win = XCreateWindow(dpy, root, 0, 0, w, h, 0, vi->depth,
			InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
		set_title();
		glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
		glXMakeCurrent(dpy, win, glc);
	}
	void set_title() {
		//Set the window title bar.
		XMapWindow(dpy, win);
		XStoreName(dpy, win, "3350 Lab1");
	}
    void clear_screen()
    {
        XClearWindow(dpy, win);
    }
    void setColor3i(int r, int g, int b)
    {
        unsigned long cref = 0L;
        cref += r;
        cref <<= 8;
        cref += g;
        cref <<= 8;
        cref += b;
        XSetForeground(dpy, gc, cref);
    }
	bool getXPending() {
		//See if there are pending events.
		return XPending(dpy);
	}
	XEvent getXNextEvent() {
		//Get a pending event.
		XEvent e;
		XNextEvent(dpy, &e);
		return e;
	}
	void swapBuffers() {
		glXSwapBuffers(dpy, win);
	}
    void drawText(int x, int y, const char *text)
    {
        XDrawString(dpy, win, gc, x, y, text, strlen(text));
    }
} x11;

//Function prototypes
void init_opengl(void);
void check_mouse(XEvent *e);
int check_keys(XEvent *e);
void movement();
void render();



//=====================================
// MAIN FUNCTION IS HERE
//=====================================
int main()
{
	srand(time(NULL));
	init_opengl();
	//Main animation loop
	int done = 0;
	while (!done) {
		//Process external events.
		while (x11.getXPending()) {
			XEvent e = x11.getXNextEvent();
			check_mouse(&e);
			done = check_keys(&e);
		}
		movement();
		render();
		x11.swapBuffers();
	}
    cleanup_fonts();
	return 0;
}

void init_opengl(void)
{
	//OpenGL initialization
	glViewport(0, 0, g.xres, g.yres);
	//Initialize matrices
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	//Set 2D mode (no perspective)
	glOrtho(0, g.xres, 0, g.yres, -1, 1);
	//Set the screen background color
	glClearColor(0.1, 0.1, 0.1, 1.0);

    //allow fonts
    glEnable(GL_TEXTURE_2D);
    initialize_fonts();
}

void makeParticle(int x, int y)
{
	if (g.n >= MAX_PARTICLES)
		return;
	cout << "makeParticle() " << x << " " << y << endl;
	//position of particle
	Particle *p = &g.particle[g.n];
	p->s.center.x = x;
	p->s.center.y = y;
	p->velocity.y = (float)rand()/(float)RAND_MAX;
	p->velocity.x = (float)rand()/(float)RAND_MAX;
	++g.n;
}

void check_mouse(XEvent *e)
{
	static int savex = 0;
	static int savey = 0;

	if (e->type != ButtonRelease &&
		e->type != ButtonPress &&
		e->type != MotionNotify) {
		//This is not a mouse event that we care about.
		return;
	}
	//
	if (e->type == ButtonRelease) {
		return;
	}
	if (e->type == ButtonPress) {
		if (e->xbutton.button==1) {
			//Left button was pressed
			int y = g.yres - e->xbutton.y;
			for(int i = 0; i < 10; i++)
			{
				makeParticle(e->xbutton.x, y);
			}
			return;
		}
		if (e->xbutton.button==3) {
			//Right button was pressed
			return;
		}
	}
	if (e->type == MotionNotify) {
		//The mouse moved!
		if (savex != e->xbutton.x || savey != e->xbutton.y) {
			savex = e->xbutton.x;
			savey = e->xbutton.y;
			int y = g.yres - e->xbutton.y;
			for(int i = 0; i < 10; i++)
			{
				makeParticle(e->xbutton.x, y);
			}
		}
	}
}

int check_keys(XEvent *e)
{
	if (e->type != KeyPress && e->type != KeyRelease)
		return 0;
	int key = XLookupKeysym(&e->xkey, 0);
	if (e->type == KeyPress) {
		switch (key) {
			case XK_1:
				//Key 1 was pressed
				break;
			case XK_a:
				//Key A was pressed
				break;
			case XK_Escape:
				//Escape key was pressed
				return 1;
		}
	}
	return 0;
}

void movement()
{
	if (g.n <= 0)
		return;

	for(int i = 0; i < g.n; i++)
	{
		Particle *p = &g.particle[i];
		p->s.center.x += p->velocity.x;
		p->s.center.y += p->velocity.y;

		//add force to particle
		p->velocity.y -= GRAVITY;

		//check for collision with shapes...
		for(int j = 0; j < MAX_BOXES; j++)
		{
		Shape *s = &g.box[j];
		
		if(p->s.center.y < (s->center.y + s->height) && 
			p->s.center.y > (s->center.y - s->height) &&
			p->s.center.x > (s->center.x - s->width) &&
			p->s.center.x < (s->center.x + s->width)) 
		{
			p->velocity.y = -p->velocity.y;
			p->velocity.y *= 0.5;
		}
		}

		//check for off-screen
		if (p->s.center.y < 0.0) {
			cout << "off screen" << endl;
			g.particle[i] = g.particle[--g.n];
		}
	}

}

void render()
{
	glClear(GL_COLOR_BUFFER_BIT);
    //x11.clear_screen();
    //x11.setColor3i(256,0,0);
    Rect r;
    //r.bot = g.yres/2;
    //r.left = g.xres/2;
    //r.center = 0;
    //ggprint8b(&r, 30, 0x00ff0000, "Test");

	//Draw shapes...
    //
	//draw a box
	for(int i = 0; i < MAX_BOXES; i++)
	{
	Shape *s;
	glColor3ub(90,140,90);
	s = &g.box[i];
	glPushMatrix();
	glTranslatef(s->center.x, s->center.y, s->center.z);
	
	float w, h;
	w = s->width;
	h = s->height;
	glBegin(GL_QUADS);
		glVertex2i(-w, -h);
		glVertex2i(-w,  h);
		glVertex2i( w,  h);
		glVertex2i( w, -h);
	glEnd();
	glPopMatrix();
	//
	//Draw the particle here
	for(int i = 0; i < g.n; i++)
	{
		glPushMatrix();
		glColor3ub(150,160,220);
		Vec *c = &g.particle[i].s.center;
		w =
		h = 2;
		glBegin(GL_QUADS);
			glVertex2i(c->x-w, c->y-h);
			glVertex2i(c->x-w, c->y+h);
			glVertex2i(c->x+w, c->y+h);
			glVertex2i(c->x+w, c->y-h);
		glEnd();
		glPopMatrix();
	}
	}
	//
	//Draw your 2D text here
    //
    //Rect r;
    r.bot = g.yres - 170;
    r.left = 185;
    ggprint8b(&r, 16, 0x00ff0000, "Requirements");
    r.bot = g.yres - 230;
    r.left = 250;
    ggprint8b(&r, 16, 0x00ff0000, "Design");
    r.bot = g.yres - 290;
    r.left = 315;
    ggprint8b(&r, 16, 0x00ff0000, "Code");
    r.bot = g.yres - 350;
    r.left = 380;
    ggprint8b(&r, 16, 0x00ff0000, "testing");
    r.bot = g.yres - 410;
    r.left = 445;
    ggprint8b(&r, 16, 0x00ff0000, "Maintenance");

}






