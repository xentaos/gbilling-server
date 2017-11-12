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
 *  server.c
 *  This file is part of gBilling (http://gbilling.sourceforge.net)
 *  Copyright (C) 2008 - 2011, Ardhan Madras <ajhwb@knac.com>
 */
 

#ifdef _WIN32
# include <windows.h>
# include <winsock2.h>
#else
# include <arpa/inet.h>
# include <sys/socket.h>
# include <sys/select.h>
# include <sys/time.h>
# include <sys/wait.h>
#endif

#include <glib.h>
#include <gtk/gtk.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "gbilling.h"
#include "server.h"
#include "gui.h"
#include "sockc.h"
#include "sqldb.h"
#include "sound.h"

#define SERVCMD_BUFFER 5
#define SERVCMD_TIMEOUT 5
#define ACCEPT_TIMEOUT 1
#define RECV_TIMEOUT 5
#define ACTIVE_TIMEOUT 3


/*
 * Berikut adalah data struktur - data struktur local `helper', hanya untuk
 * digunakan sebagai data yang di pass ke rutin yang menangani kerja.
 *
 * Mengapa kita perlu ini?, pertama adalah untuk `sebisanya' menghindari
 * race-condition pada update data client dan logging di thread
 * handle_client() dengan melakukannya di mainloop, kedua adalah
 * untuk konsistensi code ini.
 */

/**
 * Data yang di-pass dari accept_client() untuk diproses di
 * thread handle_client()
 */
typedef struct
{
    gint socket;
    gint id;
} AcceptData;

/* Digunakan untuk passing data ke rutin login */
typedef struct
{
    ClientData        *cdata;
    GbillingClientZet *cset;
} LoginData;

/* Digunakan untuk passing data ke rutin chat */
typedef struct
{
    ClientData *cdata;
    gchar      *msg;
} ChatData;

/* Digunakan untuk passing data ke rutin enable/disable akses client */
typedef struct
{
    ClientData *cdata;
    gboolean    status;
} AccessData;

struct _server *server;

/*
 * Sinkronisasi dan proteksi data server, mutex ini akan
 * di lock setiap baca dan tulis data server.
 */
GStaticMutex server_mutex = G_STATIC_MUTEX_INIT;

GbillingServerZmd servcmd[SERVCMD_BUFFER];

GList *client_list = NULL;
GStaticMutex client_mutex = G_STATIC_MUTEX_INIT;
GStaticMutex servcmd_mutex = G_STATIC_MUTEX_INIT;

GPid ping_pid = 0;

GbillingCost *default_cost = NULL;

GbillingServerInfo *server_info = NULL;
GbillingCostList *cost_list = NULL;
GbillingCostRotationList *costrotation_list = NULL;
GbillingPrepaidList *prepaid_list = NULL;
GbillingItemList *item_list = NULL;
GbillingMemberGroupList *group_list = NULL;

const SoundOption default_soundopt[11] =
{
    {  FALSE,  NULL          },
    {  FALSE,  NULL          },
    {  FALSE,  NULL          },
    {  FALSE,  NULL          },
    {  FALSE,  NULL          },
    {  TRUE,   "login.wav"   },
    {  FALSE,  NULL          },
    {  FALSE,  NULL          },
    {  TRUE,   "logout.wav"  },
    {  TRUE,   "receive.wav" },
    {  FALSE,  NULL          }
};

SoundOption current_soundopt[11];

static void remove_servcmd (ClientData *cdata);
static gint servcmd_lookup (const ClientData *cdata);
static gboolean do_client_chat (ChatData *chat);
static gpointer handle_client (gpointer data);
static gboolean update_client_row (gpointer data);
static gboolean sound_play_in_mainloop (gpointer data);

/**
 * get_first_time:
 * @t: Waktu.
 * Returns: Waktu pertama.
 *
 * Hitung waktu pertama dalam hari pada jam 00:00:00 hari tersebut.
 */
time_t
get_first_time (time_t t)
{
    struct tm *st = localtime (&t);

    t -= st->tm_hour * 3600;
    t -= st->tm_min * 60;
    t -= st->tm_sec;
    return t;
}

/**
 * get_client_by_name:
 * @name: Nama client.
 * Returns: Data client, NULL jika tidak ditemukan.
 *
 * Ambil data client berdasarkan nama client.
 **/
ClientData*
get_client_by_name (const gchar *name)
{
    g_return_val_if_fail (name != NULL, NULL);
    GList *ptr;
    ClientData *data = NULL;

    for (ptr = client_list; ptr; ptr = g_list_next (ptr))
    {
        data = ptr->data;
        if (!strcmp (name, data->name))
            break;
    }
    if (!ptr)
        return NULL;
    return data;
}

/**
 * get_client_by_ip:
 * @ip: IP Address client.
 * Returns: Data client, NULL jika tidak ditemukan.
 *
 * Ambil data client berdasarkan alamat IP address client.
 **/
ClientData*
get_client_by_ip (const gchar *ip)
{
    g_return_val_if_fail (ip != NULL, NULL);
    GList *ptr;
    ClientData *data = NULL;

    for (ptr = client_list; ptr; ptr = g_list_next (ptr))
    {
        data = ptr->data;
        if (!strcmp (ip, data->ip))
            break;
    }
    if (!ptr)
        return NULL;
    return data;
}

GbillingCost*
get_cost (const gchar *name)
{
    g_return_val_if_fail (name != NULL, NULL);
    
    GbillingCostList *ptr;
    GbillingCost *cost = NULL, *tmp;
    
    for (ptr = cost_list; ptr; ptr = ptr->next)
    {
        tmp = ptr->data;
        if (!strcmp (tmp->name, name))
        {
            cost = tmp;
            break;
        }
    }
    return cost;
}

GbillingCost*
get_current_cost (void)
{
    GbillingCostList *clist = cost_list;
    GbillingCostRotationList *rlist = costrotation_list;
    GbillingCost *cost = NULL;
    GbillingCostRotation *rotation;

    time_t t;
    struct tm *st;
    gchar minutes_in_day[60 * 24];
    gint total_minute = 60 * 24;
    gint start, end, current;
    gint id = 0;

    if (!clist) 
        return cost;

    if (!rlist)
    {
        cost = (GbillingCost *) gbilling_default_cost (cost_list);
        return cost;
    }

    /* FIXME: Not thread safe!, who care? */
    t = time (NULL);
    st = localtime (&t);

    current = st->tm_hour * 60 + st->tm_min;
    memset (minutes_in_day, -1, total_minute);

    /*
     * Misalnya interval-interval waktu berikut ini:
     * 1) 13.20  14.00  16.00  
     * 2) 23.00  04.00  08.00 
     * 3) 08.00  12.00  05.00
     */
    while (rlist)
    {
        rotation = rlist->data;
        if (!rotation->state)
        {
            rlist = rlist->next;
            id++;
            continue;
        }

        start = rotation->shour * 60 + rotation->smin;
        end = rotation->ehour * 60 + rotation->emin;

        if (start > end)
        {
            memset (minutes_in_day + start, id, total_minute - start);
            if (end)
                memset (minutes_in_day, id, end - 1);
        }
        else if (start < end)
            memset (minutes_in_day + start - 1, id, end - start - 1);
        else
            g_return_val_if_fail (start != end, NULL);

        rlist = rlist->next;
        id++;
    }

    if (minutes_in_day[current - 1] == -1)
        cost = (GbillingCost *) gbilling_default_cost (cost_list);
    else
    {
        rotation = g_list_nth_data (costrotation_list, minutes_in_day[current - 1]);
        g_return_val_if_fail (rotation != NULL, NULL);
        cost = get_cost (rotation->cost);
    }

    return cost;
}

GbillingPrepaid*
get_prepaid (const gchar *name)
{
    g_return_val_if_fail (name != NULL, NULL);
    
    GbillingPrepaidList *ptr;
    GbillingPrepaid *prepaid = NULL, *tmp;
    
    for (ptr = prepaid_list; ptr; ptr = ptr->next)
    {
        tmp = ptr->data;
        if (!strcmp (tmp->name, name))
        {
            prepaid = tmp;
            break;
        }
    }
    return prepaid;
}

GbillingItem*
get_item (const gchar *name)
{
    g_return_val_if_fail (name != NULL, NULL);
    
    GbillingItemList *ptr;
    GbillingItem *item = NULL, *tmp;
    
    for (ptr = item_list; ptr; ptr = ptr->next)
    {
        tmp = ptr->data;
        if (!strcmp (tmp->name, name))
        {
            item = tmp;
            break;
        }
    }
    return item;
}

GbillingMemberGroup*
get_group (const gchar *name)
{
    g_return_val_if_fail (name != NULL, NULL);
    
    GbillingMemberGroupList *ptr;
    GbillingMemberGroup *group = NULL, *tmp;
    
    for (ptr = group_list; ptr; ptr = ptr->next)
    {
        tmp = ptr->data;
        if (!strcmp (tmp->name, name))
        {
            group = tmp;
            break;
        }
    }
    return group;
}

/**
 * set_servcmd:
 * @client: Client.
 * @cmd: Perintah server.
 * @msg: Pesan chat atau NULL.
 *
 * Returns: Indeks buffer yang terisi, -1 jika buffer full atau tidak ditemukan.
 *
 * Cari buffer yg kosong utk message queue client, buffer bisa saja tidak dikirim 
 * jika interval waktu set_servcmd() terlalu dekat dari scheduling request 
 * client (< 1 detik).
 */
gint
set_servcmd (ClientData      *client,
             GbillingCommand  cmd, 
             const gchar     *msg)
{
    g_return_val_if_fail (client != NULL, -1);
    if (cmd == GBILLING_COMMAND_CHAT)
        g_return_val_if_fail (msg != NULL, -1);

    gint i;
    gint retval = -1;

    for (i = 0; i < SERVCMD_BUFFER; i++)
    {
        g_static_mutex_lock (&servcmd_mutex);

        /*
         * Periksa buffer yang waktunya telah expire, buffer yang
         * telah expire akan diset statusnya manjadi kosong.
         */
        if (servcmd[i].stat)
            if (time (NULL) - servcmd[i].texpire >= SERVCMD_TIMEOUT)
            {
                servcmd[i].stat = 0;
                gbilling_debug ("%s: servcmd buffer %i expired\n", __func__, i + 1);
            }

        if (servcmd[i].stat == 0)
        {
            g_static_mutex_lock (&client_mutex);
            servcmd[i].id = g_list_index (client_list, client);
            g_static_mutex_unlock (&client_mutex);

            servcmd[i].cmd = cmd;
            if (cmd == GBILLING_COMMAND_LOGIN || cmd == GBILLING_COMMAND_RELOGIN)
            {
                servcmd[i].type = client->type;
                servcmd[i].idtype = client->idtype;
                servcmd[i].tstart = client->tstart;
                g_snprintf (servcmd[i].username, sizeof(servcmd[i].username),
                            client->username);
            }
            else if (cmd == GBILLING_COMMAND_CHAT)
                g_snprintf (servcmd[i].msg, sizeof(servcmd[i].msg), msg);

            servcmd[i].texpire = time (NULL);
            servcmd[i].stat = 1;
            retval = i;
            gbilling_debug ("%s: using servcmd %i\n", __func__, i + 1);
            g_static_mutex_unlock (&servcmd_mutex);
            break;
        }        
        g_static_mutex_unlock (&servcmd_mutex);
    }

    return retval;
}

/**
 * remove_servcmd:
 * @cdata: Data client.
 *
 * Kosongkan semua buffer client @cdata dari queue.
 */
static void
remove_servcmd (ClientData *cdata)
{
    g_return_if_fail (cdata != NULL);

    gint id = g_list_index (client_list, cdata);
    gint i, count = 0;

    for (i = 0; i < SERVCMD_BUFFER; i++)
    {
        g_static_mutex_lock (&servcmd_mutex);
        if (servcmd[i].stat && servcmd[i].id == id)
        {
            servcmd[i].stat = 0;
            count++;
        }
        g_static_mutex_unlock (&servcmd_mutex);
    }

    if (count)
        gbilling_debug ("%s: removed %i buffer for %s (%s)\n",
                        __func__, count, cdata->name, cdata->ip);
}

/**
 * Cari buffer client, return indeks buffer yang berisi data untuk client
 * -1 jika tidak ada.
 */
static gint
servcmd_lookup (const ClientData *cdata)
{
    gint i, id;

    id = g_list_index (client_list, cdata);
    g_return_val_if_fail (id != -1, -1);
    for (i = 0; i < SERVCMD_BUFFER; i++)
        if ((servcmd[i].id == id) && servcmd[i].stat)
            return i;
    return -1;
}

/* timeout, set column @ClIENT_STATUS di @treev_client, sesuai dgn 
 * @client[i].active dan client[i[.status
 */
static gboolean
update_column_status (ClientData *cdata)
{
    GtkTreeModel *model;
    GdkPixbuf *pixbuf, *pix;
    gchar *tmp;
    const gchar *state;
    static const gchar *status[] =
    {
        "Aktif",
        "Pindah",
        "Login",
        "Logout",
        "Nonaktif"
    };

    tmp = get_data_file ("client.png");
    pixbuf = gdk_pixbuf_new_from_file (tmp, NULL);
    g_free (tmp);
    g_return_val_if_fail (pixbuf != NULL, FALSE);
    pix = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (pixbuf),
                          gdk_pixbuf_get_has_alpha (pixbuf), 
                          gdk_pixbuf_get_bits_per_sample (pixbuf),
                          gdk_pixbuf_get_width (pixbuf),
                          gdk_pixbuf_get_height (pixbuf));
    gdk_pixbuf_saturate_and_pixelate (pixbuf, pix, 0.0, FALSE);
    
    if (cdata->active && cdata->status == GBILLING_STATUS_NONE)
        state = status[0];
    else if (cdata->active && cdata->status == GBILLING_STATUS_MOVE)
        state = status[1];
    else if (cdata->active && cdata->status == GBILLING_STATUS_LOGIN)
        state = status[2];
    else if (cdata->active && cdata->status == GBILLING_STATUS_LOGOUT)
        state = status[3];
    else
        state = status[4];
    model = gtk_tree_view_get_model ((GtkTreeView *) window_main->treeview_client);
    gtk_list_store_set ((GtkListStore *) model, &cdata->iter, 
                        CLIENT_STATUS, state, 
                        CLIENT_PING, cdata->pingact ? "\342\210\232" : "", 
                        CLIENT_ACCESS, cdata->access ? "Buka" : "Tutup",
                        -1);
    model = gtk_icon_view_get_model ((GtkIconView *) window_main->iconview_status);
    gtk_list_store_set ((GtkListStore *) model, &cdata->iterv, ICONV_PIXBUF, 
                         cdata->status == GBILLING_STATUS_LOGIN ? pixbuf : pix, -1); 

    g_object_unref (pixbuf);
    g_object_unref (pix);
    
    return FALSE;
}

