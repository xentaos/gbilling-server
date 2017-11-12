/*
 *  gBilling is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation version 2 of the License.
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
 *  callback.c
 *  File ini adalah bagian dari gBilling (http://gbilling.sourceforge.net)
 *  Copyright (C) 2008 - 2010, Ardhan Madras <ajhwb@knac.com>
 */

#ifdef _WIN32
# include <winsock2.h>
#else
# include <arpa/inet.h>
# include <gdk/gdkx.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gstdio.h>

#include <string.h>
#include <stdlib.h>

#include "main.h"
#include "gbilling.h"
#include "gui.h"
#include "callback.h"
#include "server.h"
#include "sqldb.h"
#include "setting.h"
#include "sound.h"

typedef struct
{
    gint id;
    gint cmd;
} ServerCommand;

#ifdef _WIN32
static gboolean window_iconify = FALSE;
#endif

static ClientData* get_client_in_row (void);
static void foreach_remove_treev_log_row (GtkTreeRowReference*, GtkTreeModel*);

#define ADMIN_AUTH(title)                                               \
({                                                                      \
    GbillingAuth login;                                                 \
    gint run;                                                           \
    gboolean retval = FALSE;                                            \
                                                                        \
    run = create_dialog_auth ((GtkWindow *) window_main->window,        \
                              title, &login);                           \
    do {                                                                \
        if (run != GTK_RESPONSE_OK)                                     \
            break;                                                      \
        if (!strlen (login.username) || !strlen (login.passwd))         \
            break;                                                      \
        if (!gbilling_auth_compare (server->sauth, &login))             \
            gbilling_create_dialog (GTK_MESSAGE_WARNING,                \
                    (GtkWindow *) window_main->window,                  \
                    "Login Gagal", "Silahkan periksa informasi login "  \
                    "yang dimasukkan.");                                \
        else                                                            \
            retval = TRUE;                                              \
    } while (0);                                                        \
    retval;                                                             \
})

/* Fungsi sort kolum 'date' log, ini dicapai dengan cara konversi
   nilai kolum 'date' dan kolum 'start' ke format time_t */
gint
column_sort_func (GtkTreeModel *model,
                  GtkTreeIter  *a,
                  GtkTreeIter  *b,
                  gpointer      data)
{
    gchar *str1, *str2, *start1, *start2;
    time_t t1, t2;

    gtk_tree_model_get (model, a, SLOG_DATE, &str1, -1);
    gtk_tree_model_get (model, a, SLOG_START, &start1, -1);
    gtk_tree_model_get (model, b, SLOG_DATE, &str2, -1);
    gtk_tree_model_get (model, b, SLOG_START, &start2, -1);
    t1 = date_to_time_t (str1) + string_to_sec (start1);
    t2 = date_to_time_t (str2) + string_to_sec (start2);
    g_free (str1);
    g_free (str2);
    g_free (start1);
    g_free (start2);
    return (t1 - t2);
}

/* Cari client yang dipilih di treeview, pointer menuju ke data list client */
static ClientData*
get_client_in_row (void)
{
    GtkTreeModel *model;
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    GList *ptr;
    ClientData *cdata = NULL;
    gchar *name;

    selection = gtk_tree_view_get_selection ((GtkTreeView *) window_main->treeview_client);
    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
        return cdata;
    gtk_tree_model_get (model, &iter, CLIENT_NAME, &name, -1);
    for (ptr = client_list; ptr; ptr = g_list_next (ptr))
    {
        cdata = ptr->data;
        if (!strcmp (name, cdata->name))
            break;
    }
    g_free (name);
    g_return_val_if_fail (ptr != NULL, NULL);
    return cdata;
}

void
on_window_init_destroy (GtkWidget *widget,
                        gpointer   data)
{
    guint source;
    source = GPOINTER_TO_INT (data);
    g_source_remove (source);
}

gboolean
on_window_main_delete_event (GtkWidget *widget,
                             GdkEvent  *event,
                             gpointer   data)
{
    gint ret;

    gtk_window_deiconify ((GtkWindow *) window_main->window);
    ret = gbilling_create_dialog (GTK_MESSAGE_QUESTION, window_main->window, "Keluar",
                    "Server akan di-nonaktifkan, keluar sekarang juga?");
    if (ret != GTK_RESPONSE_OK)
        return TRUE;

    if (!ADMIN_AUTH ("Keluar dari Server"))
        return TRUE;

    gtk_main_quit (); /* TODO: atexit() and kill zombies! */
    return FALSE;
}

/* di win32, maximize kembali window jika
 * sebelumnya telah di minimize
 */
#ifdef _WIN32
gboolean
on_window_main_state_event (GtkWidget           *widget,
                            GdkEventWindowState *event,
                            gpointer             data)
{
    if (event->new_window_state == GDK_WINDOW_STATE_MAXIMIZED)
    {
        if (window_iconify)
            gtk_window_maximize ((GtkWindow *) widget);
        window_iconify = TRUE;
    }
    else if (event->new_window_state == GDK_WINDOW_STATE_ICONIFIED)
        window_iconify = FALSE;

    return FALSE;
}
#endif

void
on_selection_client_changed (GtkTreeSelection *selection,
                             gpointer          data)
{
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    gchar *name;

    selection = gtk_tree_view_get_selection ((GtkTreeView *) window_main->treeview_client);
    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
        return;
    gtk_tree_model_get (model, &iter, CLIENT_NAME, &name, -1);
    set_markup ((GtkLabel *) window_main->label_client, GBILLING_FONT_X_LARGE, TRUE, name);
    g_free (name);
    /* Sinkron dengan status icon */
    if ((path = gtk_tree_model_get_path (model, &iter)))
    {
        gtk_icon_view_set_cursor ((GtkIconView *) window_main->iconview_status, 
                                  path, NULL, FALSE);
        gtk_icon_view_select_path ((GtkIconView *) window_main->iconview_status, path);
        gtk_icon_view_scroll_to_path ((GtkIconView *) window_main->iconview_status, 
                                      path, FALSE, 0, 0);
        gtk_tree_path_free (path);
    }
}

/*
 * FIXME: Untuk sementara ditiadakan, fungsi ini selalu membuat Segmentation fault 
 * dari fungsi gtk_tree_selection_select_path().
 */
void
on_iconv_status_selection_changed (GtkIconView *iconv,
                                   gpointer     data)
{
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    GtkTreeSelection *selection;
    GList *ptr;
    ClientData *cdata = NULL;
    gchar *name;
    gboolean status;
    gint page;

    /* Jangan proses jika di PAGE_CLIENT */
    page = gtk_notebook_get_current_page ((GtkNotebook *) window_main->notebook_client);
    if (page == PAGE_CLIENT)
        return;
    if ((status = gtk_icon_view_get_cursor (iconv, &path, NULL)))
    {
        model = gtk_icon_view_get_model ((GtkIconView *) window_main->iconview_status);
        gtk_tree_model_get_iter (model, &iter, path);
        gtk_tree_model_get (model, &iter, 0, &name, -1);
        for (ptr = client_list; ptr; ptr = ptr->next)
        {
            cdata = ptr->data;
            if (!strcmp (cdata->name, name))
                break;
        }
        gtk_tree_path_free (path);
        g_free (name);
        g_return_if_fail (cdata != NULL);

        /* Sinkron dengan treeview client */
        selection = gtk_tree_view_get_selection ((GtkTreeView *) window_main->treeview_client);
        model = gtk_tree_view_get_model ((GtkTreeView *) window_main->treeview_client);
        path = gtk_tree_model_get_path (model, &cdata->iter);
        gtk_tree_selection_select_path (selection, path);

        gtk_tree_view_scroll_to_cell ((GtkTreeView *) window_main->treeview_client, 
                                      path, NULL, FALSE, 0, 0);
        gtk_tree_view_set_cursor ((GtkTreeView *) window_main->treeview_client, 
                                  path, NULL, FALSE);
        gtk_tree_path_free (path);
    }

    gtk_widget_set_sensitive (window_main->button_chat, status);
    gtk_widget_set_sensitive (window_main->button_logout, status);
    gtk_widget_set_sensitive (window_main->button_restart, status);
    gtk_widget_set_sensitive (window_main->button_shutdown, status);
    gtk_widget_set_sensitive (window_main->button_move, status);
    gtk_widget_set_sensitive (window_main->button_edit, status);
    gtk_widget_set_sensitive (window_main->button_enable, status);
    gtk_widget_set_sensitive (window_main->button_disable, status);
}

void
on_selection_log_changed (GtkTreeSelection *selection,
                          gpointer          data)
{

}

void
on_noteb_control_switch_page (GtkNotebook     *noteb,
                              GtkNotebookPage *page,
                              gint             num,
                              gpointer         data)
{

}

void
on_menu_setting_server_activated (GtkMenuItem *menuitem,
                                  gpointer     data)
{
    if (!ADMIN_AUTH ("Pengaturan Server"))
        return;

    GtkTreeModel *model;
    GtkTreeIter iter;
    gint i;
    gchar *file;
    static const gchar * const events[] =
    {
        "Server aktif",
        "Server nonaktif",
        "Client aktif",
        "Client nonaktif",
        "Login administrasi",
        "Login reguler",
        "Login paket",
        "Login member",
        "Logout",
        "Terima pesan chat",
        "Kirim pesan chat"
    };

    create_window_setting_server ();
    gtk_spin_button_set_value ((GtkSpinButton *) 
                window_setting_server->spinb_port, server->port);
    gtk_toggle_button_set_active ((GtkToggleButton *) 
                window_setting_server->checkb_autoactive, server->autoactive);

    gtk_entry_set_text ((GtkEntry *) window_setting_server->entry_susername, 
                        server->sauth->username);
    gtk_entry_set_text ((GtkEntry *) window_setting_server->entry_spasswd, 
                        server->sauth->passwd);
    gtk_entry_set_text ((GtkEntry *) window_setting_server->entry_scpasswd, 
                        server->sauth->passwd);

    gtk_widget_set_sensitive (window_setting_server->label_spasswd, FALSE);
    gtk_widget_set_sensitive (window_setting_server->entry_spasswd, FALSE);
    gtk_widget_set_sensitive (window_setting_server->label_scpasswd, FALSE);
    gtk_widget_set_sensitive (window_setting_server->entry_scpasswd, FALSE);
    gtk_widget_set_sensitive (window_setting_server->button_ssave, FALSE);

    gtk_entry_set_text ((GtkEntry *) window_setting_server->entry_cusername, 
                        server->cauth->username);
    gtk_entry_set_text ((GtkEntry *) window_setting_server->entry_cpasswd, 
                        server->cauth->passwd);
    gtk_entry_set_text ((GtkEntry *) window_setting_server->entry_ccpasswd, 
                        server->cauth->passwd);
                        
    gtk_widget_set_sensitive (window_setting_server->label_cpasswd, FALSE);
    gtk_widget_set_sensitive (window_setting_server->entry_cpasswd, FALSE);
    gtk_widget_set_sensitive (window_setting_server->label_ccpasswd, FALSE);
    gtk_widget_set_sensitive (window_setting_server->entry_ccpasswd, FALSE);
    gtk_widget_set_sensitive (window_setting_server->button_csave, FALSE);

    gtk_entry_set_text ((GtkEntry *) window_setting_server->entry_name, 
                        server_info->name);
    gtk_entry_set_text ((GtkEntry *) window_setting_server->entry_desc, 
                        server_info->desc);
    gtk_entry_set_text ((GtkEntry *) window_setting_server->entry_address, 
                        server_info->addr);
    
    gtk_toggle_button_set_active ((GtkToggleButton *)
               window_setting_server->checkb_name, server->display_name);
    gtk_toggle_button_set_active ((GtkToggleButton *) 
               window_setting_server->checkb_desc, server->display_desc);
    gtk_toggle_button_set_active ((GtkToggleButton *)
               window_setting_server->checkb_address, server->display_addr);
    gtk_toggle_button_set_active ((GtkToggleButton *)
               window_setting_server->checkb_logo, server->display_logo);
    if (server->logo)
    {
        file = g_path_get_basename (server->logo);
        gtk_entry_set_text ((GtkEntry *) window_setting_server->entry_logo, file);
        g_free (file);
    }

    model = gtk_tree_view_get_model ((GtkTreeView *) window_setting_server->treeview_sound);

    for (i = 0; i < G_N_ELEMENTS (events); i++)
    {
        gtk_list_store_append ((GtkListStore *) model, &iter);
        gtk_list_store_set ((GtkListStore *) model, &iter, 
                            0, current_soundopt[i].play, 1, events[i], -1);
    }

    gtk_widget_show (window_setting_server->window);
}

void
on_menu_setting_log_activated (GtkMenuItem *menuitem,
                               gpointer     data)
{
    if (!ADMIN_AUTH ("Pengaturan Log"))
        return;

    GbillingAuth *login;
    glong show, del;
    gint s, d;
    
    show = gbilling_server_get_log_show ();
    del = gbilling_server_get_log_deltime ();
    login = gbilling_server_get_log_auth ();

    create_window_setting_log ();
    
    /* atur @spinb_show_window_setting_log sesuai @show */
    if (show >= 0 && show < 3600) /* untuk menit, kurang dari 1 jam */
    {
        gtk_combo_box_set_active ((GtkComboBox *) window_setting_log->spinb_show, 0);
        s = (gint) show / 60;
    }
    else if (show >= 3600 && show < 24 * 3600)
    {
        gtk_combo_box_set_active ((GtkComboBox *) window_setting_log->combob_show, 1);
        s = (gint) show / (3600);
    }
    else
    {
        gtk_combo_box_set_active ((GtkComboBox *) window_setting_log->combob_show, 2);
        s = (gint) show / (24 * 3600);
    }
    gtk_spin_button_set_value ((GtkSpinButton *) window_setting_log->spinb_show, s);
    /* atur @checkb_del_window_setting_log @del */

    gtk_toggle_button_set_active ((GtkToggleButton *) window_setting_log->checkb_del, del);
    gtk_widget_set_sensitive (window_setting_log->label_del, del);
    gtk_widget_set_sensitive (window_setting_log->spinb_del, del);
    gtk_widget_set_sensitive (window_setting_log->combob_del, del);
    if (del >= 0 && del < 3600)
    {
        gtk_combo_box_set_active ((GtkComboBox *) window_setting_log->combob_del, 0);
        d = (gint) del / 60;
    }
    else if (del >= 3600 && del < 24 * 3600)
    {
        gtk_combo_box_set_active ((GtkComboBox *) window_setting_log->combob_del, 1);
        d = (gint) del / (3600);
    }
    else
    {
        gtk_combo_box_set_active ((GtkComboBox *) window_setting_log->combob_del, 2);
        d = (gint) del / (24 * 3600);
    }
    gtk_spin_button_set_value ((GtkSpinButton *) window_setting_log->spinb_del, d);

    gtk_entry_set_text ((GtkEntry *) window_setting_log->entry_name, login->username);
    gtk_entry_set_text ((GtkEntry *) window_setting_log->entry_passwd, login->passwd);
    gtk_entry_set_text ((GtkEntry *) window_setting_log->entry_passwdc, login->passwd);
    gtk_widget_set_sensitive (window_setting_log->label_passwd, FALSE);
    gtk_widget_set_sensitive (window_setting_log->entry_passwd, FALSE);
    gtk_widget_set_sensitive (window_setting_log->label_passwdc, FALSE);
    gtk_widget_set_sensitive (window_setting_log->entry_passwdc, FALSE);
    gtk_widget_set_sensitive (window_setting_log->button_save, FALSE);
    gtk_widget_show (window_setting_log->window);
    gbilling_auth_free (login);
}

void
on_menu_setting_mute_toggled (GtkCheckMenuItem *menuitem,
                              gpointer          data)
{
    server->mute_sound = gtk_check_menu_item_get_active (menuitem);
}

void
on_menu_setting_client_activated (GtkWidget *widget,
                                  gpointer   data)
{
    if (!ADMIN_AUTH ("Pengaturan Client"))
        return;

    ClientData *cdata;
    GList *ptr;
    GtkTreeModel *model;
    GtkTreeIter iter;

    create_window_setting_client ();
    model = (GtkTreeModel *) window_setting_client->store;
    ptr = client_list;
    while (ptr)
    {
        cdata = ptr->data;
        gtk_list_store_append ((GtkListStore *) model, &iter);
        gtk_list_store_set ((GtkListStore *) model, &iter,
                            SCLIENT_NAME, cdata->name,
                            SCLIENT_IP, cdata->ip,
                            -1);
        ptr = g_list_next (ptr);
    }

    gtk_toggle_button_set_active ((GtkToggleButton *) 
                window_setting_client->checkb_disable, server->autodisable);
    gtk_toggle_button_set_active ((GtkToggleButton *)
                window_setting_client->checkb_restart, server->autorestart);

    gtk_widget_show (window_setting_client->window);
}

void
on_menu_setting_cost_activate (GtkWidget *widget,
                               gpointer   data)
{
    if (!ADMIN_AUTH ("Pengaturan Tarif"))
        return;

    GtkTreeModel *model;
    GtkTreeIter iter;
    GbillingCostList *ptr;
    GbillingCost *cost;
    GbillingCostRotationList *rptr;
    GbillingCostRotation *rotation;
    gchar *interval;
    glong tcost;

    create_window_setting_cost ();
    model = (GtkTreeModel *) window_setting_cost->store;
    ptr = cost_list;
    while (ptr)
    {
        cost = ptr->data;
        gtk_list_store_append ((GtkListStore *) model, &iter);
        tcost = cost->fcost + ((60 - cost->fmin) / cost->imin * cost->icost);
        gtk_list_store_set ((GtkListStore *) model, &iter,
                            SCCOST_NAME, cost->name,
                            SCCOST_MODE, (cost->mode == GBILLING_COST_MODE_FMIN) ?
                                          "Menit Pertama" : "Flat",
                            SCCOST_COST, tcost,
                            SCCOST_DEFAULT, cost->def,
                            -1);
        ptr = g_list_next (ptr);
    }

    model = gtk_tree_view_get_model ((GtkTreeView *) window_setting_cost->treeview_rotation);
    rptr = costrotation_list;
    while (rptr)
    {
        rotation = rptr->data;
        cost = get_cost (rotation->cost);
        g_return_if_fail (cost != NULL);

        interval = g_strdup_printf ("%.2i:%.2i - %.2i:%.2i", rotation->shour,
                                    rotation->smin, rotation->ehour, rotation->emin);
        gtk_list_store_append ((GtkListStore *) model, &iter);
        gtk_list_store_set ((GtkListStore *) model, &iter,
                            SCOSTROTATION_NAME, cost->name,
                            SCOSTROTATION_INTERVAL, interval,
                            SCOSTROTATION_STATE, rotation->state,
                            -1);
        g_free (interval);
        rptr = g_list_next (rptr);
    }

    gtk_widget_show (window_setting_cost->window);
}

void
on_menu_setting_prepaid_activate (GtkWidget *menuitem,
                                  gpointer   data)
{
    if (!ADMIN_AUTH ("Pengaturan Paket"))
        return;

    GbillingPrepaidList *ptr;
    GbillingPrepaid *prepaid;
    GtkTreeModel *model;
    GtkTreeIter iter;

    create_window_setting_prepaid ();
    ptr = prepaid_list;
    if (ptr)
    {
        model = gtk_tree_view_get_model ((GtkTreeView *) window_setting_prepaid->treeview);
        while (ptr)
        {
            prepaid = ptr->data;
            ptr = ptr->next;
            gtk_list_store_append ((GtkListStore *) model, &iter);
            gtk_list_store_set ((GtkListStore *) model, &iter,
                                SCPAK_NAME, prepaid->name,
                                SCPAK_DURATION, (gint) prepaid->duration / 60,
                                SCPAK_COST, prepaid->cost,
                                SCPAK_ACTIVE, prepaid->active,
                                -1);
        }
    }
    gtk_widget_show (window_setting_prepaid->window);
}

void
on_menu_help_about_activate (GtkWidget *widget,
                             gpointer   data)
{
    gbilling_show_about_window ((GtkWindow *) window_main->window);
}

static void
set_toggleb_signal (gboolean block)
{
    void (*cb) (gpointer, gulong);

    cb = block ? g_signal_handler_block : g_signal_handler_unblock;

    cb (window_main->toggleb_client, window_main->client_handlerid);
    cb (window_main->toggleb_status, window_main->status_handlerid);
    cb (window_main->toggleb_log, window_main->log_handlerid);
}

static gint active_page = PAGE_CLIENT;

void
on_toggleb_client_toggled (GtkToggleButton *toggleb,
                           gpointer         data)
{
    gboolean status = gtk_toggle_button_get_active (toggleb);

    if (active_page == PAGE_CLIENT && !status)
    {
        gtk_toggle_button_set_active (toggleb, TRUE);
        return;
    }

    set_toggleb_signal (TRUE);
    gtk_toggle_button_set_active ((GtkToggleButton *) window_main->toggleb_status, FALSE);
    gtk_toggle_button_set_active ((GtkToggleButton *) window_main->toggleb_log, FALSE);
    gtk_toggle_button_set_active ((GtkToggleButton *) window_main->toggleb_client, TRUE);
    set_toggleb_signal (FALSE);
    gtk_notebook_set_current_page ((GtkNotebook *) window_main->notebook_client, PAGE_CLIENT);
    active_page = PAGE_CLIENT;
}

void
on_toggleb_status_toggled (GtkToggleButton *toggleb,
                           gpointer         data)
{
    gboolean status = gtk_toggle_button_get_active (toggleb);

    if (active_page == PAGE_STATUS && !status)
    {
        gtk_toggle_button_set_active (toggleb, TRUE);
        return;
    }

    set_toggleb_signal (TRUE);
    gtk_toggle_button_set_active ((GtkToggleButton *) window_main->toggleb_client, FALSE);
    gtk_toggle_button_set_active ((GtkToggleButton *) window_main->toggleb_log, FALSE);
    gtk_toggle_button_set_active ((GtkToggleButton *) window_main->toggleb_status, TRUE);
    set_toggleb_signal (FALSE);
    gtk_notebook_set_current_page ((GtkNotebook *) window_main->notebook_client, PAGE_STATUS);
    active_page = PAGE_STATUS;
}

