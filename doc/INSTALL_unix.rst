Installing Vulture's on unix or a unix clone (like linux)
*********************************************************

.. contents::

Required dependancies
=====================

I suggest reading it's documentation for best graphical and audio output, unless of course your system comes with development packages for these.

Simple DirectMedia Layer (SDL)
------------------------------
http://www.libsdl.org/

SDL-mixer
---------
http://www.libsdl.org/projects/SDL_mixer/

This needs to be built and installed with support for:
 - mp3
 - ogg vorbis
 - midi

SDL-image
---------
http://www.libsdl.org/projects/SDL_image/

This needs to be built and installed with support for:
 - pcx (to be depreciated)
 - png

Compiling and installing for system deployment and actual gameplay
==================================================================

The first thing you need to do is create the required symbolic links to the vultures source within the NetHack and the SLASH'EM directory trees.  This is done as follows:
 - ln -s ../../vultures nethack/win/vultures
 - ln -s ../../vultures slashem/win/vultures

Now proceed to each of the variants' installation instruction documents.  You will find them here:
 - nethack/sys/unix/Install.unx
 - slashem/sys/unix/Install.unx

Simply follow the instuctions for each of them to build and install the respective variant on your system.

Compiling and installing for development or experimental purposes
=================================================================

in the source code root directory type "make help" to receive instruction on installing vultures to a location within your home directory.

**NOTE** This form of installation should not be used for general gameplay, unless you are wanting to look at the game briefly and do not want it installed system-wide.

