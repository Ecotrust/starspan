# Gets the required binaries and puts them in base_dist/bin/
# Run this script under MinGW
# $Id: mingw_prepare_binaries.sh,v 1.2 2008-07-26 22:08:53 crueda Exp $

if [ "$OSTYPE" != "msys" ]; then
    echo "Warning: are you sure this is the right machine?"
fi

# the prefix used to build GEOS, GDAL, and StarSpan on MinGW
win_prefix=/usr/local

# copy required binaries into base_dist/bin/
echo "Copying required binaries from ${win_prefix} ...
cp -p ${win_prefix}/bin/libgeos_c-1.dll  base_dist/bin/
cp -p ${win_prefix}/bin/libgeos-2.dll    base_dist/bin/
cp -p ${win_prefix}/bin/libgdal-1.dll    base_dist/bin/
cp -p ${win_prefix}/bin/ogrinfo.exe      base_dist/bin/
cp -p ${win_prefix}/bin/gdalinfo.exe     base_dist/bin/
cp -p ${win_prefix}/bin/starspan2.exe    base_dist/bin/
