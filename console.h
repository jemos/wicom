/*
	This file is part of wicom.

	wicom is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	wicom is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with wicom.  If not, see <http://www.gnu.org/licenses/>.

	Copyright (C) 2009 Jean Mousinho <jean.mousinho@ist.utl.pt>
	Centro de Informatica do IST - Universidade Tecnica de Lisboa 
*/

/*
	module description

	we'll only have one console at the time, so its not usefull to have it in
	class type. it won't be replicated, there's only one global console object.

*/

#ifndef _CONSOLE_H
#define _CONSOLE_H


#define CONSOLE_MAX_LINE_CHARS 512

/* hook routine action list */
typedef enum _con_action_t {
	con_process_line = 1
} con_action_t;

/* hook routine declaration */
typedef wstatus (*CONHOOKROUTINE)(con_action_t action,const char *line,void *param);

/* console structure to save some variables */
typedef struct _con_t {
	bool enabled;
	jmlist line_list;
	char cur_line[CONSOLE_MAX_LINE_CHARS];
	char tmp_line[CONSOLE_MAX_LINE_CHARS];
	char stream_line[CONSOLE_MAX_LINE_CHARS];
	unsigned int caret_pos;
	double console_height;
	unsigned int max_line_count;
	jmlist cmd_list;
	jmlist cmd_history;
	unsigned int cmd_history_index;
} con_t;


wstatus con_initialize();
wstatus con_unintialize();
wstatus con_process_keyup_h(int key);
wstatus con_insert_line(const char *line);
wstatus con_draw_lines(void);
wstatus con_draw_carret(void);
wstatus con_hide(void);
wstatus con_show(void);
wstatus con_toggle(void);
wstatus con_draw(void);
wstatus con_attach_hook(CONHOOKROUTINE routine,void *param);
wstatus con_dettach_hook(CONHOOKROUTINE routine);
wstatus con_printf(const char *fmt,...);
wstatus con_puts(const char *text);

#endif