void
on_toggleb_log_toggled (GtkToggleButton *toggleb,
                        gpointer         data)
{
    gboolean status = gtk_toggle_button_get_active (toggleb);
    
    if (active_page == PAGE_LOG && !status)
    {
        gtk_toggle_button_set_active (toggleb, TRUE);
        return;
    }

    set_toggleb_signal (TRUE);
    gtk_toggle_button_set_active ((GtkToggleButton *) window_main->toggleb_client, FALSE);
    gtk_toggle_button_set_active ((GtkToggleButton *) window_main->toggleb_status, FALSE);
    gtk_toggle_button_set_active ((GtkToggleButton *) window_main->toggleb_log, TRUE);
    set_toggleb_signal (FALSE);
    gtk_notebook_set_current_page ((GtkNotebook *) window_main->notebook_client, PAGE_LOG);
    active_page = PAGE_LOG;
}

void
start_athread (void)
{
    server->active = TRUE;
    server->athread = g_thread_create ((GThreadFunc) accept_client, NULL, TRUE, NULL);

    gtk_widget_set_sensitive (window_main->menu_server_activate, FALSE);
    gtk_widget_set_sensitive (window_main->menu_server_deactivate, TRUE);
    set_statusbar ("Aktif");
}

gboolean
stop_athread (gpointer data)
{
    server->active = FALSE;
    g_thread_join (server->athread);
    gtk_widget_set_sensitive (window_main->menu_server_activate, TRUE);
    gtk_widget_set_sensitive (window_main->menu_server_deactivate, FALSE);
    set_statusbar ("Nonaktif");

    return FALSE;
}

void
on_toolb_connect_clicked (GtkToolButton *toolb,
                          gpointer       data)
{
    if (!ADMIN_AUTH ("Aktifasi Server"))
        return;

    start_athread ();
}

void
on_toolb_dconnect_clicked (GtkToolButton *toolb,
                           gpointer       data)
{
    if (!ADMIN_AUTH ("Nonaktifasi Server"))
        return;

    stop_athread (NULL);
}

void
on_toolb_sitem_clicked (GtkToolButton *toolb,
                        gpointer       data)
{
    if (!ADMIN_AUTH ("Pengaturan Item"))
        return;

    GbillingItemList *ptr;
    GbillingItem *item;
    GtkTreeModel *model;
    GtkTreeIter iter;

    create_window_setting_item ();
    if ((ptr = item_list))
    {
        model = gtk_tree_view_get_model ((GtkTreeView *) window_setting_item->treeview);
        while (ptr)
        {
            item = ptr->data;
            ptr = g_list_next (ptr);
            gtk_list_store_append ((GtkListStore *) model, &iter);
            gtk_list_store_set ((GtkListStore *) model, &iter,
                                SCITEM_NAME, item->name,
                                SCITEM_COST, item->cost,
                                SCITEM_ACTIVE, item->active,
                                -1);
        }
    }
    gtk_widget_show (window_setting_item->window);
}

void
on_noteb_client_switch_page (GtkNotebook     *noteb,
                             GtkNotebookPage *page,
                             gint             num,
                             gpointer         data)
{
    gboolean a = FALSE, b = FALSE;

    if (num == PAGE_CLIENT || num == PAGE_STATUS)
    {
        gtk_notebook_set_current_page ((GtkNotebook *) 
                                       window_main->notebook_menu, MENU_CLIENT);
        a = TRUE, b = !a;
    }
    else if (num == PAGE_LOG)
    {
        gtk_notebook_set_current_page ((GtkNotebook *) 
                                       window_main->notebook_menu, MENU_LOG);
        a = FALSE, b = !a;
    }
    else
        return;
    gtk_widget_set_sensitive (window_main->button_chat, a);
    gtk_widget_set_sensitive (window_main->button_logout, a);
    gtk_widget_set_sensitive (window_main->button_restart, a);
    gtk_widget_set_sensitive (window_main->button_shutdown, a);
    gtk_widget_set_sensitive (window_main->button_move, a);
    gtk_widget_set_sensitive (window_main->button_edit, a);
    gtk_widget_set_sensitive (window_main->button_enable, a);
    gtk_widget_set_sensitive (window_main->button_disable, a);
    gtk_widget_set_sensitive (window_main->button_filter, b);
    gtk_widget_set_sensitive (window_main->button_detail, b);
    gtk_widget_set_sensitive (window_main->button_delete, b);
    gtk_widget_set_sensitive (window_main->button_total, b);
    gtk_widget_set_sensitive (window_main->button_export, b);
    gtk_widget_set_sensitive (window_main->button_refresh, b);
}

void
on_button_cchat_clicked (GtkWidget *widget,
                         gpointer   data)
{
    ClientData *cdata;

    if (!(cdata = get_client_in_row ()))
        return;
    /* client harus aktif dan sementara login */
    if (!cdata->active)
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, (GtkWindow *) window_main->window, 
                "Chat", "Client sedang tidak aktif, tidak dapat "
                "chat dengan client.");
        return;
    }
    if (cdata->status != GBILLING_STATUS_LOGIN)
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, (GtkWindow *) window_main->window,
                "Chat", "Client sedang tidak login, tidak dapat "
                "chat dengan client.");
        return;
    }
    if (!cdata->wchat)
    {
        cdata->wchat = create_window_chat (cdata);
        gtk_widget_show (cdata->wchat->window);
    }
    else
        gtk_window_present ((GtkWindow *) cdata->wchat->window);
    /*
     * window chat client di "destroy" pada saat client logout
     * lihat do_client_logout() di server.c
     */
}

void
on_button_clogout_clicked (GtkWidget *widget,
                           gpointer   data)
{
    ClientData *cdata;
    gint ret;

    if (!(cdata = get_client_in_row ()))
        return;
    if (cdata->status != GBILLING_STATUS_LOGIN)
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, (GtkWindow *) window_main->window, 
                "Logout", "Client sedang tidak login, tidak dapat logout client.",
                cdata->name);
        return;
    }
    ret = gbilling_create_dialog (GTK_MESSAGE_QUESTION, (GtkWindow *) window_main->window, 
               "Logout", "Client sedang login, logout client sekarang juga?");
    if (ret == GTK_RESPONSE_OK)
    {
        /* manual, hanya jika client tidak aktif */
        if (cdata->active)
            set_servcmd (cdata, GBILLING_COMMAND_LOGOUT, NULL);
        else
            do_client_logout (cdata);
    }
}

void
on_button_crestart_clicked (GtkWidget *widget,
                            gpointer   data)
{
    ClientData *cdata;
    gint ret;

    if (!(cdata = get_client_in_row ()))
        return;
    if (!cdata->active)
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, (GtkWidget *) window_main->window, 
                "Restart", "Client sedang tidak aktif, tidak dapat restart client.");
        return;
    }
    if (cdata->status == GBILLING_STATUS_LOGIN)
        ret = gbilling_create_dialog (GTK_MESSAGE_QUESTION, (GtkWindow *) window_main->window, 
                    "Restart", "Client sedang login, restart client sekarang juga?");
    else    
        ret = gbilling_create_dialog (GTK_MESSAGE_QUESTION, (GtkWindow *) window_main->window, 
                    "Restart", "Restart client sekarang juga?");
    if (ret == GTK_RESPONSE_OK)
        set_servcmd (cdata, GBILLING_COMMAND_RESTART, NULL);
}

void
on_button_cshutdown_clicked (GtkWidget *widget,
                             gpointer   data)
{
    ClientData *cdata;
    gint ret;

    if (!(cdata = get_client_in_row ()))
        return;
    if (!cdata->active)
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, (GtkWindow *) window_main->window, 
                "Shutdown", "Client sedang tidak aktif, tidak dapat shutdown client.");
        return;
    }
    if (cdata->status == GBILLING_STATUS_LOGIN)
        ret = gbilling_create_dialog (GTK_MESSAGE_QUESTION, (GtkWindow *) window_main->window, 
                    "Shutdown", "Client sedang login, shutdown client sekarang juga?");
    else
        ret = gbilling_create_dialog (GTK_MESSAGE_QUESTION, (GtkWindow *) window_main->window, 
                    "Shutdown", "Shutdown client sekarang juga?");
    if (ret == GTK_RESPONSE_OK)
        set_servcmd (cdata, GBILLING_COMMAND_SHUTDOWN, NULL);
}

void
on_button_cmove_clicked (GtkWidget *widget,
                         gpointer   data)
{
    GList *ptr;
    ClientData *tmp, *cdata;
    gchar *msg;

    if (!(cdata = get_client_in_row ()))
        return;
    if (cdata->status != GBILLING_STATUS_LOGIN)
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, window_main->window, "Pindah",
                "Client sedang tidak login, tidak dapat memindahkan client.");
        return;
    }
    create_window_move ();
    for (ptr = client_list; ptr; ptr = ptr->next)
    {
        tmp = ptr->data;
        if (strcmp (cdata->name, tmp->name))
            gtk_combo_box_append_text ((GtkComboBox *) 
                                       window_move->combob_client, tmp->name);
    }
    gtk_combo_box_set_active ((GtkComboBox *) window_move->combob_client, 0);
    msg = g_strdup_printf ("Pindahkan client <b>%s</b> ke:", cdata->name);
    gtk_label_set_markup ((GtkLabel *) window_move->label_move, msg);
    gtk_widget_show (window_move->window);
    g_free (msg);
}

void
on_button_cedit_clicked (GtkWidget *widget,
                         gpointer   data)
{
    ClientData *cdata;
    gchar *tmp, *ucost;

    if (!(cdata = get_client_in_row ()))
        return;
    if (cdata->status != GBILLING_STATUS_LOGIN)
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, window_main->window, "Edit",
            "Client sedang tidak login, tidak dapat mengedit tarif client.");
        return;
    }

    create_window_edit ();
    gtk_label_set_text ((GtkLabel *) window_edit->label_client, cdata->name);
    ucost = g_strdup_printf ("Rp. %li", cdata->ucost);
    tmp = g_strdup_printf ("Rp. %li", cdata->ucost + cdata->acost);
    gtk_label_set_text ((GtkLabel *) window_edit->label_use, ucost);
    gtk_spin_button_set_value ((GtkSpinButton *) window_edit->spinb_add, cdata->acost);
    if (cdata->desc)
        gtk_entry_set_text ((GtkEntry *) window_edit->entry_info, cdata->desc);
    gtk_label_set_text ((GtkLabel *) window_edit->label_total, tmp);
    gtk_widget_show (window_edit->window);
    g_free (tmp);
    g_free (ucost);
}

void
on_button_cenable_clicked (GtkWidget *widget,
                           gpointer   data)
{
    ClientData *cdata;

    if (!(cdata = get_client_in_row ()))
        return;
    /* client harus aktif dan tidak sedang di buka */
    if (!cdata->active)
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, (GtkWindow *) window_main->window, 
                "Buka Akses", "Client sedang tidak aktif, tidak dapat "
                "membuka akses client.");
        return;
    }
    if (!cdata->access)
        set_servcmd (cdata, GBILLING_COMMAND_ENABLE, NULL);
}

void
on_button_cdisable_clicked (GtkWidget *widget,
                            gpointer   data)
{
    ClientData *cdata;

    if (!(cdata = get_client_in_row ()))
        return;
    if (!cdata->active)
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, (GtkWindow *) window_main->window, 
                "Tutup Akses", "Client sedang tidak aktif, tidak dapat "
                "menutup akses client.");
        return;
    }
    if (cdata->access)
        set_servcmd (cdata, GBILLING_COMMAND_DISABLE, NULL);
}

/* 
 * Window pencarian log tidak di `destroy', ini untuk mengingat 
 * nilai yang telah diset.
 */
void
on_button_lfilter_clicked (GtkWidget *widget,
                           gpointer   data)
{
    GtkTreeModel *model;
    GList *clist = client_list;
    ClientData *cdata;
    GbillingPrepaidList *plist = prepaid_list;
    GbillingPrepaid *prepaid;
    gchar *tmp;
    gint prev_index = 0;
    time_t t = time (NULL);

    if (!window_search)
    {
        create_window_search ();
        gtk_toggle_button_set_active ((GtkToggleButton *) 
                                       window_search->checkb_client, 
                                       clist != NULL);
        gtk_toggle_button_set_active ((GtkToggleButton *) 
                                      window_search->checkb_date_start, TRUE);
        gtk_widget_set_sensitive (window_search->toggleb_date_end, FALSE);
        tmp = time_t_to_date (t);
        gtk_button_set_label ((GtkButton *) window_search->toggleb_date_start, tmp);
        gtk_button_set_label ((GtkButton *) window_search->toggleb_date_end, tmp);
        g_free (tmp);
        gtk_toggle_button_set_active ((GtkToggleButton *)
                                      window_search->checkb_username, TRUE);
        gtk_toggle_button_set_active ((GtkToggleButton *)
                                      window_search->checkb_username, FALSE);
        gtk_toggle_button_set_active ((GtkToggleButton *) 
                                      window_search->checkb_type, TRUE);
        gtk_toggle_button_set_active ((GtkToggleButton *) 
                                      window_search->checkb_time_start, TRUE);
        gtk_toggle_button_set_active ((GtkToggleButton *) 
                                      window_search->checkb_time_start, FALSE);
        gtk_toggle_button_set_active ((GtkToggleButton *)
                                      window_search->checkb_cost_start, TRUE);
        gtk_toggle_button_set_active ((GtkToggleButton *)
                                      window_search->checkb_cost_start, FALSE);
        tmp = time_t_to_string (t);
        gtk_button_set_label ((GtkButton *) window_search->toggleb_time_start, tmp);
        gtk_button_set_label ((GtkButton *) window_search->toggleb_time_end, tmp);
        g_free (tmp);
        gtk_toggle_button_set_active ((GtkToggleButton *)
                                      window_search->checkb_cost_start, FALSE);
    }

    /* Selalu update list client dan prepaid */
    if (clist)
    {
        prev_index = gtk_combo_box_get_active ((GtkComboBox *) window_search->combob_client);
        model = gtk_combo_box_get_model ((GtkComboBox *) window_search->combob_client);
        gtk_list_store_clear ((GtkListStore *) model);
    }
    while (clist)
    {
        cdata = (ClientData *) clist->data;
        gtk_combo_box_append_text ((GtkComboBox *) 
                                   window_search->combob_client, cdata->name);
        clist = g_list_next (clist);
    }
    if (client_list)
        gtk_combo_box_set_active ((GtkComboBox *) window_search->combob_client, 
                                   prev_index == -1 ? 0 : prev_index);

    if (plist)
        prev_index = gtk_combo_box_get_active ((GtkComboBox *) window_search->combob_type);
    else
        prev_index = 1;

    model = gtk_combo_box_get_model ((GtkComboBox *) window_search->combob_type);
    gtk_list_store_clear ((GtkListStore *) model);
    gtk_combo_box_prepend_text ((GtkComboBox *) window_search->combob_type, "Reguler");
    gtk_combo_box_prepend_text ((GtkComboBox *) window_search->combob_type, "Admin");

    while (plist)
    {
        prepaid = (GbillingPrepaid *) plist->data;
        gtk_combo_box_append_text ((GtkComboBox *) 
                                    window_search->combob_type, prepaid->name);
        plist = g_list_next (plist);
    }
    gtk_combo_box_set_active ((GtkComboBox *) window_search->combob_type, prev_index);

    gtk_widget_show (window_search->window);
}

void
on_button_ldetail_clicked (GtkWidget *widget,
                           gpointer   data)
{
    GbillingLog *log;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar *tdate, *client, *tstart, *tmp;
    time_t t, dur;

    /* set ke single selection */
    gtk_tree_selection_set_mode (window_main->log_selection, GTK_SELECTION_SINGLE);
    if (!gtk_tree_selection_get_selected (window_main->log_selection, &model, &iter))
    {
        gtk_tree_selection_set_mode (window_main->log_selection, GTK_SELECTION_MULTIPLE);
        return;
    }

    gtk_tree_model_get (model, &iter,
                        SLOG_DATE, &tdate,
                        SLOG_CLIENT, &client,
                        SLOG_START, &tstart,
                        -1);

    t = date_to_time_t (tdate) + string_to_sec (tstart);
    log = gbilling_get_log (client, t);
    g_free (tdate);
    g_free (tstart);
    g_free (client);

    if (log == NULL)
    {
        gtk_tree_selection_set_mode (window_main->log_selection, GTK_SELECTION_MULTIPLE);
        g_return_if_fail (log != NULL);
    }

    create_window_detail ();
    gtk_entry_set_text ((GtkEntry *) window_detail->entry_client, log->client);
    gtk_entry_set_text ((GtkEntry *) window_detail->entry_username, log->username);
    gtk_entry_set_text ((GtkEntry *) window_detail->entry_type, log->type);
    
    tdate = time_t_to_date (log->tstart);
    tstart = time_t_to_string (log->tstart);
    tmp = g_strdup_printf ("%s  %s", tdate, tstart);
    gtk_entry_set_text ((GtkEntry *) window_detail->entry_start, tmp);
    g_free (tdate);
    g_free (tstart);
    g_free (tmp);

    if (log->tend)
    {
        tdate = time_t_to_date (log->tend);
        tstart = time_t_to_string (log->tend);
        tmp = g_strdup_printf ("%s  %s", tdate, tstart);
        g_free (tdate);
        g_free (tstart);
    }
    else
        tmp = g_strdup ("-");
    gtk_entry_set_text ((GtkEntry *) window_detail->entry_end, tmp);
    g_free (tmp);

    if (log->tend)
    {
        dur = log->tend - log->tstart;
        tmp = time_to_string (dur);
    }
    else
        tmp = g_strdup ("-");
    gtk_entry_set_text ((GtkEntry *) window_detail->entry_duration, tmp);
    g_free (tmp);
    
    tmp = g_strdup_printf ("%li", log->ucost);
    gtk_entry_set_text ((GtkEntry *) window_detail->entry_usage, tmp);
    g_free (tmp);
    
    tmp = g_strdup_printf ("%li", log->acost);
    gtk_entry_set_text ((GtkEntry *) window_detail->entry_add, tmp);
    g_free (tmp);

    gtk_entry_set_text ((GtkEntry *) window_detail->entry_desc, 
                        log->desc ? log->desc : "-");

    tmp = g_strdup_printf ("%li", log->acost + log->ucost);
    gtk_entry_set_text ((GtkEntry *) window_detail->entry_total, tmp);
    g_free (tmp);

    gtk_widget_show (window_detail->window);
    /* kembalikan lagi mode selection */
    gtk_tree_selection_set_mode (window_main->log_selection, GTK_SELECTION_MULTIPLE);
    gbilling_log_free (log);
}

void
on_button_ltotal_clicked (GtkWidget *widget,
                          gpointer   data)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    gboolean viter;
    gchar *date, *start, *sdur, *scost, *tmp, *time1, *time2;
    gint i;
    glong dur = 0, cost = 0;
    time_t max_time = 0, min_time = G_MAXLONG, t;

    model = gtk_tree_view_get_model ((GtkTreeView *) window_main->treeview_log);
    viter = gtk_tree_model_get_iter_first (model, &iter);
    if (!viter) return;

    for (i = 0; viter; i++)
    {
        gtk_tree_model_get (model, &iter,
                            SLOG_DATE, &date,
                            SLOG_START, &start,
                            SLOG_DURATION, &sdur,
                            SLOG_COST, &scost,
                            -1);
        t = date_to_time_t (date) + string_to_sec (start);
        max_time = MAX (t, max_time);
        min_time = MIN (t, min_time);
        dur += g_utf8_collate (sdur, "-") ? string_to_sec (sdur) : 0;
        cost += atol (scost);
        g_free (date);
        g_free (start);
        g_free (sdur);
        g_free (scost);
        viter = gtk_tree_model_iter_next (model, &iter);
    }

    create_window_total ();

    date = time_t_to_date (min_time);
    start = time_t_to_string (min_time);
    time1 = g_strdup_printf ("%s %s", date, start);
    gtk_entry_set_text ((GtkEntry *) window_total->entry_first, time1);
    g_free (date);
    g_free (start);
    g_free (time1);

    date = time_t_to_date (max_time);
    start = time_t_to_string (max_time);
    time2 = g_strdup_printf ("%s %s", date, start);
    gtk_entry_set_text ((GtkEntry *) window_total->entry_last, time2);
    g_free (date);
    g_free (start);
    g_free (time2);

    tmp = g_strdup_printf ("%i", i);
    gtk_entry_set_text ((GtkEntry *) window_total->entry_usage, tmp);
    g_free (tmp);
    tmp = time_to_string (dur);
    gtk_entry_set_text ((GtkEntry *) window_total->entry_duration, tmp);
    g_free (tmp);
    tmp = g_strdup_printf ("%li", cost);
    gtk_entry_set_text ((GtkEntry *) window_total->entry_cost, tmp);
    g_free (tmp);
    gtk_widget_show (window_total->window);
}

static void
foreach_remove_treev_log_row (GtkTreeRowReference *reference,
                              GtkTreeModel        *model)
{
    const ClientData *cdata;
    GtkTreePath *path;
    GtkTreeIter iter;
    gchar *client, *date, *start;
    time_t t;

    path = gtk_tree_row_reference_get_path (reference);
    gtk_tree_model_get_iter (model, &iter, path);
    gtk_tree_model_get (model, &iter,
                        SLOG_CLIENT, &client,
                        SLOG_DATE, &date,
                        SLOG_START, &start,
                        -1);
    t = date_to_time_t (date) + string_to_sec (start);
    /* periksa jika client sedang login */
    cdata = get_client_by_name (client);
    if (!cdata)
        gbilling_debug ("%s: client '%s' is not exist\n", __func__, client);
    else
    {
        if (cdata->status == GBILLING_STATUS_LOGIN && cdata->tstart == t)
        {
            gbilling_create_dialog (GTK_MESSAGE_WARNING, (GtkWindow *) window_main->window, 
                        "Hapus Log", "Client <b>%s</b> sedang login, tidak dapat "
                        "menghapus log sebelum pemakaian selesai.", client);
            gtk_tree_path_free (path);
            g_free (client);
            g_free (date);
            g_free (start);
            return;
        }
    }

    gbilling_delete_log (client, t);
    gtk_list_store_remove ((GtkListStore *) model, &iter);
    gtk_tree_path_free (path);
    g_free (client);
    g_free (date);
    g_free (start);
}