gpointer
update_status (gpointer data)
{
    GList *ptr;
    ClientData *cdata;

    while (1)
    {
        for (ptr = client_list; ptr; ptr = ptr->next)
        {
            cdata = ptr->data;
            g_idle_add ((GSourceFunc) update_column_status, cdata);
            g_usleep (0.5 * G_USEC_PER_SEC);
        }
        gbilling_sleep (1);
    }
}

gboolean
update_client_data0 (ClientData *cdata)
{
    g_return_val_if_fail (cdata != NULL, FALSE);
    GbillingPrepaid *prepaid;
    gulong duration;

    cdata->duration = time (NULL) - cdata->tstart;
    cdata->tend = time (NULL);

    switch (cdata->type)
    {
        case GBILLING_LOGIN_TYPE_PREPAID:
            prepaid = g_list_nth_data (prepaid_list, cdata->idtype);
            cdata->ucost = prepaid->cost;
            break;

        case GBILLING_LOGIN_TYPE_REGULAR:
        case GBILLING_LOGIN_TYPE_MEMBER:
            /*
             * One finger salute for SIGFPE signal or a inifinite loop here.
             */
            g_return_val_if_fail (cdata->cost->imin > 0, FALSE);
            duration = time (NULL) - cdata->tstart;
            g_return_val_if_fail (duration >= 0, FALSE);
            cdata->ucost = gbilling_calculate_cost (duration, cdata->cost);
            break;
    }
    cdata->tcost = cdata->ucost + cdata->acost;
    update_client_row (cdata);
    return TRUE;
}

