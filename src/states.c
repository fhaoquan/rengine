#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifdef WIN32
#include <SDL2/SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include "ini.h"
#include "bmp.h"
#include "hash.h"
#include "states.h"
#include "utils.h"
#include "game.h"
#include "resources.h"
#include "log.h"

/* Globals *******************************************/

#define MAX_NESTED_STATES 20

/*
FIXME: Looking at this now, I don't know why this isn't
a linked list, like the one in resources.c
*/
static struct game_state *game_states[MAX_NESTED_STATES];
static int state_top = 0;

/* Styles ********************************************/

#define DEFAULT_FOREGROUND      "white"
#define DEFAULT_BACKGROUND      "black"

#define DEFAULT_MARGIN          "1"
#define DEFAULT_PADDING         "1"

#define DEFAULT_BORDER          "0"
#define DEFAULT_BORDER_RADIUS   "0"

#define DEFAULT_BUTTON_PADDING          "5"
#define DEFAULT_BUTTON_BORDER_RADIUS    "1"

#define DEFAULT_FONT            "normal"

#define DEFAULT_IMAGE_ALIGN     "top"
#define DEFAULT_IMAGE_MARGIN    "0"

const char *get_style(struct game_state *s, const char *name, const char *def) {
	char *n = my_strlower(strdup(name));
	const char *value = ini_get(game_ini, s->name, n, NULL);
	if(!value)
		value = ini_get(game_ini, "styles", n, def);
	free(n);
	return value;
}

static void draw_border(struct game_state *s, struct bitmap *bmp) {
	int b = atoi(get_style(s, "border", DEFAULT_BORDER));
	if(b > 0) {
		int m = atoi(get_style(s, "margin", DEFAULT_MARGIN));
		int r = atoi(get_style(s, "border-radius", DEFAULT_BORDER_RADIUS));
		const char *f = get_style(s, "foreground", DEFAULT_FOREGROUND);

		bm_set_color_s(bmp, get_style(s, "border-color", f));
		if(atoi(get_style(s, "border", DEFAULT_BORDER)) > 1) {
			if(r > 0) {
				bm_fillroundrect(bmp, m, m, bmp->w - 1 - m, bmp->h - 1 - m, r);
			} else {
				bm_fillrect(bmp, m, m, bmp->w - 1 - m, bmp->h - 1 - m);
			}
			bm_set_color_s(bmp, get_style(s, "background", DEFAULT_BORDER));
			if(r > 0) {
				bm_fillroundrect(bmp, m + b, m + b,
					bmp->w - 1 - (m + b), bmp->h - 1 - (m + b),
					r - (b >> 1));
			} else {
				bm_fillrect(bmp, m + b, m + b,
					bmp->w - 1 - (m + b), bmp->h - 1 - (m + b));
			}
		} else {
			if(r > 0) {
				bm_roundrect(bmp, m, m, bmp->w - 1 - m, bmp->h - 1 - m, r);
			} else {
				bm_rect(bmp, m, m, bmp->w - 1 - m, bmp->h - 1 - m);
			}
		}
	}
}

/* State Functions ***********************************/

