StarSpan GUI

This is an experimental implementation of a GUI for StarSpan.
This GUI provides a direct interface to the command-line executable 
`starspan' which is part of the StarSpan core system.

Prerequisites

	- A Java Runtime Environment version 1.4 or later must be properly
	  installed on your system. In particular, the executable `java' 
	  needs to be in your PATH environment variable.
	  More info at: http://java.com

	- The StarSpan core system must be properly installed on your
	  system to actually be able to execute the operations.(*) In
	  particular, the executable `starspan' needs to be in your PATH
	  environment variable. 
	  More info at: http://starspan.casil.ucdavis.edu
	  
	  (*) Note: You will be able to run the GUI even without the
	  StarSpan core system; the actual dependency will occur at run
	  time only to fill in some informational fields and when you
	  click the "Execute" button. Only in those cases the executable
	  `starspan' will be called.
	  
Running

	Assuming DIR is the directory you chose for installation of
	StarSpan GUI:
	
	Windows:    
		- Select the shortcut created by the installer
		or
		- Double click DIR\bin\starspan-gui.bat in your file explorer
			
	Unix/Linux: 
		Run DIR/bin/starspan-gui
	
Status:
	Experimental

-------------------------------------------------------------
$Id: README.txt,v 1.2 2005-06-08 06:23:50 crueda Exp $	
