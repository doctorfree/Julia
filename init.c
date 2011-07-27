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

#include "defines.h"
#include "externs.h"

void
init_data()
{
    static int i;

    numcolors = XDisplayCells(dpy, XDefaultScreen(dpy));
    displayplanes = DisplayPlanes(dpy, XDefaultScreen(dpy));
    if (numcolors > maxcolor)
        numcolors = maxcolor;
    if (numcolors <= 16) {
        maxcolor = 16; startcolor = 1; 
        color_offset = 0;
    }
    numfreecols = numcolors - startcolor;
    a_inc = a_range / (double)width;
    b_inc = b_range / (double)height;
	if (!restfile) {
    	point.x = 0;
    	point.y = 0;
    	a = rubber_data.p_min = min_a;
    	b = rubber_data.q_min = min_b;
	}
    rubber_data.p_max = max_a;
    rubber_data.q_max = max_b;
    if (show)
        show_defaults();
	w2 = width / 2; h2 = height / 2;
	if ((2*w2) == width)
		rem = 1;
	else
		rem = 0;
	if ((center_x == 0.0) && (center_y == 0.0))
        symetrical = 1;
	else if ((func == compmandel) && (center_y == 0.0))
		symetrical = 1;
    else
        symetrical = 0;
    srand48(time(0));
}

void
init_canvas()
{
    static int i;

    /*
     * create default, writable, graphics contexts for the canvas.
     */
    for (i=0; i<maxcolor; i++) {
        Data_GC[i] = XCreateGC(dpy, DefaultRootWindow(dpy),
                (unsigned long) 0, (XGCValues *) 0);
        /* set the background to black */
        XSetBackground(dpy,Data_GC[i],BlackPixel(dpy,XDefaultScreen(dpy)));
        /* set the foreground of the ith context to i */
        XSetForeground(dpy, Data_GC[i], i);
    }
	XSetForeground(dpy,Data_GC[0],BlackPixel(dpy,XDefaultScreen(dpy)));
	XSetForeground(dpy,Data_GC[1],WhitePixel(dpy,XDefaultScreen(dpy)));
}
