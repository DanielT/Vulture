Name:           nethack-vultures
Version:        2.2.79
Release:        0
Packager:      Boyd Gerber <gerberb@zenez.com>
Summary:        NetHack - Vulture's Eye and Vulture's Claw

Group:          Amusements/Games
License:        NetHack General Public License

URL:            http://clivecrous.github.com/vultures
Source0:        http://github.com/clivecrous/vultures/tarball/%{version}/vultures-%{version}-full.tar.bz2

%if 0%{?suse_version}
Source1:        SuSE.tar.bz2
Patch0:         suse-nethack-config.patch
Patch1:         suse-nethack-decl.patch
Patch2:         suse-nethack-gzip.patch
Patch3:         suse-nethack-misc.patch
Patch4:         disable-pcmusic.patch
Patch5:         suse-nethack-syscall.patch
Patch6:         suse-nethack-yacc.patch
Patch7:         suse-nethack-gametiles.patch
%endif

%if 0%{?fedora_version}
Patch0:         %{name}-1.11.0-optflags.patch
Patch1:         %{name}-1.11.0-config.patch
Patch2:         %{name}-1.10.1-clawguide.patch
%endif

Requires:       /bin/gzip
%if 0%{?suse_version}
PreReq:         permissions
%endif

BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires:  SDL-devel SDL_mixer-devel SDL_ttf-devel SDL_mixer-devel
BuildRequires:  gcc-c++ libpng-devel SDL_image-devel SDL_ttf-devel
%if 0%{?suse_version}
%if 0%{?suse_version} > 1020
BuildRequires:  fdupes bison update-desktop-files
%endif
%if 0%{?suse_version} < 1030
#BuildRequires:  fdupes bison update-desktop-files
BuildRequires:  bison update-desktop-files
%endif
%endif

%if 0%{?fedora_version}
BuildRequires:  byacc
%endif

BuildRequires:  ncurses-devel
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

To get the sources use

git clone git://github.com/clivecrous/vultures.git


Authors:
--------
    Vultures Eye Vultures Claw
      Clive Crous <clive@darkarts.co.za>
      isometric NetHack <vultures@lists.darkarts.co.za>
      SUSE Linux/OpenSUSE maintainer
      Boyd Gerber <gerberb@zenez.com>

    Nethack http://www.nethack.org 
      SUSE Linux/OpenSUSE maintainer
      Stephen L. Ericksen <stevee@cc.usu.edu>
      Boyd Gerber <gerberb@zenez.com>

%prep
%setup -q -n vultures-%{version}
%if %{?suse_version:1}0
%if %suse_version
%patch0 
%patch1
%patch2
%patch3
%patch5
%patch6
%endif
%if %suse_version <= 930
%patch4
%endif
%patch7
%endif

%if 0%{?suse_version}
tar xvfj %{S:1}
sed -i "s/^CFLAGS.*/& $RPM_OPT_FLAGS/" nethack/sys/unix/Makefile*
%endif

%if 0%{?suse_version}
sed -i -e 's|/usr/games/lib/nethackdir|/var/games/vultureseye|g' \
    nethack/doc/{nethack,recover}.6 nethack/include/config.h
sed -i -e 's|/var/lib/games/nethack|/var/games/vultureseye|g' \
    nethack/include/unixconf.h
sed -i -e 's|/usr/games/lib/nethackdir|/var/games/vulturesclaw|g' \
    slashem/doc/{nethack,recover}.6 slashem/include/config.h
sed -i -e 's|/var/lib/games/nethack|/var/games/vulturesclaw|' \
    slashem/include/unixconf.h
%endif

%if 0%{?fedora_version}
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
%endif

%build
%if 0%{?suse_version}
# Note: no %{?_smp_mflags} in any of these: various parallel build issues.
#for i in nethack slashem ; do
#    make $i/Makefile
#    make -C $i
#    make -C $i/util recover dlb dgn_comp lev_comp
#    make -C $i/dat spec_levs quest_levs
#done
# create symlinks to makefiles
cd nethack
sh sys/unix/setup.sh 1
## tty
cp -f ../SuSE/vultures/config.h.vultureseye include/config.h
cp -f ../SuSE/vultures/unixconf.h.vultureseye include/unixconf.h
cp -f ../SuSE/vultures/Makefile.src.vultureseye src/Makefile
cp -f ../SuSE/vultures/Makefile.top.vultureseye sys/unix/Makefile.top
cd ..
#
# create symlinks to makefiles
cd slashem
sh sys/unix/setup.sh 1
## tty
cp -f ../SuSE/vultures/config.h.vulturesclaw include/config.h
cp -f ../SuSE/vultures/unixconf.h.vulturesclaw include/unixconf.h
cp -f ../SuSE/vultures/Makefile.src.vulturesclaw src/Makefile
cp -f ../SuSE/vultures/Makefile.top.vulturesclaw sys/unix/Makefile.top
cd ..
# Note: no %{?_smp_mflags} in any of these: various parallel build issues.
for i in nethack slashem ; do
    make $i/Makefile
    make -C $i
    make -C $i/util recover dlb dgn_comp lev_comp
    make -C $i/dat spec_levs quest_levs