void start_timer (ClientData *cdata)
{
    g_return_if_fail (cdata != NULL);
    cdata->timer = g_timeout_add (1000, (GSourceFunc) update_client_data0, cdata);
}

gboolean
do_client_login0 (LoginData *login)
{
    g_return_val_if_fail (login != NULL, FALSE);

    GbillingClientZet *cset = login->cset;
    g_return_val_if_fail (login->cset != NULL, FALSE);

    ClientData *cdata = login->cdata;
    g_return_val_if_fail (login->cdata != NULL, FALSE);

    cdata->username = g_strdup (cset->auth.username);
    cdata->type = cset->type;
    cdata->tstart = time (NULL);

    if (cdata->type == GBILLING_LOGIN_TYPE_PREPAID)
        cdata->ucost = ((GbillingPrepaid *) 
                            g_list_nth_data (prepaid_list, cdata->idtype))->cost;
    else
        cdata->ucost = cdata->cost->fcost;
    cdata->acost = 0L;

    cdata->status = GBILLING_STATUS_LOGIN;
    update_client_data0 (cdata);
    cdata->timer = g_timeout_add (1000, (GSourceFunc) update_client_data0, cdata);

    gbilling_insert_log (cdata, cdata->status);

    /* Tidak perlu recovery untuk login administrasi */
    if (cdata->type != GBILLING_LOGIN_TYPE_ADMIN)
        gbilling_recov_write (cdata);

    if (cdata->type == GBILLING_LOGIN_TYPE_ADMIN &&
        current_soundopt[GBILLING_EVENT_LOGIN_ADMIN].play &&
        !server->mute_sound)
        sound_play (GBILLING_EVENT_LOGIN_ADMIN);

    if (cdata->type == GBILLING_LOGIN_TYPE_REGULAR &&
        current_soundopt[GBILLING_EVENT_LOGIN_REGULAR].play &&
        !server->mute_sound)
        sound_play (GBILLING_EVENT_LOGIN_REGULAR);

    if (cdata->type == GBILLING_LOGIN_TYPE_PREPAID &&
        current_soundopt[GBILLING_EVENT_LOGIN_PREPAID].play &&
        !server->mute_sound)
        sound_play (GBILLING_EVENT_LOGIN_PREPAID);

    g_free (login->cset);
    g_free (login);
    return FALSE;
}

