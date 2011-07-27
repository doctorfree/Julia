/*************************************************************************
 *                                                                       *
 * Copyright (c) 1992-2011 Ronald Joe Record                           *
 *                                                                       *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************/

/* Julia - calculate and display Julia sets */
/*		or when invoked as mandel */
/* Mandel - calculate and display Mandelbrot sets */

/* Written by Ronald Record (rr@ronrecord.com) 18 Mar 1993 */

#include <math.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <values.h>
#include "x.h"
#include <X11/cursorfont.h>
#include "defines.h"
#include "libXrr.h"
#include "julia.h"
#include "patchlevel.h"

static char *version = JULIA_VERSION;
int button = 0, xpos = -1, ypos = -1, autodive = 0, dive_index = 0;
static int dflag = 1, do_redraw=1, simple=1, usedefault=0;
FILE *infile;

extern void BufferPoint(), InitBuffer(), FlushBuffer();

main(ac, av)
    int ac;
    char **av;
{
	char *cmdnam;
	char *cut;
    int i, j;
    XSizeHints hint;
	XVisualInfo *visual_list, visual_template;
	Cursor cursor;
    extern void init_canvas(), init_data(), init_color(), parseargs();
    extern void Clear(), restor_picture(); 
	extern void center_horizontal(), center_vertical();

	if ((cmdnam = strrchr(av[0], '/')) != 0) {
		path = strdup(av[0]);
		cut = strrchr(path, '/');
		cut[1] = 0;
		cmdnam++;
	}
	else
		cmdnam = av[0];
	if (!strncmp(cmdnam, "julia", 5))
		func = Funcs[0];
	else if (!strncmp(cmdnam, "mandel", 6))
		func = Funcs[1];
	else /* how did this happen ? */
		func = Funcs[0];
    parseargs(ac, av);
    dpy = XOpenDisplay("");
    screen = DefaultScreen(dpy);
    background = BlackPixel(dpy, screen);
	if (width < 1)
		width = XDisplayWidth(dpy, screen);
	if (height < 1)
		height = XDisplayHeight(dpy, screen);
	if (dflag)
		delta = Min(a_range, b_range) / (double)Max(height, width);
    setupmem();
	InitBuffer(&Points, maxcolor);
    init_data();
    if (displayplanes > 1)
        foreground = startcolor;
    else
        foreground = WhitePixel(dpy,XDefaultScreen(dpy));
	if (xpos == -1)
    	hint.x = (XDisplayWidth(dpy, screen) - width)/2;
	else
    	hint.x = xpos;
	if (ypos == -1)
    	hint.y = (XDisplayHeight(dpy, screen) - height)/2;
	else
    	hint.y = ypos;
    hint.width = width;
    hint.height = height;
    hint.flags = PPosition | PSize;
    /*
     * Create the window to display the Julia or Mandelbrot set
     */
    canvas = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy),
        hint.x, hint.y, hint.width, hint.height,
        5, foreground, background);
	if (func == compjulia) {
    	XSetStandardProperties(dpy, canvas, "Julia by Ron Record", 
        		"Julia", None, av, ac, &hint);
    	XChangeProperty( dpy, canvas, XA_WM_CLASS, XA_STRING, 8,
        				PropModeReplace, "julia", strlen("julia"));
	}
	else {
    	XSetStandardProperties(dpy, canvas, "Mandel by Ron Record", 
        		"Mandel", None, av, ac, &hint);
    	XChangeProperty( dpy, canvas, XA_WM_CLASS, XA_STRING, 8,
        				PropModeReplace, "mandel", strlen("mandel"));
	}
    init_canvas();
    XSelectInput(dpy,canvas,KeyPressMask|ButtonPressMask|ButtonMotionMask|
        ButtonReleaseMask|ExposureMask|StructureNotifyMask);
	visual_template.class = PseudoColor;
	visual_list = XGetVisualInfo(dpy, VisualClassMask, &visual_template, &i);
	if (! visual_list)
		usedefault = 1;
	if (usedefault)
	  cmap = DefaultColormap(dpy, screen);
	else
	  cmap = XCreateColormap(dpy, canvas, DefaultVisual(dpy, screen), AllocAll);
    XMapRaised(dpy, canvas);
	if (xpos == -1)
		center_horizontal(dpy, canvas, width);
	if (ypos == -1)
		center_vertical(dpy, canvas, height);
    if (displayplanes > 1)
		if (func == compjulia)
			if (!usedefault)
        	    init_color(dpy, canvas, cmap, Colors, startcolor, startcolor,
					maxcolor, numwheels, "julia", "Julia", 0);
		else
			if (!usedefault)
        	    init_color(dpy, canvas, cmap, Colors, startcolor, startcolor,
					maxcolor, numwheels, "mandel", "Mandel", 0);
    else
        XQueryColors(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), 
                Colors, numcolors);
    pixmap = XCreatePixmap(dpy, DefaultRootWindow(dpy), width, height, 
                           DefaultDepth(dpy, screen));
    rubber_data.band_cursor=XCreateFontCursor(dpy, XC_hand2);
    CreateXorGC();
    Clear();
	cursor = XCreateFontCursor(dpy, XC_crosshair);
	XDefineCursor(dpy, canvas, cursor);
	if (restfile)
		restor_picture();
    if (demo)
        for(;;) {
            main_event();
            if (!run) {
				if (demo == 1) {
					DemoSpin(dpy, cmap, Colors, startcolor, maxcolor, delay, 2);
            		main_event();
					for (i=0; i<=MAXWHEELS; i++) {
						if (!usedefault)
						    init_color(dpy, canvas, cmap, Colors, startcolor, 
								startcolor, maxcolor, i, "julia", "Julia", 0);
						sleep(1);
            			main_event();
					}
				}
				XCloseDisplay(dpy);
                exit(0);
            }
        }
    else
        for(;;)
            main_event();
}

main_event()
{
    int n;
    XEvent event;

    if ((*func)() == TRUE)
        run=0;
    n = XEventsQueued(dpy, QueuedAfterFlush);
    while (n--) {
        XNextEvent(dpy, &event);
        switch(event.type) {
            case KeyPress: Getkey(&event); break;
            case Expose: redisplay(canvas, &event); break;
            case ConfigureNotify: resize(); break;
            case ButtonPress: StartRubberBand(canvas, &rubber_data, &event);
                 break;
            case MotionNotify: TrackRubberBand(canvas, &rubber_data, &event);
                 break;
            case ButtonRelease: EndRubberBand(canvas, &rubber_data, &event);
                 break;
        }
    }
}

