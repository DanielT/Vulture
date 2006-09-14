/*	SCCS Id: @(#)unixmain.c	3.4	1997/01/22	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

/* main.c - Unix NetHack */

#include "hack.h"
#include "dlb.h"

#include <sys/stat.h>
#include <signal.h>
#include <pwd.h>
#ifndef O_RDONLY
#include <fcntl.h>
#endif

#if !defined(_BULL_SOURCE) && !defined(__sgi) && !defined(_M_UNIX)
# if !defined(SUNOS4) && !(defined(ULTRIX) && defined(__GNUC__))
#  if defined(POSIX_TYPES) || defined(SVR4) || defined(HPUX)
extern struct passwd *FDECL(getpwuid,(uid_t));
#  else
extern struct passwd *FDECL(getpwuid,(int));
#  endif
# endif
#endif
extern struct passwd *FDECL(getpwnam,(const char *));
#ifdef CHDIR
static void FDECL(chdirx, (const char *,BOOLEAN_P));
#endif /* CHDIR */
static boolean NDECL(whoami);
static void FDECL(process_options, (int, char **));

#ifdef _M_UNIX
extern void NDECL(check_sco_console);
extern void NDECL(init_sco_cons);
#endif
#ifdef __linux__
extern void NDECL(check_linux_console);
extern void NDECL(init_linux_cons);
#endif

#ifdef LINUX
extern void NDECL(check_linux_console);
extern void NDECL(init_linux_cons);
#endif
 
static void NDECL(wd_message);
#ifdef WIZARD
static boolean wiz_error_flag = FALSE;
#endif

