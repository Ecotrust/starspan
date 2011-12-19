# About Starspan

StarSpan is designed to bridge the raster and vector worlds of spatial
analysis using fast algorithms for pixel level extraction from
geometry features (points, lines, polygons). StarSpan generates
databases of extracted pixel values (from one or a set of raster
images), fused with the database attributes from the vector files.
This allows a user to do statistical analysis of the pixel vs.
attribute data in many existing packages and can greatly speed up
classification training and testing. See the documentation for more
details about commands, operations, and options.

This development has been possible thanks to California State
Department of Boating and Waterways.

StarSpan should compile and run on most Unix flavor (including MacOs X
and Windows via MinGW) although it has been mainly tested on Linux and
MacOS X. StarSpan's core program is run from the command line. An
experimental Java-based graphical user interface (GUI) is also
available though no longer currently maintained.

# Installing StarSpan

StarSpan requires the GDAL and GEOS libraries properly installed on 
your system. 
 
    git clone git://github.com/Ecotrust/starspan.git
    cd starspan
    ./configure 
    make 
    sudo make install

For more details, see INSTALL.

# Running StarSpan

## Zonal Statistics
A common use case for starspan is to summarize raster pixel values by vector geometries. 
For instance, you may want to find the average NDVI value for each polygon in a shapefile. 

    starspan --vector zonemap.shp --raster ndvi.tif --stats avg \
    --out-prefix ndvi --out-type table --summary-suffix _stats.csv

This would output a csv file named `test_stats.csv` which would provide statistics for each
polygon feature, one per row.

    FID,id,RID,numPixels,avg_Band1
    0,1,ndvi.tif,2890,3.869815
    2,3,ndvi.tif,225,4.781782
    3,4,ndvi.tif,682,4.871310


# Status 

This is an open source software project and you are welcome to use the
tool and contribute to this ongoing project. Please read the copyright
agreement (copyright.txt). 

The original project was developed at UC Davis (formerly housed at
http://casil.ucdavis.edu/projects/starspan/). The project is being moved
to github, unfortunately without any of the CVS history. It is currently 
being maintained by `perrygeo` with support from Ecotrust.

Documentation, as you may notice, is very lacking. I hope to remedy this 
in the future. Please bear with us as we make the transition.