void
dive() 
{
	static int i, j;
	extern void set_new_params();

    for (i=0;i<expind[frame];i++)
	  if (exponents[frame][i] > 0)
        if (exponents[frame][i] == maxcnt) {
			dive_index = i;
			break;
	  	}
	i = dive_index / width;	/* number of horizontal lines up */
	j = dive_index % width; /* number of vertical lines over */
	rubber_data.p_min = min_a+((double)j*a_range/(double)width)-(a_range/4.0);
	rubber_data.q_min = min_b+((double)i*b_range/(double)height)-(b_range/4.0);
	rubber_data.p_max = min_a+((double)j*a_range/(double)width)+(a_range/4.0);
	rubber_data.q_max = min_b+((double)i*b_range/(double)height)+(b_range/4.0);
	autodive--;
	set_new_params(canvas, &rubber_data, 1);
}

void
recalc() 
{
    static int i, index, x, y;
    
    mincnt = MAXINT - 1; maxcnt = 0;
    for (i=0;i<expind[frame];i++) {
	  if (exponents[frame][i]) {
        if (ABS(exponents[frame][i]) < mincnt)
            mincnt = ABS(exponents[frame][i]);
        if (ABS(exponents[frame][i]) > maxcnt)
        	if (ABS(exponents[frame][i]) != kmax)
            	maxcnt = ABS(exponents[frame][i]);
	  }
    }
}

draw() 
{
    extern void save_to_file(), redraw(), store_to_file();

	FlushBuffer(dpy, canvas, pixmap, Data_GC, &Points, 0, maxcolor);
	dorecalc = 1;
	recalc();
	if (autodive) {
		dive();
		return FALSE;
	}
	else {
		if (do_redraw)
			redraw(exponents[frame], expind[frame], 1);
		if (savefile)
			save_to_file();
		else if (storefile)
			store_to_file();
		return TRUE;
	}
	return TRUE;
}

find_period(x0, y0)
double x0, y0;
{
	static int k;
	static double x1, y1, x2, y2;

    k = 0; x2 = x0; y2 = y0;
    while ( k < kmax ) {
        x1 = x0*x0 - y0*y0 + a;
        y1 = 2*x0*y0 + b;
        k++;
        if ((x1*x1 + y1*y1) > M)
            return(k+kmax);
        else {
			if ((ABS(x1 - x2) < delta) && (ABS(y1 - y2) < delta))
				return(-k);
            x0 = x1;
            y0 = y1;
        }
    }
	return(kmax);
}

compmandel()
{
    register i;
	static int k;
    double x, r, x0, y0, x1, y1;

    if (!run)
        return TRUE;
    k = 0;
	x0 = 0.0; y0 = 0.0;	/* start iteration with the critical point */
    while ( k < kmax ) {
        x1 = x0*x0 - y0*y0 + a; /* this is the Real version of the */
        y1 = 2*x0*y0 + b;       /* Complex formula z -> z^2 + c    */
        k++;
        if ((x1*x1 + y1*y1) > M) /* could use a variety of exit criteria */
            break;
        else {
            x0 = x1;
            y0 = y1;
        }
    }
	if (second && (k == kmax))   /* there is a second attractor and  */
		k = find_period(x0, y0); /* we color according to its period */
    if (sendpoint(k) == TRUE)
        return FALSE;
    else {
		if (draw())
			run = 0;
       	return(run);
	}
}

compjulia()
{
    register i;
	static int k;
    double x, r, x0, y0, x1, y1;

    if (!run)
        return TRUE;
    k = 0;
	x0 = a; y0 = b;
    while ( k < kmax ) {
        x1 = x0*x0 - y0*y0 + p; /* the Real version of the Complex */
        y1 = 2*x0*y0 + q;       /* formula z -> z^2 + c            */
        k++;
        if ((x1*x1 + y1*y1) > M) /* could use a variety of exit criteria */
            break;
        else {
			if (second) /*if not going to infinity, color according to period*/
				if ((ABS(x1 - xper) < delta) && (ABS(y1 - yper) < delta)) {
					k = -k;
					break;
				}
            x0 = x1;
            y0 = y1;
        }
    }
    if (sendpoint(k) == TRUE)
        return FALSE;
    else {
		if (draw())
			run = 0;
       	return(run);
	}
}

int
find_pt() /* find a point on the second attractor if it exists */
{
	static int i;
	double x2, y2, x3, y3;

	x2 = 0.0; y2 = 0.0; /* the critical point */
	for (i=0; i<kmax; i++) {
		x3 = x2*x2 - y2*y2 + p;
		y3 = 2*x2*y2 + q;
		if ((x3*x3 + y3*y3) > M)
			return(1); /* not a second attractor */
		x2 = x3; y2 = y3;
	}
	xper = x3; yper = y3;
	return(0);
}

setmaxmin()
{
	if (wflag) {
		if (aflag)
			if (Aflag)
				a_range = max_a - min_a;
			else
    			max_a = min_a + a_range;
		else {
			if (Aflag)
				min_a = max_a - a_range;
			else {
    			max_a = center_x + (a_range/2.0);
				min_a = center_x - (a_range/2.0);
			}
		}
	}
	else if (aflag) {
		if (Aflag)
			a_range = max_a - min_a;
		else {
    		max_a = min_a + 2.0*(center_x - min_a);
			a_range = max_a - min_a;
		}
	}
	else {
		if (func == compjulia)
			a_range = DEF_WIDTH;
		else
			a_range = DEF_M_WIDTH;
		if (Aflag)
			min_a = max_a - a_range;
		else {
    		max_a = center_x + (a_range/2.0);
			min_a = center_x - (a_range/2.0);
		}
	}
	if (hflag) {
		if (bflag)
			if (Bflag)
				b_range = max_b - min_b;
			else
    			max_b = min_b + b_range;
		else {
			if (Bflag)
				min_b = max_b - b_range;
			else {
    			max_b = center_y + (b_range/2.0);
				min_b = center_y - (b_range/2.0);
			}
		}
	}
	else if (bflag) {
		if (Bflag)
			b_range = max_b - min_b;
		else {
			b_range = 2.0*(center_y - min_b);
    		max_b = min_b + b_range;
		}
	}
	else {
		if (func == compjulia)
			b_range = DEF_HEIGHT;
		else
			b_range = DEF_M_HEIGHT;
		if (Bflag)
			min_b = max_b - b_range;
		else {
    		max_b = center_y + (b_range/2.0);
			min_b = center_y - (b_range/2.0);
		}
	}
	center_x = min_a + (a_range/2.0);
	center_y = min_b + (b_range/2.0);
    a_minimums[0] = min_a; b_minimums[0] = min_b;
    a_maximums[0] = max_a; b_maximums[0] = max_b;
}