done
%endif

%if 0%{?fedora_version}
# Note: no %{?_smp_mflags} in any of these: various parallel build issues.
for i in nethack slashem ; do
    make $i/Makefile
    make -C $i
    make -C $i/util recover dlb dgn_comp lev_comp
    make -C $i/dat spec_levs quest_levs
done
%endif

#%if 0%{?suse_version}
#cp nethack/dat/options nethack/dat/options.tty
#%endif

%install
#
rm -rf $RPM_BUILD_ROOT
%if 0%{?suse_version}
%fdupes $RPM_BUILD_ROOT/usr/games/vultureseye/
%fdupes $RPM_BUILD_ROOT/usr/share/games/vultureseye/
%fdupes $RPM_BUILD_ROOT/usr/games/vulturesclaw/
%fdupes $RPM_BUILD_ROOT/usr/share/games/vulturesclaw/
make -C nethack install CHGRP=: CHOWN=: \
    GAMEDIR=$RPM_BUILD_ROOT%{_prefix}/games/vultureseye \
    VARDIR=$RPM_BUILD_ROOT%{_var}/games/vultureseye \
    SHELLDIR=$RPM_BUILD_ROOT%{_bindir}
make -C slashem install CHGRP=: CHOWN=: \
    GAMEDIR=$RPM_BUILD_ROOT%{_prefix}/games/vulturesclaw \
    VARDIR=$RPM_BUILD_ROOT%{_var}/games/vulturesclaw \
    SHELLDIR=$RPM_BUILD_ROOT%{_bindir}
#
#make -C nethack install CHGRP=: CHOWN=: \
#    GAMEDIR=$RPM_BUILD_ROOT/usr/share/games/vultureseye \
#    VARDIR=$RPM_BUILD_ROOT/var/games/vultureseye \
#    SHELLDIR=$RPM_BUILD_ROOT/usr/games/
#make -C slashem install CHGRP=: CHOWN=: \
#    GAMEDIR=$RPM_BUILD_ROOT/usr/share/games/vulturesclaw \
#    VARDIR=$RPM_BUILD_ROOT/var/games/vulturesclaw \
#    SHELLDIR=$RPM_BUILD_ROOT/usr/games/

# directories
install -d $RPM_BUILD_ROOT%{_prefix}/lib/nethack
install -d $RPM_BUILD_ROOT%{_bindir}
install -d $RPM_BUILD_ROOT%{_prefix}/share/games/nethack
install -d $RPM_BUILD_ROOT%{_mandir}/man6
install -d $RPM_BUILD_ROOT%{_prefix}/lib/vultureseye
install -d $RPM_BUILD_ROOT%{_prefix}/share/games/vultureseye
install -d $RPM_BUILD_ROOT%{_var}/games/vultureseye
#install -d $RPM_BUILD_ROOT%{_bindir}/vultureseye
install -d $RPM_BUILD_ROOT%{_prefix}/lib/vulturesclaw
install -d $RPM_BUILD_ROOT%{_prefix}/share/games/vulturesclaw
install -d $RPM_BUILD_ROOT%{_var}/games/vulturesclaw
#install -d $RPM_BUILD_ROOT%{_bindir}/vulturesclaw

## game directory
##install -d $RPM_BUILD_ROOT/var/games/nethack/save
##touch $RPM_BUILD_ROOT/var/games/nethack/perm \
##        $RPM_BUILD_ROOT/var/games/nethack/record \
##        $RPM_BUILD_ROOT/var/games/nethack/logfile
##chmod -R 0775 $RPM_BUILD_ROOT/var/games/nethack
%fdupes $RPM_BUILD_ROOT%{_prefix}/games/vultureseye/
%fdupes $RPM_BUILD_ROOT%{_prefix}/games/vultureseye/tiles/
%fdupes $RPM_BUILD_ROOT%{_var}/games/vultureseye/
%fdupes $RPM_BUILD_ROOT%{_var}/games/vultureseye/save
install -d $RPM_BUILD_ROOT%{_var}/games/vultureseye/
install -d $RPM_BUILD_ROOT%{_var}/games/vultureseye/save
##install -d $RPM_BUILD_ROOT/var/games/vultureseye/save
touch $RPM_BUILD_ROOT%{_var}/games/vultureseye/perm
touch $RPM_BUILD_ROOT%{_var}/games/vultureseye/record
touch $RPM_BUILD_ROOT%{_var}/games/vultureseye/logfile
chmod -R 0775 $RPM_BUILD_ROOT%{_var}/games/vultureseye
##touch $RPM_BUILD_ROOT/var/games/vultureseye/perm
##touch $RPM_BUILD_ROOT/var/games/vultureseye/record
##touch $RPM_BUILD_ROOT/var/games/vultureseye/logfile
##chmod -R 0775 $RPM_BUILD_ROOT/var/games/vultureseye
%fdupes $RPM_BUILD_ROOT%{_prefix}/games/vulturesclaw/
%fdupes $RPM_BUILD_ROOT%{_prefix}/games/vulturesclaw/tiles/
%fdupes $RPM_BUILD_ROOT%{_var}/games/vulturesclaw/
%fdupes $RPM_BUILD_ROOT%{_var}/games/vulturesclaw/save
touch $RPM_BUILD_ROOT%{_var}/games/vulturesclaw/perm
touch $RPM_BUILD_ROOT%{_var}/games/vulturesclaw/record
touch $RPM_BUILD_ROOT%{_var}/games/vulturesclaw/logfile
chmod -R 0775 $RPM_BUILD_ROOT%{_var}/games/vulturesclaw
##install -d $RPM_BUILD_ROOT/var/games/vulturesclaw/save
##touch $RPM_BUILD_ROOT/var/games/vulturesclaw/perm
##touch $RPM_BUILD_ROOT/var/games/vulturesclaw/record
##touch $RPM_BUILD_ROOT/var/games/vulturesclaw/logfile
##chmod -R 0775 $RPM_BUILD_ROOT/var/games/vulturesclaw

