/*
 *  gBilling is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation  version 2 of the License.
 *
 *  gBilling is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with gBilling; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  main.c
 *  File ini adalah bagian dari gBilling (http://gbilling.sourceforge.net)
 *  Copyright (C) 2008 - 2010, Ardhan Madras <ajhwb@knac.com>
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef _WIN32
# include <windows.h>
# include <winsock2.h>
#else
# include <sys/stat.h>
#endif

#include <gtk/gtk.h>
#include <glib.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "server.h"
#include "gui.h"
#include "sqldb.h"
#include "sockc.h"
#include "callback.h"
#include "setting.h"
#include "sound.h"

gint
main (gint   argc,
      gchar *argv[])
{
    gint ret;

    ret = gbilling_prepare_dir ();
    if (ret == -1)
    {
        gbilling_sysdebug ("gbilling_prepare_dir");
        return -1;
    }


    ret = read_setting ();
    if (!ret)
    {
        ret = create_setting ();
        if (!ret)
            return -1;
        read_setting ();
    }


    ret = gbilling_lock_file ("gbilling-server.lock");
    if (ret == -1)
    {
#ifndef _WIN32
        if (errno == EAGAIN)
            gbilling_debug ("another instance is currently holding the lock\n");
        else
#endif
        gbilling_sysdebug ("gbilling_lock_file");
        return 0;
    }


    gtk_init (&argc, &argv);
    if (!g_thread_supported ())
        g_thread_init (NULL);


    ret = gbilling_open_db ();
    if (ret > 0)
    {
        gbilling_create_dialog (GTK_MESSAGE_ERROR, NULL,
                                "Versi Database", "Database yang ada tidak dapat "
                                "digunakan untuk versi ini, backup dan hapus file "
                                "database yang lama terlebih dahulu.");
        return -1;
    }
    if (ret == -1)
        return -1;


    gbilling_insert_data ();

    sound_init (&argc, &argv);

    windows_init ();
    create_window_main ();
    set_server_info (window_main->window);
    insert_treev_client ();
    insert_iconv_status ();
    gbilling_log_show ();

    if (setup_server () == -1)
    {
        gtk_widget_destroy (window_init);
        gbilling_create_dialog (GTK_MESSAGE_ERROR, NULL,
                    "Inisialisasi Gagal",  "Server mungkin telah "
                    "aktif atau port sedang digunakan oleh aplikasi lain. "
                    "Jika anda memakai firewall, silahkan atur firewall anda "
                    "untuk memberi akses.");
        gbilling_close_db ();
        return -1;
    }


    if (server->autoactive)
        start_athread ();

    g_thread_create ((GThreadFunc) check_status, NULL, FALSE, NULL);
    g_thread_create ((GThreadFunc) ping_client, NULL, FALSE, NULL);
    g_thread_create ((GThreadFunc) update_status, NULL, FALSE, NULL);

    update_cost_display ((gpointer) 0);
    g_timeout_add (60 * 1000, (GSourceFunc) update_cost_display, NULL);


    if (gbilling_recov_detect ())
    {
        gbilling_debug ("recovery tables is not empty, do recovery...\n");
        do_recovery ();
    }

    gtk_widget_show (window_main->window);
    gtk_main ();

    gbilling_close_db ();
    gbilling_close (server->socket);

    if (ping_pid)
        gbilling_kill (ping_pid);
    gbilling_unlock_file ();

    return 0;
}
