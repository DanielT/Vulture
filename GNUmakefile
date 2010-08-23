# This makefile is for unix and it's clones only.
# This makefile is meant for developers and people wanting
# to experiment with vulture only. It is NOT meant for system
# installation, nor is it meant for building the game for
# final gameplay. If you want to play vulture either check if
# your distribution of unix has a port for it, or if you are
# comfortable with system installation yourself then read and
# follow the instructions of the README in the sys/unix directory
# of NetHack and/or Slash'EM

GAMEDEF = VULTURE
GAMEDIRSUFFIX = -data
DATE := $(shell date +%Y%m%d%H%M%S)
GITREVISION := $(strip $(shell if ! [ -f .git-revision ]; then git rev-list `git describe --tags --abbrev=0`..master|wc -l > .git-revision; fi; cat .git-revision))
VERSION = 2.2.$(GITREVISION)
RELEASE = 1
FULLNAME = vulture-$(VERSION)
DISTDIR = dist/$(FULLNAME)
BUILDDIR = $(DISTDIR)/build
OS = ${shell case `uname -s` in *CYGWIN*|*MINGW*|*MSYS*|*Windows*) echo "win32" ;; *) echo "unix" ;; esac}
CWD = $(shell pwd)
INSTPREFIX = $$HOME/vulture

.PHONY: \
  help \
  nethack-home nethack.dmg \
  slashem-home slashem.dmg \
  distfiles \
  distfiles.tar.gz \
  distfiles.tar.bz2 \
  distfiles.7z \
  distfiles.zip \
  clean spotless

help:
	@echo "\nPlay:"
	@echo "\t$(MAKE) nethack-home        Build $(FULLNAME) for NetHack in ~/vulture"
	@echo "\t$(MAKE) slashem-home        Build $(FULLNAME) for Slash'EM in ~/vulture"
	@echo "\t$(MAKE) home                Build $(FULLNAME) for all variants in ~/vulture"
	@echo "\nBundle:"
	@echo "\t$(MAKE) distfiles.tar.gz    Bundle $(FULLNAME) in a tar.gz file"
	@echo "\t$(MAKE) distfiles.tar.bz2   Bundle $(FULLNAME) in a tar.bz2 file"
	@echo "\t$(MAKE) distfiles.7z        Bundle $(FULLNAME) in a 7z file"
	@echo "\t$(MAKE) distfiles.zip       Bundle $(FULLNAME) in a zip file"
	@echo "\t$(MAKE) distfiles           Bundle $(FULLNAME) for all compression types"

home: nethack-home slashem-home

nethack-home: nethack/Makefile nethack/win/vulture
	@echo "Building and installing NetHack in "$(INSTPREFIX)/vulture-nethack${GAMEDIRSUFFIX}
	@mkdir -p $(INSTPREFIX)/vulture-nethack${GAMEDIRSUFFIX}
	@$(MAKE) PREFIX=$(INSTPREFIX) GAMEDIR=$(INSTPREFIX)/vulture-nethack${GAMEDIRSUFFIX} SHELLDIR=$(INSTPREFIX) \
	         GAMEPERM=0755 CHOWN=true CHGRP=true -C nethack install >/dev/null

nethack/Makefile:
	@echo "Setup NetHack build environment ..."
	@cd nethack && sh sys/unix/setup.sh - >/dev/null

nethack/win/vulture:
	@cd nethack/win && ln -s ../../vulture

slashem-home: slashem/Makefile slashem/win/vulture
	@echo "Building and installing Slash'EM in "$(INSTPREFIX)/vulture-slashem${GAMEDIRSUFFIX}
	@mkdir -p $(INSTPREFIX)/vulture-slashem${GAMEDIRSUFFIX}
	@$(MAKE) PREFIX=$(INSTPREFIX) GAMEDIR=$(INSTPREFIX)/vulture-slashem${GAMEDIRSUFFIX} SHELLDIR=$(INSTPREFIX) \
	         GAMEPERM=0755 CHOWN=true CHGRP=true -C slashem install >/dev/null

slashem/Makefile:
	@echo "Setup Slash'EM build environment ..."
	@cd slashem && sh sys/unix/setup.sh - >/dev/null

slashem/win/vulture:
	@cd slashem/win && ln -s ../../vulture


clean:
	-$(MAKE) -C nethack clean
	-$(MAKE) -C slashem clean

spotless:
	-$(MAKE) -C nethack spotless
	-$(MAKE) -C slashem spotless
	-rm -rf dist/vulture*

distfiles: distfiles.tar.gz distfiles.tar.bz2 distfiles.7z distfiles.zip

distfiles.tar.gz: \
  $(DISTDIR)/$(FULLNAME).tar.gz \
  $(DISTDIR)/$(FULLNAME)-nethack.tar.gz \
  $(DISTDIR)/$(FULLNAME)-slashem.tar.gz

