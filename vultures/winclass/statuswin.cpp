
extern "C" {
	#include "hack.h"
	
extern const char *hu_stat[];           /* defined in eat.c */
extern const char *const enc_stat[];   /* defined in botl.c */
}

#include "vultures_win.h"
#include "vultures_gra.h"
#include "vultures_sdl.h"
#include "vultures_gfl.h"
#include "vultures_txt.h"
#include "vultures_mou.h"
#include "vultures_tile.h"

#include "statuswin.h"
#include "enhancebutton.h"
#include "textwin.h"

statuswin *stwin;
static const int status_xpos[5] = { 0, 60, 100, 180, 250};
static Uint32 warn_colors[V_MAX_WARN];

statuswin::statuswin(window *p) : window(p)
{
	window *subwin;
	nh_type = NHW_STATUS;
	
	statusbg = vultures_load_graphic(NULL, V_FILENAME_STATUS_BAR);
	this->w = statusbg->w;
	this->h = statusbg->h;
	this->x = 6;
	this->y = vultures_screen->h - (this->h + 6);
	this->menu_id = V_WIN_STATUSBAR;

	/* The enhance symbol: usually invisible, it is shown only when skill enhancement is possible */
	subwin = new enhancebutton(this->parent);
	subwin->x = this->x + this->w;
	subwin->y = this->y - subwin->h;
	subwin->visible = 0;
	subwin->autobg = 1;
	subwin->menu_id = V_WIN_ENHANCE;
	
	for (int i = 0; i < 5; i++)
		for (int j = 0; j < 5; j++)
		{
			tokenarray[i][j] = new textwin(this, "");
			tokenarray[i][j]->autobg = 1;
			tokenarray[i][j]->x = 3 + status_xpos[i];
			tokenarray[i][j]->y = 2 + j*vultures_get_lineheight(V_FONT_STATUS);
			tokenarray[i][j]->w = 100;
			tokenarray[i][j]->h = vultures_get_lineheight(V_FONT_STATUS);
			tokenarray[i][j]->caption = (char *)malloc(64);
			tokenarray[i][j]->caption[0] = '\0';
		}

	/* the player is longer than everything else */
	tokenarray[0][0]->w = 250;
	
	stwin = this;

	/* Set warning colors */
	warn_colors[V_WARN_NONE] = V_COLOR_TEXT;
	warn_colors[V_WARN_NORMAL] = CLR32_GREEN;
	warn_colors[V_WARN_MORE] = CLR32_YELLOW;
	warn_colors[V_WARN_ALERT] = CLR32_ORANGE;
	warn_colors[V_WARN_CRITICAL] = CLR32_RED;
}


statuswin::~statuswin()
{
	SDL_FreeSurface(statusbg);
}


bool statuswin::draw()
{
	vultures_set_draw_region(abs_x, abs_y, abs_x + w - 1, abs_y + h - 1);
	vultures_put_img(abs_x, abs_y, statusbg);
	vultures_set_draw_region(0, 0, vultures_screen->w-1, vultures_screen->h-1);

	vultures_invalidate_region(abs_x, abs_y, w, h);
	
	return true;
}


eventresult statuswin::event_handler(window* target, void* result, SDL_Event* event)
{
	if (event->type == SDL_MOUSEMOTION)
		vultures_set_mcursor(V_CURSOR_NORMAL);
	else if (event->type == SDL_VIDEORESIZE)
		/* x coordinate does not change */
		y = parent->h - (h + 6);

	return V_EVENT_HANDLED_NOREDRAW;
}


