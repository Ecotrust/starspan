#include <iostream>
#include <cstdlib>
#include "rasterizers.h"

// $Id: test.cc,v 1.2 2008-04-18 02:12:22 crueda Exp $
//
//
//   g++ -I. -Iagg -Wall LineRasterizer.cc test.cc
//   ./a.out 0 0 3 3 10.1 5.3 20.6 10.1
//   pixel origin: (0, 0)  pixel size:  3, 3
//   From  (10.1, 5.3) -> (20.6, 10.1)
//           9,3
//           12,6
//           15,6
//           18,9
//

using namespace std;


class MyLineRasterizerObserver : public LineRasterizerObserver {
public:	
	void pixelFound(double x, double y) {
		cout << "\t" << x << "," << y << endl;
	}
};

int main(int argc, char** argv) {
	if ( argc != 9 ) {
		cerr <<  "test x0 y0 pix_size_x pix_size_y x1 y1 x2 y2" << endl;
		return 0;
	}
	int arg = 1;
	double x0 = atof(argv[arg++]);
	double y0 = atof(argv[arg++]);
	double pix_size_x = atof(argv[arg++]);
	double pix_size_y = atof(argv[arg++]);
	double x1 = atof(argv[arg++]);
	double y1 = atof(argv[arg++]);
	double x2 = atof(argv[arg++]);
	double y2 = atof(argv[arg++]);

	LineRasterizer lr(x0, y0, pix_size_x, pix_size_y);
	MyLineRasterizerObserver observer;
	lr.setObserver(&observer);
	
	cout << "pixel origin: (" << x0 << ", " << y0 << ")  "  
	     << "pixel size:  " << pix_size_x << ", " << pix_size_y << endl;  
	cout << "From  (" << x1 << ", " << y1 << ") -> (" << x2 << ", " << y2 << ")" << endl;  
	lr.line(x1, y1, x2, y2, true);
	
    return 0;
}

