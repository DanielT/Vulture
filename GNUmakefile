# This makefile is for unix and it's clones only.
# This makefile is meant for developers and people wanting
# to experiment with vultures only. It is NOT meant for system
# installation, nor is it meant for building the game for
# final gameplay. If you want to play vultures either check if
# your distribution of unix has a port for it, or if you are
# comfortable with system installation yourself then read and
# follow the instructions of INSTALL_unix in the doc directory

GAME = vultures
DATE := $(shell date +%Y%m%d%H%M%S)
VERSION = snapshot-$(DATE)
FULLNAME = $(GAME)-$(VERSION)
DISTDIR = dist/$(FULLNAME)
OS = ${shell case `uname -s` in *CYGWIN*|*MINGW*|*MSYS*|*Windows*) echo "win32" ;; *) echo "unix" ;; esac}
CWD = $(shell pwd)
TESTDIR = $(CWD)/testdir
VEINSTALLDIR = $$HOME/vultures/vultureseye
VCINSTALLDIR = $$HOME/vultures/vulturesclaw


help:
	@echo "to build NetHack in your home directory: $(MAKE) nethack-home"
	@echo "to build Slashem in your home directory: $(MAKE) slashem-home"
	@echo "to build * Both  in your home directory: $(MAKE) home"

posthook:
	@darcs record --repodir=./_darcs/ -a -m "autopatch" --run-posthook --posthook="darcs diff --last 1|grep '^>'|cut -c2-|mail -s 'darcs patch bundle applied' vultures@darkarts.co.za"
	@$(MAKE) -C doc posthook

install:
	@echo "Nothing to do for 'install'"

home: nethack-home slashem-home

test: test-$(OS)

test-win32:
	mingw-make-nethack.bat
	mingw-make-slashem.bat

test-unix: nethack-test slashem-test

nethack-home: nethack/Makefile nethack/win/vultures
	@echo "Building and installing NetHack in "$(VEINSTALLDIR)
	@mkdir -p $(VEINSTALLDIR)
	@$(MAKE) PREFIX=$(VEINSTALLDIR) GAMEPERM=0755 CHOWN=true CHGRP=true -C nethack install >/dev/null

slashem-home: slashem/Makefile slashem/win/vultures
	@echo "Building and installing Slash'EM in "$(VCINSTALLDIR)
	@mkdir -p $(VCINSTALLDIR)
	@$(MAKE) PREFIX=$(VCINSTALLDIR) GAMEPERM=0755 CHOWN=true CHGRP=true -C slashem install >/dev/null

$(TESTDIR):
	@mkdir $(TESTDIR)

slashem-test: slashem/Makefile slashem/win/vultures $(TESTDIR)
	@echo "Test building Slash'EM ..."
	@mkdir -p $(TESTDIR)/slashem
	@$(MAKE) PREFIX=$(TESTDIR)/slashem/ GAMEPERM=0755 CHOWN=true CHGRP=true -C slashem install >/dev/null
nethack-test: nethack/Makefile nethack/win/vultures $(TESTDIR)
	@echo "Test building NetHack ..."
	@mkdir -p $(TESTDIR)/nethack
	@$(MAKE) PREFIX=$(TESTDIR)/nethack/ GAMEPERM=0755 CHOWN=true CHGRP=true -C nethack install >/dev/null

nethack/Makefile:
	@echo "Setup NetHack build environment ..."
	@cd nethack && sh sys/unix/setup.sh - >/dev/null

nethack/win/vultures:
	@cd nethack/win && ln -s ../../vultures

slashem/Makefile:
	@echo "Setup Slash'EM build environment ..."
	@cd slashem && sh sys/unix/setup.sh - >/dev/null

slashem/win/vultures:
	@cd slashem/win && ln -s ../../vultures

clean:
	-$(MAKE) -C nethack clean
	-$(MAKE) -C slashem clean

spotless:
	-$(MAKE) -C nethack spotless
	-$(MAKE) -C slashem spotless
	-rm -rf dist/vultures*

distfiles:	\
	$(DISTDIR)/$(FULLNAME)-full.tar.gz	\
	$(DISTDIR)/$(FULLNAME)-eye.tar.gz	\
	$(DISTDIR)/$(FULLNAME)-claw.tar.gz	\
	$(DISTDIR)/$(FULLNAME)-full.tar.bz2	\
	$(DISTDIR)/$(FULLNAME)-eye.tar.bz2	\
	$(DISTDIR)/$(FULLNAME)-claw.tar.bz2 \
	$(DISTDIR)/$(FULLNAME)-full.zip		\
	$(DISTDIR)/$(FULLNAME)-eye.zip		\
	$(DISTDIR)/$(FULLNAME)-claw.zip		\
	$(DISTDIR)/Unix\ Installer/$(FULLNAME)-full_unix-1.bin.sh \
	$(DISTDIR)/Unix\ Installer/$(FULLNAME)-eye_unix-1.bin.sh \
	$(DISTDIR)/Unix\ Installer/$(FULLNAME)-claw_unix-1.bin.sh

$(DISTDIR)/$(FULLNAME)-full.tar.gz: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); tar zcvf $(FULLNAME)-full.tar.gz $(FULLNAME)
	cd $(DISTDIR); md5 $(FULLNAME)-full.tar.gz > $(FULLNAME)-full.tar.gz.md5