void
on_button_ldel_clicked (GtkWidget *widget,
                        gpointer   data)
{
    GtkTreeSelection *selection;
    GtkTreeRowReference	*reference;
    GtkTreeModel *model;
    GList *ref = NULL, *row = NULL, *ptr = NULL;
    GbillingAuth *auth, input;
    gint ret;

    selection = gtk_tree_view_get_selection ((GtkTreeView *) window_main->treeview_log);
    model = gtk_tree_view_get_model ((GtkTreeView *) window_main->treeview_log);
    if (!(row = gtk_tree_selection_get_selected_rows (selection, &model)))
        return;

    /* konfirmasi */
    ret = gbilling_create_dialog (GTK_MESSAGE_QUESTION, (GtkWindow *) window_main->window,
                "Hapus Log", "Hapus log yang dipilih dari server?");
    if (ret != GTK_RESPONSE_OK)
    {
        g_list_foreach (row, (GFunc) gtk_tree_path_free, NULL);
        g_list_free (row);
        return;
    }

    ret = create_dialog_auth ((GtkWindow *) window_main->window,
                              "Keamanan Log", &input);
    if (ret != GTK_RESPONSE_OK)
        return;

    auth = gbilling_server_get_log_auth ();
    ret = gbilling_auth_compare (auth, &input);
    gbilling_auth_free (auth);

    if (ret == FALSE)
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, 
                                (GtkWindow *) window_main->window,
                                "Login Gagal", "Silahkan periksa kembali "
                                "informasi login yang dimasukkan.");
        return;
    }

    ptr = row;
    while (ptr)
    {
        reference = gtk_tree_row_reference_new (model, (GtkTreePath *) ptr->data);
        ref = g_list_prepend (ref, gtk_tree_row_reference_copy (reference));
        gtk_tree_row_reference_free (reference);
        ptr = ptr->next;
    }
    g_list_foreach (ref, (GFunc) foreach_remove_treev_log_row, model);
    g_list_foreach (ref, (GFunc) gtk_tree_row_reference_free, NULL);
    g_list_foreach (row, (GFunc) gtk_tree_path_free, NULL);
    g_list_free (ref);
    g_list_free (row);
}

void
on_button_lexport_clicked (GtkWidget *widget,
                           gpointer   data)
{
    GbillingLog *log;
    GtkWidget *dialog;
    GtkFileFilter *filter1, *filter2;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GIOChannel *write;
    gchar *client, *date, *ltime, *tstart, *tend, *duration;
    gchar *ucost, *acost, *tcost;
    gchar *file, *tmp;
    gboolean viter;
    gint c;
    static gint idx = 1;
    time_t t, now;
    struct tm *st;

    model = gtk_tree_view_get_model ((GtkTreeView *) window_main->treeview_log);
    if ((viter = gtk_tree_model_get_iter_first (model, &iter)) == FALSE)
        return;
    dialog = gbilling_file_dialog ("Ekspor Log...", (GtkWindow *) window_main->window,
                    GTK_FILE_CHOOSER_ACTION_SAVE);
    now = time (NULL);
    st = localtime (&now);
    tmp = g_strdup_printf ("log-%.2i-%.2i-%i_%i.csv", st->tm_mday,
                st->tm_mon + 1, st->tm_year + 1900, idx);
    gtk_file_chooser_set_current_name ((GtkFileChooser *) dialog, tmp);
    g_free (tmp);
    filter1 = gtk_file_filter_new ();
    filter2 = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter1, "CSV (*.CSV)");
    gtk_file_filter_add_pattern (filter1, "*.csv");
    gtk_file_filter_set_name (filter2, "Semua File");
    gtk_file_filter_add_pattern (filter2, "*.*");
    gtk_file_chooser_add_filter ((GtkFileChooser *) dialog, filter1);
    gtk_file_chooser_add_filter ((GtkFileChooser *) dialog, filter2);
    if (gtk_dialog_run ((GtkDialog *) dialog) != GTK_RESPONSE_OK)
    {
        gtk_widget_destroy (dialog);
        return;
    }
    file = gtk_file_chooser_get_filename ((GtkFileChooser *) dialog);
    gtk_widget_destroy (dialog);
    /* tanya jika file telah ada */
    if (g_file_test (file, G_FILE_TEST_EXISTS))
    {
        tmp = g_path_get_basename (file);
        c = gbilling_create_dialog (GTK_MESSAGE_QUESTION, 
                        (GtkWindow *) window_main->window,
                        "Ekspor Log", "File dengan nama <b>%s</b> telah ada, "
                        "tulis ulang file ini?", tmp);
        g_free (tmp);
        if (c != GTK_RESPONSE_OK)
        {
            g_free (file);
            return;
        }
    }
    write = g_io_channel_new_file (file, "w", NULL);
    if (!write)
    {
        tmp = g_path_get_basename (file);
        gbilling_create_dialog (GTK_MESSAGE_WARNING, (GtkWindow *) window_main->window, 
                  "Ekspor Log", "File <b>%s</b> tidak dapat ditulis, periksa "
                  "kembali permisi anda dan pastikan file tidak sedang dibuka.", tmp);
        g_free (tmp);
        g_free (file);
        return;
    }
    g_free (file);
    /* CSV format, comma separated. tulis kolum... enjoy the formatting ;p */
    tmp = g_strdup_printf ("CLIENT,USERNAME,TIPE,MULAI,SELESAI,DURASI,\"TARIF PAKAI\","
                           "\"TARIF TAMBAHAN\",\"TARIF TOTAL\",KETERANGAN\n");
    g_io_channel_write_chars (write, tmp, -1, NULL, NULL);
    g_free (tmp);
    while (viter)
    {
        gtk_tree_model_get (model, &iter,
                            SLOG_DATE, &date,
                            SLOG_CLIENT, &client,
                            SLOG_START, &tstart,
                            -1);

        t = date_to_time_t (date) + string_to_sec (tstart);
        g_free (date);
        g_free (tstart);
        log = gbilling_get_log (client, t);
        if (!log)
        {
            gbilling_debug ("%s: could not find log for '%s' at %li\n", __func__, client, t);
            g_free (client);
            viter = gtk_tree_model_iter_next (model, &iter);
            continue;
        }
        g_free (client);

        date = time_t_to_date (log->tstart);
        ltime = time_t_to_string (log->tstart);
        tstart = g_strdup_printf ("%s %s", date, ltime);
        g_free (date);
        g_free (ltime);

        date = time_t_to_date (log->tend);
        ltime = time_t_to_string (log->tend);
        tend = g_strdup_printf ("%s %s", date, ltime);
        g_free (date);
        g_free (ltime);

        duration = time_to_string (log->tend - log->tstart);
        ucost = g_strdup_printf ("%li", log->ucost);
        acost = g_strdup_printf ("%li", log->acost);
        tcost = g_strdup_printf ("%li", log->ucost + log->acost);

        tmp = g_strdup_printf ("\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",%s,%s,%s,%s,\"%s\"\n", 
                               log->client, log->username, log->type, tstart,
                               tend, duration, ucost, acost, tcost, log->desc);
        g_io_channel_write_chars (write, tmp, -1, NULL, NULL);
        g_free (tmp);
        g_free (tstart);
        g_free (tend);
        g_free (duration);
        g_free (ucost);
        g_free (acost);
        g_free (tcost);
        gbilling_log_free (log);

        viter = gtk_tree_model_iter_next (model, &iter);
    } /* while */
    g_io_channel_shutdown (write, TRUE, NULL);
    g_io_channel_unref (write);
    idx++;
}

void
on_button_lrefresh_clicked (GtkWidget *widget,
                            gpointer   data)
{
    GtkTreeModel *model;

    model = gtk_tree_view_get_model ((GtkTreeView *) window_main->treeview_log);
    gtk_list_store_clear ((GtkListStore *) model);
    gbilling_log_show ();
}

gboolean
on_window_setting_server_delete_event (GtkWidget *widget,
                                       GdkEvent  *event,
                                       gpointer   data)
{
    gtk_widget_destroy (window_setting_server->window);
    g_free (window_setting_server);
    return FALSE;
}

void
on_button_logo_window_setting_server_clicked (GtkWidget *widget,
                                              gpointer   data)
{
    GtkWidget *dialog;
    GtkFileFilter *filter1, *filter2, *filter3, *filter4;
    gchar *file, *tmp, *logo_dir;

    dialog = gbilling_file_dialog ("Pilih gambar...", 
                                   window_setting_server->window,
                                   GTK_FILE_CHOOSER_ACTION_OPEN);
    filter1 = gtk_file_filter_new ();
    filter2 = gtk_file_filter_new ();
    filter3 = gtk_file_filter_new ();
    filter4 = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter1, "Semua Gambar");
    gtk_file_filter_set_name (filter2, "PNG (*.PNG)");
    gtk_file_filter_set_name (filter3, "JPG (*.JPG;*.JPEG;*.JPE)");
    gtk_file_filter_set_name (filter4, "BMP (*.BMP)");
    gtk_file_filter_add_pattern (filter1, "*.png");
    gtk_file_filter_add_pattern (filter1, "*.jpg");
    gtk_file_filter_add_pattern (filter1, "*.jpeg");
    gtk_file_filter_add_pattern (filter1, "*.jpe");
    gtk_file_filter_add_pattern (filter1, "*.bmp");
    gtk_file_filter_add_pattern (filter2, "*.png");
    gtk_file_filter_add_pattern (filter3, "*.jpg");
    gtk_file_filter_add_pattern (filter3, "*.jpeg");
    gtk_file_filter_add_pattern (filter3, "*.jpe");
    gtk_file_filter_add_pattern (filter4, "*.bmp");
    gtk_file_chooser_add_filter ((GtkFileChooser *) dialog, filter1);
    gtk_file_chooser_add_filter ((GtkFileChooser *) dialog, filter2);
    gtk_file_chooser_add_filter ((GtkFileChooser *) dialog, filter3);
    gtk_file_chooser_add_filter ((GtkFileChooser *) dialog, filter4);

    if (server->logo && g_file_test (server->logo, G_FILE_TEST_EXISTS))
        logo_dir = g_path_get_dirname (server->logo);
    else
        logo_dir = g_strdup (g_get_home_dir ());
    gtk_file_chooser_set_current_folder ((GtkFileChooser *) dialog, logo_dir);
    if (gtk_dialog_run ((GtkDialog *) dialog) != GTK_RESPONSE_OK)
    {
        gtk_widget_destroy (dialog);
        return;
    }

    file = gtk_file_chooser_get_filename ((GtkFileChooser *) dialog);
    if (server->logo)
        g_free (server->logo);
    server->logo = g_strdup (file);
    tmp = g_path_get_basename (file);
    gtk_entry_set_text ((GtkEntry *) window_setting_server->entry_logo, tmp);
    gtk_widget_destroy (dialog);
    g_free (file);
    g_free (tmp);
    g_free (logo_dir);
}

void
on_entry_suser_window_setting_server_changed (GtkEditable *editable,
                                              gpointer     data)
{
    gtk_entry_set_text ((GtkEntry *) window_setting_server->entry_spasswd, "");
    gtk_widget_set_sensitive (window_setting_server->label_spasswd, TRUE);
    gtk_widget_set_sensitive (window_setting_server->entry_spasswd, TRUE);
    gtk_widget_set_sensitive (window_setting_server->label_scpasswd, FALSE);
    gtk_widget_set_sensitive (window_setting_server->entry_scpasswd, FALSE);
    gtk_widget_set_sensitive (window_setting_server->button_ssave, FALSE);
}

void
on_entry_spasswd_window_setting_server_changed (GtkEditable *editable,
                                                gpointer     data)
{
    gtk_entry_set_text ((GtkEntry *) window_setting_server->entry_scpasswd, "");
    gtk_widget_set_sensitive (window_setting_server->label_scpasswd, TRUE);
    gtk_widget_set_sensitive (window_setting_server->entry_scpasswd, TRUE);
    gtk_widget_set_sensitive (window_setting_server->button_ssave, FALSE);
}

void
on_entry_spasswdc_window_setting_server_changed (GtkEditable *editable,
                                                 gpointer     data)
{
    gtk_widget_set_sensitive (window_setting_server->button_ssave, TRUE);
}

void
on_button_sset_window_setting_server_clicked (GtkWidget *widget,
                                              gpointer   data)
{
    GbillingAuth *auth;
    const gchar *user, *pass1, *pass2;

    user = gtk_entry_get_text ((GtkEntry *) window_setting_server->entry_susername);
    pass1 = gtk_entry_get_text ((GtkEntry *) window_setting_server->entry_spasswd);
    pass2 = gtk_entry_get_text ((GtkEntry *) window_setting_server->entry_scpasswd);

    if (!strlen (user) || !strlen (pass1))
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, window_setting_server->window,
                "Login Server", "Silahkan periksa kembali username dan password "
                "yang dimasukkan.");
        return;
    }
    if (!strcmp (user, pass1))
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, window_setting_server->window,
                "Login Server", "Username dan password tidak boleh sama.");
        return;
    }
    if (strcmp (pass1, pass2))
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, window_setting_server->window,
            "Login Server", "Password tidak sama dengan konfirmasi password.");
        return;
    }

    auth = gbilling_auth_new (user, pass1);
    gbilling_server_set_server_login (auth);
    g_static_mutex_lock (&server_mutex);
    if (server->sauth)
        gbilling_auth_free (server->sauth);
    server->sauth = g_memdup (auth, sizeof(GbillingAuth));
    g_static_mutex_unlock (&server_mutex);
    gbilling_auth_free (auth);
    gtk_widget_set_sensitive (window_setting_server->label_spasswd, FALSE);
    gtk_widget_set_sensitive (window_setting_server->entry_spasswd, FALSE);
    gtk_widget_set_sensitive (window_setting_server->label_scpasswd, FALSE);
    gtk_widget_set_sensitive (window_setting_server->entry_scpasswd, FALSE);
    gtk_widget_set_sensitive (window_setting_server->button_ssave, FALSE);
}

void
on_entry_cuser_window_setting_server_changed (GtkEditable *editable,
                                              gpointer     data)
{
    gtk_entry_set_text ((GtkEntry *) window_setting_server->entry_cpasswd, "");
    gtk_widget_set_sensitive (window_setting_server->label_cpasswd, TRUE);
    gtk_widget_set_sensitive (window_setting_server->entry_cpasswd, TRUE);
    gtk_widget_set_sensitive (window_setting_server->label_ccpasswd, FALSE);
    gtk_widget_set_sensitive (window_setting_server->entry_ccpasswd, FALSE);
    gtk_widget_set_sensitive (window_setting_server->button_csave, FALSE);
}

void
on_entry_cpasswd_window_setting_server_changed (GtkEditable *editable,
                                                gpointer     data)
{
    gtk_entry_set_text ((GtkEntry *) window_setting_server->entry_ccpasswd, "");
    gtk_widget_set_sensitive (window_setting_server->label_ccpasswd, TRUE);
    gtk_widget_set_sensitive (window_setting_server->entry_ccpasswd, TRUE);
    gtk_widget_set_sensitive (window_setting_server->button_csave, FALSE);
}

void
on_entry_cpasswdc_window_setting_server_changed (GtkEditable *editable,
                                                 gpointer     data)
{
    gtk_widget_set_sensitive (window_setting_server->button_csave, TRUE);
}

void
on_button_cset_window_setting_server_clicked (GtkWidget *widget,
                                              gpointer   data)
{
    GbillingAuth *auth;
    const gchar *user, *pass1, *pass2;

    user = gtk_entry_get_text ((GtkEntry *) window_setting_server->entry_cusername);
    pass1 = gtk_entry_get_text ((GtkEntry *) window_setting_server->entry_cpasswd);
    pass2 = gtk_entry_get_text ((GtkEntry *) window_setting_server->entry_ccpasswd);

    if (!strlen (user) || !strlen (pass1))
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, 
                (GtkWindow *) window_setting_server->window,
                "Keamanan Client", "Silahkan periksa kembali username "
                "dan password yang dimasukkan.");
        return;
    }

    if (!strcmp (user, pass1))
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, 
                (GtkWindow *) window_setting_server->window,
                "Keamanan Client", "Username dan password tidak boleh sama.");
        return;
    }

    if (strcmp (pass1, pass2))
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, 
            (GtkWindow *) window_setting_server->window,
            "Keamanan Client", "Password tidak sama dengan konfirmasi password.");
        return;
    }

    auth = gbilling_auth_new (user, pass1);
    gbilling_server_set_client_auth (auth);
    g_static_mutex_lock (&server_mutex);
    if (server->cauth)
        gbilling_auth_free (server->cauth);
    server->cauth = g_memdup (auth, sizeof(GbillingAuth));
    g_static_mutex_unlock (&server_mutex);
    gbilling_auth_free (auth);
    
    gtk_widget_set_sensitive (window_setting_server->label_cpasswd, FALSE);
    gtk_widget_set_sensitive (window_setting_server->entry_cpasswd, FALSE);
    gtk_widget_set_sensitive (window_setting_server->label_ccpasswd, FALSE);
    gtk_widget_set_sensitive (window_setting_server->entry_ccpasswd, FALSE);
    gtk_widget_set_sensitive (window_setting_server->button_csave, FALSE);
}

gboolean 
on_textv_addr_window_setting_server_key_press_event (GtkWidget   *widget, 
                                                     GdkEventKey *event, 
                                                     gpointer     data)
{
    return (event->keyval == GDK_Return || event->keyval == GDK_KP_Enter);
}

gboolean
on_window_setting_client_delete_event (GtkWidget *widget,
                                       GdkEvent  *event,
                                       gpointer   data)
{
    server->autodisable = gtk_toggle_button_get_active ((GtkToggleButton *)
                                    window_setting_client->checkb_disable);
    server->autorestart = gtk_toggle_button_get_active ((GtkToggleButton *)
                                    window_setting_client->checkb_restart);

    write_setting ();

    gtk_widget_destroy (window_setting_client->window);
    g_free (window_setting_client);
    return FALSE;
}

void
on_combob_show_window_setting_log_changed (GtkComboBox *combob,
                                           gpointer     data)
{
    gint i;
    
    i = gtk_combo_box_get_active (combob);
    if (i == 0) /* menit */
        gtk_spin_button_set_range ((GtkSpinButton *) window_setting_log->spinb_show, 1, 59);
    else if (i == 1) /* jam */
        gtk_spin_button_set_range ((GtkSpinButton *) window_setting_log->spinb_show, 1, 23);
    else /* hari */
        gtk_spin_button_set_range ((GtkSpinButton *) window_setting_log->spinb_show, 1, 30);
}

void
on_combob_del_window_setting_log_changed (GtkComboBox *combob,
                                          gpointer     data)
{
    gint i;

    i = gtk_combo_box_get_active ((GtkComboBox *) combob);
    if (i == 0) /* menit */
        gtk_spin_button_set_range ((GtkSpinButton *) window_setting_log->spinb_del, 1, 59);
    else if (i == 1) /* jam */
        gtk_spin_button_set_range ((GtkSpinButton *) window_setting_log->spinb_del, 1, 23);
    else /* hari */
        gtk_spin_button_set_range ((GtkSpinButton *) window_setting_log->spinb_del, 1, 30);
}

void
on_checkb_del_window_setting_log_toggled (GtkToggleButton *toggleb,
                                          gpointer         data)
{
    gboolean act;
    act = gtk_toggle_button_get_active ((GtkToggleButton *) toggleb);
    gtk_widget_set_sensitive (window_setting_log->label_del, act);
    gtk_widget_set_sensitive (window_setting_log->spinb_del, act);
    gtk_widget_set_sensitive (window_setting_log->combob_del, act);
}

void
on_button_ok_window_setting_log_clicked (GtkButton *button,
                                         gpointer   data)
{
    gint i;
    glong show, del = 0;
    gboolean act;

    i = gtk_combo_box_get_active ((GtkComboBox *) window_setting_log->combob_show);
    show = gtk_spin_button_get_value_as_int ((GtkSpinButton *) 
                                             window_setting_log->spinb_show);
    if (i == 0) /* menit */
        show *= 60;
    else if (i == 1) /* jam */
        show *= 60 * 60;
    else /* hari */
        show *= 24 * 60 * 60;

    act = gtk_toggle_button_get_active ((GtkToggleButton *) 
                                        window_setting_log->checkb_del);
    if (act)
    {
        i = gtk_combo_box_get_active ((GtkComboBox *) window_setting_log->combob_del);
        del = gtk_spin_button_get_value_as_int ((GtkSpinButton *) 
                                                window_setting_log->spinb_del);
        if (i == 0)
            del *= 60;
        else if (i == 1)
            del *= 60 * 60;
        else
            del *= 24 * 60 * 60;
    }

    gbilling_server_set_log_show (show);
    gbilling_server_set_log_deltime (del);

    gtk_widget_destroy (window_setting_log->window);
    g_free (window_setting_log);
}

void
on_window_setting_log_entry_name_changed (GtkEditable *editable,
                                          gpointer     data)
{
    gtk_entry_set_text ((GtkEntry *) window_setting_log->entry_passwd, "");
    gtk_widget_set_sensitive (window_setting_log->label_passwd, TRUE);
    gtk_widget_set_sensitive (window_setting_log->entry_passwd, TRUE);
    gtk_widget_set_sensitive (window_setting_log->label_passwdc, FALSE);
    gtk_widget_set_sensitive (window_setting_log->entry_passwdc, FALSE);
    gtk_widget_set_sensitive (window_setting_log->button_save, FALSE);
}

void
on_window_setting_log_entry_passwd_changed (GtkEditable *editable,
                                            gpointer     data)
{
    gtk_entry_set_text ((GtkEntry *) window_setting_log->entry_passwdc, "");
    gtk_widget_set_sensitive (window_setting_log->label_passwdc, TRUE);
    gtk_widget_set_sensitive (window_setting_log->entry_passwdc, TRUE);
    gtk_widget_set_sensitive (window_setting_log->button_save, FALSE);
}

void
on_window_setting_log_entry_passwdc_changed (GtkEditable *editable,
                                             gpointer     data)
{
    gtk_widget_set_sensitive (window_setting_log->button_save, TRUE);
}

