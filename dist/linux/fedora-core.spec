Name:           nethack-vultures
Version:        2.1.0
Release:        7%{?dist}
Summary:        NetHack - Vulture's Eye and Vulture's Claw

Group:          Amusements/Games
License:        NetHack General Public License
URL:            http://www.darkarts.co.za/projects/vultures/
Source0:        http://www.darkarts.co.za/projects/vultures/downloads/vultures-%{version}/vultures-%{version}-full.tar.bz2
Patch0:         %{name}-1.11.0-optflags.patch
Patch1:         %{name}-1.11.0-config.patch
Patch2:         %{name}-1.10.1-clawguide.patch
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  SDL-devel
BuildRequires:  SDL_mixer-devel >= 1.2.6
BuildRequires:  SDL_image-devel
BuildRequires:  SDL_ttf-devel
BuildRequires:  libpng-devel
BuildRequires:  ncurses-devel
BuildRequires:  byacc
BuildRequires:  flex
BuildRequires:  desktop-file-utils
BuildRequires:  groff
BuildRequires:  util-linux
Requires:       /usr/bin/bzip2
Obsoletes:      nethack-falconseye <= 1.9.4-6.a

%description
Vulture's Eye is a mouse-driven interface for NetHack that enhances
the visuals, audio and accessibility of the game, yet retains all the
original gameplay and game features.  Vulture's Eye is based on
Falcon's Eye, but is greatly extended.  Also included is Vulture's
Claw, which is based on the Slash'Em core.


%prep
%setup -q -n vultures-%{version}
%patch0 -p1
%patch1 -p1
%patch2
sed -i -e 's|/usr/games/lib/nethackdir|%{_prefix}/games/vultureseye|g' \
    nethack/doc/{nethack,recover}.6 nethack/include/config.h
sed -i -e 's|/var/lib/games/nethack|%{_var}/games/vultureseye|g' \
    nethack/include/unixconf.h
sed -i -e 's|/usr/games/lib/nethackdir|%{_prefix}/games/vulturesclaw|g' \
    slashem/doc/{nethack,recover}.6 slashem/include/config.h
sed -i -e 's|/var/lib/games/nethack|%{_var}/games/vulturesclaw|' \
    slashem/include/unixconf.h


%build
# Note: no %{?_smp_mflags} in any of these: various parallel build issues.
for i in nethack slashem ; do
    make $i/Makefile
    make -C $i
    make -C $i/util recover dlb dgn_comp lev_comp
    make -C $i/dat spec_levs quest_levs
    cp vultures/gamedata/graphics/gametiles.bin vultures/gamedata/graphics/gametiles.bin.$i
done


%install
rm -rf $RPM_BUILD_ROOT

make -C nethack install CHGRP=: CHOWN=: \
    GAMEDIR=$RPM_BUILD_ROOT%{_prefix}/games/vultureseye \
    VARDIR=$RPM_BUILD_ROOT%{_var}/games/vultureseye \
    SHELLDIR=$RPM_BUILD_ROOT%{_bindir}
make -C slashem install CHGRP=: CHOWN=: \
    GAMEDIR=$RPM_BUILD_ROOT%{_prefix}/games/vulturesclaw \
    VARDIR=$RPM_BUILD_ROOT%{_var}/games/vulturesclaw \
    SHELLDIR=$RPM_BUILD_ROOT%{_bindir}

install -dm 755 $RPM_BUILD_ROOT%{_mandir}/man6
install -pm 644 nethack/doc/nethack.6 \
    $RPM_BUILD_ROOT%{_mandir}/man6/vultureseye.6
install -pm 644 nethack/doc/recover.6 \
    $RPM_BUILD_ROOT%{_mandir}/man6/vultureseye-recover.6
install -pm 644 slashem/doc/nethack.6 \
    $RPM_BUILD_ROOT%{_mandir}/man6/vulturesclaw.6
install -pm 644 slashem/doc/recover.6 \
    $RPM_BUILD_ROOT%{_mandir}/man6/vulturesclaw-recover.6

install -dm 755 $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/48x48/apps
for i in vultureseye vulturesclaw ; do
    desktop-file-install \
        --vendor=fedora \
        --dir=$RPM_BUILD_ROOT%{_datadir}/applications \
        --mode=644 \
        --add-category=X-Fedora \
        dist/unix/desktop/$i.desktop
    mv $RPM_BUILD_ROOT%{_prefix}/games/$i/*.png \
        $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/48x48/apps/$i.png
    mv $RPM_BUILD_ROOT%{_prefix}/games/$i/recover \
        $RPM_BUILD_ROOT%{_bindir}/$i-recover
done

rm -r $RPM_BUILD_ROOT%{_prefix}/games/vultureseye/manual
rm -r $RPM_BUILD_ROOT%{_prefix}/games/vulturesclaw/manual

# Save some space
for f in music sound ; do
    cp $RPM_BUILD_ROOT%{_prefix}/games/vulturesclaw/$f/* $RPM_BUILD_ROOT%{_prefix}/games/vultureseye/$f/
    rm -r $RPM_BUILD_ROOT%{_prefix}/games/vulturesclaw/$f
    ln -s ../vultureseye/$f \
        $RPM_BUILD_ROOT%{_prefix}/games/vulturesclaw/$f
done

mv vultures/gamedata/graphics/gametiles.bin.nethack $RPM_BUILD_ROOT%{_prefix}/games/vultureseye/graphics/gametiles.bin
mv vultures/gamedata/graphics/gametiles.bin.slashem $RPM_BUILD_ROOT%{_prefix}/games/vulturesclaw/graphics/gametiles.bin

chmod -s $RPM_BUILD_ROOT%{_prefix}/games/vultures*/vultures* # for stripping

