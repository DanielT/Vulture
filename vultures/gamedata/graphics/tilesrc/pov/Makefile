WALL_WIDTH=57
WALL_WIDTHBOTH = 114
WALL_HEIGHT=123

all: walls02a_0.png walls02a_6.png

walls02a_0.png: walls02a_0.pov walltypes/solidbrick.inc setup/environment.inc
	povray +FN +UA -D -W$(WALL_WIDTHBOTH) -H$(WALL_HEIGHT) walls02a_0.pov

walls02a_6.png: walls02a_6.pov walltypes/marble.inc setup/environment.inc
	povray +FN +UA -D -W$(WALL_WIDTHBOTH) -H$(WALL_HEIGHT) walls02a_6.pov

clean:
	rm -f *.png