int
main(argc,argv)
int argc;
char *argv[];
{
	register int fd;
#ifdef CHDIR
	register char *dir;
#endif
	boolean exact_username;

	/* printf("DEBUG 1: Starting NetHack - Vulture's Eye...\n"); */
	hname = argv[0];
	hackpid = getpid();
	(void) umask(0777 & ~FCMASK);

        /* printf("DEBUG 2: Choosing window port...\n"); */
	choose_windows(DEFAULT_WINDOW_SYS);

        /* printf("DEBUG 3: Window port chosen.\n"); */
#ifdef CHDIR			/* otherwise no chdir() */
	/*
	 * See if we must change directory to the playground.
	 * (Perhaps hack runs suid and playground is inaccessible
	 *  for the player.)
	 * The environment variable HACKDIR is overridden by a
	 *  -d command line option (must be the first option given)
	 */
	/* printf("DEBUG 4: Getting env variable NETHACKDIR ...\n"); */
	dir = nh_getenv("NETHACKDIR");
	if (!dir) dir = nh_getenv("HACKDIR");
	/* printf("DEBUG 5: NETHACKDIR is [%s].\n", dir); */
#endif
	/* printf("DEBUG 6: Checking for command line arguments...\n"); */
	if(argc > 1) {
	  /* printf("DEBUG 7: Processing command line arguments...\n"); */
#ifdef CHDIR
	    if (!strncmp(argv[1], "-d", 2) && argv[1][2] != 'e') {
		/* avoid matching "-dec" for DECgraphics; since the man page
		 * says -d directory, hope nobody's using -desomething_else
		 */
		argc--;
		argv++;
		dir = argv[0]+2;
		if(*dir == '=' || *dir == ':') dir++;
		if(!*dir && argc > 1) {
			argc--;
			argv++;
			dir = argv[0];
		}
		if(!*dir)
		    error("Flag -d must be followed by a directory name.");
	    }
	    if (argc > 1)
#endif /* CHDIR */

	    /*
	     * Now we know the directory containing 'record' and
	     * may do a prscore().  Exclude `-style' - it's a Qt option.
	     */
	    if (!strncmp(argv[1], "-s", 2) && strncmp(argv[1], "-style", 6)) {
#ifdef CHDIR
		chdirx(dir,0);
#endif
		prscore(argc, argv);
		exit(EXIT_SUCCESS);
	    }
	}

	/*
	 * Change directories before we initialize the window system so
	 * we can find the tile file.
	 */
#ifdef CHDIR
	chdirx(dir,1);
#endif

#ifdef _M_UNIX
	check_sco_console();
#endif
#ifdef LINUX
	check_linux_console();
#endif
#ifdef PROXY_GRAPHICS
	/* Handle --proxy before options, if supported */
	if (argc > 1 && !strcmp(argv[1], "--proxy")) {
	    argv[1] = argv[0];
	    argc--;
	    argv++;
	    choose_windows("proxy");
	    lock_windows(TRUE);         /* Can't be overridden from options */
	}
#endif
	/* printf("DEBUG 10: Initializing options...\n"); */
	initoptions();
	/* printf("DEBUG 11: Initializing window port...\n"); */
	init_nhwindows(&argc,argv);
	/* printf("DEBUG 12: Window port initialized.\n"); */
	/* printf("DEBUG 13: Getting exact username...\n"); */
	exact_username = whoami();
        /* printf("DEBUG 14: Username ready.\n"); */
#ifdef _M_UNIX
	init_sco_cons();
#endif
#ifdef LINUX
	init_linux_cons();
#endif
	/*
	 * It seems you really want to play.
	 */
	u.uhp = 1;	/* prevent RIP on early quits */
	(void) signal(SIGHUP, (SIG_RET_TYPE) hangup);
#ifdef SIGXCPU
	(void) signal(SIGXCPU, (SIG_RET_TYPE) hangup);
#endif
#ifdef SIGPIPE		/* eg., a lost proxy connection */
	(void) signal(SIGPIPE, (SIG_RET_TYPE) hangup);
#endif

        /* printf("DEBUG 15: Processing command line options...\n"); */
	process_options(argc, argv);	/* command line options */
        /* printf("DEBUG 15b: Command line options processed.\n"); */

#ifdef DEF_PAGER
	if(!(catmore = nh_getenv("HACKPAGER")) && !(catmore = nh_getenv("PAGER")))
		catmore = DEF_PAGER;
#endif
#ifdef MAIL
	getmailstatus();
#endif
#ifdef WIZARD
	if (wizard)
		Strcpy(plname, "wizard");
	else
#endif
	  /* printf("DEBUG 15c: Comparing player name...\n"); */
#ifdef JTP_GRAPHICS
    if (1) {
        askname();
    } else    
#endif        
	if(!*plname || !strncmp(plname, "player", 4)
		    || !strncmp(plname, "games", 4)) {
	  /* printf("DEBUG 15c2: Starting Askname...\n"); */
		askname();
	} else if (exact_username) {
		/* guard against user names with hyphens in them */
		int len = strlen(plname);
		/* printf("DEBUG 15c3: Formatting username...\n"); */
		/* append the current role, if any, so that last dash is ours */
		if (++len < sizeof plname)
			(void)strncat(strcat(plname, "-"),
				      pl_character, sizeof plname - len - 1);
	}
        /* printf("DEBUG 15c4: Removing suffix...\n"); */
	plnamesuffix();		/* strip suffix from name; calls askname() */
				/* again if suffix was whole name */
				/* accepts any suffix */
	/* printf("DEBUG 15c5: Name ready.\n"); */
#ifdef WIZARD
	if(!wizard) {
#endif
	  /* printf("DEBUG 15c6: Getting locks...\n"); */
		/*
		 * check for multiple games under the same name
		 * (if !locknum) or check max nr of players (otherwise)
		 */
		(void) signal(SIGQUIT,SIG_IGN);
		(void) signal(SIGINT,SIG_IGN);
		/* printf("DEBUG 15c8: Signals ready...\n"); */
		if(!locknum)
			Sprintf(lock, "%d%s", (int)getuid(), plname);
                /* printf("DEBUG 15c9: Going to getlock...\n"); */
		getlock();
		/* printf("DEBUG 15c7: Locks ready.\n"); */
#ifdef WIZARD
	} else {
		Sprintf(lock, "%d%s", (int)getuid(), plname);
		getlock();
	}
#endif /* WIZARD */

        /* printf("DEBUG 15c: Initializing DLB...\n"); */
	dlb_init();	/* must be before newgame() */
        /* printf("DEBUG 15d: DLB Initialized...\n"); */

	/*
	 * Initialization of the boundaries of the mazes
	 * Both boundaries have to be even.
	 */
	x_maze_max = COLNO-1;
	if (x_maze_max % 2)
		x_maze_max--;
	y_maze_max = ROWNO-1;
	if (y_maze_max % 2)
		y_maze_max--;

        /* printf("DEBUG 15e: Initializing vision...\n"); */
	/*
	 *  Initialize the vision system.  This must be before mklev() on a
	 *  new game or before a level restore on a saved game.
	 */
	vision_init();

        /* printf("DEBUG 16: Displaying game windows...\n"); */
	display_gamewindows();

	if ((fd = restore_saved_game()) >= 0) {
#ifdef WIZARD
		/* Since wizard is actually flags.debug, restoring might
		 * overwrite it.
		 */
		boolean remember_wiz_mode = wizard;
#endif
#ifndef FILE_AREAS
		const char *fq_save = fqname(SAVEF, SAVEPREFIX, 1);

		(void) chmod(fq_save,0);	/* disallow parallel restores */
#else
		(void) chmod_area(FILE_AREA_SAVE, SAVEF, 0);
#endif
		(void) signal(SIGINT, (SIG_RET_TYPE) done1);
#ifdef NEWS
		if(iflags.news) {
		    display_file_area(NEWS_AREA, NEWS, FALSE);
		    iflags.news = FALSE; /* in case dorecover() fails */
		}
#endif
		pline("Restoring save file...");
		mark_synch();	/* flush output */
		if(!dorecover(fd))
			goto not_recovered;
#ifdef WIZARD
		if(!wizard && remember_wiz_mode) wizard = TRUE;
#endif
		check_special_room(FALSE);
		wd_message();

		if (discover || wizard) {
			if(yn("Do you want to keep the save file?") == 'n')
			    (void) delete_savefile();
			else {
#ifndef FILE_AREAS
			    (void) chmod(fq_save,FCMASK); /* back to readable */
			    compress_area(NULL, fq_save);
#else
			    (void) chmod_area(FILE_AREA_SAVE, SAVEF, FCMASK);
			    compress_area(FILE_AREA_SAVE, SAVEF);
#endif
			}
		}
		flags.move = 0;
	} else {
not_recovered:
	  /* printf("Debug 17: Player selection...\n"); */
		player_selection();
		/* printf("Debug 18: Starting New Game...\n"); */
		newgame();
		wd_message();

		flags.move = 0;
		set_wear();
		(void) pickup(1);
	}

        /* printf("Debug 19: Starting movement loop...\n"); */
	moveloop();
	exit(EXIT_SUCCESS);
	/*NOTREACHED*/
	return(0);
}

