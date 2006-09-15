%define nhgamedir %{_libdir}/games/nethackdir
%define nhdatadir %{_localstatedir}/lib/games/nethack
%define with_tty  %{!?_without_tty:1}%{?_without_tty:0}

Name:           nethack-vultureseye
Version:        1.9.4
Release:        4.a
Epoch:          0
Summary:        NetHack - Vulture's Eye

Group:          Amusements/Games
License:        NetHack General Public License
URL:            http://www.darkarts.co.za/projects/vultures/
Source0:        http://www.darkarts.co.za/projects/vultures/downloads/_current/vultures-1.9.4-eye.tar.bz2
Source1:        vultureseye.desktop
Source2:        vulturesclaw.desktop
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  SDL-devel >= 0:1.2.2, byacc, flex, desktop-file-utils
BuildRequires:  ImageMagick
BuildRequires:  python-docutils
BuildRequires:  tetex-latex
%if %{with_tty}
BuildRequires:  ncurses-devel
%endif
Requires:       /usr/bin/timidity, /usr/bin/bzip2
Provides:       nethack = %{epoch}:3.4.1

%description
Vulture's Eye is a mouse-driven interface for NetHack that enhances the
visuals, audio and accessibility of the game, yet retains all the
original gameplay and game features.  Vulture's Eye is based on Falcon's Eye,
but is greatly extended.   Also included is Vulture's Claw, which is based
on the Slash'Em core.

See %{_docdir}/%{name}-%{version}/nethack.rc and
%{nhgamedir}/defaults.nh for examples
of your personal ~/.nethackrc, and the system-wide configuration in
%{nhgamedir}/config.

Use the "--without tty" option to rpmbuild if you wish to compile a
version without TTY support.


%prep
%setup -q -n vultures

perl -pi -e \
  's|/usr/games/lib/nethackdir\b|%{nhgamedir}|g' \
  doc/nethack.6 \
  doc/nethack.txt \
  doc/recover.6 \
  doc/recover.txt \
  include/config.h

perl -pi -e \
  's|^.*(\#define\s+VAR_PLAYGROUND\s).+$|${1}"%{nhdatadir}"|' \
  include/unixconf.h

perl -pi -e 's|^(\#define\s+JTP_LOG_FILENAME\s+\")[^\"]+(\".*)$|${1}%{_localstatedir}/log/nethack/jtp_log.txt${2}|' vultures/win/jtp/jtp_gen.c

%if %{with_tty}
perl -pi -e 's|^.*(\#define\s+TTY_GRAPHICS).*$|$1|' include/config.h
perl -pi -e \
  's|^(WINSRC\s*=\s*)(.+)$|$1\$(WINTTYSRC) $2|;
   s|^(WINOBJ\s*=\s*)(.+)$|$1\$(WINTTYOBJ) $2|;
   s|^(WINLIB\s*=\s*)(.+)$|$1\$(WINTTYLIB) $2|' \
  sys/unix/Makefile.src
%endif

