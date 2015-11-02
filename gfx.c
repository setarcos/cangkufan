/*
 * gfx.c : main function and graphic related functions.
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
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "sokoban.h"

GdkGC *darea_gc; 
GtkWidget *draw_area;
GtkWidget *label_bar;
GdkPixmap* pixmaps[PIXMAP_COUNT];

/* Backing pixmap for drawing area */
static GdkPixmap *d_pixmap = NULL;

static char* pxm_name[PIXMAP_COUNT] = {DATADIR"/images/space32.xpm",
	DATADIR"/images/pos32.xpm", DATADIR"/images/wall32.xpm",
	DATADIR"/images/box32.xpm", DATADIR"/images/box_32.xpm",
	DATADIR"/images/worker32.xpm", DATADIR"/images/worker_32.xpm",
	DATADIR"/images/sel32.xpm", DATADIR"/images/sel_32.xpm"};

static GtkItemFactoryEntry menu_items[] = {
    { "/_File",     		NULL,       NULL,       0,  "<Branch>"},
    { "/File/_Restart", 	"R",        restart,    0,  NULL},
    { "/File/_Next Level",	"N",        next_level, 0,  NULL},
	{ "/File/_Prior Level",	"P",		prior_level,0,	NULL},
    { "/File/sep",      	NULL,       NULL,       0,  "<Separator>"},
    { "/File/_Quit",     	"Q",    	goodbye,    0,  NULL},
	{ "/_Moves",			NULL,		NULL,		0,	"<Branch>"},
    { "/Moves/_Undo move",	"U",		undo_move,	0,	NULL},
 	{ "/Moves/R_edo move",	"E",		redo_move,	0,	NULL},
	{ "/Moves/Replay best push",	NULL,	replay_bp, 0, NULL},
	{ "/Moves/Replay best move",	NULL,	replay_bm, 0, NULL},
	{ "/Moves/Cancel replay",	NULL,	cancel_replay, 0, NULL},
    { "/_Help",     		NULL,       NULL,       0,  "<LastBranch>"},
    { "/Help/_About",   	NULL,       about,      0,  NULL},
};

/* build the menubar */
void get_main_menu(GtkWidget *window, GtkWidget **menubar)
{   
    GtkItemFactory *item_factory;
    GtkAccelGroup *accel_group;
    gint nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);
            
    accel_group = gtk_accel_group_new();
    item_factory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel_group);
    gtk_item_factory_create_items(item_factory, nmenu_items, menu_items, NULL);

    /* attach the new accelerator group to the window. */
    gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

    if (menubar)
        /* finally, return the actual menu bar created by the item factory. */
        *menubar = gtk_item_factory_get_widget(item_factory, "<main>");
}

void about(void)
{
    static GtkWidget *window = NULL;
    GtkWidget *label;
	GtkWidget *bbox;
	GtkWidget *vbox;
	GtkWidget *frame;
    GtkWidget *button;

    if (!window)
    {
        /* create a new dialog */
        window = gtk_dialog_new();
        gtk_window_set_title(GTK_WINDOW(window), "About");
        gtk_container_set_border_width(GTK_CONTAINER(window), 5);

        /* insert a frame in the vbox part of the dialog */
        frame = gtk_frame_new(NULL);
        gtk_container_set_border_width(GTK_CONTAINER(frame), 10);
        gtk_container_add(GTK_CONTAINER(GTK_DIALOG(window)->vbox), frame);

        /* put a vbox inside this frame */
        vbox = gtk_vbox_new(FALSE, 0);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);
        gtk_container_add(GTK_CONTAINER(frame), vbox);

        /* put another frame inside this vbox */
        frame = gtk_frame_new(NULL);
        gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);
        gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

        /* put a label inside this frame */
        label = gtk_label_new("Cangkufan");
        gtk_widget_set_usize(label, 300, 30);
        gtk_container_add(GTK_CONTAINER(frame), label);

        label = gtk_label_new(
"            Cangkufan (Sokoban) for Gtk+\n\n\
(C) 2003 Pluto Yang\n\
Contributors:\n\
   Pluto Yang\n\
   Sliper Gu\n\n\
            Homepage: http://github.com/setarcos/cangkufan");

        gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 10);

        /* create a new buttonbox and put it in the action_area of the dialog */
        bbox = gtk_hbutton_box_new();
        gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
        gtk_container_add(GTK_CONTAINER(GTK_DIALOG(window)->action_area), bbox);


        /* create a button with the label "OK" and put it into bbox */
        button = gtk_button_new_with_label("OK");
        gtk_container_add(GTK_CONTAINER(bbox), button);

        /* make it the default button (ENTER will activate it) */
        GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
        gtk_widget_grab_default(button);

        /* handle the deletion of the dialog */
        g_signal_connect_object (G_OBJECT(button), "clicked", G_CALLBACK(gtk_widget_hide), G_OBJECT(window), G_CONNECT_SWAPPED);
        g_signal_connect (G_OBJECT(window), "delete_event", G_CALLBACK(gtk_widget_hide), G_OBJECT(window));

		gtk_widget_show_all (window);
    }
    else
    {
        if (!GTK_WIDGET_MAPPED(window))
            gtk_widget_show(window);
        else
            gdk_window_raise(window->window);
    }
}

