Building Vulture's on Windows
*****************************

.. contents::

Note: In the following instructions you'll see drive D as the installation disk, because
that is what I used locally. Obviously you can use a different one. However you need to put
mingw, msys and the vultures source on the same drive, because the Makefile in vultures makes
that assumption.

A possible cause of failiures during build or installation is the presence of other "unix for windows"
utilities on your path. If you do have something of the sort installed, rename it's installation
diretory before following these instructions to prevent it's tools from being run.
You can change it's name back afterwards.

In particular any shell (bash, tcsh, etc) or (in some cases) install will mess things up.


Required dependencies
=====================

MinGW
-----

`download MinGW`_

The mingw file is an installer that will download various mingw packages for you. It will ask where
to intall to; the default is *C:\\mingw*. Change the drive if you want to, but the path *\\mingw* is hardcoded
in the vultures Makefile (unless you want to change that too, in which case any path without spaces will
work) 
After that you will get to select your compilers. C is default and isn't shown. You need to select C++ here.
I also selected make, but I didn't test to see whether that is essential. You don't need ObjC, Java, Ada, etc
unless you plan to use mingw for other things later on.

msys
----

`download msys`_

Also an installer. Again change only the drive letter, the path *\\msys\\1.0* is hardcoded in the Makefile.
You will be asked wether you want a postinstall script to run; say yes (It asks a couple of easy questions,
but I can't remember the details right now...)
Now create the directories *D:\\msys\\1.0\\d* and *D:\\msys\\1.0\\src*.
Next edit *D:\\msys\\1.0\\etc\\fstab*; it should contain one line that mounts the mingw installation directory
in your msys installation. Msys left out the slash after the drive, add it back; you need it.
Also add a line "d:/ /d" to get access to your d: drive.
fstab sould now look like this: ::

 d:/mingw /mingw
 d:/ /d


SDL
---

`download SDL`_

Unpack sdl to *D:\\msys\\1.0\\src\\*. Start msys and cd to the SDL source directory (within msys it will appear to
be */usr/src/SDL-1.2.9*)
enter this command to compile and install SDL into your mingw directory: ::

 ./configure --prefix=/mingw && make && make install

SDL_image and dependencies.
---------------------------
SDL_image depends on zlib and libpng, so we need to install those first.

zlib
~~~~

`download zlib`_

To build zlib we will use a mingwPORT script. This script requires `wget`_. If you don't have
wget yet, simply download it and unpack it into *D:\\mingw\\bin*.
Unpack zlib-1.2.3-mingwPORT-1.tar.bz2 into *D:\\msys\\1.0\\src\\*. In msys cd
into */usr/src/zlib-1.2.3/mingwPORT/*. Run: ::

 ./mingwPORT.sh

You can answer all questions by simply pressing enter. That will compile and install zlib into your mingw directory.

libpng
~~~~~~

`download libpng`_

Unpack libpng-1.2.8-mingwPORT.tar.bz2 into *D:\\msys\\1.0\\src\\*. Cd into
*/usr/src/libpng-1.2.8/mingwPORT/*. Run::

 ./mingwPORT.sh

to compile and install libpng into your mingw directory.


SDL_image
~~~~~~~~~

`download SDL_image`_

We now have the bare essentials for SDL_image. Beware, the SDL_image we are about to build will NOT support jpeg,
because we did not install libjpeg. Similarly other formats that vultures does not use will also not be supported.
Of course if the only thing you're compiling SDL_image for is vultures that doesn't matter.
Unpack SDL_image-1.2.4.tar.gz into *D:\\msys\\1.0\\src\\*. Cd into the directory within msys (*/usr/src/SDL_image-1.2.4*)
Run::

 ./configure --prefix=/mingw && make && make install

to compile and install SDL_image

SDL_mixer and dependencies
--------------------------
The minimum dependencies of SDL_mixer are libogg and libvorbis.

libogg
~~~~~~

`download libogg`_

Unpack libogg-1.1.2.tar.gz into *D:\\msys\\1.0\\src\\*.  Cd into the new directory.  Run::

 ./configure --prefix=/mingw && make && make install

to compile and install libogg into your mingw directory.

libvorbis
~~~~~~~~~

`download libvorbis`_

As before unpack the file, cd into the directory and (for a change) run::

 LDFLAGS="-logg" ./configure --prefix=/mingw && make && make install

Without the LDFLAGS linking inexplicably fails. Weird...

SDL_mixer
~~~~~~~~~

`download SDL_mixer`_

Same as SDL_image; unpack, cd, run: ::

 ./configure --prefix=/mingw && make && make install


SDL_ttf and its dependency, freetype
------------------------------------

freetype
~~~~~~~~

`download freetype`_

once again unpack, then ::

 ./configure --prefix=/mingw && make && make install

SDL_ttf
~~~~~~~

`download SDL_ttf`_

 ./configure --prefix=/mingw && make && make install



Vultures
========

Building
--------

You now have the prerequisites for building vultures.
Unpack the vultures sourcecode on D:
Get yourself a normal windows command shell by selecting "Run" on your start menu and typing "cmd".
(Yes, the vultures build does NOT run inside msys)

Cd into the topmost directory of the vultures source; it contains *mingw-make-nethack.bat*
and *mingw-make-slashem.bat*

Run either or both of these to build vultureseye and/or vuluresclaw.

Post-build
----------

Once the build-script has completed you'll have a subdirectory called binary in the nethack
and/or slashem directory which contains the runnable game. It also contains the set of dlls
necessary to run the game; however these dlls are generic pre-built dlls that came with the
source code. If you built the library dependencies of vultures with machine-specific optimizations,
you will want to overwrite the dlls with the copies built by you, which are currently located
in *d:\\mingw\\bin*

You can then download `NSIS`_ and use the scripts in dist\win32 to create an installer.



.. _download MinGW: http://prdownloads.sourceforge.net/mingw/MinGW-5.0.2.exe?download
.. _download msys: http://prdownloads.sourceforge.net/mingw/MSYS-1.0.10.exe?download
.. _download SDL: http://www.libsdl.org/release/SDL-1.2.9.tar.gz
.. _download SDL_image: http://www.libsdl.org/projects/SDL_image/release/SDL_image-1.2.4.tar.gz
.. _download SDL_mixer: http://www.libsdl.org/projects/SDL_mixer/release/SDL_mixer-1.2.6.tar.gz
.. _download SDL_ttf: http://www.libsdl.org/projects/SDL_ttf/release/SDL_ttf-2.0.7.tar.gz
.. _download libogg: http://downloads.xiph.org/releases/ogg/libogg-1.1.2.tar.gz
.. _download libvorbis: http://downloads.xiph.org/releases/vorbis/libvorbis-1.1.1.tar.gz
.. _download zlib: http://prdownloads.sf.net/mingw/zlib-1.2.3-mingwPORT-1.tar.bz2?download
.. _download libpng: http://prdownloads.sf.net/mingw/libpng-1.2.8-mingwPORT.tar.bz2?download
.. _download freetype: http://download.savannah.gnu.org/releases/freetype/freetype-2.1.10.tar.bz2
.. _wget: http://xoomer.virgilio.it/hherold/wget-1.10.2b.zip
.. _NSIS: http://nsis.sourceforge.net/Main_Page
