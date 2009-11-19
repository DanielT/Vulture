# This makefile is for unix and it's clones only.
# This makefile is meant for developers and people wanting
# to experiment with vultures only. It is NOT meant for system
# installation, nor is it meant for building the game for
# final gameplay. If you want to play vultures either check if
# your distribution of unix has a port for it, or if you are
# comfortable with system installation yourself then read and
# follow the instructions of the README in the sys/unix directory
# of NetHack and/or Slash'EM

GAME = vultures
GAMEDEF = VULTURES
NETHACK = eye
SLASHEM = claw
GAMENETHACK = $(GAME)$(NETHACK)
GAMESLASHEM = $(GAME)$(SLASHEM)
DATE := $(shell date +%Y%m%d%H%M%S)
GITREVISION := $(strip $(shell if ! [ -f .git-revision ]; then git rev-list `git describe --tags --abbrev=0`..master|wc -l > .git-revision; fi; cat .git-revision))
VERSION = 2.2.$(GITREVISION)
RELEASE = 1
FULLNAME = $(GAME)-$(VERSION)
DISTDIR = dist/$(FULLNAME)
OS = ${shell case `uname -s` in *CYGWIN*|*MINGW*|*MSYS*|*Windows*) echo "win32" ;; *) echo "unix" ;; esac}
CWD = $(shell pwd)
TESTDIR = $(CWD)/testdir
DMGDIR = $(CWD)/dmgdir
INSTPREFIX = $$HOME/$(GAME)
MD5 = md5sum
SHA256 = sha256sum

help:
	@echo "Building $(FULLNAME)"
	@echo "to build NetHack in your home directory: $(MAKE) nethack-home"
	@echo "to build Slashem in your home directory: $(MAKE) slashem-home"
	@echo "to build * Both  in your home directory: $(MAKE) home"

install:
	@echo "Nothing to do for 'install'"

home: nethack-home slashem-home

test: test-$(OS)

test-win32:
	mingw-make-nethack.bat
	mingw-make-slashem.bat

test-unix: nethack-test slashem-test

nethack-home: nethack/Makefile nethack/win/$(GAME)
	@echo "Building and installing NetHack in "$(INSTPREFIX)/$(GAMENETHACK)dir
	@mkdir -p $(INSTPREFIX)/$(GAMENETHACK)dir
	@$(MAKE) PREFIX=$(INSTPREFIX) GAMEDIR=$(INSTPREFIX)/$(GAMENETHACK)dir SHELLDIR=$(INSTPREFIX) \
	         GAMEPERM=0755 CHOWN=true CHGRP=true -C nethack install >/dev/null

slashem-home: slashem/Makefile slashem/win/$(GAME)
	@echo "Building and installing Slash'EM in "$(INSTPREFIX)/$(GAMESLASHEM)dir
	@mkdir -p $(INSTPREFIX)/$(GAMESLASHEM)dir
	@$(MAKE) PREFIX=$(INSTPREFIX) GAMEDIR=$(INSTPREFIX)/$(GAMESLASHEM)dir SHELLDIR=$(INSTPREFIX) \
	         GAMEPERM=0755 CHOWN=true CHGRP=true -C slashem install >/dev/null

$(TESTDIR):
	@mkdir $(TESTDIR)

slashem-test: slashem/Makefile slashem/win/$(GAME) $(TESTDIR)
	@echo "Test building Slash'EM ..."
	@mkdir -p $(TESTDIR)/slashem
	@$(MAKE) PREFIX=$(TESTDIR)/slashem/ GAMEPERM=0755 CHOWN=true CHGRP=true -C slashem install >/dev/null
nethack-test: nethack/Makefile nethack/win/$(GAME) $(TESTDIR)
	@echo "Test building NetHack ..."
	@mkdir -p $(TESTDIR)/nethack
	@$(MAKE) PREFIX=$(TESTDIR)/nethack/ GAMEPERM=0755 CHOWN=true CHGRP=true -C nethack install >/dev/null

nethack/Makefile:
	@echo "Setup NetHack build environment ..."
	@cd nethack && sh sys/unix/setup.sh - >/dev/null

nethack/win/$(GAME):
	@cd nethack/win && ln -s ../../$(GAME)

slashem/Makefile:
	@echo "Setup Slash'EM build environment ..."
	@cd slashem && sh sys/unix/setup.sh - >/dev/null

slashem/win/$(GAME):
	@cd slashem/win && ln -s ../../$(GAME)

clean:
	-$(MAKE) -C nethack clean
	-$(MAKE) -C slashem clean