/* Redraw the screen from the backing pixmap */
static gboolean
expose_event( GtkWidget *widget, GdkEventExpose *event )
{
  gdk_draw_drawable(widget->window,
		  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		  d_pixmap,
		  event->area.x, event->area.y,
		  event->area.x, event->area.y,
		  event->area.width, event->area.height);

  return FALSE;
}

static void init_gfx (void)
{
	int i;
	
	/* Create the backup pixmap for darea */
	d_pixmap = gdk_pixmap_new(draw_area->window,
		draw_area->allocation.width,
		draw_area->allocation.height,
		-1);
	
	darea_gc = gdk_gc_new (d_pixmap);
	
	for (i = 0; i < PIXMAP_COUNT; i++)
		pixmaps[i] = gdk_pixmap_create_from_xpm (draw_area->window, NULL, NULL, pxm_name[i]);
}

void invalidate_xy(int x, int y)
{
	gdk_draw_drawable (d_pixmap,
			darea_gc,
			pixmaps[board[x][y] % PIXMAP_COUNT],
			0, 0, x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE);

	gtk_widget_queue_draw_area(draw_area,
			x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE);
}

void repaint_all (void)
{
	int i, j;

	for (i = 0; i < W_BOARD; i++)
		for (j = 0; j < H_BOARD; j++)
			invalidate_xy(i, j);
}

static gboolean delete_event( GtkWidget *widget,
                              GdkEvent  *event,
                              gpointer   data )
{
    gtk_main_quit ();
    return FALSE;
}

void goodbye (void)
{
	gtk_main_quit ();
}

static void key_press_event (GtkWidget *widget, GdkEventKey *event)
{
    if (game_state == GM_BOX_SEL) /* a box have been selected */
    {
        game_state = GM_NORMAL;
        board[box_x][box_y] -= (SOKO_SEL - SOKO_BOX);
        invalidate_xy(box_x, box_y);
	}
	
    switch(event->keyval)
    {   
        case GDK_Left:
            key_move (0, DRAW_NORMAL);  /* move left */
            break;
        case GDK_Right:
            key_move (1, DRAW_NORMAL);  /* move right */
            break;
        case GDK_Up:
            key_move (2, DRAW_NORMAL);  /* move up */
            break;
        case GDK_Down:
            key_move (3, DRAW_NORMAL);  /* move down */
            break;
		case GDK_BackSpace:
			undo_move ();
			break;
		case GDK_Page_Up:
			next_level ();
			break;
		case GDK_Page_Down:
			prior_level ();
			break;
	}
}

