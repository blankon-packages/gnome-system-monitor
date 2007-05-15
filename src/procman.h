/* Procman
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
#ifndef _PROCMAN_PROCMAN_H_
#define _PROCMAN_PROCMAN_H_



#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include <glibtop/cpu.h>

#include <time.h>

#include <map>

typedef struct _ProcConfig ProcConfig;
struct ProcInfo;
struct ProcData;

#include "smooth_refresh.h"

#include "load-graph.h"

#include "prettytable.h"

enum
{
	ALL_PROCESSES,
	MY_PROCESSES,
	ACTIVE_PROCESSES
};


static const unsigned MIN_UPDATE_INTERVAL =   1 * 1000;
static const unsigned MAX_UPDATE_INTERVAL = 100 * 1000;


enum ProcmanTab
{
	PROCMAN_TAB_SYSINFO,
	PROCMAN_TAB_PROCESSES,
	PROCMAN_TAB_RESOURCES,
	PROCMAN_TAB_DISKS
};


struct _ProcConfig
{
	gint		width;
	gint		height;
        gboolean	show_kill_warning;
        gboolean	show_tree;
	gboolean	show_all_fs;
	int		update_interval;
 	int		graph_update_interval;
 	int		disks_update_interval;
	gint		whose_process;
	gint		current_tab;
	GdkColor	cpu_color[GLIBTOP_NCPU];
	GdkColor	mem_color;
	GdkColor	swap_color;
	GdkColor	net_in_color;
	GdkColor	net_out_color;
	GdkColor	bg_color;
	GdkColor	frame_color;
	gint 		num_cpus;
	bool solaris_mode;
};


class ProcInfo
{
	/* undefined */ ProcInfo& operator=(const ProcInfo&);
	/* undefined */ ProcInfo(const ProcInfo&);
 public:

	// TODO: use a set instead
	// sorted by pid. The map has a nice property : it is sorted
	// by pid so this helps a lot when looking for the parent node
	// as ppid is nearly always < pid.
	typedef std::map<pid_t, ProcInfo*> List;
	typedef List::iterator Iterator;

	static List all;

	static ProcInfo* find(pid_t pid);
	static Iterator begin() { return ProcInfo::all.begin(); }
	static Iterator end() { return ProcInfo::all.end(); }


	ProcInfo(pid_t pid);
	~ProcInfo();
	// adds one more ref to icon
	void set_icon(GdkPixbuf *icon);

	GtkTreeIter	node;
	GdkPixbuf	*pixbuf;
	gchar		*tooltip;
	gchar		*name;
	gchar		*user; /* allocated with g_string_chunk, don't free it ! */
	gchar		*arguments;

	gchar		*status; /* shared, don't free it ! */
	gchar		*security_context;

	time_t		start_time;
	guint64		cpu_time_last;

	// all these members are filled with libgtop which uses
	// guint64 (to have fixed size data) but we don't need more
	// than an unsigned long (even for 32bit apps on a 64bit
	// kernel) as these data are amounts, not offsets.
	unsigned long	vmsize;
	unsigned long	memres;
	unsigned long	memwritable;
	unsigned long	memshared;
	unsigned long	mem; /* estimated memory usage */

	// wnck gives an unsigned long
	unsigned long	memxserver;

	const guint	pid;
	guint		ppid;
	guint		uid;

	guint8		pcpu; /* 0% - 100% */
	gint8		nice;
};

struct ProcData
{
	// lazy initialization
	static ProcData* get_instance();

	GtkUIManager	*uimanager;
	GtkActionGroup	*action_group;
	GtkWidget	*statusbar;
	gint		tip_message_cid;
	GtkWidget	*tree;
	GtkWidget	*loadavg;
	GtkWidget	*endprocessbutton;
	GtkWidget	*popup_menu;
	GtkWidget	*disk_list;
	ProcConfig	config;
	LoadGraph	*cpu_graph;
	LoadGraph	*mem_graph;
	LoadGraph	*net_graph;
	gint		cpu_label_fixed_width;
	gint		net_label_fixed_width;
	ProcInfo	*selected_process;
	GtkTreeSelection *selection;
	guint		timeout;
	guint		disk_timeout;

	PrettyTable	pretty_table;

	GConfClient	*client;
	GtkWidget	*app;
	GtkUIManager	*menu;


	/* cached username */
	GStringChunk	*users;

	unsigned	frequency;

	SmoothRefresh  *smooth_refresh;
};

void		procman_save_config (ProcData *data) G_GNUC_INTERNAL;
void		procman_save_tree_state (GConfClient *client, GtkWidget *tree, const gchar *prefix) G_GNUC_INTERNAL;
gboolean	procman_get_tree_state (GConfClient *client, GtkWidget *tree, const gchar *prefix) G_GNUC_INTERNAL;





struct ReniceArgs
{
	ProcData *procdata;
	int nice_value;
};


struct KillArgs
{
	ProcData *procdata;
	int signal;
};

#endif /* _PROCMAN_PROCMAN_H_ */