# binaries
# install -m 2755 nethack/src/nethack.tty $RPM_BUILD_ROOT/usr/lib/nethack/
# scripts
# BLG-boyd
#for STYLE in tty ; do 
#    install -m 755 SuSE/$STYLE/vultures.sh $RPM_BUILD_ROOT/usr/share/games/vultures.$STYLE
#    if [ -r SuSE/$STYLE/nethack-tty.sh ] ; then
#        install -m 755 SuSE/$STYLE/nethack-tty.sh $RPM_BUILD_ROOT/usr/share/games/nethack.tty.$STYLE
#    fi
#done
# options
#mkdir -p $RPM_BUILD_ROOT/usr/lib/vultures
#install -m 644 nethack/dat/options.tty $RPM_BUILD_ROOT/usr/lib/vultures/
#
#install -dm 755 $RPM_BUILD_ROOT%{_mandir}/man6
#install -pm 644 nethack/doc/nethack.6 \
#    $RPM_BUILD_ROOT%{_mandir}/man6/vultureseye.6
#install -pm 644 nethack/doc/recover.6 \
#    $RPM_BUILD_ROOT%{_mandir}/man6/vultureseye-recover.6
#install -pm 644 slashem/doc/nethack.6 \
#    $RPM_BUILD_ROOT%{_mandir}/man6/vulturesclaw.6
#install -pm 644 slashem/doc/recover.6 \
#    $RPM_BUILD_ROOT%{_mandir}/man6/vulturesclaw-recover.6
#
# man pages
%fdupes $RPM_BUILD_ROOT%{_mandir}/man6
install -m 644 nethack/doc/nethack.6 $RPM_BUILD_ROOT%{_mandir}/man6/vultureseye.6
install -m 644 nethack/doc/recover.6 $RPM_BUILD_ROOT%{_mandir}/man6/vultureseye-recover.6
install -m 644 slashem/doc/nethack.6 $RPM_BUILD_ROOT%{_mandir}/man6/vulturesclaw.6
install -m 644 slashem/doc/recover.6 $RPM_BUILD_ROOT%{_mandir}/man6/vulturesclaw-recover.6

# doc
mkdir -p $RPM_BUILD_ROOT%{_docdir}/nethack
%fdupes $RPM_BUILD_ROOT%{_docdir}/nethack
#install -m 644 nethack/doc/Guidebook.{tex,txt} $RPM_BUILD_ROOT/%{_docdir}/nethack
#cd nethack/doc
#tar cvfj $RPM_BUILD_ROOT%{_docdir}/nethack/fixes.tar.bz2 fixes*
#cd ../..
#chmod 644 $RPM_BUILD_ROOT%{_docdir}/nethack/fixes.tar.bz2
#install -m 644 nethack/dat/license $RPM_BUILD_ROOT/%{_docdir}/nethack
#install -m 644 SuSE/README.SuSE $RPM_BUILD_ROOT/%{_docdir}/nethack
# common data
#for file in x11tiles pet_mark.xbm rip.xpm mapbg.xpm license;
#do
#  install -m 644 nethack/dat/$file  $RPM_BUILD_ROOT/usr/share/games/nethack/
#done
## configs
# BLG-boyd
%fdupes $RPM_BUILD_ROOT/etc/vultures
install -m 755 -d $RPM_BUILD_ROOT/etc/vultures
for STYLE in vultures ; do
     install -m 755 SuSE/$STYLE/vulturesrc $RPM_BUILD_ROOT/etc/vultures/vulturesrc.$STYLE