void
on_window_setting_log_button_save_clicked (GtkButton *button,
                                           gpointer   data)
{
    GbillingAuth *auth;
    const gchar *tmp_name;
    const gchar *tmp_passwd;
    const gchar *tmp_passwdc;

    tmp_name = gtk_entry_get_text ((GtkEntry *) window_setting_log->entry_name);
    tmp_passwd = gtk_entry_get_text ((GtkEntry *) window_setting_log->entry_passwd);
    tmp_passwdc = gtk_entry_get_text ((GtkEntry *) window_setting_log->entry_passwdc);

    if (!(strlen (tmp_name) && strlen (tmp_passwd)))
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, 
                                (GtkWindow *) window_setting_log->window,
                                "Keamanan Log", "Silahkan periksa kembali username "
                                "dan password yang diisi.");
        return;
    }

    if (!strcmp (tmp_name, tmp_passwd))
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING,
                                (GtkWindow *) window_setting_log->window,
                                "Keamanan Log", "Username dan password tidak boleh sama.");
        return;
    }

    if (strcmp (tmp_passwd, tmp_passwdc))
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING,
                                (GtkWindow *) window_setting_log->window,
                                "Keamanan Log", "Password dan konfirmasi password "
                                "tidak sama.");
        return;
    }

    auth = gbilling_auth_new (tmp_name, tmp_passwd);
    gbilling_server_set_log_auth (auth);
    
    gtk_widget_set_sensitive ((GtkWidget *) window_setting_log->label_passwd, FALSE);
    gtk_widget_set_sensitive ((GtkWidget *) window_setting_log->entry_passwd, FALSE);
    gtk_widget_set_sensitive ((GtkWidget *) window_setting_log->label_passwdc, FALSE);
    gtk_widget_set_sensitive ((GtkWidget *) window_setting_log->entry_passwdc, FALSE);
    gtk_widget_set_sensitive ((GtkWidget *) window_setting_log->button_save, FALSE);

    g_free (auth);
}

void
on_checkb_logo_window_setting_server_toggled (GtkToggleButton *button,
                                              gpointer         data)
{
    gboolean active;
    
    active = gtk_toggle_button_get_active (button);
    gtk_widget_set_sensitive (window_setting_server->label_logo, active);
    gtk_widget_set_sensitive (window_setting_server->entry_logo, active);
    gtk_widget_set_sensitive (window_setting_server->button_logo, active);
}

void
on_render_play_window_setting_server_toggled (GtkCellRendererToggle *toggle,
                                              gchar                 *path_string,
                                              gpointer               data)
{
    gboolean state = gtk_cell_renderer_toggle_get_active (toggle);
    GtkTreeModel *model = gtk_tree_view_get_model ((GtkTreeView *) 
                                window_setting_server->treeview_sound);
    GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
    GtkTreeIter iter;
    gint *id = gtk_tree_path_get_indices (path);
    gboolean test;

    gtk_tree_model_get_iter (model, &iter, path);
    gtk_list_store_set ((GtkListStore *) model, &iter, 0, !state, -1);
    test = g_file_test (current_soundopt[*id].sound, 
                        G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS);

    if (!test && !state == TRUE)
    {
        gtk_list_store_set ((GtkListStore *) model, &iter, 0, FALSE, -1);
        gtk_tree_path_free (path);
        gbilling_create_dialog (GTK_MESSAGE_WARNING, 
                                (GtkWindow *) window_setting_server->window,
                                "File Suara", "Silahkan pilih file suara "
                                "terlebih dahulu.");
        return;
    }

    current_soundopt[*id].play = !state;

    write_setting ();

    gtk_tree_path_free (path);
}

void
on_treeview_sound_window_setting_server_changed (GtkTreeSelection *selection,
                                                 gpointer          data)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreePath *path;
    gchar *basename;
    gint *id;
    gboolean test;

    gtk_tree_selection_get_selected (selection, &model, &iter);
    path = gtk_tree_model_get_path (model, &iter);
    id = gtk_tree_path_get_indices (path);

    test = g_file_test (current_soundopt[*id].sound, 
                        G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS);
    if (test)
    {
        basename = g_path_get_basename (current_soundopt[*id].sound);
        gtk_entry_set_text ((GtkEntry *) window_setting_server->entry_sound, basename);
        g_free (basename);
    }
    else
        gtk_entry_set_text ((GtkEntry *) window_setting_server->entry_sound, "");

    gtk_tree_path_free (path);
}

void
on_button_browse_window_setting_server_clicked (GtkWidget *widget,
                                                gpointer   data)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    GtkWidget *filechooser;
    GtkFileFilter *filter;
    gint ret, *id;
    gboolean test;
    gchar *basename, *sound_filename;

    selection = gtk_tree_view_get_selection ((GtkTreeView *) 
                                             window_setting_server->treeview_sound);
    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
        return;

    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, "WAV Files (*.wav)");
    gtk_file_filter_add_pattern (filter, "*.wav");

    filechooser = gbilling_file_dialog ("File Suara", 
                                        (GtkWindow *) window_setting_server->window,
                                        GTK_FILE_CHOOSER_ACTION_OPEN);
    gtk_file_chooser_add_filter ((GtkFileChooser *) filechooser, filter);
    gtk_file_chooser_set_filter ((GtkFileChooser *) filechooser, filter);

    path = gtk_tree_model_get_path (model, &iter);
    id = gtk_tree_path_get_indices (path);
    test = g_file_test (current_soundopt[*id].sound,
                        G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS);
    if (test)
        gtk_file_chooser_set_filename ((GtkFileChooser *) filechooser,
                                       current_soundopt[*id].sound);

    ret = gtk_dialog_run ((GtkDialog *) filechooser);
    if (ret != GTK_RESPONSE_OK)
    {
        gtk_widget_destroy (filechooser);
        return;
    }

    sound_filename = gtk_file_chooser_get_filename ((GtkFileChooser *) filechooser);
    gtk_widget_destroy (filechooser);

    g_free (current_soundopt[*id].sound);
    current_soundopt[*id].sound = g_strdup (sound_filename);
    gtk_tree_path_free (path);

    basename = g_path_get_basename (sound_filename);
    gtk_entry_set_text ((GtkEntry *) window_setting_server->entry_sound, basename);
    g_free (basename);
    g_free (sound_filename);
}

void
on_button_preview_window_setting_server_clicked (GtkWidget *widget,
                                                 gpointer   data)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    gint *id;
    gboolean test;
    
    selection = gtk_tree_view_get_selection ((GtkTreeView *) 
                                             window_setting_server->treeview_sound);
    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
        return;

    path = gtk_tree_model_get_path (model, &iter);
    id = gtk_tree_path_get_indices (path);
    test = g_file_test (current_soundopt[*id].sound, 
                        G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS);
    if (test)
        sound_play (*id);

    gtk_tree_path_free (path);
}

void
on_button_ok_window_setting_server_clicked (GtkWidget *widget,
                                            gpointer   data)
{
    GbillingServerInfo info;
    gushort port, oldport;
    gint value;
    const gchar *str;
    gchar *tmp;

    str = gtk_entry_get_text ((GtkEntry *) window_setting_server->entry_name);
    tmp = g_strstrip (g_strdup (str));
    g_snprintf (info.name, sizeof(info.name), tmp);
    g_free (tmp);
    if (!strlen (info.name))
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, 
                                (GtkWindow *) window_setting_server->window,
                                "Nama Warnet", "Silahkan isi nama warnet anda "
                                "terlebih dahulu.");
        return;
    }

    str = gtk_entry_get_text ((GtkEntry *) window_setting_server->entry_desc);
    tmp = g_strstrip (g_strdup (str));
    g_snprintf (info.desc, sizeof(info.desc), tmp);
    g_free (tmp);
    if (!strlen (info.desc))
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, 
                                (GtkWindow *) window_setting_server->window,
                                "Deskripsi Warnet", "Silahkan isi deskripsi warnet "
                                "anda terlebih dahulu.");
        return;
    }

    str = gtk_entry_get_text ((GtkEntry *) window_setting_server->entry_address);
    tmp = g_strstrip (g_strdup (str));
    g_snprintf (info.addr, sizeof(info.addr), tmp);
    g_free (tmp);
    if (!strlen (str))
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, 
                                (GtkWindow *) window_setting_server->window,
                                "Alamat Warnet", "Silahkan isi alamat warnet "
                                "anda terlebih dahulu.");
        return;
    }
    memcpy (server_info, &info, sizeof(GbillingServerInfo));
    oldport = server->port;
    port = gtk_spin_button_get_value_as_int ((GtkSpinButton *) 
                                             window_setting_server->spinb_port);
    server->port = port;
    
    value = gtk_toggle_button_get_active ((GtkToggleButton *) 
                                          window_setting_server->checkb_autoactive);
    server->autoactive = value;

    server->display_name = gtk_toggle_button_get_active ((GtkToggleButton *)
                               window_setting_server->checkb_name);
    server->display_desc = gtk_toggle_button_get_active ((GtkToggleButton *) 
                               window_setting_server->checkb_desc);
    server->display_addr = gtk_toggle_button_get_active ((GtkToggleButton *)
                               window_setting_server->checkb_address);
    server->display_logo = gtk_toggle_button_get_active ((GtkToggleButton *)
                               window_setting_server->checkb_logo);

    gtk_widget_destroy (window_setting_server->window);
    g_free (window_setting_server);
    set_server_info (window_main->window);
    write_setting ();

    if (port != oldport)
        gbilling_create_dialog (GTK_MESSAGE_INFO, window_main->window,
                "Perubahan Port", "Port baru akan digunakan pada saat server "
                "dijalankan kembali.");
}

void
on_button_add_window_setting_client_clicked (GtkButton *button,
                                             gpointer   data)
{
    create_window_update_client (GBILLING_UPDATE_MODE_ADD);
    gtk_widget_show (window_update_client->window);
}

/* current song: AC/DC, Have Drink On Me */
void
on_button_edit_window_setting_client_clicked (GtkButton *button,
                                              gpointer   data)
{
    ClientData *cdata;
    GList *ptr;
    GbillingClient *client;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar *name;

    selection = gtk_tree_view_get_selection 
                    ((GtkTreeView *) window_setting_client->treeview);
    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
        return;
    gtk_tree_model_get (model, &iter, SCLIENT_NAME, &name, -1);

    /* Cek jika client sedang login */
    ptr = client_list;
    while (ptr)
    {
        cdata = ptr->data;
        if (!strcmp (name, cdata->name) && (cdata->status == GBILLING_STATUS_LOGIN))
        {
            gbilling_create_dialog (GTK_MESSAGE_WARNING, 
                                    (GtkWindow *) window_setting_client->window,
                                    "Edit Client", "<b>%s</b> sedang login, "
                                    "tidak dapat mengedit client.", name);
            g_free (name);
            return;
        }
        ptr = g_list_next (ptr);
    }

    create_window_update_client (GBILLING_UPDATE_MODE_EDIT);

    if ((client = gbilling_server_get_client_by_name (name)))
    {
        gtk_entry_set_text ((GtkEntry *) window_update_client->entry_name, client->name);
        gtk_entry_set_text ((GtkEntry *) window_update_client->entry_ip, client->ip);
        gbilling_client_free (client);
    }
    gtk_widget_show (window_update_client->window);
    g_free (name);
}

void
on_button_del_window_setting_client_clicked (GtkButton *button,
                                             gpointer   data)
{
    GList *ptr;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    const ClientData *cdata;
    ClientData *client;
    GbillingClient *tmp;
    gchar *name;
    gint d;

    selection = gtk_tree_view_get_selection 
                    ((GtkTreeView *) window_setting_client->treeview);
    model = (GtkTreeModel *) window_setting_client->store;
    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
        return;
    gtk_tree_model_get (model, &iter, SCLIENT_NAME, &name, -1);
    cdata = get_client_by_name (name);
    g_free (name);
    g_return_if_fail (cdata != NULL); /* yaks, bisa gagal? */
    if (cdata->status == GBILLING_STATUS_LOGIN)
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, window_setting_client->window,
            "Hapus Client", "<b>%s</b> sedang login, tidak dapat "
            "menghapus client.", cdata->name);
        return;
    }
    /* konfirmasi */
    d = gbilling_create_dialog (GTK_MESSAGE_QUESTION, window_setting_client->window,
                "Hapus Client", "Client yang dihapus tidak bisa login lagi dan "
                "data log client mungkin tidak lagi konsisten. Hapus client "
                "<b>%s</b> dari server?", cdata->name);
    if (d == GTK_RESPONSE_OK)
    { 
        /**
         * Update list client, karena get_client_by_name() return constant 
         * pointer maka kita perlu mencari data client dari list (traverse) 
         * dengan pointer tersebut.
         */
        g_static_mutex_lock (&client_mutex);
        for (ptr = client_list; ptr; ptr = g_list_next (ptr))
        {
            client = ptr->data;
            if (!strcmp (cdata->name, client->name))
                break;
        }
        if (!ptr)
        {
            gbilling_debug ("%s: client data was not found, this should "
                            "not happen\n", __func__);
            g_static_mutex_unlock (&client_mutex);
            exit (EXIT_FAILURE);
        }
        g_static_mutex_unlock (&client_mutex);
        /* update model */
        gtk_list_store_remove ((GtkListStore *) model, &iter);
        model = gtk_tree_view_get_model ((GtkTreeView *) window_main->treeview_client);
        gtk_list_store_remove ((GtkListStore *) model, &client->iter);
        model = gtk_icon_view_get_model ((GtkIconView *) window_main->iconview_status);
        gtk_list_store_remove ((GtkListStore *) model, &client->iterv);
        client_list = g_list_remove (client_list, client);
        g_free (client);
        /* update database */
        tmp = gbilling_client_new ();
        tmp->name = g_strdup (cdata->name);
        tmp->ip = g_strdup (cdata->ip);
        d = gbilling_server_delete_client (tmp);
        gbilling_client_free (tmp);
    }
}

/* Batasi hanya karakter digit dan titik yang diterima pada entry IP address */
void
on_window_update_client_entry_ip_insert_text (GtkEditable *editable,
                                              gchar       *text, 
                                              gint         len, 
                                              gint        *pos, 
                                              gpointer     data)
{
    if (len == 0) return;

    gchar last_char = text[len - 1];

    if (last_char == EOF || last_char == '.')
        return;

    if (!g_ascii_isdigit (last_char))
        g_signal_stop_emission_by_name (editable, "insert-text");
}

gboolean
on_window_update_client_delete_event (GtkWidget *widget,
                                      GdkEvent  *event,
                                      gpointer   data)
{
    gtk_widget_destroy (window_update_client->window);
    g_free (window_update_client);
    return FALSE;
}

void
on_button_ok_window_update_client_clicked (GtkButton *button,
                                           gpointer   data)
{
    GtkTreeModel *model, *cmodel, *smodel;
    GtkTreeIter iter;
    GdkPixbuf *pixbuf, *pix;
    GError *error = NULL;
    GbillingClient *new;
    ClientData *cdata;
    const gchar *name, *ip;
    gchar *pixfile, **nibble;
    gint mode;
    guint len;

    name = gtk_entry_get_text ((GtkEntry *) window_update_client->entry_name);
    ip = gtk_entry_get_text ((GtkEntry *) window_update_client->entry_ip);
    new = gbilling_client_new ();
    new->name = g_strstrip (g_strdup (name));
    new->ip = g_strstrip (g_strdup (ip));
    
    if (!strlen (new->name) || !strlen (new->ip))
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, window_update_client->window,
                "Tambah Client", "Silahkan isi nama dan alamat IP client terlebih dahulu.");
        gbilling_client_free (new);
        return;
    }

    /* cek nibble IP address (32 bit), masing-masing dalam 8 bit 
       dengan format xxx.xxx.xxx.xxx */
    nibble = g_strsplit_set (new->ip, ".", 4);
    len = g_strv_length (nibble);
    g_strfreev (nibble);

    if (inet_addr (new->ip) == INADDR_NONE || len != 4)
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, window_update_client->window,
                "Tambah Client", "Format alamat IP client tidak benar.");
        gbilling_client_free (new);
        return;
    }

    mode = GPOINTER_TO_INT (data);
    model = gtk_tree_view_get_model ((GtkTreeView *) window_setting_client->treeview);
    cmodel = gtk_tree_view_get_model ((GtkTreeView *) window_main->treeview_client);

    if (mode == GBILLING_UPDATE_MODE_ADD)
    {
        if (!gbilling_server_add_client (new))
        {
            gbilling_create_dialog (GTK_MESSAGE_WARNING, window_update_client->window,
                "Tambah Client", "Client dengan nama seperti <b>%s</b> atau dengan "
                "IP address <b>%s</b> telah ada.", new->name, new->ip);
            gbilling_client_free (new);
            return;
        }
        gtk_list_store_append ((GtkListStore *) model, &iter);
        /* update list client */
        cdata = g_new0 (ClientData, 1);
        cdata->name = g_strdup (new->name);
        cdata->ip = g_strdup (new->ip);
        g_static_mutex_lock (&client_mutex);
        client_list = g_list_append (client_list, cdata);
        g_static_mutex_unlock (&client_mutex);
        gtk_list_store_append ((GtkListStore *) cmodel, &cdata->iter);
        gtk_list_store_set ((GtkListStore *) cmodel, &cdata->iter, 
                            CLIENT_NAME, cdata->name, 
                            CLIENT_ACCESS, "Tutup",
                            CLIENT_STATUS, "Nonaktif",
                            -1);
        /* update @iconv_status */
        smodel = gtk_icon_view_get_model ((GtkIconView *) window_main->iconview_status);
        pixfile = get_data_file ("client.png");
        pixbuf = gdk_pixbuf_new_from_file (pixfile, &error);
        g_free (pixfile);
        if (pixbuf)
        {
            pix = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (pixbuf),
                                  gdk_pixbuf_get_has_alpha (pixbuf), 
                                  gdk_pixbuf_get_bits_per_sample (pixbuf),
                                  gdk_pixbuf_get_width (pixbuf),
                                  gdk_pixbuf_get_height (pixbuf));
            gdk_pixbuf_saturate_and_pixelate (pixbuf, pix, 0.0, FALSE);
            gtk_list_store_append ((GtkListStore *) smodel, &cdata->iterv);
            gtk_list_store_set ((GtkListStore *) smodel, &cdata->iterv,
                                ICONV_NAME, cdata->name,
                                ICONV_PIXBUF, pix,
                                -1);
            g_object_unref (pixbuf);
            g_object_unref (pix);
        }            
        else
        {
            g_warning ("%s: pixbuf != NULL, %s\n", __func__, error->message);
            g_error_free (error);
        }
    }
    else
    {
        GtkTreeSelection *selection;
        GbillingClient *old;
        
        selection = gtk_tree_view_get_selection ((GtkTreeView *) 
                                window_setting_client->treeview);
        gtk_tree_selection_get_selected (selection, &model, &iter); /* Always TRUE right? ;p */
        old = gbilling_client_new ();
        gtk_tree_model_get (model, &iter, 
                            SCLIENT_NAME, &old->name, 
                            SCLIENT_IP, &old->ip, 
                            -1);
        if (!gbilling_server_update_client (new, old))
        {
            gbilling_create_dialog (GTK_MESSAGE_WARNING, window_update_client->window,
                "Edit Client", "Client dengan nama seperti <b>%s</b> atau dengan "
                "IP address <b>%s</b> telah ada.", new->name, new->ip);
            gbilling_client_free (old);
            gbilling_client_free (new);
            return;
        }
        /* update list client */
        cdata = get_client_by_name (old->name);
        g_free (cdata->name);
        g_free (cdata->ip);
        cdata->name = g_strdup (new->name);
        cdata->ip = g_strdup (new->ip);
        gbilling_client_free (old);
        gtk_list_store_set ((GtkListStore *) cmodel, &cdata->iter, 
                            CLIENT_NAME, cdata->name, -1);

        /* Update iconview status */
        smodel = gtk_icon_view_get_model ((GtkIconView *) window_main->iconview_status);;
        gtk_list_store_set ((GtkListStore *) smodel, &cdata->iterv, 
                            ICONV_NAME, cdata->name,
                            -1);
    }
    gtk_list_store_set ((GtkListStore *) model, &iter,
                        SCLIENT_NAME, new->name,
                        SCLIENT_IP, new->ip,
                        -1);
    on_window_update_client_delete_event (NULL, NULL, NULL);
    gbilling_client_free (new);
}

gboolean
on_window_setting_cost_delete_event (GtkWidget *widget,
                                     GdkEvent  *event,
                                     gpointer   data)
{
    gtk_widget_destroy (window_setting_cost->window);
    g_free (window_setting_cost);
    return FALSE;
}

void
on_button_scost_add_clicked  (GtkButton *button,
                              gpointer   data)
{
    create_window_update_cost (GBILLING_UPDATE_MODE_ADD);
    set_label_sumr_window_update_cost ();
    gtk_widget_show (window_update_cost->window);
}

void
on_button_scost_edit_clicked (GtkButton *button,
                              gpointer   data)
{
    ClientData *cdata;
    GList *ptr;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GbillingCost *cost;
    gchar *name;

    selection = gtk_tree_view_get_selection 
                    ((GtkTreeView *) window_setting_cost->treeview);
    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
        return;
    gtk_tree_model_get (model, &iter, SCCOST_NAME, &name, -1);
    cost = get_cost (name);
    g_free (name);
    g_return_if_fail (cost != NULL);

    /* Cek jika ada client yang sedang menggunakan tarif ini */
    ptr = client_list;
    while (ptr)
    {
        cdata = ptr->data;
        if (cost == cdata->cost
            && (cdata->status == GBILLING_STATUS_LOGIN) 
            && (cdata->type == GBILLING_LOGIN_TYPE_REGULAR))
        {
            gbilling_create_dialog (GTK_MESSAGE_WARNING,
                                    (GtkWindow *) window_setting_cost->window,
                                    "Edit Tarif", "Tarif sedang digunakan oleh "
                                    "client yang sedang login.");
            return;
        }
        ptr = ptr->next;
    }

    create_window_update_cost (GBILLING_UPDATE_MODE_EDIT);
    if (!cost->mode)
        gtk_toggle_button_set_active ((GtkToggleButton *) 
            window_update_cost->radiob_mode1, TRUE);
    else
        gtk_toggle_button_set_active ((GtkToggleButton *) 
            window_update_cost->radiob_mode2, TRUE);
    gtk_entry_set_text ((GtkEntry *) window_update_cost->entry_name, cost->name);
    gtk_spin_button_set_value ((GtkSpinButton *) 
        window_update_cost->spinb_fcost, cost->fcost);
    gtk_spin_button_set_value ((GtkSpinButton *) 
        window_update_cost->spinb_fmin, cost->fmin);
    gtk_spin_button_set_value ((GtkSpinButton *) 
        window_update_cost->spinb_imin, cost->imin);
    gtk_spin_button_set_value ((GtkSpinButton *) 
        window_update_cost->spinb_icost, cost->icost);
    set_label_sumr_window_update_cost ();
    gtk_widget_show (window_update_cost->window);
}

