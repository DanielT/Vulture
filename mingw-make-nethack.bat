PATH=\msys\1.0\mingw\bin;\MinGW\bin\;%PATH%
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
