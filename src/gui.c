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
 *  gui.c
 *  File ini adalah bagian dari gBilling (http://gbilling.sourceforge.net)
 *  Copyright (C) 2008 - 2010, Ardhan Madras <ajhwb@knac.com>
 *
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>

#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "gbilling.h"
#include "gui.h"
#include "callback.h"
#include "server.h"
#include "sqldb.h"

static GtkBuilder *builder;

/* window_init */
GtkWidget *window_init = NULL;
GtkWidget *label_window_init_stat = NULL;

static void set_window_icon (GtkWindow*);

/* Pink Floyd - The Great Gig In The Sky, Wish You Were Here */

/**
 * windows_init:
 * 
 * Inisialisasi interface-interface window.
 **/
void
windows_init (void)
{
    window_search = NULL;
}

/**
 * Macro ini hanya untuk menset nama sementara widget, mengurangi
 * penulisan name variabel widget yang panjang seperti:
 * window_setting_server->entry_username
 **/
#define TEMP_NAME(name, widget) (name = widget)

static void
builder_init (const gchar *ui)
{
    g_return_if_fail (ui != NULL);
    gchar *file;
    guint retval;
    GError *error = NULL;

#ifdef PACKAGE_DATA_DIR
    file = g_build_filename (PACKAGE_DATA_DIR, "ui", ui, NULL);
#else
    file = g_build_filename ("share", "ui", ui, NULL);
#endif
    builder = gtk_builder_new ();
    retval = gtk_builder_add_from_file (builder, file, &error);
    g_free (file);
    if (!retval)
    {
        gbilling_debug ("%s: %s\n", __func__, error->message);
        g_error_free (error);
        exit (EXIT_FAILURE);
    }
}

static void
builder_destroy (void)
{
    g_object_unref (builder);
}

static GtkWidget*
get_widget (const gchar *name)
{
    g_return_val_if_fail (name != NULL, NULL);
    GObject *object;

    object = gtk_builder_get_object (builder, name);
    if (!object)
    {
        gbilling_debug ("%s: no widget for %s\n", __func__, name);
        exit (EXIT_FAILURE);
    }
    return GTK_WIDGET (object);
}

gchar*
get_data_file (const gchar *data)
{
    g_return_val_if_fail (data != NULL, NULL);

    gchar *file;

#ifdef PACKAGE_DATA_DIR
    file = g_build_filename (PACKAGE_DATA_DIR, data, NULL);
#else
    file = g_build_filename ("share", "data", data, NULL);
#endif

    return file;
}

gchar*
get_pixmap_file (const gchar *file)
{
    g_return_val_if_fail (file != NULL, NULL);

    gchar *data;

#ifdef PACKAGE_PIXMAPS_DIR
    data = g_build_filename (PACKAGE_PIXMAPS_DIR, file, NULL);
#else
    data = g_build_filename ("share", "pixmaps", file, NULL);
#endif

    return data;
}

static void
set_window_icon (GtkWindow *window)
{
#ifndef _WIN32
    g_return_if_fail (window != NULL);
    static GdkPixbuf *pixbuf = NULL;
    GError *error = NULL;
    gchar *icon;
    
    if (!pixbuf)
    {
        icon = get_pixmap_file ("gbilling.png");
        pixbuf = gdk_pixbuf_new_from_file (icon, &error);
        g_free (icon);
        if (!pixbuf)
        {
            gbilling_debug ("%s: %s\n", __func__, error->message);
            g_error_free (error);
            return;
        }
    }
    gtk_window_set_icon (window, pixbuf);
#endif
}

void
insert_treev_client (void)
{
    GList *ptr;
    ClientData *client;
    GtkTreeModel *model;

    model = gtk_tree_view_get_model ((GtkTreeView *) window_main->treeview_client);
    gtk_list_store_clear ((GtkListStore *) model);
    for (ptr = client_list; ptr; ptr = ptr->next)
    {
        client = ptr->data;
        gtk_list_store_append ((GtkListStore *) model, &client->iter);
        gtk_list_store_set ((GtkListStore *) model, &client->iter,
                            CLIENT_ACCESS, "Tutup",
                            CLIENT_STATUS, "Nonaktif",
                            CLIENT_NAME, client->name,
                            -1);
    }
}

void
insert_iconv_status (void)
{
    GList *ptr;
    GdkPixbuf *pixbuf, *pix;
    GtkTreeModel *model;
    ClientData *cdata;
    gchar *file;

    if (!client_list)
        return;
    file = get_data_file ("client.png");
    pixbuf = gdk_pixbuf_new_from_file (file, NULL);
    g_free (file);
    g_return_if_fail (pixbuf != NULL);

    pix = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (pixbuf),
                          gdk_pixbuf_get_has_alpha (pixbuf), 
                          gdk_pixbuf_get_bits_per_sample (pixbuf),
                          gdk_pixbuf_get_width (pixbuf),
                          gdk_pixbuf_get_height (pixbuf));
    gdk_pixbuf_saturate_and_pixelate (pixbuf, pix, 0.0, FALSE);
    model = gtk_icon_view_get_model ((GtkIconView *) window_main->iconview_status);
    gtk_list_store_clear ((GtkListStore *) model);
    for (ptr = client_list; ptr; ptr = ptr->next)
    {
        cdata = ptr->data;
        gtk_list_store_append ((GtkListStore *) model, &cdata->iterv);
        gtk_list_store_set ((GtkListStore *) model, &cdata->iterv,
                            ICONV_NAME, cdata->name,
                            ICONV_PIXBUF,
                            (cdata->status == GBILLING_STATUS_LOGIN) ? pixbuf : pix,
                            -1);
    }
    g_object_unref (pixbuf);
    g_object_unref (pix);
}

void
set_server_info (GtkWidget *window)
{
    g_return_if_fail (window != NULL);

    GdkPixbuf *pixbuf;
    gboolean test;
    gchar *file;

    gtk_window_set_title ((GtkWindow *) window_main->window, server_info->name);
    if (server->display_name)
    {
        set_markup ((GtkLabel *) window_main->label_name, GBILLING_FONT_X_LARGE, 
                    TRUE, server_info->name);
        gtk_widget_show (window_main->label_name);
    }
    else
        gtk_widget_hide (window_main->label_name);
    if (server->display_desc)
    {
        set_markup ((GtkLabel *) window_main->label_desc, GBILLING_FONT_LARGE, 
                    TRUE, server_info->desc);
        gtk_widget_show (window_main->label_desc);
    }
    else
        gtk_widget_hide (window_main->label_desc);

    if (server->display_addr)
    {
        gtk_label_set_text ((GtkLabel *) window_main->label_address, server_info->addr);
        gtk_widget_show (window_main->label_address);
    }
    else
        gtk_widget_hide (window_main->label_address);

    if (server->display_logo)
    {
        test = g_file_test (server->logo, G_FILE_TEST_IS_REGULAR);
        if (!test)
        {
            /* gunakan icon gbilling jika file tidak ada */
            file = get_pixmap_file ("gbilling.png");
            pixbuf = gdk_pixbuf_new_from_file (file, NULL);
            g_free (file);
        }
        else
            pixbuf = gdk_pixbuf_new_from_file (server->logo, NULL);
        gtk_image_set_from_pixbuf ((GtkImage *) window_main->image_logo, pixbuf);
        gtk_widget_show (window_main->image_logo);
        g_object_unref (pixbuf);
    }
    else
        gtk_widget_hide (window_main->image_logo);
}

