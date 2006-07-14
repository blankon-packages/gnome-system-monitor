/* Procman - ability to show favorite processes
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
 
#ifndef _FAVORITES_H_
#define _FAVORITES_H_


#include <gconf/gconf-client.h>
#include <glib/gtypes.h>

#include "procman.h"

void 		save_blacklist (ProcData *procdata, GConfClient *client) G_GNUC_INTERNAL;
void		add_to_blacklist (ProcData *procdata, gchar *name) G_GNUC_INTERNAL;
void		add_selected_to_blacklist (ProcData *procdata) G_GNUC_INTERNAL;
void		remove_from_blacklist (ProcData *procdata, gchar *name) G_GNUC_INTERNAL;

gboolean	is_process_blacklisted (ProcData *procdata, gchar *name) G_GNUC_INTERNAL;

void 		get_blacklist (ProcData *procdata, GConfClient *client) G_GNUC_INTERNAL;

void		create_blacklist_dialog (ProcData *procdata) G_GNUC_INTERNAL;

#endif
