#########################################################
# starspan Makefile.am 
# $Id: Makefile.am,v 1.16 2008-07-08 07:43:20 crueda Exp $
#########################################################

bin_PROGRAMS = starspan2
bin_SCRIPTS  = starspan
CLEANFILES = $(bin_SCRIPTS)
EXTRA_DIST   = starspan

starspan2_SOURCES =	\
	src/starspan2.cc \
	src/starspan_grass.cc \
	src/starspan_rasterize.cc \
	src/starspan_dup_pixel.cc \
	src/starspan_csv2.cc \
	src/starspan_minirasters2.cc \
	src/starspan_minirasterstrip2.cc \
	src/starspan_stats.cc \
	src/starspan_countbyclass.cc \
	src/starspan_csv.cc \
	src/starspan_minirasters.cc \
	src/starspan_jtstest.cc \
	src/starspan_util.cc \
	src/starspan_dump.cc \
	src/csv/Csv.cc \
	src/csv/CsvOutput.cc \
	src/jts/jts.cc \
	src/raster/Raster_gdal.cc \
	src/rasterizers/LineRasterizer.cc \
	src/stats/Stats.cc \
	src/traverser/traverser.cc \
	src/traverser/polyqt.cc \
	src/traverser/pixset.cc \
	src/util/Progress.cc \
	src/vector/Vector_ogr.cc

AM_CPPFLAGS = -g @GEOS_INC@  @GDAL_INC@

AM_LDFLAGS = @GRASS_LIB@
starspan2_LDADD = @GRASS_LDADD@

INCLUDES = \
	-I$(srcdir)/src \
	-I$(srcdir)/src/csv \
	-I$(srcdir)/src/jts \
	-I$(srcdir)/src/raster \
	-I$(srcdir)/src/rasterizers \
	-I$(srcdir)/src/stats \
	-I$(srcdir)/src/traverser \
	-I$(srcdir)/src/util \
	-I$(srcdir)/src/vector