gboolean
do_client_logout (ClientData *cdata)
{
    g_return_val_if_fail (cdata != NULL, FALSE);

    GtkTreeModel *model;
    gchar *start, *dur, *end, *cost;
    const gchar *type;

    if (!g_source_remove_by_user_data (cdata))
        gbilling_debug ("%s: could not remove timer source\n", __func__);

    cdata->status = GBILLING_STATUS_LOGOUT;
    cdata->tend = time (NULL);
    cdata->duration = cdata->tend - cdata->tstart;
    start = time_t_to_string (cdata->tstart);
    dur = time_to_string (cdata->duration);
    end = time_t_to_string (cdata->tend);
    cost = cost_to_string (cdata->ucost + cdata->acost);
    model = gtk_tree_view_get_model ((GtkTreeView *) window_main->treeview_client);

    if (cdata->type == GBILLING_LOGIN_TYPE_PREPAID)
        type = ((GbillingPrepaid *) g_list_nth_data (prepaid_list, cdata->idtype))->name;
    else
        type = gbilling_login_mode[cdata->type];

    gtk_list_store_set ((GtkListStore *) model, &cdata->iter,
                        CLIENT_USERNAME, cdata->username,
                        CLIENT_TYPE, type,
                        CLIENT_START, start,
                        CLIENT_DURATION, dur,
                        CLIENT_END, end,
                        CLIENT_COST, cost,
                        -1);

    gbilling_insert_log (cdata, cdata->status);
    gbilling_recov_del (cdata);

    g_free (cdata->username);
    if (cdata->desc)
    {
        g_free (cdata->desc);
        cdata->desc = NULL;
    }
    if (cdata->wchat)
    {
        gtk_widget_destroy (cdata->wchat->window);
        g_free (cdata->wchat);
        cdata->wchat = NULL;
    }

    if (server->autodisable && !server->autorestart)
        set_servcmd (cdata, GBILLING_COMMAND_DISABLE, NULL);
    if (server->autorestart)
        set_servcmd (cdata, GBILLING_COMMAND_RESTART, NULL);

    if (current_soundopt[GBILLING_EVENT_LOGOUT].play && !server->mute_sound)
        sound_play (GBILLING_EVENT_LOGOUT);

    g_free (start);
    g_free (dur);
    g_free (end);
    g_free (cost);
    return FALSE;
}

static gboolean
do_client_chat (ChatData *chat)
{
    g_return_val_if_fail (chat != NULL, FALSE);
    ClientData *cdata = chat->cdata;

    if (!cdata->wchat)
    {
        cdata->wchat = create_window_chat (cdata);
        gtk_widget_show (cdata->wchat->window);
    }
    else
        gtk_window_present ((GtkWindow *) cdata->wchat->window);
    fill_chat_log (cdata, chat->msg, GBILLING_CHAT_MODE_CLIENT);

    if (current_soundopt[GBILLING_EVENT_CHAT_RECEIVE].play && !server->mute_sound)
        sound_play (GBILLING_EVENT_CHAT_RECEIVE);

    g_free (chat->msg);
    g_free (chat);
    return FALSE;
}

static gboolean
do_client_access (AccessData *data)
{
    g_return_val_if_fail (data != NULL, FALSE);
    ClientData *cdata = data->cdata;
    cdata->access = data->status;
    g_free (data);

    return FALSE;
}

static gboolean
sound_play_in_mainloop (gpointer data)
{
    gint event = GPOINTER_TO_INT (data);

    if (current_soundopt[event].play && !server->mute_sound)
        sound_play (event);

    return FALSE;
}

/**
 * handle_client:
 * @data: Data koneksi yang dari thread accept_client().
 * Returns: Selalu NULL. 
 *
 * Handle client setiap koneksi socket, (iterative server model), 
 * ini multi-thread dan diproteksi dgn membuat data private untuk 
 * setiap thread, "thread local storage" (private data).
 */