$(DISTDIR)/$(FULLNAME)-eye.tar.gz: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); tar zcvfL $(FULLNAME)-eye.tar.gz $(FULLNAME)/nethack $(FULLNAME)/doc
	cd $(DISTDIR); md5 $(FULLNAME)-eye.tar.gz > $(FULLNAME)-eye.tar.gz.md5

$(DISTDIR)/$(FULLNAME)-claw.tar.gz: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); tar zcvfL $(FULLNAME)-claw.tar.gz $(FULLNAME)/slashem $(FULLNAME)/doc
	cd $(DISTDIR); md5 $(FULLNAME)-claw.tar.gz > $(FULLNAME)-claw.tar.gz.md5

$(DISTDIR)/$(FULLNAME)-full.tar.bz2: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); tar jcvf $(FULLNAME)-full.tar.bz2 $(FULLNAME)
	cd $(DISTDIR); md5 $(FULLNAME)-full.tar.bz2 > $(FULLNAME)-full.tar.bz2.md5

$(DISTDIR)/$(FULLNAME)-eye.tar.bz2: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); tar jcvfL $(FULLNAME)-eye.tar.bz2 $(FULLNAME)/nethack $(FULLNAME)/doc
	cd $(DISTDIR); md5 $(FULLNAME)-eye.tar.bz2 > $(FULLNAME)-eye.tar.bz2.md5

$(DISTDIR)/$(FULLNAME)-claw.tar.bz2: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); tar jcvfL $(FULLNAME)-claw.tar.bz2 $(FULLNAME)/slashem $(FULLNAME)/doc
	cd $(DISTDIR); md5 $(FULLNAME)-claw.tar.bz2 > $(FULLNAME)-claw.tar.bz2.md5

$(DISTDIR)/$(FULLNAME)-full.zip: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); zip -y -r -9 $(FULLNAME)-full.zip $(FULLNAME)
	cd $(DISTDIR); md5 $(FULLNAME)-full.zip > $(FULLNAME)-full.zip.md5

$(DISTDIR)/$(FULLNAME)-eye.zip: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); zip -r -9 $(FULLNAME)-eye.zip $(FULLNAME)/nethack $(FULLNAME)/doc
	cd $(DISTDIR); md5 $(FULLNAME)-eye.zip > $(FULLNAME)-eye.zip.md5

$(DISTDIR)/$(FULLNAME)-claw.zip: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); zip -r -9 $(FULLNAME)-claw.zip $(FULLNAME)/slashem $(FULLNAME)/doc
	cd $(DISTDIR); md5 $(FULLNAME)-claw.zip > $(FULLNAME)-claw.zip.md5

$(DISTDIR)/Unix\ Installer: $(DISTDIR)
	mkdir $(DISTDIR)/Unix\ Installer

$(DISTDIR)/Unix\ Installer/$(FULLNAME)-full_unix-1.bin.sh: $(DISTDIR)/$(FULLNAME) $(DISTDIR)/Unix\ Installer
	cd $(DISTDIR); makeself $(FULLNAME) Unix\ Installer/$(FULLNAME)-full_unix-1.bin.sh $(FULLNAME) "make home"
	cd $(DISTDIR)/Unix\ Installer; md5 $(FULLNAME)-full_unix-1.bin.sh > $(FULLNAME)-full_unix-1.bin.sh.md5

$(DISTDIR)/Unix\ Installer/$(FULLNAME)-eye_unix-1.bin.sh: $(DISTDIR)/$(FULLNAME) $(DISTDIR)/Unix\ Installer
	cd $(DISTDIR); mv $(FULLNAME)/slashem .slashem; makeself $(FULLNAME) Unix\ Installer/$(FULLNAME)-eye_unix-1.bin.sh $(FULLNAME) "make nethack-home"; mv .slashem $(FULLNAME)/slashem
	cd $(DISTDIR)/Unix\ Installer; md5 $(FULLNAME)-eye_unix-1.bin.sh > $(FULLNAME)-eye_unix-1.bin.sh.md5

$(DISTDIR)/Unix\ Installer/$(FULLNAME)-claw_unix-1.bin.sh: $(DISTDIR)/$(FULLNAME) $(DISTDIR)/Unix\ Installer
	cd $(DISTDIR); mv $(FULLNAME)/nethack .nethack; makeself $(FULLNAME) Unix\ Installer/$(FULLNAME)-claw_unix-1.bin.sh $(FULLNAME) "make slashem-home"; mv .nethack $(FULLNAME)/nethack
	cd $(DISTDIR)/Unix\ Installer; md5 $(FULLNAME)-claw_unix-1.bin.sh > $(FULLNAME)-claw_unix-1.bin.sh.md5

$(DISTDIR)/$(FULLNAME): $(DISTDIR)
	cp -r _darcs/current $@
	@$(MAKE) -C doc changelog.html
	cp doc/changelog.rst doc/changelog.html $@/doc/
	echo "#define JTP_PORT_VERSION \"$(VERSION)\"">$@/vultures/vultureversion.h
	ln -s ../../vultures $@/nethack/win/vultures
	ln -s ../../vultures $@/slashem/win/vultures

$(DISTDIR):
	mkdir -p $(DISTDIR)

