#include <config.h>

#include <gtkmm.h>

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <gdk/gdkx.h>

#include <glib/gi18n.h>

#include <glibtop.h>
#include <glibtop/cpu.h>
#include <glibtop/mem.h>
#include <glibtop/swap.h>
#include <glibtop/netload.h>
#include <glibtop/netlist.h>

#include <libgnomevfs/gnome-vfs-utils.h>

#include <algorithm>

#include "procman.h"
#include "load-graph.h"
#include "util.h"

#define NUM_POINTS 100
#define GRAPH_MIN_HEIGHT 30


enum {
	CPU_TOTAL,
	CPU_USED,
	N_CPU_STATES
};

struct _LoadGraph {

	guint n;
	gint type;
	guint speed;
	guint draw_width, draw_height;

	GdkColor *colors;

	gfloat* data_block;
	gfloat* data[NUM_POINTS];

	GtkWidget *main_widget;
	GtkWidget *disp;

	cairo_surface_t *buffer;

	guint timer_index;

	gboolean draw;

	LoadGraphLabels labels;

	/* union { */
		struct {
			gboolean initialized;
			guint now; /* 0 -> current, 1 -> last
				    now ^ 1 each time */
			/* times[now], times[now ^ 1] is last */
			guint64 times[2][GLIBTOP_NCPU][N_CPU_STATES];
		} cpu;

		struct {
			guint64 last_in, last_out;
			GTimeVal time;
			unsigned max;
			unsigned values[NUM_POINTS];
			size_t cur;
		} net;
	/* }; */
};



#define FRAME_WIDTH 4


/* Redraws the backing buffer for the load graph and updates the window */
void
load_graph_draw (LoadGraph *g)
{
	const double fontsize = 12.0;
	const double rmargin = 3.5 * fontsize;
	cairo_t *cr;
	int dely;
	double delx;
	int real_draw_height;
	guint i, j;
	unsigned num_bars;

	cr = cairo_create (g->buffer);

	/* clear */
	gdk_cairo_set_source_color (cr, &(g->colors [0]));
	cairo_paint (cr);

	/* draw frame */
	cairo_translate (cr, FRAME_WIDTH, FRAME_WIDTH);
	gdk_cairo_set_source_color (cr, &(g->colors [1]));
	cairo_set_line_width (cr, 1.0);
	cairo_select_font_face(cr,
			       "monospace",
			       CAIRO_FONT_SLANT_NORMAL,
			       CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, fontsize);

	// keep 100 % num_bars == 0
	switch (static_cast<int>(g->draw_height / (fontsize + 3)))
	  {
	  case 1:
	    num_bars = 1;
	    break;
	  case 2:
	  case 3:
	    num_bars = 2;
	    break;
	  case 4:
	    num_bars = 4;
	    break;
	  default:
	    num_bars = 5;
	  }

	dely = g->draw_height / num_bars; /* round to int to avoid AA blur */
	real_draw_height = dely * num_bars;

	for (i = 0; i <= num_bars; ++i) {
		char *caption;
		double y;

		if (i == 0)
		  y = 0.5 + fontsize / 2.0;
		else if (i == num_bars)
		  y = i * dely + 0.5;
		else
		  y = i * dely + fontsize / 2.0;

		cairo_move_to(cr, 0.0, y);
		caption = g_strdup_printf("%3d %%", 100 - i * (100 / num_bars));
		cairo_show_text(cr, caption);
		g_free(caption);

		cairo_move_to (cr, rmargin, i * dely + 0.5);
		cairo_line_to (cr, g->draw_width - 0.5, i * dely + 0.5);
	}

	cairo_move_to (cr, rmargin, 0.5);
	cairo_line_to (cr, rmargin, real_draw_height + 0.5);

	cairo_move_to (cr, g->draw_width - 0.5, 0.5);
	cairo_line_to (cr, g->draw_width - 0.5, real_draw_height + 0.5);

	cairo_stroke (cr);

	/* draw the graph */
	cairo_set_line_width (cr, 2.0);
	cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join (cr, CAIRO_LINE_JOIN_MITER);

	delx = (g->draw_width - 2.0 - rmargin) / (NUM_POINTS - 1);

	for (j = 0; j < g->n; ++j) {
		cairo_move_to (cr,
			       g->draw_width - 2.0,
			       (1.0f - g->data[0][j]) * real_draw_height);
		gdk_cairo_set_source_color (cr, &(g->colors [j + 2]));

		for (i = 1; i < NUM_POINTS; ++i) {
			if (g->data[i][j] == -1.0f)
				continue;

			cairo_curve_to (cr, 
				       (g->draw_width - (i-1) * delx) - (delx/2),
				       (1.0f - g->data[i-1][j]) * real_draw_height,
				       (g->draw_width - i * delx) + (delx/2),
				       (1.0f - g->data[i][j]) * real_draw_height,
				       g->draw_width - i * delx,
				       (1.0f - g->data[i][j]) * real_draw_height);
		}

		cairo_stroke (cr);
	}

	cairo_destroy (cr);

	/* repaint */
	gtk_widget_queue_draw (g->disp);
}