static void basic_state(struct game_state *s, struct bitmap *bmp) {
	int tw, th;
	int w, h;
	int tx, ty;

	const char *img_name = ini_get(game_ini, s->name, "image", NULL);
	struct bitmap *img = NULL;
	int ix = 0, iy = 0;

	const char *text = ini_get(game_ini, s->name, "text", "?");
	const char *halign = ini_get(game_ini, s->name, "halign", "center");
	const char *valign = ini_get(game_ini, s->name, "valign", "center");

	int m = atoi(get_style(s, "margin", DEFAULT_MARGIN));
	int b = atoi(get_style(s, "border", DEFAULT_BORDER));
	int p = atoi(get_style(s, "padding", DEFAULT_PADDING));

	bm_set_color_s(bmp, get_style(s, "background", DEFAULT_BACKGROUND));
	bm_clear(bmp);

	draw_border(s, bmp);

	bm_std_font(bmp, bm_font_index(get_style(s, "font", DEFAULT_FONT)));

	w = tw = bm_text_width(bmp, text);
	h = th = bm_text_height(bmp, text);

	if(img_name) {
        rlog("Loading image: %s for state", img_name);
		img = re_get_bmp(img_name);
		if(!img) {
			rerror("Unable to load '%s'", img_name);
		} else {
			const char *align = get_style(s, "image-align", DEFAULT_IMAGE_ALIGN);
			int margin = atoi(get_style(s, "image-margin", DEFAULT_IMAGE_MARGIN));
			if(!my_stricmp(align, "left") || !my_stricmp(align, "right")) {
				w += img->w + (margin << 1);
				h = MY_MAX(img->h + margin, th);
			} else {
				h += img->h + (margin << 1);
				w = MY_MAX(img->w + margin, tw);
			}
		}
	}

	if(!my_stricmp(halign, "left")) {
		tx = (m + b + p);
	} else if(!my_stricmp(halign, "right")) {
		tx = bmp->w - 1 - w - (m + b + p);
	} else {
		tx = (bmp->w - w) >> 1;
	}

	if(!my_stricmp(valign, "top")) {
		ty = (m + b + p);
	} else if(!my_stricmp(valign, "bottom")) {
		ty = bmp->h - 1 - h - (m + b + p);
	} else {
		ty = (bmp->h - h) >> 1;
	}

	if(img) {
		const char *align = get_style(s, "image-align", DEFAULT_IMAGE_ALIGN);
		int im = atoi(get_style(s, "image-margin", DEFAULT_IMAGE_MARGIN));
		const char *mask = get_style(s, "image-mask", NULL);

		if(!my_stricmp(align, "left")) {
			ix = tx;
			tx += img->w + im;
			iy = ty;
		} else if(!my_stricmp(align, "right")) {
			ix = tx + tw + im;
			iy = ty;
		} else if(!my_stricmp(align, "bottom")) {
			iy = ty + th + im;
			ix = tx;
		} else {
			// "top"
			iy = ty;
			ty += img->h + im;
			ix = tx;
		}

		if(!my_stricmp(align, "left") || !my_stricmp(align, "right")) {
			if(th > img->h) {
				iy += (th - img->h) >> 1;
			} else {
				ty += (img->h - th) >> 1;
			}
		} else {
			if(tw > img->w) {
				ix += (tw - img->w) >> 1;
			} else {
				tx += (img->w - tw) >> 1;
			}
		}

		if(mask) {
			bm_set_color_s(img, mask);
			bm_maskedblit(bmp, ix, iy, img, 0, 0, img->w, img->h);
		} else {
			bm_blit(bmp, ix, iy, img, 0, 0, img->w, img->h);
		}
	}

	bm_set_color_s(bmp, get_style(s, "foreground", DEFAULT_FOREGROUND));
	bm_puts(bmp, tx, ty, text);
}

static int basic_init(struct game_state *s) {
	reset_keys();
	return 1;
}

static int basic_deinit(struct game_state *s) {
	return 1;
}

/* Static State **************************************/

static int static_update(struct game_state *s, struct bitmap *bmp) {

	basic_state(s, bmp);

	if(kb_hit() || mouse_clck & SDL_BUTTON(1)) {
		const char *nextstate = ini_get(game_ini, s->name, "nextstate", NULL);
		struct game_state *next;

		if(!nextstate) {
			rlog("State '%s' does not specify a next state. Terminating...", s->name);
			change_state(NULL);
		} else {
			next = get_state(nextstate);
			if(!next) {
				rerror("State '%s' does not exist.", nextstate);
			}
			change_state(next);
		}
	}

	return 1;
}

static int static_init(struct game_state *s) {
	return basic_init(s);
}

static int static_deinit(struct game_state *s) {
	return basic_deinit(s);
}

static struct game_state *get_static_state(const char *name) {
	struct game_state *state = malloc(sizeof *state);
	if(!state)
		return NULL;

	state->init = static_init;
	state->update = static_update;
	state->deinit = static_deinit;

	return state;
}

/* Left-Right State **********************************/

struct left_right {
	int dir; /* left=0; right=1 */
};