void
parseargs(ac, av)
int ac;
char **av;
{
    static int c;
    static int i;
    char *ch;
    extern int optind;
    extern char *optarg;
    extern double atof();
    extern void usage(), restor_params();

	if (func == compjulia) {
		min_a = -1.5; max_a = 1.5;
		center_x = 0.0;
	}
	else {
		min_a = -2.25; max_a = 0.75;
		center_x = -0.75;
		outname = "mandel.out";
		savname = "mandel.sav";
	}
    while ((c=getopt(ac,av,
        "CLSTuvA:B:D:F:K:W:H:M:N:O:X:Y:a:b:c:d:e:i:o:p:q:w:h:r:s:x:y:"))!=EOF) {
        switch (c) {
        case 'A':    max_a=atof(optarg); Aflag++; break;
        case 'B':    max_b=atof(optarg); Bflag++; break;
		case 'C':    do_redraw = 0; break;
        case 'D':    if (strcmp(optarg, "elay")) {
						dflag=0;
						delta=atof(optarg); 
					 }
					 else
						delay = atoi(av[optind++]);
					 break;
        case 'F':    autodive=atoi(optarg); break;
        case 'H':    height=atoi(optarg); break;
        case 'K':    kmax=atoi(optarg); break;
        case 'L':    logindex=0; break;
        case 'M':    M=atoi(optarg); 
					 if (M < 4)
						fprintf(stderr, 
							"Warning: escape radius should be >= 4\n");
					 break;
        case 'N':    maxcolor=ABS(atoi(optarg)); 
                if ((maxcolor - startcolor) <= 0)
                    startcolor = 0;
                break;
        case 'O':    color_offset=atoi(optarg); break;
        case 'S':    /* Don't look for a second attractor */
				second = 0; break;
        case 'T':    /* Do look for a second attractor */
				second = 1; break;
        case 'W':    width=atoi(optarg); break;
        case 'X':    xpos=atoi(optarg); break;
        case 'Y':    ypos=atoi(optarg); break;
        case 'a':    min_a=atof(optarg); aflag++; break;
        case 'b':    min_b=atof(optarg); bflag++; break;
        case 'c':    numwheels=atoi(optarg); break;
        case 'd':    demo=atoi(optarg); break;
        case 'e':    simple=atoi(optarg); break;
        case 'h':    b_range=atof(optarg); hflag++; break;
        case 'i':    restfile++; inname=optarg; break;
        case 'o':    savefile++; outname=optarg; break;
        case 'p':    p = atof(optarg); break;
        case 'q':    q = atof(optarg); break;
        case 'r':    res=atoi(optarg); break;
        case 's':    storefile++; savname=optarg; break;
        case 'u':    usage(); break;
        case 'v':    show=1; break;
        case 'w':    a_range=atof(optarg); wflag++; break;
        case 'x':    center_x=atof(optarg); break;
        case 'y':    center_y=atof(optarg); break;
        case '?':    usage(); break;
        }
    }
	if (restfile) {
		restor_params();
		width *= res; height *= res;
	    center_x = min_a + (a_range/2.0);
	    center_y = min_b + (b_range/2.0);
        a_minimums[0] = min_a; b_minimums[0] = min_b;
        a_maximums[0] = max_a; b_maximums[0] = max_b;
	}
	else {
	    setmaxmin();
	}
	if (second && (func == compjulia))
		if (find_pt())
			second = 0; /* couldn't find a second attractor */
}

void
usage()
{
	if (func == compjulia)
      fprintf(stderr,"julia [-Ls][-W#][-H#][-a#][-b#][-w#][-h#]\n");
	else
      fprintf(stderr,"mandel [-BLs][-W#][-H#][-a#][-b#][-w#][-h#]\n");
    fprintf(stderr,"\t[-M#][-S#][-D#][-f string][-r#][-O#][-C][-c#][-m#]\n");
    fprintf(stderr,"\t\tWhere: -C indicates don't redraw after compute\n");
    fprintf(stderr,"\t\t-r# specifies the maximum rgb value\n");
    fprintf(stderr,"\t\t-u displays this message\n");
    fprintf(stderr,"\t\t-a# specifies the minimum horizontal parameter\n");
    fprintf(stderr,"\t\t-b# specifies the minimum vertical parameter\n");
    fprintf(stderr,"\t\t-A# specifies the maximum horizontal parameter\n");
    fprintf(stderr,"\t\t-B# specifies the maximum vertical parameter\n");
    fprintf(stderr,"\t\t-w# specifies the horizontal parameter range\n");
    fprintf(stderr,"\t\t-h# specifies the vertical parameter range\n");
    fprintf(stderr,"\t\t-K# specifies the iteration limit\n");
    fprintf(stderr,"\t\t-M# specifies the escape detection radius\n");
    fprintf(stderr,"\t\t-H# specifies the initial window height\n");
    fprintf(stderr,"\t\t-W# specifies the initial window width\n");
    fprintf(stderr,"\t\t-O# specifies the color offset\n");
    fprintf(stderr,"\t\t-c# specifies the desired color wheel\n");
	if (func == compjulia) {
      fprintf(stderr,"\t\t-p# specifies the real part of the parameter\n");
      fprintf(stderr,"\t\t-q# specifies the complex part of the parameter\n");
	}
    fprintf(stderr,"\tDuring display :\n");
    fprintf(stderr,"\t\tThe left mouse button zooms in on an area\n");
	if (func == compmandel)
      fprintf(stderr,"\t\tThe middle mouse button selects a Julia parameter\n");
    fprintf(stderr,"\t\te or E recalculates color indices\n");
    fprintf(stderr,"\t\tf or F saves exponents to a file\n");
    fprintf(stderr,"\t\tr or R redraws\n");
    fprintf(stderr,"\t\ts or t spins the colorwheel\n");
    fprintf(stderr,"\t\tw or W changes the color wheel\n");
    fprintf(stderr,"\t\tx or X clears the window\n");
    fprintf(stderr,"\t\tq or Q exits\n");
    exit(1);
}

Cycle_frames()
{
    static int i;
    extern void redraw();

    for (i=0;i<=maxframe;i++)
        redraw(exponents[i], expind[i], 1); 
}