static gpointer
handle_client (gpointer data)
{
    g_return_val_if_fail (data != NULL, NULL);

    static const struct timeval val = { .tv_sec = RECV_TIMEOUT, .tv_usec = 0 };
    static GStaticPrivate key = G_STATIC_PRIVATE_INIT;
    struct private
    {
        AcceptData *adata;
        ClientData *cdata;
        LoginData *login;
        ChatData *chat;
        AccessData *access;
        GbillingCost *cost;
        GbillingPrepaid *prepaid;
        GbillingMemberGroup *group;
        GList *ptr;
        GbillingClientZet cset;
        gchar cname[17];
        gchar msg[129];
        gsize bytes;
        gint8 ccmd;
        gint8 scmd;
        gint id;
        gint ret;
        gint32 timex;
    } *p = g_static_private_get (&key);

    if (p == NULL)
    {
        p = g_new0 (struct private, 1);
        p->adata = data;
        p->cdata = g_list_nth_data (client_list, p->adata->id);
        g_static_private_set (&key, p, g_free);
    }

    p->ret = gbilling_setsockopt (p->adata->socket, SOL_SOCKET, 
                                  SO_RCVTIMEO, &val, sizeof(val));
    if (p->ret == -1)
    {
        gbilling_sysdebug ("gbilling_setsockopt");
        goto done;
    }

    p->ret = gbilling_recv (p->adata->socket, &p->ccmd, sizeof(p->ccmd), 0);
    if (p->ret <= 0)
    {
        gbilling_sysdebug ("gbilling_recv client's cmd");
        goto done;
    }

    switch (p->ccmd)
    {
        case GBILLING_COMMAND_ACTIVE:
            /*
             * Buffer client dalam queue harus di kosongkan terlebih dahulu, 
             * ini karena mungkin saja buffer telah masuk queue (transisi 
             * status client dari aktif menjadi nonaktif).
             */
            remove_servcmd (p->cdata);

            p->scmd = GBILLING_COMMAND_ACCEPT;
            p->ret = gbilling_send (p->adata->socket, &p->scmd, sizeof(p->scmd), 0);
            if (p->ret <= 0)
            {
                gbilling_sysdebug ("gbilling_send server's cmd");
                goto done;
            }

            /*
             * Kirim waktu lokal server.
             */
            p->timex = g_htonl (time (NULL));
            p->ret = gbilling_data_send (p->adata->socket, &p->timex, sizeof(p->timex));
            if (p->ret <= 0)
            {
                gbilling_sysdebug ("gbilling_data_send server's localtime");
                goto done;
            }

            p->ret = gbilling_server_info_send (p->adata->socket, server_info);
            if (!p->ret)
            {
                gbilling_sysdebug ("gbilling_server_info_send");
                goto done;
            }

            /* 
             * Kirim nama client.
             */
            g_snprintf (p->cname, sizeof(p->cname), p->cdata->name);
            p->ret = gbilling_send (p->adata->socket, &p->cname, sizeof(p->cname), 0);
            if (p->ret <= 0)
            {
                gbilling_sysdebug ("gbilling_send client's name");
                goto done;
            }

            /* 
             * Kirim daftar prepaid, hanya untuk menampilkan nama prepaid,
             * tarif prepaid akan diminta pada saat login mode prepaid. 
             */
            p->ret = gbilling_prepaid_list_send (p->adata->socket, prepaid_list);
            if (!p->ret)
            {
                gbilling_sysdebug ("gbilling_prepaid_list_send");
                goto done;
            }

            /* Login ulang jika selain admin, logout jika admin. */
            if (p->cdata->status == GBILLING_STATUS_LOGIN)
            {
                if (p->cdata->type == GBILLING_LOGIN_TYPE_ADMIN)
                    do_client_logout (p->cdata);
                else
                {
                    gbilling_debug ("%s: %s (%s) set to re-login\n", __func__, 
                                    p->cdata->name, p->cdata->ip);
                    set_servcmd (p->cdata, GBILLING_COMMAND_RELOGIN, NULL);
                }
            }

            /* 
             * Kirim tarif, jika client sedang login tarif yang
             * dikirim adalah tarif current digunakan untuk login ulang, 
             * jika tidak kirim tarif default 
             */
            if (p->cdata->status == GBILLING_STATUS_LOGIN &&
                p->cdata->type == GBILLING_LOGIN_TYPE_REGULAR)
                p->cost = g_list_nth_data (cost_list, p->cdata->idtype);
            else
                p->cost = (GbillingCost *) gbilling_default_cost (cost_list);

            if (!p->cost)
            {
                gbilling_debug ("%s: cost could not be defined, please fix "
                                "your cost.\n", __func__);
                goto done;
            }

            p->ret = gbilling_cost_send (p->adata->socket, p->cost);
            if (!p->ret)
            {
                gbilling_sysdebug ("gbilling_cost_send");
                goto done;
            }


            g_idle_add ((GSourceFunc) sound_play_in_mainloop,
                        GINT_TO_POINTER (GBILLING_EVENT_CLIENT_ACTIVE));
            break;


        case GBILLING_COMMAND_LOGIN:
            p->ret = gbilling_clientset_recv (p->adata->socket, &p->cset);
            if (!p->ret)
            {
                gbilling_sysdebug ("gbilling_clientset_recv");
                goto done;
            }

            if (p->cset.type == GBILLING_LOGIN_TYPE_ADMIN)
            {
                if (gbilling_auth_compare (server->cauth, &p->cset.auth))
                    p->scmd = GBILLING_COMMAND_ACCEPT;
                else
                    p->scmd = GBILLING_COMMAND_REJECT;
                p->ret = gbilling_send (p->adata->socket, &p->scmd, sizeof(p->scmd), 0);
                if (p->scmd == GBILLING_COMMAND_REJECT)
                {
                    gbilling_debug ("%s: rejecting admin login for %s (%s)\n",
                                    __func__, p->cdata->ip, p->cdata->name);
                    goto done;
                }
                p->cost = get_current_cost ();
                if (!p->cost)
                {
                    gbilling_debug ("%s: no default cost, fix your cost setting\n", __func__);
                    goto done;
                }
                p->cdata->cost = p->cost;
            }
            else if (p->cset.type == GBILLING_LOGIN_TYPE_REGULAR)
            {
                p->cost = get_current_cost ();
                if (!p->cost)
                {
                    gbilling_debug ("%s: no default cost, fix your cost setting\n", __func__);
                    goto done;
                }
                gbilling_debug ("%s: using cost id %i\n", __func__, 
                                g_list_index (cost_list, p->cost));
                p->ret = gbilling_cost_send (p->adata->socket, p->cost);
                if (!p->ret)
                {
                    gbilling_sysdebug ("gbilling_cost_send");
                    goto done;
                }
                p->cdata->cost = p->cost;
                p->cdata->idtype = g_list_index (cost_list, p->cost);
            }
            else if (p->cset.type == GBILLING_LOGIN_TYPE_PREPAID)
            {
                p->prepaid = g_list_nth_data (prepaid_list, p->cset.id);
                if (!p->prepaid)
                {
                    gbilling_debug ("%s: unknown prepaid id %i\n", __func__, p->cset.id);
                    goto done;
                }
                p->cdata->idtype = p->cset.id;
            }
            else if (p->cset.type == GBILLING_LOGIN_TYPE_MEMBER)
            {
                gbilling_debug ("%s: member login is currently not supported\n", __func__);
                goto done;
            }
            else
            {
                gbilling_debug ("%s: unknown login type %i\n", __func__, p->cset.type);
                goto done;
            }

            p->login = g_new (LoginData, 1);
            p->login->cdata = p->cdata;
            p->login->cset = g_memdup (&p->cset, sizeof(p->cset));
            g_idle_add ((GSourceFunc) do_client_login0, p->login);
            break;


        case GBILLING_COMMAND_LOGOUT:
            g_idle_add ((GSourceFunc) do_client_logout, p->cdata);
            break;


        case GBILLING_COMMAND_CHAT:
            while (p->bytes < sizeof(p->msg))
            {
                p->ret = gbilling_recv (p->adata->socket, &p->msg + p->bytes, 
                                        sizeof(p->msg) - p->bytes, 0);
                if (p->ret == -1)
                {
                    gbilling_sysdebug ("gbilling_recv chat msg");
                    goto done;
                }
                else if (p->ret == 0)
                    break;
                p->bytes += p->ret;
            }

            p->chat = g_new (ChatData, 1);
            p->chat->cdata = p->cdata;
            p->chat->msg = g_strdup (p->msg);
            g_idle_add ((GSourceFunc) do_client_chat, p->chat);
            break;


        case GBILLING_COMMAND_REQUEST:
            p->id = servcmd_lookup (p->cdata); /* cari buffer client */
            if (p->id != -1)
            {
                g_static_mutex_lock (&servcmd_mutex);
                p->ret = gbilling_server_cmd_send (p->adata->socket, &servcmd[p->id]);
    
                /* kosongkan buffer, tidak peduli buffer berhasil di kirim
                   atau tidak, ini penting agar tidak ada pengiriman
                   buffer client lebih dari sekali, dan menjaga agar buffer 
                   tidak full. */

                servcmd[p->id].stat = 0;
                g_static_mutex_unlock (&servcmd_mutex);
                if (!p->ret)
                {
                    gbilling_sysdebug ("gbilling_server_cmd_send");
                    goto done;
                }
            }
            break;


        case GBILLING_COMMAND_UPDATE:
           break;


        case GBILLING_COMMAND_ENABLE:
        case GBILLING_COMMAND_DISABLE:
            p->access = g_new (AccessData, 1);
            p->access->cdata = p->cdata;
            p->access->status = (p->ccmd == GBILLING_COMMAND_ENABLE) ? TRUE : FALSE;
            g_idle_add ((GSourceFunc) do_client_access, p->access);
            break;


        case GBILLING_COMMAND_ITEM:
            p->ret = gbilling_item_list_send (p->adata->socket, item_list);
            if (!p->ret)
                gbilling_sysdebug ("gbilling_item_list_send");
            break;


        case GBILLING_COMMAND_TIME:
            p->timex = g_htonl (time (NULL));
            p->ret = gbilling_data_send (p->adata->socket, &p->timex, sizeof(p->timex));
            if (p->ret <= 0)
            {
                gbilling_sysdebug ("gbilling_data_send server's localtime");
                goto done;
            }
            break;


        default:
            gbilling_debug ("%s: %s (%s), unknown client command: %i\n", 
                            __func__, p->cdata->name, p->cdata->ip, p->ccmd);
    } /* switch */
    /* else 0, success atau disconnect */
done:
    gbilling_close (p->adata->socket);
    g_free (p->adata); /* remember this! */

    return NULL;
}