$(DISTDIR)/$(FULLNAME).tar.gz: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); tar zcvfh $(FULLNAME).tar.gz $(FULLNAME)
	cd $(DISTDIR); md5sum $(FULLNAME).tar.gz > $(FULLNAME).tar.gz.md5
	cd $(DISTDIR); sha256sum $(FULLNAME).tar.gz > $(FULLNAME).tar.gz.sha256
	cd $(DISTDIR); sha1sum $(FULLNAME).tar.gz > $(FULLNAME).tar.gz.sha1

$(DISTDIR)/$(FULLNAME)-nethack.tar.gz: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); tar zcvfh $(FULLNAME)-nethack.tar.gz $(FULLNAME)/nethack
	cd $(DISTDIR); md5sum $(FULLNAME)-nethack.tar.gz > $(FULLNAME)-nethack.tar.gz.md5
	cd $(DISTDIR); sha256sum $(FULLNAME)-nethack.tar.gz > $(FULLNAME)-nethack.tar.gz.sha256
	cd $(DISTDIR); sha1sum $(FULLNAME)-nethack.tar.gz > $(FULLNAME)-nethack.tar.gz.sha1

$(DISTDIR)/$(FULLNAME)-slashem.tar.gz: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); tar zcvfh $(FULLNAME)-slashem.tar.gz $(FULLNAME)/slashem
	cd $(DISTDIR); md5sum $(FULLNAME)-slashem.tar.gz > $(FULLNAME)-slashem.tar.gz.md5
	cd $(DISTDIR); sha256sum $(FULLNAME)-slashem.tar.gz > $(FULLNAME)-slashem.tar.gz.sha256
	cd $(DISTDIR); sha1sum $(FULLNAME)-slashem.tar.gz > $(FULLNAME)-slashem.tar.gz.sha1

distfiles.tar.bz2: \
  $(DISTDIR)/$(FULLNAME).tar.bz2 \
  $(DISTDIR)/$(FULLNAME)-nethack.tar.bz2 \
  $(DISTDIR)/$(FULLNAME)-slashem.tar.bz2

$(DISTDIR)/$(FULLNAME).tar.bz2: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); tar jcvfh $(FULLNAME).tar.bz2 $(FULLNAME)
	cd $(DISTDIR); md5sum $(FULLNAME).tar.bz2 > $(FULLNAME).tar.bz2.md5
	cd $(DISTDIR); sha256sum $(FULLNAME).tar.bz2 > $(FULLNAME).tar.bz2.sha256
	cd $(DISTDIR); sha1sum $(FULLNAME).tar.bz2 > $(FULLNAME).tar.bz2.sha1

$(DISTDIR)/$(FULLNAME)-nethack.tar.bz2: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); tar jcvfh $(FULLNAME)-nethack.tar.bz2 $(FULLNAME)/nethack
	cd $(DISTDIR); md5sum $(FULLNAME)-nethack.tar.bz2 > $(FULLNAME)-nethack.tar.bz2.md5
	cd $(DISTDIR); sha256sum $(FULLNAME)-nethack.tar.bz2 > $(FULLNAME)-nethack.tar.bz2.sha256
	cd $(DISTDIR); sha1sum $(FULLNAME)-nethack.tar.bz2 > $(FULLNAME)-nethack.tar.bz2.sha1

$(DISTDIR)/$(FULLNAME)-slashem.tar.bz2: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); tar jcvfh $(FULLNAME)-slashem.tar.bz2 $(FULLNAME)/slashem
	cd $(DISTDIR); md5sum $(FULLNAME)-slashem.tar.bz2 > $(FULLNAME)-slashem.tar.bz2.md5
	cd $(DISTDIR); sha256sum $(FULLNAME)-slashem.tar.bz2 > $(FULLNAME)-slashem.tar.bz2.sha256
	cd $(DISTDIR); sha1sum $(FULLNAME)-slashem.tar.bz2 > $(FULLNAME)-slashem.tar.bz2.sha1

distfiles.7z: \
  $(DISTDIR)/$(FULLNAME).7z \
  $(DISTDIR)/$(FULLNAME)-nethack.7z \
  $(DISTDIR)/$(FULLNAME)-slashem.7z

$(DISTDIR)/$(FULLNAME).7z: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); 7zr a -y -r -mx=9 $(FULLNAME).7z $(FULLNAME)
	cd $(DISTDIR); md5sum $(FULLNAME).7z > $(FULLNAME).7z.md5
	cd $(DISTDIR); sha256sum $(FULLNAME).7z > $(FULLNAME).7z.sha256
	cd $(DISTDIR); sha1sum $(FULLNAME).7z > $(FULLNAME).7z.sha1