# Clean up
sed -i -e "s|$RPM_BUILD_ROOT||" $RPM_BUILD_ROOT%{_bindir}/vultures{eye,claw}
rm $RPM_BUILD_ROOT%{_prefix}/games/vultures*/*.ico

%clean
rm -rf $RPM_BUILD_ROOT


%post
[ $1 -eq 1 ] && \
gtk-update-icon-cache -qf %{_datadir}/icons/hicolor &>/dev/null || :

%postun
gtk-update-icon-cache -qf %{_datadir}/icons/hicolor &>/dev/null || :


%files
%defattr(-,root,root,-)
%doc nethack/README nethack/dat/license nethack/dat/history nethack/dat/*help
%doc slashem/readme.txt slashem/history.txt slashem/slamfaq.txt vultures/gamedata/manual/
%{_bindir}/vultures*
%dir %{_prefix}/games/vultureseye/
%{_prefix}/games/vultureseye/config/
%{_prefix}/games/vultureseye/defaults.nh
%{_prefix}/games/vultureseye/graphics/
%{_prefix}/games/vultureseye/license
%{_prefix}/games/vultureseye/music/
%{_prefix}/games/vultureseye/nhdat
%{_prefix}/games/vultureseye/sound/
%{_prefix}/games/vultureseye/fonts/
%attr(2755,root,games) %{_prefix}/games/vultureseye/vultureseye
%dir %{_prefix}/games/vulturesclaw/
%{_prefix}/games/vulturesclaw/config/
%{_prefix}/games/vulturesclaw/defaults.nh
%{_prefix}/games/vulturesclaw/graphics/
%{_prefix}/games/vulturesclaw/Guidebook.txt
%{_prefix}/games/vulturesclaw/license
%{_prefix}/games/vulturesclaw/music/
%{_prefix}/games/vulturesclaw/nh*share
%{_prefix}/games/vulturesclaw/sound/
%{_prefix}/games/vulturesclaw/fonts/
%attr(2755,root,games) %{_prefix}/games/vulturesclaw/vulturesclaw
%{_datadir}/applications/*vultures*.desktop
%{_datadir}/icons/hicolor/48x48/apps/vultures*.png
%{_mandir}/man6/vultures*.6*
%defattr(664,root,games,775)
%dir %{_var}/games/vultureseye/
%config(noreplace) %{_var}/games/vultureseye/record
%config(noreplace) %{_var}/games/vultureseye/perm
%config(noreplace) %{_var}/games/vultureseye/logfile
%dir %{_var}/games/vultureseye/save/
%dir %{_var}/games/vulturesclaw/
%config(noreplace) %{_var}/games/vulturesclaw/record
%config(noreplace) %{_var}/games/vulturesclaw/perm
%config(noreplace) %{_var}/games/vulturesclaw/logfile
%dir %{_var}/games/vulturesclaw/save/


%changelog
* Fri Sep 15 2006 Karen Pease <meme@daughtersoftiresias.org> - 2.1.0-7
- Rebuild

* Tue Aug 29 2006 Karen Pease <meme@daughtersoftiresias.org> - 2.1.0-6
- Attempting to stop graphics duplication.

* Thu Aug 24 2006 Karen Pease <meme@daughtersoftiresias.org> - 2.1.0-5
- Attempting to stop graphics duplication.

* Wed Aug 16 2006 Karen Pease <meme@daughtersoftiresias.org> - 2.1.0-4
- Attempting to stop graphics duplication.

* Sun Aug 13 2006 Karen Pease <meme@daughtersoftiresias.org> - 2.1.0-3
- Attempting to stop graphics duplication.

* Mon Jun 26 2006 Karen Pease <meme@daughtersoftiresias.org> - 2.1.0-2
- Dealt with the gametiles.bin eye bug not present in claw.

* Thu Jun 08 2006 Karen Pease <meme@daughtersoftiresias.org> - 2.1.0-1
- Upgraded patches 2.1.1

* Wed Jun 07 2006 Karen Pease <meme@daughtersoftiresias.org> - 2.1.0-0
- Upgraded to 2.1.0

* Fri Apr 14 2006 Karen Pease <meme@daughtersoftiresias.org> - 2.0.0-5
- Upped the release tag to keep up with FC-3

* Sun Apr 09 2006 Karen Pease <meme@daughtersoftiresias.org> - 2.0.0-3
- Packaged extra fonts

* Sun Apr 09 2006 Karen Pease <meme@daughtersoftiresias.org> - 2.0.0-2
- Upped the release to try and make the plague server use the right source tarball.

* Sat Apr 08 2006 Karen Pease <meme@daughtersoftiresias.org> - 2.0.0-1
- Upgraded to 2.0.0

* Sun Mar 01 2006 Karen Pease <meme@daughtersoftiresias.org> - 1.11.2-5
- Rebuilt for FC5

* Sun Feb 02 2006 Frank Arnold <frank@scirocco-5v-turbo.de> - 1.11.2-4
- Got a working plague build by working around util-linux bug #176441.

* Sun Jan 08 2006 Karen Pease <meme@daughtersoftiresias.org> - 1.11.2-3
- To fix a strange error on the plague server, added a req for util-linux.

* Sun Jan 08 2006 Karen Pease <meme@daughtersoftiresias.org> - 1.11.2-2
- Upped revision to try to get package to build on the server.

* Fri Jan 06 2006 Karen Pease <meme@daughtersoftiresias.org> - 1.11.2-1
- Upgraded the tarball to the latest version.

* Fri Dec 23 2005 Karen Pease <meme@daughtersoftiresias.org> - 1.11.1-3
- Modified the specfile to duplicate the slash'em contents into the vultures dirs before rm'ing, to fix a missing-file crash

* Wed Dec 21 2005 Karen Pease <meme@daughtersoftiresias.org> - 1.11.1-2
- Upped revision to try to get package to build on the server.

* Tue Dec 20 2005 Karen Pease <meme@daughtersoftiresias.org> - 1.11.1-1
- Upgraded source package; fixes some keyboard bugs and problems for 64bit/big endian machines concerning transparency.

* Thu Dec 15 2005 Karen Pease <meme@daughtersoftiresias.org> - 1.11.0-8
- Forgot to relocate moved docs for postbuild.

* Thu Dec 15 2005 Karen Pease <meme@daughtersoftiresias.org> - 1.11.0-7
- Apparently we're using libpng-devel now also (nobody told me)

* Thu Dec 15 2005 Karen Pease <meme@daughtersoftiresias.org> - 1.11.0-6
- SDL image devel required for build to complete properly.

* Thu Dec 15 2005 Karen Pease <meme@daughtersoftiresias.org> - 1.11.0-6
- SDL image devel required for build to complete properly.

* Thu Dec 15 2005 Karen Pease <meme@daughtersoftiresias.org> - 1.11.0-5
- That patch was fixed, but.. the folly of not checking all patches  :P

* Thu Dec 15 2005 Karen Pease <meme@daughtersoftiresias.org> - 1.11.0-4
- Once again with the patch - ah, the folly of doing diffs by hand.  Last error.

* Thu Dec 15 2005 Karen Pease <meme@daughtersoftiresias.org> - 1.11.0-3
- Didn't quite get that patch right.

* Thu Dec 15 2005 Karen Pease <meme@daughtersoftiresias.org> - 1.11.0-2
- Forgot to update the patches previously; done.

* Thu Dec 15 2005 Karen Pease <meme@daughtersoftiresias.org> - 1.11.0-1
- Upgraded the tarball to the latest release
- Upped the version
- Removed a patch that's now part of the source

* Mon Nov 21 2005 Karen Pease <meme@daughtersoftiresias.org> - 1.10.1-1
- Made it so I don't have to manually tinker with revisions between dists
- Using a 1.x release
- Removed excess tarball

* Mon Nov 21 2005 Karen Pease <meme@daughtersoftiresias.org> - 1.10.1-0.5
- Upped revision in order to make tag

* Mon Nov 21 2005 Karen Pease <meme@daughtersoftiresias.org> - 1.10.1-0.3
- Applied patch 3 (log2stderr)

* Tue Nov 16 2005 Karen Pease <meme@daughtersoftiresias.org> - 1.10.1-0.2
- Upped revision
- Removed timidity++ dep
- Fixed manual installation
- Put stderr patch back in.

* Tue Nov 15 2005 Karen Pease <meme@daughtersoftiresias.org> - 1.10.1-0.1
- Took over maintainership of package
- Handled TODOs

* Tue Nov 15 2005 Ville Skyttä <ville.skytta at iki.fi> - 1.10.1-0.1
- 1.10.1, log crash fix applied upstream.

* Mon Nov  7 2005 Ville Skyttä <ville.skytta at iki.fi> - 1.10.0-0.1
- First build, based on my, Karen Pease's and Luke Macken's related work.