void
on_button_scost_del_clicked (GtkButton *button,
                             gpointer   data)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GbillingCost *cost;
    GbillingClientList *ptr = client_list;
    GbillingCostRotationList *cptr = costrotation_list;
    GbillingCostRotation *rotation;
    ClientData *cdata;
    gchar *name;
    gint ret;
    gboolean login = FALSE;

    selection = gtk_tree_view_get_selection ((GtkTreeView *) window_setting_cost->treeview);
    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
        return;
    gtk_tree_model_get (model, &iter, SCCOST_NAME, &name, -1);
    cost = get_cost (name);
    g_free (name);
    g_return_if_fail (cost != NULL);

    /* Return jika tarif yang akan dihapus adalah default tarif */
    if (cost->def)
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, window_setting_cost->window,
            "Pengaturan Tarif", "Tarif default tidak dapat dihapus.");
        return;
    }
    
    /* 
     * Jika rotasi tarif ada, periksa client yang sedang login 
     * menggunakan rotasi dengan tarif ini.
     */
    if (ptr)
        while (ptr && !login)
        {
            cdata = ptr->data;
            if (cdata->status == GBILLING_STATUS_LOGIN && cdata->cost == cost)
                login = TRUE;
            ptr = ptr->next;
        }

    if (login)
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, 
                                (GtkWindow *) window_setting_cost->window,
                                "Hapus Tarif", "Silahkan hapus tarif jika tidak ada "
                                "client yang sedang menggunakan tarif.");
        return;
    }

    ret = gbilling_create_dialog (GTK_MESSAGE_QUESTION, window_setting_cost->window,
            "Pengaturan Tarif", "Hapus tarif <b>%s</b> (dan rotasi yang memakai "
            "tarif ini)?", cost->name);
    if (ret != GTK_RESPONSE_OK)
        return;

    gbilling_server_delete_cost (cost->name);
    gtk_list_store_remove ((GtkListStore *) model, &iter);
    ret = g_list_index (cost_list, cost);

    /* Hapus rotasi yang menggunakan tarif ini */
    if (cptr)
    {
        while (cptr)
        {
            rotation = cptr->data;
            if (!strcmp (rotation->cost, cost->name))
            {
                costrotation_list = g_list_remove (costrotation_list, rotation);
                gbilling_server_delete_cost_rotation (rotation);
                g_free (rotation);

                /* Traverse ulang dari head list */
                cptr = costrotation_list;
                continue;
            }
            
            cptr = g_list_next (cptr);
        }
        cptr = costrotation_list;
        
        model = gtk_tree_view_get_model ((GtkTreeView *) 
                                         window_setting_cost->treeview_rotation);
        gtk_list_store_clear ((GtkListStore *) model);
    }
    
    while (cptr)
    {
        rotation = cptr->data;
        name = g_strdup_printf ("%.2i:%.2i - %.2i:%.2i", rotation->shour, 
                                rotation->smin, rotation->ehour, rotation->emin);
        gtk_list_store_append ((GtkListStore *) model, &iter);
        gtk_list_store_set ((GtkListStore *) model, &iter,
                   SCOSTROTATION_NAME, rotation->cost,
                   SCOSTROTATION_INTERVAL, name,
                   SCOSTROTATION_STATE, rotation->state,
                   -1);
        g_free (name);
        cptr = cptr->next;
    }
    
    cost_list = g_list_remove (cost_list, cost);
    g_free (cost);
}

void
on_button_default_window_setting_cost_clicked (GtkButton *button,
                                               gpointer   data)
{
    ClientData *cdata;
    GList *ptr;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter, tmp;
    GbillingCost *cost, *tcost;
    gchar *name;
    gint ret;
    gboolean valid;

    selection = gtk_tree_view_get_selection ((GtkTreeView *) 
                                             window_setting_cost->treeview);
    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
        return;

    gtk_tree_model_get (model, &iter, SCCOST_NAME, &name, -1);
    cost = gbilling_server_get_cost_by_name (name);
    g_free (name);
    g_return_if_fail (cost != NULL);

    if (cost->def)
    {
        gbilling_cost_free (cost);
        return;
    }

    /* Cek jika tarif default sedang digunakan oleh client yang login */
    for (ptr = client_list; ptr; ptr = ptr->next)
    {
        cdata = ptr->data;
        if ((cdata->status == GBILLING_STATUS_LOGIN) &&
            (cdata->type == GBILLING_LOGIN_TYPE_REGULAR))
        {
            gbilling_create_dialog (GTK_MESSAGE_WARNING,
                                    (GtkWindow *) window_setting_cost->window,
                                    "Tarif Default", "Client sedang menggunakan "
                                    "tarif default.");
            return;
        }
    }

    ret = gbilling_create_dialog (GTK_MESSAGE_QUESTION, window_setting_cost->window, 
                "Tarif Default", "Gunakan tarif <b>%s</b> sebagai "
                "tarif default?", cost->name);
    if (ret == GTK_RESPONSE_OK)
    {
        gbilling_server_set_default_cost (cost->name);
        /* Rubah data tarif di list */
        for (ptr = cost_list; ptr; ptr = ptr->next)
        {
            tcost = ptr->data;
            tcost->def = 0;
        }
        tcost = get_cost (cost->name);
        tcost->def = 1;
        valid = gtk_tree_model_get_iter_first (model, &tmp);
        while (valid)
        {
            gtk_list_store_set ((GtkListStore *) model, &tmp, SCCOST_DEFAULT, FALSE, -1);
            valid = gtk_tree_model_iter_next (model, &tmp);
        }
        gtk_list_store_set ((GtkListStore *) model, &iter, SCCOST_DEFAULT, TRUE, -1);
    }
    gbilling_cost_free (cost);
}

void
on_button_add_rotation_window_setting_cost_clicked (GtkWidget *widget,
                                                    gpointer   data)
{
    GbillingCostList *ptr = cost_list;
    GbillingCost *cost;

    /* TODO: Karena block menit dalam pemeriksaan interval tarif di 
     * on_button_ok_window_rotate_cost() hanya sebesar 1 byte (signed) maka
     * jumlah rotasi tarif maksimal adalah sebesar G_MAXINT8 (127).
     * 
     * Jumlah rotasi tarif yang mungkin adalah 60 * 24 = 1440 rotasi.
     * untuk sementara batasi penambahan rotasi jika telah mencapai G_MAXINT8.
     */
    if (g_list_length (costrotation_list) >= G_MAXINT8)
    {
        gbilling_create_dialog (GTK_MESSAGE_INFO, 
                                (GtkWindow *) window_setting_server->window,
                                "Tambah Rotasi", 
                                "Jumlah rotasi tarif telah maksimal."); 
        return;
    }

    create_window_rotate_cost (GBILLING_UPDATE_MODE_ADD);

    while (ptr)
    {
        cost = ptr->data;
        gtk_combo_box_append_text ((GtkComboBox *) 
                                   window_rotate_cost->combob_cost, cost->name);
        ptr = ptr->next;
    }

    if (cost_list)
        gtk_combo_box_set_active ((GtkComboBox *) window_rotate_cost->combob_cost, 0);

    gtk_widget_show (window_rotate_cost->window);
}

void
on_button_edit_rotation_window_setting_cost_clicked (GtkWidget *widget,
                                                     gpointer   data)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreePath *path;
    GbillingCostRotation *rotation;
    gint *id;
    
    selection = gtk_tree_view_get_selection ((GtkTreeView *) 
                                             window_setting_cost->treeview_rotation);
    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
        return;

    path = gtk_tree_model_get_path (model, &iter);
    id = gtk_tree_path_get_indices (path);
    rotation = g_list_nth_data (costrotation_list, *id);
    gtk_tree_path_free (path);
    
    g_return_if_fail (rotation != NULL);
    
    create_window_rotate_cost (GBILLING_UPDATE_MODE_EDIT);
    gtk_combo_box_prepend_text ((GtkComboBox *) window_rotate_cost->combob_cost,
                                rotation->cost);
    gtk_combo_box_set_active ((GtkComboBox *) window_rotate_cost->combob_cost, 0);
    gtk_toggle_button_set_active ((GtkToggleButton *) 
                                  window_rotate_cost->checkb_active, rotation->state);
    gtk_spin_button_set_value ((GtkSpinButton *) window_rotate_cost->spinb_shour,   
                               rotation->shour);
    gtk_spin_button_set_value ((GtkSpinButton *) window_rotate_cost->spinb_smin,
                               rotation->smin);
    gtk_spin_button_set_value ((GtkSpinButton *) window_rotate_cost->spinb_ehour,
                               rotation->ehour);
    gtk_spin_button_set_value ((GtkSpinButton *) window_rotate_cost->spinb_emin,
                               rotation->emin);
    gtk_widget_show (window_rotate_cost->window);
}

void
on_button_del_rotation_window_setting_cost_clicked (GtkWidget *widget,
                                                    gpointer   data)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    GbillingCostRotation *rotation;
    gint ret, *id;

    selection = gtk_tree_view_get_selection ((GtkTreeView *) 
                                             window_setting_cost->treeview_rotation);
    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
        return;

    ret = gbilling_create_dialog (GTK_MESSAGE_QUESTION, 
                                  (GtkWindow *) window_setting_cost->window,
                                  "Rotasi Tarif", "Hapus rotasi tarif yang dipilih?");
    if (ret != GTK_RESPONSE_OK)
        return;

    path = gtk_tree_model_get_path (model, &iter);
    id = gtk_tree_path_get_indices (path);
    rotation = g_list_nth_data (costrotation_list, *id);
    gtk_tree_path_free (path);

    g_return_if_fail (rotation != NULL);

    gbilling_server_delete_cost_rotation (rotation);
    costrotation_list = g_list_remove (costrotation_list, rotation);
    gtk_list_store_remove ((GtkListStore *) model, &iter);
    update_cost_display (NULL);
    g_free (rotation);
}

gboolean
on_window_update_cost_delete_event (GtkWidget *widget,
                                    GdkEvent  *event,
                                    gpointer   data)
{
    gtk_widget_destroy (window_update_cost->window);
    g_free (window_update_cost);
    return FALSE;
}

void
on_radiob_window_cost_mode1_toggled (GtkToggleButton *togglebutton,
                                     gpointer         data)
{
    gtk_widget_set_sensitive (window_update_cost->spinb_fcost, TRUE);
    gtk_widget_set_sensitive (window_update_cost->spinb_fmin, TRUE);
    set_label_sumr_window_update_cost ();
}

void
on_radiob_window_cost_mode2_toggled (GtkToggleButton *togglebutton,
                                     gpointer         data)
{
    gtk_widget_set_sensitive (window_update_cost->spinb_fcost, FALSE);
    gtk_widget_set_sensitive (window_update_cost->spinb_fmin, FALSE);
    set_label_sumr_window_update_cost ();
}

void
set_label_sumr_window_update_cost (void)
{
    gint imin, icost, fcost, fmin, tcost;
    gchar *markup;

    imin = gtk_spin_button_get_value_as_int ((GtkSpinButton *) window_update_cost->spinb_imin);
    icost = gtk_spin_button_get_value_as_int ((GtkSpinButton *) window_update_cost->spinb_icost);

    if (gtk_toggle_button_get_active ((GtkToggleButton *) window_update_cost->radiob_mode1))
    {
        fcost = gtk_spin_button_get_value_as_int ((GtkSpinButton *)
                    window_update_cost->spinb_fcost);
        fmin = gtk_spin_button_get_value_as_int ((GtkSpinButton *)
                    window_update_cost->spinb_fmin);
        tcost = fcost + ((60 - fmin) / imin * icost);
        markup = g_markup_printf_escaped ("<span size=\"small\"><b>Rincian:</b> "
                        "Tarif perjam <b>Rp. %i</b>.\nTarif untuk <b>%i</b> menit "
                        "pertama <b>Rp. %i</b>.\nPenambahan <b>Rp. %i</b> setiap "
                        "<b>%i</b> menit.</span>", tcost, fmin, fcost, icost, imin);
    }
    else
    {
        tcost = 60 / imin * icost;
        markup = g_markup_printf_escaped ("<span size=\"small\"><b>Rincian:</b> "
                        "Tarif perjam <b>Rp. %i</b>.\nPenambahan <b>Rp. %i</b> "
                        "setiap <b>%i</b> menit.</span>", tcost, icost, imin);
    }
    gtk_label_set_markup ((GtkLabel *) window_update_cost->label_sumr, markup);
    g_free (markup);
}

void
on_button_ok_window_update_cost_clicked (GtkButton *button,
                                         gpointer   data)
{
    GbillingCost *cost, *tmp;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar *name;
    const gchar *head[] = { "Tambah Tarif", "Edit Tarif" };
    gint mode;

    mode = GPOINTER_TO_INT (data);
    name = g_strdup (gtk_entry_get_text ((GtkEntry *) window_update_cost->entry_name));
    name = g_strstrip (name);
    if (!strlen (name))
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, window_update_cost->window,
            head[mode], "Silahkan isi nama tarif terlebih dahulu.");
        g_free (name);
        return;
    }
    cost = gbilling_cost_new ();
    cost->mode = (gtk_toggle_button_get_active ((GtkToggleButton *) 
                        window_update_cost->radiob_mode1)) ? 
                        GBILLING_COST_MODE_FMIN : GBILLING_COST_MODE_FLAT;
    g_snprintf (cost->name, sizeof(cost->name), name);
    cost->fmin = gtk_spin_button_get_value_as_int ((GtkSpinButton *) window_update_cost->spinb_fmin);
    cost->imin = gtk_spin_button_get_value_as_int ((GtkSpinButton *) window_update_cost->spinb_imin);
    cost->fcost = gtk_spin_button_get_value_as_int ((GtkSpinButton *) window_update_cost->spinb_fcost);
    cost->icost = gtk_spin_button_get_value_as_int ((GtkSpinButton *) window_update_cost->spinb_icost);

    if (mode == GBILLING_UPDATE_MODE_ADD)
    {
        if (!gbilling_server_add_cost (cost))
        {
            gbilling_create_dialog (GTK_MESSAGE_WARNING, window_update_cost->window, head[mode], 
                    "Nama tarif seperti <b>%s</b> telah ada.", cost->name);
            gbilling_cost_free (cost);
            g_free (name);
            return;
        }
        tmp = g_memdup (cost, sizeof(GbillingCost));
        cost_list = g_list_append (cost_list, tmp);
        model = gtk_tree_view_get_model ((GtkTreeView *) window_setting_cost->treeview);
        gtk_list_store_append ((GtkListStore *) model, &iter);
    }
    else
    {
        GbillingCostList *ptr;
        GtkTreeSelection *selection;
        gboolean def;

        selection = gtk_tree_view_get_selection ((GtkTreeView *) 
                                                 window_setting_cost->treeview);
        gtk_tree_selection_get_selected (selection, &model, &iter);
        gtk_tree_model_get (model, &iter, SCCOST_NAME, &name, -1);
        gtk_tree_model_get (model, &iter, SCCOST_DEFAULT, &def, -1);
        cost->def = def ? 1 : 0;
        if (!gbilling_server_update_cost (cost, name))
        {
            gbilling_create_dialog (GTK_MESSAGE_WARNING, window_update_cost->window, 
                head[mode], "Nama tarif seperti <b>%s</b> telah ada dalam server.", cost->name);
            gbilling_cost_free (cost);
            g_free (name);
            return;
        }
        for (ptr = cost_list; ptr; ptr = ptr->next)
        {
            tmp = ptr->data;
            if (!strcmp (tmp->name, name))
            {
                memcpy (tmp, cost, sizeof(GbillingCost));
                break;
            }
        }
    }
    gtk_list_store_set ((GtkListStore *) model, &iter,
                        SCCOST_NAME, cost->name,
                        SCCOST_MODE, (cost->mode == GBILLING_COST_MODE_FMIN) ?
                                     "Menit Pertama" : "Flat",
                        SCCOST_COST, gbilling_cost_per_hour (cost),
                        -1);
    gbilling_cost_free (cost);
    g_free (name);
    on_window_update_cost_delete_event (NULL, NULL, NULL);
}

gboolean
on_window_rotate_cost_deleve_event (GtkWidget *widget,
                                    GdkEvent  *event,
                                    gpointer   data)
{
    gtk_widget_destroy (window_rotate_cost->window);
    g_free (window_rotate_cost);
    return FALSE;
}

void
on_checkb_active_window_rotate_cost_toggled (GtkToggleButton *toggleb,
                                             gpointer         data)
{
    gboolean state = gtk_toggle_button_get_active (toggleb);
    gtk_widget_set_sensitive (window_rotate_cost->spinb_shour, state);
    gtk_widget_set_sensitive (window_rotate_cost->spinb_smin, state);
    gtk_widget_set_sensitive (window_rotate_cost->spinb_ehour, state);
    gtk_widget_set_sensitive (window_rotate_cost->spinb_emin, state);
}

void
on_button_ok_window_rotate_cost_clicked (GtkWidget *widget,
                                         gpointer   data)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    GbillingCostRotationList *list;
    GbillingCostRotation *rotate, *prev = NULL;
    GbillingUpdateMode mode = GPOINTER_TO_INT (data);
    gchar *interval;
    gint i, id, start1, start2, end1, end2;
    gint *indices;
    gboolean valid = TRUE;

    /*
     * Cara untuk mengetahui interval-interval rotasi tarif 
     * adalah dengan membuat blok sebanyak jumlah menit per hari,
     * yaitu 60 menit * 24 jam atau 1440 blok. Status terisi dan 
     * tidak terisi blok menentukan bahwa menit tersebut telah digunakan
     * atau tidak digunakan.
     */       

    gint total_minute = 60 * 24;
    gchar minutes_in_day[60 * 24] = { 0 };

    if (mode == GBILLING_UPDATE_MODE_ADD)
    {
        id = gtk_combo_box_get_active ((GtkComboBox *) window_rotate_cost->combob_cost);
        if (id < 0)
        {
            on_window_rotate_cost_deleve_event (NULL, NULL, NULL);
            return;
        }
    }
    else
    {
        selection = gtk_tree_view_get_selection ((GtkTreeView *) 
                                                 window_setting_cost->treeview_rotation);
        if (!gtk_tree_selection_get_selected (selection, &model, &iter))
            return;
        path = gtk_tree_model_get_path (model, &iter);
        indices = gtk_tree_path_get_indices (path);
        id = *indices;
        gtk_tree_path_free (path);

        prev = g_list_nth_data (costrotation_list, id);
        g_return_if_fail (prev != NULL);
    }

    rotate = g_new (GbillingCostRotation, 1);
    if (mode == GBILLING_UPDATE_MODE_ADD)
        g_snprintf (rotate->cost, sizeof rotate->cost, 
                    ((GbillingCost *) g_list_nth_data (cost_list, id))->name);
    rotate->shour = gtk_spin_button_get_value_as_int ((GtkSpinButton *) 
                               window_rotate_cost->spinb_shour);
    rotate->smin = gtk_spin_button_get_value_as_int ((GtkSpinButton *) 
                               window_rotate_cost->spinb_smin);
    rotate->ehour = gtk_spin_button_get_value_as_int ((GtkSpinButton *) 
                               window_rotate_cost->spinb_ehour);
    rotate->emin = gtk_spin_button_get_value_as_int ((GtkSpinButton *) 
                               window_rotate_cost->spinb_emin);
    rotate->state = gtk_toggle_button_get_active ((GtkToggleButton *)
                               window_rotate_cost->checkb_active);

    start2 = rotate->shour * 60 + rotate->smin;
    end2 = rotate->ehour * 60 + rotate->emin;
    if (start2 == end2)
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, 
                                (GtkWindow *) window_rotate_cost->window,
                                "Rotasi Tarif", "Waktu mulai rotasi sama "
                                "dengan akhir rotasi.");
        g_free (rotate);
        return;
    }

    list = costrotation_list;
    while (list)
    {
        GbillingCostRotation *tmp = list->data;
        
        if (mode == GBILLING_UPDATE_MODE_EDIT && tmp == prev)
        {
            list = list->next;
            continue;
        }

        start1 = tmp->shour * 60 + tmp->smin;
        end1 = tmp->ehour * 60 + tmp->emin;

        if (start1 > end1) /* 20.20 - 01.30, 23.00 - 00.00 */
        {
            memset (minutes_in_day + start1, 1, 
                    end1 ? total_minute - start1 : total_minute - start1 - 1);
            if (end1)
                memset (minutes_in_day, 1, end1 - 1);
        }
        else if (start1 < end1) /* 12.00 - 15.00, 00.00 - 06.00 */
            memset (minutes_in_day + start1, 1, end1 - start1 - 1);
        else
        {
            g_free (rotate);
            g_return_if_fail (start1 != end1);
        }

        if (start2 > end2)
        {
            for (i = start2 ? start2 - 1 : 0; i < total_minute; i++)
                if (minutes_in_day[i]) 
                {
                    valid = FALSE;
                    break;
                }

            for (i = 0; end2 > 0 && i < end2 - 1; i++)
                if (minutes_in_day[i])
                {
                    valid = FALSE;
                    break;
                }
        }

        else if (start2 < end2)
        {
            for (i = start2 ? start2 - 1 : 0; i < end2 - 1; i++)
                if (minutes_in_day[i])
                {
                    valid = FALSE;
                    break;
                }
        }

        if (!valid)
        {
            gbilling_create_dialog (GTK_MESSAGE_WARNING, 
                                    (GtkWindow *) window_rotate_cost->window,
                                    "Rotasi Tarif", "Rotasi tarif dengan waktu "
                                    "<b>%.2i:%.2i - %.2i:%.2i</b> konflik "
                                    "dengan rotasi lainnya.",
                                    rotate->shour, rotate->smin,
                                    rotate->ehour, rotate->emin);
            g_free (rotate);
            return;
        }
        list = g_list_next (list);
    }

    if (mode == GBILLING_UPDATE_MODE_ADD)
    {
        costrotation_list = g_list_append (costrotation_list, rotate);
        gbilling_server_add_cost_rotation (rotate);
    }
    else
    {
        gbilling_server_update_cost_rotation (prev, rotate);
        prev->shour = rotate->shour;
        prev->smin = rotate->smin;
        prev->ehour = rotate->ehour;
        prev->emin = rotate->emin;
        prev->state = rotate->state;
    }

    interval = g_strdup_printf ("%.2i:%.2i - %.2i:%.2i", rotate->shour, rotate->smin,
                                rotate->ehour, rotate->emin);

    if (mode == GBILLING_UPDATE_MODE_ADD)
    {
        model = gtk_tree_view_get_model ((GtkTreeView *) 
                                         window_setting_cost->treeview_rotation);
        gtk_list_store_append ((GtkListStore *) model, &iter);
    }

    gtk_list_store_set ((GtkListStore *) model, &iter,
                        SCOSTROTATION_NAME, mode == GBILLING_UPDATE_MODE_ADD ? 
                        rotate->cost : prev->cost,
                        SCOSTROTATION_INTERVAL, interval, 
                        SCOSTROTATION_STATE, rotate->state, 
                        -1);

    on_window_rotate_cost_deleve_event (NULL, NULL, NULL);
    update_cost_display (NULL);
    g_free (interval);
}