static gboolean
load_graph_configure (GtkWidget *widget,
		      GdkEventConfigure *event,
		      gpointer data_ptr)
{
	LoadGraph * const g = static_cast<LoadGraph*>(data_ptr);
	cairo_t *cr;

	g->draw_width = widget->allocation.width - 2 * FRAME_WIDTH;
	g->draw_height = widget->allocation.height - 2 * FRAME_WIDTH;

	cr = gdk_cairo_create (widget->window);

	if (g->buffer)
		cairo_surface_destroy (g->buffer);

	g->buffer = cairo_surface_create_similar (cairo_get_target (cr),
						  CAIRO_CONTENT_COLOR,
						  widget->allocation.width,
						  widget->allocation.height);

	cairo_destroy (cr);

	load_graph_draw (g);

	return TRUE;
}

static gboolean
load_graph_expose (GtkWidget *widget,
		   GdkEventExpose *event,
		   gpointer data_ptr)
{
	LoadGraph * const g = static_cast<LoadGraph*>(data_ptr);
	cairo_t *cr;

	cr = gdk_cairo_create(widget->window);

	cairo_set_source_surface(cr, g->buffer, 0, 0);
	cairo_rectangle(cr,
			event->area.x, event->area.y,
			event->area.width, event->area.height);
	cairo_fill(cr);

	cairo_destroy(cr);

	return TRUE;
}

static void
get_load (LoadGraph *g)
{
	guint i;
	glibtop_cpu cpu;

	glibtop_get_cpu (&cpu);

#undef NOW
#undef LAST
#define NOW  (g->cpu.times[g->cpu.now])
#define LAST (g->cpu.times[g->cpu.now ^ 1])

	if (g->n == 1) {
		NOW[0][CPU_TOTAL] = cpu.total;
		NOW[0][CPU_USED] = cpu.user + cpu.nice + cpu.sys;
	} else {
		for (i = 0; i < g->n; i++) {
			NOW[i][CPU_TOTAL] = cpu.xcpu_total[i];
			NOW[i][CPU_USED] = cpu.xcpu_user[i] + cpu.xcpu_nice[i]
				+ cpu.xcpu_sys[i];
		}
	}

	if (G_UNLIKELY(!g->cpu.initialized)) {
		/* No data yet */
		g->cpu.initialized = TRUE;
	} else {
		for (i = 0; i < g->n; i++) {
			float load;
			float total, used;
			gchar *text;

			total = NOW[i][CPU_TOTAL] - LAST[i][CPU_TOTAL];
			used  = NOW[i][CPU_USED]  - LAST[i][CPU_USED];

			load = used / MAX(total, 1.0f);
			g->data[0][i] = load;

			/* Update label */
			text = g_strdup_printf("%.1f%%", load * 100.0f);
			gtk_label_set_text(GTK_LABEL(g->labels.cpu[i]), text);
			g_free(text);
		}
	}

	g->cpu.now ^= 1;

#undef NOW
#undef LAST
}


