# This makefile is for unix and it's clones only.
# This makefile is meant for developers and people wanting to experiment with vultures only.
# It is NOT meant for system intallation, nor is it meant for building the game for final gameplay.
# If you want to play vultures either check if your distribution of unix has a port for it, or if
# you are comfortable with system installation yourself then read and follow the instructions of
# INSTALL_unix in the doc directory

GAME = vultures
DATE != date +%Y%m%d%H%M%S
VERSION = snapshot-$(DATE)
FULLNAME = $(GAME)-$(VERSION)
DISTDIR = dist/$(FULLNAME)

help:
	@echo "to build NetHack in your home directory: make nethack-home"
	@echo "to build Slashem in your home directory: make slashem-home"
	@echo "to build * Both  in your home directory: make home"

home: nethack-home slashem-home

nethack-home: nethack/Makefile nethack/win/jtp
	export PREFIX=$$HOME/$(GAME)/nethack/ && export CHOWN=true && export CHGRP=true && mkdir -p $$PREFIX && make -C nethack -e install

slashem-home: slashem/Makefile slashem/win/jtp
	export PREFIX=$$HOME/$(GAME)/slashem/ && export CHOWN=true && export CHGRP=true && mkdir -p $$PREFIX && make -C slashem -e install

nethack/Makefile:
	cd nethack && sh sys/unix/setup.sh -

nethack/win/jtp:
	cd nethack/win && ln -s ../../vultures/win/jtp

slashem/Makefile:
	cd slashem && sh sys/unix/setup.sh -

slashem/win/jtp:
	cd slashem/win && ln -s ../../vultures/win/jtp

clean:
	-make -C nethack clean
	-make -C slashem clean

spotless:
	-make -C nethack spotless
	-make -C slashem spotless
	-rm -rf dist

distfiles:	\
	$(DISTDIR)/$(FULLNAME)-full.tar.gz	\
	$(DISTDIR)/$(FULLNAME)-eye.tar.gz	\
	$(DISTDIR)/$(FULLNAME)-claw.tar.gz	\
	$(DISTDIR)/$(FULLNAME)-full.tar.bz2	\
	$(DISTDIR)/$(FULLNAME)-eye.tar.bz2	\
	$(DISTDIR)/$(FULLNAME)-claw.tar.bz2

$(DISTDIR)/$(FULLNAME)-full.tar.gz: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); tar zcvf $(FULLNAME)-full.tar.gz $(FULLNAME)
	cd $(DISTDIR); md5 $(FULLNAME)-full.tar.gz > $(FULLNAME)-full.tar.gz.md5

$(DISTDIR)/$(FULLNAME)-eye.tar.gz: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); tar zcvfL $(FULLNAME)-eye.tar.gz $(FULLNAME)/nethack
	cd $(DISTDIR); md5 $(FULLNAME)-eye.tar.gz > $(FULLNAME)-eye.tar.gz.md5

$(DISTDIR)/$(FULLNAME)-claw.tar.gz: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); tar zcvfL $(FULLNAME)-claw.tar.gz $(FULLNAME)/slashem
	cd $(DISTDIR); md5 $(FULLNAME)-claw.tar.gz > $(FULLNAME)-claw.tar.gz.md5

$(DISTDIR)/$(FULLNAME)-full.tar.bz2: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); tar jcvf $(FULLNAME)-full.tar.bz2 $(FULLNAME)
	cd $(DISTDIR); md5 $(FULLNAME)-full.tar.bz2 > $(FULLNAME)-full.tar.bz2.md5

$(DISTDIR)/$(FULLNAME)-eye.tar.bz2: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); tar jcvfL $(FULLNAME)-eye.tar.bz2 $(FULLNAME)/nethack
	cd $(DISTDIR); md5 $(FULLNAME)-eye.tar.bz2 > $(FULLNAME)-eye.tar.bz2.md5

$(DISTDIR)/$(FULLNAME)-claw.tar.bz2: $(DISTDIR)/$(FULLNAME)
	cd $(DISTDIR); tar jcvfL $(FULLNAME)-claw.tar.bz2 $(FULLNAME)/slashem
	cd $(DISTDIR); md5 $(FULLNAME)-claw.tar.bz2 > $(FULLNAME)-claw.tar.bz2.md5

$(DISTDIR)/$(FULLNAME): $(DISTDIR)
	cp -r _darcs/current $(.TARGET)
	echo "#define JTP_PORT_VERSION \"$(VERSION)\"">$(.TARGET)/vultures/win/jtp/vultureversion.h
	ln -s ../../vultures/win/jtp $(.TARGET)/nethack/win/jtp
	ln -s ../../vultures/win/jtp $(.TARGET)/slashem/win/jtp

$(DISTDIR):
	mkdir -p $(DISTDIR)