static gboolean
ping_error (gpointer data)
{
    gbilling_create_dialog (GTK_MESSAGE_ERROR, (GtkWindow *) window_main->window, 
              "Ping Client", "Utility <i>ping</i> sistem tidak dapat dijalankan.");
    return FALSE;
}

/**
 * Ping client dengan utility ping sistem.
 */
gpointer
ping_client (gpointer data)
{
    GList *ptr;
    ClientData *cdata;
    gboolean run;

#ifndef _WIN32
    GError *err = NULL;
    GPid w_pid;
    static gchar *args[] = { "ping", "-c 1", NULL, NULL };
    gint ret;
#else
    gchar *cmd;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    DWORD ret;

    memset (&si, 0, sizeof(si));
    memset (&pi, 0, sizeof(pi));
    si.cb = sizeof(si);
    /*
     * Set semua standard file ke hStdInput, hStdOutput dan hStdError.
     * dan gunakan member wShowWindow (winuser.h)
     * Console window tidak ditampilkan (winbase.h)
     */
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
#endif

    while (1)
    {
        for (ptr = client_list; ptr; ptr = g_list_next (ptr))
        {
            cdata = (ClientData *) ptr->data;
#ifndef _WIN32
            args[G_N_ELEMENTS (args) - 2] = cdata->ip;
            run = g_spawn_async (NULL, /* working dir */
                                 args, /* args */
                                 NULL, /* envp */
                                 G_SPAWN_DO_NOT_REAP_CHILD |
                                 G_SPAWN_SEARCH_PATH |
                                 G_SPAWN_STDOUT_TO_DEV_NULL |
                                 G_SPAWN_STDERR_TO_DEV_NULL,  /* flags */
                                 NULL, /* child_setup() */
                                 NULL, /* user data */
                                 &ping_pid, /* pid */
                                 &err); /* error */
#else
            cmd = g_strdup_printf ("ping -n 1 %s", cdata->ip);

            /*
             * GLib g_spawn*() tidak efektif di Win32 karena tidak ada pilihan
             * untuk tidak manampilkan console window, mungkin pilihan
             * ini nanti akan ada pada GLib 2.20, lihat bug #509201 GLib.
             * CreateProcess() adalah asynchronous, tidak akan menunggu proses
             * selesai sebelum return.
             */
            run = CreateProcess (NULL, /* ModuleName */
                                 cmd, /* CommandLine */
                                 NULL, /* ProcessSecurity */
                                 NULL, /* ThreadSecurity */
                                 TRUE, /* InheritHandles */
                                 NORMAL_PRIORITY_CLASS, /* CreateFlags */
                                 NULL, /* Environment */
                                 NULL, /* CurrentDir */
                                 &si, /* StartInfo */
                                 &pi); /* ProcessInfo */
            g_free (cmd);

#endif
            if (run)
            {
#ifndef _WIN32
                w_pid = waitpid (ping_pid, &ret, 0);
                if (w_pid == -1)
                    gbilling_sysdebug ("waitpid");
                g_spawn_close_pid (ping_pid);
#else
                ping_pid = pi.hProcess;
                ret = STILL_ACTIVE;
                while (ret == STILL_ACTIVE)
                {
                    WaitForSingleObject (ping_pid, 1000);
                    GetExitCodeProcess (ping_pid, &ret);
                }
                CloseHandle (pi.hThread);
                CloseHandle (ping_pid);
#endif
                /* Beruntung program ping pada Linux (iputils) dan Win32 mempunyai
                 * exit code '0' jika sukses, semoga sistem yang lain mempunyai 
                 * program ping dengan exit code sukses yang sama ;)
                 */
                cdata->pingact = !ret;
            }
            else
            {
#ifndef _WIN32
                gbilling_debug ("%s: %s\n", __func__, err->message);
                g_error_free (err);
#else
                gbilling_debug ("%s: %i\n", __func__, GetLastError ());
#endif
                g_idle_add ((GSourceFunc) ping_error, NULL);
                return NULL;
            }
            gbilling_sleep (1);
        }
        gbilling_sleep (1);
    }

    return NULL;
}