done
## main launcher script
# install -m 755 SuSE/nethack $RPM_BUILD_ROOT/usr/games/
# recover helper
# install -m 755 SuSE/recover-helper $RPM_BUILD_ROOT/usr/lib/nethack/
# utils
#install -m 755 nethack/util/{dgn_comp,dlb,lev_comp,makedefs,recover,tile2x11} $RPM_BUILD_ROOT/usr/lib/nethack/
#install -m 755 nethack/util/tilemap $RPM_BUILD_ROOT/usr/lib/nethack/
# x11 app-defaults
#mkdir -p $RPM_BUILD_ROOT/usr/X11R6/lib/X11/app-defaults
#install -m 644 nethack/win/X11/NetHack.ad $RPM_BUILD_ROOT/usr/X11R6/lib/X11/app-defaults/NetHack
# x11 font
#/usr/bin/X11/bdftopcf -o nh10.pcf win/X11/nh10.bdf
#mkdir -p $RPM_BUILD_ROOT/usr/X11R6/lib/X11/fonts/misc/
#install -m 644 nh10.pcf $RPM_BUILD_ROOT/usr/X11R6/lib/X11/fonts/misc/
#gzip $RPM_BUILD_ROOT/usr/X11R6/lib/X11/fonts/misc/nh10.pcf
# the font is added into fonts.dir by SuSEconfig.fonts
#
#make -C nethack install CHGRP=: CHOWN=: \
#    GAMEDIR=$RPM_BUILD_ROOT/usr/share/games/vultureseye \
#    VARDIR=$RPM_BUILD_ROOT/var/games/vultureseye \
#    SHELLDIR=$RPM_BUILD_ROOT/usr/games/
#
#mkdir -p $RPM_BUILD_ROOT/usr/share/games/vultureseye/graphics
## BLG-boyd
##cp -p vultures/gamedata/graphics/gametiles.bin $RPM_BUILD_ROOT/usr/share/games/vultureseye/graphics/
#
#make -C slashem install CHGRP=: CHOWN=: \
#    GAMEDIR=$RPM_BUILD_ROOT/usr/share/games/vulturesclaw \
#    VARDIR=$RPM_BUILD_ROOT/var/games/vulturesclaw \
#    SHELLDIR=$RPM_BUILD_ROOT/usr/games/
#
#mkdir -p $RPM_BUILD_ROOT/usr/share/games/vultureclaw/graphics
## BLG-boyd
##cp -p vultures/gamedata/graphics/gametiles.bin $RPM_BUILD_ROOT/usr/share/games/vulturesclaw/graphics/

#
##install -dm 755 $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/48x48/apps
#install -dm 755 $RPM_BUILD_ROOT/usr/share/games/icons/hicolor/48x48/apps
#for i in vultureseye vulturesclaw ; do
#    desktop-file-install \
#        --vendor=openSUSE \
#        --dir=$RPM_BUILD_ROOT/usr/share/games/applications \
#        --mode=644 \
#        --add-category=X-SUSE \
#        dist/unix/desktop/$i.desktop
#    mv $RPM_BUILD_ROOT%{_prefix}/games/$i/*.png \
#        $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/48x48/apps/$i.png
#    mv $RPM_BUILD_ROOT%{_prefix}/games/$i/recover \
#        $RPM_BUILD_ROOT%{_bindir}/$i-recover
#done
#
#rm -r $RPM_BUILD_ROOT%{_prefix}/games/vultureseye/manual
#rm -r $RPM_BUILD_ROOT%{_prefix}/games/vulturesclaw/manual
#
## install -dm 755 $RPM_BUILD_ROOT/usr/share/games/vultureseye/icons/hicolor/48x48/apps
## install -dm 755 $RPM_BUILD_ROOT/usr/share/games/vulturesclaw/icons/hicolor/48x48/apps
##        --dir=$RPM_BUILD_ROOT/usr/share/games/$i/applications \
##        $RPM_BUILD_ROOT/usr/share/games/$i/icons/hicolor/48x48/apps/$i.png
##install -dm 755 $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/48x48/apps
#install -dm 755 $RPM_BUILD_ROOT/usr/share/games/icons/hicolor/48x48/apps
#mkdir -p $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/48x48/apps/
#mkdir -p $RPM_BUILD_ROOT/usr/share/games/icons/hicolor/48x48/apps/
#install -dm 755 $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/48x48/apps/
#install -dm 755 $RPM_BUILD_ROOT/usr/share/games/icons/hicolor/48x48/apps/
install -dm 755 $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/48x48/apps/
#install -dm 755 $RPM_BUILD_ROOT/usr/share/games/icons/hicolor/48x48/apps
%if %{?suse_version:1}0
%suse_update_desktop_file -i vultureseye Game RolePlaying
%suse_update_desktop_file -i vulturesclaw Game RolePlaying
%endif
for i in vultureseye vulturesclaw ; do
%if %{!?suse_version:1}0
    desktop-file-install \
        --vendor=openSUSE \
        --dir=$RPM_BUILD_ROOT%{_prefix}/share/games/applications \
        --mode=644 \
        --add-category=X-SUSE \
        dist/unix/desktop/$i.desktop
