PATH=\MinGW\bin\;%PATH%
REM ---------- SLASH'EM ----------
del /S /Q slashem\win\jtp
mkdir slashem\win\jtp
xcopy /E /-Y vultures\win\jtp\* slashem\win\jtp\
cd slashem\sys\winnt
call nhsetup
cd ..\..\src
mingw32-make -f Makefile.gcc spotless
mingw32-make -f Makefile.gcc all
cd ..\..
