               Readme for Vulture's Eye version 1.9.3
              (NetHack isometric graphics interface)
                  Jaakko Peltonen, July 1, 2001.
		  Clive Crous, 14 April 2005.


    -------------------------------------------------------------
    NOTICE:
     
    All the binary files in this distribution are modified by 
    Jaakko Peltonen. The latest changes to the files are on 
    July 2, 2001.
    -------------------------------------------------------------

1. Introduction

    This file is a brief overview of an isometric graphics 
    interface to NetHack. The interface was called "Falcon's Eye", 
    because the viewpoint resembles a "bird's eye view". This fork
    of the origional "Falcon's Eye" is called "Vulture's Eye"
    because it feeds of the corpse of the now dead "Falcon's Eye".


2. About this binary distribution

    This program is distributed under the NetHack licence. The 
    source code for version 1.9.3 of Falcon's Eye is available 
    from the following web page:

      http://www.hut.fi/~jtpelto2/nethack.html
      
    The source code for version 1.9.3 of Vulture's Eye is
    available from:
    
      http://www.darkarts.co.za/

    This distribution is based on NetHack, version 3.4.3.
    Since the program has been recompiled from modified source 
    code, all the binary files are 'modified' with respect to 
    the 3.4.3 binary distribution.

    This distribution is based on a development version of the
    source code. Thus, it may not be as stable as the eventual
    'final' release. It is meant as a 'preview' of the complete 
    program. Questions and comments are appreciated, see 'Contact
    Information' below.

    This distribution contains a HTML-based game manual. It is 
    in the 'manual' subdirectory. The original NetHack guidebook
    is also there in plain text format. 
 
    To view the manual, open the file 'index.html' in your 
    favourite web browser. The manual uses frames and Style 
    sheets, however it can be viewed without support for them 
    (open 'main.html' instead of 'index.html').


3. System requirements

    Required:
      - 386 or better processor
      - a unix or unix clone (like linux)
      - two-button mouse
      - 16 Mb memory
      - 12 Mb disk space

    Note: I haven't tried the program on a 386 or 486. In 
    practice, the game might be unplayable on a 386.

    Recommended:
      - Pentium 90 or better processor    
      - mouse
      - sound card with General MIDI support for music
      - 16 Mb memory
      - 12 Mb disk space
    
3. Startup options

    Vulture's Eye uses four initialization files, found in 
    the 'config' subdirectory. The files are 'jtp_opts.txt', 
    'jtp_snds.txt', 'jtp_keys.txt' and 'jtp_intr.txt'.
    You can modify them to change the graphics settings and 
    game interface of Vulture's Eye.

    For example, if you want to use a higher in-game resolution, 
    you can change it from 'jtp_opts.txt'. See chapter 9 of the 
    game manual for more information.

4. File organization

    Vulture's Eye has four subdirectories under the NetHack 
    directory:  'graphics' contains external graphics files used
    by the interface. 'sound' contains sound effects and music. 
    'manual' contains the HTML manual and the original Guidebook.
    'config' contains configuration settings and other external 
    data files.
    
    All the files in the 'graphics' and 'sound' subdirectories 
    are copyright (c) Jaakko Peltonen, 2001. Modifying the files
    may cause the game to crash. A guide to how the graphics and 
    sounds are organized will be included later (although the 
    files are mostly self-explanatory). The graphics files are 
    all 256-color PCX files.
    
    [ Clive: Jaakko Peltonen never did release this information,
             it has become defunct with Vulture's Eye however.
	     The plan with Vulture's eye is to move quickly away
	     from 256 colours and into the realm of 24/32 bit ]
    
5. Known problems

    This window port is still in development. No problems that
    hinder gameplay are known. However, some 'annoyances' are 
    known. The webpage

       http://www.hut.fi/~jtpelto2/nethack.html

    contains a link to a bug list for Falcon's Eye, which is
    more or less up-to-date.
    
    For Vulture's Eye bug lists and patches, ETAs on fixes etc
    can all be found at:
    
       http://www.darkarts.co.za/

    If you think you have found a new bug / problem in the 
    window port, please report it to me (see contact information 
    below). Note: bugs concerning NetHack itself (rather than 
    the window port) should be reported to the NetHack 
    development team (devteam@nethack.org).


6. Contact information for Falcon's Eye
    
    E-mail:		jaakko.peltonen@hut.fi
    Regular mail:	Jaakko Peltonen
			Meripoiju 3G 65
			02320 Espoo
			Finland

    The main webpage for Falcon's Eye is

       http://www.hut.fi/~jtpelto2/nethack.html

    There is also a discussion forum for Falcon's Eye; there is 
    a link to it at the webpage.

7. Contact information for Vulture's Eye

	email:		clive@darkarts.co.za
	irc:		irc.freenode.net #darkarts .. I am 'Entro-P'

    The main webpage for Vulture's Eye is

       http://www.darkarts.co.za/

Brand and product names are trademarks or registered trademarks
of their respective holders.

