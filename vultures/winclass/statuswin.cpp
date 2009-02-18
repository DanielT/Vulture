/* NetHack may be freely redistributed.  See license for details. */

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

#include <sstream>

statuswin *stwin;
static const int status_xpos[5] = { 0, 60, 100, 180, 250};
static Uint32 warn_colors[V_MAX_WARN];

statuswin::statuswin(window *p) : window(p)
{
	window *subwin;
	
	statusbg = vultures_load_graphic(V_FILENAME_STATUS_BAR);
	this->w = statusbg->w;
	this->h = statusbg->h;
	this->x = 6;
	this->y = vultures_screen->h - (this->h + 6);
	this->menu_id = V_WIN_STATUSBAR;

	/* The enhance symbol: usually invisible, it is shown only when skill enhancement is possible */
	subwin = new enhancebutton(this->parent);
	subwin->x = this->x + this->w;
	subwin->y = this->y - subwin->h;
	subwin->menu_id = V_WIN_ENHANCE;
	
	for (int i = 0; i < 5; i++)
		for (int j = 0; j < 5; j++)
		{
			tokenarray[i][j] = new textwin(this, "");
			tokenarray[i][j]->x = 3 + status_xpos[i];
			tokenarray[i][j]->y = 2 + j*vultures_get_lineheight(V_FONT_STATUS);
			tokenarray[i][j]->w = 100;
			tokenarray[i][j]->h = vultures_get_lineheight(V_FONT_STATUS);
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


eventresult statuswin::handle_mousemotion_event(window* target, void* result, int xrel, 
                                             int yrel, int state)
{
	vultures_set_mcursor(V_CURSOR_NORMAL);
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult statuswin::handle_mousebuttonup_event(window* target, void* result,
                                            int mouse_x, int mouse_y, int button, int state)
{
	return V_EVENT_HANDLED_NOREDRAW;
}


eventresult statuswin::handle_resize_event(window* target, void* result, int res_w, int res_h)
{
	/* x coordinate does not change */
	y = parent->h - (h + 6);
	return V_EVENT_HANDLED_NOREDRAW;
}


void statuswin::parse_statusline(std::string str)
{
	int hp, hpmax, nconds;
	long val;
  std::string txt;
	int cap = near_capacity();
	size_t pos;
	char buf[64];

	/* get player name + title */
	pos = str.find("St:", 0);
	if (pos != std::string::npos)
		tokenarray[0][0]->caption = str.substr(0, pos);

	/* strength needs special treatment */
	if (ACURR(A_STR) > 18) 
	{
		if (ACURR(A_STR) > STR18(100))
			sprintf(buf,"St:%2d", ACURR(A_STR)-100);
		else if (ACURR(A_STR) < STR18(100))
			sprintf(buf, "St:18/%02d", ACURR(A_STR)-18);
		else
			sprintf(buf,"St:18/**");
	}
	else
		sprintf(buf, "St:%-1d", ACURR(A_STR));
	tokenarray[0][1]->caption = buf;

	/* the other stats */
	sprintf(buf, "Dx:%-1d", ACURR(A_DEX));
	tokenarray[0][2]->caption = buf;
	
	sprintf(buf, "Co:%-1d", ACURR(A_CON));
	tokenarray[0][3]->caption = buf;
	
	sprintf(buf, "In:%-1d", ACURR(A_INT));
	tokenarray[1][1]->caption = buf;
	
	sprintf(buf, "Wi:%-1d", ACURR(A_WIS));
	tokenarray[1][2]->caption = buf;
	
	sprintf(buf, "Ch:%-1d", ACURR(A_CHA));
	tokenarray[1][3]->caption = buf;

	/* alignment */
	tokenarray[4][0]->visible = 1;
	tokenarray[4][0]->caption = (u.ualign.type == A_CHAOTIC) ? "Chaotic" :
			(u.ualign.type == A_NEUTRAL) ? "Neutral" : "Lawful";

	/* score */
#ifdef SCORE_ON_BOTL
	if (flags.showscore) {
		sprintf(buf, "S:%ld", botl_score());
		tokenarray[3][4]->caption = buf;
	} else
		tokenarray[3][4]->caption.clear();
#endif

	/* money */
#ifndef GOLDOBJ
		val = u.ugold;
#else
		val = money_cnt(invent);
#endif
	if (val >= 100000) {
		sprintf(buf, "%c:%-2ldk", oc_syms[COIN_CLASS], val / 1000);
		tokenarray[3][2]->caption = buf;
	} else {
		sprintf(buf, "%c:%-2ld", oc_syms[COIN_CLASS], val);
		tokenarray[3][2]->caption = buf;
	}

	/* Experience */
	if (Upolyd) {
		sprintf(buf, "HD:%d", mons[u.umonnum].mlevel);
		tokenarray[3][0]->caption = buf;
	}
#ifdef EXP_ON_BOTL
	else if(flags.showexp)
	{
		Sprintf(buf, "Xp:%u/%-1ld", u.ulevel,u.uexp);
		tokenarray[3][0]->caption = buf;
		/* if the exp gets too long, suppress displaying the alignment */
		if (tokenarray[3][0]->caption.length() > 10)
			tokenarray[4][0]->visible = 0;
	}
#endif
	else {
		Sprintf(buf, "Exp:%u", u.ulevel);
		tokenarray[3][0]->caption = buf;
	}

	/* HP, energy, armor */
	hp = Upolyd ? u.mh : u.uhp;
	hpmax = Upolyd ? u.mhmax : u.uhpmax;
	sprintf(buf, "HP:%d(%d)", hp, hpmax);
	tokenarray[2][1]->caption = buf;
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
	sprintf(buf, "Pw:%d(%d)", u.uen, u.uenmax);
	tokenarray[2][2]->caption = buf;
	sprintf(buf, "AC:%-2d", u.uac);
	tokenarray[2][3]->caption = buf;

	/* time */
	if(flags.time) {
		sprintf(buf, "T:%ld", moves);
		tokenarray[3][3]->caption = buf;
	} else
		tokenarray[3][3]->caption.clear();

	/* depth again (numeric) */
	sprintf(buf, "Dlvl:%-2d ", depth(&u.uz));
	tokenarray[3][1]->caption = buf;

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
  {
    std::stringstream stream;
    stream << "Wt:" << static_cast<long>((inv_weight()+weight_cap())) << "/" << static_cast<long>(weight_cap()) ;
    tokenarray[0][4]->caption = stream.str();
  }
#endif

}


void statuswin::add_cond(std::string str, int warnno, int color)
{
	static const point pos[9] = {{4,1}, {4,2}, {4,3}, {4,4}, {3,4}, {2,4}, {1,4}, {0,4}, {4,0}};
	if (warnno >= 9)
		return;

	tokenarray[pos[warnno].x][pos[warnno].y]->caption = str;
	tokenarray[pos[warnno].x][pos[warnno].y]->textcolor = warn_colors[color];
}