static void
get_memory (LoadGraph *g)
{
	float mempercent, swappercent;
	gchar *text1, *text2, *text3;

	glibtop_mem mem;
	glibtop_swap swap;

	glibtop_get_mem (&mem);
	glibtop_get_swap (&swap);

	/* There's no swap on LiveCD : 0.0f is better than NaN :) */
	swappercent = (swap.total ? (float)swap.used / (float)swap.total : 0.0f);
	mempercent  = (float)mem.user  / (float)mem.total;

	text1 = SI_gnome_vfs_format_file_size_for_display (mem.total);
	text2 = SI_gnome_vfs_format_file_size_for_display (mem.user);
	text3 = g_strdup_printf ("  %.1f %%", mempercent * 100.0f);
	gtk_label_set_text (GTK_LABEL (g->labels.memused), text2);
	gtk_label_set_text (GTK_LABEL (g->labels.memtotal), text1);
	gtk_label_set_text (GTK_LABEL (g->labels.mempercent), text3);
	g_free (text1);
	g_free (text2);
	g_free (text3);

	text1 = SI_gnome_vfs_format_file_size_for_display (swap.total);
	text2 = SI_gnome_vfs_format_file_size_for_display (swap.used);
	text3 = g_strdup_printf ("  %.1f %%", swappercent * 100.0f);
	gtk_label_set_text (GTK_LABEL (g->labels.swapused), text2);
	gtk_label_set_text (GTK_LABEL (g->labels.swaptotal), text1);
	gtk_label_set_text (GTK_LABEL (g->labels.swappercent), text3);
	g_free (text1);
	g_free (text2);
	g_free (text3);

	g->data[0][0] = mempercent;
	g->data[0][1] = swappercent;
}

static void
net_scale (LoadGraph *g, unsigned din, unsigned dout)
{
	g->data[0][0] = 1.0f * din / g->net.max;
	g->data[0][1] = 1.0f * dout / g->net.max;

	unsigned dmax = std::max(din, dout);
	g->net.values[g->net.cur] = dmax;
	g->net.cur = (g->net.cur + 1) % NUM_POINTS;

	unsigned new_max;
	// both way, new_max is the greatest value
	if (dmax >= g->net.max)
		new_max = dmax;
	else
		new_max = *std::max_element(&g->net.values[0],
					    &g->net.values[NUM_POINTS]);

	// round up
	new_max = 11U * new_max / 10U;
	// make sure max is not 0 to avoid / 0
	new_max = std::max(new_max, 1U);

	if (new_max == g->net.max)
		return;

	// if max has decreased but not so much, don't do anything
	// to avoid rescaling
	if ((8U * g->net.max / 10U) < new_max && new_max < g->net.max)
		return;

	const float scale = 1.0f * g->net.max / new_max;

	for (size_t i = 0; i < NUM_POINTS; i++) {
		if (g->data[i][0] >= 0.0f) {
			g->data[i][0] *= scale;
			g->data[i][1] *= scale;
		}
	}

	procman_debug("dmax = %u max = %u new_max = %u", dmax, g->net.max, new_max);

	g->net.max = new_max;
}

