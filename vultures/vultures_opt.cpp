/* NetHack may be freely redistributed.  See license for details. */

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "vultures_win.h"
#include "vultures_sound.h"
#include "vultures_gen.h"
#include "vultures_main.h"
#include "vultures_tile.h"
#include "vultures_sdl.h"
#include "vultures_opt.h"

#include "winclass/levelwin.h"

enum interface_opts_menu_ids {
	V_IOMID_RECENTER = 1,
	V_IOMID_PLAY_MUSIC,
	V_IOMID_PLAY_EFFECTS,
	V_IOMID_WALL_STYLE,
	V_IOMID_WALL_OPACITY,
	V_IOMID_WIDTH,
	V_IOMID_HEIGHT,
	V_IOMID_FULLSCREEN,
	V_IOMID_HELPTB,
	V_IOMID_ACTIONTB,
	V_IOMID_MINIMAP,
	V_IOMID_USESTANDARDINVENTORY,
	V_IOMID_USESTANDARDOBJMENUS,
	V_IOMID_MESSAGELINES,
	V_IOMID_KEYROTATION,
	V_IOMID_HIGHLIGHT_CURSOR_SQUARE,
	V_IOMID_DEBUG,
	V_IOMID_MACROS = 99
};



struct vultures_optstruct vultures_opts;


/* read vultures.conf and set up all options */
void vultures_read_main_config(FILE * fp)
{
	char *optend, *param;
	char configline[1024];
	int macronum = 0, len;

	while (fgets(configline, sizeof(configline), fp))
	{
		if (configline[0] == '%')
			continue;

		optend = strchr(configline, '=');
		if (!optend)
			continue;

		param = optend+1;

		/* remove spaces from the end of the option name */
		while ( isspace(*(optend-1)) )
			optend--;

		/* remove spaces from the start of the parameter */
		while ( isspace(*param) )
			param++;

		if (!strncmp(configline, "screen_xsize", optend - configline))
			vultures_opts.width = atoi(param);
		else if (!strncmp(configline, "screen_ysize", optend - configline))
			vultures_opts.height = atoi(param);
		else if (!strncmp(configline, "wall_style", optend - configline))
		{
			if (!strncmp(param, "full", 4))
				vultures_opts.wall_style = V_WALL_DISPLAY_STYLE_FULL;
			else if (!strncmp(param, "half_height", 11))
				vultures_opts.wall_style = V_WALL_DISPLAY_STYLE_HALF_HEIGHT;
		}
		else if (!strncmp(configline, "wall_opacity", optend - configline))
			vultures_opts.wall_opacity = atof(param);
		else if (!strncmp(configline, "recenter_after_movement", optend - configline))
			vultures_opts.recenter = atoi(param);
		else if (!strncmp(configline, "play_music", optend - configline))
			vultures_opts.play_music = atoi(param);
		else if (!strncmp(configline, "play_effects", optend - configline))
			vultures_opts.play_effects = atoi(param);
		else if (!strncmp(configline, "fullscreen", optend - configline))
			vultures_opts.fullscreen = atoi(param);
		else if (!strncmp(configline, "show_helpbar", optend - configline))
			vultures_opts.show_helptb = atoi(param);
		else if (!strncmp(configline, "show_actionbar", optend - configline))
			vultures_opts.show_actiontb = atoi(param);
		else if (!strncmp(configline, "show_minimap", optend - configline))
			vultures_opts.show_minimap = atoi(param);
		else if (!strncmp(configline, "use_standard_inventory", optend - configline))
			vultures_opts.use_standard_inventory = atoi(param);
		else if (!strncmp(configline, "use_standard_objmenus", optend - configline))
			vultures_opts.use_standard_object_menus = atoi(param);
		else if (!strncmp(configline, "messagelines", optend - configline))
			vultures_opts.messagelines = atoi(param);
		else if (!strncmp(configline, "no_key_translation", optend - configline))
			vultures_opts.no_key_translation = atoi(param);
		else if (!strncmp(configline, "highlight_cursor_square", optend - configline))
			vultures_opts.highlight_cursor_square = atoi(param);
		else if (!strncmp(configline, "debug", optend - configline))
			vultures_opts.debug = atoi(param);
		else if (!strncmp(configline, "macro", 5))
		{
			macronum = atoi(configline+5);
			if (macronum > 0 && macronum < 7)
			{
				strncpy(vultures_opts.macro[macronum-1], param, 9);
				vultures_opts.macro[macronum-1][9] = '\0';

				len = strlen(vultures_opts.macro[macronum-1]);
				while (len >= 0 && (isspace(vultures_opts.macro[macronum-1][len]) ||
					vultures_opts.macro[macronum-1][len] == '\0'))
					vultures_opts.macro[macronum-1][len--] = '\0';
			}
		}
	}
}


