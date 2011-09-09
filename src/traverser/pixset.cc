//
// StarSpan project
// PixSet - Set of pixel locations
// Carlos A. Rueda
// $Id: pixset.cc,v 1.1 2005-07-05 03:56:09 crueda Exp $
//

#include "traverser.h"           

#include <cstdlib>
#include <cassert>
#include <cstring>

/////////////////////////////////////////////////////////////////////
//
//    PixSet
//

PixSet::PixSet() {
}

PixSet::~PixSet() {
}

void PixSet::insert(int col, int row) {
	EPixel colrow(col, row);
	_set.insert(colrow);
}

bool PixSet::contains(int col, int row) {
	return _set.find(EPixel(col, row)) != _set.end() ;
}

int PixSet::size() {
	return _set.size();
}

void PixSet::clear() {
	_set.clear();
}

PixSet::Iterator* PixSet::iterator() {
	return new PixSet::Iterator(this); 
}


/////////////////////////////////////////////////////////////////////
//
//    PixSet::Iterator
//

PixSet::Iterator::Iterator(PixSet* ps) : ps(ps) { 
	colrow = ps->_set.begin();
}

PixSet::Iterator::~Iterator() { 
}


bool PixSet::Iterator::hasNext() {
	return colrow != ps->_set.end();
}

void PixSet::Iterator::next(int *col, int *row) {
	*col = colrow->col;
	*row = colrow->row;
	colrow++;
}