static void
get_net (LoadGraph *g)
{
	glibtop_netlist netlist;
	char **ifnames;
	guint32 i;
	guint64 in = 0, out = 0;
	GTimeVal time;
	unsigned din, dout;
	gchar *text1, *text2;

	ifnames = glibtop_get_netlist(&netlist);

	for (i = 0; i < netlist.number; ++i)
	{
		glibtop_netload netload;
		glibtop_get_netload (&netload, ifnames[i]);

		if (netload.if_flags & (1 << GLIBTOP_IF_FLAGS_LOOPBACK))
			continue;

		/* Don't skip interfaces that are down (GLIBTOP_IF_FLAGS_UP)
		   to avoid spikes when they are brought up */

		in  += netload.bytes_in;
		out += netload.bytes_out;
	}

	g_strfreev(ifnames);

	g_get_current_time (&time);

	if (in >= g->net.last_in && out >= g->net.last_out &&
	    g->net.time.tv_sec != 0) {
		float dtime;
		dtime = time.tv_sec - g->net.time.tv_sec +
			(float) (time.tv_usec - g->net.time.tv_usec) / G_USEC_PER_SEC;
		din   = static_cast<unsigned>((in  - g->net.last_in)  / dtime);
		dout  = static_cast<unsigned>((out - g->net.last_out) / dtime);
	} else {
		/* Don't calc anything if new data is less than old (interface
		   removed, counters reset, ...) or if it is the first time */
		din  = 0;
		dout = 0;
	}

	g->net.last_in  = in;
	g->net.last_out = out;
	g->net.time     = time;

	net_scale(g, din, dout);

	text1 = SI_gnome_vfs_format_file_size_for_display (din);
	// xgettext: rate, 10MiB/s
	text2 = g_strdup_printf (_("%s/s"), text1);
	gtk_label_set_text (GTK_LABEL (g->labels.net_in), text2);
	g_free (text1);
	g_free (text2);

	text1 = SI_gnome_vfs_format_file_size_for_display (in);
	gtk_label_set_text (GTK_LABEL (g->labels.net_in_total), text1);
	g_free (text1);

	text1 = SI_gnome_vfs_format_file_size_for_display (dout);
	// xgettext: rate, 10MiB/s
	text2 = g_strdup_printf (_("%s/s"), text1);
	gtk_label_set_text (GTK_LABEL (g->labels.net_out), text2);
	g_free (text1);
	g_free (text2);

	text1 = SI_gnome_vfs_format_file_size_for_display (out);
	gtk_label_set_text (GTK_LABEL (g->labels.net_out_total), text1);
	g_free (text1);
}


/*
  Shifts data right

  data[i+1] = data[i]

  data[i] are float*, so we just move the pointer, not the data.
  But moving data loses data[n-1], so we save data[n-1] and reuse
  it as new data[0]. In fact, we rotate data[].

*/

static void
shift_right(LoadGraph *g)
{
	unsigned i;
	float* last_data;

	/* data[NUM_POINTS - 1] becomes data[0] */
	last_data = g->data[NUM_POINTS - 1];

	/* data[i+1] = data[i] */
	for(i = NUM_POINTS - 1; i != 0; --i)
		g->data[i] = g->data[i-1];

	g->data[0] = last_data;
}

/* Updates the load graph when the timeout expires */
static gboolean
load_graph_update (gpointer user_data)
{
	LoadGraph * const g = static_cast<LoadGraph*>(user_data);

	shift_right(g);

	switch (g->type) {
	case LOAD_GRAPH_CPU:
		get_load(g);
		break;
	case LOAD_GRAPH_MEM:
		get_memory(g);
		break;
	case LOAD_GRAPH_NET:
		get_net(g);
		break;
	default:
		g_assert_not_reached();
	}

	if (g->draw)
		load_graph_draw (g);

	return TRUE;
}

static void
load_graph_unalloc (LoadGraph *g)
{
	g_free(g->data_block);

	if (g->buffer) {
		cairo_surface_destroy (g->buffer);
		g->buffer = NULL;
	}
}

static void
load_graph_alloc (LoadGraph *g)
{
	guint i, j;

	/* Allocate data in a contiguous block */
	g->data_block = g_new(float, g->n * NUM_POINTS);

	for (i = 0; i < NUM_POINTS; ++i) {
		g->data[i] = g->data_block + i * g->n;
		for (j = 0; j < g->n; ++j)
			g->data[i][j] = -1.0f;
	}
}

static gboolean
load_graph_destroy (GtkWidget *widget, gpointer data_ptr)
{
	LoadGraph * const g = static_cast<LoadGraph*>(data_ptr);

	load_graph_stop (g);

	if (g->timer_index)
		g_source_remove (g->timer_index);

	load_graph_unalloc(g);

	return FALSE;
}

