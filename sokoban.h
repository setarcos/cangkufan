#ifndef __SOKOBAN_H__
#define __SOKOBAN_H__
/* dimensions of the board (unit = tile) */

#include <gtk/gtk.h>

#define TILE_SIZE 32
#define W_BOARD	20
#define H_BOARD	16
#define DATADIR "."

#define PIXMAP_COUNT 9
#define MAX_STEP 4096

#define MAX_LEVEL 50

#define BASE_NOR 25
#define BASE_PUSH 26
#define BASE_MOUSE 27
#define BASE_MOUSE2 28

#define HIS_NOR BASE_NOR * 4  /* 'd' to 'g' for normal move in history list */
#define HIS_PUSH BASE_PUSH * 4 /* 'h' to 'k' for push */
#define HIS_MOUSE BASE_MOUSE * 4 /* Mouse moves */
#define HIS_MOUSE2 BASE_MOUSE2 * 4 /* Mouse pushes */


#define BEST_MOVE 0
#define BEST_PUSH 1

#define ANIM_PLAY 100

struct level_info
{
	gboolean solved;
	int best_steps;
	int best_pushes;
};

enum { SOKO_NONE, SOKO_CONT, SOKO_WALL, SOKO_BOX, SOKO_BOX2, 
	SOKO_WORKER, SOKO_WORKER2, SOKO_SEL, SOKO_SEL2, SOKO_OUTER };

/* Game mode */
enum { GM_NORMAL, GM_BOX_SEL, GM_PLAYBACK, GM_MOVING};

/* Move flag */
enum { DRAW_NORMAL, NO_DRAW, NO_ENDING};

typedef char tboard[W_BOARD][H_BOARD];


/* Global variables */

/* Board layout */
tboard board;

/* Character position */
int char_x, char_y;

int steps, pushes, boxes_left;

int level_no;

gboolean auto_move; /* Unset this to cancel playback or mouse_move. */
int auto_steps;

int game_state;

char steps_his[MAX_STEP];

struct level_info level_result[MAX_LEVEL];

int box_x, box_y;

void invalidate_xy (int, int);
void refresh_status (void);
void repaint_all (void);
void goodbye (void);
void about (void);

int is_wall (int x, int y);
int load_level (char* levelname);
int level_change (int number);
void save_steps (void);
void load_steps (void);
void save_his (int);
void restart (void);
void next_level (void);
void prior_level (void);
void replay_bp (void);
void replay_bm (void);
void cancel_replay (void);

int key_move (int, int);
void undo_move (void);
void redo_move (void);
void replay(gchar *moves);
gboolean try_go_xy (tboard b, int x, int y, int cx, int cy, char *path);
gboolean try_push_xy (tboard b, int x, int y, char *path);

#endif
