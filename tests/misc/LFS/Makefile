#
# make   -->  GDAL test
# make simple --> Big file creation
#

.PHONY: gdal simple

# <gdal>  Test of LFS using GDAL
# May need to adjust the following:
UL=/usr/local

cc=g++
gdal_cflags=-I$(UL)/include
gdal_ld=-L $(UL)/lib -lgdal
cflags=-Wall -g $(gdal_cflags)

gdal: lfstest
	./lfstest 
	@echo
	@echo " --> printed message above should be 01    --Don't forget to 'make clean'"

lfstest: lfstest.cc
	$(cc) lfstest.cc -Wall -g $(cflags) -o $@ $(gdal_ld) 
	
# </gdal> 


# <simple>   Simple test to create a BIG file -- doesn't use any special libraries
simple: create_big_file
	./create_big_file BIGFILE 10
	@echo
	@ls -l BIGFILE
	@echo "Note: under MinGW, the size may not be properly reported; use your file explorer."
	@echo "Try using 'od' to check that the last byte is 1 and 'make clean'"
	@echo "for example:  od -c -j 2147483650 BIGFILE"

create_big_file: create_big_file.cc lfs.h
	g++ -Wall create_big_file.cc -o create_big_file

# </simple>



tidy:
	rm -f *.o *~
	
clean: tidy
	rm -f lfstest create_big_file BIGRASTER* BIGFILE *.exe
	

