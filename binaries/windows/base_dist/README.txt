StarSpan GUI

This is an experimental StarSpan binary distribution for Windows.
This distribution contains the StarSpan core system and a Java-based
graphical interface.

Prerequisites

	- A Java Runtime Environment version 1.4 or later must be properly
	  installed on your system in order to run the GUI. In particular,
	  the executable `java' needs to be in your PATH environment variable.
	  More info at: http://java.com

Running

	To run the GUI:
		Either use the file browser to run DIR\bin\starspan-gui.bat,
		where DIR is the installation directory, or launch the shortcut
		created by the installer.
		
	To run the command line interface:
		Open a DOS command window and just type: DIR\bin\starspan2.exe
		where DIR is the installation directory. For convenience you
		may want to include DIR\bin in your PATH environment variable.
		See your OS documentation for more details.

Documentation

	Please visit the StarSpan website at:
		http://starspan.casil.ucdavis.edu

Supporting libraries

	GEOS - Geometry Engine Open Source 
		http://geos.refractions.net/
		Copyright (C) 2005 Refractions Research Inc.
		Used under the GNU Lesser General Public Licence as published
		by the Free Software Foundation.
		
	GDAL - Geospatial Data Abstraction Library
		http://www.gdal.org/
		Copyright (c) 1998, 2002 Frank Warmerdam
		
		
Limitations

	- GDAL is compiled in a way that raster files larger that 2 Gb are
	  not supported. This is because MinGW (the environment being used
	  to build GDAL under Windows) is currently not supported as an 
	  official GDAL build environment.
	  See: http://xserve.flids.com/pipermail/gdal-dev/2005-May/008588.html

	- In general this binary is experimental, so it may certainly 
	  contain bugs. We'd appreciate your feedback.
	  
StarSpan: Copyright (c) 2005 The Regents of the University of California.
Please read the doc\copyright.txt file.
	  
-------------------------------------------------------------
$Id: README.txt,v 1.1 2005-06-15 01:46:56 crueda Exp $	
