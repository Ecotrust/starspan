//
// Progress - A simple progress indicator
// Carlos A. Rueda
// $Id: Progress.cc,v 1.1 2005-02-19 21:29:20 crueda Exp $
// See Progress.h for public doc.
//

#include "Progress.h"
#include <iomanip>
#include <cassert>

Progress::Progress(long size, double perc_incr_, ostream& out)
: size(size), perc_incr(perc_incr_), out(out) {
	assert( perc_incr > 0 );
	count_update = 0;
	if ( perc_incr < 100.0 / size )
		perc_incr = 100.0 / size;
	next_perc = perc_incr;
	out << setprecision(2);
}

Progress::Progress(long size, ostream& out)
: size(size), out(out) {
	count_update = 0;
	perc_incr = 0;
}

void Progress::start() {
	if ( perc_incr > 0 )
		out << "0% ";
	else
		out << "0 ";
	out.flush();
}

void Progress::update() {
	count_update++;
	if ( perc_incr > 0 ) {
		curr_perc = 100.0 * count_update / size;
		if ( curr_perc >= next_perc ) {
			if ( curr_perc < 100 )
				out << curr_perc << "% ";
			else {
				out << "100%";
			}
			out.flush();
			next_perc += perc_incr;
		}
		if ( next_perc > 100.0 )
			next_perc = 100.0;
	}
	else if ( count_update % size == 0 ) {
		out << count_update << " ";
		out.flush();
	}
}

void Progress::complete() {
	if ( perc_incr > 0 ) {
		if ( curr_perc < 100 )
			out << "100%";
	}
	else if ( count_update % size != 0 ) {
		out << count_update;
	}
	out.flush();
}
	