# Fix line terminators of config and text files.
perl -pi -e 's|\r\n|\n|g' win/jtp/gamedata/*.txt win/jtp/gamedata/config/*.txt


%build
# sh ./sys/unix/setup.sh links
make -e home VEINSTALLDIR=$RPM_BUILD_ROOT%{nhgamedir} VCINSTALLDIR=$RPM_BUILD_ROOT%{nhgamedir}
make %{?_smp_mflags} -C nethack/util recover dlb
make %{?_smp_mflags} -C slashem/util recover dlb
convert vultures/win/jtp/gamedata/VulturesEye.ico VulturesEye.png
convert vultures/win/jtp/gamedata/VulturesClaw.ico VulturesClaw.png


%install
rm -rf $RPM_BUILD_ROOT

%makeinstall \
  CHOWN=/bin/true \
  CHGRP=/bin/true \
  GAMEDIR=$RPM_BUILD_ROOT%{nhgamedir} \
  VARDIR=$RPM_BUILD_ROOT%{nhdatadir} \
  SHELLDIR=$RPM_BUILD_ROOT%{_bindir} \
  JTPMANUALDIR=$(pwd)/manual \
  GAMEPERM=0755 \
  FILEPERM=0644 \
  EXEPERM=0755 \
  DIRPERM=0755

perl -pi -e "s|$RPM_BUILD_ROOT||" $RPM_BUILD_ROOT%{_bindir}/nethack

install -d -m 0755 $RPM_BUILD_ROOT%{_mandir}/man6
make -C doc MANDIR=$RPM_BUILD_ROOT%{_mandir}/man6 INSTALL_unix

mkdir -p $RPM_BUILD_ROOT%{nhgamedir}
install -p -m 0755 nethack/util/recover nethack/util/dgn_comp nethack/util/lev_comp nethack/util/dlb \
  $RPM_BUILD_ROOT%{nhgamedir}

install -p -m 0755 slashem/util/recover slashem/util/dgn_comp slashem/util/lev_comp slashem/util/dlb \
  $RPM_BUILD_ROOT%{nhgamedir}

install -p -m 0755 nethack/compiled/games/lib/vultureseyedir/vultureseye \
  $RPM_BUILD_ROOT%{_bindir}

install -p -m 0755 slashem/compiled/games/lib/vulturesclawdir/vulturesclaw \
  $RPM_BUILD_ROOT%{_bindir}

install -d -m 0775 $RPM_BUILD_ROOT%{_localstatedir}/log/nethack
> $RPM_BUILD_ROOT%{_localstatedir}/log/nethack/jtp_log.txt

ln -s %{nhgamedir}/license .

desktop-file-install \
  --vendor fedora \
  --mode 0644 \
  --dir $RPM_BUILD_ROOT%{_datadir}/applications \
  --add-category X-Fedora \
  %{SOURCE1}
install -D -p -m 0644 vultures/win/jtp/gamedata/VulturesEye.png \
  $RPM_BUILD_ROOT%{_datadir}/pixmaps/VulturesEye.png

desktop-file-install \
  --vendor fedora \
  --mode 0644 \
  --dir $RPM_BUILD_ROOT%{_datadir}/applications \
  --add-category X-Fedora \
  %{SOURCE2}
install -D -p -m 0644 vultures/win/jtp/gamedata/VulturesClaw.png \
  $RPM_BUILD_ROOT%{_datadir}/pixmaps/VulturesClaw.png


%clean
rm -rf $RPM_BUILD_ROOT


%post
touch %{_localstatedir}/log/nethack/jtp_log.txt
chown root.games %{_localstatedir}/log/nethack/jtp_log.txt
chmod 0664 %{_localstatedir}/log/nethack/jtp_log.txt


%files
%defattr(-,root,root,-)
%doc doc/Guidebook doc/fixes* README manual vulture.txt license
%doc win/X11/nethack.rc win/jtp/gamedata/readme.txt
%{_bindir}/nethack
%dir %{nhgamedir}
%dir %{nhgamedir}/config
%{nhgamedir}/config/jtp_intr.txt
%{nhgamedir}/config/jtp_lit1.dat
%config %{nhgamedir}/config/jtp_keys.txt
%config %{nhgamedir}/config/jtp_opts.txt
%config %{nhgamedir}/config/jtp_snds.txt
%{nhgamedir}/[A-Zabdefghjklmopqrstvw]*
%{nhgamedir}/c[am]*
%{_mandir}/*/*
%{_datadir}/pixmaps/*
%{_datadir}/applications/*
%defattr(0664,root,games,0775)
%dir %{_localstatedir}/log/nethack
%ghost %{_localstatedir}/log/nethack/jtp_log.txt
%defattr(0660,root,games,0770)
%attr(2755,root,games) %{nhgamedir}/nethack
%config(noreplace) %{nhdatadir}


%changelog
* Fri Sep 16 2005 Karen Pease <meme@daughtersoftiresias.org> - 0:1.9.4-0.fdr.4.a
- Adapted the specfile for Vulture's Eye; adding in the previous release's "done" list
-   Changelog:
- automated building, and better versioning for it 
- ignore make clean failures 
- supporting code for new graphics 
- new graphics 
- upgrade from slashem 0.0.7e7 development, to 0.0.7e7f1 stable 
- new graphics 
- typo correction in README 
- supporting code for new graphics 
- dont remove html docs, as they are usefull online 
- code changes for new graphics 
- new graphics 
- oops, forgot to copy the music 
- added release strategy as plan within /doc 
- rename .txt to .rst within docs 
- legacy commented out code, unused 
- seperate music directory 
- aliasing for oggenc 
- create a new repository for enhanced media 
- move music data 
- fix the issue of items still showing even if picked up 
- fix the DISPLAY_LAYERS issue, but not perfectly, see full note 
- html footer for build-docs 
- add contents to unix build doc 
- should never clean by default 
- more complete doc building 
- build doc, referal stubs 
- unix build documentation 
- fix #17: Incorrect build instructions in the Makefile 
- oops, forgot to add versioning template 
- updated dll tree for win32 
- build flags for win32 versioning 
- better executable naming for the variants 
- easier versioning 
- removal of version number mentioning in all docs 
- for some reason it was unlocking, but never actually locking to start with 
- ensure pallette restoration after splash or intro 
- update the nethack logo for compatibility 
- script to make oggs from mids 
- remove TODO, all moved to mantis 
- out with the old, in with the new (icons) 
- new icons 
- start playing intro music just a wee bit earlier 
- get caption ready for icon inclusion 
- allow music to be switched off in options file 
- slashem didn't know about the ogg 
- allow win32 builds access to sounds and music 
- blank screen after pallette refresh 
- enforce name asking, this is primarily a graphical interface afterall 
- reminder in TODO, regarding Claw sfx 
- add reminder to TODO about music editing, the delay irritates me 
- pallete restoration side-effect removed 
- temporarily add a TODO file, till i can move all the info into mantis 
- redundant options removed 
- better looking default options 
- SDL_mixer for win32 builds 
- the almighty sound system move, sdl_mixer now required, again this will break win32 builds until tweaked 
- version id is reliant on date.h 
- windows build should use moved, globalised, common, include location :) 
- use port id as window caption 
- better (but not best) port id 
- hilite tame monsters (pets) 
- tile special properties required to be available at runtime 
- generalise some more variant specific code/comments/text 
- move vultures include files out of variant trees into common area where they belong.- NOTE: This change will break non-unix builds untill they too are tweaked. 
- ensure that a background (floor) tile is always drawn 
- added a TODO marker, regarding passage drawing 
- it's only used by map drawing functions jaakko 
- friendlier foonix devmake 
- rampant ^M removal 
- fix issue with logo not displaying in win32 builds 
- make damn sure windows users dont switch on the sound and incinerate the multiverse in the process 
- the quick hack to allow windows version to compile, should not remove foonix users' sound 
- remove artifacts support for now, it is not mature enough a system yet 
- support for DISPLAY_LAYERS is still incomplete, remove as the default 
- default lit_corridor to off until claw tile issue can be resolved 
- visible ingame versioning 
- it should build, regardless of cleanups' success 
- add vultures to the standard win32 gcc build for slashem 
- add vultures to the standard win32 gcc build for nethack 
- updated splash screens 
- due to falcons eye brokenness accross platforms disable music and sound by default until I can stabilise this, must be done prior to the next release 
- how did that get in there ? 
- there is no need for tty to be built in slashem either 
- avoid those ugly win32 build messages for dup DLB 
- win32 build files 
- required win32 dll 
- removal of an odd piece of code that depended on tty being there 
- vultures shouldn't bother with tty aswell, it's redundant 
- tentatively remove system dependant code 
- update artifacts' artifacts to be compliant with new artifacts version 
- closed artifact #3 
- another nethack graphics patch 
- remove win32 style newlines from jtp_win.c 
- add artifact #7 
- artifact adjustment (#6) to ensure correct format for null assignment, see artifacts' artifact #17 
- add artifact #6 
- add artifact #5 and some priority 
- remove last known vestages of directx functionality 
- completed and closed artifact #4 
- add artifact #4 
- artifacts default command is an enduser setting and should not be imposed by the project 
- added artifact #3 
- better dev building 
- added artifact #2 
- added artifact #1 
- initialise artifacts 
- some new nethack graphics 
- layers uses cmaps not glyphs 
- add preliminary Vulture's Claw intro image 
- cater for differing Vulture's Claw logo 
- use memory_glyph for layers 
- Guidebook.txt is generated, remove from repo 
- code alignment issues, more visible nesting 
- FreeBSD 
- compilation line tweak 
- ensure Makefile creation within metaMakefile 
- Alchemy Smock does not appear in Slash'EM 
- meta Makefile for development ease 
- Merge Vulture's Claw repository (Slash'EM 0.0.7E7) 
- prepare for the claw

* Sun Nov  2 2003 Ville Skyttä <ville.skytta at iki.fi> - 0:1.9.4-0.fdr.4.a
- Add fix for bug C341-13.

* Sun Aug 17 2003 Ville Skyttä <ville.skytta at iki.fi> - 0:1.9.4-0.fdr.3.a
- Drop NPTL patch, doesn't seem necessary with 1.9.4.
- Tightened file permissions, no setgid during install.
- Drop explicit stripping of the game binary (no setgid allows rpm to do it).
- Add fix for bug C341-53.
- Move manual to doc dir.

* Fri Jul 25 2003 Ville Skyttä <ville.skytta at iki.fi> - 0:1.9.4-0.fdr.2.a
- Update to 1.9.4a.

* Sat Jul 12 2003 Ville Skyttä <ville.skytta at iki.fi> - 0:1.9.4-0.fdr.2
- Approaching FHS (not quite there yet).

* Fri Jul 11 2003 Ville Skyttä <ville.skytta at iki.fi> - 0:1.9.4-0.fdr.1
- Update to (unofficial) 1.9.4.
- Major spec and patch cleanup.
- Drop QT version.

* Sat Apr  5 2003 Ville Skyttä <ville.skytta at iki.fi> - 0:1.9.3-0.fdr.3
- Require /usr/bin/timidity, the game doesn't work without a MIDI player.
- Fix Source0 URL (#15 comment 11).
- Save .spec in UTF-8.

* Sun Mar 23 2003 Ville Skyttä <ville.skytta at iki.fi> - 0:1.9.3-0.fdr.2
- Use LD_ASSUME_KERNEL=2.2.5 in startup script (#15).
- Install icon to %%{_datadir}/pixmaps (#15).

* Sat Mar 22 2003 Ville Skyttä <ville.skytta at iki.fi> - 0:1.9.3-0.fdr.1
- Update to current Fedora guidelines.

* Thu Feb 20 2003 Ville Skyttä <ville.skytta at iki.fi> - 1.9.3-1.fedora.1
- First Fedora release.

* Sat Oct 19 2002 Ville Skyttä <ville.skytta at iki.fi> 2:1.9.3-2cr
- Fix config dir permissions, thanks to roi for the heads up.

* Sun Oct  6 2002 Ville Skyttä <ville.skytta at iki.fi> 2:1.9.3-1cr
- Rebuild for Red Hat 8.0 using compat GCC.
- Add desktop icon.

* Sat Jun  8 2002 Ville Skyttä <ville.skytta at iki.fi> 2:1.9.3-0.1
- Rebuilt with RedHat 7.3.

* Thu May  2 2002 Ville Skyttä <ville.skytta at iki.fi> 1:1.9.3-cr4
- Added XFree86-devel build requirement.
- Rebuild with QT 2.3.2.
- Some spec cleanups.

* Fri Apr 19 2002 Ville Skyttä <ville.skytta at iki.fi> 1:1.9.3-cr3
- Added vultureseye-savegame.patch, thanks to Patrick Gosling.
- Some specfile cleanups.

* Mon Nov 12 2001 Ville Skyttä <ville.skytta at iki.fi> 1.9.3-cr2
- added patches 1-5 (bugs C331-{4,30,51}, SC331-{3,4,6})
- better %doc handling

* Sun Nov 11 2001 Ville Skyttä <ville.skytta at iki.fi> 1.9.3-cr1
- removed explicit requirements for smoother experience on Mandrake; still
  need --nodeps to build on MDK, I guess
- added Serial to allow upgrade due to release tag change (vsX -> crX)
- tweaked %post and %postun to happen only on install and erase
- spec cleanups

* Sat Oct 27 2001 Ville Skyttä <ville.skytta at iki.fi> 1.9.3-vs10
- added ncurses-devel, byacc and flex to build requirements
- recompiled in redhat 7.2

* Tue Aug 21 2001 Ville Skyttä <ville.skytta at iki.fi>
- better config file handling
- recompiled with SDL-1.2.2-2 from rawhide
- release 1.9.3-vs9

* Wed Aug 15 2001 Ville Skyttä <ville.skytta at iki.fi>
- use mpg321 instead of mpg123 by default
- release 1.9.3-vs8

* Sat Jul 14 2001 Ville Skyttä <ville.skytta at iki.fi>
- recompiled with SDL-1.2.1-4
- release 1.9.3-vs7

* Wed Jul 11 2001 Ville Skyttä <ville.skytta at iki.fi>
- added some post-installation config notes (suggestion by Kormac)
- added some comments about KDE/artsd in jtp_opts.txt
- updated patch location again
- release 1.9.3-vs6

* Sun Jul  8 2001 Ville Skyttä <ville.skytta at iki.fi>
- updated patch location
- release 1.9.3-vs5