static void
process_options(argc, argv)
int argc;
char *argv[];
{
	int i;


	/*
	 * Process options.
	 */
	while(argc > 1 && argv[1][0] == '-'){
		argv++;
		argc--;
		switch(argv[0][1]){
		case 'D':
		case 'Z':
#ifdef WIZARD
			{
			  char *user;
			  int uid;
			  struct passwd *pw = (struct passwd *)0;

			  uid = getuid();
			  user = getlogin();
			  if (user) {
			      pw = getpwnam(user);
			      if (pw && (pw->pw_uid != uid)) pw = 0;
			  }
			  if (pw == 0) {
			      user = nh_getenv("USER");
			      if (user) {
				  pw = getpwnam(user);
				  if (pw && (pw->pw_uid != uid)) pw = 0;
			      }
			      if (pw == 0) {
				  pw = getpwuid(uid);
			      }
			  }
			  if (pw && !strcmp(pw->pw_name,WIZARD)) {
			      wizard = TRUE;
			      break;
			  }
			}
			/* otherwise fall thru to discover */
			wiz_error_flag = TRUE;
#endif
		case 'X':
			discover = TRUE;
			break;
#ifdef NEWS
		case 'n':
			iflags.news = FALSE;
			break;
#endif
		case 'u':
			if(argv[0][2])
			  (void) strncpy(plname, argv[0]+2, sizeof(plname)-1);
			else if(argc > 1) {
			  argc--;
			  argv++;
			  (void) strncpy(plname, argv[0], sizeof(plname)-1);
			} else
				raw_print("Player name expected after -u");
			break;
		case 'i':
			if (!strncmpi(argv[0]+1, "IBM", 3))
				switch_graphics(IBM_GRAPHICS);
			break;
		case 'd':
			if (!strncmpi(argv[0]+1, "DEC", 3))
				switch_graphics(DEC_GRAPHICS);
			break;
		case 'p': /* profession (role) */
			if (argv[0][2]) {
			    if ((i = str2role(&argv[0][2])) >= 0)
			    	flags.initrole = i;
			} else if (argc > 1) {
				argc--;
				argv++;
			    if ((i = str2role(argv[0])) >= 0)
			    	flags.initrole = i;
			}
			break;
		case 'r': /* race */
			if (argv[0][2]) {
			    if ((i = str2race(&argv[0][2])) >= 0)
			    	flags.initrace = i;
			} else if (argc > 1) {
				argc--;
				argv++;
			    if ((i = str2race(argv[0])) >= 0)
			    	flags.initrace = i;
			}
			break;
		case 'g': /* gender */
			if (argv[0][2]) {
			    if ((i = str2gend(&argv[0][2])) >= 0)
			    	flags.initgend = i;
			} else if (argc > 1) {
				argc--;
				argv++;
			    if ((i = str2gend(argv[0])) >= 0)
			    	flags.initgend = i;
			}
			break;
		case 'a': /* align */
			if (argv[0][2]) {
			    if ((i = str2align(&argv[0][2])) >= 0)
			    	flags.initalign = i;
			} else if (argc > 1) {
				argc--;
				argv++;
			    if ((i = str2align(argv[0])) >= 0)
			    	flags.initalign = i;
			}
			break;
		case '@':
			flags.randomall = 1;
			break;
		default:
			if ((i = str2role(&argv[0][1])) >= 0) {
			    flags.initrole = i;
				break;
			}
			/* else raw_printf("Unknown option: %s", *argv); */
		}
	}

	if(argc > 1)
		locknum = atoi(argv[1]);
#ifdef MAX_NR_OF_PLAYERS
	if(!locknum || locknum > MAX_NR_OF_PLAYERS)
		locknum = MAX_NR_OF_PLAYERS;
#endif
}