spotless:
	-$(MAKE) -C nethack spotless
	-$(MAKE) -C slashem spotless
	-rm -rf dist/$(GAME)*
	-rm -rf snapshot

snapshot: $(DISTDIR)/$(FULLNAME)-full.tar.gz
	-mkdir snapshot
	-mv $(DISTDIR)/$(FULLNAME)-full.tar.gz* snapshot/.
	-rm -rf $(DISTDIR)

distfiles_targz: \
	$(DISTDIR)/$(FULLNAME)-full.tar.gz	\
	$(DISTDIR)/$(FULLNAME)-$(NETHACK).tar.gz	\
	$(DISTDIR)/$(FULLNAME)-$(SLASHEM).tar.gz

distfiles_tarbz2: \
	$(DISTDIR)/$(FULLNAME)-full.tar.bz2	\
	$(DISTDIR)/$(FULLNAME)-$(NETHACK).tar.bz2	\
	$(DISTDIR)/$(FULLNAME)-$(SLASHEM).tar.bz2

distfiles_zip: \
	$(DISTDIR)/$(FULLNAME)-full.zip		\
	$(DISTDIR)/$(FULLNAME)-$(NETHACK).zip		\
	$(DISTDIR)/$(FULLNAME)-$(SLASHEM).zip

distfiles_7z: \
	$(DISTDIR)/$(FULLNAME)-full.7z		\
	$(DISTDIR)/$(FULLNAME)-$(NETHACK).7z		\
	$(DISTDIR)/$(FULLNAME)-$(SLASHEM).7z

distfiles_unixbin:	\
	$(DISTDIR)/Unix\ Installer/$(FULLNAME)-full_unix-$(RELEASE).bin.sh \
	$(DISTDIR)/Unix\ Installer/$(FULLNAME)-$(NETHACK)_unix-$(RELEASE).bin.sh \
	$(DISTDIR)/Unix\ Installer/$(FULLNAME)-$(SLASHEM)_unix-$(RELEASE).bin.sh

distfiles: distfiles_targz distfiles_tarbz2 distfiles_zip distfiles_7z distfiles_unixbin

$(DISTDIR)/$(FULLNAME)-full.tar.gz: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); tar zcvf $(FULLNAME)-full.tar.gz $(FULLNAME)
	cd $(DISTDIR); $(MD5) $(FULLNAME)-full.tar.gz > $(FULLNAME)-full.tar.gz.md5
	cd $(DISTDIR); $(SHA256) $(FULLNAME)-full.tar.gz > $(FULLNAME)-full.tar.gz.sha256

$(DISTDIR)/$(FULLNAME)-$(NETHACK).tar.gz: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); tar zcvfh $(FULLNAME)-$(NETHACK).tar.gz $(FULLNAME)/nethack
	cd $(DISTDIR); $(MD5) $(FULLNAME)-$(NETHACK).tar.gz > $(FULLNAME)-$(NETHACK).tar.gz.md5
	cd $(DISTDIR); $(SHA256) $(FULLNAME)-$(NETHACK).tar.gz > $(FULLNAME)-$(NETHACK).tar.gz.sha256

$(DISTDIR)/$(FULLNAME)-$(SLASHEM).tar.gz: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); tar zcvfh $(FULLNAME)-$(SLASHEM).tar.gz $(FULLNAME)/slashem
	cd $(DISTDIR); $(MD5) $(FULLNAME)-$(SLASHEM).tar.gz > $(FULLNAME)-$(SLASHEM).tar.gz.md5
	cd $(DISTDIR); $(SHA256) $(FULLNAME)-$(SLASHEM).tar.gz > $(FULLNAME)-$(SLASHEM).tar.gz.sha256

$(DISTDIR)/$(FULLNAME)-full.tar.bz2: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); tar jcvf $(FULLNAME)-full.tar.bz2 $(FULLNAME)
	cd $(DISTDIR); $(MD5) $(FULLNAME)-full.tar.bz2 > $(FULLNAME)-full.tar.bz2.md5
	cd $(DISTDIR); $(SHA256) $(FULLNAME)-full.tar.bz2 > $(FULLNAME)-full.tar.bz2.sha256

$(DISTDIR)/$(FULLNAME)-$(NETHACK).tar.bz2: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); tar jcvfh $(FULLNAME)-$(NETHACK).tar.bz2 $(FULLNAME)/nethack
	cd $(DISTDIR); $(MD5) $(FULLNAME)-$(NETHACK).tar.bz2 > $(FULLNAME)-$(NETHACK).tar.bz2.md5
	cd $(DISTDIR); $(SHA256) $(FULLNAME)-$(NETHACK).tar.bz2 > $(FULLNAME)-$(NETHACK).tar.bz2.sha256

