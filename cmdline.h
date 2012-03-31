#ifndef CMDLINE_H
#define CMDLINE_H

#include "ptr-array.h"
#include "gbuf.h"
#include "term.h"

struct cmdline {
	struct gbuf buf;
	unsigned int pos;
	int search_pos;
	char *search_text;
};

enum {
	CMDLINE_UNKNOWN_KEY,
	CMDLINE_KEY_HANDLED,
	CMDLINE_CANCEL,
};

#define CMDLINE(name) struct cmdline name = { GBUF_INIT, 0, -1, NULL }

void cmdline_clear(struct cmdline *c);
void cmdline_set_text(struct cmdline *c, const char *text);
void cmdline_insert_bytes(struct cmdline *c, const char *buf, int size);
void cmdline_reset_history_search(struct cmdline *c);
int cmdline_handle_key(struct cmdline *c, struct ptr_array *history, enum term_key_type type, unsigned int key);

#endif