static void draw_button(struct game_state *s, struct bitmap *bmp, int x, int y, int w, int h, int pad_left, int pad_top, const char *label, int highlight) {
	int br = atoi(get_style(s, "border-radius", DEFAULT_BORDER_RADIUS));
	bm_set_color_s(bmp, get_style(s, "foreground", DEFAULT_FOREGROUND));
	if(highlight) {
		if(br > 1)
			bm_fillroundrect(bmp, x, y, x + w, y + h, br);
		else
			bm_fillrect(bmp, x, y, x + w, y + h);
		bm_set_color_s(bmp, get_style(s, "background", DEFAULT_BACKGROUND));
	} else {
		if(br > 1)
			bm_roundrect(bmp, x, y, x + w, y + h, br);
		else
			bm_rect(bmp, x, y, x + w, y + h);
	}
	bm_puts(bmp, x + pad_left, y + pad_top, label);
}

static int leftright_update(struct game_state *s, struct bitmap *bmp) {
	int bw1, bh1, bw2, bh2, bw, bh;
	int bx1, by1, bx2, by2;

	int m = atoi(get_style(s, "margin", DEFAULT_MARGIN));
	int b = atoi(get_style(s, "border", DEFAULT_BORDER));
	int p = atoi(get_style(s, "padding", DEFAULT_PADDING));
	int bp = atoi(get_style(s, "button-padding", DEFAULT_BUTTON_PADDING));

	int clicked = 0;

	const char *left_label = ini_get(game_ini, s->name, "left-label", "Left");
	const char *right_label = ini_get(game_ini, s->name, "right-label", "Right");

	struct left_right *lr = s->data;

	basic_state(s, bmp);

	/* FIXME: styles to position the buttons better */
	bw1 = bm_text_width(bmp, left_label);
	bh1 = bm_text_height(bmp, left_label);

	bw2 = bm_text_width(bmp, right_label);
	bh2 = bm_text_height(bmp, right_label);

	bw = MY_MAX(bw1, bw2) + (bp << 1);
	bh = MY_MAX(bh1, bh2) + (bp << 1);

	/* Left button */
	bx1 = (m + b + p);
	by1 = bmp->h - 1 - bh - (m + b + p);

	/* Right button */
	bx2 = bmp->w - 1 - bw - (m + b + p);
	by2 = bmp->h - 1 - bh - (m + b + p);

	/* Cursor over  */
	if(mouse_x >= bx1 && mouse_x <= bx1 + bw && mouse_y >= by1 && mouse_y <= by1 + bh) {
		clicked = mouse_clck & SDL_BUTTON(1);
		lr->dir = 0;
	} else if(mouse_x >= bx2 && mouse_x <= bx2 + bw && mouse_y >= by2 && mouse_y <= by2 + bh) {
		clicked = mouse_clck & SDL_BUTTON(1);
		lr->dir = 1;
	}

	if(kb_hit() || clicked) {
        int k = readkey();
		if(k == SDL_SCANCODE_SPACE || k == SDL_SCANCODE_RETURN || clicked) {
			if(!lr->dir) {
				const char *nextstate = ini_get(game_ini, s->name, "left-state", NULL);
				struct game_state *next;

				if(!nextstate) {
					rlog("State '%s' does not specify a left state. Terminating...", s->name);
					change_state(NULL);
				} else {
					next = get_state(nextstate);
					if(!next) {
						rerror("State '%s' does not exist", nextstate);
					}
					change_state(next);
				}
			} else {
				const char *nextstate = ini_get(game_ini, s->name, "right-state", NULL);
				struct game_state *next;

				if(!nextstate) {
					rlog("State '%s' does not specify a right state. Terminating...", s->name);
					change_state(NULL);
				} else {
					next = get_state(nextstate);
					if(!next) {
						rerror("State '%s' does not exist", nextstate);
					}
					change_state(next);
				}
			}
			return 1;
		}

		if(k == SDL_SCANCODE_LEFT || k == SDL_SCANCODE_UP) {
			lr->dir = 0;
		} else if(k == SDL_SCANCODE_RIGHT || k == SDL_SCANCODE_DOWN) {
			lr->dir = 1;
		}
	}

	/* Draw the buttons */
	draw_button(s, bmp, bx1, by1, bw, bh, ((bw-bw1)>>1), ((bh-bh1)>>1), left_label, !lr->dir);

	draw_button(s, bmp, bx2, by2, bw, bh, ((bw-bw2)>>1), ((bh-bh2)>>1), right_label, lr->dir);

	return 1;
}

