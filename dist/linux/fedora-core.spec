Name:           nethack-vulture
Version:        2.1.0
Release:        7%{?dist}
Summary:        NetHack - Vulture's Eye and Vulture's Claw

Group:          Amusements/Games
License:        NetHack General Public License
URL:            http://www.darkarts.co.za/projects/vulture/
Source0:        http://www.darkarts.co.za/projects/vulture/downloads/vulture-%{version}/vulture-%{version}-full.tar.bz2
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
%setup -q -n vulture-%{version}
%patch0 -p1
%patch1 -p1
%patch2
sed -i -e 's|/usr/games/lib/nethackdir|%{_prefix}/games/vulture-nethack|g' \
    nethack/doc/{nethack,recover}.6 nethack/include/config.h
sed -i -e 's|/var/lib/games/nethack|%{_var}/games/vulture-nethack|g' \
    nethack/include/unixconf.h
sed -i -e 's|/usr/games/lib/nethackdir|%{_prefix}/games/vulture-slashem|g' \
    slashem/doc/{nethack,recover}.6 slashem/include/config.h
sed -i -e 's|/var/lib/games/nethack|%{_var}/games/vulture-slashem|' \
    slashem/include/unixconf.h


%build
# Note: no %{?_smp_mflags} in any of these: various parallel build issues.
for i in nethack slashem ; do
    make $i/Makefile
    make -C $i
    make -C $i/util recover dlb dgn_comp lev_comp
    make -C $i/dat spec_levs quest_levs
    cp vulture/gamedata/graphics/gametiles.bin vulture/gamedata/graphics/gametiles.bin.$i
done


%install
rm -rf $RPM_BUILD_ROOT

make -C nethack install CHGRP=: CHOWN=: \
    GAMEDIR=$RPM_BUILD_ROOT%{_prefix}/games/vulture-nethack \
    VARDIR=$RPM_BUILD_ROOT%{_var}/games/vulture-nethack \
    SHELLDIR=$RPM_BUILD_ROOT%{_bindir}
make -C slashem install CHGRP=: CHOWN=: \
    GAMEDIR=$RPM_BUILD_ROOT%{_prefix}/games/vulture-slashem \
    VARDIR=$RPM_BUILD_ROOT%{_var}/games/vulture-slashem \
    SHELLDIR=$RPM_BUILD_ROOT%{_bindir}

install -dm 755 $RPM_BUILD_ROOT%{_mandir}/man6
install -pm 644 nethack/doc/nethack.6 \
    $RPM_BUILD_ROOT%{_mandir}/man6/vulture-nethack.6
install -pm 644 nethack/doc/recover.6 \
    $RPM_BUILD_ROOT%{_mandir}/man6/vulture-nethack-recover.6
install -pm 644 slashem/doc/nethack.6 \
    $RPM_BUILD_ROOT%{_mandir}/man6/vulture-slashem.6
install -pm 644 slashem/doc/recover.6 \
    $RPM_BUILD_ROOT%{_mandir}/man6/vulture-slashem-recover.6

install -dm 755 $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/48x48/apps
for i in vulture-nethack vulture-slashem ; do
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

rm -r $RPM_BUILD_ROOT%{_prefix}/games/vulture-nethack/manual
rm -r $RPM_BUILD_ROOT%{_prefix}/games/vulture-slashem/manual

# Save some space
for f in music sound ; do
    cp $RPM_BUILD_ROOT%{_prefix}/games/vulture-slashem/$f/* $RPM_BUILD_ROOT%{_prefix}/games/vulture-nethack/$f/
    rm -r $RPM_BUILD_ROOT%{_prefix}/games/vulture-slashem/$f
    ln -s ../vulture-nethack/$f \
        $RPM_BUILD_ROOT%{_prefix}/games/vulture-slashem/$f
done

mv vulture/gamedata/graphics/gametiles.bin.nethack $RPM_BUILD_ROOT%{_prefix}/games/vulture-nethack/graphics/gametiles.bin
mv vulture/gamedata/graphics/gametiles.bin.slashem $RPM_BUILD_ROOT%{_prefix}/games/vulture-slashem/graphics/gametiles.bin

chmod -s $RPM_BUILD_ROOT%{_prefix}/games/vulture*/vulture* # for stripping

# Clean up
sed -i -e "s|$RPM_BUILD_ROOT||" $RPM_BUILD_ROOT%{_bindir}/vulture{eye,claw}
rm $RPM_BUILD_ROOT%{_prefix}/games/vulture*/*.ico

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
%doc slashem/readme.txt slashem/history.txt slashem/slamfaq.txt vulture/gamedata/manual/
%{_bindir}/vulture*
%dir %{_prefix}/games/vulture-nethack/
%{_prefix}/games/vulture-nethack/config/
%{_prefix}/games/vulture-nethack/defaults.nh
%{_prefix}/games/vulture-nethack/graphics/
%{_prefix}/games/vulture-nethack/license
%{_prefix}/games/vulture-nethack/music/
%{_prefix}/games/vulture-nethack/nhdat
%{_prefix}/games/vulture-nethack/sound/
%{_prefix}/games/vulture-nethack/fonts/
%attr(2755,root,games) %{_prefix}/games/vulture-nethack/vulture-nethack
%dir %{_prefix}/games/vulture-slashem/
%{_prefix}/games/vulture-slashem/config/
%{_prefix}/games/vulture-slashem/defaults.nh
%{_prefix}/games/vulture-slashem/graphics/
%{_prefix}/games/vulture-slashem/Guidebook.txt
%{_prefix}/games/vulture-slashem/license
%{_prefix}/games/vulture-slashem/music/
%{_prefix}/games/vulture-slashem/nh*share
%{_prefix}/games/vulture-slashem/sound/
%{_prefix}/games/vulture-slashem/fonts/
%attr(2755,root,games) %{_prefix}/games/vulture-slashem/vulture-slashem
%{_datadir}/applications/*vulture*.desktop
%{_datadir}/icons/hicolor/48x48/apps/vulture*.png
%{_mandir}/man6/vulture*.6*
%defattr(664,root,games,775)
%dir %{_var}/games/vulture-nethack/
%config(noreplace) %{_var}/games/vulture-nethack/record
%config(noreplace) %{_var}/games/vulture-nethack/perm
%config(noreplace) %{_var}/games/vulture-nethack/logfile
%dir %{_var}/games/vulture-nethack/save/
%dir %{_var}/games/vulture-slashem/
%config(noreplace) %{_var}/games/vulture-slashem/record
%config(noreplace) %{_var}/games/vulture-slashem/perm
%config(noreplace) %{_var}/games/vulture-slashem/logfile
%dir %{_var}/games/vulture-slashem/save/


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
- Modified the specfile to duplicate the slash'em contents into the vulture dirs before rm'ing, to fix a missing-file crash

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
