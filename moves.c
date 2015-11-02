/*
 * moves.c : move and undo functions. 
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

#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include "sokoban.h"

static int dirs[4][2]={{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
static int back_dirs[4]={1, 0, 3, 2};
static int dirs_mask[4]={1, 2, 4, 8};

gboolean auto_play (gpointer data)
{
	gchar *str = (gchar *) data;

	if ((auto_move) && (str[auto_steps]))
	{
		if (key_move (str[auto_steps] % 4, NO_ENDING) == -1) /* Wrong? Stop! */
			return FALSE;
		auto_steps++;
		return TRUE;
	}
	auto_move = FALSE;
	return FALSE;
}


void replay(char *moves)
{
	auto_move = TRUE;
	auto_steps = 0;
	gtk_timeout_add ( ANIM_PLAY, auto_play, (gpointer) moves);
}

int key_move(int dir, int flag)
{
	int i, j;
	
	if (steps == MAX_STEP) return -1;
	
	i = board[char_x + dirs[dir][0]][char_y + dirs[dir][1]];
	if ((i == SOKO_NONE) || (i == SOKO_CONT))
	{
		board[char_x][char_y] -= SOKO_WORKER;
		if (flag != NO_DRAW) invalidate_xy(char_x, char_y);
		char_x += dirs[dir][0];
		char_y += dirs[dir][1];
		board[char_x][char_y] += SOKO_WORKER;
		if (flag != NO_DRAW) invalidate_xy(char_x, char_y);
		steps_his[steps] = HIS_NOR + dir;
		steps++;
		if (flag == DRAW_NORMAL) steps_his[steps] = 0;
	}
	else if ((i == SOKO_BOX) || (i == SOKO_BOX2))
	{
		j =  board[char_x + dirs[dir][0] * 2][char_y + dirs[dir][1] * 2];
		if ((j == SOKO_NONE) || (j == SOKO_CONT))
		{
			board[char_x][char_y] -= SOKO_WORKER;
			if (flag != NO_DRAW) invalidate_xy(char_x, char_y);
			char_x += dirs[dir][0];
			char_y += dirs[dir][1];
			board[char_x][char_y] -= (SOKO_BOX - SOKO_WORKER);
			if (flag != NO_DRAW) invalidate_xy(char_x, char_y);
			board[char_x + dirs[dir][0]][char_y + dirs[dir][1]] += SOKO_BOX;
			if (flag != NO_DRAW) invalidate_xy(char_x + dirs[dir][0], char_y + dirs[dir][1]);
			steps_his[steps] = HIS_PUSH + dir;
			steps++;
			if (flag == DRAW_NORMAL) steps_his[steps] = 0;
			pushes++;
			if (board[char_x][char_y] == SOKO_WORKER2)
				boxes_left++;
			if (board[char_x + dirs[dir][0]][char_y + dirs[dir][1]] == SOKO_BOX2)
				boxes_left--;
		}
	}
	else /* Can not move on this direction */
		return -1;
	
	/* Max steps reached */
	if (steps == MAX_STEP)
	{
		/* Please giveup */
	}

	/* Level up */
	if (boxes_left == 0)
	{
		level_result[level_no].solved = TRUE;
		i = level_result[level_no].best_pushes;
		if ((i == 0) || (i > pushes))
		{
			level_result[level_no].best_pushes = pushes;
			save_his (BEST_PUSH);
		}
		i = level_result[level_no].best_steps;
		if ((i == 0) || (i > steps))
		{
			level_result[level_no].best_steps = steps;
			save_his (BEST_MOVE);
		}
	}
	
	refresh_status ();
	
	return 0;
}

void redo_move(void)
{
	if (auto_move) return; /* replay mode */
	if (steps_his[steps]) key_move (steps_his[steps] % 4, NO_ENDING);
}

void undo_move(void)
{
	int dir;
	
	if (steps == 0) return;
	if (auto_move) return; /* replay mode */

    if (game_state == GM_BOX_SEL) /* Box moves */
    {
        game_state = GM_NORMAL;
        board[box_x][box_y] -= (SOKO_SEL - SOKO_BOX);
        invalidate_xy(box_x, box_y);
	}
	
	steps--;
	dir = steps_his[steps] % 4;
	
	switch (steps_his[steps] / 4)
	{
		case BASE_NOR:
			board[char_x][char_y] -= SOKO_WORKER;
			invalidate_xy(char_x, char_y);
			char_x -= dirs[dir][0];
			char_y -= dirs[dir][1];
			board[char_x][char_y] += SOKO_WORKER;
			invalidate_xy(char_x, char_y);
			break;
		case BASE_PUSH:
			if (board[char_x + dirs[dir][0]][char_y + dirs[dir][1]] == SOKO_BOX2)
				boxes_left++;
			if (board[char_x][char_y] == SOKO_WORKER2) boxes_left--;
			board[char_x + dirs[dir][0]][char_y + dirs[dir][1]] -= SOKO_BOX;
			invalidate_xy(char_x + dirs[dir][0], char_y + dirs[dir][1]);
			board[char_x][char_y] -= (SOKO_WORKER - SOKO_BOX);
			invalidate_xy(char_x, char_y);
			char_x -= dirs[dir][0];
			char_y -= dirs[dir][1];
			board[char_x][char_y] += SOKO_WORKER;
			invalidate_xy(char_x, char_y);
			pushes--;
			break;
		case BASE_MOUSE:
			break;
	}
	refresh_status ();
}

