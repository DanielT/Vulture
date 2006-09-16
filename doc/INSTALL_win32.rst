Building Vulture's on Windows
*****************************

.. contents::

Note: In the following instructions you'll see drive D as the installation disk, because
that is what I used locally. Obviously you can use a different one. However you need to put
mingw, msys and the vultures source on the same drive, because the Makefile in vultures makes
that assumption.

Required dependancies
=====================

MinGW
-----

`download MinGW`_

The mingw file is an installer that will download various mingw packages for you. It will ask where
to intall to; the default is C:\\mingw. Change the drive if you want to, but the path \\mingw is hardcoded
in the vultures Makefile (unless you want to change that too, in which case any path without spaces will
work) 
After that you will get to select your compilers. C is default and isn't shown. You need to select C++ here.
I also selected make, but I didn't test to see wether that is essential. You don't need ObjC, Java, Ada, etc
unless you plan to use mingw for other things later on.

msys
----

`download msys`_

Also an installer. Again change only the drive letter, the path \\msys\\1.0 is hardcoded in the Makefile.
You will be asked wether you want a postinstall script to run; say yes (It asks a couple of easy questions,
but I can't remember the details right now...)
Now create the directories D:\\msys\\1.0\\d and D:\\msys\\1.0\\src.
Next edit D:\\msys\\1.0\\etc\\fstab; it should contain one line that mounts the mingw installation directory
in your msys installation. Msys left out the slash after the drive, add it back; you need it.
Also add a line "d:/ /d" to get access to your d: drive.
fstab sould now look like this:
d:/mingw /mingw
d:/ d/

SDL
---

`download SDL`_

Unpack sdl to D:\\msys\\1.0\\src\\. Start msys and cd to the SDL source directory (within msys it will appear to
be /usr/src/SDL-1.2.9)
enter this command to compile and install SDL into your mingw directory:
./configure --prefix=/mingw && make && make install

SDL_image and dependencies.
---------------------------
SDL_image depends on zlib and libpng, so we need to install those first.

zlib
~~~~

`download zlib`_

Unpack the zlib file anywhere on d:. You will have noticed that it is very small; it is actually just a set
of scripts that will download and compile zlib for you.
In msys cd into your new directory (/d/<whatever>/zlib-1.2.3/mingwPORT/) and run
./mingwPORT.sh
The script will ask a number of questions which you can answer by just pressing enter to select the default.
Once it has completed you have zlib installed.

libpng
~~~~~~

`download libpng`_

This is also a set of mingwPORT scripts; you again unpack and run them to get libpng.

SDL_image
~~~~~~~~~

`download SDL_image`_

We now have the bare essentials for SDL_image. Beware, the SDL_image we are about to build will NOT support jpeg,
because we did not install libjpeg. Similarly other formats that vultures does not use will aslo not be supported.
Of course if the only thing you're compiling SDL_image for is vultures that doesn't matter.
Unpack SDL_image-1.2.4.tar.gz into D:\\msys\\1.0\\src\\. Cd into the directory within msys (/usr/src/SDL_image-1.2.4)
Run
./configure --prefix=/mingw && make && make install
to compile and install SDL_image

SDL_mixer and dependencies
--------------------------
The minimum dependencies of SDL_mixer are libogg and libvorbis.

libogg
~~~~~~

`download libogg`_

Unpack libogg-1.1.2.tar.gz into D:\\msys\\1.0\\src\\. Cd into the new directory. Run
./configure --prefix=/mingw && make && make install
to compile and install libogg into your mingw directory.

libvorbis
~~~~~~~~~

`download libvorbis`_

As before unpack the file, cd into the directory and (for a change) run:
LDFLAGS="-logg" ./configure --prefix=/mingw && make && make install
Without the LDFLAGS linking inexplicably failed on my system. Weird...

SDL_mixer
~~~~~~~~~

`download SDL_mixer`_

Same as SDL_image; unpack, cd, run:
./configure --prefix=/mingw && make && make install


Vultures
========

You now have the prerequisites for building vultures.
Unpack the vultures sourcecode on D:
Get yourself a normal wondows command shell by selecting "Run" on your start menu and typing "cmd".
(Yes, the vultures build does NOT run inside msys)
cd into the topmost directory of the vultures source; it contains mingw-make-nethack.bat and mingw-make-slashem.bat
Run either or both of these to build vultureseye and/or vuluresclaw.

.. _download MinGW: http://prdownloads.sourceforge.net/mingw/MinGW-5.0.0.exe?download
.. _download msys: http://prdownloads.sourceforge.net/mingw/MSYS-1.0.9.exe?download
.. _download SDL: http://www.libsdl.org/release/SDL-1.2.9.tar.gz
.. _download SDL_image: http://www.libsdl.org/projects/SDL_image/release/SDL_image-1.2.4.tar.gz
.. _download SDL_mixer: http://www.libsdl.org/projects/SDL_mixer/release/SDL_mixer-1.2.6.tar.gz
.. _download libogg: http://downloads.xiph.org/releases/ogg/libogg-1.1.2.tar.gz
.. _download libvorbis: http://downloads.xiph.org/releases/vorbis/libvorbis-1.1.1.tar.gz
.. _download zlib: http://prdownloads.sourceforge.net/mingw/zlib-1.2.3-mingwPORT.tar.bz2?download
.. _download libpng: http://prdownloads.sourceforge.net/mingw/libpng-1.2.8-mingwPORT.tar.bz2?download