Getkey(event)
XKeyEvent *event;
{
    unsigned char key;
    static int i, running, spinning=0, spindir=0;
	extern int _numdivs;
    extern void init_color(), print_values(), print_help(), write_cmap();
    extern void go_init(), go_back(), go_down(), Clear(), store_to_file();
    extern void save_to_file(), Redraw(), redraw(), set_new_params();

    if (XLookupString(event, (char *)&key, sizeof(key), (KeySym *)0,
            (XComposeStatus *) 0) > 0)
        switch (key) {
            case '\015': /* write out current colormap to $HOME/.<prog>map */
					if (func == compjulia)
        				write_cmap(dpy,cmap,Colors,maxcolor,"julia","Julia");
					else
        				write_cmap(dpy,cmap,Colors,maxcolor,"mandel","Mandel");
					break;
			case '\030':	/* <ctrl>-X */
				if (_numdivs > 1)
					_numdivs--;
				else
					_numdivs = MAXDIVS;
				if (displayplanes > 1)
					if (func == compjulia)
						if (!usedefault)
        				    init_color(dpy,canvas,cmap,Colors,startcolor,
							startcolor,maxcolor,numwheels,"julia","Julia",0);
					else
						if (!usedefault)
        				    init_color(dpy,canvas,cmap,Colors,startcolor,
							startcolor,maxcolor,numwheels,"mandel","Mandel",0);
				break;
			case '\031':	/* <ctrl>-Y */
				if (_numdivs < MAXDIVS)
					_numdivs++;
				else
					_numdivs = 1;
				if (displayplanes > 1)
					if (func == compjulia)
						if (!usedefault)
        				    init_color(dpy,canvas,cmap,Colors,startcolor,
							startcolor,maxcolor,numwheels,"julia","Julia",0);
					else
						if (!usedefault)
        				    init_color(dpy,canvas,cmap,Colors,startcolor,
							startcolor,maxcolor,numwheels,"mandel","Mandel",0);
				break;
			case ')': delay += 25; break;
			case '(': delay -= 25; if (delay < 0) delay = 0; break;
			case '>': kmax *= 2; break;
			case '<': kmax /= 2; if (kmax < 1) kmax = 1; break;
			case ']': delta *= 2.0; break;
			case '[': delta /= 2.0; break;
			case '+': M *= 2; break;
			case '-': M /= 2; if (M < 4) M = 4; break;
			case '9': /* zoom in to center quarter */
					  rubber_data.p_min = center_x - (a_range/4.0);
					  rubber_data.q_min = center_y - (b_range/4.0);
					  rubber_data.p_max = center_x + (a_range/4.0);
					  rubber_data.q_max = center_y + (b_range/4.0);
					  set_new_params(canvas, &rubber_data, 0);
					  break;
			case '0': /* zoom out by doubling window size */
					  rubber_data.p_min = center_x - a_range;
					  rubber_data.q_min = center_y - b_range;
					  rubber_data.p_max = center_x + a_range;
					  rubber_data.q_max = center_y + b_range;
					  set_new_params(canvas, &rubber_data, 0);
					  break;
			case 'B': /* toggle autodive around deepest iteration count */
					  autodive = (!autodive);
					  if (autodive) {
					  	recalc();
					  	dive();
					  }
					  break;
			case 'b': /* toggle coloration for second attractor */
					  second = (!second);
                  	  redraw(exponents[frame], expind[frame], 1);
					  break;
            case 'd': go_down(); break;
            case 'D': FlushBuffer(dpy, canvas, pixmap, Data_GC, &Points, 
							0, maxcolor); 
					  break;
            case 'e':
            case 'E': FlushBuffer(dpy, canvas, pixmap, Data_GC, &Points, 
							0, maxcolor);
                  	  dorecalc = (!dorecalc);
                  	  if (dorecalc)
                      	recalc(); 
                  	  else
                      	maxcnt = 0;
                  	  redraw(exponents[frame], expind[frame], 1);
                  	  break;
            case 'f': FlushBuffer(dpy, canvas, pixmap, Data_GC, &Points, 
									0, maxcolor); 
					  store_to_file(); break;
            case 'F': FlushBuffer(dpy, canvas, pixmap, Data_GC, &Points, 
									0, maxcolor); 
            		  save_to_file(); break;
            case 'i': 
                  if (stripe_interval > 0) {
                      stripe_interval--;
                      if (displayplanes > 1) {
                          running = run; run = 0;
						  if (func == compjulia)
							if (!usedefault)
        					    init_color(dpy,canvas,cmap,Colors,startcolor,
							   startcolor,maxcolor,numwheels,"julia","Julia",0);
						  else
							if (!usedefault)
        					    init_color(dpy,canvas,cmap,Colors,startcolor,
							 startcolor,maxcolor,numwheels,"mandel","Mandel",0);
                          run = running;
                      }
                  }
                  break;
            case 'I': stripe_interval++;
                  if (displayplanes > 1) {
                      running = run; run = 0;
					  if (func == compjulia)
						if (!usedefault)
        				    init_color(dpy,canvas,cmap,Colors,startcolor,
							startcolor,maxcolor,numwheels,"julia","Julia",0);
					  else
						if (!usedefault)
        				    init_color(dpy,canvas,cmap,Colors,startcolor,
							startcolor,maxcolor,numwheels,"mandel","Mandel",0);
                      run = running;
                  }
                  break;
            case 'l':
            case 'L': logindex = (!logindex); break;
            case 'p':
            case 'P': negative = (!negative); 
					  FlushBuffer(dpy, canvas, pixmap, Data_GC, &Points, 
									0, maxcolor); 
					  redraw(exponents[frame], expind[frame], 1); 
                  	  break;
            case 'r': FlushBuffer(dpy, canvas, pixmap, Data_GC, &Points, 
								0, maxcolor); 
					  redraw(exponents[frame],expind[frame],1); 
                  	  break;
            case 'R': FlushBuffer(dpy, canvas, pixmap, Data_GC, &Points, 
									0, maxcolor); 
					  Redraw(); 
					  break;
            case 'S':
				  spinning = 0; break;
            case 's':
				  spinning = 1; spindir = (!spindir);
				  Spin(dpy, cmap, Colors, startcolor, maxcolor, delay, spindir);
				  break;
            case 'u': go_back(); break;
            case 'U': go_init(); break;
            case 'v':
            case 'V': print_values(); break;
            case '\027': /* (ctrl-W) read color palette from $HOME/.juliamap */
                  numwheels = 0;
                  if (displayplanes > 1) {
                      running = run; run = 0;
					  if (func == compjulia)
						if (!usedefault)
        				    init_color(dpy,canvas,cmap,Colors,startcolor,
							startcolor,maxcolor,numwheels,"julia","Julia",0);
					  else
						if (!usedefault)
        				    init_color(dpy,canvas,cmap,Colors,startcolor,
							startcolor,maxcolor,numwheels,"mandel","Mandel",0);
                      run = running;
                  }
                  break;
            case 'W': if (numwheels < MAXWHEELS)
                          numwheels++;
                      else
                          numwheels = 0;
                      if (displayplanes > 1) {
                          running = run; run = 0;
						  if (func == compjulia)
							if (!usedefault)
        					    init_color(dpy,canvas,cmap,Colors,startcolor,
							   startcolor,maxcolor,numwheels,"julia","Julia",0);
						  else
							if (!usedefault)
        					    init_color(dpy,canvas,cmap,Colors,startcolor,
							 startcolor,maxcolor,numwheels,"mandel","Mandel",0);
                          run = running;
                      }
                      break;
            case 'w': if (numwheels > 0)
                          numwheels--;
                      else
                          numwheels = MAXWHEELS;
                      if (displayplanes > 1) {
                          running = run; run = 0;
						  if (func == compjulia)
							if (!usedefault)
        					    init_color(dpy,canvas,cmap,Colors,startcolor,
							   startcolor,maxcolor,numwheels,"julia","Julia",0);
						  else
							if (!usedefault)
        					    init_color(dpy,canvas,cmap,Colors,startcolor,
							 startcolor,maxcolor,numwheels,"mandel","Mandel",0);
                          run = running;
                      }
                      break;
            case 'x': Clear(); break;
            case 'X': Destroy_frame(); break;
            case 'z': running = run; run = 0;
                  Cycle_frames(); 
                  run = running; redraw(exponents[frame], expind[frame], 1);
                  break;
            case 'Z': running = run; run = 0;
                  while (!XPending(dpy)) Cycle_frames(); 
                  run = running; redraw(exponents[frame], expind[frame], 1); 
                  break;
            case 'q':
            case 'Q': XCloseDisplay(dpy); exit(0); break;
            case '?':
            case 'h':
            case 'H': print_help(); break;
            default:  break;
        }
		if (spinning)
			Spin(dpy, cmap, Colors, startcolor, maxcolor, delay, spindir);
}

