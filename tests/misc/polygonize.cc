#include "geos.h"
#include "geos/opPolygonize.h"
#include <vector>


int main() {
	cout << "starting\n";
	geos::WKTReader r(new geos::GeometryFactory());
	geos::WKTWriter w;
	cout << "reader created\n";

	geos::Polygonizer pp;
	
	geos::Geometry* noded = 0;
	try {
		noded = r.read("LINESTRING (0 0, 100 100)");
		noded = noded->Union(r.read("LINESTRING (100 100, 100 0)"));
		noded = noded->Union(r.read("LINESTRING (100 0, 0 100)"));
		noded = noded->Union(r.read("LINESTRING (0 100, 0 0)"));
	}
	catch(...) {
		cout << " something happend\n";
		return 1;
	}
	
	cout << w.write(noded) << endl;
	
	// vector<geos::Polygon*>* polys = pp.getPolygons();
	// cout << polys->size() << " polys obtained\n";
	
	return 0;
}
	
	
