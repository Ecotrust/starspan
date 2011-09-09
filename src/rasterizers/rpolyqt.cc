//
// StarSpan project
// LayerRasterizer::processValidPolygon_QT
// Carlos A. Rueda
// $$
// Quadtree algorithm for polygon rasterization 
//

#include "LayerRasterizer.h"           

#include <cstdlib>
#include <cassert>
#include <cstring>

// for polygon processing:
#include "geos/opPolygonize.h"


inline void swap_if_greater(int& a, int&b) {
	if ( a > b ) {
		int tmp = a;
		a = b;
		b = tmp;
	}
}



// processValidPolygon_QT: Quadtree algorithm
void LayerRasterizer::processValidPolygon_QT(Polygon* geos_poly) {
	const Envelope* intersection_env = geos_poly->getEnvelopeInternal();
	
	// get envelope corners in pixel coordinates:
	int minCol, minRow, maxCol, maxRow;
	toColRow(intersection_env->getMinX(), intersection_env->getMinY(), &minCol, &minRow);
	toColRow(intersection_env->getMaxX(), intersection_env->getMaxY(), &maxCol, &maxRow);
	
	// since minCol is not necessarily <= maxCol (ditto for *Row):
	swap_if_greater(minCol, maxCol);
	swap_if_greater(minRow, maxRow);
	
	double x, y;
	toGridXY(minCol, minRow, &x, &y);
	_Rect env(this, x, y, maxCol - minCol + 1, maxRow - minRow +1);
	
	rasterize_poly_QT(env, geos_poly);
}

// 
// rasterize_geometry_QT and rasterize_poly_QT are mutually recursive
// functions that implicitily do a quadtree-like scan:
//
void LayerRasterizer::rasterize_poly_QT(_Rect& e, Polygon* i) {
	if ( i == NULL || e.empty() )
		return;

	// compare envelope and intersection areas: 
	double area_e = e.area();
	double area_i = i->getArea();
	
	// because of subtleties of comparing floating point numbers, 
	// add an epsilon relative to the value:
	assert( area_i <= area_e * 1.0001 ) ;

	
	//
	// If the area of intersection is at least the area of the whole
	// envelope minus a fraction dependent on the pixel proportion,
	// then all pixels in envelope are to be included in the intersection:
	//
	if ( area_i >= area_e - (pix_abs_area - pixelProportion_times_pix_abs_area) ) {
		// all pixels in e are to be reported:
		dispatchRect_QT(e);
		return;
	}
	
	//	
	// if pixelProportion > 0.0 we can check if the area of intersection
	// is too small compared to the minimum required by that pixel proportion:
	if ( pixelProportion > 0.0 ) {
		if ( area_i < pixelProportion_times_pix_abs_area ) { 
			// do nothing (we can safely discard the whole envelope).
			return;
		}
	}
	else {  
		// pixelProportion == 0.0: means that just the intersection 
		// (i != null) will be enough condition to include the envelope 
		// when this is just a pixel:
		if ( e.cols == e.rows && e.rows == 1 ) {
			dispatchRect_QT(e);
			return;
		}
	}
	
	//
	// In other cases, we just recur to each of the children in the
	// quadtree decomposition:
	//
	
	_Rect e_ul = e.upperLeft();
	Geometry* i_ul = e_ul.intersect(i);
	if ( i_ul ) {		
		rasterize_geometry_QT(e_ul, i_ul);
		delete i_ul;
	}
	
	_Rect e_ur = e.upperRight();
	Geometry* i_ur = e_ur.intersect(i);
	if ( i_ur ) {		
		rasterize_geometry_QT(e_ur, i_ur);
		delete i_ur;
	}

	_Rect e_ll = e.lowerLeft();
	Geometry* i_ll = e_ll.intersect(i);
	if ( i_ll ) {		
		rasterize_geometry_QT(e_ll, i_ll);
		delete i_ll;
	}

	_Rect e_lr = e.lowerRight();
	Geometry* i_lr = e_lr.intersect(i);
	if ( i_lr ) {		
		rasterize_geometry_QT(e_lr, i_lr);
		delete i_lr;
	}
}

// implicitily does a quadtree-like scan:
void LayerRasterizer::rasterize_geometry_QT(_Rect& e, Geometry* i) {
	if ( i == NULL || e.empty() )
		return;
	GeometryTypeId type = i->getGeometryTypeId();
	switch ( type ) {
		case GEOS_POLYGON:
			rasterize_poly_QT(e, (Polygon*) i);
			break;
			
		case GEOS_MULTIPOLYGON:
		case GEOS_GEOMETRYCOLLECTION: {
			GeometryCollection* gc = (GeometryCollection*) i; 
			for ( unsigned j = 0; j < gc->getNumGeometries(); j++ ) {
				Geometry* g = (Geometry*) gc->getGeometryN(j);
				rasterize_geometry_QT(e, (Polygon*) g);
			}
			break;
		}
	
		default:
			if ( logstream ) {
				(*logstream)
					<< "Warning: rasterize_geometry_QT: "
					<< "intersection ignored: "
					<< i->getGeometryType() <<endl
					//<< wktWriter.write(i) <<endl
				;
			}
	}
	
}

void LayerRasterizer::dispatchRect_QT(_Rect& r) {
	int col, row;
	toColRow(r.x, r.y, &col, &row);
	for ( int i = 0; i < r.rows; i++ ) {
		double y = r.y + i * pix_y_size; 
		for ( int j = 0; j < r.cols; j++ ) {
			double x = r.x + j * pix_x_size;
			dispatchPixel(col + j, row + i, x, y);
		}
	}
}

