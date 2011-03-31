/* NetHack may be freely redistributed.  See license for details. */

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "vulture_win.h"
#include "vulture_sound.h"
#include "vulture_gen.h"
#include "vulture_main.h"
#include "vulture_tile.h"
#include "vulture_sdl.h"
#include "vulture_opt.h"

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



struct vulture_optstruct vulture_opts;


/* read vulture.conf and set up all options */
void vulture_read_main_config(FILE * fp)
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
			vulture_opts.width = atoi(param);
		else if (!strncmp(configline, "screen_ysize", optend - configline))
			vulture_opts.height = atoi(param);
		else if (!strncmp(configline, "wall_style", optend - configline))
		{
			if (!strncmp(param, "full", 4))
				vulture_opts.wall_style = V_WALL_DISPLAY_STYLE_FULL;
			else if (!strncmp(param, "half_height", 11))
				vulture_opts.wall_style = V_WALL_DISPLAY_STYLE_HALF_HEIGHT;
		}
		else if (!strncmp(configline, "wall_opacity", optend - configline))
			vulture_opts.wall_opacity = atof(param);
		else if (!strncmp(configline, "recenter_after_movement", optend - configline))
			vulture_opts.recenter = atoi(param);
		else if (!strncmp(configline, "play_music", optend - configline))
			vulture_opts.play_music = atoi(param);
		else if (!strncmp(configline, "play_effects", optend - configline))
			vulture_opts.play_effects = atoi(param);
		else if (!strncmp(configline, "fullscreen", optend - configline))
			vulture_opts.fullscreen = atoi(param);
		else if (!strncmp(configline, "show_helpbar", optend - configline))
			vulture_opts.show_helptb = atoi(param);
		else if (!strncmp(configline, "show_actionbar", optend - configline))
			vulture_opts.show_actiontb = atoi(param);
		else if (!strncmp(configline, "show_minimap", optend - configline))
			vulture_opts.show_minimap = atoi(param);
		else if (!strncmp(configline, "use_standard_inventory", optend - configline))
			vulture_opts.use_standard_inventory = atoi(param);
		else if (!strncmp(configline, "use_standard_objmenus", optend - configline))
			vulture_opts.use_standard_object_menus = atoi(param);
		else if (!strncmp(configline, "messagelines", optend - configline))
			vulture_opts.messagelines = atoi(param);
		else if (!strncmp(configline, "no_key_translation", optend - configline))
			vulture_opts.no_key_translation = atoi(param);
		else if (!strncmp(configline, "highlight_cursor_square", optend - configline))
			vulture_opts.highlight_cursor_square = atoi(param);
		else if (!strncmp(configline, "debug", optend - configline))
			vulture_opts.debug = atoi(param);
		else if (!strncmp(configline, "macro", 5))
		{
			macronum = atoi(configline+5);
			if (macronum > 0 && macronum < 7)
			{
				strncpy(vulture_opts.macro[macronum-1], param, 9);
				vulture_opts.macro[macronum-1][9] = '\0';

				len = strlen(vulture_opts.macro[macronum-1]);
				while (len >= 0 && (isspace(vulture_opts.macro[macronum-1][len]) ||
					vulture_opts.macro[macronum-1][len] == '\0'))
					vulture_opts.macro[macronum-1][len--] = '\0';
			}
		}
	}
}