%endif
# BLG-boyd
    mv $RPM_BUILD_ROOT%{_prefix}/games/$i/*.png \
        $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/48x48/apps/$i.png
    mv $RPM_BUILD_ROOT%{_prefix}/games/$i/recover \
        $RPM_BUILD_ROOT%{_bindir}/$i-recover
#    mv $RPM_BUILD_ROOT/usr/share/games/$i/*.png \
#        $RPM_BUILD_ROOT/usr/share/games/icons/hicolor/48x48/apps/$i.png
#    mv $RPM_BUILD_ROOT/usr/share/games/$i/recover \
#        $RPM_BUILD_ROOT/usr/share/games/$i-recover
#echo ""
done

touch $RPM_BUILD_ROOT%{_prefix}/share/games/vultureseye/vultures_log.txt
touch $RPM_BUILD_ROOT%{_prefix}/share/games/vulturesclaw/vultures_log.txt
#touch $RPM_BUILD_ROOT/usr/share/games/vultureseye/vultures_log.txt
#touch $RPM_BUILD_ROOT/usr/share/games/vulturesclaw/vultures_log.txt
#install -m 644 vultures/build_n/gamedata/graphics/gamestiles.bin  $RPM_BUILD_ROOT/usr/share/games/vultureseye/
#install -m 644 vultures/build_s/gamedata/graphics/gamestiles.bin  $RPM_BUILD_ROOT/usr/share/games/vulturesclaw/

#rm -r $RPM_BUILD_ROOT/usr/share/games/vultureseye/manual
#rm -r $RPM_BUILD_ROOT/usr/share/games/vulturesclaw/manual

#
## Save some space
for f in graphics music sound ; do
    rm -r $RPM_BUILD_ROOT%{_prefix}/games/vulturesclaw/$f
    ln -s ../vultureseye/$f \
        $RPM_BUILD_ROOT%{_prefix}/games/vulturesclaw/$f
done
#
# Save some space
# for f in graphics music sound ; do
#for f in music sound ; do
#    rm -r $RPM_BUILD_ROOT%{_prefix}/share/games/vulturesclaw/$f
#    ln -s ../vultureseye/$f \
#        $RPM_BUILD_ROOT%{_prefix}/share/games/vulturesclaw/$f
#done
#for f in music sound ; do
#    rm -r $RPM_BUILD_ROOT/usr/share/games/vulturesclaw/$f
#    ln -s ../vultureseye/$f \
#        $RPM_BUILD_ROOT/usr/share/games/vulturesclaw/$f
#done

#chmod -s $RPM_BUILD_ROOT/usr/games/vultures*/vultures* # for stripping
chmod -s $RPM_BUILD_ROOT%{_prefix}/share/games/vultures*/vultures* # for stripping
#chmod -s $RPM_BUILD_ROOT/usr/share/games/vultures*/vultures* # for stripping
#chmod -s $RPM_BUILD_ROOT/usr/games/vultures* # for stripping

# Clean up
#sed -i -e "s|$RPM_BUILD_ROOT||" $RPM_BUILD_ROOT%{_prefix}/games/vultures{eye,claw}
#sed -i -e "s|$RPM_BUILD_ROOT||" $RPM_BUILD_ROOT/usr/games/vultures{eye,claw}
#rm $RPM_BUILD_ROOT%{_prefix}/share/games/vultures*/*.ico
chmod -R 0775 $RPM_BUILD_ROOT%{_var}/games/vultureseye
chmod -R 0775 $RPM_BUILD_ROOT%{_var}/games/vulturesclaw
#sed -i -e "s|$RPM_BUILD_ROOT||" $RPM_BUILD_ROOT%{_prefix}/games/vultures{eye,claw}
#sed -i -e "s|$RPM_BUILD_ROOT||" $RPM_BUILD_ROOT/usr/games/vultures{eye,claw}
#rm $RPM_BUILD_ROOT/usr/share/games/vultures*/*.ico
#chmod -R 0775 $RPM_BUILD_ROOT/var/games/vultureseye
#chmod -R 0775 $RPM_BUILD_ROOT/var/games/vulturesclaw
%fdupes $RPM_BUILD_ROOT

%clean
#
#rm -rf $RPM_BUILD_ROOT
#
rm -rf $RPM_BUILD_ROOT

%post
#
#[ $1 -eq 1 ] && \
#gtk-update-icon-cache -qf %{_datadir}/icons/hicolor &>/dev/null || :
#
[ $1 -eq 1 ] && \
gtk-update-icon-cache -qf %{_datadir}/icons/hicolor &>/dev/null || :
ln -s /var/games/vulturesclaw/logfile /usr/share/games/vulturesclaw/logfile &>/dev/null 
ln -s /var/games/vulturesclaw/perm /usr/share/games/vulturesclaw/perm &>/dev/null 
ln -s /var/games/vulturesclaw/record /usr/share/games/vulturesclaw/record &>/dev/null 
ln -s /var/games/vulturesclaw/save /usr/share/games/vulturesclaw/save &>/dev/null 
ln -s /var/games/vultureseye/logfile /usr/share/games/vultureseye/logfile &>/dev/null 
ln -s /var/games/vultureseye/perm /usr/share/games/vultureseye/perm &>/dev/null 
ln -s /var/games/vultureseye/record /usr/share/games/vultureseye/record &>/dev/null 
ln -s /var/games/vultureseye/save /usr/share/games/vultureseye/save &>/dev/null 