void vultures_read_sound_config(FILE * fp)
{
	char configline[1024];
	char buffer[1024];
	char *tok, *tok2;
	int soundtype;
	vultures_event_sound cur_event_sound;

	while (fgets(configline, sizeof(configline), fp))
	{
		if ((configline[0] == '%') || (configline[0] == '\0') ||
			(configline[0] == 10) || (configline[0] == 13))
			/* comment or empty line */
			continue;


		/* Check for sound type */

		soundtype = -1;
		if ((tok = strstr(configline, "],SND,[")) != NULL) /* sound found */
			soundtype = V_EVENT_SOUND_TYPE_SND;
		else if ((tok = strstr(configline, "],MUS,[")) != NULL) /* music found */      
			soundtype = V_EVENT_SOUND_TYPE_MUS;
		else if ((tok = strstr(configline, "],RSNG,[")) != NULL) /* Random song file found */
			soundtype = V_EVENT_SOUND_TYPE_RANDOM_SONG;
		else if ((tok = strstr(configline, "],CDAU,[")) != NULL) /* CD audio track found */
			soundtype = V_EVENT_SOUND_TYPE_CD_AUDIO;
		else if ((tok = strstr(configline, "],NONE,[")) != NULL) /* NONE placeholder found */
			soundtype = V_EVENT_SOUND_TYPE_NONE;
		else
			/* invalid sound type */
			continue;


		cur_event_sound.searchpattern = new char[V_MAX_FILENAME_LENGTH];
		memcpy(cur_event_sound.searchpattern, configline+1, tok-configline-1);
		cur_event_sound.searchpattern[tok-configline-1] = '\0';

		/* Check for a background music event */
		if (!strcmp(cur_event_sound.searchpattern, "nhfe_music_background")) {
			vultures_n_background_songs++;
			delete cur_event_sound.searchpattern;
			cur_event_sound.searchpattern = (char *)malloc(strlen("nhfe_music_background")+4);
			sprintf(cur_event_sound.searchpattern, "nhfe_music_background%03d",
                    vultures_n_background_songs-1);
		}

		if (soundtype == V_EVENT_SOUND_TYPE_SND ||
			soundtype == V_EVENT_SOUND_TYPE_MUS)
			tok = tok + 7;
		else
			tok = tok + 8;

		tok2 = strstr(tok, "]");
		memcpy(buffer, tok, tok2-tok);
		buffer[tok2-tok] = '\0';

		cur_event_sound.filename = buffer;
		cur_event_sound.soundtype = soundtype;

		/* If this isn't a CD track, add path to sounds subdirectory before filename */
		if (soundtype != V_EVENT_SOUND_TYPE_CD_AUDIO)
			cur_event_sound.filename =
				vultures_make_filename(soundtype == V_EVENT_SOUND_TYPE_SND ? V_SOUND_DIRECTORY : V_MUSIC_DIRECTORY, "", cur_event_sound.filename);
		
		vultures_event_sounds.push_back(cur_event_sound);
	} /* while (fgets(...)) */
}