sendpoint(count)
int count;
{
    static int i, j, index, m, n;
	static double ratio;
	extern double log();

	if ((count == kmax) || (count == 0) || ((count < 0) && (!second)))
		index = 0;
	else {
		if (maxcnt) {
			if (simple) {
				i = count - mincnt;
				j = maxcnt - mincnt;
				if (j == 0)
					j = maxcnt;
				if ((j == 1) || (i == 0))
					ratio = (double)ABS(i);
				else if (logindex)
					ratio = log((double)ABS(i))/log((double)j);
				else
					ratio = (double)ABS(i)/(double)j;
			}
			else {
				if (maxcnt == 1)
					ratio = (double)ABS(count);
				else if (logindex)
					ratio = log((double)ABS(count))/log((double)maxcnt);
				else
					ratio = (double)ABS(count)/(double)maxcnt;
			}
			ratio /= (double)(second + 1);
			if (count > 0)
				index=((numfreecols-1)*ratio)+startcolor;
			else
				index=numcolors-((numfreecols-1)*ratio)-1;
		}
		else if (simple == 1) {
			if (logindex)
				ratio = log((double)ABS(count))/log((double)kmax);
			else
				ratio = (double)ABS(count)/(double)kmax;
			ratio /= (double)(second + 1);
			if (count > 0)
				index=((numfreecols-1)*ratio)+startcolor;
			else
				index=numcolors-((numfreecols-1)*ratio)-1;
		}
		else if (simple > 1) {
			i = numfreecols / (second + 1);
			if (count > 0)
        		index = ((simple*count) % i) + startcolor;
			else
        		index = numcolors - (ABS(simple*count) % i) - 1;
		}
		else if (simple < 0) {
			simple = -simple;
			i = numfreecols / (second + 1);
			j = i/2;
			if (count >= i)
        		index = ((simple*count) % j) + startcolor + j;
			else if ((count > 0) && (count < i))
        		index = ((simple*count) % j) + startcolor;
			else
        		index = numcolors - (ABS(simple*count) % i) - 1;
		}
		else {
			i = numfreecols / (second + 1);
    		if (ABS(count) < 64) {
				if (count > 0)
        			index = (count % i) + startcolor;
				else
        			index = numcolors - (ABS(count) % i) - 1;
			}
    		else if (ABS(count) < 128) {
					if (count > 0)
            			index = (((count/8)+64) % i) + startcolor;
					else
            			index = numcolors - (((ABS(count)/8)+64) % i) - 1;
    		}
    		else if (ABS(count) < 192) {
					if (count > 0)
            			index = (((count/16)+72) % i) + startcolor;
					else
            			index = numcolors - (((ABS(count)/16)+72) % i) - 1;
    		}
    		else if (ABS(count) < 256) {
					if (count > 0)
            			index = (((count/32)+88) % i) + startcolor;
					else
            			index = numcolors - (((ABS(count)/32)+88) % i) - 1;
    		}
        	else
				if (count > 0)
            		index = (((count/64)+120) % i) + startcolor;
				else
            		index = numcolors - (((ABS(count)/64) % i)+120) - 1;
		}
	}
	for (i=0; i<res; i++)
		for (j=0; j<res; j++) {
			m = point.x + j;
			n = point.y + i;
			BufferPoint(dpy, canvas, pixmap, Data_GC, &Points, index,
						m, height - n);
			if (symetrical)
			  if (func == compjulia)
    			BufferPoint(dpy, canvas, pixmap, Data_GC, &Points, index, 
							m+(2*(w2-m))-rem, height - (n+(2*(h2-n))));
			  else
    			BufferPoint(dpy, canvas, pixmap, Data_GC, &Points, index, 
							m, height - (n+(2*(h2-n))));
		}
    if (save)
        exponents[frame][expind[frame]++] = count;
	a += a_inc;
    point.x += res;
    if (point.x >= width) {
        point.y += res;
        point.x = 0;
        if (save) {
            b += b_inc;
            a = min_a;
        }
        if ((point.y >= height) || (symetrical && (point.y > h2))) {
    		FlushBuffer(dpy, canvas, pixmap, Data_GC, &Points, 0, maxcolor);
            return FALSE;
		}
        else
            return TRUE;
    }
    return TRUE;
}

void 
redisplay (w, event)
Window          w;
XExposeEvent    *event;
{
    /*
     * Extract the exposed area from the event and copy
     * from the saved pixmap to the window.
     */
    XCopyArea(dpy, pixmap, canvas, Data_GC[0], 
           event->x, event->y, event->width, event->height, 
           event->x, event->y);
}

void
resize()
{
    Window r;
    int n, x, y;
    unsigned int bw, d, new_w, new_h;
    extern void Clear(), Redraw();

	if (demo && restfile)
		return;
    XGetGeometry(dpy,canvas,&r,&x,&y,&new_w,&new_h,&bw,&d);
    if ((new_w == width) && (new_h == height))
        return;
    width = new_w; height = new_h;
	w2 = width / 2; h2 = height / 2;
	if ((2*w2) == width)
		rem = 1;
	else
		rem = 0;
    XClearWindow(dpy, canvas);
    if (pixmap)
        XFreePixmap(dpy, pixmap);
    pixmap = XCreatePixmap(dpy, DefaultRootWindow(dpy), 
            width, height, DefaultDepth(dpy, screen));
    a_inc = a_range / (double)width;
    b_inc = b_range / (double)height;
    point.x = 0;
    point.y = 0;
    run = 1;
    a = rubber_data.p_min = min_a;
    b = rubber_data.q_min = min_b;
    rubber_data.p_max = max_a;
    rubber_data.q_max = max_b;
	if (dflag)
		delta = Min(a_range, b_range) / (double)Max(height, width);
    freemem();
    setupmem();
    for (n=0;n<MAXFRAMES;n++)
        if ((n <= maxframe) && (n != frame))
            resized[n] = 1;
    InitBuffer(&Points, maxcolor);
    Clear();
    Redraw();
}

