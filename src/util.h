#ifndef H_GNOME_SYSTEM_MONITOR_UTIL_1123178725
#define H_GNOME_SYSTEM_MONITOR_UTIL_1123178725

#include <glib.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <stddef.h>

#define PROCMAN_CMP(X, Y) (((X) == (Y)) ? 0 : (((X) < (Y)) ? -1 : 1))
#define PROCMAN_RCMP(X, Y) PROCMAN_CMP((Y), (X))


GtkWidget*
procman_make_label_for_mmaps_or_ofiles(const char *format,
					     const char *process_name,
					     unsigned pid) G_GNUC_INTERNAL;


gchar*
SI_gnome_vfs_format_file_size_for_display (GnomeVFSFileSize size) G_GNUC_INTERNAL;


gboolean
load_symbols(const char *module, ...) G_GNUC_INTERNAL G_GNUC_NULL_TERMINATED;


void
procman_debug(const char *format, ...) G_GNUC_INTERNAL G_GNUC_PRINTF(1, 2);

#endif /* H_GNOME_SYSTEM_MONITOR_UTIL_1123178725 */
