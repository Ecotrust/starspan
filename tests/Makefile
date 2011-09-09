#
# Makefile for tests
# $Id: Makefile,v 1.15 2008-07-08 07:43:20 crueda Exp $
#

STARSPAN=../starspan

# TESTS involves comparisons with expected outputs:
TESTS=test_csv test_stats test_miniraster test_miniraster_strip

# GENS involves the generation of some outputs to just check that the program runs:
GENS=gen_miniraster_box gen_miniraster_strip_box gen_rasterize

.PHONY: test init $(TESTS) $(GENS) ALL_TESTS ALL_GENS ALL
        

ALL_TESTS: init $(TESTS)
	@echo "!!!!!!! $@ : OK !!!!!!!"

ALL_GENS: init $(GENS)
	@echo "!!!!!!! $@ : OK !!!!!!!"

ALL: ALL_TESTS ALL_GENS

init:
	$(shell if [ ! -x ${STARSPAN} ]; then chmod +x ${STARSPAN}; fi)
	mkdir -p generated/

test_csv:
	mkdir -p generated/csv/
	rm -f generated/csv/*.csv
	${STARSPAN} \
		--vector data/vector/ply \
		--raster data/raster/starspan[1-3]raster.img \
		--out-type table \
		--out-prefix generated/csv/PRFX \
		--table-suffix output.csv
	zcat expected/csv/myoutput.csv.gz | diff - generated/csv/PRFXoutput.csv
	@echo "$@ : OK"
	@echo
	
test_stats:
	mkdir -p generated/stats/
	rm -f generated/stats/*.csv
	${STARSPAN} \
		--verbose \
		--fields none \
		--vector data/vector/ply \
		--raster data/raster/starspan[1-3]raster.img \
		--nodata 0 \
		--out-type table \
		--out-prefix generated/stats/PRFX \
		--summary-suffix output.csv \
		--stats avg mode stdev min max sum median nulls
	zcat expected/stats/myoutput.csv.gz | diff - generated/stats/PRFXoutput.csv
	@echo "$@ : OK"
	@echo
	
test_minirasters:
	mkdir -p generated/miniraster/
	${STARSPAN} \
		--vector data/vector/ply \
		--raster data/raster/starspan2raster.img \
		--out-type mini_rasters \
		--out-prefix generated/miniraster/myprefix \
		--mr-img-suffix _ \
		--in \
		--nodata 1 \
		--fid 3 
	cmp expected/miniraster/myprefix_0003.img \
	   generated/miniraster/myprefix_0003.img
	${STARSPAN} \
		--vector data/vector/ply \
		--raster data/raster/starspan2raster.img \
		--out-type mini_rasters \
		--out-prefix generated/miniraster/myprefix \
		--mr-img-suffix _buff2_ \
		--in \
		--buffer 2 \
		--fid 3
	cmp expected/miniraster/myprefix_buff2_0003.img \
	   generated/miniraster/myprefix_buff2_0003.img
	@echo "$@ : OK"
	@echo
	
test_miniraster_strip:
	mkdir -p generated/mrstrip/
	${STARSPAN} \
		--vector data/vector/ply \
		--raster data/raster/starspan2raster.img \
		--out-type mini_raster_strip \
		--out-prefix generated/mrstrip/myoutput \
		--mrst-img-suffix _mrst.img \
		--mrst-shp-suffix _mrst.shp \
		--mrst-fid-suffix _mrid.img \
		--mrst-glt-suffix _mrloc.img \
		--in
	cmp expected/mrstrip/myoutput_mr.img \
	   generated/mrstrip/myoutput_mr.img
	cmp expected/mrstrip/myoutput_mrid.img \
	   generated/mrstrip/myoutput_mrid.img
	cmp expected/mrstrip/myoutput_mrloc.glt \
	   generated/mrstrip/myoutput_mrloc.glt
	@echo "$@ : OK"
	@echo

# preliminary generation of miniraster along with --box option	
gen_miniraster_box:
	mkdir -p generated/miniraster_box/
	${STARSPAN} \
		--vector data/vector/ply \
		--raster data/raster/starspan2raster.img \
		--out-type mini_rasters \
		--out-prefix generated/miniraster_box/box \
		--mr-img-suffix _100_ \
		--box 100

# preliminary generation of miniraster strip along with --box and --separation options
gen_miniraster_strip_box:
	mkdir -p generated/mrstrip_box/
	${STARSPAN} \
		--vector data/vector/ply \
		--raster data/raster/starspan2raster.img \
		--RID path \
		--out-type mini_raster_strip \
		--out-prefix generated/mrstrip_box/box_100 \
		--mrst-img-suffix _mrst.img \
		--mrst-shp-suffix _mrst.shp \
		--mrst-fid-suffix _mrid.img \
		--mrst-glt-suffix _mrloc.img \
		--box 100 \
		--separation 10
		
# preliminary rasterization with --rasterize option
gen_rasterize:
	mkdir -p generated/rasterize/
	${STARSPAN} \
		--vector data/vector/ply \
		--raster data/raster/starspan2raster.img \
		--out-type rasterization \
		--out-prefix generated/rasterize/ \
		--rasterize-suffix rasterized