string vultures_get_userdir(void)
{
	char userdir[512];
	
#ifdef WIN32
	/* %appdir% = X:\Documents and Settings\<username>\Application Data */
	snprintf(userdir, 512, "%s\\Vultures\\", getenv("APPDATA"));
#else
	/* everywhere else we use the user's homedir (/home/<name>/) */
	snprintf(userdir, 512, "%s/.vultures/", getenv("HOME"));
#endif

	return userdir;
}


void vultures_read_options(void)
{
	FILE *fp;
	string filename;
	int i;
	string userdir = vultures_get_userdir();
	
	/* initialize these, in case they aren't set in the config file */
	vultures_opts.wall_opacity = 1.0;
	vultures_opts.wall_style = V_WALL_DISPLAY_STYLE_FULL;
	vultures_opts.width = 800;
	vultures_opts.height = 600;
	vultures_opts.fullscreen = 0;
	vultures_opts.play_music = 0;
	vultures_opts.play_effects = 0;
	vultures_opts.show_helptb = 1;
	vultures_opts.show_actiontb = 1;
	vultures_opts.show_minimap = 1;
	vultures_opts.use_standard_inventory = 0;
	vultures_opts.use_standard_object_menus = 0;
	vultures_opts.messagelines = 3;
	vultures_opts.no_key_translation = 0;
	vultures_opts.highlight_cursor_square = 1;
	vultures_opts.debug = 0;

	strcpy(vultures_opts.macro[0], "n100.");
	strcpy(vultures_opts.macro[1], "n20s");
	for (i = 2; i < 6; i++)
		vultures_opts.macro[i][0] = '\0';

	/* Read interface options */
	
	/* main config file */
	filename = vultures_make_filename(V_CONFIG_DIRECTORY, "", V_FILENAME_OPTIONS);
	fp = fopen(filename.c_str(), "rb");

	if (fp) {
		vultures_read_main_config(fp);
		fclose(fp);
	}
	else
		printf("Could nor open %s: %s\n", filename.c_str(), strerror(errno));

	/* user's config file */
	filename = userdir + V_FILENAME_OPTIONS;
	fp = fopen(filename.c_str(), "rb");

	if (fp) {
		vultures_read_main_config(fp);
		fclose(fp);
	}

	if (vultures_opts.wall_opacity > 1.0 || vultures_opts.wall_opacity < 0) {
		fprintf(stderr, "WARNING: detected an invalid value for wall_opacity. Set 1.0 instead.\n");
		vultures_opts.wall_opacity = 1.0;
	}

	/* minimum window size: the map + inventory windows need at least this much space */
	if (vultures_opts.width < 640)
		vultures_opts.width = 640;
	if (vultures_opts.height < 480)
		vultures_opts.height = 480;


	/* Read event sounds options */
	vultures_event_sounds.clear();

	filename = vultures_make_filename(V_CONFIG_DIRECTORY, "", V_FILENAME_SOUNDS_CONFIG);
	fp = fopen(filename.c_str(), "rb");

	if (fp) {
		vultures_read_sound_config(fp);
		fclose(fp);
	}


	/* user's sound config file */
	filename = userdir + V_FILENAME_SOUNDS_CONFIG;
	fp = fopen(filename.c_str(), "rb");

	if (fp) {
		vultures_read_main_config(fp);
		fclose(fp);
	}
}


