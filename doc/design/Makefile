# $Id: Makefile,v 1.3 2004-09-30 07:32:12 crueda Exp $

DOT=dot

############################################# 
# class diagrams

java_files := $(shell find $(diagrams) -name *.java)
dot_files  := $(subst .java,.dot,$(java_files))

png_files := $(subst .dot,.png,$(dot_files))
eps_files := $(subst .dot,.eps,$(dot_files))


uml_graph:=/home/carueda/prg/UMLGraph-2.8/UmlGraph.jar

jd:=javadoc
jd_flags := -docletpath $(uml_graph) -doclet UmlGraph -private
# jd_flags += -nodefontname luxisr -nodefontabstractname luxisri
# jd_flags += -edgefontname luxisr
# jd_flags += -nodefontsize 8 -edgefontsize 8
jd_flags += -nodefillcolor LemonChiffon
detailed_flags := -attributes -operations -types


%.png : %.eps
	#$(DOT)-Elabelfontcolor=DarkSlateBlue -Tpng -o $@ $<
	convert -resize 100% $< $@

%.eps : %.dot
	$(DOT) -Nheight=0.2 -Elabelfontcolor=DarkSlateBlue -Elabelfontsize=8 -Tps -o $@ $<

%.dot : %.java
	$(jd) $(jd_flags) $(detailed_flags) -output $@ $<


	
all: $(eps_files) $(png_files) \
	csv-sd.eps csv-sd.png \
	envisl-sd.eps envisl-sd.png


############################################# 
# sequence diagrams

csv-sd.png: csv-sd.eps
	convert -resize 120% $< csv-sd.png

csv-sd.eps: csv-sd.pic
	LD_LIBRARY_PATH=~/lib pic2plot -Tps $< > csv-sd.eps
	

envisl-sd.png: envisl-sd.eps
	convert -resize 120% $< envisl-sd.png

envisl-sd.eps: envisl-sd.pic
	LD_LIBRARY_PATH=~/lib pic2plot -Tps $< > envisl-sd.eps
	

############################################# 
clean:
	rm -f *~ *.png *.eps *.dot