void vulture_read_sound_config(FILE * fp)
{
	char configline[1024];
	char buffer[1024];
	char *tok, *tok2;
	int soundtype;
	vulture_event_sound cur_event_sound;

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


    cur_event_sound.filenames.clear();
		cur_event_sound.searchpattern = new char[V_MAX_FILENAME_LENGTH];
		memcpy(cur_event_sound.searchpattern, configline+1, tok-configline-1);
		cur_event_sound.searchpattern[tok-configline-1] = '\0';

		/* Check for a background music event */
		if (!strcmp(cur_event_sound.searchpattern, "nhfe_music_background")) {
			vulture_n_background_songs++;
			delete cur_event_sound.searchpattern;
			cur_event_sound.searchpattern = (char *)malloc(strlen("nhfe_music_background")+4);
			sprintf(cur_event_sound.searchpattern, "nhfe_music_background%03d",
                    vulture_n_background_songs-1);
		}

		if (soundtype == V_EVENT_SOUND_TYPE_SND ||
			soundtype == V_EVENT_SOUND_TYPE_MUS)
			tok = tok + 7;
		else
			tok = tok + 8;

    while ( ( tok2 = strstr( tok, "," ) ) != 0 )
    {
      memcpy(buffer, tok, tok2-tok);
      buffer[tok2-tok] = '\0';
      std::string filename( buffer );

      cur_event_sound.soundtype = soundtype;

      /* If this isn't a CD track, add path to sounds subdirectory before filename */
      if (soundtype != V_EVENT_SOUND_TYPE_CD_AUDIO)
        cur_event_sound.filenames.push_back(
          vulture_make_filename(soundtype == V_EVENT_SOUND_TYPE_SND ? V_SOUND_DIRECTORY : V_MUSIC_DIRECTORY, "", filename ) );
      else
        cur_event_sound.filenames.push_back( filename );

      tok = tok2+1;
    }

		tok2 = strstr(tok, "]");

    memcpy(buffer, tok, tok2-tok);
    buffer[tok2-tok] = '\0';
    std::string filename( buffer );

    cur_event_sound.soundtype = soundtype;

    /* If this isn't a CD track, add path to sounds subdirectory before filename */
    if (soundtype != V_EVENT_SOUND_TYPE_CD_AUDIO)
      cur_event_sound.filenames.push_back(
        vulture_make_filename(soundtype == V_EVENT_SOUND_TYPE_SND ? V_SOUND_DIRECTORY : V_MUSIC_DIRECTORY, "", filename ) );
    else
      cur_event_sound.filenames.push_back( filename );
		
		vulture_event_sounds.push_back(cur_event_sound);
	} /* while (fgets(...)) */
}



std::string vulture_get_userdir(void)
{
	char userdir[512];
	
#ifdef WIN32
	/* %appdir% = X:\Documents and Settings\<username>\Application Data */
	snprintf(userdir, 512, "%s\\Vulture\\", getenv("APPDATA"));
#else
	/* everywhere else we use the user's homedir (/home/<name>/) */
	snprintf(userdir, 512, "%s/.vulture/", getenv("HOME"));
#endif

	return userdir;
}


void vulture_read_options(void)
{
	FILE *fp;
  std::string filename;
	int i;
  std::string userdir = vulture_get_userdir();
	
	/* initialize these, in case they aren't set in the config file */
	vulture_opts.wall_opacity = 0.8;
	vulture_opts.wall_style = V_WALL_DISPLAY_STYLE_FULL;
	vulture_opts.width = 800;
	vulture_opts.height = 600;
	vulture_opts.fullscreen = 0;
	vulture_opts.play_music = 0;
	vulture_opts.play_effects = 0;
	vulture_opts.show_helptb = 1;
	vulture_opts.show_actiontb = 1;
	vulture_opts.show_minimap = 1;
	vulture_opts.use_standard_inventory = 0;
	vulture_opts.use_standard_object_menus = 0;
	vulture_opts.messagelines = 5;
	vulture_opts.no_key_translation = 0;
	vulture_opts.highlight_cursor_square = 1;
	vulture_opts.debug = 0;

	strcpy(vulture_opts.macro[0], "n100.");
	strcpy(vulture_opts.macro[1], "n20s");
	for (i = 2; i < 6; i++)
		vulture_opts.macro[i][0] = '\0';

	/* Read interface options */
	
	/* main config file */
	filename = vulture_make_filename(V_CONFIG_DIRECTORY, "", V_FILENAME_OPTIONS);
	fp = fopen(filename.c_str(), "rb");

	if (fp) {
		vulture_read_main_config(fp);
		fclose(fp);
	}
	else
		printf("Could nor open %s: %s\n", filename.c_str(), strerror(errno));

	/* user's config file */
	filename = userdir + V_FILENAME_OPTIONS;
	fp = fopen(filename.c_str(), "rb");

	if (fp) {
		vulture_read_main_config(fp);
		fclose(fp);
	}

	if (vulture_opts.wall_opacity > 1.0 || vulture_opts.wall_opacity < 0) {
		fprintf(stderr, "WARNING: detected an invalid value for wall_opacity. Set 1.0 instead.\n");
		vulture_opts.wall_opacity = 0.8;
	}

	/* minimum window size: the map + inventory windows need at least this much space */
	if (vulture_opts.width < 640)
		vulture_opts.width = 640;
	if (vulture_opts.height < 480)
		vulture_opts.height = 480;


	/* Read event sounds options */
	vulture_event_sounds.clear();

	filename = vulture_make_filename(V_CONFIG_DIRECTORY, "", V_FILENAME_SOUNDS_CONFIG);
	fp = fopen(filename.c_str(), "rb");

	if (fp) {
		vulture_read_sound_config(fp);
		fclose(fp);
	}


	/* user's sound config file */
	filename = userdir + V_FILENAME_SOUNDS_CONFIG;
	fp = fopen(filename.c_str(), "rb");

	if (fp) {
		vulture_read_main_config(fp);
		fclose(fp);
	}
}


