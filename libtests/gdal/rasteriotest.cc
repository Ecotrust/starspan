/* This test reveals a bug in GDAL RasterIO operation.
 * Carlos Rueda - 2005-02-16 reported.
 * $Id: rasteriotest.cc,v 1.3 2008-05-09 21:05:28 crueda Exp $ 
 */
#include "gdal.h"           
#include "gdal_priv.h"
#include <iostream>
#include <cassert>
using namespace std; 

/* RasterIO bug:
        $ g++ rasteriotest.cc -lgdal -Wall
        $ ./a.out
        GDAL_RELEASE_NAME = 1.5.0
        RasterIO returned = 0
        Raster contents should be:
        XXXXXXXXXXXXXXXXXXXXX_______________________________________
        This is the actual contents using ``cat raster'':
        _____________X______________________________________________
*/
int main() {
    cout<<"GDAL_RELEASE_NAME = " <<GDAL_RELEASE_NAME<< endl;
    GDALAllRegister();
    GDALDriver* hDriver = GetGDALDriverManager()->GetDriverByName("ENVI");
    assert(hDriver);
    GDALDataset* dataset = hDriver->Create(
       "raster",
       10,       //nXSize
       6,       //nYSize
       1,          //nBands
       GDT_Byte,  //GDALDataType
       NULL        //papszParmList
    );
    dataset->GetRasterBand(1)->Fill('_');
    char value = 'X';
    int result = dataset->RasterIO(
        GF_Write,
        0,          //nXOff,
        0,             //nYOff,
        7,           //nXSize,
        3,           //nYSize,
        &value,        //pData,
        1,             //nBufXSize,
        1,             //nBufYSize,
        GDT_Byte,     //eBufType,
        1,             //nBandCount,
        NULL,          //panBandMap,
        0,             //nPixelSpace,
        0,             //nLineSpace,
        0              //nBandSpace
    );     
    dataset->FlushCache();
    delete dataset;
    cout<< "RasterIO returned = " <<result<< endl;
    cout<< "Raster contents should be:" <<endl;
    cout<< "XXXXXXXXXXXXXXXXXXXXX_______________________________________" <<endl;
    cout<< "This is the actual contents using ``cat raster'':" <<endl;
    system("cat raster");
    return 0;
}

