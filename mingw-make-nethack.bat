PATH=\MinGW\bin\;%PATH%
REM ---------- NetHack ----------
del /S /Q nethack\win\jtp
mkdir nethack\win\jtp
xcopy /E /-Y vultures\win\jtp\* nethack\win\jtp\
cd nethack\sys\winnt
call nhsetup
cd ..\..\src
mingw32-make -f Makefile.gcc spotless
mingw32-make -f Makefile.gcc all
cd ..\..
