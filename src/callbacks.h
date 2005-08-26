/* Procman - callbacks
 * Copyright (C) 2001 Kevin Vandersloot
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 */


#ifndef _PROCMAN_CALLBACKS_H_
#define _PROCMAN_CALLBACKS_H_

#include "procman.h"


void		cb_properties_activate (GtkMenuItem *menuitem, gpointer user_data);

void		cb_show_memory_maps (GtkMenuItem *menuitem, gpointer data);
void		cb_renice (GtkMenuItem *menuitem, gpointer data);
void		cb_add_to_favorites (GtkMenuItem *menuitem, gpointer data);
void		cb_end_process (GtkMenuItem *menuitem, gpointer data);
void		cb_kill_process (GtkMenuItem *menuitem, gpointer data);
void		cb_hide_process (GtkMenuItem *menuitem, gpointer data);
void		cb_show_hidden_processes (GtkMenuItem *menuitem, gpointer data);
void		cb_preferences_activate (GtkMenuItem *menuitem, gpointer user_data);

void		cb_about_activate (GtkMenuItem *menuitem, gpointer user_data);

void		cb_app_exit (GtkObject *object, gpointer user_data);
gboolean	cb_app_delete (GtkWidget *window, GdkEventAny *event, gpointer data);
#if 0
gboolean	cb_close_simple_dialog (GnomeDialog *dialog, gpointer data);
#endif
void		cb_proc_combo_changed (GtkComboBox *combo, gpointer data);

void		cb_end_process_button_pressed (GtkButton *button, gpointer data);
void		cb_logout (GtkButton *button, gpointer data);

void		popup_menu_renice (GtkMenuItem *menuitem, gpointer data);
void		popup_menu_show_memory_maps (GtkMenuItem *menuitem, gpointer data);
void		popup_menu_hide_process (GtkMenuItem *menuitem, gpointer data);
void		popup_menu_end_process (GtkMenuItem *menuitem, gpointer data);
void		popup_menu_kill_process (GtkMenuItem *menuitem, gpointer data);
void		popup_menu_about_process (GtkMenuItem *menuitem, gpointer data);

void		cb_info_button_pressed (GtkButton *button, gpointer user_data);
void		cb_search (GtkEditable *editable, gpointer data);


void		cb_cpu_color_changed (GnomeColorPicker *cp, guint r, guint g, guint b,
				      guint a, gpointer data);
void		cb_mem_color_changed (GnomeColorPicker *cp, guint r, guint g, guint b,
				      guint a, gpointer data);
void		cb_swap_color_changed (GnomeColorPicker *cp, guint r, guint g, guint b,
				      guint a, gpointer data);
void		cb_bg_color_changed (GnomeColorPicker *cp, guint r, guint g, guint b,
				      guint a, gpointer data);
void		cb_frame_color_changed (GnomeColorPicker *cp, guint r, guint g, guint b,
				      guint a, gpointer data);


void		cb_row_selected (GtkTreeSelection *selection, gpointer data);
void		cb_tree_row_activated (GtkTreeView *view, GtkTreePath *path,
				       GtkTreeViewColumn *column, gpointer data);
gboolean	cb_tree_popup_menu (GtkWidget *widget, gpointer data);
gboolean	cb_tree_button_pressed (GtkWidget *widget, GdkEventButton *event,
					gpointer data);

void		cb_switch_page (GtkNotebook *nb, GtkNotebookPage *page,
				gint num, gpointer data);

gint		cb_update_disks (gpointer data);
gint		cb_timeout (gpointer data);


#endif /* _PROCMAN_CALLBACKS_H_ */
