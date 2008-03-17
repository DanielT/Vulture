@set GNUWIN_DIR=C:\Program Files\Gnuwin32

@rem (WoW64 cmd prompt sets PROCESSOR_ARCHITECTURE to x86) [taken from msys-bat --daniel]
@if not "x%PROCESSOR_ARCHITECTURE%" == "xAMD64" goto _NotX64
@set GNUWIN_DIR=C:\Program Files (x86)\Gnuwin32
:_NotX64

@if not exist "%GNUWIN_DIR%\bin\bison.exe" GOTO need_gnuwin
@if not exist "%GNUWIN_DIR%\bin\flex.exe" GOTO need_gnuwin

PATH=\msys\1.0\mingw\bin;\MinGW\bin\;%GNUWIN_DIR%\bin;%PATH%
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

goto EOF

:need_gnuwin
@echo.
@echo You need flex and bison to build vultures. 
@echo A good place to get them is the gnuwin project
@echo see http://gnuwin32.sourceforge.net/
@echo.


:EOF
