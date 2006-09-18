WALL_WIDTH=57
WALL_HEIGHT=123

POVDIR=pov
TMP=tmp

all: walls02a.png

$(TMP):
	mkdir -p $(TMP)

# walls02a ##########################################################

walls02a.png: templates/walls02a.png \
		$(TMP)/walls02a_0l.png $(TMP)/walls02a_0r.png \
		$(TMP)/walls02a_6l.png $(TMP)/walls02a_6r.png
	cp templates/walls02a.png $@
	# walls02a_0
	composite \
		$(TMP)/walls02a_0l.png -geometry +121+1 \
		$@ -depth 8 $@
	composite \
		$(TMP)/walls02a_0r.png -geometry +181+1 \
		$@ -depth 8 $@
	# walls02a_6
	composite \
		$(TMP)/walls02a_6l.png -geometry +121+253 \
		$@ -depth 8 $@
	composite \
		$(TMP)/walls02a_6r.png -geometry +181+253 \
		$@ -depth 8 $@

##### walls02a_0 ####################################################

$(POVDIR)/walls02a_0.png:
	$(MAKE) -C $(POVDIR)

$(TMP)/walls02a_0l.png: $(TMP) $(POVDIR)/walls02a_0.png
	convert $(POVDIR)/walls02a_0.png -crop $(WALL_WIDTH)x$(WALL_HEIGHT)+0+0 $@

$(TMP)/walls02a_0r.png: $(TMP) $(POVDIR)/walls02a_0.png
	convert $(POVDIR)/walls02a_0.png -crop $(WALL_WIDTH)x$(WALL_HEIGHT)+$(WALL_WIDTH)+0 $@

##### walls02a_6 ####################################################

$(POVDIR)/walls02a_6.png:
	$(MAKE) -C $(POVDIR)

$(TMP)/walls02a_6l.png: $(TMP) $(POVDIR)/walls02a_6.png
	convert $(POVDIR)/walls02a_6.png -crop $(WALL_WIDTH)x$(WALL_HEIGHT)+0+0 $@

$(TMP)/walls02a_6r.png: $(TMP) $(POVDIR)/walls02a_6.png
	convert $(POVDIR)/walls02a_6.png -crop $(WALL_WIDTH)x$(WALL_HEIGHT)+$(WALL_WIDTH)+0 $@

#####################################################################

clean:
	$(MAKE) -C $(POVDIR) clean
	rm -f walls02a*
	rm -rf tmp
