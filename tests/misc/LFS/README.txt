LFS tests
Carlos A. Rueda
$Id: README.txt,v 1.2 2007-10-19 01:52:23 crueda Exp $ 

* Simple test:  make simple
A basic test to check that the underlying system is able to create
and write data to a big file beyond the 2Gb limit.

* GDAL test: make gdal
lfstest creates a >2Gb-size raster with name BIGRASTER. It skips the
first 2Gb bytes (thus "filling" them with zeroes) and then fills the
trailing cells with ones; then uses GDAL RasterIO to read the two byte
values just on the 2Gb boundary and prints them to stdout. If the
underlying system supports large files and GDAL has been compiled
accordingly, then the printed message should be "01".