%postun
#
#gtk-update-icon-cache -qf %{_datadir}/icons/hicolor &>/dev/null || :
#
gtk-update-icon-cache -qf %{_datadir}/icons/hicolor &>/dev/null || :
rm /usr/share/games/vulturesclaw/logfile &>/dev/null 
rm /usr/share/games/vulturesclaw/perm &>/dev/null 
rm /usr/share/games/vulturesclaw/record &>/dev/null 
rm /var/games/vulturesclaw/save /usr/share/games/vulturesclaw/save &>/dev/null 
rm /var/games/vultureseye/logfile /usr/share/games/vultureseye/logfile &>/dev/null 
rm /var/games/vultureseye/perm /usr/share/games/vultureseye/perm &>/dev/null 
rm /var/games/vultureseye/record /usr/share/games/vultureseye/record &>/dev/null 
rm /var/games/vultureseye/save /usr/share/games/vultureseye/save &>/dev/null 

%run_permissions

%verifyscript
# %verify_permissions -e /usr/lib/nethack/nethack.tty

%files
#
#%defattr(-,root,root,-)
#%doc nethack/README nethack/dat/license nethack/dat/history nethack/dat/*help
##%doc slashem/readme.txt slashem/history.txt slashem/slamfaq.txt vultures/win/jtp/gamedata/manual/
#%doc slashem/readme.txt slashem/history.txt slashem/slamfaq.txt
#%{_bindir}/vultures*
#%dir %{_prefix}/games/vultureseye/
#%{_prefix}/games/vultureseye/config/
#%{_prefix}/games/vultureseye/defaults.nh
#%{_prefix}/games/vultureseye/graphics/
#%{_prefix}/games/vultureseye/license
#%{_prefix}/games/vultureseye/music/
#%{_prefix}/games/vultureseye/nhdat
#%{_prefix}/games/vultureseye/sound/
#%attr(2755,root,games) %{_prefix}/games/vultureseye/vultureseye
#%dir %{_prefix}/games/vulturesclaw/
#%{_prefix}/games/vulturesclaw/config/
#%{_prefix}/games/vulturesclaw/defaults.nh
#%{_prefix}/games/vulturesclaw/graphics/
#%{_prefix}/games/vulturesclaw/Guidebook.txt
#%{_prefix}/games/vulturesclaw/license
#%{_prefix}/games/vulturesclaw/music/
#%{_prefix}/games/vulturesclaw/nh*share
#%{_prefix}/games/vulturesclaw/sound/
#%attr(2755,root,games) %{_prefix}/games/vulturesclaw/vulturesclaw
#%{_datadir}/applications/*vultures*.desktop
##%{_datadir}/icons/hicolor/48x48/apps/vultures*.png
#/usr/share/icons/hicolor/
#/usr/share/icons/hicolor/48x48/
#/usr/share/icons/hicolor/48x48/apps/
#/usr/share/icons/hicolor/48x48/apps/vultures*.png
#%{_mandir}/man6/vultures*.6*
#%defattr(664,root,games,775)
#%dir %{_var}/games/vultureseye/
#%config(noreplace) %{_var}/games/vultureseye/record
#%config(noreplace) %{_var}/games/vultureseye/perm
#%config(noreplace) %{_var}/games/vultureseye/logfile
#%dir %{_var}/games/vultureseye/save/
#%dir %{_var}/games/vulturesclaw/
#%config(noreplace) %{_var}/games/vulturesclaw/record
#%config(noreplace) %{_var}/games/vulturesclaw/perm
#%config(noreplace) %{_var}/games/vulturesclaw/logfile
#%dir %{_var}/games/vulturesclaw/save/
#%{_prefix}/games/vulturesclaw/fonts/VeraSe.ttf
#%{_prefix}/games/vultureseye/fonts/VeraSe.ttf
#