void
redraw(exparray, index, cont)
int *exparray;
int index, cont;
{
    static int i;
    static int x_sav, y_sav;
	static double a_sav, b_sav;

    x_sav = point.x;
    y_sav = point.y;
    a_sav = a;
    b_sav = b;

    point.x = 0;
    point.y = 0;

    save=0;
    for (i=0;i<index;i++)
        sendpoint(exparray[i]);
    save=1;
    
    if (cont) {
        point.x = x_sav;
        point.y = y_sav;
        a = a_sav;
        b = b_sav;
    }
    else {
        a = point.x * a_inc + min_a;
        b = point.y * b_inc + min_b;
    }
    FlushBuffer(dpy, canvas, pixmap, Data_GC, &Points, 0, maxcolor);
}

void
Redraw() 
{
	dorecalc = maxcnt = 0;
    FlushBuffer(dpy, canvas, pixmap, Data_GC, &Points, 0, maxcolor);
    point.x = 0;
    point.y = 0;
    run = 1;
    a = min_a;
    b = min_b;
    expind[frame] = 0;
    resized[frame] = 0;
}

/* Store colormap indices in file so we can read them in later */
void
store_to_file() 
{
    FILE *outfile;
    static int i;

    outfile = fopen(savname,"w");
    if(!outfile) {
        perror(savname);
		XCloseDisplay(dpy);
        exit(-1);
    }

    fprintf(outfile,"# width=%d height=%d run=%d\n",width,height,run);
	fprintf(outfile,"# kmax=%d (p,q)=(%.12lg,%.12lg)\n", kmax, p, q);
    fprintf(outfile,"# min_a=%.12lg  a_rng=%.12lg  max_a=%.12lg\n",
			min_a,a_range,max_a);
    fprintf(outfile,"# min_b=%.12lg  b_rng=%.12lg  max_b=%.12lg\n",
			min_b,b_range,max_b);
    fprintf(outfile,"# a=%.12lg  b=%.12lg  res=%d\n",a,b,res);
    fprintf(outfile,"# point.x=%d point.y=%d\n",point.x,point.y);
    fprintf(outfile,"# expind=%d\n",expind[frame]);
    fprintf(outfile,"%d\n",numcolors-1);

    for (i=0;i<expind[frame];i++)
    	fprintf(outfile,"%d\n",exponents[frame][i]);
    fclose(outfile);
}

/* Store color pics in PPM format and monochrome in PGM */
void
save_to_file() 
{
    FILE *outfile;
    unsigned char c;
    XImage *ximage;
    static int i,j;
    struct Colormap {
        unsigned char red;
        unsigned char green;
        unsigned char blue;
    };
    struct Colormap *colormap=NULL;

    if (colormap)
        free(colormap);
    if ((colormap=
        (struct Colormap *)malloc(sizeof(struct Colormap)*maxcolor))
            == NULL) {
        fprintf(stderr,"Error malloc'ing colormap array\n");
		XCloseDisplay(dpy);
        exit(-1);
    }
    outfile = fopen(outname,"w");
    if(!outfile) {
        perror(outname);
		XCloseDisplay(dpy);
        exit(-1);
    }

    ximage=XGetImage(dpy, pixmap, 0, 0, width, height, AllPlanes, ZPixmap);

    if (displayplanes > 1) {
        for (i=0;i<maxcolor;i++) {
            colormap[i].red=(unsigned char)(Colors[i].red >> 8);
            colormap[i].green=(unsigned char)(Colors[i].green >> 8);
            colormap[i].blue =(unsigned char)(Colors[i].blue >> 8);
        }
        fprintf(outfile,"P%d %d %d\n",6,width,height);
    }
    else
        fprintf(outfile,"P%d %d %d\n",5,width,height);
	fprintf(outfile,"# kmax=%d (p,q)=(%.12lg,%.12lg)\n", kmax, p, q);
    fprintf(outfile,"# min_a=%.12lg  a_rng=%.12lg  max_a=%.12lg\n",
			min_a,a_range,max_a);
    fprintf(outfile,"# min_b=%.12lg  b_rng=%.12lg  max_b=%.12lg\n",
			min_b,b_range,max_b);
    fprintf(outfile,"%d\n",numcolors-1);

    for (j=0;j<height;j++)
        for (i=0;i<width;i++) {
            c = (unsigned char)XGetPixel(ximage,i,j);
            if (displayplanes > 1)
                fwrite((char *)&colormap[c],sizeof colormap[0],1,outfile);
            else
                fwrite((char *)&c,sizeof c,1,outfile);
        }
    fclose(outfile);
}

/* Read saved file with parameters */
void
restor_params() 
{
	unsigned char s[16];

    infile = fopen(inname,"r");
    if(!infile) {
        perror(inname);
        exit(-1);
    }

    fscanf(infile,"%1s%6s%d%7s%d%4s%d",&s,&s,&width,&s,&height,&s,&run);
	fscanf(infile,"%1s%5s%d%7s%lg%1s%lg%1s", &s,&s,&kmax,&s,&p,&s,&q,&s);
/* printf("kmax=%d p=%.12lg q=%.12lg\n",kmax,p,q); */
    fscanf(infile,"%1s%6s%lg%6s%lg%6s%lg", &s,&s,&min_a,&s,&a_range,&s,&max_a);
/* printf("min_a=%.12lg a_range=%.12lg max_a=%.12lg\n",min_a,a_range,max_a); */
    fscanf(infile,"%1s%6s%lg%6s%lg%6s%lg", &s,&s,&min_b,&s,&b_range,&s,&max_b);
/* printf("min_b=%.12lg b_range=%.12lg max_b=%.12lg\n",min_b,b_range,max_b); */
    fscanf(infile,"%1s%2s%lg%2s%lg%4s%d", &s,&s,&a,&s,&b,&s,&res);
/* printf("a=%.12lg b=%.12lg\n",a,b); */
    fscanf(infile,"%1s%8s%d%8s%d",&s,&s,&point.x,&s,&point.y);
/* printf("point.x=%d point.y=%d\n",point.x,point.y); */
    fscanf(infile,"%1s%7s%d",&s,&s,&expind[0]);
	fread(&numcolors, sizeof numcolors, 1, infile);
	numcolors++;
    rubber_data.p_min = a_minimums[frame] = min_a ;
    rubber_data.q_min = b_minimums[frame] = min_b ;
    rubber_data.p_max = a_maximums[frame] = max_a;
    rubber_data.q_max = b_maximums[frame] = max_b ;
}