void vulture_write_userconfig(void)
{
  std::string filename, dir;
	struct stat statbuf;
	mode_t oldmask;
	int i;
	FILE *fp;

	dir = vulture_get_userdir();

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
		fprintf(fp, "screen_xsize=%d\n", vulture_opts.width);
		fprintf(fp, "screen_ysize=%d\n", vulture_opts.height);
		fprintf(fp, "fullscreen=%d\n",   vulture_opts.fullscreen);
		fprintf(fp, "play_music=%d\n",   vulture_opts.play_music);
		fprintf(fp, "play_effects=%d\n", vulture_opts.play_effects);
		fprintf(fp, "wall_style=%s\n",   vulture_opts.wall_style == V_WALL_DISPLAY_STYLE_FULL ?
										"full" : "half_height");
		fprintf(fp, "wall_opacity=%1.1f\n", vulture_opts.wall_opacity);
		fprintf(fp, "recenter_after_movement=%d\n", vulture_opts.recenter);
		fprintf(fp, "show_helpbar=%d\n", vulture_opts.show_helptb);
		fprintf(fp, "show_actionbar=%d\n", vulture_opts.show_actiontb);
		fprintf(fp, "show_minimap=%d\n", vulture_opts.show_minimap);
		fprintf(fp, "use_standard_inventory=%d\n", vulture_opts.use_standard_inventory);
		fprintf(fp, "use_standard_objmenus=%d\n", vulture_opts.use_standard_object_menus);
		fprintf(fp, "messagelines=%d\n", vulture_opts.messagelines);
		fprintf(fp, "no_key_translation=%d\n", vulture_opts.no_key_translation);
		fprintf(fp, "highlight_cursor_square=%d\n", vulture_opts.highlight_cursor_square);
		fprintf(fp, "debug=%d\n", vulture_opts.debug);

		for (i = 0; i < 6; i++)
			if (vulture_opts.macro[i][0] != '\0')
				fprintf(fp, "macro%d=%s\n", i+1, vulture_opts.macro[i]);

		fprintf(fp, "\n");
		fclose(fp);
	}
	else
		printf("Could not open %s for writing: %s\n", filename.c_str(), strerror(errno));
}