#if 0
void
on_button_ok_window_rotate_cost_clicked (GtkWidget *widget,
                                         gpointer   data)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    GbillingCostRotationList *list;
    GbillingCostRotation *rotate, *prev = NULL;
    GbillingUpdateMode mode = GPOINTER_TO_INT (data);
    gchar *interval;
    gint id, *indices;
    gboolean valid = TRUE;
    time_t t, first_time, start1, start2, end1, end2;

    if (mode == GBILLING_UPDATE_MODE_ADD)
    {
        id = gtk_combo_box_get_active ((GtkComboBox *) window_rotate_cost->combob_cost);
        if (id < 0)
        {
            on_window_rotate_cost_deleve_event (NULL, NULL, NULL);
            return;
        }
    }
    else
    {
        selection = gtk_tree_view_get_selection ((GtkTreeView *) 
                                                 window_setting_cost->treeview_rotation);
        if (!gtk_tree_selection_get_selected (selection, &model, &iter))
            return;
        path = gtk_tree_model_get_path (model, &iter);
        indices = gtk_tree_path_get_indices (path);
        id = *indices;
        gtk_tree_path_free (path);

        prev = g_list_nth_data (costrotation_list, id);
        g_return_if_fail (prev != NULL);
    }

    rotate = g_new (GbillingCostRotation, 1);
    rotate->cost = id;
    rotate->shour = gtk_spin_button_get_value_as_int ((GtkSpinButton *) 
                               window_rotate_cost->spinb_shour);
    rotate->smin = gtk_spin_button_get_value_as_int ((GtkSpinButton *) 
                               window_rotate_cost->spinb_smin);
    rotate->ehour = gtk_spin_button_get_value_as_int ((GtkSpinButton *) 
                               window_rotate_cost->spinb_ehour);
    rotate->emin = gtk_spin_button_get_value_as_int ((GtkSpinButton *) 
                               window_rotate_cost->spinb_emin);
    rotate->state = gtk_toggle_button_get_active ((GtkToggleButton *)
                               window_rotate_cost->checkb_active);

    t = time (NULL);
    first_time = get_first_time (t);
    start2 = first_time + (rotate->shour * 3600) + (rotate->smin * 60);
    end2 = first_time + (rotate->ehour * 3600) + (rotate->emin * 60);

    if (start2 == end2)
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, 
                                (GtkWindow *) window_rotate_cost->window,
                                "Rotasi Tarif", "Waktu mulai rotasi sama "
                                "dengan akhir rotasi.");
        g_free (rotate);
        return;
    }

    list = costrotation_list;
    while (list)
    {
        GbillingCostRotation *tmp = list->data;
        
        if (mode == GBILLING_UPDATE_MODE_EDIT && tmp == prev)
        {
            list = list->next;
            continue;
        }

        start1 = first_time + (tmp->shour * 3600) + (tmp->smin * 60);
        end1 = first_time + (tmp->ehour * 3600) + (tmp->emin * 60);

        valid = (start2 < start1 || start2 >= end1) && (end2 <= start1 || end2 > end1);

        if (!valid)
        {
            gbilling_create_dialog (GTK_MESSAGE_WARNING, 
                                    (GtkWindow *) window_rotate_cost->window,
                                    "Rotasi Tarif", "Rotasi tarif dengan waktu "
                                    "<b>%.2i:%.2i - %.2i:%.2i</b> konflik "
                                    "dengan rotasi lainnya.",
                                    rotate->shour, rotate->smin,
                                    rotate->ehour, rotate->emin);
            g_free (rotate);
            return;
        }
        list = g_list_next (list);
    }

    if (mode == GBILLING_UPDATE_MODE_ADD)
    {
        costrotation_list = g_list_append (costrotation_list, rotate);
        gbilling_server_add_cost_rotation (rotate);
    }
    else
    {
        gbilling_server_update_cost_rotation (prev, rotate);
        prev->shour = rotate->shour;
        prev->smin = rotate->smin;
        prev->ehour = rotate->ehour;
        prev->emin = rotate->emin;
        prev->state = rotate->state;
    }

    interval = g_strdup_printf ("%.2i:%.2i - %.2i:%.2i", rotate->shour, rotate->smin,
                                rotate->ehour, rotate->emin);

    if (mode == GBILLING_UPDATE_MODE_ADD)
    {
        model = gtk_tree_view_get_model ((GtkTreeView *) window_setting_cost->treeview_rotation);
        gtk_list_store_append ((GtkListStore *) model, &iter);
    }

    gtk_list_store_set ((GtkListStore *) model, &iter,
                        SCOSTROTATION_NAME, mode == GBILLING_UPDATE_MODE_ADD ?
                            ((GbillingCost *) g_list_nth_data (cost_list, id))->name :
                            ((GbillingCost *) g_list_nth_data (cost_list, prev->cost))->name,
                        SCOSTROTATION_INTERVAL, interval, 
                        SCOSTROTATION_STATE, rotate->state, 
                        -1);

    on_window_rotate_cost_deleve_event (NULL, NULL, NULL);
    update_cost_display (NULL);
    g_free (interval);
}
#endif

void
on_radiob_cost1_toggled (GtkWidget *widget,
                         gpointer   data)
{

}

void
on_radiob_cost2_toggled (GtkWidget *widget,
                         gpointer   data)
{

}

gboolean
on_window_setting_prepaid_delete_event (GtkWidget *widget,
                                        GdkEvent  *event,
                                        gpointer   data)
{
    gtk_widget_destroy (window_setting_prepaid->window);
    g_free (window_setting_prepaid);
    return FALSE;
}

void
on_button_add_window_setting_prepaid_clicked (GtkButton *button,
                                              gpointer   data)
{
    create_window_update_prepaid (GBILLING_UPDATE_MODE_ADD);
    gtk_toggle_button_set_active ((GtkToggleButton *) 
                                  window_update_prepaid->checkb_status, TRUE);
    gtk_widget_show (window_update_prepaid->window);
}

void
on_button_edit_window_setting_prepaid_clicked (GtkButton *button,
                                               gpointer   data)
{
    ClientData *cdata;
    GList *ptr;
    GbillingPrepaid *prepaid;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar *name;

    selection = gtk_tree_view_get_selection ((GtkTreeView *) window_setting_prepaid->treeview);
    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
        return;
    gtk_tree_model_get (model, &iter, SCPREPAID_NAME, &name, -1);
    prepaid = get_prepaid (name);
    g_free (name);
    g_return_if_fail (prepaid != NULL);

    /* Periksa jika ada client yang sedang menggunakan paket ini */
    for (ptr = client_list; ptr != NULL; ptr = g_list_next (ptr))
    {
        cdata = ptr->data;
        if (cdata->type == GBILLING_LOGIN_TYPE_PREPAID 
            && cdata->status == GBILLING_STATUS_LOGIN
            && g_list_nth_data (prepaid_list, cdata->idtype) == prepaid)
        {
            gbilling_create_dialog (GTK_MESSAGE_WARNING, 
                                    (GtkWindow *) window_setting_prepaid->window,
                                    "Edit Paket", 
                                    "Paket <b>%s</b> sedang digunakan.", prepaid->name);
            return;
        }
    }

    create_window_update_prepaid (GBILLING_UPDATE_MODE_EDIT);
    gtk_entry_set_text ((GtkEntry *) window_update_prepaid->entry_name, prepaid->name);
    gtk_spin_button_set_value 
        ((GtkSpinButton *) window_update_prepaid->spinb_dur, prepaid->duration / 60);
    gtk_spin_button_set_value 
        ((GtkSpinButton *) window_update_prepaid->spinb_cost, prepaid->cost);
    gtk_toggle_button_set_active 
        ((GtkToggleButton *) window_update_prepaid->checkb_status, prepaid->active);

    gtk_widget_show (window_update_prepaid->window);
}

void
on_button_del_window_setting_prepaid_clicked (GtkButton *button,
                                              gpointer   data)
{
    ClientData *cdata;
    GList *ptr;
    GbillingPrepaid *prepaid;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar *name;
    gint ret;

    /* Periksa jika ada client yang sedang menggunakan prepaid */
    for (ptr = client_list; ptr != NULL; ptr = g_list_next (ptr))
    {
        cdata = ptr->data;
        if (cdata->type == GBILLING_LOGIN_TYPE_PREPAID &&
            cdata->status == GBILLING_STATUS_LOGIN)
        {
            gbilling_create_dialog (GTK_MESSAGE_WARNING, 
                                    (GtkWindow *) window_setting_prepaid->window,
                                    "Hapus Paket", "Client ada yang sedang menggunakan "
                                    "sistem paket, tidak dapat menghapus paket.");
            return;
        }
    }

    selection = gtk_tree_view_get_selection ((GtkTreeView *) window_setting_prepaid->treeview);
    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
        return;
    gtk_tree_model_get (model, &iter, SCPREPAID_NAME, &name, -1);
    ret = gbilling_create_dialog (GTK_MESSAGE_QUESTION, window_setting_prepaid->window,
                "Hapus Paket", "Hapus paket dengan nama <b>%s</b> dari server?", name);

    if (ret == GTK_RESPONSE_OK)
    {
        gbilling_server_delete_prepaid (name);
        prepaid = get_prepaid (name);
        prepaid_list = g_list_remove (prepaid_list, prepaid);
        gbilling_prepaid_free (prepaid);
        gtk_list_store_remove ((GtkListStore *) model, &iter);
    }
    g_free (name);
}

void
on_button_ok_window_update_prepaid_clicked (GtkButton *button,
                                            gpointer   data)
{
    GList *ptr;
    GbillingPrepaid *prepaid, *new;
    GtkTreeModel *model;
    GtkTreeIter iter;
    const gchar *tmp;
    const gchar *head[] = { "Tambah Paket", "Edit Paket" };
    gchar *name;
    gint mode;

    tmp = gtk_entry_get_text ((GtkEntry *) window_update_prepaid->entry_name);
    name = g_strstrip (g_strdup (tmp));
    mode = GPOINTER_TO_INT (data);
    
    if (!strlen (name))
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, window_update_prepaid->window,
                mode == GBILLING_UPDATE_MODE_ADD ? "Tambah Paket" : "Edit Paket",
                "Silahkan isi name paket terlebih dahulu.");
        gtk_entry_set_text ((GtkEntry *) window_update_prepaid->entry_name, "");
        g_free (name);
        return;
    }
    
    new = gbilling_prepaid_new ();
    g_snprintf (new->name, sizeof(new->name), name);
    new->duration = gtk_spin_button_get_value_as_int ((GtkSpinButton *) 
                            window_update_prepaid->spinb_dur) * 60;
    new->cost = gtk_spin_button_get_value_as_int ((GtkSpinButton *)
                        window_update_prepaid->spinb_cost);
    new->active = gtk_toggle_button_get_active ((GtkToggleButton *)
                            window_update_prepaid->checkb_status);

    if (mode == GBILLING_UPDATE_MODE_ADD)
    {
        if (!gbilling_server_add_prepaid (new))
        {
            gbilling_create_dialog (GTK_MESSAGE_WARNING, window_update_prepaid->window,
                    head[mode], "Nama paket seperti <b>%s</b> telah ada "
                    "dalam server.", new->name);
            gbilling_prepaid_free (new);
            g_free (name);
            return;
        }
        gbilling_server_add_prepaid (new);
        prepaid = g_memdup (new, sizeof(GbillingPrepaid));
        prepaid_list = g_list_append (prepaid_list, prepaid);
        model = gtk_tree_view_get_model ((GtkTreeView *) window_setting_prepaid->treeview);
        gtk_list_store_append ((GtkListStore *) model, &iter);
    }
    else
    {
        GtkTreeSelection *selection;
        
        selection = gtk_tree_view_get_selection ((GtkTreeView *) window_setting_prepaid->treeview);
        gtk_tree_selection_get_selected (selection, &model, &iter);
        gtk_tree_model_get (model, &iter, SCPREPAID_NAME, &name, -1);
        if (!gbilling_server_update_prepaid (new, name))
        {
            gbilling_create_dialog (GTK_MESSAGE_WARNING, window_update_prepaid->window,
                    head[mode], "Nama paket seperti <b>%s</b> telah ada "
                    "dalam server.", new->name);
            gbilling_prepaid_free (new);
            g_free (name);
            return;
        }
        for (ptr = prepaid_list; ptr; ptr = g_list_next (ptr))
        {
            prepaid = ptr->data;
            if (!strcmp (prepaid->name, name))
                break;
        }
        g_return_if_fail (ptr != NULL);
        gbilling_prepaid_free (prepaid);
        prepaid = g_memdup (new, sizeof(GbillingPrepaid));            
    }
    gtk_list_store_set ((GtkListStore *) model, &iter,
                        SCPREPAID_NAME, new->name,
                        SCPREPAID_DURATION, (gint) new->duration / 60,
                        SCPREPAID_COST, new->cost,
                        SCPREPAID_ACTIVE, new->active,
                        -1);
    on_window_update_prepaid_delete_event (NULL, NULL, NULL);
    gbilling_prepaid_free (new);
    g_free (name);
}

gboolean
on_window_update_prepaid_delete_event (GtkWidget *widget,
                                       GdkEvent  *event,
                                       gpointer   data)
{
    gtk_widget_destroy (window_update_prepaid->window);
    g_free (window_update_prepaid);
    return FALSE;
}

gboolean
on_window_setting_item_delete_event (GtkWidget *widget,
                                     GdkEvent  *event,
                                     gpointer   data)
{
    gtk_widget_destroy (window_setting_item->window);
    g_free (window_setting_item);
    return FALSE;
}

void
on_button_add_window_setting_item_clicked (GtkButton *button,
                                           gpointer   data)
{
    create_window_update_item (GBILLING_UPDATE_MODE_ADD);
    gtk_toggle_button_set_active 
        ((GtkToggleButton *) window_update_item->checkb_active, TRUE);
    gtk_widget_show (window_update_item->window);
}

void
on_button_edit_window_setting_item_clicked (GtkButton *button,
                                            gpointer   data)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GbillingItem *item;
    gchar *name;
    
    selection = gtk_tree_view_get_selection ((GtkTreeView *) window_setting_item->treeview);
    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
        return;
    gtk_tree_model_get (model, &iter, SCITEM_NAME, &name, -1);
    item = gbilling_server_get_item_by_name (name);
    g_free (name);
    g_return_if_fail (item != NULL);
    create_window_update_item (GBILLING_UPDATE_MODE_EDIT);
    gtk_entry_set_text ((GtkEntry *) window_update_item->entry_name, item->name);
    gtk_spin_button_set_value ((GtkSpinButton *) window_update_item->spinb_price, item->cost);
    gtk_toggle_button_set_active 
        ((GtkToggleButton *) window_update_item->checkb_active, item->active);
    gtk_widget_show (window_update_item->window);
    gbilling_item_free (item);
}

void
on_button_del_window_setting_item_clicked (GtkButton *button,
                                           gpointer   data)
{
    GbillingItem *item;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar *name;
    gint ret;
    
    selection = gtk_tree_view_get_selection ((GtkTreeView *) window_setting_item->treeview);
    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
        return;
    gtk_tree_model_get (model, &iter, SCITEM_NAME, &name, -1);
    ret = gbilling_create_dialog (GTK_MESSAGE_QUESTION, window_setting_item->window,
                "Hapus Item", "Hapus item dengan nama <b>%s</b> dari server?", name);
    if (ret == GTK_RESPONSE_OK)
    {
        item = get_item (name);
        g_return_if_fail (item != NULL);
        item_list = g_list_remove (item_list, item);
        gbilling_item_free (item);
        gbilling_server_delete_item (name);
        gtk_list_store_remove ((GtkListStore *) model, &iter);
    }
    g_free (name);
}

gboolean
on_window_update_item_delete_event (GtkWidget *widget,
                                    GdkEvent  *event,
                                    gpointer   data)
{
    gtk_widget_destroy (window_update_item->window);
    g_free (window_update_item);
    return FALSE;
}

void
on_button_ok_window_update_item_clicked (GtkButton *button,
                                         gpointer   data)
{
    GbillingItem *item, *new;
    GtkTreeModel *model;
    GtkTreeIter iter;
    const gchar *head[] = { "Tambah Item", "Edit Item" };
    gchar *name;
    gint mode;
   
    mode = GPOINTER_TO_INT (data);
    name = g_strdup (gtk_entry_get_text ((GtkEntry *) window_update_item->entry_name));
    name = g_strstrip (name);
    if (!strlen (name))
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, window_update_item->window, 
                mode == GBILLING_UPDATE_MODE_ADD ? head[0] : head[1], 
                "Silahkan masukkan nama item terlebih dahulu.");
        gtk_entry_set_text ((GtkEntry *) window_update_item->entry_name, "");
        g_free (name);
        return;
    }
    new = gbilling_item_new ();
    g_snprintf (new->name, sizeof(new->name), name);
    new->cost = gtk_spin_button_get_value_as_int ((GtkSpinButton *)
                                                  window_update_item->spinb_price);
    new->active = gtk_toggle_button_get_active ((GtkToggleButton *)
                                       window_update_item->checkb_active);
    
    if (mode == GBILLING_UPDATE_MODE_ADD)
    {
        if (!gbilling_server_add_item (new))
        {
            gbilling_create_dialog (GTK_MESSAGE_WARNING, window_update_item->window, 
                    mode == GBILLING_UPDATE_MODE_ADD ? head[0] : head[1], 
                    "Item dengan nama seperti <b>%s</b> telah ada di server.", new->name);
            gbilling_item_free (new);
            g_free (name);
            return;
        }
        item = g_memdup (new, sizeof(GbillingItem));
        item_list = g_list_append (item_list, item);
        model = gtk_tree_view_get_model ((GtkTreeView *) window_setting_item->treeview);
        gtk_list_store_append ((GtkListStore *) model, &iter);
    }
    else
    {
        GtkTreeSelection *selection;
        
        selection = gtk_tree_view_get_selection ((GtkTreeView *) window_setting_item->treeview);
        gtk_tree_selection_get_selected (selection, &model, &iter);
        g_free (name);
        gtk_tree_model_get (model, &iter, SCITEM_NAME, &name, -1);
        if (!gbilling_server_update_item (new, name))
        {
            gbilling_create_dialog (GTK_MESSAGE_WARNING, window_update_item->window,
                mode == GBILLING_UPDATE_MODE_ADD ? head[0] : head[1], 
                "Item dengan nama seperti <b>%s</b> telah ada di server.", new->name);
            gbilling_item_free (new);
            g_free (name);
            return;
        }
        item = get_item (name);
        g_return_if_fail (item != NULL);
        gbilling_item_free (item);
        item = g_memdup (new, sizeof(GbillingItem));
    }
    gtk_list_store_set ((GtkListStore *) model, &iter,
                        SCITEM_NAME, new->name,
                        SCITEM_COST, new->cost,
                        SCITEM_ACTIVE, new->active,
                        -1);
    gtk_widget_destroy (window_update_item->window);
    g_free (window_update_item);
    gbilling_item_free (new);
    g_free (name);
}

#if 0
void
on_button_add_window_group_member_clicked (GtkButton *button,
                                           gpointer   data)
{
    GbillingCostList *ptr;
    GtkTreeIter iter;
    GbillingCost *cost;
    GtkTreeModel *model;

    window_update_group_member = create_window_update_group_member (GBILLING_UPDATE_MODE_ADD);
    if ((ptr = cost_list))
    {
        model = gtk_combo_box_get_model ((GtkComboBox *) combob_cost_window_update_group_member);
        for (ptr = cost_list; ptr; ptr = ptr->next)
        {
            cost = ptr->data;
            gtk_list_store_append ((GtkListStore *) model, &iter);
            gtk_list_store_set ((GtkListStore *) model, &iter, 0, cost->name, -1);
        }
        gtk_combo_box_set_active ((GtkComboBox *) combob_cost_window_update_group_member, 0);
    }
    else
        gtk_widget_set_sensitive (combob_cost_window_update_group_member, FALSE);
    gtk_widget_show (window_update_group_member);
}
#endif

#if 0
void
on_button_edit_window_group_member_clicked (GtkButton *button,
                                            gpointer   data)
{
    GbillingCostList *ptr;
    GbillingCost *cost;
    GbillingMemberGroup *group;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gchar *name, *cst;
    gint i = 0, cmp;

    selection = gtk_tree_view_get_selection ((GtkTreeView *) treev_window_group_member);
    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
        return;
    gtk_tree_model_get (model, &iter, 
                        CMEMBER_GROUP_NAME, &name, 
                        CMEMBER_GROUP_COST, &cst,
                        -1);
    group = gbilling_server_get_member_group_by_name (name);
    window_update_group_member = create_window_update_group_member (GBILLING_UPDATE_MODE_EDIT);
    gtk_entry_set_text ((GtkEntry *) entry_name_window_update_group_member, group->name);

    for (ptr = cost_list; ptr; ptr = ptr->next)
    {
        cost = ptr->data;
        gtk_combo_box_append_text ((GtkComboBox *) 
            combob_cost_window_update_group_member, cost->name);
    }
    g_free (name);
    /* select combobox based on group->cost */
    model = gtk_combo_box_get_model ((GtkComboBox *) combob_cost_window_update_group_member);
    if (gtk_tree_model_get_iter_first (model, &iter))
    {
        do
        {
            gtk_tree_model_get (model, &iter, 0, &name, -1);
            cmp = strcmp (name, cst);
            g_free (name);
            i++;
        } while (cmp && gtk_tree_model_iter_next (model, &iter));
        gtk_combo_box_set_active ((GtkComboBox *) combob_cost_window_update_group_member, --i);
    }
    gtk_widget_show (window_update_group_member);
    g_free (cst);
}
#endif