%defattr(-,games,games)
# %verify(not mode) %attr(0755,games,games) /usr/lib/nethack/nethack.tty
#/usr/lib/vultures/options.tty
/etc/vultures
/etc/vultures/vulturesrc.vultures
/usr/games
%attr(2775,games,games) /usr/bin/vultureseye
%attr(666,games,games) /usr/bin/vultureseye-recover
#%attr(666,games,games) /usr/bin/games/vultureseye/vultures_log.txt
#/usr/bin/vultureseye
#/usr/bin/vultureseye-recover
%attr(2775,games,games) /usr/bin/vulturesclaw
%attr(666,games,games) /usr/bin/vulturesclaw-recover
/usr/games/vultureseye
/usr/games/vulturesclaw
/usr/share/games/vultureseye
/usr/share/games/vulturesclaw
/var/games/vultureseye
/var/games/vulturesclaw
%attr(0775,games,games) /var/games/vultureseye
%attr(0775,games,games) /var/games/vulturesclaw
#%doc slashem/readme.txt slashem/history.txt slashem/slamfaq.txt vultures/win/jtp/gamedata/manual/
%doc slashem/readme.txt slashem/history.txt slashem/slamfaq.txt
%attr(0775,games,games) %dir /usr/share/games/vultureseye/
#/usr/share/games/vultureseye/config/
#/usr/share/games/vultureseye/defaults.nh
#/usr/share/games/vultureseye/graphics/
#/usr/share/games/vultureseye/license
#/usr/share/games/vultureseye/music/
#/usr/share/games/vultureseye/nhdat
#/usr/share/games/vultureseye/sound/
##%attr(666,games,games) /usr/share/games/vultureseye-recover
#%attr(666,games,games) /usr/share/games/vultureseye/vultures_log.txt
##%attr(2775,games,games) /usr/share/games/vultureseye/vultureseye
%attr(0775,games,games) %dir /usr/share/games/vulturesclaw/
#/usr/share/games/vulturesclaw/config/
#/usr/share/games/vulturesclaw/defaults.nh
#/usr/share/games/vulturesclaw/graphics/
#/usr/share/games/vulturesclaw/Guidebook.txt
#/usr/share/games/vulturesclaw/license
#/usr/share/games/vulturesclaw/music/
#/usr/share/games/vulturesclaw/nh*share
#/usr/share/games/vulturesclaw/sound/
#%attr(666,games,games) /usr/share/games/vulturesclaw-recover
#%attr(666,games,games) /usr/share/games/vulturesclaw/vultures_log.txt
#%attr(2775,games,games) /usr/share/games/vulturesclaw/vulturesclaw
%{_datadir}/applications/*vultures*.desktop
#/usr/share/games/applications/*vultures*.desktop
#/usr/share/icons/hicolor/
#/usr/share/icons/hicolor/48x48/
#/usr/share/icons/hicolor/48x48/apps/
%dir %{_datadir}/icons/hicolor/
%dir %{_datadir}/icons/hicolor/48x48/
%dir %{_datadir}/icons/hicolor/48x48/apps/
%{_datadir}/icons/hicolor/48x48/apps/vultures*.png
#/usr/share/games/icons/hicolor/48x48/apps/vultures*.png
%defattr(666,games,games,775)
%dir /var/games/vultureseye/
%config(noreplace) %attr(666,games,games) /var/games/vultureseye/record
%config(noreplace) %attr(666,games,games) /var/games/vultureseye/perm
%config(noreplace) %attr(666,games,games) /var/games/vultureseye/logfile
%dir /var/games/vultureseye/save/
%dir /var/games/vulturesclaw/
%config(noreplace) %attr(666,games,games) /var/games/vulturesclaw/record
%config(noreplace) %attr(666,games,games) /var/games/vulturesclaw/perm
%config(noreplace) %attr(666,games,games) /var/games/vulturesclaw/logfile
%dir /var/games/vulturesclaw/save/
#/usr/share/games/vulturesclaw/fonts/VeraSe.ttf
#/usr/share/games/vultureseye/fonts/VeraSe.ttf
/usr/share/man/man6/vulturesclaw-recover.6.gz
/usr/share/man/man6/vulturesclaw.6.gz
/usr/share/man/man6/vultureseye-recover.6.gz
/usr/share/man/man6/vultureseye.6.gz

#make -C nethack install CHGRP=: CHOWN=: \
#    GAMEDIR=$RPM_BUILD_ROOT%{_prefix}/games/vultureseye \
#    VARDIR=$RPM_BUILD_ROOT%{_var}/games/vultureseye \
#    SHELLDIR=$RPM_BUILD_ROOT%{_bindir}
#make -C slashem install CHGRP=: CHOWN=: \
#    GAMEDIR=$RPM_BUILD_ROOT%{_prefix}/games/vulturesclaw \
#    VARDIR=$RPM_BUILD_ROOT%{_var}/games/vulturesclaw \
#    SHELLDIR=$RPM_BUILD_ROOT%{_bindir}
#
#install -dm 755 $RPM_BUILD_ROOT%{_mandir}/man6
#install -pm 644 nethack/doc/nethack.6 \
#    $RPM_BUILD_ROOT%{_mandir}/man6/vultureseye.6
#install -pm 644 nethack/doc/recover.6 \
#    $RPM_BUILD_ROOT%{_mandir}/man6/vultureseye-recover.6
#install -pm 644 slashem/doc/nethack.6 \
#    $RPM_BUILD_ROOT%{_mandir}/man6/vulturesclaw.6
#install -pm 644 slashem/doc/recover.6 \
#    $RPM_BUILD_ROOT%{_mandir}/man6/vulturesclaw-recover.6
#
##install -dm 755 $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/48x48/apps
#install -dm 755 $RPM_BUILD_ROOT/usr/share/games/icons/hicolor/48x48/apps
#for i in vultureseye vulturesclaw ; do
#    desktop-file-install \
#        --vendor=openSUSE \
#        --dir=$RPM_BUILD_ROOT/usr/share/games/applications \
#        --mode=644 \
#        --add-category=X-SUSE \
#        dist/unix/desktop/$i.desktop
#    mv $RPM_BUILD_ROOT%{_prefix}/games/$i/*.png \
#        $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/48x48/apps/$i.png
#    mv $RPM_BUILD_ROOT%{_prefix}/games/$i/recover \
#        $RPM_BUILD_ROOT%{_bindir}/$i-recover
#done
#
#rm -r $RPM_BUILD_ROOT%{_prefix}/games/vultureseye/manual
#rm -r $RPM_BUILD_ROOT%{_prefix}/games/vulturesclaw/manual
#
## Save some space
#for f in graphics music sound ; do
#    rm -r $RPM_BUILD_ROOT%{_prefix}/games/vulturesclaw/$f
#    ln -s ../vultureseye/$f \
#        $RPM_BUILD_ROOT%{_prefix}/games/vulturesclaw/$f
#done
#
#chmod -s $RPM_BUILD_ROOT%{_prefix}/games/vultures*/vultures* # for stripping
#
## Clean up
#sed -i -e "s|$RPM_BUILD_ROOT||" $RPM_BUILD_ROOT%{_bindir}/vultures{eye,claw}
#rm $RPM_BUILD_ROOT%{_prefix}/games/vultures*/*.ico
#
#%clean
#rm -rf $RPM_BUILD_ROOT
#
#
#%post
#[ $1 -eq 1 ] && \
#gtk-update-icon-cache -qf %{_datadir}/icons/hicolor &>/dev/null || :
#
#%postun
#gtk-update-icon-cache -qf %{_datadir}/icons/hicolor &>/dev/null || :
#
#
#%files
#%defattr(-,root,root,-)
#%doc nethack/README nethack/dat/license nethack/dat/history nethack/dat/*help
##%doc slashem/readme.txt slashem/history.txt slashem/slamfaq.txt vultures/win/jtp/gamedata/manual/
#%doc slashem/readme.txt slashem/history.txt slashem/slamfaq.txt
#%{_bindir}/vultures*
#%dir %{_prefix}/games/vultureseye/
#%{_prefix}/games/vultureseye/config/
#%{_prefix}/games/vultureseye/defaults.nh
#%{_prefix}/games/vultureseye/graphics/
#%{_prefix}/games/vultureseye/license
#%{_prefix}/games/vultureseye/music/
#%{_prefix}/games/vultureseye/nhdat
#%{_prefix}/games/vultureseye/sound/
#%attr(2755,root,games) %{_prefix}/games/vultureseye/vultureseye
#%dir %{_prefix}/games/vulturesclaw/
#%{_prefix}/games/vulturesclaw/config/
#%{_prefix}/games/vulturesclaw/defaults.nh
#%{_prefix}/games/vulturesclaw/graphics/
#%{_prefix}/games/vulturesclaw/Guidebook.txt
#%{_prefix}/games/vulturesclaw/license
#%{_prefix}/games/vulturesclaw/music/
#%{_prefix}/games/vulturesclaw/nh*share
#%{_prefix}/games/vulturesclaw/sound/
#%attr(2755,root,games) %{_prefix}/games/vulturesclaw/vulturesclaw
#%{_datadir}/applications/*vultures*.desktop
##%{_datadir}/icons/hicolor/48x48/apps/vultures*.png
#/usr/share/icons/hicolor/
#/usr/share/icons/hicolor/48x48/
#/usr/share/icons/hicolor/48x48/apps/
#/usr/share/icons/hicolor/48x48/apps/vultures*.png
#%{_mandir}/man6/vultures*.6*
#%defattr(664,root,games,775)
#%dir %{_var}/games/vultureseye/
#%config(noreplace) %{_var}/games/vultureseye/record
#%config(noreplace) %{_var}/games/vultureseye/perm
#%config(noreplace) %{_var}/games/vultureseye/logfile
#%dir %{_var}/games/vultureseye/save/
#%dir %{_var}/games/vulturesclaw/
#%config(noreplace) %{_var}/games/vulturesclaw/record
#%config(noreplace) %{_var}/games/vulturesclaw/perm
#%config(noreplace) %{_var}/games/vulturesclaw/logfile
#%dir %{_var}/games/vulturesclaw/save/
#%{_prefix}/games/vulturesclaw/fonts/VeraSe.ttf
#%{_prefix}/games/vultureseye/fonts/VeraSe.ttf
#
%endif


