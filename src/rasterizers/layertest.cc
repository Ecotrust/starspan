//
// StarSpan project
// LayerRasterizer test
// Carlos A. Rueda
// $Id: layertest.cc,v 1.1 2008-04-18 07:46:56 crueda Exp $
//

#include <geos/version.h>
#if GEOS_VERSION_MAJOR < 3
	#include <geos.h>
#else
	#include <geos/unload.h>
	using namespace geos::io;   // for Unload
#endif


#include "LayerRasterizer.h"
#include "Raster.h"



#include <cstdlib>
#include <ctime>




class MyLayerRasterizerObserver : public LayerRasterizerObserver {
public:	                 

    void init(RastInfo& info) {
        cout<< "==>> init \n";
    }
    
    void intersectionFound(RastIntersectionInfo& intersInfo) {
        cout<< "==>> intersectionFound \n";
    }
    
    void intersectionEnd(RastIntersectionInfo& intersInfo) {
        cout<< "\n==>> intersectionEnd \n";
    }
    
	void addPixel(RastEvent& ev) {
		cout << ".";
        cout.flush();
	}
    
    void end(void) {
        cout << "==>> end \n";
    }
};

int main(int argc, char** argv) {
    
    
    const char* vector_filename = "../../tests/data/vector/starspan_testply.shp";
    const char* raster_filename = "../../tests/data/raster/starspan2raster.img";
    
    
    OGRRegisterAll();
    GDALAllRegister();
    
    
	OGRDataSource* ds = OGRSFDriverRegistrar::Open(vector_filename);
    if ( ds == NULL ) {
        cerr<< "Unable to open datasource " <<vector_filename<< endl;
		return 0;
	};
    
	OGRLayer* layer = ds->GetLayer(0);
	if( layer == NULL ) {
		cerr<< " Couldn't fetch layer 0\n";
		return 0;
	}


    Raster* raster = new Raster(raster_filename);
    if ( raster == NULL ) {
        cerr<< "Unable to open raster " <<raster_filename<< endl;
		return 0;
	}
    
	int width, height;
    double x0, y0, x1, y1;
    double pix_x_size, pix_y_size;
    raster->getSize(&width, &height, NULL);
    raster->getCoordinates(&x0, &y0, &x1, &y1);
    raster->getPixelSize(&pix_x_size, &pix_y_size);
    
    cout<< "\t width=" <<width<< endl
        << "\t height=" <<height<< endl
        << "\t x0=" <<x0<< endl
        << "\t y0=" <<y0<< endl
        << "\t x1=" <<x1<< endl
        << "\t y1=" <<y1<< endl
        << "\t pix_x_size=" <<pix_x_size<< endl
        << "\t pix_y_size=" <<pix_y_size<< endl
    ;
    
    
    cout<< "creating rasterizer...\n";
	LayerRasterizer lr(
        layer,
        width, height,
        x0, y0, x1, y1,
        pix_x_size, pix_y_size
    );
    
    lr.setVerbose(true);
    
	MyLayerRasterizerObserver observer;
	lr.addObserver(&observer);

    cout<< "rasterizing...\n";
    lr.rasterize();	
	
    return 0;
}

