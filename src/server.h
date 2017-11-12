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
 *  server.h
 *  File ini adalah bagian dari gBilling (http://gbilling.sourceforge.net)
 *  Copyright (C) 2008 - 2010, Ardhan Madras <ajhwb@knac.com>
 */

#ifndef __SERVER_H__
#define __SERVER_H__

#include <gtk/gtk.h>
#include <glib.h>

#include "gbilling.h"

typedef struct
{
    GtkWidget *window;
    GtkWidget *textv_log;
    GtkWidget *textv_msg;
    GtkWidget *button_chat;
} WindowChat;

typedef struct
{
    gboolean play;
    gchar *sound;
} SoundOption;

struct _server 
{
    gushort tclient;        /* Total client */
    gushort tpaket;         /* Jumlah paket server */
    gushort titem;          /* Jumlah item server */
    gushort port;           /* Port server */
    gint socket;            /* Socket server */
    gboolean autoactive;    /* Otomatis aktifkan server */
    gboolean active;        /* State aktif/nonaktif */
    gboolean lockserver;    /* State buka password server */
    gboolean locklog;       /* State buka password log */
    gboolean display_name;  /* Tampilan nama */
    gboolean display_logo;  /* Tampilkan logo */
    gboolean display_desc;  /* Tampilkan deskripsi */
    gboolean display_addr;  /* Tampilkan alamat */
    gboolean autodisable;   /* Disable client setelah logout */
    gboolean autorestart;   /* Restart client setelah pemakaian */
    gboolean autodel;       /* Otomatis hapus log */
    gboolean mute_sound;    /* Mute all notifications sound */
    glong logshow;          /* Umur log yang ditampilkan */
    glong deltime;          /* Umut log yang otomatis dihapus */
    gchar *logo;            /* Logo warnet/server */
    GThread *athread;       /* Accept thread server */
    GbillingCost *cost;     /* Tarif default */
    GbillingAuth *sauth;    /* Autentikasi admin server */
    GbillingAuth *cauth;    /* Autentikasi admin client */
    GbillingAuth *lauth;    /* Autentikasi admin log */
};

typedef struct
{
    gint status;            /* Status client */
    gint num;               /* Detik timeout client */
    gushort type;           /* Tipe login client */
    gint idtype;            /* ID prepaid, group, recovery cost */
    gshort group;           /* Member group client */
    gboolean pingact;       /* Status ping client */
    gboolean active;        /* Status aktif client */
    gboolean access;        /* Akses login client */
    gulong voucher;         /* Voucher client */
    gulong ucost;           /* Tarif pemakaian client */
    gulong acost;           /* Tarif tambahan (menu) */
    gulong tcost;           /* Tarif total */
    gchar *name;            /* Nama komputer client */
    gchar *username;        /* Username user */
    gchar *ip;              /* IP address client */
    gchar *desc;            /* Keterangan client */
    gchar *debug;           /* Pesan debug client */
    time_t tstart;          /* Waktu mulai */
    glong duration;         /* Durasi client */
    time_t tend;            /* Waktu berhenti client */
    guint timer;            /* Source update timer */
    GThread *thread;        /* Thread update data client */
    GtkTreeIter iter;       /* Row iterator client */
    GtkTreeIter iterv;      /* Row iterator icon view */
    WindowChat *wchat;      /* Window chat client */
    GbillingCost *cost;     /* Tarif yang digunakan */
} ClientData;


extern struct _server *server;
extern GStaticMutex server_mutex;
extern GbillingServerZmd servcmd[];

extern GList *client_list;
extern GStaticMutex client_mutex;

extern gboolean use_win32;
extern GPid ping_pid;

extern GbillingCost *default_cost;
extern GbillingServerInfo *server_info;
extern GbillingCostList *cost_list;
extern GbillingCostRotationList *costrotation_list;
extern GbillingPrepaidList *prepaid_list;
extern GbillingItemList *item_list;
extern GbillingMemberGroupList *group_list;

extern const SoundOption default_soundopt[11];
extern SoundOption current_soundopt[11];

time_t get_first_time (time_t);
ClientData* get_client_by_name (const gchar*);
ClientData* get_client_by_ip (const gchar*);
GbillingCost* get_cost (const gchar*);
GbillingPrepaid* get_prepaid (const gchar*);
GbillingItem* get_item (const gchar*);
GbillingMemberGroup* get_group (const gchar*);
gint setup_server (void);
gpointer ping_client (gpointer);
gpointer check_status (gpointer);
gpointer accept_client (gpointer);
gboolean do_client_logout (ClientData*);
gint set_servcmd (ClientData*, GbillingCommand, const gchar*);
void start_timer (ClientData*);
gpointer clean_servcmd (gpointer);
gpointer update_status (gpointer);
gboolean update_cost_display (gpointer);
void fill_chat_log (const ClientData*, const gchar*, GbillingChatMode);
gboolean fill_debug_log (const gchar*);
void do_recovery (void);

#endif /* __SERVER_H__ */