void
restor_picture() {
    static int i, k, l, h, w;
	unsigned char c;
	unsigned char s[8];
    XEvent event;

    for (i=0;i<expind[frame];i++)
    	fscanf(infile, "%d", &exponents[frame][i]);
    fclose(infile);
	if (!run) {
		dorecalc = 1;
		recalc();
	}
	redraw(exponents[frame], expind[frame], 1); 
	FlushBuffer(dpy, canvas, pixmap, Data_GC, &Points, 0, maxcolor);
}

void
Clear() 
{
    XFillRectangle(dpy, pixmap, Data_GC[0], 0, 0, width, height);
    XCopyArea(dpy, pixmap, canvas, Data_GC[0], 
                0, 0, width, height, 0, 0);
    InitBuffer(&Points, maxcolor);
}

void
show_defaults() 
{

    printf("Width=%d  Height=%d  numcolors=%d\n", width,height,numcolors);
    printf("min_a=%.12lg  a_range=%.12lg  max_a=%.12lg\n", min_a,a_range,max_a);
    printf("min_b=%.12lg  b_range=%.12lg  max_b=%.12lg\n", min_b,b_range,max_b);
    printf("mincnt=%d  maxcnt=%d\n", mincnt,maxcnt);
    exit(0);
}

void
CreateXorGC()
{
    XGCValues values;

    values.foreground = foreground;
    values.line_style = LineSolid;
    values.function = GXxor;
    RubberGC = XCreateGC(dpy, DefaultRootWindow(dpy),
          GCForeground | GCBackground | GCFunction | GCLineStyle, &values);
}

void
SetupCorners(corners, data)
XPoint *corners;
image_data_t *data;
{
    corners[0].x = data->rubber_band.start_x;
    corners[0].y = data->rubber_band.start_y;
    corners[1].x = data->rubber_band.start_x;
    corners[1].y = data->rubber_band.last_y;
    corners[2].x = data->rubber_band.last_x;
    corners[2].y = data->rubber_band.last_y;
    corners[3].x = data->rubber_band.last_x;
    corners[3].y = data->rubber_band.start_y;
    corners[4] = corners[0];
}

void 
StartRubberBand(w, data, event)
Window w;
image_data_t *data;
XButtonEvent *event;
{
    XPoint corners[5];
	static char xystr[40];

    nostart = 0;
	switch (event->button) {
		case Button1:
    		data->rubber_band.last_x = data->rubber_band.start_x = event->x;
    		data->rubber_band.last_y = data->rubber_band.start_y = event->y;
			button = 1;
    		SetupCorners(corners, data);
    		XDrawLines(dpy, canvas, RubberGC,
        		corners, sizeof(corners) / sizeof(corners[0]), CoordModeOrigin);
			break;
		case Button2:
			button = 2;
			if (func == compmandel) {
				p = min_a + (((double)event->x/(double)width)*a_range);
				q = max_b - (((double)event->y/(double)height)*b_range);
				sprintf(xystr," (%.12lg,%.12lg) ", p, q);
				XDrawImageString(dpy, canvas, Data_GC[1], width/4, height - 20, 
								xystr, strlen(xystr));
			}
			break;
	}
}

void 
TrackRubberBand(w, data, event)
Window w;
image_data_t *data;
XMotionEvent *event;
{
    XPoint corners[5];
    int xdiff, ydiff, diff;
	static char xystr[40];

    if (nostart)
        return;
	if (button == 1) {
    	SetupCorners(corners, data);
    	XDrawLines(dpy, canvas, RubberGC, corners, 
              		sizeof(corners) / sizeof(corners[0]), CoordModeOrigin);
    	ydiff = event->y - data->rubber_band.start_y;
    	xdiff = event->x - data->rubber_band.start_x;
    	data->rubber_band.last_x = data->rubber_band.start_x + xdiff;
    	data->rubber_band.last_y = data->rubber_band.start_y + ydiff;
    	if (data->rubber_band.last_y < data->rubber_band.start_y ||
       		data->rubber_band.last_x < data->rubber_band.start_x) {
           		data->rubber_band.last_y = data->rubber_band.start_y;
           		data->rubber_band.last_x = data->rubber_band.start_x;
    	}
    	SetupCorners(corners, data);
    	XDrawLines(dpy, canvas, RubberGC, corners, 
              		sizeof(corners) / sizeof(corners[0]), CoordModeOrigin);
	}
	else if (button == 2) {
		if (func == compmandel) {
			p = min_a + (((double)event->x/(double)width)*a_range);
			q = max_b - (((double)event->y/(double)height)*b_range);
			sprintf(xystr," (%.12lg,%.12lg) ", p, q);
			XDrawImageString(dpy, canvas, Data_GC[1], width/4, height - 20, 
							xystr, strlen(xystr));
		}
	}
}

void
set_new_params(w, data, center)
Window w;
image_data_t *data;
int center;
{
    extern void Clear();

    frame = (maxframe + 1) % MAXFRAMES;
    if (frame > maxframe)
        maxframe = frame;
    a_range = data->p_max - data->p_min;
    b_range = data->q_max - data->q_min;
    a_minimums[frame] = min_a = data->p_min;
    b_minimums[frame] = min_b = data->q_min;
	if (center) {
		center_x = min_a + (a_range/2.0);
		center_y = min_b + (b_range/2.0);
		if ((center_x == 0.0) && (center_y == 0.0))
        	symetrical = 1;
		else if ((func == compmandel) && (center_y == 0.0))
			symetrical = 1;
    	else
        	symetrical = 0;
	}
    a_inc = a_range / (double)width;
    b_inc = b_range / (double)height;
    point.x = 0;
    point.y = 0;
    run = 1;
    a = min_a;
    b = min_b;
    a_maximums[frame] = max_a = data->p_max;
    b_maximums[frame] = max_b = data->q_max;
    expind[frame] = 0; dorecalc = maxcnt = 0;
	if (dflag)
		delta = Min(a_range, b_range) / (double)Max(height, width);
    Clear();
}

