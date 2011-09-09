#
# Makefile to build raster module
# $Id: Makefile,v 1.1 2005-02-19 21:04:32 crueda Exp $
#
#
RASTER_INCLUDE=-I/usr/local/include

CXXFLAGS = \
		   -g -Wall \
		   $(RASTER_INCLUDE)

all: Raster_gdal.o

.cc.o:
	g++ $(CXXFLAGS) -c $<