void
create_window_main (void)
{
    GtkWidget *widget;
    GtkListStore *store;
    GtkTreeSelection *selection;
    gchar *tmp;

    builder_init ("main.ui");
    window_main = g_new (WindowMain, 1);

    window_main->window = get_widget ("window");
    gtk_window_set_position ((GtkWindow *) window_main->window, GTK_WIN_POS_CENTER);

#ifndef _WIN32
    set_window_icon ((GtkWindow *) window_main->window);
#endif
    g_signal_connect ((GObject *) window_main->window, "delete-event", 
		              (GCallback) on_window_main_delete_event, NULL);

    /* 
     * Di Win32, window_main selalu di kembalikan ke state semula
     * setelah minimize, track window iconify disini!
     */
#ifdef _WIN32
    g_signal_connect ((GObject *) window_main->window, "window-state-event", 
            (GCallback) on_window_main_state_event, NULL);
#endif
    
    window_main->menu_server_activate = get_widget ("menu_server_activate");
    g_signal_connect ((GObject *) window_main->menu_server_activate, "activate",
      			      (GCallback) on_toolb_connect_clicked, NULL);
    
    window_main->menu_server_deactivate = get_widget ("menu_server_deactivate");
    g_signal_connect ((GObject *) window_main->menu_server_deactivate, "activate",
                      (GCallback) on_toolb_dconnect_clicked, NULL);
    
    window_main->menu_server_quit = get_widget ("menu_server_quit");
    g_signal_connect ((GObject *) window_main->menu_server_quit, "activate",
            (GCallback) on_window_main_delete_event, NULL);
    
    window_main->menu_setting_server = get_widget ("menu_setting_server");
    g_signal_connect ((GObject *) window_main->menu_setting_server, "activate",
            (GCallback) on_menu_setting_server_activated, NULL);
    
    window_main->menu_setting_log = get_widget ("menu_setting_log");
    g_signal_connect ((GObject *) window_main->menu_setting_log, "activate",
            (GCallback) on_menu_setting_log_activated, NULL);
    
    window_main->menu_setting_mute = get_widget ("menu_setting_mute");
    gtk_check_menu_item_set_active ((GtkCheckMenuItem *) 
                                    window_main->menu_setting_mute, FALSE);
    g_signal_connect ((GObject *) window_main->menu_setting_mute, "toggled",
            (GCallback) on_menu_setting_mute_toggled, NULL);
    
    window_main->menu_setting_client = get_widget ("menu_setting_client");
    g_signal_connect ((GObject *) window_main->menu_setting_client, "activate",
            (GCallback) on_menu_setting_client_activated, NULL);
    
    window_main->menu_setting_cost = get_widget ("menu_setting_cost");
    g_signal_connect ((GObject *) window_main->menu_setting_cost, "activate",
            (GCallback) on_menu_setting_cost_activate, NULL);
    
    window_main->menu_setting_prepaid = get_widget ("menu_setting_prepaid");
    g_signal_connect ((GObject *) window_main->menu_setting_prepaid, "activate",
            (GCallback) on_menu_setting_prepaid_activate, NULL);
    
    window_main->menu_setting_item = get_widget ("menu_setting_item");
    g_signal_connect ((GObject *) window_main->menu_setting_item, "activate",
            (GCallback) on_toolb_sitem_clicked, NULL);
    
    window_main->menu_help_about = get_widget ("menu_help_about");
    g_signal_connect ((GObject *) window_main->menu_help_about, "activate",
            (GCallback) on_menu_help_about_activate, NULL);

    /* Toolbar */
    window_main->toggleb_client = get_widget ("toggleb_client");
    gtk_toggle_button_set_active ((GtkToggleButton *) 
                                  window_main->toggleb_client, TRUE);
    window_main->client_handlerid = 
        g_signal_connect ((GObject *) window_main->toggleb_client, 
                "toggled", (GCallback) on_toggleb_client_toggled, NULL);
    
    widget = get_widget ("image_toolb_client");
    tmp = get_data_file ("client-tab.png");
    gtk_image_set_from_file ((GtkImage *) widget, tmp);
    g_free (tmp);

    window_main->toggleb_status = get_widget ("toggleb_status");
    window_main->status_handlerid =
        g_signal_connect ((GObject *) window_main->toggleb_status, 
            "toggled", (GCallback) on_toggleb_status_toggled, NULL);

    widget = get_widget ("image_toolb_status");
    tmp = get_data_file ("status-tab.png");
    gtk_image_set_from_file ((GtkImage *) widget, tmp);
    g_free (tmp);

    window_main->toggleb_log = get_widget ("toggleb_log");
    window_main->log_handlerid =
        g_signal_connect ((GObject *) window_main->toggleb_log, 
            "toggled", (GCallback) on_toggleb_log_toggled, NULL);

    widget = get_widget ("image_toolb_log");
    tmp = get_data_file ("log-tab.png");
    gtk_image_set_from_file ((GtkImage *) widget, tmp);
    g_free (tmp);

    /* Informasi server */
    window_main->image_logo = get_widget ("image_logo");
    window_main->label_name = get_widget ("label_name");
    window_main->label_desc = get_widget ("label_desc");
    window_main->label_address = get_widget ("label_address");

    /* notebook */
    window_main->notebook_client = get_widget ("notebook_client");
    g_signal_connect ((GObject *) window_main->notebook_client, "switch-page", 
            (GCallback) on_noteb_client_switch_page, NULL);

    /* Notebook tab 1 */
    GtkTreeViewColumn *column_cname;
    GtkTreeViewColumn *column_cping;
    GtkTreeViewColumn *column_caccess;
    GtkTreeViewColumn *column_cstatus;
    GtkTreeViewColumn *column_cusername;
    GtkTreeViewColumn *column_ctype;
    GtkTreeViewColumn *column_cstart;
    GtkTreeViewColumn *column_cduration;
    GtkTreeViewColumn *column_cend;
    GtkTreeViewColumn *column_ccost;
    GtkCellRenderer *render_cname;
    GtkCellRenderer *render_cping;
    GtkCellRenderer *render_caccess;
    GtkCellRenderer *render_cstatus;
    GtkCellRenderer *render_cusername;
    GtkCellRenderer *render_ctype;
    GtkCellRenderer *render_cstart;
    GtkCellRenderer *render_cduration;
    GtkCellRenderer *render_cend;
    GtkCellRenderer *render_ccost;

    render_cname = gtk_cell_renderer_text_new ();
    column_cname = gtk_tree_view_column_new_with_attributes ("Client", render_cname,
                                          "text", CLIENT_NAME, NULL);
    gtk_tree_view_column_set_sort_column_id (column_cname, CLIENT_NAME);
    gtk_tree_view_column_set_resizable (column_cname, TRUE);
    gtk_tree_view_column_set_expand (column_cname, TRUE);

    render_cping = gtk_cell_renderer_text_new ();
    g_object_set ((GObject *) render_cping, "xalign", 0.5, NULL);    
    column_cping = gtk_tree_view_column_new_with_attributes ("Ping", render_cping,
                                              "text", CLIENT_PING, NULL);
    gtk_tree_view_column_set_sort_column_id (column_cping, CLIENT_PING);
    gtk_tree_view_column_set_resizable (column_cping, TRUE);
    gtk_tree_view_column_set_alignment (column_cping, 0.5);

    render_caccess = gtk_cell_renderer_text_new ();
    column_caccess = gtk_tree_view_column_new_with_attributes ("Akses",
                                render_caccess, "text", CLIENT_ACCESS, NULL);
    gtk_tree_view_column_set_sort_column_id (column_caccess, CLIENT_ACCESS);
    gtk_tree_view_column_set_resizable (column_caccess, TRUE);
    gtk_tree_view_column_set_expand (column_caccess, TRUE);

    render_cstatus = gtk_cell_renderer_text_new ();
    column_cstatus = gtk_tree_view_column_new_with_attributes ("Status",
                                      render_cstatus, "text", CLIENT_STATUS, NULL);
    gtk_tree_view_column_set_sort_column_id (column_cstatus, CLIENT_STATUS);
    gtk_tree_view_column_set_resizable (column_cstatus, TRUE);
    gtk_tree_view_column_set_expand (column_cstatus, TRUE);

    render_cusername = gtk_cell_renderer_text_new ();
    column_cusername = gtk_tree_view_column_new_with_attributes ("Username",
                              render_cusername, "text", CLIENT_USERNAME, NULL);
    gtk_tree_view_column_set_sort_column_id (column_cusername, CLIENT_USERNAME);
    gtk_tree_view_column_set_resizable (column_cusername, TRUE);
    gtk_tree_view_column_set_expand (column_cusername, TRUE);

    render_ctype = gtk_cell_renderer_text_new ();
    column_ctype = gtk_tree_view_column_new_with_attributes ("Tipe", render_ctype,
                                          "text", CLIENT_TYPE, NULL);
    gtk_tree_view_column_set_sort_column_id (column_ctype, CLIENT_TYPE);
    gtk_tree_view_column_set_resizable (column_ctype, TRUE);
    gtk_tree_view_column_set_expand (column_ctype, TRUE);

    render_cstart = gtk_cell_renderer_text_new ();
    column_cstart = gtk_tree_view_column_new_with_attributes ("Mulai", render_cstart,
                                                   "text", CLIENT_START, NULL);
    gtk_tree_view_column_set_sort_column_id (column_cstart, CLIENT_START);
    gtk_tree_view_column_set_resizable (column_cstart, TRUE);
    gtk_tree_view_column_set_expand (column_cstart, TRUE);

    render_cduration = gtk_cell_renderer_text_new ();
    column_cduration = gtk_tree_view_column_new_with_attributes ("Durasi",
                              render_cduration, "text", CLIENT_DURATION, NULL);
    gtk_tree_view_column_set_sort_column_id (column_cduration, CLIENT_DURATION);
    gtk_tree_view_column_set_resizable (column_cduration, TRUE);
    gtk_tree_view_column_set_expand (column_cduration, TRUE);

    render_cend = gtk_cell_renderer_text_new ();
    column_cend = gtk_tree_view_column_new_with_attributes ("Selesai", render_cend,
                                                    "text", CLIENT_END, NULL);
    gtk_tree_view_column_set_sort_column_id (column_cend, CLIENT_END);
    gtk_tree_view_column_set_resizable (column_cend, TRUE);
    gtk_tree_view_column_set_expand (column_cend, TRUE);

    render_ccost = gtk_cell_renderer_text_new ();
    column_ccost = gtk_tree_view_column_new_with_attributes ("Tarif", render_ccost,
                                                "text", CLIENT_COST, NULL);
    gtk_tree_view_column_set_sort_column_id (column_ccost, CLIENT_COST);
    gtk_tree_view_column_set_resizable (column_ccost, TRUE);
    gtk_tree_view_column_set_expand (column_ccost, TRUE);

    store = gtk_list_store_new (CLIENT_COLUMN, G_TYPE_STRING, 
                                G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, 
                                G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, 
                                G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    
    gtk_tree_sortable_set_sort_column_id ((GtkTreeSortable *) store,
                                CLIENT_NAME, GTK_SORT_ASCENDING);

    window_main->treeview_client = get_widget ("treeview_client");
    TEMP_NAME (widget, window_main->treeview_client);
    gtk_tree_view_set_headers_visible ((GtkTreeView *) widget, TRUE);
    gtk_tree_view_set_rules_hint ((GtkTreeView *) widget, TRUE);

    gtk_tree_view_set_model ((GtkTreeView *) widget, (GtkTreeModel *) store);
    gtk_tree_view_append_column ((GtkTreeView *) widget, column_cname);
    gtk_tree_view_append_column ((GtkTreeView *) widget, column_cping);
    gtk_tree_view_append_column ((GtkTreeView *) widget, column_caccess);
    gtk_tree_view_append_column ((GtkTreeView *) widget, column_cstatus);
    gtk_tree_view_append_column ((GtkTreeView *) widget, column_cusername);
    gtk_tree_view_append_column ((GtkTreeView *) widget, column_ctype);
    gtk_tree_view_append_column ((GtkTreeView *) widget, column_cstart);
    gtk_tree_view_append_column ((GtkTreeView *) widget, column_cduration);
    gtk_tree_view_append_column ((GtkTreeView *) widget, column_cend);
    gtk_tree_view_append_column ((GtkTreeView *) widget, column_ccost);
    
    selection = gtk_tree_view_get_selection ((GtkTreeView *) widget);
    g_signal_connect ((GObject *) selection, "changed", 
                      (GCallback) on_selection_client_changed, NULL);
    
    /* Notebook tab 2 */
    store = gtk_list_store_new (ICONV_COLUMN, G_TYPE_STRING, GDK_TYPE_PIXBUF);
    
    window_main->iconview_status = get_widget ("iconview_status");
    TEMP_NAME (widget, window_main->iconview_status);
    gtk_icon_view_set_text_column ((GtkIconView *) widget, ICONV_NAME);
    gtk_icon_view_set_pixbuf_column ((GtkIconView *) widget, ICONV_PIXBUF);
    gtk_icon_view_set_model ((GtkIconView *) widget, (GtkTreeModel *) store);
    /* g_signal_connect ((GObject *) widget, "selection-changed", 
                 (GCallback) on_iconv_status_selection_changed, NULL); */

    /* Notebook tab 3 */
    GtkTreeViewColumn *column_ldate;
    GtkTreeViewColumn *column_lclient;
    GtkTreeViewColumn *column_lusername;
    GtkTreeViewColumn *column_ltype;
    GtkTreeViewColumn *column_lstart;
    GtkTreeViewColumn *column_lduration;
    GtkTreeViewColumn *column_lend;
    GtkTreeViewColumn *column_lcost;
    GtkCellRenderer *render_ldate;
    GtkCellRenderer *render_lclient;
    GtkCellRenderer *render_lusername;
    GtkCellRenderer *render_ltype;
    GtkCellRenderer *render_lstart;
    GtkCellRenderer *render_lduration;
    GtkCellRenderer *render_lend;
    GtkCellRenderer *render_lcost;

    render_ldate = gtk_cell_renderer_text_new ();
    column_ldate = gtk_tree_view_column_new_with_attributes ("Tanggal", render_ldate,
                                                  "text", SLOG_DATE, NULL);
    gtk_tree_view_column_set_sort_column_id (column_ldate, SLOG_DATE);
    gtk_tree_view_column_set_resizable (column_ldate, TRUE);
    gtk_tree_view_column_set_expand (column_ldate, TRUE);

    render_lclient = gtk_cell_renderer_text_new ();
    column_lclient = gtk_tree_view_column_new_with_attributes ("Client", render_lclient,
                                                   "text", SLOG_CLIENT, NULL);
    gtk_tree_view_column_set_sort_column_id (column_lclient, SLOG_CLIENT);
    gtk_tree_view_column_set_resizable (column_lclient, TRUE);
    gtk_tree_view_column_set_expand (column_lclient, TRUE);

    render_lusername = gtk_cell_renderer_text_new ();
    column_lusername = gtk_tree_view_column_new_with_attributes ("Username",
                              render_lusername, "text", SLOG_USERNAME, NULL);
    gtk_tree_view_column_set_sort_column_id (column_lusername, SLOG_USERNAME);
    gtk_tree_view_column_set_resizable (column_lusername, TRUE);
    gtk_tree_view_column_set_expand (column_lusername, TRUE);
    
    render_ltype = gtk_cell_renderer_text_new ();
    column_ltype = gtk_tree_view_column_new_with_attributes ("Tipe", render_ltype,
                                                  "text", SLOG_TYPE, NULL);
    gtk_tree_view_column_set_sort_column_id (column_ltype, SLOG_TYPE);
    gtk_tree_view_column_set_resizable (column_ltype, TRUE);
    gtk_tree_view_column_set_expand (column_ltype, TRUE);

    render_lstart = gtk_cell_renderer_text_new ();
    column_lstart = gtk_tree_view_column_new_with_attributes ("Mulai", render_lstart,
                                                "text", SLOG_START, NULL);
    gtk_tree_view_column_set_sort_column_id (column_lstart, SLOG_START);
    gtk_tree_view_column_set_resizable (column_lstart, TRUE);
    gtk_tree_view_column_set_expand (column_lstart, TRUE);

    render_lduration = gtk_cell_renderer_text_new ();
    column_lduration = gtk_tree_view_column_new_with_attributes ("Durasi",
                                render_lduration, "text", SLOG_DURATION, NULL);
    gtk_tree_view_column_set_sort_column_id (column_lduration, SLOG_DURATION);
    gtk_tree_view_column_set_resizable (column_lduration, TRUE);
    gtk_tree_view_column_set_expand (column_lduration, TRUE);

    render_lend = gtk_cell_renderer_text_new ();
    column_lend = gtk_tree_view_column_new_with_attributes ("Selesai", render_lend,
                                                    "text", SLOG_END, NULL);
    gtk_tree_view_column_set_sort_column_id (column_lend, SLOG_END);
    gtk_tree_view_column_set_resizable (column_lend, TRUE);
    gtk_tree_view_column_set_expand (column_lend, TRUE);

    render_lcost = gtk_cell_renderer_text_new ();
    column_lcost = gtk_tree_view_column_new_with_attributes ("Tarif", render_lcost,
                                                      "text", SLOG_COST, NULL);
    gtk_tree_view_column_set_sort_column_id (column_lcost, SLOG_COST);
    gtk_tree_view_column_set_resizable (column_lcost, TRUE);
    gtk_tree_view_column_set_expand (column_lcost, TRUE);

    store = gtk_list_store_new (SLOG_COLUMN, G_TYPE_STRING, G_TYPE_STRING,
                                G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                                G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    gtk_tree_sortable_set_sort_column_id ((GtkTreeSortable *) store,
                                          SLOG_DATE, GTK_SORT_ASCENDING);

    /* Fungsi sort khusus untuk kolum 'date' 
     * TODO: Agar kolum `date' bisa di sort tanpa fungsi sortir khusus,
     * format tanggal log perlu dibuat menjadi YYYY-MM-DD
     */
    gtk_tree_sortable_set_sort_func ((GtkTreeSortable *) store, SLOG_DATE,
                      column_sort_func, NULL, NULL);

    window_main->treeview_log = get_widget ("treeview_log");
    TEMP_NAME (widget, window_main->treeview_log);
    gtk_tree_view_set_headers_visible ((GtkTreeView *) widget, TRUE);
    gtk_tree_view_set_rules_hint ((GtkTreeView *) widget, TRUE);

    gtk_tree_view_set_model ((GtkTreeView *) widget, (GtkTreeModel *) store);
    gtk_tree_view_append_column ((GtkTreeView *) widget, column_ldate);
    gtk_tree_view_append_column ((GtkTreeView *) widget, column_lclient);
    gtk_tree_view_append_column ((GtkTreeView *) widget, column_lusername);
    gtk_tree_view_append_column ((GtkTreeView *) widget, column_ltype);
    gtk_tree_view_append_column ((GtkTreeView *) widget, column_lstart);
    gtk_tree_view_append_column ((GtkTreeView *) widget, column_lduration);
    gtk_tree_view_append_column ((GtkTreeView *) widget, column_lend);
    gtk_tree_view_append_column ((GtkTreeView *) widget, column_lcost);

    window_main->log_selection = gtk_tree_view_get_selection ((GtkTreeView *) widget);
    gtk_tree_selection_set_mode (window_main->log_selection, GTK_SELECTION_MULTIPLE);
    g_signal_connect ((GObject *) window_main->log_selection, "changed", 
                      (GCallback) on_selection_log_changed, NULL);

    /* Notebook control tab 1 */
    window_main->notebook_menu = get_widget ("notebook_menu");
    window_main->label_client = get_widget ("label_client");
    
    window_main->button_chat = get_widget ("button_chat");
    g_signal_connect ((GObject *) window_main->button_chat, "clicked", 
            (GCallback) on_button_cchat_clicked, NULL);
    
    window_main->button_logout = get_widget ("button_logout");
    g_signal_connect ((GObject *) window_main->button_logout, "clicked", 
            (GCallback) on_button_clogout_clicked, NULL);
        
    window_main->button_restart = get_widget ("button_restart");
    g_signal_connect ((GObject *) window_main->button_restart, "clicked", 
            (GCallback) on_button_crestart_clicked, NULL);
    
    window_main->button_shutdown = get_widget ("button_shutdown");
    g_signal_connect ((GObject *) window_main->button_shutdown, "clicked", 
            (GCallback) on_button_cshutdown_clicked, NULL);
    
    window_main->button_move = get_widget ("button_move");
    g_signal_connect ((GObject *) window_main->button_move, "clicked", 
            (GCallback) on_button_cmove_clicked, NULL);
    
    window_main->button_edit = get_widget ("button_edit");
    g_signal_connect ((GObject *) window_main->button_edit, "clicked", 
            (GCallback) on_button_cedit_clicked, NULL);
    
    window_main->button_enable = get_widget ("button_enable");
    g_signal_connect ((GObject *) window_main->button_enable, "clicked", 
            (GCallback) on_button_cenable_clicked, NULL);
    
    window_main->button_disable = get_widget ("button_disable");    
    g_signal_connect ((GObject *) window_main->button_disable, "clicked", 
            (GCallback) on_button_cdisable_clicked, NULL);

    /* Notebook control tab 2 */
    window_main->button_filter = get_widget ("button_filter");
    g_signal_connect ((GObject *) window_main->button_filter, "clicked", 
            (GCallback) on_button_lfilter_clicked, NULL);
    
    window_main->button_detail = get_widget ("button_detail");
    g_signal_connect ((GObject *) window_main->button_detail, "clicked", 
            (GCallback) on_button_ldetail_clicked, NULL);
    
    window_main->button_delete = get_widget ("button_delete");
    g_signal_connect ((GObject *) window_main->button_delete, "clicked", 
            (GCallback) on_button_ldel_clicked, NULL);
    
    window_main->button_total = get_widget ("button_total");
    g_signal_connect ((GObject *) window_main->button_total, "clicked", 
            (GCallback) on_button_ltotal_clicked, NULL);

    window_main->button_export = get_widget ("button_export");
    g_signal_connect ((GObject *) window_main->button_export, "clicked", 
            (GCallback) on_button_lexport_clicked, NULL);

    window_main->button_refresh = get_widget ("button_refresh");
    g_signal_connect ((GObject *) window_main->button_refresh, "clicked",
            (GCallback) on_button_lrefresh_clicked, NULL);
    /* End notebook control */
    
    /* Statusbar */
    window_main->statusbar = get_widget ("statusbar");
    window_main->statusbar_cost = get_widget ("statusbar_cost");

    builder_destroy ();
}

void
set_statusbar (const gchar *text)
{
    g_return_if_fail (text != NULL);

    gtk_statusbar_pop ((GtkStatusbar *) window_main->statusbar, 0);
    gtk_statusbar_push ((GtkStatusbar *) window_main->statusbar, 0, text);
}

void
create_window_setting_server (void)
{
    GtkWidget *button, *widget;
    GtkTreeViewColumn *column_play;
    GtkTreeViewColumn *column_event;
    GtkCellRenderer *render_play;
    GtkCellRenderer *render_event;
    GtkListStore *store;
    GtkTreeSelection *selection;

    builder_init ("server_setting.ui");
    window_setting_server = g_new (WindowSettingServer, 1);
    
    window_setting_server->window = get_widget ("window");
    gtk_window_set_transient_for ((GtkWindow *) window_setting_server->window,
                                  (GtkWindow *) window_main->window);
    gtk_window_set_title ((GtkWindow *) window_setting_server->window,
                          "Pengaturan Server");
    set_window_icon ((GtkWindow *) window_setting_server->window);
    g_signal_connect ((GObject *) window_setting_server->window, "delete-event",
                      (GCallback) on_window_setting_server_delete_event, NULL);
    
    window_setting_server->spinb_port = get_widget ("spinb_port");
    gtk_spin_button_set_range ((GtkSpinButton *) window_setting_server->spinb_port,
                               1, G_MAXUSHORT);
    
    window_setting_server->spinb_recovery = get_widget ("spinb_recovery");
    gtk_spin_button_set_range ((GtkSpinButton *) 
                               window_setting_server->spinb_recovery, 1, 10);
    window_setting_server->checkb_autoactive = get_widget ("checkb_autoactive");
    
    window_setting_server->entry_susername = get_widget ("entry_susername");
    gtk_entry_set_max_length ((GtkEntry *) window_setting_server->entry_susername,
                              MAX_USERNAME);
    g_signal_connect ((GObject *) window_setting_server->entry_susername, "changed",
                      (GCallback) on_entry_suser_window_setting_server_changed, NULL);
    
    window_setting_server->label_spasswd = get_widget ("label_spasswd");
    
    window_setting_server->entry_spasswd = get_widget ("entry_spasswd");
    gtk_entry_set_visibility ((GtkEntry *) window_setting_server->entry_spasswd, FALSE);
    gtk_entry_set_max_length ((GtkEntry *) window_setting_server->entry_spasswd,
                              MAX_PASSWORD);
    g_signal_connect ((GObject *) window_setting_server->entry_spasswd, "changed",
                      (GCallback) on_entry_spasswd_window_setting_server_changed, NULL);
    
    window_setting_server->label_scpasswd = get_widget ("label_scpasswd");
    
    window_setting_server->entry_scpasswd = get_widget ("entry_scpasswd");
    gtk_entry_set_visibility ((GtkEntry *) window_setting_server->entry_scpasswd, FALSE);
    gtk_entry_set_max_length ((GtkEntry *) window_setting_server->entry_scpasswd,
                              MAX_PASSWORD);
    g_signal_connect_after ((GObject *) window_setting_server->entry_scpasswd, "changed",
                      (GCallback) on_entry_spasswdc_window_setting_server_changed, NULL);
    
    window_setting_server->button_ssave = get_widget ("button_ssave");
    g_signal_connect ((GObject *) window_setting_server->button_ssave, "clicked",
                      (GCallback) on_button_sset_window_setting_server_clicked, NULL);

    window_setting_server->entry_cusername = get_widget ("entry_cusername");
    gtk_entry_set_max_length ((GtkEntry *) window_setting_server->entry_cusername,
                              MAX_USERNAME);
    g_signal_connect ((GObject *) window_setting_server->entry_cusername, "changed",
                      (GCallback) on_entry_cuser_window_setting_server_changed, NULL);

    window_setting_server->label_cpasswd = get_widget ("label_cpasswd");
    
    window_setting_server->entry_cpasswd = get_widget ("entry_cpasswd");
    gtk_entry_set_visibility ((GtkEntry *) window_setting_server->entry_cpasswd, FALSE);
    gtk_entry_set_max_length ((GtkEntry *) window_setting_server->entry_cpasswd,
                              MAX_PASSWORD);
    g_signal_connect ((GObject *) window_setting_server->entry_cpasswd, "changed",
                      (GCallback) on_entry_cpasswd_window_setting_server_changed, NULL);
    
    window_setting_server->label_ccpasswd = get_widget ("label_ccpasswd");
    
    window_setting_server->entry_ccpasswd = get_widget ("entry_ccpasswd");
    gtk_entry_set_visibility ((GtkEntry *) window_setting_server->entry_ccpasswd, FALSE);
    gtk_entry_set_max_length ((GtkEntry *) window_setting_server->entry_ccpasswd,
                              MAX_PASSWORD);
    g_signal_connect ((GObject *) window_setting_server->entry_ccpasswd, "changed",
                      (GCallback) on_entry_cpasswdc_window_setting_server_changed, NULL);
    
    window_setting_server->button_csave = get_widget ("button_csave");
    g_signal_connect ((GObject *) window_setting_server->button_csave, "clicked",
                      (GCallback) on_button_cset_window_setting_server_clicked, NULL);

    window_setting_server->entry_name = get_widget ("entry_name");
    gtk_entry_set_max_length ((GtkEntry *) window_setting_server->entry_name, 
                               sizeof(server_info->name) - 1);

    window_setting_server->entry_desc = get_widget ("entry_desc");
    gtk_entry_set_max_length ((GtkEntry *) window_setting_server->entry_desc,
                              sizeof(server_info->desc) - 1);

    window_setting_server->entry_address = get_widget ("entry_address");
    gtk_entry_set_max_length ((GtkEntry *) window_setting_server->entry_address,
                              sizeof(server_info->addr) - 1);
    
    window_setting_server->checkb_name = get_widget ("checkb_name");
    window_setting_server->checkb_desc = get_widget ("checkb_desc");
    window_setting_server->checkb_address = get_widget ("checkb_address");
    
    window_setting_server->label_logo = get_widget ("label_logo");
    window_setting_server->entry_logo = get_widget ("entry_logo");

    window_setting_server->button_logo = get_widget ("button_logo");
    g_signal_connect ((GObject *) window_setting_server->button_logo, "clicked", 
                      (GCallback) on_button_logo_window_setting_server_clicked,
                      NULL);

    window_setting_server->checkb_logo = get_widget ("checkb_logo");
    g_signal_connect ((GObject *) window_setting_server->checkb_logo, "toggled",
                      (GCallback) on_checkb_logo_window_setting_server_toggled, 
                      NULL);
    gtk_toggle_button_set_active ((GtkToggleButton *) 
                                  window_setting_server->checkb_logo,
                                  server->display_logo);
    on_checkb_logo_window_setting_server_toggled ((GtkToggleButton *)
                                                  window_setting_server->checkb_logo, 
                                                  NULL);

    store = gtk_list_store_new (2, G_TYPE_BOOLEAN, G_TYPE_STRING);

    render_play = gtk_cell_renderer_toggle_new ();
    column_play = gtk_tree_view_column_new_with_attributes ("Play", render_play,
                                                            "active", 0, NULL);
    g_signal_connect ((GObject *) render_play, "toggled", 
                      (GCallback) on_render_play_window_setting_server_toggled, 
                      NULL);

    render_event = gtk_cell_renderer_text_new ();
    column_event = gtk_tree_view_column_new_with_attributes ("Kejadian", render_event,
                                                             "text", 1, NULL);

    window_setting_server->treeview_sound = get_widget ("treeview_sound");
    TEMP_NAME (widget, window_setting_server->treeview_sound);

    gtk_tree_view_append_column ((GtkTreeView *) widget, column_play);
    gtk_tree_view_append_column ((GtkTreeView *) widget, column_event);
    gtk_tree_view_set_model ((GtkTreeView *) widget, (GtkTreeModel *) store);

    selection = gtk_tree_view_get_selection ((GtkTreeView *) widget);
    g_signal_connect ((GObject *) selection, "changed",
                      (GCallback) on_treeview_sound_window_setting_server_changed,
                      NULL);

    window_setting_server->entry_sound = get_widget ("entry_sound");

    window_setting_server->button_browse = get_widget ("button_browse");
    g_signal_connect ((GObject *) window_setting_server->button_browse, "clicked",
                      (GCallback) on_button_browse_window_setting_server_clicked,
                      NULL);

    window_setting_server->button_preview = get_widget ("button_preview");
    g_signal_connect ((GObject *) window_setting_server->button_preview, "clicked",
                      (GCallback) on_button_preview_window_setting_server_clicked,
                      NULL);

    button = get_widget ("button_ok");
    g_signal_connect ((GObject *) button, "clicked",
                      (GCallback) on_button_ok_window_setting_server_clicked,
                      NULL);
    builder_destroy ();
}

/**
 * setting client
 */
void
create_window_setting_client (void)
{
    GtkCellRenderer *render_name;
    GtkCellRenderer *render_ip;
    GtkTreeViewColumn *column_name;
    GtkTreeViewColumn *column_ip;
    
    builder_init ("client_setting.ui");
    window_setting_client = g_new (WindowSettingClient, 1);
    
    window_setting_client->window = get_widget ("window");
    set_window_icon ((GtkWindow *) window_setting_client->window);
    gtk_window_set_transient_for ((GtkWindow *) window_setting_client->window,
                                  (GtkWindow *) window_main->window);
    gtk_window_set_title ((GtkWindow *) window_setting_client->window,
                          "Pengaturan Client");
    g_signal_connect ((GObject *) window_setting_client->window, "delete-event",
                      (GCallback) on_window_setting_client_delete_event, NULL);

    window_setting_client->store = gtk_list_store_new (SCLIENT_COLUMN, 
                                                       G_TYPE_STRING, G_TYPE_STRING);
    
    window_setting_client->treeview = get_widget ("treeview");
    gtk_tree_view_set_rules_hint ((GtkTreeView *) window_setting_client->treeview, TRUE);
    gtk_tree_view_set_model ((GtkTreeView *) window_setting_client->treeview, 
                             (GtkTreeModel *) window_setting_client->store);

    render_name = gtk_cell_renderer_text_new ();
    column_name = gtk_tree_view_column_new_with_attributes ("Nama", render_name,
                                            "text", SCLIENT_NAME, NULL);
    gtk_tree_view_column_set_sizing (column_name, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width (column_name, 150);
    gtk_tree_view_append_column 
        ((GtkTreeView *) window_setting_client->treeview, column_name);

    render_ip = gtk_cell_renderer_text_new ();
    column_ip = gtk_tree_view_column_new_with_attributes ("Alamat IP", render_ip,
                                            "text", SCLIENT_IP, NULL);
    gtk_tree_view_column_set_sizing (column_ip, GTK_TREE_VIEW_COLUMN_FIXED);
    gtk_tree_view_column_set_fixed_width (column_ip, 120);
    gtk_tree_view_append_column 
        ((GtkTreeView *) window_setting_client->treeview, column_ip);

    window_setting_client->button_add = get_widget ("button_add");
    g_signal_connect ((GObject *) window_setting_client->button_add, "clicked",
                      (GCallback) on_button_add_window_setting_client_clicked, NULL);

    window_setting_client->button_edit = get_widget ("button_edit");
    g_signal_connect ((GObject *) window_setting_client->button_edit, "clicked",
                      (GCallback) on_button_edit_window_setting_client_clicked, NULL);

    window_setting_client->button_del = get_widget ("button_del");
    g_signal_connect ((GObject *) window_setting_client->button_del, "clicked",
                      (GCallback) on_button_del_window_setting_client_clicked, NULL);

    window_setting_client->checkb_disable = get_widget ("checkb_disable");
    window_setting_client->checkb_restart = get_widget ("checkb_restart");

    window_setting_client->button_ok = get_widget ("button_ok");
    g_signal_connect ((GObject *) window_setting_client->button_ok, "clicked",
                      (GCallback) on_window_setting_client_delete_event, NULL);
    builder_destroy ();
}

void
create_window_update_client (GbillingUpdateMode mode)
{
    g_return_if_fail (mode >= GBILLING_UPDATE_MODE_ADD &&
                      mode <= GBILLING_UPDATE_MODE_EDIT);
    builder_init ("update_client.ui");
    
    window_update_client = g_new (WindowUpdateClient, 1);
    
    window_update_client->window = get_widget ("window");
    set_window_icon ((GtkWindow *) window_update_client->window);
    gtk_window_set_transient_for ((GtkWindow *) window_update_client->window,
                                  (GtkWindow *) window_setting_client->window);
    gtk_window_set_title ((GtkWindow *) window_update_client->window,
                          mode == GBILLING_UPDATE_MODE_ADD ? 
                          "Tambah Client" : "Edit Client");
    g_signal_connect ((GObject *) window_update_client->window, "delete-event",
                      (GCallback) on_window_update_client_delete_event, NULL);

    window_update_client->entry_name = get_widget ("entry_name");
    gtk_entry_set_max_length ((GtkEntry *) window_update_client->entry_name,
                              MAX_CLIENT_NAME);

    window_update_client->entry_ip = get_widget ("entry_ip");
    gtk_entry_set_max_length ((GtkEntry *) window_update_client->entry_ip, MAX_IP);
    g_signal_connect (window_update_client->entry_ip, "insert-text",
                      (GCallback) on_window_update_client_entry_ip_insert_text, NULL);

    window_update_client->button_close = get_widget ("button_close");
    g_signal_connect ((GObject *) window_update_client->button_close, "clicked",
                      (GCallback) on_window_update_client_delete_event, NULL);

    window_update_client->button_ok = get_widget ("button_ok");
    g_signal_connect ((GObject *) window_update_client->button_ok, "clicked",
                      (GCallback) on_button_ok_window_update_client_clicked, 
                      GINT_TO_POINTER (mode));
    builder_destroy ();
}

void
create_window_setting_cost (void)
{
    GtkCellRenderer *render_name;
    GtkCellRenderer *render_model;
    GtkCellRenderer *render_cost;
    GtkCellRenderer *render_default;
    GtkCellRenderer *render_cost_rotation;
    GtkCellRenderer *render_interval_rotation;
    GtkCellRenderer *render_state_rotation;
    GtkTreeViewColumn *column_name;
    GtkTreeViewColumn *column_model;
    GtkTreeViewColumn *column_cost;
    GtkTreeViewColumn *column_default;
    GtkTreeViewColumn *column_cost_rotation;
    GtkTreeViewColumn *column_interval_rotation;
    GtkTreeViewColumn *column_state_rotation;

    GtkListStore *store;

    builder_init ("cost_setting.ui");
    
    window_setting_cost = g_new (WindowSettingCost, 1);
    
    window_setting_cost->window = get_widget ("window");
    set_window_icon ((GtkWindow *) window_setting_cost->window);
    gtk_window_set_transient_for ((GtkWindow *) window_setting_cost->window,
                                  (GtkWindow *) window_main->window);
    gtk_window_set_title ((GtkWindow *) window_setting_cost->window,
                          "Pengaturan Tarif");
    g_signal_connect ((GObject *) window_setting_cost->window, "delete-event",
                      (GCallback) on_window_setting_cost_delete_event, NULL);
    
    render_name = gtk_cell_renderer_text_new ();
    column_name = gtk_tree_view_column_new_with_attributes ("Nama", render_name,
                        "text", SCCOST_NAME, NULL);
    gtk_tree_view_column_set_min_width (column_name, 100);
    gtk_tree_view_column_set_expand (column_name, TRUE);
        
    render_model = gtk_cell_renderer_text_new ();
    column_model = gtk_tree_view_column_new_with_attributes ("Model", render_model,
                        "text", SCCOST_MODE, NULL);
    gtk_tree_view_column_set_min_width (column_model, 100);
    gtk_tree_view_column_set_expand (column_model, TRUE);
        
    render_cost = gtk_cell_renderer_text_new ();
    column_cost = gtk_tree_view_column_new_with_attributes ("Tarif",
                        render_cost, "text", SCCOST_COST, NULL);
    gtk_tree_view_column_set_min_width (column_cost, 50);
    gtk_tree_view_column_set_expand (column_cost, TRUE);

    render_default = gtk_cell_renderer_toggle_new ();
    gtk_cell_renderer_toggle_set_radio ((GtkCellRendererToggle *) render_default, TRUE);
    
    column_default = gtk_tree_view_column_new_with_attributes ("Default",
                        render_default, "active", SCOST_DEFAULT, NULL);
    gtk_tree_view_column_set_expand (column_default, TRUE);
    gtk_tree_view_column_set_alignment (column_default, 0.5);

    window_setting_cost->store = gtk_list_store_new (SCOST_COLUMN, G_TYPE_STRING,
                                                     G_TYPE_STRING, G_TYPE_ULONG, 
                                                     G_TYPE_BOOLEAN);

    window_setting_cost->treeview = get_widget ("treeview");
    gtk_tree_view_set_rules_hint ((GtkTreeView *) window_setting_cost->treeview, TRUE);
    gtk_tree_view_set_model ((GtkTreeView *) window_setting_cost->treeview, 
                             (GtkTreeModel *) window_setting_cost->store);
    gtk_tree_view_append_column ((GtkTreeView *) window_setting_cost->treeview, column_name);
    gtk_tree_view_append_column ((GtkTreeView *) window_setting_cost->treeview, column_model);
    gtk_tree_view_append_column ((GtkTreeView *) window_setting_cost->treeview, column_cost);
    gtk_tree_view_append_column ((GtkTreeView *) window_setting_cost->treeview, column_default);

    window_setting_cost->button_add = get_widget ("button_add");
    g_signal_connect ((GObject *) window_setting_cost->button_add, "clicked",
                      (GCallback) on_button_scost_add_clicked, NULL);
    
    window_setting_cost->button_edit = get_widget ("button_edit");
    g_signal_connect ((GObject *) window_setting_cost->button_edit, "clicked",
                      (GCallback) on_button_scost_edit_clicked, NULL);
    
    window_setting_cost->button_del = get_widget ("button_del");
    g_signal_connect ((GObject *) window_setting_cost->button_del, "clicked",
                      (GCallback) on_button_scost_del_clicked, NULL);
    
    window_setting_cost->button_default = get_widget ("button_default");
    g_signal_connect ((GObject *) window_setting_cost->button_default, "clicked",
                      (GCallback) on_button_default_window_setting_cost_clicked, NULL);

    render_cost_rotation = gtk_cell_renderer_text_new ();
    column_cost_rotation = gtk_tree_view_column_new_with_attributes ("Tarif",
                                render_cost_rotation, "text", SCOSTROTATION_NAME, NULL);
    gtk_tree_view_column_set_expand (column_cost_rotation, TRUE);

    render_interval_rotation = gtk_cell_renderer_text_new ();
    column_interval_rotation = gtk_tree_view_column_new_with_attributes ("Interval",
                                    render_interval_rotation, "text", 
                                    SCOSTROTATION_INTERVAL, NULL);
    gtk_tree_view_column_set_min_width (column_interval_rotation, 120);

    render_state_rotation = gtk_cell_renderer_toggle_new ();
    column_state_rotation = gtk_tree_view_column_new_with_attributes ("Aktif",
                                    render_state_rotation, "active", 
                                    SCOSTROTATION_STATE, NULL);

    store = gtk_list_store_new (SCOSTROTATION_COLUMN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);

    window_setting_cost->treeview_rotation = get_widget ("treeview_rotation");
    gtk_tree_view_set_rules_hint ((GtkTreeView *) window_setting_cost->treeview_rotation, TRUE);
    gtk_tree_view_set_model ((GtkTreeView *) window_setting_cost->treeview_rotation, 
                             (GtkTreeModel *) store);
    gtk_tree_view_append_column ((GtkTreeView *) window_setting_cost->treeview_rotation,
                                 column_cost_rotation);
    gtk_tree_view_append_column ((GtkTreeView *) window_setting_cost->treeview_rotation,
                                 column_interval_rotation);
    gtk_tree_view_append_column ((GtkTreeView *) window_setting_cost->treeview_rotation,
                                 column_state_rotation);

    window_setting_cost->button_add_rotation = get_widget ("button_add_rotation");
    g_signal_connect ((GObject *) window_setting_cost->button_add_rotation, "clicked",
                      (GCallback) on_button_add_rotation_window_setting_cost_clicked, NULL);

    window_setting_cost->button_edit_rotation = get_widget ("button_edit_rotation");
    g_signal_connect ((GObject *) window_setting_cost->button_edit_rotation, "clicked",
                      (GCallback) on_button_edit_rotation_window_setting_cost_clicked, NULL);

    window_setting_cost->button_del_rotation = get_widget ("button_del_rotation");
    g_signal_connect ((GObject *) window_setting_cost->button_del_rotation, "clicked",
                      (GCallback) on_button_del_rotation_window_setting_cost_clicked, NULL);

    window_setting_cost->button_ok = get_widget ("button_ok");
    g_signal_connect ((GObject *) window_setting_cost->button_ok, "clicked",
                      (GCallback) on_window_setting_cost_delete_event, NULL);
    
    builder_destroy ();
}

void
create_window_update_cost (GbillingUpdateMode mode)
{
    GbillingCost cost;

    builder_init ("update_cost.ui");
    window_update_cost = g_new (WindowUpdateCost, 1);

    window_update_cost->window = get_widget ("window");
    set_window_icon ((GtkWindow *) window_update_cost->window);
    gtk_window_set_title ((GtkWindow *) window_update_cost->window,
                          mode == GBILLING_UPDATE_MODE_ADD ? 
                          "Tambah Tarif" : "Edit Tarif");
    gtk_window_set_transient_for ((GtkWindow *) window_update_cost->window,
                                  (GtkWindow *) window_setting_cost->window);
    g_signal_connect ((GObject *) window_update_cost->window, "delete-event",
                      (GCallback) on_window_update_cost_delete_event, NULL);

    window_update_cost->radiob_mode1 = get_widget ("radiob_mode1");
    g_signal_connect ((GObject *) window_update_cost->radiob_mode1, "toggled",
                      (GCallback) on_radiob_window_cost_mode1_toggled, NULL);
    
    window_update_cost->radiob_mode2 = get_widget ("radiob_mode2");
    g_signal_connect ((GObject *) window_update_cost->radiob_mode2, "toggled",
                      (GCallback) on_radiob_window_cost_mode2_toggled, NULL);
    
    window_update_cost->entry_name = get_widget ("entry_name");
    gtk_entry_set_max_length ((GtkEntry *) window_update_cost->entry_name, 
                              sizeof cost.name - 1);
    
    window_update_cost->spinb_fcost = get_widget ("spinb_fcost");
    gtk_spin_button_set_digits ((GtkSpinButton *) window_update_cost->spinb_fcost, 0);
    gtk_spin_button_set_increments ((GtkSpinButton *) window_update_cost->spinb_fcost,
                                    50.0, 100.0);
    gtk_spin_button_set_range ((GtkSpinButton *) window_update_cost->spinb_fcost,
                               50.0, 1000000.0);
    g_signal_connect ((GObject *) window_update_cost->spinb_fcost, "value-changed",
                      (GCallback) set_label_sumr_window_update_cost, NULL);
    
    window_update_cost->spinb_fmin = get_widget ("spinb_fmin");
    gtk_spin_button_set_digits ((GtkSpinButton *) window_update_cost->spinb_fmin, 0);
    gtk_spin_button_set_increments ((GtkSpinButton *) window_update_cost->spinb_fmin,
                                    1.0, 5.0);
    gtk_spin_button_set_range ((GtkSpinButton *) window_update_cost->spinb_fmin,
                               1.0, 59.0);
    g_signal_connect ((GObject *) window_update_cost->spinb_fmin, "value-changed",
                      (GCallback) set_label_sumr_window_update_cost, NULL);
    
    window_update_cost->spinb_imin = get_widget ("spinb_imin");
    gtk_spin_button_set_digits ((GtkSpinButton *) window_update_cost->spinb_imin, 0);
    gtk_spin_button_set_increments ((GtkSpinButton *) window_update_cost->spinb_imin,
                                    1.0, 5.0);
    gtk_spin_button_set_range ((GtkSpinButton *) window_update_cost->spinb_imin,
                               1.0, 59.0);
    g_signal_connect ((GObject *) window_update_cost->spinb_imin, "value-changed",
                      (GCallback) set_label_sumr_window_update_cost, NULL);
    
    window_update_cost->spinb_icost = get_widget ("spinb_icost");
    gtk_spin_button_set_digits ((GtkSpinButton *) window_update_cost->spinb_icost, 0);
    gtk_spin_button_set_increments ((GtkSpinButton *) window_update_cost->spinb_icost,
                                    50.0, 100.0);
    gtk_spin_button_set_range ((GtkSpinButton *) window_update_cost->spinb_icost,
                               50.0, 1000000.0);
    g_signal_connect ((GObject *) window_update_cost->spinb_icost, "value-changed",
                      (GCallback) set_label_sumr_window_update_cost, NULL);

    window_update_cost->label_sumr = get_widget ("label_sumr");

    gtk_spin_button_set_value ((GtkSpinButton *) window_update_cost->spinb_fcost, 1500.0);
    gtk_spin_button_set_value ((GtkSpinButton *) window_update_cost->spinb_fmin, 30.0);
    gtk_spin_button_set_value ((GtkSpinButton *) window_update_cost->spinb_imin, 10.0);
    gtk_spin_button_set_value ((GtkSpinButton *) window_update_cost->spinb_icost, 500.0);
    
    window_update_cost->button_close = get_widget ("button_close");
    g_signal_connect ((GObject *) window_update_cost->button_close, "clicked",
                      (GCallback) on_window_update_cost_delete_event, NULL);
    
    window_update_cost->button_ok = get_widget ("button_ok");
    g_signal_connect ((GObject *) window_update_cost->button_ok, "clicked",
                      (GCallback) on_button_ok_window_update_cost_clicked, 
                      GINT_TO_POINTER (mode));
    
    builder_destroy ();
}

void
create_window_rotate_cost (GbillingUpdateMode mode)
{
    builder_init ("cost_rotate.ui");

    window_rotate_cost = g_new (WindowRotateCost, 1);

    window_rotate_cost->window = get_widget ("window");
    set_window_icon ((GtkWindow *) window_rotate_cost->window);
    gtk_window_set_title ((GtkWindow *) window_rotate_cost->window, 
                          mode == GBILLING_UPDATE_MODE_ADD ? 
                              "Tambah Rotasi" : "Edit Rotasi");
    gtk_window_set_transient_for ((GtkWindow *) window_rotate_cost->window,
                                  (GtkWindow *) window_setting_cost->window);
    g_signal_connect ((GObject *) window_rotate_cost->window, "delete-event",
                      (GCallback) on_window_rotate_cost_deleve_event, NULL);

    window_rotate_cost->combob_cost = get_widget ("combob_cost");
    gtk_widget_set_sensitive (window_rotate_cost->combob_cost, 
                              mode == GBILLING_UPDATE_MODE_ADD);

    window_rotate_cost->checkb_active = get_widget ("checkb_active");
    if (mode == GBILLING_UPDATE_MODE_ADD)
        gtk_toggle_button_set_active ((GtkToggleButton *) 
                                      window_rotate_cost->checkb_active, TRUE);
    g_signal_connect ((GObject *) window_rotate_cost->checkb_active, "toggled",
                      (GCallback) on_checkb_active_window_rotate_cost_toggled, NULL);

    window_rotate_cost->spinb_shour = get_widget ("spinb_shour");
    gtk_spin_button_set_range ((GtkSpinButton *) window_rotate_cost->spinb_shour, 0, 23);

    window_rotate_cost->spinb_smin = get_widget ("spinb_smin");
    gtk_spin_button_set_range ((GtkSpinButton *) window_rotate_cost->spinb_smin, 0, 59);
    
    window_rotate_cost->spinb_ehour = get_widget ("spinb_ehour");
    gtk_spin_button_set_range ((GtkSpinButton *) window_rotate_cost->spinb_ehour, 0, 23);

    window_rotate_cost->spinb_emin = get_widget ("spinb_emin");
    gtk_spin_button_set_range ((GtkSpinButton *) window_rotate_cost->spinb_emin, 0, 59);

    window_rotate_cost->button_close = get_widget ("button_close");
    g_signal_connect ((GObject *) window_rotate_cost->button_close, "clicked",  
                      (GCallback) on_window_rotate_cost_deleve_event, NULL);

    window_rotate_cost->button_ok = get_widget ("button_ok");
    g_signal_connect ((GObject *) window_rotate_cost->button_ok, "clicked",
                      (GCallback) on_button_ok_window_rotate_cost_clicked, 
                      GINT_TO_POINTER (mode));
    
    builder_destroy ();
}

void
create_window_setting_prepaid (void)
{
    GtkWidget *widget;
    GtkListStore *store;
    GtkCellRenderer *render_name;
    GtkCellRenderer *render_duration;
    GtkCellRenderer *render_cost;
    GtkCellRenderer *render_active;
    GtkTreeViewColumn *column_name;
    GtkTreeViewColumn *column_duration;
    GtkTreeViewColumn *column_cost;
    GtkTreeViewColumn *column_active;
    
    builder_init ("prepaid_setting.ui");
    window_setting_prepaid = g_new (WindowSettingPrepaid, 1);
    
    window_setting_prepaid->window = get_widget ("window");
    set_window_icon ((GtkWindow *) window_setting_prepaid->window);
    gtk_window_set_transient_for ((GtkWindow *) window_setting_prepaid->window,
                                  (GtkWindow *) window_main->window);
    gtk_window_set_title ((GtkWindow *) window_setting_prepaid->window,
                          "Pengaturan Paket");
    g_signal_connect ((GObject *) window_setting_prepaid->window, "delete-event",
                      (GCallback) on_window_setting_prepaid_delete_event, NULL);
    set_window_icon ((GtkWindow *) window_setting_prepaid->window);

    render_name = gtk_cell_renderer_text_new ();
    column_name = gtk_tree_view_column_new_with_attributes ("Nama", render_name,
                                                "text", SCPREPAID_NAME, NULL);
    gtk_tree_view_column_set_min_width (column_name, 120);
    gtk_tree_view_column_set_expand (column_name, TRUE);

    render_duration = gtk_cell_renderer_text_new ();
    g_object_set ((GObject *) render_duration, "xalign", 0.5, NULL);
    
    column_duration = gtk_tree_view_column_new_with_attributes ("Durasi",
                              render_duration, "text", SCPREPAID_DURATION, NULL);
    gtk_tree_view_column_set_expand (column_duration, TRUE);
    gtk_tree_view_column_set_alignment (column_duration, 0.5);

    render_cost = gtk_cell_renderer_text_new ();    
    column_cost = gtk_tree_view_column_new_with_attributes ("Tarif", render_cost,
                                                "text", SCPREPAID_COST, NULL);
    gtk_tree_view_column_set_min_width (column_cost, 60);
    gtk_tree_view_column_set_expand (column_cost, TRUE);

    render_active = gtk_cell_renderer_toggle_new ();
    column_active = gtk_tree_view_column_new_with_attributes ("Aktif",
                                render_active, "active", SCPREPAID_ACTIVE, NULL);
    gtk_tree_view_column_set_alignment (column_active, 0.5);
    gtk_tree_view_column_set_expand (column_active, TRUE);

    store = gtk_list_store_new (SCPREPAID_COLUMN, G_TYPE_STRING, 
                                G_TYPE_UINT, G_TYPE_ULONG, G_TYPE_BOOLEAN);

    window_setting_prepaid->treeview = get_widget ("treeview");
    TEMP_NAME (widget, window_setting_prepaid->treeview);
    gtk_tree_view_set_model ((GtkTreeView *) widget, (GtkTreeModel *) store);
    gtk_tree_view_set_rules_hint ((GtkTreeView *) window_setting_prepaid->treeview, TRUE);
    gtk_tree_view_append_column ((GtkTreeView *) widget, column_name);
    gtk_tree_view_append_column ((GtkTreeView *) widget, column_duration);
    gtk_tree_view_append_column ((GtkTreeView *) widget, column_cost);
    gtk_tree_view_append_column ((GtkTreeView *) widget, column_active);
    
    window_setting_prepaid->button_add = get_widget ("button_add");
    g_signal_connect ((GObject *) window_setting_prepaid->button_add, "clicked",
            (GCallback) on_button_add_window_setting_prepaid_clicked, NULL);
    
    window_setting_prepaid->button_edit = get_widget ("button_edit");
    g_signal_connect ((GObject *) window_setting_prepaid->button_edit, "clicked",
            (GCallback) on_button_edit_window_setting_prepaid_clicked, NULL);
    
    window_setting_prepaid->button_del = get_widget ("button_del");
    g_signal_connect ((GObject *) window_setting_prepaid->button_del, "clicked",
            (GCallback) on_button_del_window_setting_prepaid_clicked, NULL);
    
    window_setting_prepaid->button_ok = get_widget ("button_ok");
    g_signal_connect ((GObject *) window_setting_prepaid->button_ok, "clicked",
                      (GCallback) on_window_setting_prepaid_delete_event, NULL);
    
    builder_destroy ();
}

void
create_window_update_prepaid (GbillingUpdateMode mode)
{
    g_return_if_fail (mode >= GBILLING_UPDATE_MODE_ADD &&
                      mode <= GBILLING_UPDATE_MODE_EDIT);
    GbillingPrepaid prepaid;

    builder_init ("update_prepaid.ui");
    window_update_prepaid = g_new (WindowUpdatePrepaid, 1);
    
    window_update_prepaid->window = get_widget ("window");
    set_window_icon ((GtkWindow *) window_update_prepaid->window);
    gtk_window_set_transient_for ((GtkWindow *) window_update_prepaid->window,
                                  (GtkWindow *) window_setting_prepaid->window);
    gtk_window_set_title ((GtkWindow *) window_update_prepaid->window,
                          mode == GBILLING_UPDATE_MODE_ADD ? 
                          "Tambah Paket" : "Edit Paket");
    g_signal_connect ((GObject *) window_update_prepaid->window, "delete-event",
                      (GCallback) on_window_update_prepaid_delete_event, NULL);
    
    window_update_prepaid->entry_name = get_widget ("entry_name");
    gtk_entry_set_max_length ((GtkEntry *) window_update_prepaid->entry_name,
                              sizeof(prepaid.name) - 1);
    
    window_update_prepaid->spinb_dur = get_widget ("spinb_dur");
    gtk_spin_button_set_range ((GtkSpinButton *) 
                               window_update_prepaid->spinb_dur, 1.0, 60 * 24);
    
    window_update_prepaid->spinb_cost = get_widget ("spinb_cost");
    gtk_spin_button_set_range ((GtkSpinButton *)    
                               window_update_prepaid->spinb_cost, 100.0, 1000000.0);
    gtk_spin_button_set_increments ((GtkSpinButton *) window_update_prepaid->spinb_cost,
                                    100.0, 500.0);
    gtk_spin_button_set_snap_to_ticks ((GtkSpinButton *) 
                                       window_update_prepaid->spinb_cost, TRUE);
    
    window_update_prepaid->checkb_status = get_widget ("checkb_status");
    
    window_update_prepaid->button_close = get_widget ("button_close");
    g_signal_connect ((GObject *) window_update_prepaid->button_close, "clicked",
                      (GCallback) on_window_update_prepaid_delete_event, NULL);
    
    window_update_prepaid->button_ok = get_widget ("button_ok");
    g_signal_connect ((GObject *) window_update_prepaid->button_ok, "clicked",
                      (GCallback) on_button_ok_window_update_prepaid_clicked, 
                      GINT_TO_POINTER (mode));
    
    builder_destroy ();
}

void
create_window_setting_item (void)
{
    GtkCellRenderer *render_name;
    GtkCellRenderer *render_cost;
    GtkCellRenderer *render_active;
    GtkTreeViewColumn *column_name;
    GtkTreeViewColumn *column_cost;
    GtkTreeViewColumn *column_active;

    builder_init ("setting_item.ui");
    window_setting_item = g_new (WindowSettingItem, 1);
    
    window_setting_item->window = get_widget ("window");
    set_window_icon ((GtkWindow *) window_setting_item->window);
    gtk_window_set_title ((GtkWindow *) window_setting_item->window, "Pengaturan Item");
    gtk_window_set_transient_for ((GtkWindow *) window_setting_item->window,
                                  (GtkWindow *) window_main->window);
    g_signal_connect ((GObject *) window_setting_item->window, "delete-event",
                      (GCallback) on_window_setting_item_delete_event, NULL);
    
    render_name = gtk_cell_renderer_text_new ();
    column_name = gtk_tree_view_column_new_with_attributes ("Name", render_name,
                                                "text", SCITEM_NAME, NULL);
    gtk_tree_view_column_set_min_width (column_name, 150);
    gtk_tree_view_column_set_expand (column_name, TRUE);
        
    render_cost = gtk_cell_renderer_text_new ();
    column_cost = gtk_tree_view_column_new_with_attributes ("Harga", render_cost,
                                                "text", SCITEM_COST, NULL);
    gtk_tree_view_column_set_min_width (column_cost, 80);
    gtk_tree_view_column_set_expand (column_cost, TRUE);
        
    render_active = gtk_cell_renderer_toggle_new ();
    g_object_set ((GObject *) render_active, "xalign", 0.5, NULL);
    
    column_active = gtk_tree_view_column_new_with_attributes ("Aktif", render_active,
                                                "active", SCITEM_ACTIVE, NULL);
    gtk_tree_view_column_set_alignment (column_active, 0.5);
    gtk_tree_view_column_set_expand (column_active, TRUE);
    
    window_setting_item->store = gtk_list_store_new (SCITEM_COLUMN, G_TYPE_STRING, 
                                                     G_TYPE_ULONG, G_TYPE_BOOLEAN);
    
    window_setting_item->treeview = get_widget ("treeview");
    gtk_tree_view_set_rules_hint ((GtkTreeView *) window_setting_item->treeview, TRUE);
    gtk_tree_view_set_model ((GtkTreeView *) window_setting_item->treeview, 
                             (GtkTreeModel *) window_setting_item->store);
    gtk_tree_view_append_column 
            ((GtkTreeView *) window_setting_item->treeview, column_name);
    gtk_tree_view_append_column 
            ((GtkTreeView *) window_setting_item->treeview, column_cost);
    gtk_tree_view_append_column 
            ((GtkTreeView *) window_setting_item->treeview, column_active);
    
    window_setting_item->button_add = get_widget ("button_add");
    g_signal_connect ((GObject *) window_setting_item->button_add, "clicked",
                      (GCallback) on_button_add_window_setting_item_clicked, NULL);
    
    window_setting_item->button_edit = get_widget ("button_edit");
    g_signal_connect ((GObject *) window_setting_item->button_edit, "clicked",
                      (GCallback) on_button_edit_window_setting_item_clicked, NULL);
    
    window_setting_item->button_del = get_widget ("button_del");
    g_signal_connect ((GObject *) window_setting_item->button_del, "clicked",
                      (GCallback) on_button_del_window_setting_item_clicked, NULL);
    
    window_setting_item->button_ok = get_widget ("button_ok");
    g_signal_connect ((GObject *) window_setting_item->button_ok, "clicked",
                      (GCallback) on_window_setting_item_delete_event, NULL);
}

void
create_window_update_item (GbillingUpdateMode mode)
{
    g_return_if_fail (mode >= GBILLING_UPDATE_MODE_ADD &&
                      mode <= GBILLING_UPDATE_MODE_EDIT);

    GbillingItem item;

    builder_init ("update_item.ui");
    window_update_item = g_new (WindowUpdateItem, 1);

    window_update_item->window = get_widget ("window");
    set_window_icon ((GtkWindow *) window_update_item->window);
    gtk_window_set_transient_for ((GtkWindow *) window_update_item->window,
                                  (GtkWindow *) window_setting_item->window);
    gtk_window_set_title ((GtkWindow *) window_update_item->window,
                          mode == GBILLING_UPDATE_MODE_ADD ? 
                          "Tambah Item" : "Edit Item");
    g_signal_connect ((GObject *) window_update_item->window, "delete-event",
                      (GCallback) on_window_update_item_delete_event, NULL);
                      
    window_update_item->entry_name = get_widget ("entry_name");
    gtk_entry_set_max_length ((GtkEntry *) window_update_item->entry_name,
                              sizeof(item.name) - 1);
    
    window_update_item->spinb_price = get_widget ("spinb_price");
    gtk_spin_button_set_range ((GtkSpinButton *) window_update_item->spinb_price,
                               100, 1000000);
    gtk_spin_button_set_increments ((GtkSpinButton *) 
                                    window_update_item->spinb_price, 100, 500);
                               
    window_update_item->checkb_active = get_widget ("checkb_active");
                              
    window_update_item->button_close = get_widget ("button_close");
    g_signal_connect ((GObject *) window_update_item->button_close, "clicked",
                      (GCallback) on_window_update_item_delete_event, NULL);
    
    window_update_item->button_ok = get_widget ("button_ok");
    g_signal_connect ((GObject *) window_update_item->button_ok, "clicked",
                      (GCallback) on_button_ok_window_update_item_clicked, 
                      GINT_TO_POINTER (mode));
    builder_destroy ();
}

void
create_window_setting_log (void)
{
    GtkTreeModel *store_show, *store_del;
    GtkTreeIter iter;
    static const gchar *times[] = { "Menit", "Jam", "Hari" };
    gint i;

    builder_init ("log_setting.ui");
    window_setting_log = g_new (WindowSettingLog, 1);
    
    window_setting_log->window = get_widget ("window");
    set_window_icon ((GtkWindow *) window_setting_log->window);
    gtk_window_set_title ((GtkWindow *) window_setting_log->window, "Pengaturan Log");
    gtk_window_set_transient_for ((GtkWindow *) window_setting_log->window,
                                  (GtkWindow *) window_main->window);
    
    window_setting_log->spinb_show = get_widget ("spinb_show");
    
    window_setting_log->combob_show = get_widget ("combob_show");
    g_signal_connect ((GObject *) window_setting_log->combob_show, "changed",
                      (GCallback) on_combob_show_window_setting_log_changed, NULL);
    store_show = gtk_combo_box_get_model ((GtkComboBox *)
                                          window_setting_log->combob_show);
    for (i = 0; i < G_N_ELEMENTS (times); i++)
    {
        gtk_list_store_append ((GtkListStore *) store_show, &iter);
        gtk_list_store_set ((GtkListStore *) store_show, &iter, 0, times[i], -1);
    }

    window_setting_log->label_del = get_widget ("label_del");
    
    window_setting_log->checkb_del = get_widget ("checkb_del");
    g_signal_connect ((GObject *) window_setting_log->checkb_del, "toggled",
                      (GCallback) on_checkb_del_window_setting_log_toggled, NULL);
    
    window_setting_log->spinb_del = get_widget ("spinb_del");
    
    window_setting_log->combob_del = get_widget ("combob_del");
    g_signal_connect ((GObject *) window_setting_log->combob_del, "changed",
                      (GCallback) on_combob_del_window_setting_log_changed, NULL);
    store_del = gtk_combo_box_get_model ((GtkComboBox *)
                                         window_setting_log->combob_del);
    
    for (i = 0; i < G_N_ELEMENTS (times); i++)
    {
        gtk_list_store_append ((GtkListStore *) store_del, &iter);
        gtk_list_store_set ((GtkListStore *) store_del, &iter, 0, times[i], -1);
    }
    
    window_setting_log->entry_name = get_widget ("entry_name");
    g_signal_connect ((GObject *) window_setting_log->entry_name, "changed",
                      (GCallback) on_window_setting_log_entry_name_changed, NULL);

    window_setting_log->label_passwd = get_widget ("label_passwd");
    window_setting_log->label_passwdc = get_widget ("label_passwdc");
    
    window_setting_log->entry_passwd = get_widget ("entry_passwd");
    g_signal_connect ((GObject *) window_setting_log->entry_passwd, "changed",
                      (GCallback) on_window_setting_log_entry_passwd_changed, NULL);

    window_setting_log->entry_passwdc = get_widget ("entry_passwdc");
    g_signal_connect ((GObject *) window_setting_log->entry_passwdc, "changed",
                      (GCallback) on_window_setting_log_entry_passwdc_changed, NULL);

    window_setting_log->button_save = get_widget ("button_save");
    g_signal_connect ((GObject *) window_setting_log->button_save, "clicked",
                      (GCallback) on_window_setting_log_button_save_clicked, NULL);
    
    window_setting_log->button_ok = get_widget ("button_ok");
    g_signal_connect ((GObject *) window_setting_log->button_ok, "clicked",
                      (GCallback) on_button_ok_window_setting_log_clicked, NULL);
    
    builder_destroy ();
}

WindowChat*
create_window_chat (ClientData *cdata)
{
    WindowChat *chat;

    g_return_val_if_fail (cdata != NULL, NULL);

    builder_init ("chat.ui");
    chat = g_new (WindowChat, 1);

    chat->window = get_widget ("window");
    gtk_window_set_title ((GtkWindow *) chat->window, cdata->name);
    set_window_icon ((GtkWindow *) chat->window);

    g_signal_connect ((GObject *) chat->window, "delete-event", 
            (GCallback) gtk_widget_hide_on_delete, NULL);
    g_signal_connect ((GObject *) chat->window, "key-press-event", 
            (GCallback) on_wchat_window_key_press_event, cdata);

    chat->textv_log = get_widget ("textv_log");
    chat->textv_msg = get_widget ("textv_msg");

    chat->button_chat = get_widget ("button_chat");
    g_signal_connect ((GObject *) chat->button_chat, "clicked",
                      (GCallback) on_wchat_bsend_clicked, cdata);
    
    gtk_window_set_focus ((GtkWindow *) chat->window, chat->textv_msg);
    
    builder_destroy ();
    return chat;
}

void
window_chat_destroy (WindowChat *chat)
{
    g_return_if_fail (chat != NULL);
    gtk_widget_destroy (chat->window);
    g_free (chat);
}

gint
create_dialog_auth (GtkWindow    *parent,
                    const gchar  *msg,
                    GbillingAuth *auth)
{
    g_return_val_if_fail (msg != NULL, GTK_RESPONSE_CANCEL);

    GtkWidget *window;
    GtkWidget *label_msg;
    GtkWidget *entry_username;
    GtkWidget *entry_passwd;
    const gchar *tmp;
    gchar *sum, *markup;
    gint ret;

    builder_init ("auth.ui");
    
    window = get_widget ("dialog");
    gtk_window_set_title ((GtkWindow *) window, "Login");
    if (parent)
        gtk_window_set_transient_for ((GtkWindow *) window, parent);
    else
        set_window_icon ((GtkWindow *) window);

    label_msg = get_widget ("label_msg");
    markup = g_strdup_printf ("<b>%s</b>", msg);
    gtk_label_set_markup ((GtkLabel *) label_msg, markup);
    g_free (markup);
    
    entry_username = get_widget ("entry_username");
    gtk_entry_set_max_length ((GtkEntry *) entry_username, 
                              sizeof(auth->username) - 1);
    entry_passwd = get_widget ("entry_passwd");
    gtk_entry_set_max_length ((GtkEntry *) entry_passwd, 
                              sizeof(auth->username) - 1);

    ret = gtk_dialog_run ((GtkDialog *) window);
    while (ret == GTK_RESPONSE_OK)
    {
        ret = GTK_RESPONSE_NO;
        tmp = gtk_entry_get_text ((GtkEntry *) entry_username);
        if (!strlen (tmp))
            break;
        g_snprintf (auth->username, sizeof(auth->username), tmp);
        tmp = gtk_entry_get_text ((GtkEntry *) entry_passwd);
        if (!strlen (tmp))
            break;
        sum = gbilling_str_checksum (tmp);
        g_snprintf (auth->passwd, sizeof(auth->passwd), sum);
        g_free (sum);

        ret = GTK_RESPONSE_OK;
        break;
    }

    gtk_widget_destroy (window);
    builder_destroy ();

    return ret;
}

void
create_window_detail (void)
{
    builder_init ("detail.ui");
    window_detail = g_new (WindowDetail, 1);
    
    window_detail->window = get_widget ("window");
    gtk_window_set_transient_for ((GtkWindow *) window_detail->window,
                                  (GtkWindow *) window_main->window);
    gtk_window_set_title ((GtkWindow *) window_detail->window, "Detail Log");
    g_signal_connect ((GObject *) window_detail->window, "delete-event",
                      (GCallback) on_window_detail_delete_event, NULL);
    
    window_detail->entry_client = get_widget ("entry_client");
    window_detail->entry_username = get_widget ("entry_username");
    window_detail->entry_type = get_widget ("entry_type");
    window_detail->entry_start = get_widget ("entry_start");
    window_detail->entry_end = get_widget ("entry_end");
    window_detail->entry_duration = get_widget ("entry_duration");
    window_detail->entry_usage = get_widget ("entry_usage");
    window_detail->entry_add = get_widget ("entry_add");
    window_detail->entry_desc = get_widget ("entry_desc");
    window_detail->entry_total = get_widget ("entry_total");
    
    window_detail->button_ok = get_widget ("button_ok");
    g_signal_connect ((GObject *) window_detail->button_ok, "clicked",
                      (GCallback) on_window_detail_delete_event, NULL);
    builder_destroy ();
}

void
create_window_total (void)
{
    builder_init ("total.ui");
    window_total = g_new (WindowTotal, 1);

    window_total->window = get_widget ("window");
    set_window_icon ((GtkWindow *) window_total->window);
    gtk_window_set_title ((GtkWindow *) window_total->window, "Total Log");
    gtk_window_set_transient_for ((GtkWindow *) window_total->window,
                                  (GtkWindow *) window_main->window);
    g_signal_connect ((GObject *) window_total->window, "delete-event",
                      (GCallback) on_window_logtotal_delete_event, NULL);

    window_total->entry_first = get_widget ("entry_first");
    window_total->entry_last = get_widget ("entry_last");
    window_total->entry_usage = get_widget ("entry_usage");
    window_total->entry_duration = get_widget ("entry_duration");
    window_total->entry_cost = get_widget ("entry_cost");

    window_total->button_ok = get_widget ("button_ok");
    g_signal_connect ((GObject *) window_total->button_ok, "clicked",
                      (GCallback) on_window_logtotal_delete_event, NULL);
    builder_destroy ();
}

void
create_window_move (void)
{
    builder_init ("move.ui");
    window_move = g_new (WindowMove, 1);

    window_move->window = get_widget ("window");
    set_window_icon ((GtkWindow *) window_move->window);
    gtk_window_set_title ((GtkWindow *) window_move->window, "Pindah Client");
    gtk_window_set_transient_for ((GtkWindow *) window_move->window,
                                  (GtkWindow *) window_main->window);
    g_signal_connect ((GObject *) window_move->window, "delete-event",
                      (GCallback) on_window_move_delete_event, NULL);

    window_move->label_move = get_widget ("label_move");
    window_move->combob_client = get_widget ("combob_client");

    window_move->button_close = get_widget ("button_close");
    g_signal_connect ((GObject *) window_move->button_close, "clicked",
                      (GCallback) on_window_move_delete_event, NULL);

    window_move->button_ok = get_widget ("button_ok");
    g_signal_connect ((GObject *) window_move->button_ok, "clicked",
                      (GCallback) on_button_moveok_clicked, NULL);
    builder_destroy ();
}

void
create_window_search (void)
{
    builder_init ("search.ui");
    window_search = g_new (WindowSearch, 1);

    window_search->window = get_widget ("window");
    gtk_window_set_title ((GtkWindow *) window_search->window, "Filter Log");
    set_window_icon ((GtkWindow *) window_search->window);
    gtk_window_set_transient_for ((GtkWindow *) window_search->window,
                                  (GtkWindow *) window_main->window);
    g_signal_connect ((GObject *) window_search->window, "delete-event",
                      (GCallback) on_window_logf_delete_event, NULL);

    window_search->checkb_client = get_widget ("checkb_client");
    g_signal_connect ((GObject *) window_search->checkb_client, "toggled",
                      (GCallback) on_checkb_wlogfclient_toggled, NULL);

    window_search->combob_client = get_widget ("combob_client");

    window_search->checkb_date_start = get_widget ("checkb_date_start");
    g_signal_connect ((GObject *) window_search->checkb_date_start, "toggled",
                      (GCallback) on_checkb_wlogfdate_toggled, NULL);

    window_search->toggleb_date_start = get_widget ("toggleb_date_start");
    g_signal_connect ((GObject *) window_search->toggleb_date_start, "toggled",
                      (GCallback) on_toggleb_wlogfdate_toggled, NULL);

    window_search->checkb_date_end = get_widget ("checkb_date_end");
    g_signal_connect ((GObject *) window_search->checkb_date_end, "toggled",
                      (GCallback) on_checkb_wlogfdate2_toggled, NULL);

    window_search->toggleb_date_end = get_widget ("toggleb_date_end");
    g_signal_connect ((GObject *) window_search->toggleb_date_end, "toggled",
                      (GCallback) on_toggleb_wlogfdate_toggled, NULL);

    window_search->label_username = get_widget ("label_username");

    window_search->checkb_username = get_widget ("checkb_username");
    g_signal_connect ((GObject *) window_search->checkb_username, "toggled",
                      (GCallback) on_checkb_username_window_search_toggled, NULL);

    window_search->entry_username = get_widget ("entry_username");

    window_search->checkb_type = get_widget ("checkb_type");
    g_signal_connect ((GObject *) window_search->checkb_type, "toggled",
                      (GCallback) on_checkb_wlogftype_toggled, NULL);

    window_search->combob_type = get_widget ("combob_type");

    window_search->checkb_time_start = get_widget ("checkb_time_start");
    g_signal_connect ((GObject *) window_search->checkb_time_start, "toggled",
                      (GCallback) on_checkb_wlogfstart_toggled, NULL);

    window_search->toggleb_time_start = get_widget ("toggleb_time_start");
    g_signal_connect ((GObject *) window_search->toggleb_time_start, "toggled",
                      (GCallback) on_toggleb_wlogfstart_toggled,
                      window_search->toggleb_time_start);

    window_search->checkb_time_end = get_widget ("checkb_time_end");
    g_signal_connect ((GObject *) window_search->checkb_time_end, "toggled",
                      (GCallback) on_checkb_wlogfend_toggled, NULL);

    window_search->toggleb_time_end = get_widget ("toggleb_time_end");
    g_signal_connect ((GObject *) window_search->toggleb_time_end, "toggled",
                      (GCallback) on_toggleb_wlogfstart_toggled,
                      window_search->toggleb_time_end);

    window_search->checkb_cost_start = get_widget ("checkb_cost_start");
    g_signal_connect ((GObject *) window_search->checkb_cost_start, "toggled",
                      (GCallback) on_checkb_wlogfcost_toggled, NULL);

    window_search->spinb_cost_start = get_widget ("spinb_cost_start");
    gtk_spin_button_set_increments ((GtkSpinButton *) 
                                    window_search->spinb_cost_start, 100, 500);
    gtk_spin_button_set_range ((GtkSpinButton *) 
                               window_search->spinb_cost_start, 0, 10000000);

    window_search->checkb_cost_end = get_widget ("checkb_cost_end");
    g_signal_connect ((GObject *) window_search->checkb_cost_end, "toggled",
                      (GCallback) on_checkb_wlogfcost2_toggled, NULL);

    window_search->spinb_cost_end = get_widget ("spinb_cost_end");
    gtk_spin_button_set_increments ((GtkSpinButton *) 
                                    window_search->spinb_cost_end, 100, 500);
    gtk_spin_button_set_range ((GtkSpinButton *) 
                               window_search->spinb_cost_end, 0, 10000000);

    window_search->button_close = get_widget ("button_close");
    g_signal_connect ((GObject *) window_search->button_close, "clicked",
                      (GCallback) on_window_logf_delete_event, NULL);

    window_search->button_ok = get_widget ("button_ok");
    g_signal_connect ((GObject *) window_search->button_ok, "clicked",
                      (GCallback) on_button_wlogfok_clicked, NULL);
    
    builder_destroy ();
}    

void
create_window_edit (void)
{
    builder_init ("edit.ui");
    window_edit = g_new (WindowEdit, 1);
    
    window_edit->window = get_widget ("window");
    set_window_icon ((GtkWindow *) window_edit->window);
    gtk_window_set_title ((GtkWindow *) window_edit->window, "Edit Tarif");
    gtk_window_set_transient_for ((GtkWindow *) window_edit->window,
                                  (GtkWindow *) window_main->window);
    g_signal_connect ((GObject *) window_edit->window, "delete-event",
                      (GCallback) on_window_edit_delete_event, NULL);

    window_edit->label_client = get_widget ("label_client");
    window_edit->label_use = get_widget ("label_use");

    window_edit->spinb_add = get_widget ("spinb_add");
    gtk_spin_button_set_range ((GtkSpinButton *) window_edit->spinb_add,
                                0, 1000000);
    gtk_spin_button_set_increments ((GtkSpinButton *) window_edit->spinb_add, 50, 100);
    g_signal_connect ((GObject *) window_edit->spinb_add, "value-changed",
                      (GCallback) on_spinb_editacost_value_changed, NULL);
    
    window_edit->entry_info = get_widget ("entry_info");
    window_edit->label_total = get_widget ("label_total");
    
    window_edit->button_ok = get_widget ("button_ok");
    g_signal_connect ((GObject *) window_edit->button_ok, "clicked",
                      (GCallback) on_button_editok_clicked, NULL);
    builder_destroy ();
}

void
create_window_calendar (GtkWindow *parent)
{
    builder_init ("calendar.ui");
    window_calendar = g_new (WindowCalendar, 1);
    
    window_calendar->window = get_widget ("window");
    gtk_window_set_title ((GtkWindow *) window_calendar->window, "Set Tanggal");
    gtk_window_set_transient_for ((GtkWindow *) window_calendar->window, parent);
    g_signal_connect ((GObject *) window_calendar->window, "delete-event",
                      (GCallback) on_window_calendar_delete_event, NULL);
    
    window_calendar->calendar = get_widget ("calendar");
    g_signal_connect ((GObject *) window_calendar->calendar, "day-selected-double-click",
                      (GCallback) on_calendar_day_selected_double_click, NULL);
    builder_destroy ();
}

void
create_window_time (GtkWindow *parent)
{
    builder_init ("time.ui");
    window_time = g_new (WindowTime, 1);
    
    window_time->window = get_widget ("window");
    gtk_window_set_title ((GtkWindow *) window_time->window, "Set Waktu");
    gtk_window_set_transient_for ((GtkWindow *) window_time->window, parent);
    g_signal_connect ((GObject *) window_time->window, "delete-event",
                      (GCallback) on_window_time_delete_event, NULL);
                      
    window_time->spinb_hour = get_widget ("spinb_hour");
    gtk_spin_button_set_range ((GtkSpinButton *) window_time->spinb_hour, 0, 23);
    
    window_time->spinb_min = get_widget ("spinb_min");
    gtk_spin_button_set_range ((GtkSpinButton *) window_time->spinb_min, 0, 59);
    
    window_time->spinb_sec = get_widget ("spinb_sec");
    gtk_spin_button_set_range ((GtkSpinButton *) window_time->spinb_sec, 0, 59);
    
    window_time->button_ok = get_widget ("button_ok");
    g_signal_connect ((GObject *) window_time->button_ok, "clicked",
                      (GCallback) on_button_wtimeok_clicked, NULL);
    builder_destroy ();
}

