========================================
Release strategy and plans for Vulture's
========================================

.. contents::

.. warning:: This document is incredibly out of date

release strategy
================

The following issues marked under each version, when complete, will trigger the version release.

1.10.0
------

=====     ==================================================================
ID        Description
=====     ==================================================================
`1`_      mouse scroll wheel should scroll scrollbars in windows
`8`_      holding in left mouse button causes insane amount of screen
          refreshes
`9`_      holding in left mouse button makes character walk real fast, delay
          is ignored
`11`_     split directory sound/ into sound/ and music/
`12`_     all music files need 'whitespace' removal from the start and the
`13`_     MENU\ _ commands need to be supported
`14`_     standard key assignments to be used
`15`_     non ext_cmd to be supported, but not as the default behavior
          end
`19`_     compilation instructions
`21`_     Cannot drop only 3 of 6 the same items
`20`_     No indication of being in the Gnomish Mines
`24`_     CapsLock / NumLock brings up the help menu
`28`_     vultures eye hangs/crashes on SUSE Linux 9.3 / AMD64
=====     ==================================================================
 
2.0.0 
-----

=====     ==================================================================
ID        Description
=====     ==================================================================
`3`_      grouping tiles should have unique graphic
`4`_      tiles required for unexplored areas
`7`_      Use the "m" key to pop the map!
=====     ==================================================================

unallocated
-----------

=====     ==================================================================
ID        Description
=====     ==================================================================
`26`_     Unable to save the game, or having to press <ctrl> "S" to do so.
`27`_     Using the small SDL.DLL instead of the huge one deforms sound
=====     ==================================================================

.. _1: http://www.diguru.com/mantis/view.php?id=1
.. _3: http://www.diguru.com/mantis/view.php?id=3
.. _4: http://www.diguru.com/mantis/view.php?id=4
.. _7: http://www.diguru.com/mantis/view.php?id=7
.. _8: http://www.diguru.com/mantis/view.php?id=8
.. _9: http://www.diguru.com/mantis/view.php?id=9
.. _10: http://www.diguru.com/mantis/view.php?id=10
.. _11: http://www.diguru.com/mantis/view.php?id=11
.. _12: http://www.diguru.com/mantis/view.php?id=12
.. _13: http://www.diguru.com/mantis/view.php?id=13
.. _14: http://www.diguru.com/mantis/view.php?id=14
.. _15: http://www.diguru.com/mantis/view.php?id=15
.. _17: http://www.diguru.com/mantis/view.php?id=17
.. _18: http://www.diguru.com/mantis/view.php?id=18
.. _19: http://www.diguru.com/mantis/view.php?id=19
.. _20: http://www.diguru.com/mantis/view.php?id=20
.. _21: http://www.diguru.com/mantis/view.php?id=21
.. _24: http://www.diguru.com/mantis/view.php?id=24
.. _25: http://www.diguru.com/mantis/view.php?id=25
.. _26: http://www.diguru.com/mantis/view.php?id=26
.. _27: http://www.diguru.com/mantis/view.php?id=27
.. _28: http://www.diguru.com/mantis/view.php?id=28

Version numbering
=================

Version numbering format
------------------------

a.b.c_d

What does this mean
-------------------

 - 'a': Version number
      If this increments then *major* changes are underway.
 - 'b': Release number
      This should increment for each standard release which includes
      many feature additions media addons etc.  If this number is an
      odd number, dont expect the game to be stable.  It means you
      have a development release.
 - 'c': Patch level
      This is just an indication of the bugfix 'level' the release is
      at.  If this has increased it means no new features where added,
      just some bugfixes (unless of course a bugfix required a feature
      addition)
 - 'd': Package number
      This increments if there was a mistake made while packaging. No
      bugs where fixed, no features where added, the packager just
      fixed an installation error or any other packaging related
      problem.

      The only exception being any 1.9.3_* packages.  This is just
      after the `Falcon's Eye`_ fork and has many sequential package
      releases

Useful references
=================

Primary vultures eye development website : http://www.darkarts.co.za/projects/vultures/

Vulture's forums, bug-tracking and download mirror : http://www.diguru.com/

.. _`Falcon's Eye`: http://www.diguru.com/mantis/view.php?id=1

