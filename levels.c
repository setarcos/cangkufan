/*
 * level.c : level management.
 * 
 * Copyright (C) 2003 Pluto Yang <yangyj.ee@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <sys/stat.h>

#include "sokoban.h"

int is_wall (int x, int y)
{
	if ((x < 0) || (x >= W_BOARD)) return 1;
	if ((y < 0) || (y >= H_BOARD)) return 1;
	if (board[x][y] % PIXMAP_COUNT == SOKO_WALL) return 1;
	return 0;
}

void restart (void)
{
	level_change (level_no);
}

void next_level (void)
{
	level_change (level_no + 1);
}

void prior_level (void)
{
	level_change (level_no - 1);
}

int load_level (char* levelname)
{
	FILE *fp;
	char buf[1024];
	char tmp;
	int i, j, k, width, height;


	for (i = 0; i < W_BOARD; i++)
		for (j = 0; j < H_BOARD; j++)
			board[i][j] = SOKO_WALL;	/* Initial value */
	
	fp = fopen (levelname, "r");
	if (fp == NULL) return -1;

	height = 0;
	while (fgets(buf, 1024, fp) != NULL)
	{
		if (buf[0] != '#') continue;  /* Ignore comment line */
		
		width = strlen(buf);

		/* Fixe end of line */
		for (i = 0; i < 2; i ++)
		{
			if ((buf[width-1] == '\r') || (buf[width-1] == '\n'))
			{
				buf[width-1] = 0;
				width--;
			}
		}
		
		if (width > W_BOARD) return -1; /* Invalid level file */
		
		for (i = 0; i < width; i++)
		{
			switch (buf[i])
			{
				case ' ':
					tmp = SOKO_NONE;
					break;
				case '#':	/* Wall */
					tmp = SOKO_WALL;
					break;
				case '.':	/* Container */
					tmp = SOKO_CONT;
					break;
				case '$':	/* Box */
					tmp = SOKO_BOX;
					break;
				case '@':	/* Worker */
					tmp = SOKO_WORKER;
					break;
				case '*':	/* Box over container */
					tmp = SOKO_BOX2;
					break;
				case '+':	/* Worker over container */
					tmp = SOKO_WORKER2;
					break;
				default:
					return -2;  /* Invalid level file */
			}
		board [(W_BOARD - width) / 2 + i][height] = tmp; /* Adjust the horizontal position */
		}
		height++;
	}
	fclose(fp);

	/* Adjust the vertical position */
	k = (H_BOARD - height) / 2;
	for (i = H_BOARD - k; i >= 0; i--)
		for (j = 0; j < W_BOARD; j ++)
		{
			if ((i - k) >= 0) board[j][i] = board[j][i - k];
			else board[j][i] = SOKO_WALL;
		}

	/* Remove outer wall */
	for (i = 0; i < W_BOARD; i++)
		for (j = 0; j < H_BOARD; j++)
			if ((is_wall(i-1, j-1)) &&
					(is_wall(i, j-1)) &&
					(is_wall(i+1, j-1)) &&
					(is_wall(i-1, j)) &&
					(is_wall(i+1, j)) &&
					(is_wall(i-1, j+1)) &&
					(is_wall(i, j+1)) &&
					(is_wall(i+1, j+1)))
				board[i][j] = board[i][j] + PIXMAP_COUNT;
	
	boxes_left = 0;
	for (i = 0; i < W_BOARD; i++)
		for (j = 0; j < H_BOARD; j++)
		{
			if (board[i][j] > PIXMAP_COUNT) board[i][j] = SOKO_OUTER;

			/* Get the number of boxes left */
			if (board[i][j] == SOKO_BOX) boxes_left++;

			/* Get the character position */
			if ((board[i][j] == SOKO_WORKER) || (board[i][j] == SOKO_WORKER2))
			{
				char_x = i;
				char_y = j;
			}
		}
	/* Initial the start values */
	steps = 0;
	pushes = 0;
	game_state = GM_NORMAL;
	return 0;
}