void vultures_write_userconfig(void)
{
	string filename, dir;
	struct stat statbuf;
	mode_t oldmask;
	int i;
	FILE *fp;

	dir = vultures_get_userdir();

#ifdef WIN32
	/* On windows just try to create the directory and be done with it.
	* This should work everywhere people haven't changed permissions to
	* something really whacky and if they have, they'll just have to fix
	* it themselves */
	mkdir(dir.c_str());
#else

	/* elsewhere we try to be more intelligent: create the dir only if it doesn't
	* exist, and when we do so, use a sane umask + permissions */
	if (stat(dir.c_str(), &statbuf) == -1)
	{
		if (errno == ENOENT) {
			oldmask = umask(0022);

			if (mkdir(dir.c_str(), 0755) == -1) {
				printf("Could not create %s: %s\n", dir.c_str(), strerror(errno));
				return;
			}
			umask(oldmask);
		} else {
			printf("Could not stat %s: %s\n", dir.c_str(), strerror(errno));
			return;
		}
	}
#endif
		
	filename = dir + V_FILENAME_OPTIONS;
	fp = fopen(filename.c_str(), "wb");
	if (fp)
	{
		fprintf(fp, "screen_xsize=%d\n", vultures_opts.width);
		fprintf(fp, "screen_ysize=%d\n", vultures_opts.height);
		fprintf(fp, "fullscreen=%d\n",   vultures_opts.fullscreen);
		fprintf(fp, "play_music=%d\n",   vultures_opts.play_music);
		fprintf(fp, "play_effects=%d\n", vultures_opts.play_effects);
		fprintf(fp, "wall_style=%s\n",   vultures_opts.wall_style == V_WALL_DISPLAY_STYLE_FULL ?
										"full" : "half_height");
		fprintf(fp, "wall_opacity=%1.1f\n", vultures_opts.wall_opacity);
		fprintf(fp, "recenter_after_movement=%d\n", vultures_opts.recenter);
		fprintf(fp, "show_helpbar=%d\n", vultures_opts.show_helptb);
		fprintf(fp, "show_actionbar=%d\n", vultures_opts.show_actiontb);
		fprintf(fp, "show_minimap=%d\n", vultures_opts.show_minimap);
		fprintf(fp, "use_standard_inventory=%d\n", vultures_opts.use_standard_inventory);
		fprintf(fp, "use_standard_objmenus=%d\n", vultures_opts.use_standard_object_menus);
		fprintf(fp, "messagelines=%d\n", vultures_opts.messagelines);
		fprintf(fp, "no_key_translation=%d\n", vultures_opts.no_key_translation);
		fprintf(fp, "highlight_cursor_square=%d\n", vultures_opts.highlight_cursor_square);
		fprintf(fp, "debug=%d\n", vultures_opts.debug);

		for (i = 0; i < 6; i++)
			if (vultures_opts.macro[i][0] != '\0')
				fprintf(fp, "macro%d=%s\n", i+1, vultures_opts.macro[i]);

		fprintf(fp, "\n");
		fclose(fp);
	}
	else
		printf("Could not open %s for writing: %s\n", filename.c_str(), strerror(errno));
}


