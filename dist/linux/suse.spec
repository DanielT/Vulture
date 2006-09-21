Name:           nethack-vultures
Version:        2.1.0
Release:        0
Summary:        NetHack - Vulture's Eye and Vulture's Claw

Group:          Amusements/Games
License:        NetHack General Public License
URL:            http://www.darkarts.co.za/projects/vultures/
Source0:        http://www.darkarts.co.za/projects/vultures/downloads/vultures-%{version}/vultures-%{version}-full.tar.bz2
Patch0:         suse-nethack-config.patch
Patch1:         suse-nethack-decl.patch
Patch2:         suse-nethack-gzip.patch
Patch3:         suse-nethack-misc.patch
Patch4:         disable-pcmusic.diff
Patch5:         suse-nethack-syscall.patch
Patch6:         suse-nethack-yacc.patch
Patch7:         suse-nethack-gametiles.patch
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  SDL-devel
#%if %{?suse_version:1}0
#%endif
%if 0%{?suse_version}
%if %suse_version  > 930
BuildRequires:  SDL_mixer-devel
%else
BuildRequires:  SDL_mixer
%endif
%else
BuildRequires:  SDL_ttf-devel
BuildRequires:  SDL_mixer-devel
%endif
%if 0%{?suse_version}
BuildRequires:  bison
%endif
%if 0%{?fedora_version}
BuildRequires:  byacc
%endif
BuildRequires:  ncurses-devel
BuildRequires:  flex
BuildRequires:  desktop-file-utils
BuildRequires:  groff
BuildRequires:  libpng-devel SDL_image-devel SDL_ttf
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
%if %{?suse_version:1}0
%if %suse_version
%patch0 
%patch1
%patch2
%patch3
%patch5
%endif
%if %suse_version <= 930
%patch4
%endif
%patch6
%endif
#:w
%patch7
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
#%doc slashem/readme.txt slashem/history.txt slashem/slamfaq.txt vultures/win/jtp/gamedata/manual/
%doc slashem/readme.txt slashem/history.txt slashem/slamfaq.txt
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
%{_prefix}/games/vulturesclaw/fonts/VeraSe.ttf
%{_prefix}/games/vultureseye/fonts/VeraSe.ttf


%changelog
* Tue Sep 19 2006 Boyd Gerber <gerberb@zenez.com> - 2.1.0 
- Patches for SUSE Linux/OpenSUSE Linux
- Applied patch 0 (suse-nethack-config.diff)
- Applied patch 1 (suse-nethack-decl.patch)
- Applied patch 2 (suse-nethack-gzip.patch)
- Applied patch 3 (suse-nethack-misc.patch)
- Applied patch 4 (disable-pcmusic.diff)
- Applied patch 5 (suse-nethack-syscall.patch)