void statuswin::parse_statusline(const char *str)
{
	int len, hp, hpmax, nconds;
	long val;
	char * ptr;
	int cap = near_capacity();


	/* get player name + title */
	ptr = strstr(str, "St:");
	if (ptr)
	{
		len = ptr - str;
		strncpy(tokenarray[0][0]->caption, str, len);
		tokenarray[0][0]->caption[len] = '\0';
	}

	/* strength needs special treatment */
	ptr = tokenarray[0][1]->caption;
	if (ACURR(A_STR) > 18) 
	{
		if (ACURR(A_STR) > STR18(100))
			sprintf(ptr,"St:%2d", ACURR(A_STR)-100);
		else if (ACURR(A_STR) < STR18(100))
			sprintf(ptr, "St:18/%02d", ACURR(A_STR)-18);
		else
			sprintf(ptr,"St:18/**");
	}
	else
		sprintf(ptr, "St:%-1d", ACURR(A_STR));

	/* the other stats */
	sprintf(tokenarray[0][2]->caption, "Dx:%-1d", ACURR(A_DEX));
	sprintf(tokenarray[0][3]->caption, "Co:%-1d", ACURR(A_CON));
	sprintf(tokenarray[1][1]->caption, "In:%-1d", ACURR(A_INT));
	sprintf(tokenarray[1][2]->caption, "Wi:%-1d", ACURR(A_WIS));
	sprintf(tokenarray[1][3]->caption, "Ch:%-1d", ACURR(A_CHA));

	/* alignment */
	tokenarray[4][0]->visible = 1;
	sprintf(tokenarray[4][0]->caption, (u.ualign.type == A_CHAOTIC) ? "Chaotic" :
			(u.ualign.type == A_NEUTRAL) ? "Neutral" : "Lawful");

	/* score */
#ifdef SCORE_ON_BOTL
	if (flags.showscore)
		sprintf(tokenarray[3][4]->caption, "S:%ld", botl_score());
	else
		tokenarray[3][4]->caption[0] = '\0';
#endif

	/* money */
#ifndef GOLDOBJ
		val = u.ugold;
#else
		val = money_cnt(invent);
#endif
	if (val >= 100000)
		sprintf(tokenarray[3][2]->caption, "%c:%-2ldk", oc_syms[COIN_CLASS], val / 1000);
	else
		sprintf(tokenarray[3][2]->caption, "%c:%-2ld", oc_syms[COIN_CLASS], val);

	/* Experience */
	if (Upolyd)
		Sprintf(tokenarray[3][0]->caption, "HD:%d", mons[u.umonnum].mlevel);
#ifdef EXP_ON_BOTL
	else if(flags.showexp)
	{
		Sprintf(tokenarray[3][0]->caption, "Xp:%u/%-1ld", u.ulevel,u.uexp);
		/* if the exp gets too long, suppress displaying the alignment */
		if (strlen(tokenarray[3][0]->caption) > 10)
			tokenarray[4][0]->visible = 0;
	}
#endif
	else
		Sprintf(tokenarray[3][0]->caption, "Exp:%u", u.ulevel);


	/* HP, energy, armor */
	hp = Upolyd ? u.mh : u.uhp;
	hpmax = Upolyd ? u.mhmax : u.uhpmax;
	sprintf(tokenarray[2][1]->caption, "HP:%d(%d)", hp, hpmax);
	if (hp >= ((hpmax * 90) / 100))
		tokenarray[2][1]->textcolor = warn_colors[V_WARN_NONE];
	else if (hp >= ((hpmax * 70) / 100))
		tokenarray[2][1]->textcolor = warn_colors[V_WARN_NORMAL];
	else if (hp >= ((hpmax * 50) / 100))
		tokenarray[2][1]->textcolor = warn_colors[V_WARN_MORE];
	else if (hp >= ((hpmax * 25) / 100))
		tokenarray[2][1]->textcolor = warn_colors[V_WARN_ALERT];
	else
		tokenarray[2][1]->textcolor = warn_colors[V_WARN_CRITICAL];
	sprintf(tokenarray[2][2]->caption, "Pw:%d(%d)", u.uen, u.uenmax);
	sprintf(tokenarray[2][3]->caption, "AC:%-2d", u.uac);

	/* time */
	if(flags.time)
		sprintf(tokenarray[3][3]->caption, "T:%ld", moves);
	else
		tokenarray[3][3]->caption[0] = '\0';

	/* depth again (numeric) */
	sprintf(tokenarray[3][1]->caption, "Dlvl:%-2d ", depth(&u.uz));

	/* conditions (hunger, confusion, etc) */
	nconds = 0;

	if (u.uhs > 1) /* hunger */
		add_cond(hu_stat[u.uhs], nconds++, u.uhs-1);
	else if (u.uhs < 1) /* satiated */
		add_cond(hu_stat[u.uhs], nconds++, 0);

	if(Confusion)
		add_cond("Conf", nconds++, V_WARN_MORE);

	if(Sick)
	{
		if (u.usick_type & SICK_VOMITABLE)
			add_cond("FoodPois", nconds++, V_WARN_ALERT);
		if (u.usick_type & SICK_NONVOMITABLE)
			add_cond("Ill", nconds++, V_WARN_ALERT);
	}

	if(Blind)          add_cond("Blind", nconds++, V_WARN_MORE);
	if(Stunned)        add_cond("Stun",  nconds++, V_WARN_MORE);
	if(Hallucination)  add_cond("Hallu", nconds++, V_WARN_MORE);
	if(Slimed)         add_cond("Slime", nconds++, V_WARN_ALERT);

	if(cap > UNENCUMBERED)
		add_cond(enc_stat[cap], nconds++, cap);

	/* reset the empty positions */
	for ( ;nconds < 8; nconds++)
		add_cond("", nconds, 0);

#ifdef SHOW_WEIGHT
	if (flags.showweight && !tokenarray[0][4]->caption[0])
		sprintf(tokenarray[0][4]->caption, "Wt:%ld/%ld", (long)(inv_weight()+weight_cap()),
			(long)weight_cap());
#endif

}


void statuswin::add_cond(const char *str, int warnno, int color)
{
	static const point pos[9] = {{4,1}, {4,2}, {4,3}, {4,4}, {3,4}, {2,4}, {1,4}, {0,4}, {4,0}};
	if (warnno >= 9)
		return;

	strcpy(tokenarray[pos[warnno].x][pos[warnno].y]->caption, str);
	tokenarray[pos[warnno].x][pos[warnno].y]->textcolor = warn_colors[color];
}