#ifdef CHDIR
static void
chdirx(dir, wr)
const char *dir;
boolean wr;
{
	if (dir					/* User specified directory? */
# ifdef HACKDIR
	       && strcmp(dir, HACKDIR)		/* and not the default? */
# endif
		) {
# ifdef SECURE
	  /* printf("DEBUG[unixmain.c/40]: Setting permissions...\n"); */
	    (void) setgid(getgid());
	    (void) setuid(getuid());		/* Ron Wessels */
# endif
	} else {
	    /* non-default data files is a sign that scores may not be
	     * compatible, or perhaps that a binary not fitting this
	     * system's layout is being used.
	     */
# ifdef VAR_PLAYGROUND
	    int len = strlen(VAR_PLAYGROUND);

	    fqn_prefix[SCOREPREFIX] = (char *)alloc(len+2);
	    Strcpy(fqn_prefix[SCOREPREFIX], VAR_PLAYGROUND);
	    if (fqn_prefix[SCOREPREFIX][len-1] != '/') {
		fqn_prefix[SCOREPREFIX][len] = '/';
		fqn_prefix[SCOREPREFIX][len+1] = '\0';
	    }
# endif
	}

# ifdef HACKDIR
	if (dir == (const char *)0)
	    dir = HACKDIR;
# endif

	if (dir && chdir(dir) < 0) {
	    perror(dir);
	    error("Cannot chdir to %s.", dir);
	}

	/* warn the player if we can't write the record file */
	/* perhaps we should also test whether . is writable */
	/* unfortunately the access system-call is worthless */
	if (wr) {
# ifdef VAR_PLAYGROUND
	    fqn_prefix[LEVELPREFIX] = fqn_prefix[SCOREPREFIX];
	    fqn_prefix[SAVEPREFIX] = fqn_prefix[SCOREPREFIX];
	    fqn_prefix[BONESPREFIX] = fqn_prefix[SCOREPREFIX];
	    fqn_prefix[LOCKPREFIX] = fqn_prefix[SCOREPREFIX];
	    fqn_prefix[TROUBLEPREFIX] = fqn_prefix[SCOREPREFIX];
# endif
	    check_recordfile(dir);
	}
}
#endif /* CHDIR */

static boolean
whoami() {
	/*
	 * Who am i? Algorithm: 1. Use name as specified in NETHACKOPTIONS
	 *			2. Use $USER or $LOGNAME	(if 1. fails)
	 *			3. Use getlogin()		(if 2. fails)
	 * The resulting name is overridden by command line options.
	 * If everything fails, or if the resulting name is some generic
	 * account like "games", "play", "player", "hack" then eventually
	 * we'll ask him.
	 * Note that we trust the user here; it is possible to play under
	 * somebody else's name.
	 */
	register char *s;

	if (*plname) return FALSE;
	if(/* !*plname && */ (s = nh_getenv("USER")))
		(void) strncpy(plname, s, sizeof(plname)-1);
	if(!*plname && (s = nh_getenv("LOGNAME")))
		(void) strncpy(plname, s, sizeof(plname)-1);
	if(!*plname && (s = getlogin()))
		(void) strncpy(plname, s, sizeof(plname)-1);
	return TRUE;
}

#ifdef PORT_HELP
void
port_help()
{
	/*
	 * Display unix-specific help.   Just show contents of the helpfile
	 * named by PORT_HELP.
	 */
	display_file_area(FILE_AREA_SHARE, PORT_HELP, TRUE);
}
#endif

static void
wd_message()
{
#ifdef WIZARD
	if (wiz_error_flag) {
		pline("Only user \"%s\" may access debug (wizard) mode.",
# ifndef KR1ED
			WIZARD);
# else
			WIZARD_NAME);
# endif
		pline("Entering discovery mode instead.");
	} else
#endif
	if (discover)
		You("are in non-scoring discovery mode.");
}

/*
 * Add a slash to any name not ending in /. There must
 * be room for the /
 */
void
append_slash(name)
char *name;
{
	char *ptr;

	if (!*name)
		return;
	ptr = name + (strlen(name) - 1);
	if (*ptr != '/') {
		*++ptr = '/';
		*++ptr = '\0';
	}
	return;
}

/*unixmain.c*/