/* display the interface options dialog */
int vultures_iface_opts(void)
{
	struct window * win;
	int winid, num_selected, num_selected_sub, i;
	int size_changed = 0;
	anything any;
	menu_item *selected, *selected_sub;
	char * str;

	/* create a menu window */
	winid = vultures_create_nhwindow(NHW_MENU);
	vultures_start_menu(winid);
	
	str = new char[256];

	any.a_int = V_IOMID_RECENTER;
	sprintf(str, "Recenter after movement\t[%s]", vultures_opts.recenter ? "yes" : "no");
	vultures_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_PLAY_MUSIC;
	sprintf(str, "Play music\t[%s]", vultures_opts.play_music ? "yes" : "no");
	vultures_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_PLAY_EFFECTS;
	sprintf(str, "Play effects\t[%s]", vultures_opts.play_effects ? "yes" : "no");
	vultures_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_WALL_STYLE;
	sprintf(str, "Wall style\t[%s]", (vultures_opts.wall_style == V_WALL_DISPLAY_STYLE_FULL) ?
				"full" : "half-height");
	vultures_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_WALL_OPACITY;
	sprintf(str, "Wall opacity\t[%01.1f]", vultures_opts.wall_opacity);
	vultures_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_WIDTH;
	sprintf(str, "Width\t[%d]", vultures_opts.width);
	vultures_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_HEIGHT;
	sprintf(str, "Height\t[%d]", vultures_opts.height);
	vultures_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_FULLSCREEN;
	sprintf(str, "Fullscreen mode\t[%s]", vultures_opts.fullscreen ? "yes" : "no");
	vultures_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_HELPTB;
	sprintf(str, "Show helpbar\t[%s]", vultures_opts.show_helptb ? "yes" : "no");
	vultures_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_ACTIONTB;
	sprintf(str, "Show actionbar\t[%s]", vultures_opts.show_actiontb ? "yes" : "no");
	vultures_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_MINIMAP;
	sprintf(str, "Show minimap\t[%s]", vultures_opts.show_minimap ? "yes" : "no");
	vultures_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_USESTANDARDINVENTORY;
	sprintf(str, "Use plain inventory\t[%s]", vultures_opts.use_standard_inventory ? "yes" : "no");
	vultures_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_USESTANDARDOBJMENUS;
	sprintf(str, "Use plain object menus\t[%s]", vultures_opts.use_standard_object_menus ? "yes" : "no");
	vultures_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_MESSAGELINES;
	sprintf(str, "Message lines\t[%d]", vultures_opts.messagelines);
	vultures_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_KEYROTATION;
	sprintf(str, "Disable key rotation\t[%s]", vultures_opts.no_key_translation ? "yes" : "no");
	vultures_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_HIGHLIGHT_CURSOR_SQUARE;
	sprintf(str, "Highlight under the cursor\t[%s]", vultures_opts.highlight_cursor_square ? "yes" : "no");
	vultures_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_DEBUG;
	sprintf(str, "Run-time debugging\t[%s]", vultures_opts.debug ? "yes" : "no");
	vultures_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	for (i = 0; i < 6; i++)
	{
		any.a_int = V_IOMID_MACROS + i;
		sprintf(str, "F%d macro\t[%s]", i+1, vultures_opts.macro[i]);
		vultures_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);
	}

	/* display the menu and get the user's selection */
	vultures_end_menu(winid, "Interface options");
	num_selected = vultures_select_menu(winid, PICK_ANY, &selected);
	vultures_destroy_nhwindow(winid);

	/* toggle the selected options or get new values from the user */
	for (i = 0; i < num_selected; i++)
	{
		switch(selected[i].item.a_int)
		{
			case V_IOMID_RECENTER:
				vultures_opts.recenter = !vultures_opts.recenter;
				break;

			case V_IOMID_PLAY_MUSIC:
				vultures_opts.play_music = !vultures_opts.play_music;
				if (vultures_opts.play_music)
					vultures_init_sound();
				else
					vultures_stop_music();
				break;

			case V_IOMID_PLAY_EFFECTS:
				vultures_opts.play_effects = !vultures_opts.play_effects;
				if (vultures_opts.play_effects)
					vultures_init_sound();
				break;

			case V_IOMID_WALL_STYLE:
				winid = vultures_create_nhwindow(NHW_MENU);
				vultures_start_menu(winid);

				any.a_int = V_WALL_DISPLAY_STYLE_FULL+1;
				vultures_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, "full", MENU_UNSELECTED);

				any.a_int = V_WALL_DISPLAY_STYLE_HALF_HEIGHT+1;
				vultures_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, "half-height", MENU_UNSELECTED);

				vultures_end_menu(winid, "Wall style");
				num_selected_sub = vultures_select_menu(winid, PICK_ONE, &selected_sub);
				vultures_destroy_nhwindow(winid);

				if (num_selected_sub < 1)
					break;

				vultures_opts.wall_style = selected_sub[0].item.a_int-1;
				levwin->set_wall_style(vultures_opts.wall_style);
				levwin->force_redraw();
				vultures_display_nhwindow(WIN_MAP, 0);
				break;

			case V_IOMID_WALL_OPACITY:
				if (vultures_get_input(-1, -1, "New wall opacity", str))
				{
					vultures_opts.wall_opacity = atof(str);
					if (vultures_opts.wall_opacity > 1.0 || vultures_opts.wall_opacity < 0)
						vultures_opts.wall_opacity = 1.0;
				}
				/* flush tile arrays to ensure the new transparency gets used */
				vultures_tilecache_discard();
				/* force redraw */
				levwin->force_redraw();
				vultures_display_nhwindow(WIN_MAP, 0);
				break;

			case V_IOMID_WIDTH:
				if (vultures_get_input(-1, -1, "New width", str))
				{
					vultures_opts.width = atoi(str);
					if (vultures_opts.width < 640)
						vultures_opts.width = 640;
				}
				size_changed = 1;
				break;

			case V_IOMID_HEIGHT:
				if (vultures_get_input(-1, -1, "New height", str))
				{
					vultures_opts.height = atoi(str);
					if (vultures_opts.height < 510)
						vultures_opts.height = 510;
				}
				size_changed = 1;
				break;

			case V_IOMID_FULLSCREEN:
				vultures_opts.fullscreen = !vultures_opts.fullscreen;
				vultures_set_screensize();
				break;

			case V_IOMID_HELPTB:
				vultures_opts.show_helptb = !vultures_opts.show_helptb;
				win = ROOTWIN; /* map window */
				static_cast<levelwin*>(win)->toggle_uiwin(V_WIN_TOOLBAR2, vultures_opts.show_helptb);
				vultures_display_nhwindow(WIN_MAP, 0);
				break;

			case V_IOMID_ACTIONTB:
				vultures_opts.show_actiontb = !vultures_opts.show_actiontb;
				win = ROOTWIN; /* map window */
				static_cast<levelwin*>(win)->toggle_uiwin(V_WIN_TOOLBAR1, vultures_opts.show_actiontb);
				vultures_display_nhwindow(WIN_MAP, 0);
				break;

			case V_IOMID_MINIMAP:
				vultures_opts.show_minimap = !vultures_opts.show_minimap;
				win = ROOTWIN; /* map window */
				static_cast<levelwin*>(win)->toggle_uiwin(V_WIN_MINIMAP, vultures_opts.show_minimap);
				vultures_display_nhwindow(WIN_MAP, 0);
				break;

			case V_IOMID_USESTANDARDINVENTORY:
				vultures_opts.use_standard_inventory = ! vultures_opts.use_standard_inventory;
				break;

			case V_IOMID_USESTANDARDOBJMENUS:
				vultures_opts.use_standard_object_menus = ! vultures_opts.use_standard_object_menus;
				break;

			case V_IOMID_MESSAGELINES:
				if (vultures_get_input(-1, -1, "Number of lines in the message area", str))
				{
					vultures_opts.messagelines = atoi(str);
					if (vultures_opts.messagelines < 1 || vultures_opts.messagelines > 10)
						vultures_opts.messagelines = 3;
				}
				break;

			case V_IOMID_KEYROTATION:
				vultures_opts.no_key_translation = !vultures_opts.no_key_translation;
				break;

			case V_IOMID_HIGHLIGHT_CURSOR_SQUARE:
				vultures_opts.highlight_cursor_square = !vultures_opts.highlight_cursor_square;
				break;

			case V_IOMID_DEBUG:
				vultures_opts.debug = !vultures_opts.debug;
				break;

			default:
				if (selected[i].item.a_int >= V_IOMID_MACROS && selected[i].item.a_int <= (V_IOMID_MACROS+6))
				{
					if (vultures_get_input(-1, -1, "New macro", str))
					{
						strncpy(vultures_opts.macro[selected[i].item.a_int - V_IOMID_MACROS],str,9);
						vultures_opts.macro[selected[i].item.a_int - V_IOMID_MACROS][9] = '\0';
					}
				}
		}
	}

	/* if the screen size was changed, the actual resize happens here, rather
	* separately for width and height with 2 modechanges */
	if (size_changed)
		vultures_set_screensize();

	delete str;

	vultures_write_userconfig();

	return 0;
}