$(DISTDIR)/$(FULLNAME)-$(SLASHEM).tar.bz2: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); tar jcvfh $(FULLNAME)-$(SLASHEM).tar.bz2 $(FULLNAME)/slashem
	cd $(DISTDIR); $(MD5) $(FULLNAME)-$(SLASHEM).tar.bz2 > $(FULLNAME)-$(SLASHEM).tar.bz2.md5
	cd $(DISTDIR); $(SHA256) $(FULLNAME)-$(SLASHEM).tar.bz2 > $(FULLNAME)-$(SLASHEM).tar.bz2.sha256

$(DISTDIR)/$(FULLNAME)-full.zip: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); zip -y -r -9 $(FULLNAME)-full.zip $(FULLNAME)
	cd $(DISTDIR); $(MD5) $(FULLNAME)-full.zip > $(FULLNAME)-full.zip.md5
	cd $(DISTDIR); $(SHA256) $(FULLNAME)-full.zip > $(FULLNAME)-full.zip.sha256

$(DISTDIR)/$(FULLNAME)-$(NETHACK).zip: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); zip -r -9 $(FULLNAME)-$(NETHACK).zip $(FULLNAME)/nethack
	cd $(DISTDIR); $(MD5) $(FULLNAME)-$(NETHACK).zip > $(FULLNAME)-$(NETHACK).zip.md5
	cd $(DISTDIR); $(SHA256) $(FULLNAME)-$(NETHACK).zip > $(FULLNAME)-$(NETHACK).zip.sha256

$(DISTDIR)/$(FULLNAME)-$(SLASHEM).zip: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); zip -r -9 $(FULLNAME)-$(SLASHEM).zip $(FULLNAME)/slashem
	cd $(DISTDIR); $(MD5) $(FULLNAME)-$(SLASHEM).zip > $(FULLNAME)-$(SLASHEM).zip.md5
	cd $(DISTDIR); $(SHA256) $(FULLNAME)-$(SLASHEM).zip > $(FULLNAME)-$(SLASHEM).zip.sha256

$(DISTDIR)/$(FULLNAME)-full.7z: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); 7zr a -y -r -mx=9 $(FULLNAME)-full.7z $(FULLNAME)
	cd $(DISTDIR); $(MD5) $(FULLNAME)-full.7z > $(FULLNAME)-full.7z.md5
	cd $(DISTDIR); $(SHA256) $(FULLNAME)-full.7z > $(FULLNAME)-full.7z.sha256

$(DISTDIR)/$(FULLNAME)-$(NETHACK).7z: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); 7zr a -y -r -mx=9 $(FULLNAME)-$(NETHACK).7z $(FULLNAME)/nethack
	cd $(DISTDIR); $(MD5) $(FULLNAME)-$(NETHACK).7z > $(FULLNAME)-$(NETHACK).7z.md5
	cd $(DISTDIR); $(SHA256) $(FULLNAME)-$(NETHACK).7z > $(FULLNAME)-$(NETHACK).7z.sha256

$(DISTDIR)/$(FULLNAME)-$(SLASHEM).7z: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); 7zr a -y -r -mx=9 $(FULLNAME)-$(SLASHEM).7z $(FULLNAME)/slashem
	cd $(DISTDIR); $(MD5) $(FULLNAME)-$(SLASHEM).7z > $(FULLNAME)-$(SLASHEM).7z.md5
	cd $(DISTDIR); $(SHA256) $(FULLNAME)-$(SLASHEM).7z > $(FULLNAME)-$(SLASHEM).7z.sha256

$(DISTDIR)/Unix\ Installer: $(DISTDIR)
	mkdir $(DISTDIR)/Unix\ Installer

$(DISTDIR)/Unix\ Installer/$(FULLNAME)-full_unix-$(RELEASE).bin.sh: $(DISTDIR)/$(FULLNAME) $(DISTDIR)/Unix\ Installer
	cd $(DISTDIR); makeself $(FULLNAME) Unix\ Installer/$(FULLNAME)-full_unix-$(RELEASE).bin.sh $(FULLNAME) "make home"
	cd $(DISTDIR)/Unix\ Installer; $(MD5) $(FULLNAME)-full_unix-$(RELEASE).bin.sh > $(FULLNAME)-full_unix-$(RELEASE).bin.sh.md5
	cd $(DISTDIR)/Unix\ Installer; $(SHA256) $(FULLNAME)-full_unix-$(RELEASE).bin.sh > $(FULLNAME)-full_unix-$(RELEASE).bin.sh.sha256