void 
EndRubberBand(w, data, event)
Window w;
image_data_t *data;
XButtonEvent *event;
{
    XPoint corners[5];
    XPoint top, bot;
    double r_delta, diff;
	char pval[64], qval[64];
	char wval[16], hval[16], kval[16];
	char xval[8], yval[8];

    nostart = 1;
	switch (event->button) {
		case Button1:
    		SetupCorners(corners, data);
    		XDrawLines(dpy, canvas, RubberGC,
        		corners, sizeof(corners) / sizeof(corners[0]), CoordModeOrigin);
    		if (data->rubber_band.start_x >= data->rubber_band.last_x ||
        		data->rubber_band.start_y >= data->rubber_band.last_y)
        		return;
    		top.x = data->rubber_band.start_x;
    		bot.x = data->rubber_band.last_x;
    		top.y = data->rubber_band.start_y;
    		bot.y = data->rubber_band.last_y;
    		diff = data->q_max - data->q_min;
    		r_delta = (double)(height - bot.y) / (double)height;
    		data->q_min += diff * r_delta;
    		r_delta = (double)top.y / (double)height;
    		data->q_max -= diff * r_delta;
    		diff = data->p_max - data->p_min;
    		r_delta = (double)top.x / (double)width;
    		data->p_min += diff * r_delta;
    		r_delta = (double)(width - bot.x) / (double)width;
    		data->p_max -= diff * r_delta;
    		fflush(stdout);
    		set_new_params(w, data, 1);
			break;
		case Button2:
			if (func == compmandel) {
			  if ((event->x >= 0) && (event->x <= width) && 
				  (event->y >= 0) && (event->y <= height)) {
				switch (fork()) {
					case -1:
						fprintf(stderr,"Can't create new process\n");
						return;
					case 0:
						xpos = event->x*XDisplayWidth(dpy, screen)/width;
						xpos -= (width/4); if (xpos < 0) xpos = 0;
						ypos = event->y*XDisplayHeight(dpy, screen)/height;
						ypos -= (height/4); if (ypos < 0) ypos = 0;
						p = min_a + (((double)event->x/(double)width)*a_range);
						q = max_b - (((double)event->y/(double)height)*b_range);
						sprintf(xval, "%d", xpos);
						sprintf(yval, "%d", ypos);
						sprintf(wval, "%d", width/2);
						sprintf(hval, "%d", height/2);
						sprintf(kval, "%d", kmax);
						sprintf(pval, "%.12lg", p);
						sprintf(qval, "%.12lg", q);
						if (path)
							strcat(path, "julia");
						else
							path = strdup("julia");
						execlp(path,path,"-X",xval,"-Y",yval,"-p",pval,
						       "-q",qval,"-W",wval,"-H",hval,"-K",kval,NULL);
						exit(0);
				}
			  }
			  XCopyArea(dpy,pixmap,canvas,Data_GC[1],0,0,width,height,0,0);
			}
			break;
	}
	button = 0;
}

void
go_down() 
{
    static int i;
    
    frame++;
    if (frame > maxframe)
        frame = 0;
    jumpwin();
}

void
go_back() 
{
    static int i;
    
    frame--;
    if (frame < 0)
        frame = maxframe;
    jumpwin();
}

jumpwin()
{
	if (!maxframe)
		return;
    rubber_data.p_min = min_a = a_minimums[frame];
    rubber_data.q_min = min_b = b_minimums[frame];
    rubber_data.p_max = max_a = a_maximums[frame];
    rubber_data.q_max = max_b = b_maximums[frame];
    a_range = max_a - min_a;
    b_range = max_b - min_b;
	center_x = min_a + (a_range/2.0);
	center_y = min_b + (b_range/2.0);
	if ((center_x == 0.0) && (center_y == 0.0))
        symetrical = 1;
	else if ((func == compmandel) && (center_y == 0.0))
		symetrical = 1;
    else
        symetrical = 0;
    a_inc = a_range / (double)width;
    b_inc = b_range / (double)height;
    point.x = 0;
    point.y = 0;
    a = min_a;
    b = min_b;
	dorecalc = maxcnt = 0;
    Clear();
    if (resized[frame])
        Redraw();
    else
        redraw(exponents[frame], expind[frame], 0);
	if ((point.y >= height) || (symetrical && (point.y > (height/2)))) {
		run = 0;
		dorecalc = 1;
		recalc();
        redraw(exponents[frame], expind[frame], 1);
	}
	else
		run = 1;
}

void
go_init() 
{
    static int i;
    
	if (frame) {
    	frame = 0;
    	jumpwin();
	}
}

Destroy_frame()
{
    static int i;

    for (i=frame; i<maxframe; i++) {
        exponents[frame] = exponents[frame+1];
        expind[frame] = expind[frame+1];
        a_minimums[frame] = a_minimums[frame+1];
        b_minimums[frame] = b_minimums[frame+1];
        a_maximums[frame] = a_maximums[frame+1];
        b_maximums[frame] = b_maximums[frame+1];
    }
    maxframe--;
    go_back();
}

void
print_help() 
{
    printf("During run-time, interactive control can be exerted via : \n");
    printf("Mouse buttons allow rubber-banding of a zoom box\n");
    printf("D flushes the drawing buffer\n");
    printf("e or E recalculates color indices\n");
    printf("f or F saves exponents to a file\n");
    printf("h or H or ? displays this message\n");
    printf("i decrements, I increments the stripe interval\n");
    printf("p or P reverses the colormap for negative/positive exponents\n");
    printf("r redraws without recalculating\n");
    printf("R redraws while recalculating with new values\n");
    printf("s or S spins the colorwheel\n");
    printf("u pops back up to the last zoom\n");
    printf("U pops back up to the first picture\n");
    printf("v or V displays the values of various settings\n");
    printf("w decrements, W increments the color wheel index\n");
    printf("x or X clears the window\n");
    printf("q or Q exits\n");
}

void
print_values() 
{
    static int i;

	if (func == compjulia)
		printf("julia: (p,q)=(%.12lg, %.12lg)\n", p, q);
	else
		printf("mandel:\n");
    printf("\nmincnt=%d maxcnt=%d run=%d\n",mincnt,maxcnt,run);
    printf("width=%d height=%d res=%d\n",width,height,res);
    printf("kmax=%d M=%d\n", kmax,M); 
    printf("point.x=%d point.y=%d\n",point.x,point.y); 
    printf("a=%.12lg  b=%.12lg\n",a,b);
    printf("min_a=%.12lg  max_a=%.12lg\n",min_a,max_a);
    printf("min_b=%.12lg  max_b=%.12lg\n",min_b,max_b);
    printf("center=(%.12lg, %.12lg) a_rng=%.12lg b_rng=%.12lg\n",
			center_x,center_y,a_range,b_range);
    printf("numcolors=%d\n",numcolors-1);
}

freemem()
{
    static int i;

    for (i=0;i<MAXFRAMES;i++)
        free(exponents[i]);
}

setupmem()
{
    static int i;

    for (i=0;i<MAXFRAMES;i++) {
        if((exponents[i]=
            (int *)malloc(sizeof(int)*width*height))==NULL){
                fprintf(stderr,"Error malloc'ing exponent array.\n");
                exit(-1);
        }
    }
}
