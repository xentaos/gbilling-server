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
 *  sqldb.h
 *  File ini adalah bagian dari gBilling (http://gbilling.sourceforge.net)
 *  Copyright (C) 2008 - 2009, Ardhan Madras <ajhwb@knac.com>
 */

#ifndef __SQLDB_H__
#define __SQLDB_H__

#include "sqlite3.h"
#include "gbilling.h"
#include "server.h"

#define DB_FILE                "server.db"
#define DB_VERSION             "301"
#define DB_ID                  "id"
#define DB_NAME                "name"
#define DB_VALUE               "value"
#define DB_DESC                "description"
#define DB_ACTIVE              "active"
/* TABLE */
#define DBTABLE_INFO            "info"
#define DBTABLE_SET             "setting"
#define DBTABLE_CLIENT          "client"
#define DBTABLE_COST            "cost"
#define DBTABLE_ITEM            "item"
#define DBTABLE_PAKET           "paket"
#define DBTABLE_PREPAID         "paket"
#define DBTABLE_MEMBER_GROUP    "member_group"
#define DBTABLE_MEMBER          "member"
#define DBTABLE_LOG             "log"
#define DBTABLE_RECOV           "recovery"
#define DBTABLE_COSTROTATION    "cost_rotation"
/* DBTABLE_CLIENT */
#define DBCLIENT_ID             DB_ID
#define DBCLIENT_NAME           DB_NAME
#define DBCLIENT_IP             "ip"
#define DBCLIENT_STATE          "state"
/* DBTABLE_COST */
#define DBCOST_ID               "id"
#define DBCOST_NAME             "name"
#define DBCOST_MODE             "mode"
#define DBCOST_DEFAULT          "def"
#define DBCOST_FMIN             "fmin"
#define DBCOST_IMIN             "imin"
#define DBCOST_FCOST            "fcost"
#define DBCOST_ICOST            "icost"
/* DBTABLE_PAKET */
#define DBPAKET_DURATION        "duration"
#define DBPAKET_COST            "cost"
/* DBTABLE_PREPAID */
#define DBPREPAID_ID            DB_ID
#define DBPREPAID_NAME          DB_NAME
#define DBPREPAID_DURATION      "duration"
#define DBPREPAID_COST          "cost"
#define DBPREPAID_ACTIVE        "active"
/* DBTABLE_ITEM */
#define DBITEM_ID               DB_ID
#define DBITEM_NAME             DB_NAME
#define DBITEM_COST             "cost"
#define DBITEM_ACTIVE           "active"
/* DBTABLE_MEMBER_GROUP */
#define DBMEMBER_GROUP_ID       DB_ID
#define DBMEMBER_GROUP_NAME     DB_NAME
#define DBMEMBER_GROUP_COST     "cost"
/* DBTABLE_MEMBER */
#define DBMEMBER_ID             DB_ID
#define DBMEMBER_REGISTER       "register"
#define DBMEMBER_STATUS         "status"
#define DBMEMBER_USERNAME       "username"
#define DBMEMBER_PASSWD         "password"
#define DBMEMBER_GROUP          "member_group"
#define DBMEMBER_FULLNAME       "fullname"
#define DBMEMBER_ADDRESS        "address"
#define DBMEMBER_PHONE          "phone"
#define DBMEMBER_EMAIL          "email"
#define DBMEMBER_IDCARD         "idcard"
/* DBTABLE_LOG */
#define DBLOG_ID                DB_ID
#define DBLOG_TSTART            "tstart"
#define DBLOG_TEND              "tend"
#define DBLOG_CLIENT            "client"
#define	DBLOG_USERNAME          "username"
#define DBLOG_TYPE              "type"
#define DBLOG_VOUCHER           "voucher"
#define DBLOG_START             "start"
#define	DBLOG_DURATION          "duration"
#define	DBLOG_END               "end"
#define	DBLOG_UCOST             "ucost"
#define	DBLOG_ACOST             "acost"
#define	DBLOG_TCOST             "tcost"
#define DBLOG_DESC              "desc"
/* DBTABLE_RECOV */
#define DBRECOV_ID              DB_ID
#define DBRECOV_USERNAME        "username"
#define DBRECOV_TYPE            "type"
#define DBRECOV_PAKET           "paket"
#define DBRECOV_VOUCHER         "voucher"
#define DBRECOV_START           "start"
#define DBRECOV_ACOST           "acost"
#define DBRECOV_UCOST           "ucost"
#define DBRECOV_DESC            DB_DESC
/* DBTABLE_SET */
#define DBSET_NAME              DB_NAME
#define DBSET_VALUE             DB_VALUE
#define DBSET_VERSION           "version"
#define DBSET_SERVER_PORT       "server_port"
#define DBSET_SERVER_BACKUP     "server_backup"
#define DBSET_SERVER_AUTOSTART  "server_autostart"
#define DBSET_SERVER_NAME       "server_name"
#define DBSET_SERVER_DESC       "server_desc"
#define DBSET_SERVER_ADDR       "server_addr"
#define DBSET_SERVER_LOGO       "server_logo"
#define DBSET_SERVER_SUSER      "server_suser"
#define DBSET_SERVER_SPASSWD    "server_spasswd"
#define DBSET_SERVER_CUSER      "server_cuser"
#define DBSET_SERVER_CPASSWD    "server_cpasswd"
#define DBSET_LOG_SHOW          "log_show"
#define DBSET_LOG_DELTIME       "log_deltime"
#define DBSET_LOG_USER          "log_user"
#define DBSET_LOG_PASSWD        "log_passwd"
#define DBSET_DISPLAY_NAME      "display_name"
#define DBSET_DISPLAY_DESC      "display_desc"
#define DBSET_DISPLAY_ADDR      "display_addr"
#define DBSET_DISPLAY_LOGO      "display_logo"
/* DBTABLE_COST */
#define DBSET_COST_ID           "cost_id"
#define DBSET_COST_NAME         "cost_name"
#define DBSET_COST_MODE         "cost_mode"
#define DBSET_COST_FMIN         "cost_fmin"
#define DBSET_COST_IMIN         "cost_imin"
#define DBSET_COST_FCOST        "cost_fcost"
#define DBSET_COST_ICOST        "cost_icost"
/* DBTABLE_COSTROTATION */
#define DBCOSTROTATION_COST     "cost"
#define DBCOSTROTATION_SHOUR    "shour"
#define DBCOSTROTATION_SMIN     "smin"
#define DBCOSTROTATION_EHOUR    "ehour"
#define DBCOSTROTATION_EMIN     "emin"
#define DBCOSTROTATION_STATE    "state"


void gbilling_sql_exec0 (const gchar*, ...);

void gbilling_server_set_port (gushort);
gushort gbilling_server_get_port (void);
void gbilling_server_set_backup_time (guint);
guint gbilling_server_get_backup_time (void);
void gbilling_server_set_autostart (gboolean);
gboolean gbilling_server_get_autostart (void);

void gbilling_server_set_login_auth (GbillingAuth*);
void gbilling_server_set_server_login (GbillingAuth*);
GbillingAuth* gbilling_server_get_server_auth (void);
void gbilling_server_set_client_auth (GbillingAuth*);
GbillingAuth* gbilling_server_get_client_auth (void);

void gbilling_server_set_info (GbillingServerInfo*);
GbillingServerInfo* gbilling_server_get_info (void);
void gbilling_db_set_logo (const gchar*);
gchar* gbilling_db_get_logo (void);

gboolean gbilling_db_get_display_name (void);
void gbilling_db_set_display_name (gboolean);
gboolean gbilling_db_get_display_logo (void);
void gbilling_db_set_display_logo (gboolean);
gboolean gbilling_db_get_display_desc (void);
void gbilling_db_set_display_desc (gboolean);
gboolean gbilling_db_get_display_addr (void);
void gbilling_db_set_display_addr (gboolean);

gboolean gbilling_server_add_client (const GbillingClient*);
GbillingClient* gbilling_server_get_client_by_id (guint);
GbillingClient* gbilling_server_get_client_by_name (const gchar*);
GbillingClient* gbilling_server_get_client_by_ip (const gchar*);
gboolean gbilling_server_delete_client (const GbillingClient*);
gboolean gbilling_server_update_client (const GbillingClient*, const GbillingClient*);
GbillingClientList* gbilling_server_get_client_list (void);

void gbilling_server_set_log_show (glong);
glong gbilling_server_get_log_show (void);
void gbilling_server_set_log_deltime (glong);
glong gbilling_server_get_log_deltime (void);
void gbilling_server_set_log_auth (const GbillingAuth*);
GbillingAuth* gbilling_server_get_log_auth (void);

gint treev_log_sqlcb (gpointer, gint, gchar**, gchar**);
gint gbilling_log_show (void);
gint gbilling_log_filter_show (time_t, time_t, time_t, time_t, glong, glong, 
                                    const gchar*, const gchar*, const gchar*);
void gbilling_close_db (void);
gint gbilling_open_db (void);
gint gbilling_insert_data (void);
gint gbilling_insert_log (ClientData*, GbillingStatus);
GbillingLog* gbilling_get_log (const gchar*, time_t);
void gbilling_delete_log (const gchar*, time_t);
void gbilling_recov_read (void);
gboolean gbilling_recov_detect (void);
void gbilling_recov_write (const ClientData*);
void gbilling_recov_del (const ClientData*);

GbillingCost* gbilling_server_get_default_cost (void);
gboolean gbilling_server_set_default_cost (const gchar*);
GbillingCost* gbilling_server_get_cost_by_id (guint);
GbillingCost* gbilling_server_get_cost_by_name (const gchar*);
GbillingCostList* gbilling_server_get_cost_list (void);
gboolean gbilling_server_add_cost (const GbillingCost*);
gboolean gbilling_server_delete_cost (const gchar*);
gboolean gbilling_server_update_cost (const GbillingCost*, const gchar*);

GbillingCostRotationList* gbilling_server_get_cost_rotation (void);
void gbilling_server_add_cost_rotation (const GbillingCostRotation*);
void gbilling_server_update_cost_rotation (const GbillingCostRotation*, const GbillingCostRotation*);
void gbilling_server_delete_cost_rotation (const GbillingCostRotation*);

gboolean gbilling_server_add_prepaid (const GbillingPrepaid*);
gboolean gbilling_server_delete_prepaid (const gchar*);
gboolean gbilling_server_update_prepaid (const GbillingPrepaid*, const gchar*);
GbillingPrepaid* gbilling_server_get_prepaid_by_name (const gchar*);
GbillingPrepaid* gbilling_server_get_prepaid_by_id (guint);
GbillingPrepaidList* gbilling_server_get_prepaid_list (void);

gboolean gbilling_server_add_item (const GbillingItem*);
gboolean gbilling_server_delete_item (const gchar*);
gboolean gbilling_server_update_item (const GbillingItem*, const gchar*);
GbillingItem* gbilling_server_get_item_by_name (const gchar*);
GbillingItemList* gbilling_server_get_item_list (void);

gboolean gbilling_server_add_member_group (const GbillingMemberGroup*);
gboolean gbilling_server_update_member_group (const GbillingMemberGroup*, const gchar*);
gboolean gbilling_server_delete_member_group (const gchar*);
GbillingMemberGroup* gbilling_server_get_member_group_by_name (const gchar*);
GbillingMemberGroupList* gbilling_server_get_member_group_list (void);

gboolean gbilling_server_add_member (const GbillingMember*);
gboolean gbilling_server_update_member (const GbillingMember*, const GbillingMember*);
gboolean gbilling_server_delete_member (const GbillingMember*);
GbillingMember* gbilling_server_get_member (const gchar*, const gchar*);
gint gbilling_server_get_member_total (const gchar*);
GbillingMemberList* gbilling_server_get_member_list (const gchar*);

void gbilling_member_group_update (GbillingMemberGroup*, const gchar*, GbillingUpdateMode);
gint gbilling_create_member_table (void);
void gbilling_member_update (GbillingMember*, GbillingMember*, GbillingUpdateMode);
GbillingMember* gbilling_get_member (const gchar*, const gchar*);

#endif /* __SQLDB_H__ */

