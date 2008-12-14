#ifndef _messagewin_h_
#define _messagewin_h_


#include "window.h"


#define V_MESSAGEBUF_SIZE 512



class messagewin : public window
{
public:
	messagewin(window *p);
	~messagewin();
	virtual bool draw();
	virtual eventresult event_handler(window* target, void* result, SDL_Event* event);
	
	void add_message(const char *msg);
	void setshown(int first);
	int getshown(void);
	char *get_message(int offset, int *age);
	void view_all(void);

private:
	SDL_Surface *bg_img;
	int message_ages[V_MESSAGEBUF_SIZE];
	char *message_buf[V_MESSAGEBUF_SIZE];
	int message_cur, message_top;
};

extern messagewin *msgwin;


#endif