gboolean try_go_xy (tboard b, int x, int y, int cx, int cy, char *path)
{
	int i, j, k, l, m, n, deep;
	int locale_b[W_BOARD][H_BOARD];

	if ((x == cx) && (y == cy))
	{
		path[0] = 0;
		return TRUE;
	}

	for (i = 0; i < W_BOARD; i++)
		for (j = 0; j < H_BOARD; j++)
		{
			if ((b[i][j] == SOKO_NONE) || (b[i][j] == SOKO_CONT )
					|| (b[i][j] == SOKO_WORKER) || (b[i][j] == SOKO_WORKER2))
				locale_b[i][j] = 0;
			else locale_b[i][j] = -1;
		}
	
	locale_b[cx][cy] = 1;
	
	deep = 1;
	while (1)
	{
		l = 0;
		for (i = 0; i < W_BOARD; i++)
			for (j = 0; j < H_BOARD; j++)
			{
				if (locale_b[i][j] == deep)
					for (k = 0; k < 4; k++)
						if (locale_b[i + dirs[k][0]][j + dirs[k][1]] == 0)
						{
							if ((i + dirs[k][0] == x) && (j + dirs[k][1] == y))
							{
								/* Found way out */
								path[deep] = 0;
								for (m = 0; m < deep; m++)
									for (n = 0; n < 4; n++)
									{
										if ((locale_b[x + dirs[n][0]][y + dirs[n][1]]) == deep - m)
										{
											x += dirs[n][0];
											y += dirs[n][1];
											path[deep - m - 1] = HIS_NOR + back_dirs[n];
											break;
										}
									}
								return TRUE;
							}
							locale_b[i + dirs[k][0]][j + dirs[k][1]] = deep + 1;
							l++;
						}
			}
		
		if (l == 0) return FALSE; /* No path found */
		deep++;
	}
}

void copyboard (tboard a, tboard b)
{
	int i, j;
	for (i = 0; i < W_BOARD; i++)
		for (j = 0; j < H_BOARD; j++)
			b[i][j] = a[i][j];
}

gboolean try_push_xy (tboard b, int x, int y, char *path)
{
	int i, j, l, m, n, index;
	char go_path[MAX_STEP];
	tboard c;					/* pass to try_go_xy */
	int a[W_BOARD][H_BOARD];	/* board to compute the pass */
	int ix, iy;

	int ii, ij;
	
	struct
	{
		int up;
		int dir;
		int newbox_x, newbox_y;
		int newcx, newcy;
	}search_path[W_BOARD * H_BOARD];

    for (i = 0; i < W_BOARD; i++)
		for (j = 0; j < H_BOARD; j++)
		{
			if ((b[i][j] == SOKO_NONE) || (b[i][j] == SOKO_CONT )
				|| (b[i][j] == SOKO_WORKER) || (b[i][j] == SOKO_WORKER2))
				a[i][j] = 0;
			else a[i][j] = -1;
		}
	a[box_x][box_y] = 0;

	index = 1;
	search_path[0].up = -1;
	search_path[0].newbox_x = box_x;
	search_path[0].newbox_y = box_y;
	search_path[0].newcx = char_x;
	search_path[0].newcy = char_y;

	m = 0; /* Search start */
	n = 1; /* Search end */
	
	while (1)
	{
		l = 0;
		for (j = m; j < n; j++)
		{
			copyboard(b, c);
			ix = search_path[j].newbox_x;
			iy = search_path[j].newbox_y;
			c[box_x][box_y] -= SOKO_BOX;
			c[ix][iy] += SOKO_BOX;
			
			for (i = 0; i < 4; i++)
			{
				if (   (a[ix - dirs[i][0]][iy - dirs[i][1]] != -1)
					&& ((a[ix + dirs[i][0]][iy + dirs[i][1]] & dirs_mask[i]) == 0)
					&& (try_go_xy(c, ix - dirs[i][0], iy - dirs[i][1],
						search_path[j].newcx, search_path[j].newcy, go_path)))
				{
					//printf ("up: %d, dir: %d, index: %d\n", j, i, index);
					search_path[index].up = j;
					search_path[index].dir = i;
					search_path[index].newcx = ix;
					search_path[index].newcy = iy;
					search_path[index].newbox_x = search_path[j].newbox_x + dirs[i][0];
					search_path[index].newbox_y = search_path[j].newbox_y + dirs[i][1];
					a[search_path[index].newbox_x][search_path[index].newbox_y] |= dirs_mask[i];
					if ((search_path[index].newbox_x == x) && (search_path[index].newbox_y == y))
					{
						/* Found path out */
						ii = search_path[index].up;
						while(ii != -1) /* reverse the "up" index */
						{
							ij = ii;
							ii = search_path[ij].up;
							search_path[ij].up = index;
							index = ij;
						}

						/* index must be 0 now */
						ix = 0;
						iy = 0;
						path[0] = 0;
						while (1)
						{
							copyboard(b, c);
							ix = search_path[index].newbox_x;
							iy = search_path[index].newbox_y;
							if ((ix == x) && (iy == y)) break;
							ii = search_path[search_path[index].up].dir;
							
							c[box_x][box_y] -= SOKO_BOX;
							c[ix][iy] += SOKO_BOX;
							try_go_xy(c, ix - dirs[ii][0], iy - dirs[ii][1],
									search_path[index].newcx,
									search_path[index].newcy,
									go_path);
							ij = strlen(go_path);
							go_path[ij] = HIS_NOR + ii;
							go_path[ij + 1] = 0;
							strcat(path, go_path);
							index = search_path[index].up;
						}
						return TRUE;
					}
					l++;
					index++;
				}
			}
			
		}
		if (l == 0) return FALSE; /* No path found */
		m = n;
		n = index;
	}
}