/**
 * Ini cara gbilling utk menentukan client yang tidak aktif (gbilling client 
 * tidak berjalan di komputer client): 
 * 
 * Jika client melakukan request, client.num akan diset ke ACTIVE_TIMEOUT 
 * di accept_client(), client.num kemudian akan dikurangi di check_status() 
 * sampai -ACTIVE_TIMEOUT, jika client.num client tersebut telah mencapai 
 * -ACTIVE_TIMEOUT, client dinyatakan tidak aktif (client.num tidak akan 
 * mencapi -ACTIVE_TIMEOUT jika client tersebut selalu melakukan request). 
 *
 * Btw, cara di versi beta1 sangat tidak baik dan memalukan ;p
 */
gpointer
check_status (gpointer data)
{
    GList *ptr;
    ClientData *cdata;
    guint len;

    while (1)
    {
        for (ptr = client_list, len = g_list_length (ptr);
             ptr != NULL; ptr = g_list_next (ptr))
        {
            g_static_mutex_lock (&client_mutex);
            cdata = ptr->data;
            if (cdata->num-- <= 0)
            {
                cdata->num = ACTIVE_TIMEOUT - 1;
                cdata->active = FALSE;

                /* 
                 * FIXME: Race condition dengan current_soundopt,
                 * tapi ini lebih baik daripada dispatch terus-menerus
                 * fungsi sound_play_in_mainloop().
                 */
                if (current_soundopt[GBILLING_EVENT_CLIENT_NONACTIVE].play)
                    g_idle_add ((GSourceFunc) sound_play_in_mainloop,
                                GINT_TO_POINTER (GBILLING_EVENT_CLIENT_NONACTIVE));
            }
            g_static_mutex_unlock (&client_mutex);
            g_usleep ((1 * G_USEC_PER_SEC) / len);
            g_usleep (150000);
        }
        g_usleep (150000);
    }
    return NULL;
}

/**
 * accept_client:
 * @data: Tidak dipakai.
 * Returns: Selalu NULL.
 *
 * Thread terima koneksi client, ini single-thread select() disini bukan 
 * untuk multiplexing, hanya untuk timeout agar accept() tidak dieksekusi.
 */
gpointer
accept_client (gpointer data)
{
    AcceptData *adata;
    ClientData *cdata;
    struct sockaddr_in addr;
    struct timeval val;
    guint len = sizeof(struct sockaddr_in);
    gint ret;
    fd_set rset;

    gbilling_debug ("%s: thread started\n", __func__);
    
    g_idle_add ((GSourceFunc) sound_play_in_mainloop,
                GINT_TO_POINTER (GBILLING_EVENT_SERVER_ACTIVE));

    while (server->active)
    {
        FD_ZERO (&rset);
        FD_SET (server->socket, &rset);
        val = (struct timeval) { ACCEPT_TIMEOUT, 0 };
        ret = gbilling_select (server->socket + 1, &rset, NULL, NULL, &val);
        if (ret == 0)
            continue;
        else if (ret == -1)
        {
            gbilling_sysdebug ("select");
            break;
        }

        adata = g_new (AcceptData, 1);
        adata->socket = gbilling_accept (server->socket, (struct sockaddr *) &addr, &len);
        if (adata->socket == -1)
        {
            gbilling_sysdebug ("accept");
            g_free (adata);
            continue;
        }
        /* reject client jika tidak ada dalam list server */
        cdata = get_client_by_ip (inet_ntoa (addr.sin_addr));
        if (!cdata)
        {
            gbilling_debug ("%s: unknown client %s\n", __func__, 
                            inet_ntoa (addr.sin_addr));
            gbilling_close (adata->socket);
            g_free (adata);
        }
        else
        {
            /* lihat check_status() */
            cdata->active = TRUE;
            cdata->num = ACTIVE_TIMEOUT;
            adata->id = g_list_index (client_list, cdata);
            /* @adata di release dari co-thread handle_client() */
            g_thread_create ((GThreadFunc) handle_client, adata, FALSE, NULL); 
        }
    }
    gbilling_debug ("%s: thread end\n", __func__);

    g_idle_add ((GSourceFunc) sound_play_in_mainloop,
                GINT_TO_POINTER (GBILLING_EVENT_SERVER_NONACTIVE));

    return NULL;
}

/* setup server, set listen socket server, alamat server, bind. */
gint
setup_server (void)
{
    struct sockaddr_in addr;
    gint reuseaddr = 1;
    gint ret;

#ifdef _WIN32
    WORD wsaver;
    WSADATA wsadata;

    wsaver = MAKEWORD (2, 2);
    if (WSAStartup (wsaver, &wsadata))
    {
        errno = WSAGetLastError ();
        WSACleanup ();
        return -1;
    }
#endif

    server->socket = gbilling_socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server->socket == -1)
    {
        gbilling_sysdebug ("socket");
        return -1;
    }
    ret = gbilling_setsockopt (server->socket, SOL_SOCKET, 
                               SO_REUSEADDR, &reuseaddr, sizeof(gint));
    if (ret == -1)
    {
        gbilling_sysdebug ("setsockopt");
        return -1;
    }

    memset (&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl (INADDR_ANY);
    addr.sin_port = g_htons (gbilling_server_get_port ());

    ret = gbilling_bind (server->socket, (struct sockaddr *) &addr, sizeof(addr));
    if (ret == -1)
    {
        gbilling_sysdebug ("bind");
        return -1;
    }
    ret = gbilling_listen (server->socket, 10);
    if (ret == -1)
    {
        gbilling_sysdebug ("listen");
        return -1;
    }

    return 0;
}