#if 0
void
on_button_delete_window_group_member_clicked (GtkButton *button,
                                              gpointer   data)
{
    GtkTreeModel *model;
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    GbillingMemberGroup *group;
    gchar *name;
    gint ret;

    model = gtk_tree_view_get_model ((GtkTreeView *) treev_window_group_member);
    selection = gtk_tree_view_get_selection ((GtkTreeView *) treev_window_group_member);
    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
        return;
    /* konfirmasi, hapus semua member dalam group! */
    gtk_tree_model_get (model, &iter, CMEMBER_GROUP_NAME, &name, -1);
    ret = gbilling_create_dialog (GTK_MESSAGE_QUESTION, window_group_member,
                "Hapus Grup", "Grup member dengan nama \"%s\" akan dihapus beserta "
                "semua member yang termasuk dalam grup ini, hapus grup ini "
                "dari server?", name);
    if (ret != GTK_RESPONSE_OK)
    {
        g_free (name);
        return;
    }
    gbilling_server_delete_member_group (name);
    group = get_group (name);
    group_list = g_list_remove (group_list, group);
    gbilling_member_group_free (group);
    gtk_list_store_remove ((GtkListStore *) model, &iter);
    g_free (name);
    /* TODO: delete all members referenced by this group */
}
#endif

#if 0
void
on_button_ok_window_update_group_member_clicked (GtkButton *button,
                                                 gpointer   data)
{
    GbillingMemberGroup *group, *new;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    const gchar *tmp;
    gchar *name, *cost;
    gint mode, total;

    tmp = gtk_entry_get_text ((GtkEntry *) entry_name_window_update_group_member);
    name = g_strstrip (g_strdup (tmp));
    cost = gtk_combo_box_get_active_text ((GtkComboBox *) combob_cost_window_update_group_member);
    if (!strlen (name) || !cost)
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, (GtkWindow *) window_update_group_member,
                "Tambah Grup", "Silahkan isi nama grup dan pilih tarif member.");
        g_free (name);
        return;
    }

    new = gbilling_member_group_new ();
    g_snprintf (new->name, sizeof(new->name), name);
    g_snprintf (new->cost, sizeof(new->cost), cost);
    g_free (cost);

    mode = GPOINTER_TO_INT (data);
    if (mode == GBILLING_UPDATE_MODE_ADD)
    {
        if (!gbilling_server_add_member_group (new))
        {
            gbilling_create_dialog (GTK_MESSAGE_WARNING, (GtkWindow *) window_update_group_member,
                "Tambah Group", "Nama grup seperti \"%s\" telah ada dalam server.", new->name);
            gbilling_member_group_free (new);
            g_free (name);
            return;
        }
        cost = g_memdup (new, sizeof(GbillingMemberGroup));
        group_list = g_list_append (group_list, cost);
        model = gtk_tree_view_get_model ((GtkTreeView *) treev_window_group_member);
        gtk_list_store_append ((GtkListStore *) model, &iter);
    }
    else
    {
        selection = gtk_tree_view_get_selection ((GtkTreeView *) treev_window_group_member);
        gtk_tree_selection_get_selected (selection, &model, &iter);
        g_free (name);
        gtk_tree_model_get (model, &iter, CMEMBER_GROUP_NAME, &name, -1);
        if (!gbilling_server_update_member_group (new, name))
        {
            gbilling_create_dialog (GTK_MESSAGE_WARNING, (GtkWindow *) window_update_group_member,
                "Tambah Group", "Nama grup seperti \"%s\" telah ada dalam server.", new->name);
            gbilling_member_group_free (new);
            g_free (name);
            return;
        }
        group = get_group (name);
        memcpy (group, new, sizeof(GbillingMemberGroup));
    }
    total = gbilling_server_get_member_total (name);
    gtk_list_store_set ((GtkListStore *) model, &iter,
                        CMEMBER_GROUP_NAME, new->name,
                        CMEMBER_GROUP_COST, new->cost,
                        CMEMBER_GROUP_NMEMBER, total,
                        -1);
    gtk_widget_destroy (window_update_group_member);
    gbilling_member_group_free (new);
    g_free (name);
}
#endif

#if 0
void
on_treev_window_member_column_clicked (GtkTreeViewColumn *column,
                                       gpointer           data)
{
    GtkListStore *store;
    GtkTreeSortable *sortable;
    GtkSortType order, neworder;
    gint sortid, col;

    store = (GtkListStore *) gtk_tree_view_get_model ((GtkTreeView *) treev_window_member);
    sortable = (GtkTreeSortable *) store;
    if (gtk_tree_sortable_get_sort_column_id (sortable, &sortid, &order))
    {
        neworder = (order == GTK_SORT_ASCENDING) ? GTK_SORT_DESCENDING : GTK_SORT_ASCENDING;
        col = GPOINTER_TO_INT (data);
        gtk_tree_sortable_set_sort_column_id (sortable, col, neworder);
    }
}
#endif

#if 0
void
on_button_window_member_add_clicked (GtkButton *button,
                                     gpointer   data)
{
    GbillingMemberGroupList *list, *ptr;
    GbillingMemberGroup *group;
    
    window_umember = create_window_umember (GBILLING_UPDATE_MODE_ADD);
    list = gbilling_server_get_member_group_list ();
    for (ptr = list; ptr; ptr = g_list_next (ptr))
    {
        group = ptr->data;
        gtk_combo_box_append_text ((GtkComboBox *) combob_window_umember_group, group->name);
    }
    gtk_toggle_button_set_active ((GtkToggleButton *) checkb_window_umember_status, TRUE);
    if (list)
        gtk_combo_box_set_active ((GtkComboBox *) combob_window_umember_group, 0);
    gtk_widget_show (window_umember);
    gbilling_member_group_list_free (list);
}
#endif

#if 0
void
on_button_window_member_edit_clicked (GtkButton *button,
                                      gpointer   data)
{
    GtkTreeModel *model;
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    GbillingMemberGroupList *ptr;
    GbillingMemberGroup *mgroup;
    GbillingMember *member;
    gchar *username, *group, *date;
    gint pos;

    model = gtk_tree_view_get_model ((GtkTreeView *) treev_window_member);
    selection = gtk_tree_view_get_selection ((GtkTreeView *) treev_window_member);
    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
        return;
    gtk_tree_model_get (model, &iter,
                        CMEMBER_USERNAME, &username,
                        CMEMBER_GROUP, &group,
                        -1);
    member = gbilling_server_get_member (username, group);
    g_free (username);
    g_free (group);
    g_return_if_fail (member != NULL); /* you still need this? */

    window_umember = create_window_umember (GBILLING_UPDATE_MODE_EDIT);
    /* set combob_window_umember_group */
    if ((ptr = group_list))
    {
        while (ptr)
        {
            mgroup = ptr->data;
            ptr = ptr->next;
            gtk_combo_box_append_text ((GtkComboBox *) combob_window_umember_group, 
                                       mgroup->name);
        }
    }
    /* pilih group di combob_window_umember_group */
    mgroup = get_group (member->group);
    pos = g_list_index (group_list, mgroup);
    gtk_combo_box_set_active ((GtkComboBox *) combob_window_umember_group, pos);

    date = time_t_to_date (member->reg);
    gtk_label_set_text ((GtkLabel *) label_window_umember_register, date);
    gtk_toggle_button_set_active
        ((GtkToggleButton *) checkb_window_umember_status, member->status);
    gtk_entry_set_text ((GtkEntry *) entry_window_umember_username, member->username);

    gtk_combo_box_set_active ((GtkComboBox *) combob_window_umember_group, pos);
    gtk_entry_set_text ((GtkEntry *) entry_window_umember_pass, member->pass);
    gtk_entry_set_text ((GtkEntry *) entry_window_umember_passc, member->pass);
    gtk_entry_set_text ((GtkEntry *) entry_window_umember_fullname, member->fullname);
    gtk_entry_set_text ((GtkEntry *) entry_window_umember_address, member->address);
    gtk_entry_set_text ((GtkEntry *) entry_window_umember_phone, member->phone);
    gtk_entry_set_text ((GtkEntry *) entry_window_umember_email, member->email);
    gtk_entry_set_text ((GtkEntry *) entry_window_umember_idcard, member->idcard);
    gtk_widget_show (window_umember);
    
    g_free (date);
    gbilling_member_free (member);
}
#endif

#if 0
void
on_button_window_member_del_clicked (GtkButton *button,
                                     gpointer   data)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreeSelection *selection;
    GtkTreeRowReference *reference;
    GList *row, *ptr, *ref = NULL;
    GbillingMember *member;
    gchar *username, *group;
    gint ret;

    model = gtk_tree_view_get_model ((GtkTreeView *) treev_window_member);
    selection = gtk_tree_view_get_selection ((GtkTreeView *) treev_window_member);
    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
        return;
    gtk_tree_model_get (model, &iter,
                        CMEMBER_USERNAME, &username,
                        CMEMBER_GROUP, &group,
                        -1);
    ret = gbilling_create_dialog (GTK_MESSAGE_QUESTION, window_member,
                "Hapus Member", "Hapus member \"%s\" dengan group \"%s\" "
                "dari server?", username, group);
    if (ret != GTK_RESPONSE_OK)
    {
        g_free (username);
        g_free (group);
        return;
    }
    member = gbilling_get_member (username, group);
    /* Harusnya @member tidak pernah NULL, no handling... :) */
    g_free (username);
    g_free (group);
    gbilling_member_update (member, NULL, GBILLING_UPDATE_MODE_DELETE);
    gbilling_member_free (member);

    row = gtk_tree_selection_get_selected_rows (selection, &model);
    ptr = row;
    while (ptr)
    {
        reference = gtk_tree_row_reference_new (model, (GtkTreePath *) ptr->data);
        ref = g_list_prepend (ref, gtk_tree_row_reference_copy (reference));
        gtk_tree_row_reference_free (reference);
        ptr = ptr->next;
    }
    g_list_foreach (ref, (GFunc) foreach_remove_row, model);
    g_list_foreach (ref, (GFunc) gtk_tree_row_reference_free, NULL);
    g_list_foreach (row, (GFunc) gtk_tree_path_free, NULL);
    g_list_free (ref);
    g_list_free (row);
}
#endif

#if 0
void
on_button_window_member_search_clicked (GtkButton *button,
                                        gpointer   data)
{
    GbillingMemberGroupList *glist, *tmp;
    GbillingMemberList *mlist;
    GbillingMemberGroup *group;
    GbillingMember *member;
    GtkTreeModel *model;
    gchar *date;

    if (!window_member_search)
        window_member_search = create_window_member_search ();
    /* set @window_member_search_register2, cari tanggal member pertama */
    mlist = gbilling_server_get_member_list (NULL);
    if (mlist)
    {
        member = mlist->data;
        date = time_t_to_date (member->reg);
    }
    else
        date = time_t_to_date (time (NULL));
    gtk_button_set_label ((GtkButton *) toggleb_window_member_search_register1, date);
    g_free (date);
    date = time_t_to_date (time (NULL));
    gtk_button_set_label ((GtkButton *) toggleb_window_member_search_register2, date);

    /* selalu ulangi masukan data group, bisa saja group di update
     * tapi combobox ini tidak diupdate krn window_member_search
     * tidak akan di destroy setelah ter-inisialisasi
     */
    glist = gbilling_server_get_member_group_list ();
    if ((tmp = glist))
    {
        model = gtk_combo_box_get_model ((GtkComboBox *) combob_window_member_search_group);
        gtk_list_store_clear ((GtkListStore *) model);
        while (tmp)
        {
            group = tmp->data;
            tmp = tmp->next;
            gtk_combo_box_append_text
                ((GtkComboBox *) combob_window_member_search_group, group->name);
        }
    }
    gtk_widget_show (window_member_search);
    
    gbilling_member_group_list_free (glist);
    gbilling_member_list_free (mlist);
    g_free (date);
}
#endif

#if 0
void
on_window_umember_destroy (GtkWidget *widget,
                           gpointer   data)
{
    /* set state "changed" entry utk callback on_button_window_umember_ok_clicked() */
    entry_window_umember_pass_changed = FALSE;
    gtk_widget_destroy (window_umember);
}
#endif

#if 0
void
on_entry_window_umember_pass_changed (GtkEntry *entry,
                                      gpointer  data)
{
    /* Signal "changed" entry akan di-emit jika @window_umember di-show
     * karena insertion pada entry. Return callback ini sebelum entry
     * di-show (realized).
     */
    if (!GTK_WIDGET_REALIZED ((GtkWidget *) entry))
        return;
    /* set state kembali */
    entry_window_umember_pass_changed = TRUE;
    gtk_entry_set_text ((GtkEntry *) entry_window_umember_passc, "");
}
#endif

#if 0
void
on_button_window_umember_ok_clicked (GtkButton *button,
                                     gpointer   data)
{
    GbillingMember *new = NULL, *old = NULL;
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gint mode;
    gboolean status;
    gchar *tmp_name = NULL, *tmp_pass = NULL, *tmp_passc = NULL, *date;
    const gchar *username, *pass, *passc, *fullname,
                *address, *phone, *email, *idcard;
    const gchar *head[] = { "Tambah Member", "Edit Member" };
    
    mode = GPOINTER_TO_INT (data);
    status = gtk_toggle_button_get_active ((GtkToggleButton *) checkb_window_umember_status);
    username = gtk_entry_get_text ((GtkEntry *) entry_window_umember_username);
    pass = gtk_entry_get_text ((GtkEntry *) entry_window_umember_pass);
    passc = gtk_entry_get_text ((GtkEntry *) entry_window_umember_passc);
    fullname = gtk_entry_get_text ((GtkEntry *) entry_window_umember_fullname);
    address = gtk_entry_get_text ((GtkEntry *) entry_window_umember_address);
    phone = gtk_entry_get_text ((GtkEntry *) entry_window_umember_phone);
    email = gtk_entry_get_text ((GtkEntry *) entry_window_umember_email);
    idcard = gtk_entry_get_text ((GtkEntry *) entry_window_umember_idcard);
    
    tmp_name = g_strstrip (g_strdup (username));
    if (!strlen (tmp_name))
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, window_umember, head[mode],
            "Silahkan isi username member terlebih dahulu.");
        goto cleanup;
    }
    
    tmp_pass = g_strstrip (g_strdup (pass));
    tmp_passc = g_strstrip (g_strdup (passc));
    if (!strlen (tmp_pass) || !strlen (tmp_passc))
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, window_umember, head[mode],
                "Silahkan isi password dan konfirmasi password member.");
        goto cleanup;
    }

    if (strcmp (tmp_pass, tmp_passc))
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, window_umember, head[mode],
                "Password dan konfirmasi password member tidak sama.");
        goto cleanup;
    }
    
    new = gbilling_member_new ();
    new->reg = time (NULL);
    new->status = gtk_toggle_button_get_active ((GtkToggleButton *) 
                        checkb_window_umember_status);
    new->username = g_strstrip (g_strdup (tmp_name));
    new->group = gtk_combo_box_get_active_text ((GtkComboBox *) 
                    combob_window_umember_group);
    new->fullname = g_strstrip (g_strdup (fullname));
    new->address = g_strstrip (g_strdup (address));
    new->phone = g_strstrip (g_strdup (phone));
    new->email = g_strstrip (g_strdup (email));
    new->idcard = g_strstrip (g_strdup (idcard));

    if (mode == GBILLING_UPDATE_MODE_ADD)
    {
        new->pass = gbilling_str_checksum (tmp_pass);
        if (!gbilling_server_add_member (new))
        {
            gbilling_create_dialog (GTK_MESSAGE_WARNING, window_umember, head[mode],
                "Username seperti \"%s\" telah ada dalam grup \"%s\", silahkan "
                "pilih username atau grup yang lain.", new->username, new->group);
            goto cleanup;
        }
        model = gtk_tree_view_get_model ((GtkTreeView *) treev_window_member);
        gtk_list_store_append ((GtkListStore *) model, &iter);
    }
    else
    {
        gchar *sname, *sgroup;
        
        selection = gtk_tree_view_get_selection ((GtkTreeView *) treev_window_member);
        gtk_tree_selection_get_selected (selection, &model, &iter);
        old = gbilling_member_new ();
        gtk_tree_model_get (model, &iter,
                            CMEMBER_USERNAME, &sname,
                            CMEMBER_GROUP, &sgroup,
                            -1);
        old = gbilling_server_get_member (sname, sgroup);
        g_free (sname);
        g_free (sgroup);
        /* set password, lihat on_entry_window_umember_pass_changed() */
        if (entry_window_umember_pass_changed)
            new->pass = gbilling_str_checksum (tmp_pass);
        else
            new->pass = g_strdup (old->pass);
        if (!gbilling_server_update_member (new, old))
        {
            gbilling_create_dialog (GTK_MESSAGE_WARNING, window_umember, head[mode],
                "Username seperti \"%s\" telah ada dalam grup \"%s\", silahkan "
                "pilih username atau grup yang lain.", new->username, new->group);
            goto cleanup;
        }
    }

    date = time_t_to_date (new->reg);
    gtk_list_store_set ((GtkListStore *) model, &iter,
                        CMEMBER_USERNAME, new->username,
                        CMEMBER_GROUP, new->group,
                        CMEMBER_REGISTER, date,
                        CMEMBER_STATUS, new->status,
                        -1);
    g_free (date);
    gtk_widget_destroy (window_umember);
    
    cleanup:
    gbilling_member_free (new);
    gbilling_member_free (old);
    g_free (tmp_name);
    g_free (tmp_pass);
    g_free (tmp_passc);
}
#endif

#if 0
gboolean
on_window_member_search_delete_event (GtkWidget *widget,
                                      GdkEvent  *event,
                                      gpointer   data)
{
    gtk_widget_hide (widget);
    return TRUE; /* re-invoke */
}
#endif

#if 0
void
on_checkb_window_member_search_toggled (GtkToggleButton *toggleb,
                                        gpointer         data)
{
    gtk_widget_set_sensitive (GTK_WIDGET (data), gtk_toggle_button_get_active (toggleb));
}
#endif

#if 0
void
on_checkb_window_member_search_register1_toggled (GtkToggleButton *toggleb,
                                                  gpointer         data)
{
    gboolean toggled;

    toggled = gtk_toggle_button_get_active (toggleb);
    gtk_widget_set_sensitive (toggleb_window_member_search_register1, toggled);
    gtk_widget_set_sensitive (checkb_window_member_search_register2, toggled);
    if (!GTK_WIDGET_SENSITIVE (checkb_window_member_search_register2))
        gtk_widget_set_sensitive (toggleb_window_member_search_register2, FALSE);
    else
        if (gtk_toggle_button_get_active ((GtkToggleButton *)
                checkb_window_member_search_register2))
            gtk_widget_set_sensitive (toggleb_window_member_search_register2, TRUE);
}
#endif

#if 0
void
on_toggleb_window_member_search_register1_toggled (GtkToggleButton *toggleb,
                                                   gpointer         data)
{
    g_return_if_fail (data != NULL);

    GtkToggleButton *toggle;
    const gchar *date;
    gchar **set;
    guint len, d, m, y;

    toggle = data;
    if (!gtk_toggle_button_get_active (toggle))
        return;

    date = gtk_button_get_label ((GtkButton *) toggle);
    set = g_strsplit_set (date, "-", 3);
    len = g_strv_length (set);
    if (len != 3)
    {
        g_strfreev (set);
        g_return_if_fail (len != 3);
    }
    d = atoi (set[0]); /* lihat on_toggleb_wlogfdate_toggled() */
    if (!d)
    {
        d = atoi (++set[0]);
        set[0]--;
    }
    m = atoi (set[1]);
    if (!m)
    {
        m = atoi (++set[1]);
        set[1]--;
    }
    y = atoi (set[2]);
    g_strfreev (set);
    window_calendar_member = create_window_calendar_member (window_member_search);
    gtk_calendar_select_day ((GtkCalendar *) calendar_window_calendar_member, d);
    gtk_calendar_select_month ((GtkCalendar *)
            calendar_window_calendar_member, m - 1, y);
    gtk_widget_show (window_calendar_member);
}
#endif

#if 0
void
on_checkb_window_member_search_status_toggled (GtkToggleButton *toggleb,
                                               gpointer         data)
{
    gboolean toggled;

    toggled = gtk_toggle_button_get_active (toggleb);
    gtk_widget_set_sensitive (radiob_window_member_search_active, toggled);
    gtk_widget_set_sensitive (radiob_window_member_search_inactive, toggled);
}
#endif

#if 0
void
on_button_window_member_search_ok_clicked (GtkButton *button,
                                           gpointer   data)
{
    GbillingMemberList *list, *ptr;
    GbillingMember *member;
    GtkTreeModel *model;
    GtkTreeIter iter;
    time_t reg_start = 0L, reg_end = time (NULL);
    time_t now = reg_end;
    gboolean status, active = FALSE;
    gboolean check;
    const gchar *label, *username = "%";
    gchar *group, *cmd, *date;

    check = gtk_toggle_button_get_active ((GtkToggleButton *)
                checkb_window_member_search_register1);
    if (check)
    {
        label = gtk_button_get_label ((GtkButton *)
                    toggleb_window_member_search_register1);
        reg_start = date_to_time_t (label); /* detik pertama hari tsb. */
        check = gtk_toggle_button_get_active ((GtkToggleButton *)
                    checkb_window_member_search_register2);
    }
    if (check)
    {
        label = gtk_button_get_label ((GtkButton *)
                    toggleb_window_member_search_register2);
        reg_end = date_to_time_t (label);
    }
    /* cek validitas tanggal */
    if (reg_start > now || reg_end > now)
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, window_member_search,
            "Pencarian Member", "Tanggal yang dipilih salah, pastikan untuk "
            "memilih tanggal yang belum dilewati.");
        return;
    }
    reg_end += (3600 * 24) - 1; /* detik terakhir hari tsb. */
    if (reg_end < reg_start)
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, window_member_search,
            "Pencarian Member", "Interval waktu pencarian tidak benar.");
        return;
    }

    status = gtk_toggle_button_get_active ((GtkToggleButton *)
                checkb_window_member_search_status);
    if (status)
        active = gtk_toggle_button_get_active ((GtkToggleButton *)
                    radiob_window_member_search_active);

    check = gtk_toggle_button_get_active ((GtkToggleButton *)
                checkb_window_member_search_username);
    if (check)
        username = gtk_entry_get_text ((GtkEntry *) entry_window_member_search_username);

    check = gtk_toggle_button_get_active ((GtkToggleButton *)
                checkb_window_member_search_group);
    if (check)
        group = gtk_combo_box_get_active_text ((GtkComboBox *)
                    combob_window_member_search_group);
    else
        group = g_strdup ("%");

    cmd = g_strdup_printf ("SELECT * FROM "DBTABLE_MEMBER" WHERE "
                           ""DBMEMBER_USERNAME"  LIKE '%s' AND   "
                           ""DBMEMBER_GROUP"     LIKE '%s' AND   "
                           ""DBMEMBER_REGISTER"    >= %li  AND   "
                           ""DBMEMBER_REGISTER"    <= %li  AND   "
                           ""DBMEMBER_STATUS"      >= %i   AND   "
                           ""DBMEMBER_STATUS"      <= %i         ",
                            username, group, reg_start, reg_end,
                            status ? (active ? 1 : 0) : 0,
                            status ? (active ? 1 : 0) : 1
                          );
    model = gtk_tree_view_get_model ((GtkTreeView *) treev_window_member);
    gtk_list_store_clear ((GtkListStore *) model); /* clear sebelum insertion */
    list = gbilling_server_get_member_list (cmd);
    for (ptr = list; ptr; ptr = g_list_next (ptr))
    {
        member = ptr->data;
        date = time_t_to_date (member->reg);
        gtk_list_store_append ((GtkListStore *) model, &iter);
        gtk_list_store_set ((GtkListStore *) model, &iter,
                            CMEMBER_USERNAME, member->username,
                            CMEMBER_GROUP, member->group,
                            CMEMBER_REGISTER, date,
                            CMEMBER_STATUS, member->status,
                            -1);
        g_free (date);
    }
    gtk_widget_hide (window_member_search);
    gbilling_member_list_free (list);
    g_free (cmd);
}
#endif

