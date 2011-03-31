# Windows Makefile for vulture
# Note: this Makefile is meant to be used in a Mingw/Msys shell
# reading build_instructions.txt is strongly recommended
#
# Created in March 2011 by Daniel Thaler

CFLAGS := -O2 -march=i686
SDL_CFLAGS := $(shell sdl-config --cflags)

# static link everything so that we don't need to ship tons of dependencies
# NOTE: the oder the libraries appear in matters for static linking!
LDFLAGS := -static -lpng -lz -lSDL_ttf -lfreetype -lstdc++ -lSDL_mixer -lvorbisfile \
           -lvorbis -logg -L/mingw/lib -lmingw32 -lSDLmain -lSDL -lwinmm -mwindows

####################################

ifeq ($(VARIANT), vulture-nethack)

VULTURE_BIN = vulture-nethack
BUILDDIR = vulture-nethack-build
NHDIR = nethack
CFLAGS += -D VULTURE_NETHACK

all: vulture-binary

else ifeq ($(VARIANT), vulture-slashem)

VULTURE_BIN = vulture-slashem
BUILDDIR = vulture-slashem-build
NHDIR = slashem
CFLAGS += -D VULTURE_SLASHEM

all: vulture-binary

else ifeq ($(VARIANT), vulture-unnethack)

VULTURE_BIN = vulture-unnethack
BUILDDIR = vulture-unnethack-build
NHDIR = unnethack
CFLAGS += -D VULTURE_UNNETHACK

all: vulture-binary

else

all:
	@echo "you must invoke this makefile with VARIANT=(vulture-nethack|vulture-slashem|vulture-unnethack)"
endif

####################################

INCLUDE = -I $(NHDIR)/include -I $(BUILDDIR)/include -I $(NHDIR)/sys/winnt -I vulture -I vulture/winclass


MAKEDEFS_SRC = $(NHDIR)/util/makedefs.c \
               $(NHDIR)/src/monst.c \
               $(NHDIR)/src/objects.c
DGN_COMP_SRC = $(BUILDDIR)/src/dgn_yacc.c \
               $(BUILDDIR)/src/dgn_lex.c \
               $(NHDIR)/util/dgn_main.c \
               $(NHDIR)/util/panic.c \
               $(NHDIR)/src/alloc.c
LEV_COMP_SRC = $(BUILDDIR)/src/lev_yacc.c \
               $(BUILDDIR)/src/lev_lex.c \
               $(NHDIR)/util/lev_main.c \
               $(NHDIR)/util/panic.c \
               $(NHDIR)/src/alloc.c \
               $(NHDIR)/src/drawing.c \
               $(NHDIR)/src/monst.c \
               $(NHDIR)/src/objects.c \
               $(NHDIR)/src/decl.c
TILEMAP_SRC = $(NHDIR)/win/share/tilemap.c
RECOVER_SRC = $(NHDIR)/util/recover.c
DLB_SRC = $(NHDIR)/util/dlb_main.c \
          $(NHDIR)/src/dlb.c \
          $(NHDIR)/util/panic.c \
          $(NHDIR)/src/alloc.c