int level_change (int number)
{
	int tmp;
	gchar *filename;

    if (game_state == GM_BOX_SEL) /* Box moves */
    {
        game_state = GM_NORMAL;
        board[box_x][box_y] -= (SOKO_SEL - SOKO_BOX);
        invalidate_xy(box_x, box_y);
	}
	
	if ((number > MAX_LEVEL) || (number < 1)) return -1;
	level_no = number;
	
	filename = g_strdup_printf ( DATADIR "/levels/%i.xsb", level_no);
	tmp = load_level(filename);
	if (tmp) return tmp; /* Level file corrupted */

	repaint_all ();
	refresh_status ();
	g_free (filename);
	return 0;
}

	/* Load initial level and position */
void load_steps (void)
{
	gchar *filename, *dirname;
	FILE *fp;
	int i, j;

	filename = g_strdup_printf ( "%s/.sokoban/default", getenv("HOME"));
	if (access (filename, R_OK))
	{
		dirname = g_strdup_printf ("%s/.sokoban", getenv("HOME"));
		mkdir (dirname, 0777); /* Use system umask ? */
		level_no = 1;
		level_change (1);
		g_free (dirname);
	}
	else
	{
		fp = fopen (filename, "r");
		if (fscanf (fp, "%d", &level_no) != 1)
			level_no = 1;
		level_change (level_no);
		
		if (fscanf (fp, "%s", steps_his) == 1) /* Possible overflow */
		{
			for (i = 0; i < strlen(steps_his); i++)
			{
				/* if something is wrong */
				if (key_move(steps_his[i] % 4, NO_DRAW))
				{
					level_change (level_no); /* Reload level */
					break;
				}
			}
			repaint_all ();
		}
		fclose (fp);
	}
	g_free (filename);

	filename = g_strdup_printf ("%s/.sokoban/high_score", getenv("HOME"));
	j = access (filename, R_OK);
	if (!j)
	{
		fp = fopen (filename, "r");
		if (fread (level_result, 1, sizeof(level_result), fp) != sizeof(level_result))
			j = 1;
		fclose (fp);
	}
	if (j) memset(level_result, 0, sizeof(level_result));
	g_free (filename);
	refresh_status ();
}

	/* Save current level and postion */
void save_steps (void)
{
	gchar *filename;
	FILE *fp;

	filename = g_strdup_printf ("%s/.sokoban/default", getenv("HOME"));

	steps_his[steps] = 0; /* Terminate this string */
	
	if ((fp = fopen (filename, "w")) != NULL)
		fprintf (fp, "%d\n%s", level_no, steps_his);
	fclose (fp);
	g_free (filename);

	filename = g_strdup_printf ("%s/.sokoban/high_score", getenv("HOME"));
	if ((fp = fopen (filename, "w")) != NULL)
		fwrite (level_result, 1, sizeof(level_result), fp);
	fclose(fp);
	g_free (filename);
}

void save_his (int best)
{
	gchar *filename;
	FILE *fp;

	if (best == BEST_MOVE)
		filename = g_strdup_printf ("%s/.sokoban/%d.move", getenv("HOME"), level_no);
	if (best == BEST_PUSH)
		filename = g_strdup_printf ("%s/.sokoban/%d.push", getenv("HOME"), level_no);
	steps_his[steps] = 0;
	if ((fp = fopen (filename, "w")) != NULL)
		fprintf (fp, "%s", steps_his);
	fclose (fp);
	g_free (filename);
}

void replay_best (int best)
{
	gchar *filename;
	FILE *fp;

	level_change (level_no);
    if (best == BEST_MOVE)
		filename = g_strdup_printf ("%s/.sokoban/%d.move", getenv("HOME"), level_no);
	if (best == BEST_PUSH)
		filename = g_strdup_printf ("%s/.sokoban/%d.push", getenv("HOME"), level_no);
	if ((fp = fopen (filename, "r")) != NULL)
		fscanf (fp, "%s", steps_his);
	fclose (fp);
	g_free (filename);
	replay (steps_his);
}

void replay_bp (void)
{
	replay_best (BEST_PUSH);
}

void replay_bm (void)
{
	replay_best (BEST_MOVE);
}

void cancel_replay (void)
{
	auto_move = FALSE;
}

	/* Update high scores */
/* int update_level (void)
{
}

*/

/*
int main(void)
{
	int i, j;
	
	i = load_level("levels/1.xsb");

	if (i) 
	{
		printf (":(, %d\n", i);
		return -1;
	}

	for (i = 0; i < H_BOARD; i++)
	{
		for (j =0; j < W_BOARD; j++)
			printf("%2d", board[i][j]);
		printf("\n");
	}
	return 0;
}

*/
