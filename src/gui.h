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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.c
 *
 *  gui.h
 *  File ini adalah bagian dari gBilling (http://gbilling.sourceforge.net)
 *  Copyright(c) 2008 - 2010, Ardhan Madras <ajhwb@knac.com>
 *
 */

#ifndef __GUI_H__
#define __GUI_H__

#include <gtk/gtk.h>

#include "server.h"

enum
{
    PAGE_CLIENT = 0,
    PAGE_STATUS,
    PAGE_LOG,
};

enum
{
    MENU_CLIENT = 0,
    MENU_LOG
};

typedef enum
{
    CLIENT_NAME = 0,
    CLIENT_PING,
    CLIENT_ACCESS,
    CLIENT_STATUS,
    CLIENT_USERNAME,
    CLIENT_TYPE,
    CLIENT_START,
    CLIENT_DURATION,
    CLIENT_END,
    CLIENT_COST,
    CLIENT_COLUMN
} ClientColumn;

typedef enum
{
    ICONV_NAME,
    ICONV_PIXBUF,
    ICONV_COLUMN
} IconvColumn;

typedef enum
{
    SLOG_DATE = 0,
    SLOG_CLIENT,
    SLOG_USERNAME,
    SLOG_TYPE,
    SLOG_START,
    SLOG_DURATION,
    SLOG_END,
    SLOG_COST,
    SLOG_COLUMN
} LogColumn;

typedef enum
{
    SCLIENT_NAME,
    SCLIENT_IP,
    SCLIENT_COLUMN
} SclientColumn;

typedef enum
{
    SCOST_NAME = 0,
    SCOST_MODE,
    SCOST_COST,
    SCOST_DEFAULT,
    SCOST_COLUMN
} ScostColumn;

enum
{
    SCOSTROTATION_NAME,
    SCOSTROTATION_INTERVAL,
    SCOSTROTATION_STATE,
    SCOSTROTATION_COLUMN
};

enum
{
    SCCOST_NAME = 0,
    SCCOST_MODE,
    SCCOST_COST,
    SCCOST_DEFAULT,
    SCCOST_COLUMN
};

typedef enum
{
    SCPAK_NAME = 0,
    SCPAK_DURATION,
    SCPAK_COST,
    SCPAK_ACTIVE,
    SCPAK_COLUMN
} ScpakColumn;

enum
{
    SCPREPAID_NAME = 0,
    SCPREPAID_DURATION,
    SCPREPAID_COST,
    SCPREPAID_ACTIVE,
    SCPREPAID_COLUMN
};

typedef enum
{
    SCITEM_NAME = 0,
    SCITEM_COST,
    SCITEM_ACTIVE,
    SCITEM_COLUMN
} ScitemColumn;

enum
{
    CMEMBER_GROUP_NAME = 0,
    CMEMBER_GROUP_COST,
    CMEMBER_GROUP_NMEMBER,
    CMEMBER_GROUP_COLUMN
};

enum
{
    CMEMBER_USERNAME = 0,
    CMEMBER_GROUP,
    CMEMBER_REGISTER,
    CMEMBER_STATUS,
    CMEMBER_COLUMN
};

typedef struct
{
    GtkWidget *window;
    GtkWidget *menu_server_activate;
    GtkWidget *menu_server_deactivate;
    GtkWidget *menu_server_quit;
    GtkWidget *menu_setting_server;
    GtkWidget *menu_setting_client;
    GtkWidget *menu_setting_cost;
    GtkWidget *menu_setting_prepaid;
    GtkWidget *menu_setting_item;
    GtkWidget *menu_setting_log;
    GtkWidget *menu_setting_mute;
    GtkWidget *menu_help_about;
    GtkWidget *toggleb_client;
    GtkWidget *toggleb_status;
    GtkWidget *toggleb_log;
    GtkWidget *image_logo;
    GtkWidget *label_name;
    GtkWidget *label_desc;
    GtkWidget *label_address;
    GtkWidget *notebook_client;
    GtkWidget *notebook_menu;
    GtkWidget *treeview_client;
    GtkWidget *label_client;
    GtkWidget *button_chat;
    GtkWidget *button_logout;
    GtkWidget *button_restart;
    GtkWidget *button_shutdown;
    GtkWidget *button_move;
    GtkWidget *button_edit;
    GtkWidget *button_enable;
    GtkWidget *button_disable;
    GtkWidget *iconview_status;
    GtkWidget *treeview_log;
    GtkWidget *button_filter;
    GtkWidget *button_detail;
    GtkWidget *button_delete;
    GtkWidget *button_total;
    GtkWidget *button_export;
    GtkWidget *button_refresh;
    GtkWidget *statusbar;
    GtkWidget *statusbar_cost;
    GtkListStore *store_client;
    GtkListStore *store_status;
    GtkListStore *store_log;
    GtkTreeSelection *client_selection;
    GtkTreeSelection *log_selection;
    gulong client_handlerid;
    gulong status_handlerid;
    gulong log_handlerid;
} WindowMain;

WindowMain *window_main;

typedef struct
{
    GtkWidget *window;
    GtkWidget *spinb_port;
    GtkWidget *spinb_recovery;
    GtkWidget *checkb_autoactive;
    GtkWidget *entry_susername;
    GtkWidget *label_spasswd;
    GtkWidget *entry_spasswd;
    GtkWidget *label_scpasswd;
    GtkWidget *entry_scpasswd;
    GtkWidget *button_ssave;
    GtkWidget *entry_cusername;
    GtkWidget *label_cpasswd;
    GtkWidget *entry_cpasswd;
    GtkWidget *label_ccpasswd;
    GtkWidget *entry_ccpasswd;
    GtkWidget *button_csave;
    GtkWidget *entry_name;
    GtkWidget *entry_desc;
    GtkWidget *entry_address;
    GtkWidget *checkb_name;
    GtkWidget *checkb_desc;
    GtkWidget *checkb_address;
    GtkWidget *checkb_logo;
    GtkWidget *label_logo;
    GtkWidget *entry_logo;
    GtkWidget *button_logo;
    GtkWidget *treeview_sound;
    GtkWidget *entry_sound;
    GtkWidget *button_browse;
    GtkWidget *button_preview;
    GtkWidget *button_ok;
} WindowSettingServer;

WindowSettingServer *window_setting_server;

typedef struct
{
    GtkWidget *window;
    GtkWidget *treeview;
    GtkWidget *button_add;
    GtkWidget *button_edit;
    GtkWidget *button_del;
    GtkWidget *checkb_disable;
    GtkWidget *checkb_restart;
    GtkWidget *button_ok;
    GtkListStore *store;
} WindowSettingClient;

WindowSettingClient *window_setting_client;

typedef struct
{
    GtkWidget *window;
    GtkWidget *entry_name;
    GtkWidget *entry_ip;
    GtkWidget *button_ok;
    GtkWidget *button_close;
} WindowUpdateClient;

WindowUpdateClient *window_update_client;

typedef struct
{
    GtkWidget *window;
    GtkWidget *treeview;
    GtkWidget *treeview_rotation;
    GtkWidget *button_add;
    GtkWidget *button_edit;
    GtkWidget *button_del;
    GtkWidget *button_default;
    GtkWidget *button_add_rotation;
    GtkWidget *button_edit_rotation;
    GtkWidget *button_del_rotation;
    GtkWidget *button_ok;
    GtkListStore *store;
} WindowSettingCost;

WindowSettingCost *window_setting_cost;

typedef struct
{
    GtkWidget *window;
    GtkWidget *radiob_mode1;
    GtkWidget *radiob_mode2;
    GtkWidget *entry_name;
    GtkWidget *spinb_fcost;
    GtkWidget *spinb_fmin;
    GtkWidget *spinb_imin;
    GtkWidget *spinb_icost;
    GtkWidget *label_sumr;
    GtkWidget *button_close;
    GtkWidget *button_ok;
} WindowUpdateCost;

WindowUpdateCost *window_update_cost;

typedef struct
{
    GtkWidget *window;
    GtkWidget *combob_cost;
    GtkWidget *checkb_active;
    GtkWidget *spinb_shour;
    GtkWidget *spinb_smin;
    GtkWidget *spinb_ehour;
    GtkWidget *spinb_emin;
    GtkWidget *button_close;
    GtkWidget *button_ok;
} WindowRotateCost;

WindowRotateCost *window_rotate_cost;

typedef struct
{
    GtkWidget *window;
    GtkWidget *treeview;
    GtkWidget *button_add;
    GtkWidget *button_edit;
    GtkWidget *button_del;
    GtkWidget *button_ok;
    GtkListStore *store;
} WindowSettingPrepaid;

WindowSettingPrepaid *window_setting_prepaid;

typedef struct
{
    GtkWidget *window;
    GtkWidget *entry_name;
    GtkWidget *spinb_dur;
    GtkWidget *spinb_cost;
    GtkWidget *checkb_status;
    GtkWidget *button_close;
    GtkWidget *button_ok;
} WindowUpdatePrepaid;

WindowUpdatePrepaid *window_update_prepaid;

typedef struct
{
    GtkWidget *window;
    GtkWidget *treeview;
    GtkWidget *button_add;
    GtkWidget *button_edit;
    GtkWidget *button_del;
    GtkWidget *button_ok;
    GtkListStore *store;
} WindowSettingItem;

WindowSettingItem *window_setting_item;

typedef struct
{
    GtkWidget *window;
    GtkWidget *entry_name;
    GtkWidget *spinb_price;
    GtkWidget *checkb_active;
    GtkWidget *button_close;
    GtkWidget *button_ok;
} WindowUpdateItem;

WindowUpdateItem *window_update_item;

typedef struct
{
    GtkWidget *window;
    GtkWidget *spinb_show;
    GtkWidget *combob_show;
    GtkWidget *checkb_del;
    GtkWidget *label_del;
    GtkWidget *spinb_del;
    GtkWidget *combob_del;
    GtkWidget *entry_name;
    GtkWidget *label_passwd;
    GtkWidget *entry_passwd;
    GtkWidget *label_passwdc;
    GtkWidget *entry_passwdc;
    GtkWidget *button_save;
    GtkWidget *button_ok;
} WindowSettingLog;

WindowSettingLog *window_setting_log;

typedef struct
{
    GtkWidget *window;
    GtkWidget *label_msg;
    GtkWidget *entry_username;
    GtkWidget *entry_passwd;
    GtkWidget *button_ok;
    GtkWidget *button_cancel;
} DialogAuth;

DialogAuth *dialog_auth;

typedef struct
{
    GtkWidget *window;
    GtkWidget *label_client;
    GtkWidget *label_use;
    GtkWidget *spinb_add;
    GtkWidget *entry_info;
    GtkWidget *label_total;
    GtkWidget *button_ok;
} WindowEdit;

WindowEdit *window_edit;

typedef struct
{
    GtkWidget *window;
    GtkWidget *label_move;
    GtkWidget *combob_client;
    GtkWidget *button_ok;
    GtkWidget *button_close;
} WindowMove;

WindowMove *window_move;

typedef struct
{
    GtkWidget *window;
    GtkWidget *checkb_client;
    GtkWidget *combob_client;
    GtkWidget *checkb_date_start;
    GtkWidget *toggleb_date_start;
    GtkWidget *checkb_date_end;
    GtkWidget *toggleb_date_end;
    GtkWidget *label_username;
    GtkWidget *checkb_username;
    GtkWidget *entry_username;
    GtkWidget *checkb_type;
    GtkWidget *combob_type;
    GtkWidget *checkb_time_start;
    GtkWidget *toggleb_time_start;
    GtkWidget *checkb_time_end;
    GtkWidget *toggleb_time_end;
    GtkWidget *checkb_cost_start;
    GtkWidget *spinb_cost_start;
    GtkWidget *checkb_cost_end;
    GtkWidget *spinb_cost_end;
    GtkWidget *button_close;
    GtkWidget *button_ok;
} WindowSearch;

WindowSearch *window_search;

typedef struct
{
    GtkWidget *window;
    GtkWidget *spinb_hour;
    GtkWidget *spinb_min;
    GtkWidget *spinb_sec;
    GtkWidget *button_ok;
} WindowTime;

WindowTime *window_time;

typedef struct
{
    GtkWidget *window;
    GtkWidget *calendar;
} WindowCalendar;

WindowCalendar *window_calendar;

typedef struct
{
    GtkWidget *window;
    GtkWidget *entry_client;
    GtkWidget *entry_username;
    GtkWidget *entry_type;
    GtkWidget *entry_start;
    GtkWidget *entry_end;
    GtkWidget *entry_duration;
    GtkWidget *entry_usage;
    GtkWidget *entry_add;
    GtkWidget *entry_desc;
    GtkWidget *entry_total;
    GtkWidget *button_ok;
} WindowDetail;

WindowDetail *window_detail;

typedef struct
{
    GtkWidget *window;
    GtkWidget *entry_first;
    GtkWidget *entry_last;
    GtkWidget *entry_usage;
    GtkWidget *entry_duration;
    GtkWidget *entry_cost;
    GtkWidget *button_ok;
} WindowTotal;

WindowTotal *window_total;

typedef struct
{
    GtkWidget *window;
    GtkWidget *img_dev;
    GtkWidget *img_gtk;
    GtkWidget *img_sqlite;
    GtkWidget *label_name;
    GtkWidget *label_desc;
    GtkWidget *label_build;
    GtkWidget *label_power;
    GtkWidget *button_copy;
    GtkWidget *button_ok;
    GtkWidget *linkb_site;
    GtkWidget *textv_dev;
    GtkWidget *textv_license;
    GtkWidget *textv_thanks;
} WindowAbout;

WindowAbout *window_about;

void windows_init (void);

/* window_init */
extern GtkWidget *window_init;
extern GtkWidget *label_window_init_stat;


gchar* get_data_file (const gchar *);
gchar* get_pixmap_file (const gchar*);
void insert_treev_client (void);
void insert_iconv_status (void);
void set_server_info (GtkWidget*);

void create_window_main (void);
void set_statusbar (const gchar*);
void create_window_setting_server (void);
void create_window_setting_client (void);

void create_window_update_client (GbillingUpdateMode);

void create_window_setting_cost (void);

void create_window_update_cost (GbillingUpdateMode);
void create_window_rotate_cost (GbillingUpdateMode);

void create_window_setting_prepaid (void);
void create_window_update_prepaid (GbillingUpdateMode);

void create_window_setting_item (void);
void create_window_update_item (GbillingUpdateMode);

void create_window_setting_log (void);

gint create_dialog_auth (GtkWindow*, const gchar*, GbillingAuth*);

void create_window_detail (void);
void create_window_total (void);
void create_window_move (void);
void create_window_search (void);
void create_window_edit (void);
void create_window_calendar (GtkWindow*);
void create_window_time (GtkWindow*);

WindowChat* create_window_chat (ClientData*);
void window_chat_destroy (WindowChat*);

#endif /* _GUI_H_ */

