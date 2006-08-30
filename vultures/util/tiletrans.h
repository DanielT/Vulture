/* Copyright (c) 2005 Daniel Thaler
 * This file may be freely distributed under the Nethack license*/

enum tiletype {
	T_OBJECT,
	T_SPECIAL,
	T_EXPL,
	T_DUNGEON,
	T_FLOOR,
	T_WALL,
	T_MONSTER,
	T_STATUE,
	T_FIGURINE,
	T_CURSOR,
	
	MAX_TYPES
};

struct tiledef {
	enum tiletype type;
	char * name;
	char * filename;
	int topleft_x, topleft_y;
	int width, height, line;
};

struct classdefmap {
	int class;
	char * mapto;
};

struct eqmap {
	enum tiletype type;
	char * mapfrom;
	char * mapto;
};

struct gametile {
	long file_offset;
	long data_len;
	int ptr_to;
	int hot_x;
	int hot_y;
	int line;
};

struct floorstyles {
	char * basename;
	int x;
	int y;
};

struct wallstyle {
	int north;
	int east;
	int south;
	int west;
};