/* player pressed mouse button */
gint button_press_event(GtkWidget* widget, GdkEventButton* event) 
{
    int x,y;
	static char path[MAX_STEP];

    /* ignore double- and triple clicks */
    if ((event->type == GDK_2BUTTON_PRESS) ||
         (event->type == GDK_3BUTTON_PRESS) )
        return TRUE;

    /* ignore everything but left button */
    if (event->button != 1) 
       return TRUE;

	if (auto_move) 
	{
		auto_move = FALSE;
		return TRUE;
	}

	x = (int)event->x / TILE_SIZE;
	y = (int)event->y / TILE_SIZE;

	if ((x > W_BOARD) || (y > H_BOARD)) return FALSE;

	if ((board[x][y] == SOKO_NONE) || (board[x][y] == SOKO_CONT)
		|| (board[x][y] == SOKO_WORKER) || (board[x][y] == SOKO_WORKER2))
	{
		if (game_state == GM_NORMAL) /* Character moves */
		{
			if (try_go_xy(board, x, y, char_x, char_y, path))
			{
			//	printf("%s\n", path);
				replay(path);
			}
		}
		if (game_state == GM_BOX_SEL) /* Box moves */
		{
			game_state = GM_NORMAL;
			board[box_x][box_y] -= (SOKO_SEL - SOKO_BOX);
			invalidate_xy(box_x, box_y);
			if (try_push_xy(board, x, y, path))
			{
		//		printf("%s\n", path);
				replay(path);
			}
		}
	}
	if ((board[x][y] == SOKO_BOX) || (board[x][y] == SOKO_BOX2)
		|| (board[x][y] == SOKO_SEL) || (board[x][y] == SOKO_SEL2))
	{
		if (game_state == GM_NORMAL)
		{
			game_state = GM_BOX_SEL;
			box_x = x;
			box_y = y;
			board[x][y] += (SOKO_SEL - SOKO_BOX);
			invalidate_xy(x, y);
			return TRUE;
		}
		if (game_state == GM_BOX_SEL)
		{
			board[box_x][box_y] -= (SOKO_SEL - SOKO_BOX);
			invalidate_xy(box_x, box_y);
			game_state = GM_NORMAL;
			if ((box_x != x) || (box_y != y))
			{
				board[x][y] += (SOKO_SEL - SOKO_BOX);
				invalidate_xy(x, y);
				game_state = GM_BOX_SEL;
				box_x = x;
				box_y = y;
			}
		}
	}
    return TRUE;
}


void refresh_status (void)
{
	gchar *buff;

	buff = g_strdup_printf ("%s   Best steps: %d, Best pushes: %d,\
	Level: %d, Steps: %d, Pushes: %d", 
		level_result[level_no].solved ? "Solved!" : "Unsolved!",
		level_result[level_no].best_steps,
		level_result[level_no].best_pushes,
		level_no, steps, pushes);
	gtk_label_set_label (GTK_LABEL (label_bar), buff);
	g_free (buff);
}

int main( int   argc,
          char *argv[] )
{
    GtkWidget *window;
    GtkWidget *box1;
    GtkWidget *menubar;

    gtk_init (&argc, &argv);
	
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    gtk_window_set_title (GTK_WINDOW (window), g_locale_to_utf8("�ֿⷬ", -1, NULL, NULL, NULL));
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

    g_signal_connect (G_OBJECT (window), "delete_event",
		      G_CALLBACK (delete_event), NULL);
	g_signal_connect (G_OBJECT(window), "key_press_event",
		G_CALLBACK (key_press_event), NULL);

    gtk_container_set_border_width (GTK_CONTAINER (window), 1);

    box1 = gtk_vbox_new (FALSE, 0);

    gtk_container_add (GTK_CONTAINER (window), box1);

    /* create the menu and put it into box */
    get_main_menu(window, &menubar);
    gtk_box_pack_start(GTK_BOX(box1), menubar, FALSE, TRUE, 0);


	draw_area = gtk_drawing_area_new ();
	gtk_widget_set_size_request (draw_area, W_BOARD * TILE_SIZE, H_BOARD * TILE_SIZE);
	g_signal_connect (G_OBJECT(draw_area), "expose_event",
		G_CALLBACK(expose_event), NULL);
	g_signal_connect (G_OBJECT(draw_area), "button_press_event",
		G_CALLBACK(button_press_event), NULL);
	gtk_widget_set_events (draw_area, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
    gtk_box_pack_start (GTK_BOX(box1), draw_area, TRUE, TRUE, 0);


	/* Status to display numbers of pushs and steps */
	label_bar = gtk_label_new ("Level: 1, Steps: 0, Pushes: 0");
    gtk_box_pack_start (GTK_BOX (box1), label_bar, TRUE, TRUE, 0);

    gtk_widget_show (label_bar);
	gtk_widget_show (draw_area);
	gtk_widget_show (menubar);
    gtk_widget_show (box1);
    gtk_widget_show (window);
    
    /* Rest in gtk_main and wait for the fun to begin! */

	init_gfx ();
	load_steps ();

    gtk_main ();
	
	save_steps ();
    return 0;
}