$(DISTDIR)/$(FULLNAME)-nethack.7z: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); 7zr a -y -r -mx=9 $(FULLNAME)-nethack.7z $(FULLNAME)/nethack
	cd $(DISTDIR); md5sum $(FULLNAME)-nethack.7z > $(FULLNAME)-nethack.7z.md5
	cd $(DISTDIR); sha256sum $(FULLNAME)-nethack.7z > $(FULLNAME)-nethack.7z.sha256
	cd $(DISTDIR); sha1sum $(FULLNAME)-nethack.7z > $(FULLNAME)-nethack.7z.sha1

$(DISTDIR)/$(FULLNAME)-slashem.7z: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); 7zr a -y -r -mx=9 $(FULLNAME)-slashem.7z $(FULLNAME)/slashem
	cd $(DISTDIR); md5sum $(FULLNAME)-slashem.7z > $(FULLNAME)-slashem.7z.md5
	cd $(DISTDIR); sha256sum $(FULLNAME)-slashem.7z > $(FULLNAME)-slashem.7z.sha256
	cd $(DISTDIR); sha1sum $(FULLNAME)-slashem.7z > $(FULLNAME)-slashem.7z.sha1

distfiles.zip: \
  $(DISTDIR)/$(FULLNAME).zip \
  $(DISTDIR)/$(FULLNAME)-nethack.zip \
  $(DISTDIR)/$(FULLNAME)-slashem.zip

$(DISTDIR)/$(FULLNAME).zip: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); zip -y -r -9 $(FULLNAME).zip $(FULLNAME)
	cd $(DISTDIR); md5sum $(FULLNAME).zip > $(FULLNAME).zip.md5
	cd $(DISTDIR); sha256sum $(FULLNAME).zip > $(FULLNAME).zip.sha256
	cd $(DISTDIR); sha1sum $(FULLNAME).zip > $(FULLNAME).zip.sha1

$(DISTDIR)/$(FULLNAME)-nethack.zip: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); zip -y -r -9 $(FULLNAME)-nethack.zip $(FULLNAME)/nethack
	cd $(DISTDIR); md5sum $(FULLNAME)-nethack.zip > $(FULLNAME)-nethack.zip.md5
	cd $(DISTDIR); sha256sum $(FULLNAME)-nethack.zip > $(FULLNAME)-nethack.zip.sha256
	cd $(DISTDIR); sha1sum $(FULLNAME)-nethack.zip > $(FULLNAME)-nethack.zip.sha1

$(DISTDIR)/$(FULLNAME)-slashem.zip: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); zip -y -r -9 $(FULLNAME)-slashem.zip $(FULLNAME)/slashem
	cd $(DISTDIR); md5sum $(FULLNAME)-slashem.zip > $(FULLNAME)-slashem.zip.md5
	cd $(DISTDIR); sha256sum $(FULLNAME)-slashem.zip > $(FULLNAME)-slashem.zip.sha256
	cd $(DISTDIR); sha1sum $(FULLNAME)-slashem.zip > $(FULLNAME)-slashem.zip.sha1


$(DISTDIR)/$(FULLNAME): $(DISTDIR)
	git submodule init
	git submodule update
	git clone ./ $@
	cp .git-revision $@/.
	rm -rf $@/nethack
	git clone ./nethack/ $@/nethack
	rm -rf $@/slashem
	git clone ./slashem/ $@/slashem
	rm -rf $@/.git
	rm -rf $@/.gitmodules
	rm -rf $@/nethack/.git
	rm -rf $@/slashem/.git
	echo "#define VULTURE_PORT_VERSION \"$(VERSION)\"">$@/vulture/vulture_port_version.h
	ln -s ../../vulture $@/nethack/win/vulture
	ln -s ../../vulture $@/slashem/win/vulture

$(DISTDIR):
	mkdir -p $(DISTDIR)
	
nethack.dmg:
	@echo "Building NetHack dmg"
	@rm -rf $(BUILDDIR)
	@mkdir -p $(BUILDDIR)/nethack $(DISTDIR)
	@$(MAKE) PREFIX=$(BUILDDIR)/nethack/ GAMEPERM=0755 CHOWN=true CHGRP=true -C nethack install >/dev/null
	@chmod +x dist/macosx/makedmg-nethack
	@dist/macosx/makedmg-nethack $(BUILDDIR)/nethack/games $(DISTDIR) dist/macosx $(VERSION) NetHack $(RELEASE)
	@echo "dmg should now be located in $(DISTDIR)"

slashem.dmg:
	@echo "Building Slash'EM dmg"
	@rm -rf $(BUILDDIR)
	@mkdir -p $(BUILDDIR)/slashem $(DISTDIR)
	@$(MAKE) PREFIX=$(BUILDDIR)/slashem/ GAMEPERM=0755 CHOWN=true CHGRP=true -C slashem install >/dev/null
	@chmod +x dist/macosx/makedmg-slashem
	@dist/macosx/makedmg-slashem $(BUILDDIR)/slashem/games $(DISTDIR) dist/macosx $(VERSION) Slash'EM $(RELEASE)
	@echo "dmg should now be located in $(DISTDIR)"