#if 0
gboolean
on_window_calendar_member_delete_event (GtkWidget *widget,
                                        GdkEvent  *event,
                                        gpointer   data)
{
    gtk_toggle_button_set_active ((GtkToggleButton *)
        toggleb_window_member_search_register1, FALSE);
    gtk_toggle_button_set_active ((GtkToggleButton *)
        toggleb_window_member_search_register2, FALSE);
    gtk_widget_destroy (widget);

    return FALSE;
}
#endif

#if 0
void
on_calendar_window_calendar_member_day_selected_double_click (GtkCalendar *calendar,
                                                              gpointer     data)
{
    gpointer button;
    gboolean toggled;
    gchar *label;
    guint d, m, y;

    /* toggle button register1 dan register2 salah satunya akan aktif,
     * tidak pernah button2 ini aktif bersamaan */
    toggled = gtk_toggle_button_get_active ((GtkToggleButton *)
                    toggleb_window_member_search_register1);
    if (toggled)
        button = toggleb_window_member_search_register1;
    else
        button = toggleb_window_member_search_register2;

    gtk_calendar_get_date ((GtkCalendar *) calendar_window_calendar_member, &y, &m, &d);
    label = g_strdup_printf ("%.2u-%.2u-%.2u", d, m + 1, y);
    gtk_button_set_label ((GtkButton *) button, label);
    gtk_toggle_button_set_active ((GtkToggleButton *) button, FALSE);
    gtk_widget_destroy (window_calendar_member);
    g_free (label);
}
#endif

gboolean
on_wchat_window_key_press_event (GtkWidget   *widget,
                                 GdkEventKey *event,
                                 ClientData  *cdata)
{
    /* Lihat gdk/gdkkeysyms.h */
    if (event->keyval == GDK_Return || event->keyval == GDK_KP_Enter)
        gtk_widget_grab_focus (cdata->wchat->button_chat);
    return FALSE;
}

void
on_wchat_bsend_clicked (GtkWidget  *widget,
                        ClientData *cdata)
{
    GtkTextBuffer *buf;
    GtkTextIter start, end;
    gchar *tmp;

    buf = gtk_text_view_get_buffer ((GtkTextView *) cdata->wchat->textv_msg);
    gtk_text_buffer_get_bounds (buf, &start, &end);
    tmp = gtk_text_buffer_get_text (buf, &start, &end, FALSE);
    if (strlen (g_strstrip (tmp)))
    {
        set_servcmd (cdata, GBILLING_COMMAND_CHAT, tmp);
        gtk_text_buffer_delete (buf, &start, &end);
        fill_chat_log (cdata, tmp, GBILLING_CHAT_MODE_SERVER);
    }

    gtk_widget_grab_focus (cdata->wchat->textv_msg);

    if (current_soundopt[GBILLING_EVENT_CHAT_SEND].play && !server->mute_sound)
        sound_play (GBILLING_EVENT_CHAT_SEND);

    g_free (tmp);
}

gboolean
on_window_logf_delete_event (GtkWidget *widget,
                             GdkEvent  *event,
                             gpointer   data)
{
    gtk_widget_hide (window_search->window);
    return TRUE;
}

void
on_button_wlogfok_clicked (GtkButton *button,
                           gpointer   data)
{
    GtkTreeModel *model;
    time_t c = time (NULL);
    time_t s = 0L;
    time_t e = G_MAXLONG;
    gboolean date1, date2;
    glong cs = 0L;
    glong ce = G_MAXLONG;
    glong st = 0L;
    glong et = 0L;
    gchar *client = NULL;
    gchar *type = NULL;
    const gchar *user = NULL;

    date1 = gtk_toggle_button_get_active ((GtkToggleButton *) window_search->checkb_date_start);
    if (date1)
        s = date_to_time_t (gtk_button_get_label ((GtkButton *) window_search->toggleb_date_start));
    date2 = gtk_toggle_button_get_active ((GtkToggleButton *) window_search->checkb_date_end);
    if (date2)
    {
        e = date_to_time_t (gtk_button_get_label ((GtkButton *) window_search->toggleb_date_end));
        e = e + (3600 * 24) - 1; /* ke detik terakhir hari */
    }
    if ((date1 && s > c) || (date2 && e - (3600 * 24) + 1 > c))
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, (GtkWindow *) window_search->window,
              "Filter Log", "Interval tanggal salah atau tanggal telah dilewati.");
        return;
    }

    if (gtk_toggle_button_get_active ((GtkToggleButton *) window_search->checkb_username))
        user = gtk_entry_get_text ((GtkEntry *) window_search->entry_username);
    if (gtk_toggle_button_get_active ((GtkToggleButton *) window_search->checkb_cost_start))
        cs = gtk_spin_button_get_value_as_int ((GtkSpinButton *) window_search->spinb_cost_start);
    if (gtk_toggle_button_get_active ((GtkToggleButton *) window_search->checkb_cost_end))
        ce = gtk_spin_button_get_value_as_int ((GtkSpinButton *) window_search->spinb_cost_end);
    if (cs > ce)
    {
        gbilling_create_dialog (GTK_MESSAGE_WARNING, (GtkWindow *) window_search->window,
                "Filter Log", "Tarif pertama lebih besar daripada tarif kedua.");
        return;
    }

    if (gtk_toggle_button_get_active ((GtkToggleButton *) window_search->checkb_type))
        type = gtk_combo_box_get_active_text ((GtkComboBox *) window_search->combob_type);

    if (gtk_toggle_button_get_active ((GtkToggleButton *) window_search->checkb_client))
        client = gtk_combo_box_get_active_text ((GtkComboBox *) window_search->combob_client);

    if (gtk_toggle_button_get_active ((GtkToggleButton *) window_search->checkb_time_start))
        st = string_to_sec (gtk_button_get_label ((GtkButton *) window_search->toggleb_time_start));
    if (gtk_toggle_button_get_active ((GtkToggleButton *) window_search->checkb_time_end))
        et = string_to_sec (gtk_button_get_label ((GtkButton *) window_search->toggleb_time_end));

    model = gtk_tree_view_get_model ((GtkTreeView *) window_main->treeview_log);
    gtk_list_store_clear ((GtkListStore *) model);

    gbilling_log_filter_show (s, e, st, et, cs, ce, client, user, type);
    g_free (client);
    g_free (type);
    on_window_logf_delete_event (NULL, NULL, NULL);
}

void
on_checkb_wlogfdate_toggled (GtkToggleButton *toggleb,
                             gpointer         data)
{
    gboolean state;

    state = gtk_toggle_button_get_active (toggleb);
    gtk_widget_set_sensitive (window_search->toggleb_date_start, state);
    gtk_widget_set_sensitive (window_search->checkb_date_end, state);
    if (!GTK_WIDGET_SENSITIVE (window_search->checkb_date_end))
        gtk_widget_set_sensitive (window_search->toggleb_date_end, FALSE);
    else
        if (gtk_toggle_button_get_active ((GtkToggleButton *) 
                                          window_search->checkb_date_end))
            gtk_widget_set_sensitive (window_search->toggleb_date_end, TRUE);
    /* enjoy ;p */
}

void
on_toggleb_wlogfdate_toggled (GtkToggleButton *toggleb,
                              gpointer         data)
{
    gchar **set;
    gint m;

    if (gtk_toggle_button_get_active (toggleb))
    {
        create_window_calendar ((GtkWindow *) window_search->window);
        /* set widget2 di window_calendar dan window_time sesuai
         * label toggleb
         */
        set = g_strsplit_set (gtk_button_get_label ((GtkButton *) toggleb), "-", 3);
        /* krn d == 0 invalid, calendar akan memilih waktu yg sebelumnya
         * di set, d tidak perlu dicek!
         */
        gtk_calendar_select_day ((GtkCalendar *) window_calendar->calendar, atoi (set[0]));
        m = atoi (set[1]);
        if (m == 0) /* mis. set[i] = "04", atoi (set[i]) = 0 */
        {
            m = atoi (++set[1]);
            set[1]--;
        }
        gtk_calendar_select_month ((GtkCalendar *) window_calendar->calendar, m - 1, atoi (set[2]));
        gtk_widget_show (window_calendar->window);
        g_strfreev (set);
    }
    else
        gtk_widget_destroy (window_calendar->window);
}

void
on_checkb_wlogfdate2_toggled (GtkToggleButton *toggleb,
                              gpointer         data)
{
    gtk_widget_set_sensitive (window_search->toggleb_date_end,
         gtk_toggle_button_get_active ((GtkToggleButton *) toggleb));
}

void
on_checkb_wlogfclient_toggled (GtkToggleButton *toggleb,
                               gpointer         data)
{
    gtk_widget_set_sensitive (window_search->combob_client,
           gtk_toggle_button_get_active (toggleb));
}

void
on_checkb_username_window_search_toggled (GtkToggleButton *toggleb,
                                          gpointer         data)
{
    gtk_widget_set_sensitive (window_search->entry_username,
            gtk_toggle_button_get_active (toggleb));
}

void
on_checkb_wlogftype_toggled (GtkToggleButton *toggleb,
                             gpointer         data)
{
    gtk_widget_set_sensitive (window_search->combob_type,
           gtk_toggle_button_get_active (toggleb));
}

void
on_checkb_wlogfstart_toggled (GtkToggleButton *toggleb,
                              gpointer         data)
{
    gboolean state;

    state = gtk_toggle_button_get_active (toggleb);
    gtk_widget_set_sensitive (window_search->toggleb_time_start, state);
    gtk_widget_set_sensitive (window_search->checkb_time_end, state);
    if (!GTK_WIDGET_SENSITIVE(window_search->checkb_time_end))
        gtk_widget_set_sensitive (window_search->toggleb_time_end, FALSE);
    else
        if (gtk_toggle_button_get_active ((GtkToggleButton *) 
                                          window_search->checkb_time_end))
            gtk_widget_set_sensitive (window_search->toggleb_time_end, TRUE);
}

void
on_toggleb_wlogfstart_toggled (GtkToggleButton *toggleb,
                               gpointer         data)
{
    gchar **set;
    gint h, m, s;

    if (gtk_toggle_button_get_active ((GtkToggleButton *) data))
    {
        set = g_strsplit_set (gtk_button_get_label ((GtkButton *) data), ":", 3);
        h = atoi (set[0]);
        m = atoi (set[1]);
        s = atoi (set[2]);
        if (h == 0)
        {
            h = atoi (++set[0]);
            set[0]--;
        }
        if (m == 0)
        {
            m = atoi (++set[1]);
            set[1]--;
        }
        if (s == 0)
        {
            s = atoi (++set[2]);
            set[2]--;
        }
        g_strfreev (set);
        create_window_time ((GtkWindow *) window_search->window);
        gtk_spin_button_set_value ((GtkSpinButton *) window_time->spinb_hour, h);
        gtk_spin_button_set_value ((GtkSpinButton *) window_time->spinb_min, m);
        gtk_spin_button_set_value ((GtkSpinButton *) window_time->spinb_sec, s);
        gtk_widget_show (window_time->window);
    }
    else
        gtk_widget_hide (window_time->window);
}

void
on_checkb_wlogfend_toggled (GtkToggleButton *toggleb,
                            gpointer         data)
{
    gtk_widget_set_sensitive (window_search->toggleb_time_end,
                gtk_toggle_button_get_active ((GtkToggleButton *) toggleb));
}

void
on_checkb_wlogfcost_toggled (GtkToggleButton *toggleb,
                             gpointer         data)
{
    gboolean state;

    state = gtk_toggle_button_get_active (toggleb);
    gtk_widget_set_sensitive (window_search->spinb_cost_start, state);
    gtk_widget_set_sensitive (window_search->checkb_cost_end, state);
    if (!GTK_WIDGET_SENSITIVE(window_search->checkb_cost_end))
        gtk_widget_set_sensitive (window_search->spinb_cost_end, FALSE);
    else
        if (gtk_toggle_button_get_active ((GtkToggleButton *)
                                          window_search->checkb_cost_end))
            gtk_widget_set_sensitive (window_search->spinb_cost_end, TRUE);
}

void
on_checkb_wlogfcost2_toggled (GtkToggleButton *toggleb,
                              gpointer         data)
{
    gtk_widget_set_sensitive (window_search->spinb_cost_end,
            gtk_toggle_button_get_active ((GtkToggleButton *) toggleb));
}

gboolean
on_window_detail_delete_event (GtkWidget *widget,
                               GdkEvent  *event,
                               gpointer   data)
{
    gtk_widget_destroy (window_detail->window);
    g_free (window_detail);
    return FALSE;
}

gboolean
on_window_logtotal_delete_event (GtkWidget *widget, 
                                 GdkEvent  *event,
                                 gpointer   data)
{
    gtk_widget_destroy (window_total->window);
    g_free (window_total);
    return FALSE;
}

gboolean
on_window_move_delete_event (GtkWidget *widget,
                             GdkEvent  *event,
                             gpointer   data)
{
    gtk_widget_destroy (window_move->window);
    g_free (window_move);
    return FALSE;
}

void
on_button_moveok_clicked (GtkWidget *widget,
                          gpointer   data)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter iter;
    ClientData *csource, *cdest;
    gchar *name;
    gboolean selected;
    gint response;

#define CHECK_CSOURCE \
({ \
    if (csource->status != GBILLING_STATUS_LOGIN) \
    { \
        gbilling_create_dialog (GTK_MESSAGE_WARNING, (GtkWindow *) window_main->window, \
                                "Pindah Client", "Client <b>%s</b> sedang " \
                                "tidak login.", csource->name); \
        return; \
    } \
})

#define CHECK_CDEST \
({ \
    if (!cdest->active || cdest->status == GBILLING_STATUS_LOGIN) \
    { \
        gbilling_create_dialog (GTK_MESSAGE_WARNING, (GtkWindow *) window_main->window, \
                                "Pindah Client", "Client <b>%s</b> sedang tidak " \
                                "aktif atau sedang login.", cdest->name); \
        return; \
    } \
})

    selection = gtk_tree_view_get_selection ((GtkTreeView *) window_main->treeview_client);
    selected = gtk_tree_selection_get_selected (selection, &model, &iter);
    if (!selected) return;

    gtk_tree_model_get (model, &iter, CLIENT_NAME, &name, -1);
    csource = get_client_by_name (name);
    g_free (name);
    g_return_if_fail (csource != NULL);
    CHECK_CSOURCE;

    /* Client tujuan harus aktif dan tidak sedang login */
    name = gtk_combo_box_get_active_text ((GtkComboBox *) window_move->combob_client);
    cdest = get_client_by_name (name);
    g_free (name);
    g_return_if_fail (cdest != NULL);
    CHECK_CDEST;

    response = gbilling_create_dialog (GTK_MESSAGE_QUESTION, 
                                       (GtkWindow *) window_move->window,
                                       "Pindah Client", "Pindahkan pemakaian "
                                       "client <b>%s</b> ke client <b>%s</b>?",
                                       csource->name, cdest->name);
    if (response != GTK_RESPONSE_OK)
        return;

    /* Periksa kembali status client dan client tujuan */
    CHECK_CSOURCE;
    CHECK_CDEST;

    /* Copy informasi client pertama ke client tujuan */
    cdest->type = csource->type;
    cdest->idtype = csource->idtype;
    cdest->tstart = csource->tstart;
    cdest->ucost = csource->ucost;
    cdest->acost = csource->acost;
    cdest->cost = csource->cost;
    g_free (cdest->username);
    g_free (cdest->desc);
    cdest->username = g_strdup (csource->username);
    cdest->desc = g_strdup (csource->desc);
    g_free (csource->desc);
    csource->desc = g_strdup_printf ("%s (%s)", cdest->desc, cdest->name);

    /* Hapus data recovery client yang dipindahkan, tulis ke client tujuan */
    gbilling_recov_write (cdest);
    if (csource->active)
        set_servcmd (csource, GBILLING_COMMAND_LOGOUT, NULL);
    else
        do_client_logout (csource);

    /* Perintah client tujuan untuk login */
    set_servcmd (cdest, GBILLING_COMMAND_RELOGIN, NULL);
    cdest->status = GBILLING_STATUS_LOGIN;
    start_timer (cdest);

    gtk_widget_destroy (window_move->window);
    g_free (window_move);

#undef CHECK_CSOURCE
#undef CHECK_CDEST

}

void
on_window_edit_delete_event (GtkWidget *widget,
                             GdkEvent  *event,
                             gpointer   data)
{
    gtk_widget_destroy (window_edit->window);
    g_free (window_edit);
}

void
on_spinb_editacost_value_changed (GtkSpinButton *spinb,
                                  gpointer data)
{
    ClientData *cdata;
    gchar *tmp;
    gint acost;

    cdata = get_client_in_row ();
    g_return_if_fail (cdata != NULL);

    acost = gtk_spin_button_get_value_as_int (spinb);
    tmp = g_strdup_printf ("Rp. %li", cdata->ucost + acost);
    gtk_label_set_text ((GtkLabel *) window_edit->label_total, tmp);
    g_free (tmp);
}

void
on_button_editok_clicked (GtkWidget *widget,
                          gpointer   data)
{
    ClientData *cdata;
    const gchar *tmp;

    cdata = get_client_in_row ();
    g_return_if_fail (cdata);

    cdata->acost = gtk_spin_button_get_value_as_int ((GtkSpinButton *) 
                                                     window_edit->spinb_add);
    tmp = gtk_entry_get_text ((GtkEntry *) window_edit->entry_info);
    if (cdata->desc)
        g_free (cdata->desc);
    cdata->desc = g_strstrip (g_strdup (tmp));
    /* update data recovery client */
    gbilling_recov_del (cdata);
    gbilling_recov_write (cdata);
    gtk_widget_destroy (window_edit->window);
    g_free (window_edit);
}

gboolean
on_window_calendar_delete_event (GtkWidget *widget,
                                 GdkEvent  *event,
                                 gpointer   data)
{
    if (gtk_toggle_button_get_active ((GtkToggleButton *) 
                                      window_search->toggleb_date_start))
        gtk_toggle_button_set_active ((GtkToggleButton *) 
                                      window_search->toggleb_date_start, FALSE);
    if (gtk_toggle_button_get_active ((GtkToggleButton *) 
                                      window_search->toggleb_date_end))
        gtk_toggle_button_set_active ((GtkToggleButton *) 
                                      window_search->toggleb_date_end, FALSE);
    gtk_widget_destroy (window_calendar->window);

    return TRUE;
}

void
on_calendar_day_selected_double_click (GtkCalendar *calendar,
                                       gpointer     data)
{
    guint d, m, y;
    gchar *s;

    gtk_calendar_get_date (calendar, &y, &m, &d);
    s = g_strdup_printf ("%.2u-%.2u-%u", d, m + 1, y);
    if (gtk_toggle_button_get_active ((GtkToggleButton *)
                                      window_search->toggleb_date_start))
    {
        gtk_toggle_button_set_active ((GtkToggleButton *) 
                                      window_search->toggleb_date_start, FALSE);
        gtk_button_set_label ((GtkButton *) window_search->toggleb_date_start, s);
    }
    if (gtk_toggle_button_get_active ((GtkToggleButton *) 
                                      window_search->toggleb_date_end))
    {
        gtk_toggle_button_set_active ((GtkToggleButton *) 
                                      window_search->toggleb_date_end, FALSE);
        gtk_button_set_label ((GtkButton *) window_search->toggleb_date_end, s);
    }
    g_free (window_calendar);
    g_free (s);
}

gboolean
on_window_time_delete_event (GtkWidget *widget,
                             GdkEvent  *event,
                             gpointer   data)
{
    if (gtk_toggle_button_get_active ((GtkToggleButton *) 
                                      window_search->toggleb_time_start))
        gtk_toggle_button_set_active ((GtkToggleButton *) 
                                      window_search->toggleb_time_start, FALSE);
    if (gtk_toggle_button_get_active ((GtkToggleButton *) 
                                      window_search->toggleb_time_end))
        gtk_toggle_button_set_active ((GtkToggleButton *) 
                                      window_search->toggleb_time_end, FALSE);
    gtk_widget_destroy (window_time->window);
    return TRUE;
}

void
on_button_wtimeok_clicked (GtkButton *button,
                           gpointer   data)
{
    gint h, m, s;
    gchar *tmp;

    h = gtk_spin_button_get_value_as_int ((GtkSpinButton *) window_time->spinb_hour);
    m = gtk_spin_button_get_value_as_int ((GtkSpinButton *) window_time->spinb_min);
    s = gtk_spin_button_get_value_as_int ((GtkSpinButton *) window_time->spinb_sec);
    tmp = g_strdup_printf ("%.2i:%.2i:%.2i", h, m, s);

    if (gtk_toggle_button_get_active ((GtkToggleButton *) 
                                      window_search->toggleb_time_start))
    {
        gtk_toggle_button_set_active ((GtkToggleButton *) 
                                      window_search->toggleb_time_start, FALSE);
        gtk_button_set_label ((GtkButton *) window_search->toggleb_time_start, tmp);
    }
    if (gtk_toggle_button_get_active ((GtkToggleButton *) window_search->toggleb_time_end))
    {
        gtk_toggle_button_set_active ((GtkToggleButton *) 
                                      window_search->toggleb_time_end, FALSE);
        gtk_button_set_label ((GtkButton *) window_search->toggleb_time_end, tmp);
    }

    g_free (tmp);
    gtk_widget_destroy (window_time->window);
}


