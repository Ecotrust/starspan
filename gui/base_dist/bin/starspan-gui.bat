@echo off
@rem DOS Batch file to invoke the StarSpan GUI
@rem $Id: starspan-gui.bat,v 1.1 2005-05-19 21:37:45 crueda Exp $
SET STARSPANGUIDIR="$INSTALL_PATH"
java -jar "%STARSPANGUIDIR%\bin\guie.jar" "%STARSPANGUIDIR%\bin\starspan.guie"
@echo on