LoadGraph *
load_graph_new (gint type, ProcData *procdata)
{
	LoadGraph *g;
	guint i = 0;

	g = g_new0 (LoadGraph, 1);

	g->type = type;
	switch (type) {
	case LOAD_GRAPH_CPU:
		if (procdata->config.num_cpus == 0)
			g->n = 1;
		else
			g->n = procdata->config.num_cpus;

		for(i = 0; i < G_N_ELEMENTS(g->labels.cpu); ++i)
			g->labels.cpu[i] = gtk_label_new(NULL);

		break;

	case LOAD_GRAPH_MEM:
		g->n = 2;
		g->labels.memused = gtk_label_new(NULL);
		g->labels.memtotal = gtk_label_new(NULL);
		g->labels.mempercent = gtk_label_new(NULL);
		g->labels.swapused = gtk_label_new(NULL);
		g->labels.swaptotal = gtk_label_new(NULL);
		g->labels.swappercent = gtk_label_new(NULL);
		break;

	case LOAD_GRAPH_NET:
		g->n = 2;
		g->net.max = 1;
		g->labels.net_in = gtk_label_new(NULL);
		g->labels.net_in_total = gtk_label_new(NULL);
		g->labels.net_out = gtk_label_new(NULL);
		g->labels.net_out_total = gtk_label_new(NULL);
		break;
	}

	g->speed  = procdata->config.graph_update_interval;

	g->colors = g_new0 (GdkColor, g->n + 2);

	g->colors[0] = procdata->config.bg_color;
	g->colors[1] = procdata->config.frame_color;
	switch (type) {
	case LOAD_GRAPH_CPU:
		memcpy(g->colors + 2, procdata->config.cpu_color,
		       g->n * sizeof g->colors[0]);
		break;
	case LOAD_GRAPH_MEM:
		g->colors[2] = procdata->config.mem_color;
		g->colors[3] = procdata->config.swap_color;
		break;
	case LOAD_GRAPH_NET:
		g->colors[2] = procdata->config.net_in_color;
		g->colors[3] = procdata->config.net_out_color;
		break;
	}

	g->timer_index = 0;

	g->draw = FALSE;

	g->main_widget = gtk_vbox_new (FALSE, FALSE);
	gtk_widget_set_size_request(g->main_widget, -1, GRAPH_MIN_HEIGHT);
	gtk_widget_show (g->main_widget);

	g->disp = gtk_drawing_area_new ();
	gtk_widget_show (g->disp);
	g_signal_connect (G_OBJECT (g->disp), "expose_event",
			  G_CALLBACK (load_graph_expose), g);
	g_signal_connect (G_OBJECT(g->disp), "configure_event",
			  G_CALLBACK (load_graph_configure), g);
	g_signal_connect (G_OBJECT(g->disp), "destroy",
			  G_CALLBACK (load_graph_destroy), g);

	gtk_widget_set_events (g->disp, GDK_EXPOSURE_MASK);

	gtk_box_pack_start (GTK_BOX (g->main_widget), g->disp, TRUE, TRUE, 0);

	load_graph_alloc (g);

	gtk_widget_show_all (g->main_widget);

	load_graph_start(g);
	load_graph_stop(g);

	return g;
}

void
load_graph_start (LoadGraph *g)
{
	if(!g->timer_index) {

		load_graph_update(g);

		g->timer_index = g_timeout_add (g->speed,
						load_graph_update,
						g);
	}

	g->draw = TRUE;
}

void
load_graph_stop (LoadGraph *g)
{
	/* don't draw anymore, but continue to poll */
	g->draw = FALSE;
}

void
load_graph_change_speed (LoadGraph *g,
			 guint new_speed)
{
	if (g->speed == new_speed)
		return;

	g->speed = new_speed;

	g_assert(g->timer_index);

	if(g->timer_index) {
		g_source_remove (g->timer_index);
		g->timer_index = g_timeout_add (g->speed,
						load_graph_update,
						g);
	}
}

GdkColor*
load_graph_get_colors (LoadGraph *g)
{
	return g->colors;
}


LoadGraphLabels*
load_graph_get_labels (LoadGraph *g)
{
	return &g->labels;
}

GtkWidget*
load_graph_get_widget (LoadGraph *g)
{
	return g->main_widget;
}