$(DISTDIR)/Unix\ Installer/$(FULLNAME)-$(NETHACK)_unix-$(RELEASE).bin.sh: $(DISTDIR)/$(FULLNAME) $(DISTDIR)/Unix\ Installer
	cd $(DISTDIR); mv $(FULLNAME)/slashem .slashem; makeself $(FULLNAME) Unix\ Installer/$(FULLNAME)-$(NETHACK)_unix-$(RELEASE).bin.sh $(FULLNAME) "make nethack-home"; mv .slashem $(FULLNAME)/slashem
	cd $(DISTDIR)/Unix\ Installer; $(MD5) $(FULLNAME)-$(NETHACK)_unix-$(RELEASE).bin.sh > $(FULLNAME)-$(NETHACK)_unix-$(RELEASE).bin.sh.md5
	cd $(DISTDIR)/Unix\ Installer; $(SHA256) $(FULLNAME)-$(NETHACK)_unix-$(RELEASE).bin.sh > $(FULLNAME)-$(NETHACK)_unix-$(RELEASE).bin.sh.sha256

$(DISTDIR)/Unix\ Installer/$(FULLNAME)-$(SLASHEM)_unix-$(RELEASE).bin.sh: $(DISTDIR)/$(FULLNAME) $(DISTDIR)/Unix\ Installer
	cd $(DISTDIR); mv $(FULLNAME)/nethack .nethack; makeself $(FULLNAME) Unix\ Installer/$(FULLNAME)-$(SLASHEM)_unix-$(RELEASE).bin.sh $(FULLNAME) "make slashem-home"; mv .nethack $(FULLNAME)/nethack
	cd $(DISTDIR)/Unix\ Installer; $(MD5) $(FULLNAME)-$(SLASHEM)_unix-$(RELEASE).bin.sh > $(FULLNAME)-$(SLASHEM)_unix-$(RELEASE).bin.sh.md5
	cd $(DISTDIR)/Unix\ Installer; $(SHA256) $(FULLNAME)-$(SLASHEM)_unix-$(RELEASE).bin.sh > $(FULLNAME)-$(SLASHEM)_unix-$(RELEASE).bin.sh.sha256

$(DISTDIR)/$(FULLNAME): $(DISTDIR)
	git submodule init
	git submodule update
	git clone ./ $@
	cp .git-revision $@/.
	rm -rf $@/nethack
	rm -rf $@/slashem
	git clone ./nethack/ $@/nethack
	git clone ./slashem/ $@/slashem
	rm -rf $@/.git
	rm -rf $@/.gitmodules
	rm -rf $@/nethack/.git
	rm -rf $@/slashem/.git
	echo "#define $(GAMEDEF)_PORT_VERSION \"$(VERSION)\"">$@/$(GAME)/$(GAME)_port_version.h
	ln -s ../../$(GAME) $@/nethack/win/$(GAME)
	ln -s ../../$(GAME) $@/slashem/win/$(GAME)

$(DISTDIR):
	mkdir -p $(DISTDIR)
	
nethack-dmg:
	@echo "Building nethack dmg"
	@mkdir -p $(TESTDIR)/nethack $(DMGDIR)
	@$(MAKE) PREFIX=$(TESTDIR)/nethack/ GAMEPERM=0755 CHOWN=true CHGRP=true -C nethack install >/dev/null
	@chmod +x dist/macosx/makedmg-nethack
	@dist/macosx/makedmg-nethack $(TESTDIR)/nethack/games $(DMGDIR) dist/macosx $(VERSION) $(NETHACK) $(RELEASE)
	@echo "dmg should now be located in $(DMGDIR)"
	
slashem-dmg:
	@echo "Building slashem dmg"
	@mkdir -p $(TESTDIR)/slashem $(DMGDIR)
	@$(MAKE) PREFIX=$(TESTDIR)/slashem/ GAMEPERM=0755 CHOWN=true CHGRP=true -C slashem install >/dev/null
	@chmod +x dist/macosx/makedmg-slashem
	@dist/macosx/makedmg-slashem $(TESTDIR)/slashem/local $(DMGDIR) dist/macosx $(VERSION) $(SLASHEM) $(RELEASE)
	@echo "dmg should now be located in $(DMGDIR)"


