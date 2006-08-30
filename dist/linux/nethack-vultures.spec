Name:           nethack-vultures
Version:        1.10.1
Release:        0.3
Summary:        NetHack - Vulture's Eye and Vulture's Claw

Group:          Amusements/Games
License:        NetHack General Public License
URL:            http://www.darkarts.co.za/projects/vultures/
Source0:        http://www.darkarts.co.za/projects/vultures/downloads/vultures-%{version}/vultures-%{version}-full.tar.bz2
Patch0:         %{name}-1.10.1-optflags.patch
Patch1:         %{name}-1.10.1-config.patch
Patch2:         %{name}-1.10.1-clawguide.patch
Patch3:         %{name}-1.10.1-log2stderr.patch
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  SDL-devel
BuildRequires:  SDL_mixer-devel
BuildRequires:  ncurses-devel
BuildRequires:  byacc
BuildRequires:  flex
BuildRequires:  desktop-file-utils
BuildRequires:  groff
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
%patch3
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
for f in graphics music sound ; do
    rm -r $RPM_BUILD_ROOT%{_prefix}/games/vulturesclaw/$f
    ln -s ../vultureseye/$f \
        $RPM_BUILD_ROOT%{_prefix}/games/vulturesclaw/$f
done

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
%doc slashem/readme.txt slashem/history.txt slashem/slamfaq.txt vultures/win/jtp/gamedata/manual/
%{_bindir}/vultures*
%dir %{_prefix}/games/vultureseye/
%{_prefix}/games/vultureseye/config/
%{_prefix}/games/vultureseye/defaults.nh
%{_prefix}/games/vultureseye/graphics/
%{_prefix}/games/vultureseye/license
%{_prefix}/games/vultureseye/music/
%{_prefix}/games/vultureseye/nhdat
%{_prefix}/games/vultureseye/sound/
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