%if 0%{?fedora_version}
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

#install -dm 755 $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/48x48/apps
install -dm 755 $RPM_BUILD_ROOT/usr/share/games/icons/hicolor/48x48/apps
for i in vultureseye vulturesclaw ; do
    desktop-file-install \
        --vendor=fedora \
#        --dir=$RPM_BUILD_ROOT%{_datadir}/applications \
        --dir=$RPM_BUILD_ROOT/usr/share/games/applications \
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
#%{_datadir}/icons/hicolor/48x48/apps/vultures*.png
/usr/share/icons/hicolor/
/usr/share/icons/hicolor/48x48/
/usr/share/icons/hicolor/48x48/apps/
/usr/share/icons/hicolor/48x48/apps/vultures*.png
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
%endif

%changelog
* Sat Jul 18 2009 Boyd Gerber <gerberb@zenez.com> - 2.2.X 
- Patches for SUSE Linux/OpenSUSE Linux
- Applied patch 0 (suse-nethack-config.diff)
- Applied patch 1 (suse-nethack-decl.patch)
- Applied patch 2 (suse-nethack-gzip.patch)
- Applied patch 3 (suse-nethack-misc.patch)
- Applied patch 4 (disable-pcmusic.diff)
- Applied patch 5 (suse-nethack-syscall.patch)
- Applied patch 6 (suse-nethack-yacc.patch)
- Applied patch 7 (suse-nethack-gametiles.patch)