static gboolean
update_client_row (gpointer data)
{
    g_return_val_if_fail (data != NULL, FALSE);

    GbillingPrepaid *prepaid;
    ClientData *client = data;
    GtkTreeModel *model;
    gchar *type, *start, *dur, *cost;

    if (client->status != GBILLING_STATUS_LOGOUT && 
        client->status != GBILLING_STATUS_MOVE)
    {
        switch (client->type)
        {
            case GBILLING_LOGIN_TYPE_PREPAID:
                prepaid = g_list_nth_data (prepaid_list, client->idtype);
                type = (gchar *) prepaid->name;
                break;
            default:
                type = (gchar *) gbilling_login_mode[client->type];
        }
        start = time_t_to_string (client->tstart);
        dur = time_to_string (client->duration);
        cost = cost_to_string (client->tcost);
        model = gtk_tree_view_get_model ((GtkTreeView *) window_main->treeview_client);
        gtk_list_store_set ((GtkListStore *) model, &client->iter,
                            CLIENT_USERNAME, client->username,
                            CLIENT_TYPE, type,
                            CLIENT_START, start,
                            CLIENT_DURATION, dur,
                            CLIENT_END, "-",
                            CLIENT_COST, cost,
                            -1);
        g_free (start);
        g_free (dur);
        g_free (cost);
    }
    return FALSE;
}

gboolean
update_cost_display (gpointer data)
{
    GbillingCost *cost;
    guint id;

    cost = get_current_cost ();
    g_return_val_if_fail (cost != NULL, FALSE);

    id = gtk_statusbar_get_context_id ((GtkStatusbar *) window_main->statusbar_cost, cost->name);
    gtk_statusbar_pop ((GtkStatusbar *) window_main->statusbar_cost, id);
    gtk_statusbar_push ((GtkStatusbar *) window_main->statusbar_cost, id, cost->name);

    return TRUE;
}

void
fill_chat_log (const ClientData *cdata, 
               const gchar      *msg, 
               GbillingChatMode  mode)
{
    g_return_if_fail (cdata != NULL);
    g_return_if_fail (mode >= GBILLING_CHAT_MODE_SERVER && 
                      mode <= GBILLING_CHAT_MODE_CLIENT);
    g_return_if_fail (msg != NULL);

    GtkTextBuffer *tbuf;
    GtkTextIter iter;
    GtkTextMark *mark;
    GtkTextTag *tag;
    gchar *buf;
    struct tm *st;
    time_t t;

    t = time (NULL);
    st = localtime (&t);
    tbuf = gtk_text_view_get_buffer ((GtkTextView *) cdata->wchat->textv_log);
    tag = gtk_text_buffer_create_tag (tbuf, NULL, "weight", PANGO_WEIGHT_BOLD, NULL);
    gtk_text_buffer_get_end_iter (tbuf, &iter);
    if (mode == GBILLING_CHAT_MODE_SERVER)
    {
        buf = g_strdup_printf ("%.2i:%.2i  %s", st->tm_hour, st->tm_min, msg);
        gtk_text_buffer_insert (tbuf, &iter, buf, -1);
        g_free (buf);
    }
    else /* GBILLING_CHAT_MODE_CLIENT */
    {
        buf = g_strdup_printf ("%.2i:%.2i  ", st->tm_hour, st->tm_min);
        gtk_text_buffer_insert (tbuf, &iter, buf, -1);
        g_free (buf);
        gtk_text_buffer_insert_with_tags (tbuf, &iter, cdata->name, -1, tag, NULL);
        buf = g_strdup_printf (": %s", msg);
        gtk_text_buffer_insert (tbuf, &iter, buf, -1);
        g_free (buf);
    }

    gtk_text_buffer_insert (tbuf, &iter, "\n", -1);
    gtk_text_buffer_get_end_iter (tbuf, &iter);
    mark = gtk_text_buffer_create_mark (tbuf, NULL, &iter, FALSE);
    gtk_text_view_scroll_mark_onscreen ((GtkTextView *) cdata->wchat->textv_log, mark);
}

/*
 * Sebelum melanjutkan state client, kita perlu periksa apakah tarif atau 
 * paket tersebut masih valid dan waktu paket belum expire, jika tarif atau 
 * paket tersebut tidak valid atau expire, login tidak di lanjutkan.
 *
 * Current song: Alter Bridge - Life Must Go On.
 */
void
do_recovery (void)
{
    GList *ptr;
    ClientData *cdata;
    GbillingPrepaid *prepaid;
    gboolean valid = FALSE;

    gbilling_recov_read ();
    for (ptr = client_list; ptr; ptr = g_list_next (ptr))
    {
        cdata = ptr->data;
        if (cdata->status != GBILLING_STATUS_LOGIN)
            continue;

        if (cdata->type == GBILLING_LOGIN_TYPE_REGULAR)
        {
            cdata->cost = g_list_nth_data (cost_list, cdata->idtype);
            if (!cdata->cost)
            {
                gbilling_debug ("%s: recovery fail due missing cost nth data %i\n",
                                __func__, cdata->idtype);
                return;
            }
            valid = TRUE;
        }

        else if (cdata->type == GBILLING_LOGIN_TYPE_PREPAID)
        {
            prepaid = g_list_nth_data (prepaid_list, cdata->idtype);
            if (prepaid)
            {
                if (ABS (time (NULL) - cdata->tstart) < prepaid->duration)
                    valid = TRUE;
                else
                {
                    do_client_logout (cdata);
                    gbilling_debug ("%s: %s (%s) prepaid has been expire\n", __func__,
                                    cdata->name, cdata->ip);
                }
            }
            else
            {
                valid = FALSE;
                cdata->status = GBILLING_STATUS_NONE;
                gbilling_debug ("%s: %s (%s) recovery fail due missing "
                                "prepaid nth data %i\n", __func__, cdata->name,
                                cdata->ip, cdata->idtype);
            }
        }

        /*
         * Login administrasi tidak melakukan recovery, ini sebaiknya
         * di bypass pada rutin baca recovery.
         */
        else if (cdata->type == GBILLING_LOGIN_TYPE_ADMIN)
        {
            cdata->status = GBILLING_STATUS_NONE;
            g_free (cdata->username);
            g_free (cdata->desc);
        }

        if (valid)
            cdata->timer = g_timeout_add (1000, (GSourceFunc) 
                                          update_client_data0, cdata);
    }
}