/* display the interface options dialog */
int vulture_iface_opts(void)
{
	window * win;
	int winid, num_selected, num_selected_sub, i;
	int size_changed = 0;
	anything any;
	menu_item *selected, *selected_sub;
	char * str;

	/* create a menu window */
	winid = vulture_create_nhwindow(NHW_MENU);
	vulture_start_menu(winid);
	
	str = new char[256];

	any.a_int = V_IOMID_RECENTER;
	sprintf(str, "Recenter after movement\t[%s]", vulture_opts.recenter ? "yes" : "no");
	vulture_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_PLAY_MUSIC;
	sprintf(str, "Play music\t[%s]", vulture_opts.play_music ? "yes" : "no");
	vulture_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_PLAY_EFFECTS;
	sprintf(str, "Play effects\t[%s]", vulture_opts.play_effects ? "yes" : "no");
	vulture_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_WALL_STYLE;
	sprintf(str, "Wall style\t[%s]", (vulture_opts.wall_style == V_WALL_DISPLAY_STYLE_FULL) ?
				"full" : "half-height");
	vulture_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_WALL_OPACITY;
	sprintf(str, "Wall opacity\t[%01.1f]", vulture_opts.wall_opacity);
	vulture_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_WIDTH;
	sprintf(str, "Width\t[%d]", vulture_opts.width);
	vulture_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_HEIGHT;
	sprintf(str, "Height\t[%d]", vulture_opts.height);
	vulture_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_FULLSCREEN;
	sprintf(str, "Fullscreen mode\t[%s]", vulture_opts.fullscreen ? "yes" : "no");
	vulture_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_HELPTB;
	sprintf(str, "Show helpbar\t[%s]", vulture_opts.show_helptb ? "yes" : "no");
	vulture_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_ACTIONTB;
	sprintf(str, "Show actionbar\t[%s]", vulture_opts.show_actiontb ? "yes" : "no");
	vulture_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_MINIMAP;
	sprintf(str, "Show minimap\t[%s]", vulture_opts.show_minimap ? "yes" : "no");
	vulture_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_USESTANDARDINVENTORY;
	sprintf(str, "Use plain inventory\t[%s]", vulture_opts.use_standard_inventory ? "yes" : "no");
	vulture_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_USESTANDARDOBJMENUS;
	sprintf(str, "Use plain object menus\t[%s]", vulture_opts.use_standard_object_menus ? "yes" : "no");
	vulture_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_MESSAGELINES;
	sprintf(str, "Message lines\t[%d]", vulture_opts.messagelines);
	vulture_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_KEYROTATION;
	sprintf(str, "Disable key rotation\t[%s]", vulture_opts.no_key_translation ? "yes" : "no");
	vulture_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_HIGHLIGHT_CURSOR_SQUARE;
	sprintf(str, "Highlight under the cursor\t[%s]", vulture_opts.highlight_cursor_square ? "yes" : "no");
	vulture_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	any.a_int = V_IOMID_DEBUG;
	sprintf(str, "Run-time debugging\t[%s]", vulture_opts.debug ? "yes" : "no");
	vulture_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);

	for (i = 0; i < 6; i++)
	{
		any.a_int = V_IOMID_MACROS + i;
		sprintf(str, "F%d macro\t[%s]", i+1, vulture_opts.macro[i]);
		vulture_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, str, MENU_UNSELECTED);
	}

	/* display the menu and get the user's selection */
	vulture_end_menu(winid, "Interface options");
	num_selected = vulture_select_menu(winid, PICK_ANY, &selected);
	vulture_destroy_nhwindow(winid);

	/* toggle the selected options or get new values from the user */
	for (i = 0; i < num_selected; i++)
	{
		switch(selected[i].item.a_int)
		{
			case V_IOMID_RECENTER:
				vulture_opts.recenter = !vulture_opts.recenter;
				break;

			case V_IOMID_PLAY_MUSIC:
				vulture_opts.play_music = !vulture_opts.play_music;
				if (vulture_opts.play_music)
					vulture_init_sound();
				else
					vulture_stop_music();
				break;

			case V_IOMID_PLAY_EFFECTS:
				vulture_opts.play_effects = !vulture_opts.play_effects;
				if (vulture_opts.play_effects)
					vulture_init_sound();
				break;

			case V_IOMID_WALL_STYLE:
				winid = vulture_create_nhwindow(NHW_MENU);
				vulture_start_menu(winid);

				any.a_int = V_WALL_DISPLAY_STYLE_FULL+1;
				vulture_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, "full", MENU_UNSELECTED);

				any.a_int = V_WALL_DISPLAY_STYLE_HALF_HEIGHT+1;
				vulture_add_menu(winid, NO_GLYPH, &any, 0, 0, ATR_BOLD, "half-height", MENU_UNSELECTED);

				vulture_end_menu(winid, "Wall style");
				num_selected_sub = vulture_select_menu(winid, PICK_ONE, &selected_sub);
				vulture_destroy_nhwindow(winid);

				if (num_selected_sub < 1)
					break;

				vulture_opts.wall_style = selected_sub[0].item.a_int-1;
				levwin->set_wall_style(vulture_opts.wall_style);
				levwin->force_redraw();
				vulture_display_nhwindow(WIN_MAP, 0);
				break;

			case V_IOMID_WALL_OPACITY:
				if (vulture_get_input(-1, -1, "New wall opacity", str))
				{
					vulture_opts.wall_opacity = atof(str);
					if (vulture_opts.wall_opacity > 1.0 || vulture_opts.wall_opacity < 0)
						vulture_opts.wall_opacity = 1.0;
				}
				/* flush tile arrays to ensure the new transparency gets used */
				vulture_tilecache_discard();
				/* force redraw */
				levwin->force_redraw();
				vulture_display_nhwindow(WIN_MAP, 0);
				break;

			case V_IOMID_WIDTH:
				if (vulture_get_input(-1, -1, "New width", str))
				{
					vulture_opts.width = atoi(str);
					if (vulture_opts.width < 640)
						vulture_opts.width = 640;
				}
				size_changed = 1;
				break;

			case V_IOMID_HEIGHT:
				if (vulture_get_input(-1, -1, "New height", str))
				{
					vulture_opts.height = atoi(str);
					if (vulture_opts.height < 510)
						vulture_opts.height = 510;
				}
				size_changed = 1;
				break;

			case V_IOMID_FULLSCREEN:
				vulture_opts.fullscreen = !vulture_opts.fullscreen;
				vulture_set_screensize();
				break;

			case V_IOMID_HELPTB:
				vulture_opts.show_helptb = !vulture_opts.show_helptb;
				win = ROOTWIN; /* map window */
				static_cast<levelwin*>(win)->toggle_uiwin(V_WIN_TOOLBAR2, vulture_opts.show_helptb);
				vulture_display_nhwindow(WIN_MAP, 0);
				break;

			case V_IOMID_ACTIONTB:
				vulture_opts.show_actiontb = !vulture_opts.show_actiontb;
				win = ROOTWIN; /* map window */
				static_cast<levelwin*>(win)->toggle_uiwin(V_WIN_TOOLBAR1, vulture_opts.show_actiontb);
				vulture_display_nhwindow(WIN_MAP, 0);
				break;

			case V_IOMID_MINIMAP:
				vulture_opts.show_minimap = !vulture_opts.show_minimap;
				win = ROOTWIN; /* map window */
				static_cast<levelwin*>(win)->toggle_uiwin(V_WIN_MINIMAP, vulture_opts.show_minimap);
				vulture_display_nhwindow(WIN_MAP, 0);
				break;

			case V_IOMID_USESTANDARDINVENTORY:
				vulture_opts.use_standard_inventory = ! vulture_opts.use_standard_inventory;
				break;

			case V_IOMID_USESTANDARDOBJMENUS:
				vulture_opts.use_standard_object_menus = ! vulture_opts.use_standard_object_menus;
				break;

			case V_IOMID_MESSAGELINES:
				if (vulture_get_input(-1, -1, "Number of lines in the message area", str))
				{
					vulture_opts.messagelines = atoi(str);
					if (vulture_opts.messagelines < 1 || vulture_opts.messagelines > 10)
						vulture_opts.messagelines = 5;
				}
				break;

			case V_IOMID_KEYROTATION:
				vulture_opts.no_key_translation = !vulture_opts.no_key_translation;
				break;

			case V_IOMID_HIGHLIGHT_CURSOR_SQUARE:
				vulture_opts.highlight_cursor_square = !vulture_opts.highlight_cursor_square;
				break;

			case V_IOMID_DEBUG:
				vulture_opts.debug = !vulture_opts.debug;
				break;

			default:
				if (selected[i].item.a_int >= V_IOMID_MACROS && selected[i].item.a_int <= (V_IOMID_MACROS+6))
				{
					if (vulture_get_input(-1, -1, "New macro", str))
					{
						strncpy(vulture_opts.macro[selected[i].item.a_int - V_IOMID_MACROS],str,9);
						vulture_opts.macro[selected[i].item.a_int - V_IOMID_MACROS][9] = '\0';
					}
				}
		}
	}

	/* if the screen size was changed, the actual resize happens here, rather
	* separately for width and height with 2 modechanges */
	if (size_changed)
		vulture_set_screensize();

	delete str;

	vulture_write_userconfig();

	return 0;
}


