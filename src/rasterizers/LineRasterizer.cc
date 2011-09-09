//
// STARSpan project
// Line rasterization using a Bresenham interpolator 
// with subpixel accuracy (Anti-grain geometry, AGG)
// Carlos A. Rueda
// $Id: LineRasterizer.cc,v 1.2 2008-04-18 02:12:22 crueda Exp $
//

#include "rasterizers.h"           
#include "agg/agg_dda_line.h"   // line_bresenham_interpolator

#include <math.h>

// Implementation is based on agg_renderer_primitives.h


// (null-object pattern)
static LineRasterizerObserver null_observer;

// user location to agg location with 8-bit accuracy
inline int to_int(double dv, double size_v) { 
	int v = (int) rint( 256.0 * (dv / size_v) );
	//***int v = (int) rint( 256.0 * ((dv + size_v/2) / size_v) );
	return v;
}

// agg location to user location
inline double to_double(int iv, double size_v) { 
	double v = iv * size_v;
	//***double v = iv * size_v - size_v/2;
	return v;
}


LineRasterizer::LineRasterizer(double x0, double y0, double pixel_size_x, double pixel_size_y) 
: x0(x0), y0(y0), pixel_size_x(pixel_size_x), pixel_size_y(pixel_size_y) {
	observer = &null_observer;
}

//
// Based on agg::renderer_primitives#line
//
void LineRasterizer::line(double dx1, double dy1, double dx2, double dy2, bool last)  {
	int x1 = to_int(dx1 - x0, pixel_size_x);
	int y1 = to_int(dy1 - y0, pixel_size_y);
	int x2 = to_int(dx2 - x0, pixel_size_x);
	int y2 = to_int(dy2 - y0, pixel_size_y);

	agg::line_bresenham_interpolator li(x1, y1, x2, y2);

	unsigned len = li.len();
	if(len == 0) {
		if ( last ) {
			double x = x0 + to_double(li.line_lr(x1),  pixel_size_x);
			double y = y0 + to_double(li.line_lr(y1),  pixel_size_y);
			observer->pixelFound(x, y);
		}
		return;
	}

	if ( last )
		++len;

	if(li.is_ver()) {
		do {
			double x = x0 + to_double(li.x2(), pixel_size_x);
			double y = y0 + to_double(li.y1(), pixel_size_y);
			observer->pixelFound(x, y);
			li.vstep();
		}
		while(--len);
	}
	else {
		do {
			double x = x0 + to_double(li.x1(), pixel_size_x);
			double y = y0 + to_double(li.y2(), pixel_size_y);
			observer->pixelFound(x, y);
			li.hstep();
		}
		while(--len);
	}
}