static int leftright_init(struct game_state *s) {
	struct left_right *lr = malloc(sizeof *lr);
	if(!lr)
		return 0;
	lr->dir = 0;
	s->data = lr;
	return basic_init(s);
}

static int leftright_deinit(struct game_state *s) {
	free(s->data);
	return basic_deinit(s);
}

static struct game_state *get_leftright_state(const char *name) {
	struct game_state *state = malloc(sizeof *state);
	if(!state)
		return NULL;

	state->init = leftright_init;
	state->update = leftright_update;
	state->deinit = leftright_deinit;

	return state;
}

/* Functions *****************************************/

struct game_state *current_state() {
	assert(state_top >= 0 && state_top < MAX_NESTED_STATES);
	return game_states[state_top];
}

void states_initialize() {
	int i;
	for(i = 0; i < MAX_NESTED_STATES; i++) {
		game_states[i] = NULL;
	}
}

int change_state(struct game_state *next) {
    
    if(next)
        rlog("Changing to state %s", next->name);
    else
        rlog("Changing to null state");
	
    if(game_states[state_top] && game_states[state_top]->deinit) {
		if(!game_states[state_top]->deinit(game_states[state_top])) {
			rerror("Deinitialising old state");
			return 0;
		}
		if(game_states[state_top]->name)
			free(game_states[state_top]->name);

		if(game_states[state_top]->styles) {
			ht_free(game_states[state_top]->styles, NULL);
		}

		free(game_states[state_top]);
	}
	
    game_states[state_top] = next;
	if(game_states[state_top]){
        const char *show_cursor_local;
        int show;

		game_states[state_top]->styles = ht_create(0);

        show_cursor_local = ini_get(game_ini, next->name, "show-cursor", NULL);
        show = show_cursor;
        if(show_cursor_local) {
            show = !!atoi(show_cursor_local);
        }
        if(SDL_ShowCursor(show) < 0) {
            rerror("SDL_ShowCursor: %s", SDL_GetError());
        }

		if(game_states[state_top]->init && !game_states[state_top]->init(game_states[state_top])) {
			rerror("Initialising new state");
			return 0;
		}
	}
    
    if(game_states[state_top])
        rlog("Game state is now %s", game_states[state_top]->name);
    else
        rlog("Game state is now null");
        
	return 1;
}

#if 0
/* As of now, consider these functions deprecated */
int push_state(struct game_state *next) {
	if(state_top > MAX_NESTED_STATES - 1) {
		rerror("Too many nested states");
		return 0;
	}
	state_top++;
	re_push();
	return change_state(next);
}

int pop_state(struct game_state *next) {
	int r;
	if(state_top <= 0) {
		rerror("State stack underflow.");
		return 0;
	}
	r = change_state(NULL);
	state_top--;

	re_pop();
	return r;
}
#endif

struct game_state *get_state(const char *name) {
	const char *type;

	struct game_state *next = NULL;

	if(!name) return NULL;

	if(!ini_has_section(game_ini, name)) {
		rerror("No state %s in ini file", name);
		quit = 1;
		return NULL;
	}

	type = ini_get(game_ini, name, "type", NULL);
	if(!type) {
		rerror("No state type for state '%s' in ini file", name);
		quit = 1;
		return NULL;
	}

	if(!my_stricmp(type, "static")) {
		next = get_static_state(name);
	} else if(!my_stricmp(type, "lua")) {
		next = get_lua_state(name);
	} else if(!my_stricmp(type, "leftright")) {
		next = get_leftright_state(name);
	} else {
		rerror("Invalid type '%s' for state '%s' in ini file", type, name);
		quit = 1;
		return NULL;
	}
	if(!next) {
		rerror("Memory allocation error obtaining state %s", name);
		quit = 1;
	}

	next->name = strdup(name);

	return next;
}

int set_state(const char *name) {
	struct game_state *next;
	rlog("Changing to state '%s'", name);
	next = get_state(name);
	return change_state(next);
}