CORE_SRC = $(wildcard $(NHDIR)/src/*.c) \
           $(NHDIR)/sys/share/pcmain.c \
           $(NHDIR)/sys/share/pcsys.c \
           $(NHDIR)/sys/share/pcunix.c \
           $(NHDIR)/sys/share/random.c \
           $(NHDIR)/sys/winnt/winnt.c \
           $(BUILDDIR)/src/tile.c \
           $(BUILDDIR)/src/monstr.c \
           $(BUILDDIR)/src/vis_tab.c
VULTURE_SRC = $(wildcard vulture/*.cpp) \
              $(wildcard vulture/winclass/*.cpp) \
              $(BUILDDIR)/src/vulture_tileconfig.parser.cpp \
              $(BUILDDIR)/src/vulture_tileconfig.lexer.cpp


CORE_OBJ = $(patsubst %.c,$(BUILDDIR)/%.o, $(notdir $(CORE_SRC)))
VULTURE_OBJ = $(patsubst %.cpp,$(BUILDDIR)/%.o, $(notdir $(VULTURE_SRC)))

MAKEDEFS_GENERATED = $(BUILDDIR)/include/date.h \
                     $(BUILDDIR)/include/pm.h \
                     $(BUILDDIR)/include/onames.h \
                     $(BUILDDIR)/include/vis_tab.h
                     
ifeq ($(VARIANT), vulture-slashem)
MAKEDEFS_GENERATED += $(BUILDDIR)/include/filename.h
endif


ifeq ($(VARIANT), vulture-slashem)
vulture-binary:  $(BUILDDIR)/binary/nhshare  $(BUILDDIR)/binary/nhushare
else
vulture-binary:  $(BUILDDIR)/binary/nhdat
endif
vulture-binary: $(BUILDDIR)/makedefs $(BUILDDIR)/dgn_comp $(BUILDDIR)/lev_comp $(BUILDDIR)/tilemap \
                 $(BUILDDIR)/recover $(BUILDDIR)/binary/$(VULTURE_BIN)
	cp -r vulture/gamedata/* $(BUILDDIR)/binary/
	-@cp -f $(NHDIR)/dat/{license,history} $(BUILDDIR)/binary/
	-@cp -f $(NHDIR)/doc/{nethack.txt,slashem.txt,Guidebook.txt} $(BUILDDIR)/binary/
	

####################################
# data files

DESFILES := $(wildcard $(NHDIR)/dat/*.des)
HELPFILES := help hh cmdhelp history opthelp wizhelp
DATA_FILES := data oracles ttyoptions quest.dat rumors
ifeq ($(VARIANT), vulture-unnethack)
DATA_FILES := data oracles options quest.dat rumors
endif

# slashem shared data library
DLBFILES_SHARE := $(HELPFILES) $(DATA_FILES)
DLBFILES_SHARE_DEP := $(patsubst %,$(BUILDDIR)/dat/%, $(DATA_FILES))
$(BUILDDIR)/binary/nhshare: $(BUILDDIR)/dlb $(DLBFILES_SHARE_DEP)
	@mkdir -p $(BUILDDIR)/dat/ $(BUILDDIR)/binary/
	@cp $(NHDIR)/dat/{help,hh,cmdhelp,history,opthelp,wizhelp} $(BUILDDIR)/dat/
	(cd $(BUILDDIR)/dat/ ; ../dlb cf nhshare $(DLBFILES_SHARE) )
	@cp $(BUILDDIR)/dat/nhshare $(BUILDDIR)/binary/nhshare
	
# slashem unshared data library
DLBFILES_UNSHARE := dungeon
DLBFILES_UNSHARE_DEP := $(patsubst %,$(BUILDDIR)/dat/%, levs dungeon $(DATA_FILES))
$(BUILDDIR)/binary/nhushare: $(BUILDDIR)/dlb $(DLBFILES_UNSHARE_DEP)
	@mkdir -p $(BUILDDIR)/dat/ $(BUILDDIR)/binary/
	(cd $(BUILDDIR)/dat/ ; ../dlb cf nhushare $(DLBFILES_UNSHARE) `ls *.lev` )
	@cp $(BUILDDIR)/dat/nhushare $(BUILDDIR)/binary/nhushare
	
# nethack + unnethack data library
DLBFILES := $(HELPFILES) dungeon $(DATA_FILES)
DLBFILES_DEP := $(patsubst %,$(BUILDDIR)/dat/%, levs dungeon $(DATA_FILES))
$(BUILDDIR)/binary/nhdat: $(BUILDDIR)/dlb $(DLBFILES_DEP)
	@mkdir -p $(BUILDDIR)/dat/ $(BUILDDIR)/binary/
	@cp $(NHDIR)/dat/{help,hh,cmdhelp,history,opthelp,wizhelp} $(BUILDDIR)/dat/
	(cd $(BUILDDIR)/dat/ ; ../dlb cf nhdat $(DLBFILES) `ls *.lev` )
	@cp $(BUILDDIR)/dat/nhdat $(BUILDDIR)/binary/nhdat

$(BUILDDIR)/dat/data: $(NHDIR)/dat/data.base $(BUILDDIR)/makedefs
	@mkdir -p $(BUILDDIR)/dat/
	@cp $(NHDIR)/dat/data.base $(BUILDDIR)/dat/data.base
	(cd $(BUILDDIR)/dat/ ; ../makedefs -d )

$(BUILDDIR)/dat/levs: $(DESFILES) $(BUILDDIR)/lev_comp
	@mkdir -p $(BUILDDIR)/dat/
	@cp $(DESFILES) $(BUILDDIR)/dat
	@rm -f $(BUILDDIR)/dat/template.des
	@(cd $(BUILDDIR)/dat ; for X in $(notdir $(DESFILES)); do echo $$X; ../lev_comp $$X; done )
	@touch $(BUILDDIR)/dat/levs

$(BUILDDIR)/dat/dungeon: $(NHDIR)/dat/dungeon.def $(BUILDDIR)/makedefs $(BUILDDIR)/dgn_comp
	@mkdir -p $(BUILDDIR)/dat/
	@cp $(NHDIR)/dat/dungeon.def $(BUILDDIR)/dat/
	(cd $(BUILDDIR)/dat/ ; ../makedefs -e ; ../dgn_comp dungeon.pdf )

$(BUILDDIR)/dat/oracles: $(NHDIR)/dat/oracles.txt $(BUILDDIR)/makedefs
	@mkdir -p $(BUILDDIR)/dat/
	@cp $(NHDIR)/dat/oracles.txt $(BUILDDIR)/dat/
	(cd $(BUILDDIR)/dat/ ; ../makedefs -h )

$(BUILDDIR)/dat/ttyoptions: $(BUILDDIR)/include/date.h

$(BUILDDIR)/dat/quest.dat: $(NHDIR)/dat/quest.txt $(BUILDDIR)/makedefs
	@mkdir -p $(BUILDDIR)/dat/
	@cp $(NHDIR)/dat/quest.txt $(BUILDDIR)/dat/
	(cd $(BUILDDIR)/dat/ ; ../makedefs -q )

$(BUILDDIR)/dat/rumors: $(NHDIR)/dat/rumors.tru $(NHDIR)/dat/rumors.fal $(BUILDDIR)/makedefs
	@mkdir -p $(BUILDDIR)/dat/
	@cp $(NHDIR)/dat/rumors.* $(BUILDDIR)/dat/
	(cd $(BUILDDIR)/dat/ ; ../makedefs -r )


####################################
# makedefs and source files generated by makedefs

$(BUILDDIR)/makedefs: $(MAKEDEFS_SRC)
	@mkdir -p $(BUILDDIR)
	gcc $(CFLAGS) $(INCLUDE) $(MAKEDEFS_SRC) -o $(BUILDDIR)/makedefs


$(BUILDDIR)/include/date.h: $(BUILDDIR)/makedefs
	@mkdir -p $(BUILDDIR)/include/ $(BUILDDIR)/dat/
	(cd $(BUILDDIR)/include/ ; ../makedefs -v )

$(BUILDDIR)/include/pm.h: $(BUILDDIR)/makedefs
	@mkdir -p $(BUILDDIR)/include/
	(cd $(BUILDDIR)/include/ ; ../makedefs -p )

$(BUILDDIR)/include/onames.h: $(BUILDDIR)/makedefs
	@mkdir -p $(BUILDDIR)/include/
	(cd $(BUILDDIR)/include/ ; ../makedefs -o )

$(BUILDDIR)/include/vis_tab.h: $(BUILDDIR)/makedefs
	@mkdir -p $(BUILDDIR)/include/ $(BUILDDIR)/src/
	(cd $(BUILDDIR)/include/ ; ../makedefs -z )

$(BUILDDIR)/src/vis_tab.c: $(BUILDDIR)/include/vis_tab.h

$(BUILDDIR)/src/monstr.c: $(BUILDDIR)/makedefs
	@mkdir -p $(BUILDDIR)/src/
	(cd $(BUILDDIR)/src/ ; ../makedefs -m )
	
ifeq ($(VARIANT), vulture-slashem)
$(BUILDDIR)/include/filename.h : $(BUILDDIR)/makedefs
	@mkdir -p $(BUILDDIR)/include/
	@cp $(NHDIR)/include/file.h $(BUILDDIR)/include/
	(cd $(BUILDDIR)/include/ ; ../makedefs -f )
	@rm $(BUILDDIR)/include/file.h
endif

####################################
# dungeon compiler

$(BUILDDIR)/src/dgn_yacc.c: $(NHDIR)/util/dgn_comp.y
	@mkdir -p $(BUILDDIR)/src $(BUILDDIR)/include
	bison -y --defines=$(BUILDDIR)/include/dgn_comp.h -o $(BUILDDIR)/src/dgn_yacc.c $(NHDIR)/util/dgn_comp.y

$(BUILDDIR)/src/dgn_lex.c: $(NHDIR)/util/dgn_comp.l
	@mkdir -p $(BUILDDIR)/src
	flex -o $(BUILDDIR)/src/dgn_lex.c $(NHDIR)/util/dgn_comp.l

$(BUILDDIR)/dgn_comp: $(DGN_COMP_SRC) $(MAKEDEFS_GENERATED)
	@mkdir -p $(BUILDDIR)
	gcc $(CFLAGS) $(INCLUDE) $(DGN_COMP_SRC) -o $(BUILDDIR)/dgn_comp

####################################
# level compiler

$(BUILDDIR)/src/lev_yacc.c: $(NHDIR)/util/lev_comp.y
	@mkdir -p $(BUILDDIR)/src $(BUILDDIR)/include
	bison -y --defines=$(BUILDDIR)/include/lev_comp.h -o $(BUILDDIR)/src/lev_yacc.c $(NHDIR)/util/lev_comp.y

$(BUILDDIR)/src/lev_lex.c: $(NHDIR)/util/lev_comp.l
	@mkdir -p $(BUILDDIR)/src
	flex -o $(BUILDDIR)/src/lev_lex.c $(NHDIR)/util/lev_comp.l

$(BUILDDIR)/lev_comp: $(LEV_COMP_SRC) $(MAKEDEFS_GENERATED)
	@mkdir -p $(BUILDDIR)
	gcc $(CFLAGS) $(INCLUDE) $(LEV_COMP_SRC) -o $(BUILDDIR)/lev_comp

####################################
# tilemap

$(BUILDDIR)/tilemap: $(TILEMAP_SRC)  $(BUILDDIR)/include/pm.h $(BUILDDIR)/include/onames.h
	@mkdir -p $(BUILDDIR)
	gcc $(CFLAGS) $(INCLUDE) $(TILEMAP_SRC) -o $(BUILDDIR)/tilemap

ifeq ($(VARIANT), vulture-slashem)
$(BUILDDIR)/src/tile.c: $(BUILDDIR)/tilemap
	@mkdir -p $(BUILDDIR)/src/ $(BUILDDIR)/win/share/
	@cp $(NHDIR)/win/share/tile_t.h $(BUILDDIR)/win/share/
	(cd $(BUILDDIR)/src/ ; ../tilemap )

else
$(BUILDDIR)/src/tile.c: $(BUILDDIR)/tilemap
	@mkdir -p $(BUILDDIR)/src/
	(cd $(BUILDDIR)/src/ ; ../tilemap )
endif
	
####################################
# recover

$(BUILDDIR)/recover: $(RECOVER_SRC)
	@mkdir -p $(BUILDDIR)
	gcc $(CFLAGS) $(INCLUDE) $(RECOVER_SRC) -o $(BUILDDIR)/recover
	
####################################
# data librarian

$(BUILDDIR)/dlb: $(DLB_SRC)
	@mkdir -p $(BUILDDIR)
	gcc $(CFLAGS) $(INCLUDE) $(DLB_SRC) -o $(BUILDDIR)/dlb
	
####################################
# main binary

$(BUILDDIR)/src/vulture_tileconfig.parser.cpp: vulture/vulture_tileconfig.parser.y
	@mkdir -p $(BUILDDIR)/src $(BUILDDIR)/include
	bison --defines=$(BUILDDIR)/include/vulture_tileconfig.parser.h -o $(BUILDDIR)/src/vulture_tileconfig.parser.cpp vulture/vulture_tileconfig.parser.y
	
$(BUILDDIR)/src/vulture_tileconfig.lexer.cpp: vulture/vulture_tileconfig.lexer.l
	@mkdir -p $(BUILDDIR)/src
	flex -o $(BUILDDIR)/src/vulture_tileconfig.lexer.cpp vulture/vulture_tileconfig.lexer.l

## implicit build rules

$(BUILDDIR)/%.o: vulture/%.cpp $(MAKEDEFS_GENERATED)
	@echo compiling $<
	@g++ $(CFLAGS) $(SDL_CFLAGS) $(INCLUDE) -c $< -o $@

$(BUILDDIR)/%.o: vulture/winclass/%.cpp $(MAKEDEFS_GENERATED)
	@echo compiling $<
	@g++ $(CFLAGS) $(SDL_CFLAGS) $(INCLUDE) -c $< -o $@

$(BUILDDIR)/%.o: $(BUILDDIR)/src/%.cpp $(MAKEDEFS_GENERATED)
	@echo compiling $<
	@g++ $(CFLAGS) $(SDL_CFLAGS) $(INCLUDE) -c $< -o $@


$(BUILDDIR)/%.o: $(NHDIR)/src/%.c $(MAKEDEFS_GENERATED)
	@echo compiling $<
	@gcc $(CFLAGS) $(SDL_CFLAGS) $(INCLUDE) -c $< -o $@

$(BUILDDIR)/%.o: $(NHDIR)/sys/share/%.c $(MAKEDEFS_GENERATED)
	@echo compiling $<
	@gcc $(CFLAGS) $(SDL_CFLAGS) $(INCLUDE) -c $< -o $@

$(BUILDDIR)/%.o: $(NHDIR)/sys/winnt/%.c $(MAKEDEFS_GENERATED)
	@echo compiling $<
	@gcc $(CFLAGS) $(SDL_CFLAGS) $(INCLUDE) -c $< -o $@

$(BUILDDIR)/%.o: $(BUILDDIR)/src/%.c $(MAKEDEFS_GENERATED)
	@echo compiling $<
	@gcc $(CFLAGS) $(SDL_CFLAGS) $(INCLUDE) -c $< -o $@

##

$(BUILDDIR)/binary/$(VULTURE_BIN): $(CORE_OBJ) $(VULTURE_OBJ)
	@mkdir -p $(BUILDDIR)/binary/
	gcc $(CORE_OBJ) $(VULTURE_OBJ) $(LDFLAGS) -o $(BUILDDIR)/binary/$(VULTURE_BIN)
	strip $(BUILDDIR)/binary/$(VULTURE_BIN).exe



clean:
	rm -rf vulture-nethack-build vulture-slashem-build vulture-unnethack-build

.PHONY: all clean vulture-binary std