PATH=\msys\1.0\mingw\bin;\MinGW\bin\;%PATH%
REM ---------- SLASH'EM ----------
del /S /Q slashem\win\vultures
mkdir slashem\win\vultures
xcopy /E /-Y vultures\* slashem\win\vultures\
cd slashem\sys\winnt
call nhsetup
cd ..\..\src
mingw32-make -f Makefile.gcc spotless
mingw32-make -f Makefile.gcc all
cd ..\..
