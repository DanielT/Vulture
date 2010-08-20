@which bison
@IF ERRORLEVEL 1 GOTO need_gnuwin
@which flex
@if ERRORLEVEL 1 GOTO need_gnuwin

REM ---------- NetHack ----------
del /S /Q nethack\win\vultures
mkdir nethack\win\vultures
xcopy /E /-Y vultures\* nethack\win\vultures\
cd nethack\sys\winnt
call nhsetup
cd ..\..\src
mingw32-make -f Makefile.gcc spotless
mingw32-make -f Makefile.gcc all
cd ..\..
strip --strip-all nethack\binary\Vulture-NetHack.exe

goto EOF

:need_gnuwin
@echo.
@echo You need flex and bison to build vultures. 
@echo A good place to get them is the gnuwin project
@echo see http://gnuwin32.sourceforge.net/
@echo.
@echo Be sure to use the gnuwin32 prompt for correct paths
@echo.

:EOF
