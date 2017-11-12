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
 *  sqldb.c
 *  File ini adalah bagian dari gBilling (http://gbilling.sourceforge.net)
 *  Copyright (C) 2008 - 2010, Ardhan Madras <ajhwb@knac.com>
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <glib.h>

#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "sqlite3.h"
#include "gbilling.h"
#include "sqldb.h"
#include "gui.h"
#include "server.h"

static sqlite3 *db_file;

static void gbilling_sql_free_table (gchar**);

#define GBILLING_SQLITE3_EXIT_ERROR(func, parent) \
({ \
    gbilling_debug ("%s: %s\n", __func__, sqlite3_errmsg (db_file)); \
    exit (EXIT_FAILURE); \
})

/**
 * gbilling_sql_exec:
 * @cmd: Statement SQL.
 * @func: Fungsi callback query sql.
 *
 * Wrapper sqlite3_exec(), macro ini akan mengeluarkan program jika query sql gagal.
 */
#define gbilling_sql_exec(cmd, func) \
({ \
    gint __sqlret = sqlite3_exec (db_file, cmd, func, NULL, NULL); \
    if (__sqlret != SQLITE_OK) \
    { \
        g_critical ("%s: %s\n", __func__, sqlite3_errmsg (db_file)); \
        exit (EXIT_FAILURE); \
    } \
})

/**
 * gbilling_sql_get_table:
 * @cmd: Statement SQL.
 * @table: Table data yang akan diisikan.
 * @row: Jumlah row yang dihasilkan.
 * @col: Jumlah column yang dihasilkan.
 *
 * Wrapper sqlite3_get_table(), macro ini akan mengeluarkan program jika query sql gagal.
 */
#define gbilling_sql_get_table(cmd, table, row, col) \
({ \
    gint __sqlret = sqlite3_get_table (db_file, cmd, table, row, col, NULL); \
    if (__sqlret != SQLITE_OK) \
    { \
        g_critical ("%s: %s\n", __func__, sqlite3_errmsg (db_file)); \
        exit (EXIT_FAILURE); \
    } \
})

static void
gbilling_sql_free_table (gchar **table)
{
    if (table)
        sqlite3_free_table (table);
}

#define gbilling_sql_assert_row(row, table, ret) \
({ \
    if (row > 1) \
    { \
        gbilling_debug ("%s: Multiple row result, this should not be happen. " \
                        "Please fix your database.\n", __func__); \
        gbilling_sql_free_table (table); \
        return ret; \
    } \
})

/**
 * gbilling_server_set_port:
 *
 * @port: Listening port.
 * Set port server (default: 1705).
 */
void
gbilling_server_set_port (gushort port)
{
    gchar *cmd;
    cmd = sqlite3_mprintf ("UPDATE "DBTABLE_SET" SET "DBSET_VALUE" = %i "
                           "WHERE "DBSET_NAME" = '"DBSET_SERVER_PORT"'", port);
    gbilling_sql_exec (cmd, NULL);
    sqlite3_free (cmd);
}

/**
 * gbilling_server_get_port:
 *
 * Ambil port server.
 * Returns: Port server.
 */
gushort 
gbilling_server_get_port (void)
{
    gchar *cmd, **table;
    gint col;
    gushort port;
    
    cmd = sqlite3_mprintf ("SELECT "DBSET_VALUE" FROM "DBTABLE_SET" "
                           "WHERE "DBSET_NAME" = '"DBSET_SERVER_PORT"'");
    gbilling_sql_get_table (cmd, &table, NULL, &col);
    sqlite3_free (cmd);
    port = atoi (table[col]);
    gbilling_sql_free_table (table);

    return port;
}

/**
 * gbilling_server_set_backup_time:
 * @minute: Interval menit untuk melakukan backup (1 - 10 menit).
 *
 * Set interval backup data server dalam menit, nilai menit adalah dari 1 sampai 
 * 10, jika kurang atau lebih dari nilai ini, maka menit akan disesuaikan sesuai
 * batas di atas.
 */
void
gbilling_server_set_backup_time (guint minute)
{
    gchar *cmd;
    
    cmd = sqlite3_mprintf ("UPDATE "DBTABLE_SET" SET "DBSET_VALUE" = %u "
                           "WHERE "DBSET_NAME" = '"DBSET_SERVER_BACKUP"'", minute);
    gbilling_sql_exec (cmd, NULL);
    sqlite3_free (cmd);
}

/**
 * gbilling_server_get_backup_time:
 *
 * Ambil interval menit backup data server.
 * Returns: Menit backup.
 */
guint
gbilling_server_get_backup_time (void)
{
    gchar *cmd, **table;
    gint col;
    guint backup;
    
    cmd = sqlite3_mprintf ("SELECT "DBSET_VALUE" FROM "DBTABLE_SET" "
                           "WHERE "DBSET_NAME" = '"DBSET_SERVER_BACKUP"'");
    gbilling_sql_get_table (cmd, &table, NULL, &col);
    sqlite3_free (cmd);
    backup = atoi (table[col]);
    gbilling_sql_free_table (table);
    
    return backup;
}

/**
 * gbilling_server_set_autostart:
 * @start: %TRUE jika akan mengaktifkan server pada saat dijalankan.
 *
 * Aktifkan otomatis server ketika dijalankan.
 */
void
gbilling_server_set_autostart (gboolean start)
{
    gchar *cmd;
    
    cmd = sqlite3_mprintf ("UPDATE "DBTABLE_SET" SET "DBSET_VALUE" = %i WHERE "
                           ""DBSET_NAME" = '"DBSET_SERVER_AUTOSTART"'", start ? 1 : 0);
    gbilling_sql_exec (cmd, NULL);
    sqlite3_free (cmd);
}

/** 
 * gbilling_server_get_autostart:
 * 
 * Ambil status aktif otomatis server pada saat dijalankan.
 * Returns: Status aktif otomatis server.
 */
gboolean
gbilling_server_get_autostart (void)
{
    gchar *cmd, **table;
    gint col;
    gboolean start;
    
    cmd = sqlite3_mprintf ("SELECT "DBSET_VALUE" FROM "DBTABLE_SET" WHERE "
                           ""DBSET_NAME" = '"DBSET_SERVER_AUTOSTART"'");
    gbilling_sql_get_table (cmd, &table, NULL, &col);
    sqlite3_free (cmd);
    start = atoi (table[col]);
    gbilling_sql_free_table (table);
    
    return start;
}

/**
 * gbilling_server_set_server_login:
 * @auth: Data autentikasi.
 * 
 * Set data autentikasi server.
 */
void
gbilling_server_set_server_login (GbillingAuth *login)
{
    g_return_if_fail (login != NULL);
    gchar *cmd;
    
    cmd = sqlite3_mprintf ("UPDATE "DBTABLE_SET" SET "DBSET_VALUE" = '%q' "
                           "WHERE "DBSET_NAME" = '"DBSET_SERVER_SUSER"'", login->username);
    gbilling_sql_exec (cmd, NULL);
    sqlite3_free (cmd);
    cmd = sqlite3_mprintf ("UPDATE "DBTABLE_SET" SET "DBSET_VALUE" = '%q' "
                           "WHERE "DBSET_NAME" = '"DBSET_SERVER_SPASSWD"'", login->passwd);
    gbilling_sql_exec (cmd, NULL);
    sqlite3_free (cmd);
}

/**
 * gbilling_server_get_server_auth:
 *
 * Ambil data autentikasi server.
 * Returns: Data %GbillingAuth, dealokasi dengan gbilling_auth_free().
 */
GbillingAuth*
gbilling_server_get_server_auth (void)
{
    GbillingAuth *login;
    gchar *cmd, **table;
    gint col;

    cmd = sqlite3_mprintf ("SELECT "DBSET_VALUE" FROM "DBTABLE_SET" "
                           "WHERE "DBSET_NAME" = '"DBSET_SERVER_SUSER"'");
    gbilling_sql_get_table (cmd, &table, NULL, &col);
    sqlite3_free (cmd);
    login = g_new (GbillingAuth, 1);
    g_snprintf (login->username, sizeof(login->username), table[col]);
    gbilling_sql_free_table (table);
    
    cmd = sqlite3_mprintf ("SELECT "DBSET_VALUE" FROM "DBTABLE_SET" "
                           "WHERE "DBSET_NAME" = '"DBSET_SERVER_SPASSWD"'");
    gbilling_sql_get_table (cmd, &table, NULL, &col);
    sqlite3_free (cmd);
    g_snprintf (login->passwd, sizeof(login->passwd), table[col]);
    gbilling_sql_free_table (table);

    return login;
}

/**
 * gbilling_server_set_client_auth:
 * @auth: Data autentikasi.
 * 
 * Set data autentikasi administrasi client.
 */
void
gbilling_server_set_client_auth (GbillingAuth *auth)
{
    g_return_if_fail (auth != NULL);
    gchar *cmd;
    
    cmd = sqlite3_mprintf ("UPDATE "DBTABLE_SET" SET "DBSET_VALUE" = '%q' "
                           "WHERE "DBSET_NAME" = '"DBSET_SERVER_CUSER"'", auth->username);
    gbilling_sql_exec (cmd, NULL);
    sqlite3_free (cmd);
    cmd = sqlite3_mprintf ("UPDATE "DBTABLE_SET" SET "DBSET_VALUE" = '%q' "
                           "WHERE "DBSET_NAME" = '"DBSET_SERVER_CPASSWD"'", auth->passwd);
    gbilling_sql_exec (cmd, NULL);
    sqlite3_free (cmd);
}

/**
 * gbilling_client_get_server_auth:
 *
 * Ambil data autentikasi administrasi client.
 * Returns: Data %GbillingAuth, dealokasi dengan gbilling_auth_free().
 */
GbillingAuth*
gbilling_server_get_client_auth (void)
{
    GbillingAuth *login;
    gchar *cmd, **table;
    gint col;

    cmd = sqlite3_mprintf ("SELECT "DBSET_VALUE" FROM "DBTABLE_SET" "
                           "WHERE "DBSET_NAME" = '"DBSET_SERVER_CUSER"'");
    gbilling_sql_get_table (cmd, &table, NULL, &col);
    sqlite3_free (cmd);
    login = g_new (GbillingAuth, 1);
    g_snprintf (login->username, sizeof(login->username), table[col]);
    gbilling_sql_free_table (table);
    
    cmd = sqlite3_mprintf ("SELECT "DBSET_VALUE" FROM "DBTABLE_SET" "
                           "WHERE "DBSET_NAME" = '"DBSET_SERVER_CPASSWD"'");
    gbilling_sql_get_table (cmd, &table, NULL, &col);
    sqlite3_free (cmd);
    g_snprintf (login->passwd, sizeof(login->passwd), table[col]);
    gbilling_sql_free_table (table);

    return login;
}

/**
 * gbilling_server_get_log_auth:
 *
 * Ambil data autentikasi menajemen log.
 * Returns: Data %GbillingAuth, dealokasi dengan gbilling_auth_free().
 */
GbillingAuth*
gbilling_server_get_log_auth (void)
{
    GbillingAuth *login;
    gchar *cmd, **table;
    gint col;

    cmd = sqlite3_mprintf ("SELECT "DBSET_VALUE" FROM "DBTABLE_SET" "
                           "WHERE "DBSET_NAME" = '"DBSET_LOG_USER"'");
    gbilling_sql_get_table (cmd, &table, NULL, &col);
    sqlite3_free (cmd);
    login = g_new (GbillingAuth, 1);
    g_snprintf (login->username, sizeof(login->username), table[col]);
    gbilling_sql_free_table (table);
    
    cmd = sqlite3_mprintf ("SELECT "DBSET_VALUE" FROM "DBTABLE_SET" "
                           "WHERE "DBSET_NAME" = '"DBSET_LOG_PASSWD"'");
    gbilling_sql_get_table (cmd, &table, NULL, &col);
    sqlite3_free (cmd);
    g_snprintf (login->passwd, sizeof(login->passwd), table[col]);
    gbilling_sql_free_table (table);

    return login;
}

/**
 * gbilling_server_set_log_auth:
 * @auth: Data autentikasi.
 *
 * Set data autentikasi pengaturan manejemen log.
 */
void
gbilling_server_set_log_auth (const GbillingAuth *auth)
{
    g_return_if_fail (auth != NULL);
    gchar *cmd;
    
    cmd = sqlite3_mprintf ("UPDATE "DBTABLE_SET" SET "DBSET_VALUE" = '%q' "
                           "WHERE "DBSET_NAME" = '"DBSET_LOG_USER"'", auth->username);
    gbilling_sql_exec (cmd, NULL);
    sqlite3_free (cmd);
    cmd = sqlite3_mprintf ("UPDATE "DBTABLE_SET" SET "DBSET_VALUE" = '%q' "
                           "WHERE "DBSET_NAME" = '"DBSET_LOG_PASSWD"'", auth->passwd);
    gbilling_sql_exec (cmd, NULL);
    sqlite3_free (cmd);
}

/** 
 * gbilling_server_set_info:
 * @info: Data %GbillingServerInfo yang akan diset.
 *
 * Set informasi server.
 */
void
gbilling_server_set_info (GbillingServerInfo *info)
{
    gchar *cmd;

    g_return_if_fail (info != NULL);
    cmd = sqlite3_mprintf ("UPDATE "DBTABLE_SET" SET             "
                           ""DBSET_VALUE" = '%q' WHERE           "
                           ""DBSET_NAME"  = '"DBSET_SERVER_NAME"'",
                           info->name);
    gbilling_sql_exec (cmd, NULL);
    sqlite3_free (cmd);

    cmd = sqlite3_mprintf ("UPDATE "DBTABLE_SET" SET             "
                           ""DBSET_VALUE" = '%q' WHERE           "
                           ""DBSET_NAME"  = '"DBSET_SERVER_DESC"'",
                           info->desc);
    gbilling_sql_exec (cmd, NULL);
    sqlite3_free (cmd);

    cmd = sqlite3_mprintf ("UPDATE "DBTABLE_SET" SET             "
                           ""DBSET_VALUE" = '%q' WHERE           "
                           ""DBSET_NAME"  = '"DBSET_SERVER_ADDR"'",
                           info->addr);
    gbilling_sql_exec (cmd, NULL);
    sqlite3_free (cmd);
}

/**
 * gbilling_server_get_info:
 *
 * Returns: Data %GbillingServerInfo, dealokasi dengan gbilling_server_info_free().
 * Ambil informasi data server %GbillingServerInfo.
 */
GbillingServerInfo*
gbilling_server_get_info (void)
{
    GbillingServerInfo *info;
    gchar *cmd, **table;
    gchar name[MAX_SERVER_NAME], desc[MAX_SERVER_DESC], addr[MAX_SERVER_ADDR];
    gint col;

    info = gbilling_server_info_new ();

    cmd = sqlite3_mprintf ("SELECT "DBSET_VALUE" FROM "DBTABLE_SET" "
                           "WHERE "DBSET_NAME" = '"DBSET_SERVER_NAME"'");
    gbilling_sql_get_table (cmd, &table, NULL, &col);
    sqlite3_free (cmd);
    g_snprintf (name, sizeof (name), table[col]);
    gbilling_sql_free_table (table);

    cmd = sqlite3_mprintf ("SELECT "DBSET_VALUE" FROM "DBTABLE_SET" "
                           "WHERE "DBSET_NAME" = '"DBSET_SERVER_DESC"'");
    gbilling_sql_get_table (cmd, &table, NULL, &col);
    sqlite3_free (cmd);
    g_snprintf (desc, sizeof(desc), table[col]);
    gbilling_sql_free_table (table);

    cmd = sqlite3_mprintf ("SELECT "DBSET_VALUE" FROM "DBTABLE_SET" "
                           "WHERE "DBSET_NAME" = '"DBSET_SERVER_ADDR"'");
    gbilling_sql_get_table (cmd, &table, NULL, &col);
    sqlite3_free (cmd);
    g_snprintf (addr, sizeof(addr), table[col]);
    gbilling_sql_free_table (table);

    info = gbilling_server_info_new ();
    g_snprintf (info->name, sizeof(info->name), name);
    g_snprintf (info->desc, sizeof(info->desc), desc);
    g_snprintf (info->addr, sizeof(info->addr), addr);

    return info;
}

/** 
 * gbilling_server_set_logo:
 * @logo: Nama file logo.
 *
 * Set filename logo.
 */
void
gbilling_db_set_logo (const gchar *logo)
{
    g_return_if_fail (logo != NULL);

    gchar *cmd;

    cmd = sqlite3_mprintf ("UPDATE "DBTABLE_SET" SET "DBSET_VALUE" = '%q' "
                           "WHERE "DBSET_NAME" = '"DBSET_SERVER_LOGO"'", logo);
    gbilling_sql_exec (cmd, NULL);
    sqlite3_free (cmd);
}

/**
 * gbilling_server_get_logo:
 * 
 * Ambil nama file logo.
 *
 * Returns: Filename logo, atau #NULL jika gagal. Dealokasi dengan g_free().
 */
gchar*
gbilling_db_get_logo (void)
{
    gchar *cmd, **table, *logo = NULL;
    gint col;

    cmd = sqlite3_mprintf ("SELECT "DBSET_VALUE" FROM "DBTABLE_SET"   "
                           "WHERE "DBSET_NAME" = '"DBSET_SERVER_LOGO"'");
    gbilling_sql_get_table (cmd, &table, NULL, &col);
    if (col == 1)
        logo = g_strdup (table[col]);
    sqlite3_free (cmd);
    return logo;
}

/**
 * gbilling_db_get_display_name:
 * 
 * Returns: TRUE jika nama cafe ditampilkan, FALSE jika tidak.
 * Ambil status tampilan nama cafe.
 **/
gboolean
gbilling_db_get_display_name (void)
{
    gchar *cmd, **table;
    gint col;
    gboolean disp;

    cmd = sqlite3_mprintf ("SELECT "DBSET_VALUE" FROM "DBTABLE_SET" "
                           "WHERE "DBSET_NAME" = '"DBSET_DISPLAY_NAME"'");
    gbilling_sql_get_table (cmd, &table, NULL, &col);
    disp = atoi (table[col]);
    sqlite3_free (cmd);
    gbilling_sql_free_table (table);
    return disp;
}

/**
 * gbilling_db_set_display_name:
 * @display: TRUE jika nama cafe ditampilkan, FALSE jika tidak.
 * 
 * Set tampilan nama cafe.
 **/
void
gbilling_db_set_display_name (gboolean disp)
{
    gchar *cmd;

    cmd = sqlite3_mprintf ("UPDATE "DBTABLE_SET" SET "DBSET_VALUE" = %i "
                           "WHERE "DBSET_NAME" = '"DBSET_DISPLAY_NAME"'",
                           disp ? 1 : 0);
    gbilling_sql_exec (cmd, NULL);
    sqlite3_free (cmd);
}

/**
 * gbilling_db_get_display_logo:
 * 
 * Returns: TRUE jika logo ditampilkan, FALSE jika tidak.
 * Ambil status tampilan logo.
 */
gboolean
gbilling_db_get_display_logo (void)
{
    gchar *cmd, **table;
    gint col;
    gboolean disp;
    
    cmd = sqlite3_mprintf ("SELECT "DBSET_VALUE" FROM "DBTABLE_SET" "
                           "WHERE "DBSET_NAME" = '"DBSET_DISPLAY_LOGO"'");
    gbilling_sql_get_table (cmd, &table, NULL, &col);
    disp = atoi (table[col]);
    sqlite3_free (cmd);
    gbilling_sql_free_table (table);
    return disp;
}

/**
 * gbilling_db_set_display_logo:
 * @disp: TRUE jika logo ditampilkan, FALSE jika tidak.
 *
 * Set tampilan logo.
 */
void
gbilling_db_set_display_logo (gboolean disp)
{
    gchar *cmd;
    
    cmd = sqlite3_mprintf ("UPDATE "DBTABLE_SET" SET "DBSET_VALUE" = %i "
                           "WHERE "DBSET_NAME" = '"DBSET_DISPLAY_LOGO"'", 
                           disp ? 1 : 0);
    gbilling_sql_exec (cmd, NULL);
    sqlite3_free (cmd);
}

/**
 * gbilling_db_get_display_desc:
 * 
 * Returns: TRUE jika deksripsi cafe ditampilkan, FALSE jika tidak.
 * Ambil status tampilan deksripsi cafe.
 */
gboolean
gbilling_db_get_display_desc (void)
{
    gchar *cmd, **table;
    gint col;
    gboolean disp;
    
    cmd = sqlite3_mprintf ("SELECT "DBSET_VALUE" FROM "DBTABLE_SET" "
                           "WHERE "DBSET_NAME" = '"DBSET_DISPLAY_DESC"'");
    gbilling_sql_get_table (cmd, &table, NULL, &col);
    disp = atoi (table[col]);
    sqlite3_free (cmd);
    gbilling_sql_free_table (table);
    return disp;
}

/**
 * gbilling_db_set_display_desc:
 * @disp: TRUE jika deskripsi cafe ditampilkan, FALSE jika tidak.
 *
 * Set tampilan deskripsi cafe.
 */
void
gbilling_db_set_display_desc (gboolean disp)
{
    gchar *cmd;
    
    cmd = sqlite3_mprintf ("UPDATE "DBTABLE_SET" SET "DBSET_VALUE" = %i "
                           "WHERE "DBSET_NAME" = '"DBSET_DISPLAY_DESC"'", 
                           disp ? 1 : 0);
    gbilling_sql_exec (cmd, NULL);
    sqlite3_free (cmd);
}

/**
 * gbilling_db_get_display_addr:
 * 
 * Returns: TRUE jika alamat cafe ditampilkan, FALSE jika tidak.
 * Ambil status tampilan alamat cafe.
 */
gboolean
gbilling_db_get_display_addr (void)
{
    gchar *cmd, **table;
    gint col;
    gboolean disp;
    
    cmd = sqlite3_mprintf ("SELECT "DBSET_VALUE" FROM "DBTABLE_SET" "
                           "WHERE "DBSET_NAME" = '"DBSET_DISPLAY_ADDR"'");
    gbilling_sql_get_table (cmd, &table, NULL, &col);
    disp = atoi (table[col]);
    sqlite3_free (cmd);
    gbilling_sql_free_table (table);
    return disp;
}

/**
 * gbilling_db_set_display_addr:
 * @disp: TRUE jika alamat cafe ditampilkan, FALSE jika tidak.
 *
 * Set tampilan alamat cafe.
 */
void
gbilling_db_set_display_addr (gboolean disp)
{
    gchar *cmd;
    
    cmd = sqlite3_mprintf ("UPDATE "DBTABLE_SET" SET "DBSET_VALUE" = %i "
                           "WHERE "DBSET_NAME" = '"DBSET_DISPLAY_ADDR"'", 
                           disp ? 1 : 0);
    gbilling_sql_exec (cmd, NULL);
    sqlite3_free (cmd);
}

/**
 * gbilling_server_add_client:
 * @client:
 *
 * Returns:
 *
 */
gboolean
gbilling_server_add_client (const GbillingClient *client)
{
    g_return_val_if_fail (client != NULL, FALSE);
        
    gchar *cmd, **table;
    gint row;
    gboolean ret = FALSE;
    
    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_CLIENT" WHERE "DBCLIENT_NAME" "
                           "LIKE '%q' OR "DBCLIENT_IP" = '%q'", client->name, client->ip);
    gbilling_sql_get_table (cmd, &table, &row, NULL);
    sqlite3_free (cmd);
    gbilling_sql_assert_row (row, table, ret);
    if (row == 0)
    {
        cmd = sqlite3_mprintf ("INSERT INTO "DBTABLE_CLIENT" VALUES (NULL, '%q', "
                               "'%q', %i)", client->name, client->ip, 
                               client->state ? 1 : 0);
        gbilling_sql_exec (cmd, NULL);
        sqlite3_free (cmd);
        ret = TRUE;
    }
    gbilling_sql_free_table (table);

    return ret;
}

/**
 * gbilling_server_get_client_by_id:
 * @id:
 *
 * Returns:
 *
 */
GbillingClient*
gbilling_server_get_client_by_id (guint id)
{
    g_return_val_if_fail (id > 0, NULL);
    
    GbillingClient *client = NULL;
    gchar *cmd, **table;
    gint row, col, i;
    
    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_CLIENT" "
                           "WHERE "DBCLIENT_ID" = %u", id);
    gbilling_sql_get_table (cmd, &table, &row, &col);
    sqlite3_free (cmd);
    gbilling_sql_assert_row (row, table, client);
    if (row == 1)
    {
        i = col;
        client = gbilling_client_new ();
        client->id = atoi (table[i++]);
        client->name = g_strdup (table[i++]);
        client->ip = g_strdup (table[i++]);
        client->state = atoi (table[i]);
    }
    gbilling_sql_free_table (table);
    return client;
}

/**
 * gbilling_server_get_client_by_name:
 * @name:
 *
 * Returns:
 *
 */
GbillingClient*
gbilling_server_get_client_by_name (const gchar *name)
{
    g_return_val_if_fail (name != NULL, NULL);
        
    GbillingClient *client = NULL;
    gchar *cmd, **table;
    gint row, col, i;
    
    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_CLIENT" WHERE "
                           ""DBCLIENT_NAME" LIKE '%q'", name);
    gbilling_sql_get_table (cmd, &table, &row, &col);
    sqlite3_free (cmd);
    gbilling_sql_assert_row (row, table, client);
    if (row == 1)
    {
        i = col;
        client = gbilling_client_new ();
        client->id = atoi (table[i++]);
        client->name = g_strdup (table[i++]);
        client->ip = g_strdup (table[i++]);
        client->state = atoi (table[i]);
    }
    gbilling_sql_free_table (table);
    return client;
}     

/**
 * gbilling_server_get_client_by_ip:
 * @ip:
 *
 * Returns:
 *
 */
GbillingClient*
gbilling_server_get_client_by_ip (const gchar *ip)
{
    GbillingClient *client = NULL;
    gchar *cmd, **table;
    gint row, col, i;

    g_return_val_if_fail (ip != NULL, NULL);
    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_CLIENT" WHERE "
                           ""DBCLIENT_IP" = '%q'", ip);
    gbilling_sql_get_table (cmd, &table, &row, &col);
    sqlite3_free (cmd);
    gbilling_sql_assert_row (row, table, client);
    if (row == 1)
    {
        i = col;
        client = gbilling_client_new ();
        client->id = atoi (table[i++]);
        client->name = g_strdup (table[i++]);
        client->ip = g_strdup (table[i++]);
        client->state = atoi (table[i]);
    }
    gbilling_sql_free_table (table);
    return client;
}

/**
 * gbilling_server_delete_client:
 * @client:
 *
 * Returns:
 *
 */
gboolean
gbilling_server_delete_client (const GbillingClient *client)
{
    g_return_val_if_fail (client != NULL, FALSE);
    
    gchar *cmd, **table;
    gint row;
    gboolean ret = FALSE;

    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_CLIENT" WHERE "
                           ""DBCLIENT_NAME" LIKE '%q'", client->name);
    gbilling_sql_get_table (cmd, &table, &row, NULL);
    sqlite3_free (cmd);
    gbilling_sql_assert_row (row, table, ret);
    if (row == 1)
    {
        cmd = sqlite3_mprintf ("DELETE FROM "DBTABLE_CLIENT" WHERE "
                               ""DBCLIENT_NAME" = '%q'", client->name);
        gbilling_sql_exec (cmd, NULL);
        sqlite3_free (cmd);
        ret = TRUE;
    }
    gbilling_sql_free_table (table);
    return ret;
}

/**
 * gbilling_server_update_client:
 * @new: Data client yang baru.
 * @old: Data client yang lama.
 *
 * Returns: %TRUE jika update berhasil, %FALSE jika gagal.
 * Update data client.
 */
gboolean
gbilling_server_update_client (const GbillingClient *new,
                               const GbillingClient *old)
{
    g_return_val_if_fail (new != NULL && old != NULL, FALSE);

    gboolean test_name = FALSE, test_ip = FALSE;
    gboolean ret = FALSE;
    gchar *cmd, **table;
    gint row;

    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_CLIENT" WHERE "
                           ""DBCLIENT_NAME" LIKE '%q'", new->name);
    gbilling_sql_get_table (cmd, &table, &row, NULL);
    sqlite3_free (cmd);
    if (row == 1 && !g_ascii_strcasecmp (new->name, old->name))
        test_name = TRUE;
    if (row == 0)
        test_name = TRUE;
    gbilling_sql_free_table (table);
    
    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_CLIENT" WHERE "
                           ""DBCLIENT_IP" = '%q'", new->ip);
    gbilling_sql_get_table (cmd, &table, &row, NULL);
    sqlite3_free (cmd);
    if (row == 1 && !g_ascii_strcasecmp (new->ip, old->ip))
        test_ip = TRUE;
    if (row == 0)
        test_ip = TRUE;
    gbilling_sql_free_table (table);
    
    if (test_name && test_ip)
    {
        cmd = sqlite3_mprintf ("UPDATE "DBTABLE_CLIENT" SET "
                               ""DBCLIENT_NAME"  = '%q',    "
                               ""DBCLIENT_IP"    = '%q',    "
                               ""DBCLIENT_STATE" = %i       "
                               "WHERE "DBCLIENT_NAME" = '%q'",
                               new->name, new->ip, 
                               new->state ? 1 : 0, old->name);
        gbilling_sql_exec (cmd, NULL);
        sqlite3_free (cmd);
        ret = TRUE;
    }
    return ret;
}

GbillingClientList*
gbilling_server_get_client_list (void)
{
    GbillingClientList *list = NULL;
    gchar *cmd, **table;
    gint row, col, i, j;
    
    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_CLIENT"");
    gbilling_sql_get_table (cmd, &table, &row, &col);
    sqlite3_free (cmd);
    if (row > 0)
    {
        for (i = 0, j = col; i < row; i++)
        {
            GbillingClient *client = gbilling_client_new ();
            client->id = atoi (table[j++]);
            client->name = g_strdup (table[j++]);
            client->ip = g_strdup (table[j++]);
            client->state = atoi (table[j++]);
            list = g_list_prepend (list, client);
         }
         list = g_list_reverse (list);
    }
    gbilling_sql_free_table (table);
    return list;
}

/**
 * gbilling_server_set_log_show:
 * @show: Waktu dalam detik.
 *
 * Set umur log yang akan ditampilkan pada saat server dijalankan.
 */
void
gbilling_server_set_log_show (glong show)
{
    gchar *cmd;

    cmd = sqlite3_mprintf ("UPDATE "DBTABLE_SET" SET "DBSET_VALUE" = %li "
                           "WHERE "DBSET_NAME" = '"DBSET_LOG_SHOW"'", show);
    gbilling_sql_exec (cmd, NULL);
    sqlite3_free (cmd);
}

/**
 * gbilling_server_get_log_show:
 *
 * Returns: Waktu dalam detik.
 * Ambil waktu log yang akan ditampilkan pada saat server dijalankan.
 */
glong
gbilling_server_get_log_show (void)
{
    gchar *cmd, **table;
    gint col;
    glong show;
    
    cmd = sqlite3_mprintf ("SELECT "DBSET_VALUE" FROM "DBTABLE_SET" "
                           "WHERE "DBSET_NAME" = '"DBSET_LOG_SHOW"'");
    gbilling_sql_get_table (cmd, &table, NULL, &col);
    sqlite3_free (cmd);
    show = atol (table[col]);
    gbilling_sql_free_table (table);
    return show;
}

/**
 * gbilling_server_set_log_deltime:
 * @deltime: Waktu dalam detik untuk menghapus log secara otomatis.
 *
 * Set untuk menghapus data log secara otomatis sesuai waktu yang diberikan, 
 * data, jika @deltime = %0, maka log tidak akan dihapus secara otomatis.
 */
void
gbilling_server_set_log_deltime (glong deltime)
{
    gchar *cmd;
    
    cmd = sqlite3_mprintf ("UPDATE "DBTABLE_SET" SET "DBSET_VALUE" = %li "
                           "WHERE "DBSET_NAME" = '"DBSET_LOG_DELTIME"'", deltime);
    gbilling_sql_exec (cmd, NULL);
    sqlite3_free (cmd);
}

/**
 * gbilling_server_get_log_deltime:
 *
 * Set waktu untuk menghapus otomatis data log, set %0 untuk tidak menghapus otomatis.
 */
glong
gbilling_server_get_log_deltime (void)
{
    gchar *cmd, **table;
    gint col;
    glong deltime;
    
    cmd = sqlite3_mprintf ("SELECT "DBSET_VALUE" FROM "DBTABLE_SET" "
                           "WHERE "DBSET_NAME" = '"DBSET_LOG_DELTIME"'");
    gbilling_sql_get_table (cmd, &table, NULL, &col);
    sqlite3_free (cmd);
    deltime = atol (table[col]);
    gbilling_sql_free_table (table);
    return deltime;
}

/**
 * gbilling_server_get_default_cost:
 *
 * Returns: Data #GbillingCost yang baru, atau NULL jika tarif default belum ditentukan. 
 * Dealokasi dengan gbilling_cost_free().
 * Ambil tarif default.
 */
GbillingCost*
gbilling_server_get_default_cost (void)
{
    GbillingCost *cost = NULL;
    gchar *cmd, **table;
    const gchar identifier[] = "cost_%";
    gint col, i;

    cmd = sqlite3_mprintf ("SELECT "DBSET_VALUE" FROM "DBTABLE_SET" "
                           "WHERE "DBSET_NAME" LIKE '%q'", identifier);
    gbilling_sql_get_table (cmd, &table, NULL, &col);
    sqlite3_free (cmd);
    if (atoi (table[col]))
    {
        i = col;
        cost = gbilling_cost_new ();
        cost->id = atoi (table[i++]);
        g_snprintf (cost->name, sizeof(cost->name), table[i++]);
        cost->mode = atoi (table[i++]);
        cost->fmin = atoi (table[i++]);
        cost->imin = atoi (table[i++]);
        cost->fcost = atol (table[i++]);
        cost->icost = atol (table[i]);
    }
    gbilling_sql_free_table (table);
    return cost;
}

/**
 * gbilling_server_set_default_cost:
 * @cost: Tarif yang akan diset.
 * 
 * Set tarif default.
 */
gboolean
gbilling_server_set_default_cost (const gchar *name)
{
    g_return_val_if_fail (name != NULL, FALSE);

    gchar **table, *cmd;
    gint row;
    gboolean ret = FALSE;

    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_COST" WHERE "
                           ""DBCOST_NAME" = '%q'", name);
    gbilling_sql_get_table (cmd, &table, &row, NULL);
    sqlite3_free (cmd);
    gbilling_sql_assert_row (row, table, ret);
    if (row == 1)
    {
        cmd = sqlite3_mprintf ("UPDATE "DBTABLE_COST" SET "DBCOST_DEFAULT" = 0");
        gbilling_sql_exec (cmd, NULL);
        sqlite3_free (cmd);
        cmd = sqlite3_mprintf ("UPDATE "DBTABLE_COST" SET  "
                               ""DBCOST_DEFAULT" = 1 WHERE "
                               ""DBCOST_NAME" = '%q'", name);
        gbilling_sql_exec (cmd, NULL);
        sqlite3_free (cmd);
        ret ^= 1;
    }
    gbilling_sql_free_table (table);
    return ret;
}

/**
 * gbilling_server_get_cost_by_id:
 * @id:
 * 
 * Returns:
 *
 */
GbillingCost*
gbilling_server_get_cost_by_id (guint id)
{
    g_return_val_if_fail (id > 0, NULL);

    GbillingCost *cost = NULL;
    gchar *cmd, **table;
    gint row, col, i;

    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_COST" "
                           "WHERE "DBCOST_ID" = %u", id);
    gbilling_sql_get_table (cmd, &table, &row, &col);
    sqlite3_free (cmd);
    gbilling_sql_assert_row (row, table, cost);

    if (row == 1)
    {
        i = col;
        cost = gbilling_cost_new ();
        cost->id = atoi (table[i++]);
        g_snprintf (cost->name, sizeof(cost->name), table[i++]);
        cost->mode = atoi (table[i++]);
        cost->fmin = atoi (table[i++]);
        cost->imin = atoi (table[i++]);
        cost->fcost = atol (table[i++]);
        cost->icost = atol (table[i]);
    }
    gbilling_sql_free_table (table);
    return cost;
}

/**
 * gbilling_server_get_cost_by_name:
 * @name: Nama tarif yang akan dicari.
 *
 * Cari tarif berdasarkan nama tarif.
 *
 * Returns: #GbillingCost yang baru, dealokasi dengan gbilling_cost_free().
 * Atau %NULL jika tarif tidak ditemukan.
 **/
GbillingCost*
gbilling_server_get_cost_by_name (const gchar *name)
{
    g_return_val_if_fail (name != NULL, NULL);

    GbillingCost *cost = NULL;
    gchar *cmd, **table;
    gint row, col, i;

    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_COST" WHERE "
                           ""DBCOST_NAME" LIKE '%q'", name);
    gbilling_sql_get_table (cmd, &table, &row, &col);
    sqlite3_free (cmd);

    if (row == 1)
    {
        i = col;
        cost = gbilling_cost_new ();
        cost->id = atoi (table[i++]);
        g_snprintf (cost->name, sizeof(cost->name), table[i++]);
        cost->mode = atoi (table[i++]);
        cost->def = atoi (table[i++]);
        cost->fmin = atoi (table[i++]);
        cost->imin = atoi (table[i++]);
        cost->fcost = atol (table[i++]);
        cost->icost = atol (table[i]);
    }
    gbilling_sql_free_table (table);
    return cost;
}

/**
 * gbilling_server_get_cost_list:
 *
 * Returns: Double linked-list #GbillingCostList, dealokasi dengan 
 * gbilling_cost_list_free().
 * Ambil semua tarif masukkan kedalam #GbillingCostList.
 */
GbillingCostList*
gbilling_server_get_cost_list (void)
{
    GbillingCostList *list = NULL;
    gchar *cmd, **table;
    gint row, col, i, j;

    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_COST"");
    gbilling_sql_get_table (cmd, &table, &row, &col);
    sqlite3_free (cmd);
    if (row > 0)
    {
        for (i = 0, j = col; i < row; i++)
        {
            GbillingCost *cost;
            cost = gbilling_cost_new ();
            cost->id = atoi (table[j++]);
            g_snprintf (cost->name, sizeof(cost->name), table[j++]);
            cost->mode = atoi (table[j++]);
            cost->def = atoi (table[j++]);
            cost->fmin = atoi (table[j++]);
            cost->imin = atoi (table[j++]);
            cost->fcost = atol (table[j++]);
            cost->icost = atol (table[j++]);
            list = g_list_prepend (list, cost);
        }
        list = g_list_reverse (list);
    }
    gbilling_sql_free_table (table);
    return list;
}

gboolean
gbilling_server_add_cost (const GbillingCost *cost)
{
    g_return_val_if_fail (cost != NULL, FALSE);
    
    gchar *cmd, **table;
    gint row;
    gboolean ret = FALSE;

    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_COST" WHERE "
                           ""DBCOST_NAME" LIKE '%q'", cost->name);
    gbilling_sql_get_table (cmd, &table, &row, NULL);
    sqlite3_free (cmd);
    gbilling_sql_assert_row (row, table, ret);
    if (row == 0)
    {
        cmd = sqlite3_mprintf ("INSERT INTO "DBTABLE_COST" VALUES   "
                               "(NULL, '%q', %i, %i, %i, %i, %i, %i)", 
                               cost->name, cost->mode, cost->def, cost->fmin, 
                               cost->imin, cost->fcost, cost->icost);
        gbilling_sql_exec (cmd, NULL);
        sqlite3_free (cmd);
        ret = TRUE;
    }
    gbilling_sql_free_table (table);
    return ret;
}

/**
 * gbilling_server_delete_cost:
 * @name:
 *
 * Returns:
 *
 */
gboolean
gbilling_server_delete_cost (const gchar *name)
{
    g_return_val_if_fail (name != NULL, FALSE);

    gchar *cmd, **table;
    gint row;
    gboolean ret = FALSE;

    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_COST" WHERE "
                           ""DBCOST_NAME" LIKE '%q'", name);
    gbilling_sql_get_table (cmd, &table, &row, NULL);
    sqlite3_free (cmd);
    gbilling_sql_assert_row (row, table, ret);
    if (row == 1)
    {
        cmd = sqlite3_mprintf ("DELETE FROM "DBTABLE_COST" WHERE "
                               ""DBCOST_NAME" = '%q'", name);
        gbilling_sql_exec (cmd, NULL);
        sqlite3_free (cmd);
        ret = TRUE;
    }
    gbilling_sql_free_table (table);
    return ret;
}

gboolean
gbilling_server_update_cost (const GbillingCost *new,
                             const gchar        *old_name) /* why not use only a old name? */
{
    g_return_val_if_fail (new != NULL && old_name != NULL, FALSE);
    
    gchar *cmd, **table;
    gint row;
    gboolean ret = FALSE, test_name = FALSE;

    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_COST" WHERE "
                           ""DBCOST_NAME" LIKE '%q'", new->name);
    gbilling_sql_get_table (cmd, &table, &row, NULL);
    sqlite3_free (cmd);
    gbilling_sql_assert_row (row, table, ret);

    if (row == 1 && !g_ascii_strcasecmp (new->name, old_name))
        test_name = TRUE;
    if (row == 0)
        test_name = TRUE;
    if (test_name)
    {
        cmd = sqlite3_mprintf ("UPDATE "DBTABLE_COST" SET   "
                               ""DBCOST_NAME"    = '%q',    "
                               ""DBCOST_MODE"    = %i,      "
                               ""DBCOST_DEFAULT" = %i,      "
                               ""DBCOST_FMIN"    = %i,      "
                               ""DBCOST_IMIN"    = %i,      "
                               ""DBCOST_FCOST"   = %i,      "
                               ""DBCOST_ICOST"   = %i WHERE "
                               ""DBCOST_NAME"    = '%q'",                    
                               new->name, new->mode, new->def,
                               new->fmin, new->imin, new->fcost, 
                               new->icost, old_name);
        gbilling_sql_exec (cmd, NULL);
        sqlite3_free (cmd);
        ret = TRUE;
    }
    gbilling_sql_free_table (table);
    return ret;
}

GbillingCostRotationList*
gbilling_server_get_cost_rotation (void)
{
    GbillingCostRotationList *list = NULL;
    GbillingCostRotation *data;
    gchar *cmd, **table;
    gint i, j, row, col;

    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_COSTROTATION"");
    gbilling_sql_get_table (cmd, &table, &row, &col);
    sqlite3_free (cmd);

    for (i = 0, j = col; i < row && row > 0; i++)
    {
        data = g_new (GbillingCostRotation, 1);
        g_snprintf (data->cost, sizeof data->cost, table[j++]);
        data->shour = atoi (table[j++]);
        data->smin = atoi (table[j++]);
        data->ehour = atoi (table[j++]);
        data->emin = atoi (table[j++]);
        data->state = atoi (table[j++]);
        list = g_list_prepend (list, data);
    }

    list = g_list_reverse (list);
    gbilling_sql_free_table (table);
    return list;
}

void
gbilling_server_add_cost_rotation (const GbillingCostRotation *rotation)
{
    g_return_if_fail (rotation != NULL);

    gchar *cmd;

    cmd = sqlite3_mprintf ("INSERT INTO "DBTABLE_COSTROTATION" "
                           "VALUES ('%q', %i, %i, %i, %i, %i)",
                           rotation->cost, rotation->shour, rotation->smin,
                           rotation->ehour, rotation->emin, rotation->state);
    gbilling_sql_exec (cmd, NULL);
    sqlite3_free (cmd);
}

void
gbilling_server_update_cost_rotation (const GbillingCostRotation *old,
                                      const GbillingCostRotation *new)
{
    g_return_if_fail (old != NULL && new != NULL);

    gchar *cmd;

    cmd = sqlite3_mprintf ("UPDATE "DBTABLE_COSTROTATION" SET "
                           ""DBCOSTROTATION_SHOUR" = %i,      "
                           ""DBCOSTROTATION_SMIN"  = %i,      "
                           ""DBCOSTROTATION_EHOUR" = %i,      "
                           ""DBCOSTROTATION_EMIN"  = %i,      "
                           ""DBCOSTROTATION_STATE" = %i WHERE "
                           ""DBCOSTROTATION_COST"  = '%q' AND "
                           ""DBCOSTROTATION_SHOUR" = %i AND   "
                           ""DBCOSTROTATION_SMIN"  = %i AND   "
                           ""DBCOSTROTATION_EHOUR" = %i AND   "
                           ""DBCOSTROTATION_EMIN"  = %i AND   "
                           ""DBCOSTROTATION_STATE" = %i",
                           new->shour, new->smin, new->ehour, 
                           new->emin, new->state, old->cost, 
                           old->shour, old->smin, old->ehour, 
                           old->emin, old->state);

    gbilling_sql_exec (cmd, NULL);
    sqlite3_free (cmd);
}

void
gbilling_server_delete_cost_rotation (const GbillingCostRotation *rotation)
{
    g_return_if_fail (rotation != NULL);

    gchar *cmd;
    
    cmd = sqlite3_mprintf ("DELETE FROM "DBTABLE_COSTROTATION" WHERE ("
                           ""DBCOSTROTATION_COST" ='%q' AND "
                           ""DBCOSTROTATION_SHOUR" = %i AND "
                           ""DBCOSTROTATION_SMIN"  = %i AND "
                           ""DBCOSTROTATION_EHOUR" = %i AND "
                           ""DBCOSTROTATION_EMIN"  = %i AND "
                           ""DBCOSTROTATION_STATE" = %i)",
                           rotation->cost, rotation->shour, rotation->smin,
                           rotation->ehour, rotation->emin, rotation->state);

    gbilling_sql_exec (cmd, NULL);
    sqlite3_free (cmd);
}

/**
 * treev_log_sqlcb:
 *
 * Callback sqlite3_exec(), set @treev_log, skema DBTABLE_LOG:
 * id|tstart|tend|client|username|type|voucher|start|duration|end|ucost|acost|tcost|description
 *  0   1      2     3      4       5     6      7       8     9    10    11  12     13
 * Returns: Selalu 0
 */
gint
treev_log_sqlcb (gpointer   data,
                 gint       argc,
                 gchar    **argv,
                 gchar    **col)
{
    GtkTreeIter iter;
    GtkListStore *store;
    gchar *date, *start, *dur, *end, *cost;

    date = time_t_to_date (atol (argv[1]));
    start = time_t_to_string (atol (argv[1]));
    dur = argv[8] ? (atol (argv[8]) ? time_to_string (atol (argv[8])) : g_strdup ("-")) : g_strdup ("-");
    end = argv[2] ? (atol (argv[2]) ? time_t_to_string (atol (argv[2])) : g_strdup ("-")) : g_strdup ("-");
    cost = cost_to_string (atol (argv[12]));

    store = (GtkListStore *) gtk_tree_view_get_model ((GtkTreeView *) window_main->treeview_log);
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
                        SLOG_DATE, date,
                        SLOG_CLIENT, argv[3],
                        SLOG_USERNAME, argv[4],
                        SLOG_TYPE, argv[5],
                        SLOG_START, start,
                        SLOG_DURATION, dur,
                        SLOG_END, end,
                        SLOG_COST, cost,
                        -1);
    g_free (date);
    g_free (start);
    g_free (dur);
    g_free (end);
    g_free (cost);

    return 0;
}

/**
 * client_data_sqlcb:
 * Returns: Selalu 0.
 *
 * Callback query gbilling_recov_read().
 */
gint
client_data_sqlcb (gpointer   data,
                   gint       argc,
                   gchar    **argv,
                   gchar    **col)
{
    ClientData *cdata;

    gint i = atoi (argv[0]);
    cdata = g_list_nth_data (client_list, i);
    g_return_val_if_fail (cdata != NULL, 0);

    cdata->status   = GBILLING_STATUS_LOGIN;
    cdata->username = g_strdup (argv[1]);
    cdata->type     = atoi (argv[2]);
    cdata->idtype   = atoi (argv[3]);
    cdata->voucher  = atol (argv[4]);
    cdata->tstart   = atol (argv[5]);
    cdata->ucost    = atol (argv[6]);
    cdata->acost    = atol (argv[7]);
    cdata->desc     = g_strdup (argv[8]);

    return 0;
}

/**
 * tampilkan log, waktu yg dipilih dari setting
 * log #DBLOG_SHOWTIME
 */
gint
gbilling_log_show (void)
{
    gchar **res, *cmd;
    glong val;

    /* TODO: recode to directly read from server data */
    cmd = sqlite3_mprintf ("SELECT "DB_VALUE" FROM "DBTABLE_SET" WHERE "
                           ""DB_NAME" = '"DBSET_LOG_SHOW"'");
    gbilling_sql_get_table (cmd, &res, NULL, NULL);
    sqlite3_free (cmd);
    val = res[1] ? g_strtod (res[1], NULL) : 0L;
    gbilling_sql_free_table (res);
    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_LOG" WHERE            "
                           ""DBLOG_TSTART" >= %li ORDER BY "DBLOG_TSTART"",
                           time (NULL) - val);
    gbilling_sql_exec (cmd, treev_log_sqlcb);
    sqlite3_free (cmd);
    return 0;
}

/**
 * gbilling_log_filter_show:
 * @start_date: Tanggal mulai.
 * @end_date: Tanggal selesai.
 * @start_time: Waktu mulai.
 * @end_time: Waktu selesai.
 * @min_cost: Tarif minimal.
 * @max_cost: Tarif maksimal.
 * @client: Name client.
 * @username: Username pemakai.
 * @type: Tipe login.
 *
 * Returns: Selalu 0.
 *
 * Filter log dan tampilkan filter log.
**/
gint
gbilling_log_filter_show (time_t       start_date,
                          time_t       end_date,
                          time_t       start_time,
                          time_t       end_time,
                          glong        min_cost,
                          glong        max_cost,
                          const gchar *client,
                          const gchar *username,
                          const gchar *type)
{
    gchar *cmd, *tclient, *tusername, *ttype;

    tclient = client ? g_strdup (client) : g_strdup ("%");
    tusername = username ? g_strdup (username) : g_strdup ("%");
    ttype = type ? g_strdup (type) : g_strdup ("%");

    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_LOG" WHERE "
                           ""DBLOG_TSTART" >= %li AND         "
                           ""DBLOG_TSTART" <= %li AND         "
                           ""DBLOG_CLIENT" LIKE '%q' AND      "
                           ""DBLOG_TYPE" LIKE '%q' AND        "
                           ""DBLOG_USERNAME" LIKE '%q' AND    "
                           ""DBLOG_START" >= %li AND          "
                           ""DBLOG_END" >= %li AND            "
                           ""DBLOG_TCOST" >= %li AND          "
                           ""DBLOG_TCOST" <= %li              "
                           "ORDER BY "DB_ID"                  ", 
                           start_date, end_date, tclient, ttype, 
                           g_strstrip (tusername), start_time, end_time, 
                           min_cost, max_cost);

    gbilling_sql_exec (cmd, treev_log_sqlcb);
    sqlite3_free (cmd);
    g_free (tclient);
    g_free (tusername);
    g_free (ttype);
    return 0;
}

void
gbilling_close_db (void)
{
    sqlite3_close (db_file);
}

/**
 * gbilling_check_db: 
 * Returns: -1 jika gagal, 0 jika versi sama atau lebih besar 0 jika berbeda.
 *
 * Periksa versi database.
 */
static gint
gbilling_check_db (void)
{
    gchar **table, *cmd;
    gint ret, row, col;

    cmd = sqlite3_mprintf ("SELECT "DBSET_VALUE" FROM "DBTABLE_SET" "
                           "WHERE "DBSET_NAME" = '"DBSET_VERSION"'");

    gbilling_sql_get_table (cmd, &table, &row, &col);
    sqlite3_free (cmd);
    if (!row)
        return -1;

    if (!table[col])
    {
        gbilling_sql_free_table (table);
        return 1;
    }

    ret = strcmp (table[col], DB_VERSION);
    gbilling_sql_free_table (table);
    return ret;
}

/**
 * gbilling_open_db:
 *
 * Buat dan inisialisasi database.
 */
gboolean
gbilling_open_db (void)
{
    gint retval;
    gchar *db;
    gboolean test = FALSE;

    db = g_build_filename (g_get_home_dir (), ".gbilling", DB_FILE, NULL);
    test = g_file_test (db, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR);
    retval = sqlite3_open (db, &db_file);
    g_free (db);
    if (retval != SQLITE_OK)
    {
        gbilling_debug ("%s: %s\n", __func__, sqlite3_errmsg (db_file));
        return -1;
    }

    if (test)
    {
        retval = gbilling_check_db ();
        return retval;
    }

    const gchar * const cmd[] =
    {
        /* general setting */
        "CREATE TABLE  "DBTABLE_SET" (      "
        ""DB_ID"       INTEGER PRIMARY KEY, "
        ""DB_NAME"     TEXT,                "
        ""DB_VALUE"    TEXT)                ",
        /* client */
        "CREATE TABLE "DBTABLE_CLIENT" (       "
        ""DB_ID"          INTEGER PRIMARY KEY, "
        ""DB_NAME"        TEXT,                "
        ""DBCLIENT_IP"    TEXT,                "
        ""DBCLIENT_STATE" INTEGER)             ",
        /* cost */
        "CREATE TABLE "DBTABLE_COST" (         "
        ""DBCOST_ID"      INTEGER PRIMARY KEY, "
        ""DBCOST_NAME"    TEXT,                "
        ""DBCOST_MODE"    INTEGER,             "
        ""DBCOST_DEFAULT" INTEGER,             "
        ""DBCOST_FMIN"    INTEGER,             "
        ""DBCOST_IMIN"    INTEGER,             "
        ""DBCOST_FCOST"   INTEGER,             "
        ""DBCOST_ICOST"   INTEGER)             ",
        /* item */
        "CREATE TABLE "DBTABLE_ITEM" (         "
        ""DBITEM_ID"      INTEGER PRIMARY KEY, "
        ""DBITEM_NAME"    TEXT,                "
        ""DBITEM_COST"    INTEGER,             "
        ""DBITEM_ACTIVE"  INTEGER)             ",
        /* paket */
        "CREATE TABLE "DBTABLE_PREPAID" (            "
        ""DBPREPAID_ID"         INTEGER PRIMARY KEY, "
        ""DBPREPAID_NAME"       TEXT,                "
        ""DBPREPAID_DURATION"   INTEGER,             "
        ""DBPREPAID_COST"       INTEGER,             "
        ""DBPREPAID_ACTIVE"     INTEGER)             ",
        /* log */
        "CREATE TABLE "DBTABLE_LOG" (            "
        ""DBLOG_ID"         INTEGER PRIMARY KEY, "
        ""DBLOG_TSTART"     INTEGER,             "
        ""DBLOG_TEND"       INTEGER,             "
        ""DBLOG_CLIENT"     TEXT,                "
        ""DBLOG_USERNAME"   TEXT,                "
        ""DBLOG_TYPE"       TEXT,                "
        ""DBLOG_VOUCHER"    INTEGER,             "
        ""DBLOG_START"      INTEGER,             "
        ""DBLOG_DURATION"   INTEGER,             "
        ""DBLOG_END"        INTEGER,             "
        ""DBLOG_UCOST"      INTEGER,             "
        ""DBLOG_ACOST"      INTEGER,             "
        ""DBLOG_TCOST"      INTEGER,             "
        ""DBLOG_DESC"       TEXT)                ",
        /* member_group */
        "CREATE TABLE "DBTABLE_MEMBER_GROUP" (      "
        ""DBMEMBER_GROUP_ID"   INTEGER PRIMARY KEY, "
        ""DBMEMBER_GROUP_NAME" TEXT,                "
        ""DBMEMBER_GROUP_COST" TEXT)                ",
        /* member */
        "CREATE TABLE "DBTABLE_MEMBER" (          "
        ""DBMEMBER_ID"       INTEGER PRIMARY KEY, "
        ""DBMEMBER_REGISTER" INTEGER,             "
        ""DBMEMBER_STATUS"   INTEGER,             "
        ""DBMEMBER_USERNAME" TEXT,                "
        ""DBMEMBER_PASSWD"   TEXT,                "
        ""DBMEMBER_GROUP"    TEXT,                "
        ""DBMEMBER_FULLNAME" TEXT,                "
        ""DBMEMBER_ADDRESS"  TEXT,                "
        ""DBMEMBER_PHONE"    TEXT,                "
        ""DBMEMBER_EMAIL"    TEXT,                "
        ""DBMEMBER_IDCARD"   TEXT)                ",
        /* recovery */
        "CREATE TABLE "DBTABLE_RECOV" ("
        ""DB_ID"            INTEGER,   "
        ""DBRECOV_USERNAME" TEXT,      "
        ""DBRECOV_TYPE"     INTEGER,   "
        ""DBRECOV_PAKET"    INTEGER,   "
        ""DBRECOV_VOUCHER"  INTEGER,   "
        ""DBRECOV_START"    INTEGER,   "
        ""DBRECOV_UCOST"    INTEGER,   "
        ""DBRECOV_ACOST"    INTEGER,   "
        ""DB_DESC"          TEXT)      ",
        /* cost rotation */
        "CREATE TABLE "DBTABLE_COSTROTATION" ("
        ""DBCOSTROTATION_COST"   TEXT,        "
        ""DBCOSTROTATION_SHOUR"  INTEGER,     "
        ""DBCOSTROTATION_SMIN"   INTEGER,     "
        ""DBCOSTROTATION_EHOUR"  INTEGER,     "
        ""DBCOSTROTATION_EMIN"   INTEGER,     "
        ""DBCOSTROTATION_STATE"  INTEGER)     ",
    };

    gchar *logo, **cmdz;
    gint i, j;
    const int def_setting_prealloc_str = 32;

    for (i = 0; i < G_N_ELEMENTS (cmd); i++)
        gbilling_sql_exec (cmd[i], NULL);

#ifdef PACKAGE_PIXMAPS_DIR
    logo = g_build_filename (PACKAGE_PIXMAPS_DIR, "gbilling.png", NULL);
#else
    gchar *tmp = g_get_current_dir ();
    logo = g_build_filename (tmp, "share", "pixmaps", "gbilling.png", NULL);
    g_free (tmp);
#endif

    /* alokasi ini tidak menghitung jumlah member, tapi hanya dengan memberi limit */
    cmdz = g_new (gchar*, def_setting_prealloc_str);
    i = 0;
    /* masukkan setting default server */
    cmdz[i++] = g_strdup ("INSERT INTO "DBTABLE_SET" VALUES (NULL, "
                          "'"DBSET_VERSION"', '"DB_VERSION"')");
    cmdz[i++] = g_strdup ("INSERT INTO "DBTABLE_SET" VALUES (NULL, "
                          "'"DBSET_SERVER_PORT"', '"GBILLING_PORT"')");
    cmdz[i++] = g_strdup ("INSERT INTO "DBTABLE_SET" VALUES (NULL, "
                          "'"DBSET_SERVER_BACKUP"', '"GBILLING_BACKUP"')");
    cmdz[i++] = g_strdup ("INSERT INTO "DBTABLE_SET" VALUES (NULL, "
                          "'"DBSET_SERVER_AUTOSTART"', 1)");
    cmdz[i++] = g_strdup ("INSERT INTO "DBTABLE_SET" VALUES (NULL, "
                          "'"DBSET_SERVER_NAME"', '"GBILLING_NAME"')");
    cmdz[i++] = g_strdup ("INSERT INTO "DBTABLE_SET" VALUES (NULL, "
                          "'"DBSET_SERVER_DESC"', '"GBILLING_DESC"')");
    cmdz[i++] = g_strdup ("INSERT INTO "DBTABLE_SET" VALUES (NULL, "
                          "'"DBSET_SERVER_ADDR"', '"GBILLING_SITE"')");
    cmdz[i++] = g_strdup_printf ("INSERT INTO "DBTABLE_SET" VALUES (NULL, "
                                 "'"DBSET_SERVER_LOGO"', '%s')", logo);
    cmdz[i++] = g_strdup ("INSERT INTO "DBTABLE_SET" VALUES (NULL, "
                          "'"DBSET_SERVER_SUSER"', '"GBILLING_USER"')");
    cmdz[i++] = g_strdup ("INSERT INTO "DBTABLE_SET" VALUES (NULL, "
                          "'"DBSET_SERVER_SPASSWD"', '"GBILLING_PASS"')");
    cmdz[i++] = g_strdup ("INSERT INTO "DBTABLE_SET" VALUES (NULL, "
                          "'"DBSET_SERVER_CUSER"', '"GBILLING_USER"')");
    cmdz[i++] = g_strdup ("INSERT INTO "DBTABLE_SET" VALUES (NULL, "
                          "'"DBSET_SERVER_CPASSWD"', '"GBILLING_PASS"')");
    cmdz[i++] = g_strdup ("INSERT INTO "DBTABLE_SET" VALUES (NULL, "
                          "'"DBSET_DISPLAY_NAME"', 0)");
    cmdz[i++] = g_strdup ("INSERT INTO "DBTABLE_SET" VALUES (NULL, "
                          "'"DBSET_DISPLAY_DESC"', 0)");
    cmdz[i++] = g_strdup ("INSERT INTO "DBTABLE_SET" VALUES (NULL, "
                          "'"DBSET_DISPLAY_ADDR"', 0)");
    cmdz[i++] = g_strdup ("INSERT INTO "DBTABLE_SET" VALUES (NULL, "
                          "'"DBSET_DISPLAY_LOGO"', 0)");
    /* setting log default */
    cmdz[i++] = g_strdup ("INSERT INTO "DBTABLE_SET" VALUES (NULL, "
                          "'"DBSET_LOG_SHOW"', '"GBILLING_LOGS"')");
    cmdz[i++] = g_strdup ("INSERT INTO "DBTABLE_SET" VALUES (NULL, "
                          "'"DBSET_LOG_DELTIME"', 0)");
    cmdz[i++] = g_strdup ("INSERT INTO "DBTABLE_SET" VALUES (NULL, "
                          "'"DBSET_LOG_USER"', '"GBILLING_USER"')");
    cmdz[i++] = g_strdup ("INSERT INTO "DBTABLE_SET" VALUES (NULL, "
                          "'"DBSET_LOG_PASSWD"', '"GBILLING_PASS"')");
    /* tarif default */
    cmdz[i++] = g_strdup_printf ("INSERT INTO "DBTABLE_COST" VALUES (NULL, "
                                 "'%s', %i, 1, %i, %i, %i, %i)",
                                 GBILLING_COST_NAME, GBILLING_COST_MODE_FMIN,
                                 GBILLING_COST_FMIN, GBILLING_COST_IMIN,
                                 GBILLING_COST_FCOST, GBILLING_COST_ICOST);
    g_assert (i < def_setting_prealloc_str);

    for (j = 0; j < i; j++)
    {
        gbilling_sql_exec (cmdz[j], NULL);
        g_free (cmdz[j]);
    }

    g_free (cmdz);
    g_free (logo);
    return 0;
}

/**
 * masukkan data dari @db_file ke struktur data client
 * return: 0 jika sukses, -1 jika query gagal
 */
gint
gbilling_insert_data (void)
{
    GList *list, *ptr;
    GbillingClient *client;

    list = gbilling_server_get_client_list ();
    if (list)
    {
        for (ptr = list; ptr; ptr = g_list_next (ptr))
        {
            ClientData *cdata = g_new0 (ClientData, 1);
            client = ptr->data;
            cdata->name = g_strdup (client->name);
            cdata->ip = g_strdup (client->ip);
            client_list = g_list_prepend (client_list, cdata);
        }
        client_list = g_list_reverse (client_list);
        gbilling_client_list_free (list);
    }
    cost_list = gbilling_server_get_cost_list ();
    costrotation_list = gbilling_server_get_cost_rotation ();
    prepaid_list = gbilling_server_get_prepaid_list ();
    item_list = gbilling_server_get_item_list ();
    group_list = gbilling_server_get_member_group_list ();

    server->sauth = gbilling_server_get_server_auth ();
    server->cauth = gbilling_server_get_client_auth ();
    server->lauth = gbilling_server_get_log_auth ();

    return 0;
}

/**
 * gbilling_insert_log:
 * @cdata: Data client.
 * @status: Status client saat log dimasukkan.
 * Returns: 0 jika berhasil, -1 jika gagal.
 *
 * Masukkan log client.
 */
gint
gbilling_insert_log (ClientData     *cdata,
                     GbillingStatus  status)
{
    g_return_val_if_fail (cdata != NULL, -1);

    GbillingPrepaid *prepaid;
    glong start, end;
    gchar *cmd = NULL, *type;

    switch (cdata->type)
    {
        case GBILLING_LOGIN_TYPE_PREPAID:
          prepaid = g_list_nth_data (prepaid_list, cdata->idtype);
          type = (gchar *) prepaid->name;
          break;
          
        default:
          type = (gchar *) gbilling_login_mode[cdata->type];
          break;
    }

    switch (status)
    {
        case GBILLING_STATUS_LOGIN:
          start = time_t_to_sec (cdata->tstart);
          cmd = sqlite3_mprintf ("INSERT INTO "DBTABLE_LOG" VALUES (NULL, %li, "
                                 "0, '%q', '%q', '%q', NULL, %li, 0, 0, %li,   "
                                 "%li, %li, '')", cdata->tstart, cdata->name, 
                                 cdata->username, type, start, cdata->ucost, 
                                 cdata->acost, cdata->ucost + cdata->acost);
          break;

        case GBILLING_STATUS_LOGOUT:
          /* setiap client mempunyai nama yg berbeda, dan client tsb harusnya
           * mempunyai time_t tstart yg berbeda.
           */
          end = time_t_to_sec (cdata->tend);
          cmd = sqlite3_mprintf ("UPDATE "DBTABLE_LOG" SET    "
                                 ""DBLOG_TEND"         = %li, "
                                 ""DBLOG_DURATION"     = %li, "
                                 ""DBLOG_END"          = %li, "
                                 ""DBLOG_UCOST"        = %li, "
                                 ""DBLOG_ACOST"        = %li, "
                                 ""DBLOG_TCOST"        = %li, "
                                 ""DBLOG_DESC"         = '%q' "
                                 "WHERE "DBLOG_CLIENT" = '%q' "
                                 "AND   "DBLOG_TSTART" = %li  ", 
                                 cdata->tend, cdata->duration, end,
                                 cdata->ucost, cdata->acost, 
                                 cdata->ucost + cdata->acost,
                                 cdata->desc ? cdata->desc : "", 
                                 cdata->name, cdata->tstart);
          break;

        case GBILLING_STATUS_MOVE: /* hapus log client yang pindah */
          start = time_t_to_sec (cdata->tstart);
          cmd = sqlite3_mprintf ("DELETE FROM "DBTABLE_LOG" WHERE "
                                 ""DBLOG_CLIENT" = '%q' AND       "
                                 ""DBLOG_TSTART" = %li            ",
                                 cdata->name, cdata->tstart);
          break;

        default:
          return -1;
    } /* switch */

    gbilling_sql_exec (cmd, NULL);
    sqlite3_free (cmd);

    return 0;
}

/**
 * gbilling_get_log:
 * @cname: Nama client.
 * @start: Waktu mulai client.
 *
 * Returns: Data log client atau %NULL jika tidak ditemukan.
 *
 * Ambil data log client, dealokasi dengan gbilling_log_free(). Jika data hasil
 * query lebih dari satu, maka data pertama yang diambil.
 */
GbillingLog*
gbilling_get_log (const gchar *cname,
                  time_t       start)
{
    g_return_val_if_fail (cname != NULL && start > 0, NULL);
    GbillingLog *log = NULL;
    gchar *cmd, **table;
    gint row, col;

    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_LOG" WHERE "
                           ""DBLOG_CLIENT" = '%q' AND "
                           ""DBLOG_TSTART" = %li LIMIT 1",
                           cname, start);
    gbilling_sql_get_table (cmd, &table, &row, &col);
    sqlite3_free (cmd);
    if (row > 0)
    {
        log = gbilling_log_new ();
        log->tstart = atol (table[col + 1]);
        log->tend = atol (table[col + 2]);
        /* log->voucher = atol (table[col + 6]); */
        log->ucost = atol (table[col + 10]);
        log->acost = atol (table[col + 11]);
        log->client = g_strdup (table[col + 3]);
        log->username = g_strdup (table[col + 4]);
        log->type = g_strdup (table[col + 5]);
        log->desc = g_strdup (table[col + 13]);
    }
    gbilling_sql_free_table (table);
    return log;
}

/**
 * gbilling_delete_log:
 * @cname: Nama client.
 */
void
gbilling_delete_log (const gchar *cname,
                     time_t       start)
{
    g_return_if_fail (cname != NULL && start > 0);
    gchar *cmd;
    
    cmd = sqlite3_mprintf ("DELETE FROM "DBTABLE_LOG" WHERE "
                           ""DBLOG_CLIENT" = '%q' AND "
                           ""DBLOG_TSTART" = %li", 
                           cname, start);
    gbilling_sql_exec (cmd, NULL);
    sqlite3_free (cmd);
}

/**
 * gbilling_recov_read:
 *
 * Isi list client dari data recovery.
 */
void
gbilling_recov_read (void)
{
    gchar *cmd;
    
    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_RECOV" ORDER BY "DB_ID"");
    gbilling_sql_exec (cmd, client_data_sqlcb);
    sqlite3_free (cmd);
}

/**
 * gbilling_recov_detect:
 * Returns: TRUE jika ada data recovery, FALSE jika data tidak ada.
 * 
 * Deteksi recovery login client.
 */
gboolean
gbilling_recov_detect (void)
{
    gint row;
    gchar **res;
    gchar *cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_RECOV"");

    gbilling_sql_get_table (cmd, &res, &row, NULL);
    gbilling_sql_free_table (res);
    sqlite3_free (cmd);
    return (row > 0 ? TRUE : FALSE);
}

void
gbilling_recov_write (const ClientData *cdata)
{
    g_return_if_fail (cdata != NULL);

    gchar *cmd;
    gint id;

    id = g_list_index (client_list, cdata);
    g_return_if_fail (id >= 0);
    cmd = sqlite3_mprintf ("INSERT INTO "DBTABLE_RECOV" VALUES (%i, '%q', "
                           "%i, %i, %lu, %li, %lu, %lu, '%q')", id,
                           cdata->username, cdata->type, cdata->idtype,
                           cdata->voucher, cdata->tstart, cdata->ucost,
                           cdata->acost, cdata->desc ? cdata->desc : "");
    gbilling_sql_exec (cmd, NULL);
    sqlite3_free (cmd);
}

/**
 * gbilling_recov_del:
 * @cdata: Data client yang ditulis.
 * 
 * Hapus data recovery client, dipanggil pada saat selesai pemakaian
 * atau update data recovery.
 **/
void
gbilling_recov_del (const ClientData *cdata)
{
    g_return_if_fail (cdata != NULL);

    gchar *cmd;
    gint id;

    id = g_list_index (client_list, cdata);
    g_return_if_fail (id >= 0);

    cmd = sqlite3_mprintf ("DELETE FROM "DBTABLE_RECOV" WHERE "DB_ID" = %i", id);
    gbilling_sql_exec (cmd, NULL);
    sqlite3_free (cmd);
}

/** 
 * gbilling_server_add_prepaid:
 * @prepaid:
 *
 * Returns:
 *
 */
gboolean
gbilling_server_add_prepaid (const GbillingPrepaid *prepaid)
{
    gchar *cmd, **table;
    gint row;
    gboolean ret = FALSE;
    
    g_return_val_if_fail (prepaid != NULL, ret);
    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_PREPAID" WHERE "
                           ""DBPREPAID_NAME" = '%q'", prepaid->name);
    gbilling_sql_get_table (cmd, &table, &row, NULL);
    sqlite3_free (cmd);
    gbilling_sql_assert_row (row, table, ret);
    if (row == 0)
    {
        cmd = sqlite3_mprintf ("INSERT INTO "DBTABLE_PREPAID" VALUES "
                               "(NULL, '%q', %i, %i, %i)", prepaid->name,
                               prepaid->duration, prepaid->cost,
                               prepaid->active ? 1 : 0);
        gbilling_sql_exec (cmd, NULL);
        sqlite3_free (cmd);
        ret = TRUE;
    }
    gbilling_sql_free_table (table);
    return ret;
}

/** 
 * gbilling_server_delete_prepaid:
 * @name:
 *
 * Returns:
 *
 */
gboolean
gbilling_server_delete_prepaid (const gchar *name)
{
    gchar *cmd, **table;
    gint row;
    gboolean ret = FALSE;
    
    g_return_val_if_fail (name != NULL, ret);
    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_PREPAID" WHERE "
                           ""DBPREPAID_NAME" LIKE '%q'", name);
    gbilling_sql_get_table (cmd, &table, &row, NULL);
    sqlite3_free (cmd);
    gbilling_sql_assert_row (row, table, ret);
    if (row == 1)
    {
        cmd = sqlite3_mprintf ("DELETE FROM "DBTABLE_PREPAID" WHERE "
                               ""DBPREPAID_NAME" = '%q'", name);
        gbilling_sql_exec (cmd, NULL);
        sqlite3_free (cmd);
    }
    gbilling_sql_free_table (table);
    return ret;
}

/**
 * gbilling_server_update_prepaid:
 * @new: Data baru paket.
 * @old_name: Nama lama paket.
 *
 * Returns:
 */
gboolean
gbilling_server_update_prepaid (const GbillingPrepaid *new,
                                const gchar           *old_name)
{
    gchar *cmd, **table;
    gint row;
    gboolean ret = FALSE, test_name = FALSE;
    
    g_return_val_if_fail (new != NULL && old_name != NULL, ret);
    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_PREPAID" WHERE "
                           ""DBPREPAID_NAME" LIKE '%q'", new->name);
    gbilling_sql_get_table (cmd, &table, &row, NULL);
    sqlite3_free (cmd);
    gbilling_sql_assert_row (row, table, ret);
    if (row == 1 && !g_ascii_strcasecmp (new->name, old_name))
        test_name = TRUE;
    if (row == 0)
        test_name = TRUE;
    gbilling_sql_free_table (table);
    if (test_name)
    {
        cmd = sqlite3_mprintf ("UPDATE "DBTABLE_PREPAID" SET "
                               ""DBPREPAID_NAME"     = '%q', "
                               ""DBPREPAID_DURATION" =  %i,  "
                               ""DBPREPAID_COST"     =  %i,  "
                               ""DBPREPAID_ACTIVE"   =  %i   "
                               "WHERE "DBPREPAID_NAME" = '%q'",
                               new->name, new->duration, new->cost, 
                               new->active ? 1 : 0, old_name);
        gbilling_sql_exec (cmd, NULL);
        sqlite3_free (cmd);
        ret = TRUE;
    }
    return ret;                          
}

/**
 * gbilling_server_get_prepaid_by_name:
 * @name:
 * 
 * Returns:
 *
 */
GbillingPrepaid*
gbilling_server_get_prepaid_by_name (const gchar *name)
{
    GbillingPrepaid *prepaid = NULL;
    gchar *cmd, **table;
    gint row, col, i;
    
    g_return_val_if_fail (name != NULL, prepaid);
    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_PREPAID" WHERE "
                           ""DBPREPAID_NAME" = '%q'", name);
    gbilling_sql_get_table (cmd, &table, &row, &col);
    sqlite3_free (cmd);
    gbilling_sql_assert_row (row, table, prepaid);
    if (row == 1)
    {
        i = col;
        prepaid = gbilling_prepaid_new ();
        prepaid->id = atoi (table[i++]);
        g_snprintf (prepaid->name, sizeof(prepaid->name), table[i++]);
        prepaid->duration = atol (table[i++]);
        prepaid->cost = atol (table[i++]);
        prepaid->active = atoi (table[i]);
    }
    gbilling_sql_free_table (table);
    return prepaid;
}

/**
 * gbilling_server_get_prepaid_by_id:
 * @id:
 * 
 * Returns:
 *
 */
GbillingPrepaid*
gbilling_server_get_prepaid_by_id (guint id)
{
    GbillingPrepaid *prepaid = NULL;
    gchar *cmd, **table;
    gint row, col, i;
    
    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_PREPAID"");
    gbilling_sql_get_table (cmd, &table, &row, &col);
    sqlite3_free (cmd);
    /* nth row = (n - 1)th id, n > 0 */
    if (row > 0 && id < row)
    {
        if (table[id])
        {
            i = col;
            prepaid = gbilling_prepaid_new ();
            prepaid->id = atoi (table[i++]);
            g_snprintf (prepaid->name, sizeof(prepaid->name), table[i++]);
            prepaid->duration = atol (table[i++]);
            prepaid->cost = atol (table[i++]);
            prepaid->active = atoi (table[i]);
        }
    }
    gbilling_sql_free_table (table);
    return prepaid;
}

/**
 * gbilling_server_get_prepaid_list:
 *
 * Returns:
 *
 */
GbillingPrepaidList*
gbilling_server_get_prepaid_list (void)
{
    GbillingPrepaidList *list = NULL;
    gchar *cmd, **table;
    gint row, col;

    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_PREPAID"");
    gbilling_sql_get_table (cmd, &table, &row, &col);
    sqlite3_free (cmd);

    if (row > 0)
    {
        gint i, j;
        for (i = 0, j = col; i < row; i++)
        {
            GbillingPrepaid *prepaid;        
            prepaid = gbilling_prepaid_new ();
            prepaid->id = atoi (table[j++]);
            g_snprintf (prepaid->name, sizeof(prepaid->name), table[j++]);
            prepaid->duration = atol (table[j++]);
            prepaid->cost = atol (table[j++]);
            prepaid->active = atoi (table[j++]);
            list = g_list_prepend (list, prepaid);
        }
        list = g_list_reverse (list);
        gbilling_sql_free_table (table);
    }
    return list;
}

/**
 * gbilling_server_add_item:
 * @item:
 *
 * Returns:
 *
 */
gboolean
gbilling_server_add_item (const GbillingItem *item)
{
    g_return_val_if_fail (item != NULL, FALSE);

    gchar *cmd, **table;
    gint row;
    gboolean ret = FALSE;

    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_ITEM" WHERE "
                           ""DBITEM_NAME" LIKE '%q'", item->name);
    gbilling_sql_get_table (cmd, &table, &row, NULL);
    sqlite3_free (cmd);
    gbilling_sql_assert_row (row, table, ret);
    if (row == 0)
    {
        cmd = sqlite3_mprintf ("INSERT INTO "DBTABLE_ITEM" VALUES ("
                               "NULL, '%q', %i, %i)", item->name,
                               item->cost, item->active ? 1 : 0);
        gbilling_sql_exec (cmd, NULL);
        sqlite3_free (cmd);
        ret = TRUE;
    }
    gbilling_sql_free_table (table);
    return ret;
}

/**
 * gbilling_server_delete_item:
 * @item:
 *
 * Returns:
 *
 */
gboolean
gbilling_server_delete_item (const gchar *name)
{
    g_return_val_if_fail (name != NULL, FALSE);
    
    gchar *cmd, **table;
    gint row;
    gboolean ret = FALSE;
    
    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_ITEM" WHERE "
                           ""DBITEM_NAME" LIKE '%q'", name);
    gbilling_sql_get_table (cmd, &table, &row, NULL);
    sqlite3_free (cmd);
    gbilling_sql_assert_row (row, table, ret);
    if (row == 1)
    {
        cmd = sqlite3_mprintf ("DELETE FROM "DBTABLE_ITEM" WHERE "
                               ""DBITEM_NAME" = '%q'", name);
        gbilling_sql_exec (cmd, NULL);
        sqlite3_free (cmd);
        ret = TRUE;
    }
    gbilling_sql_free_table (table);
    return ret;
}

/**
 * gbilling_server_update_item:
 * @new:
 * @old:
 *
 * Returns:
 *
 */
gboolean
gbilling_server_update_item (const GbillingItem *new,
                             const gchar        *old_name)
{
    g_return_val_if_fail (new != NULL && old_name != NULL, FALSE);
    
    gchar *cmd, **table;
    gint row;
    gboolean ret = FALSE, test_name = FALSE;
    
    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_ITEM" WHERE "
                           ""DBITEM_NAME" LIKE '%q'", new->name);
    gbilling_sql_get_table (cmd, &table, &row, NULL);
    sqlite3_free (cmd);
    gbilling_sql_assert_row (row, table, ret);
    if (row == 1 && !g_ascii_strcasecmp (new->name, old_name))
        test_name = TRUE;
    if (row == 0)
        test_name = TRUE;
    gbilling_sql_free_table (table);
    if (test_name)
    {
        cmd = sqlite3_mprintf ("UPDATE "DBTABLE_ITEM" SET "
                               ""DBITEM_NAME"   = '%q',   "
                               ""DBITEM_COST"   =  %i,    "
                               ""DBITEM_ACTIVE" =  %i     "
                               "WHERE "DBITEM_NAME" = '%q'",
                               new->name, new->cost,
                               new->active ? 1 : 0, old_name);
        gbilling_sql_exec (cmd, NULL);
        sqlite3_free (cmd);
        ret = TRUE;
    }
    return ret;
}

/**
 * gbilling_server_get_item_by_name:
 * @name:
 *
 * Returns:
 *
 */
GbillingItem*
gbilling_server_get_item_by_name (const gchar *name)
{
    g_return_val_if_fail (name != NULL, NULL);
    
    GbillingItem *item = NULL;
    gchar *cmd, **table;
    gint row, col, i;
    
    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_ITEM" "
                           "WHERE "DBITEM_NAME" = '%q'   ", name);
    gbilling_sql_get_table (cmd, &table, &row, &col);
    sqlite3_free (cmd);
    gbilling_sql_assert_row (row, table, item);
    if (row == 1)
    {
        i = col;
        item = gbilling_item_new ();
        item->id = atoi (table[i++]);
        g_snprintf (item->name, sizeof(item->name), table[i++]);
        item->cost = atol (table[i++]);
        item->active = atoi (table[i]);
    }
    gbilling_sql_free_table (table);
    return item;
}

GbillingItemList*
gbilling_server_get_item_list (void)
{
    GbillingItemList *list = NULL;
    gchar *cmd, **table;
    gint row, col;
    
    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_ITEM"");
    gbilling_sql_get_table (cmd, &table, &row, &col);
    sqlite3_free (cmd);
    if (row > 0)
    {
        gint i, j;

        for (i = 0, j = col; i < row; i++)
        {
            GbillingItem *item;
            item = gbilling_item_new ();
            item->id = atoi (table[j++]);
            g_snprintf (item->name, sizeof(item->name), table[j++]);
            item->cost = atol (table[j++]);
            item->active = atoi (table[j++]);
            list = g_list_prepend (list, item);
         }
         list = g_list_reverse (list);
    }
    gbilling_sql_free_table (table);
    return list;
}

/**
 * gbilling_server_add_member_group:
 * @group:
 *
 * Returns:
 *
 */
gboolean
gbilling_server_add_member_group (const GbillingMemberGroup *group)
{
    g_return_val_if_fail (group != NULL, FALSE);
    
    gchar *cmd, **table;
    gint row;
    gboolean ret = FALSE;
    
    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_MEMBER_GROUP" WHERE "
                           ""DBMEMBER_GROUP_NAME" LIKE '%q'", group->name);
    gbilling_sql_get_table (cmd, &table, &row, NULL);
    sqlite3_free (cmd);
    gbilling_sql_assert_row (row, table, ret);
    if (row == 0)
    {
        cmd = sqlite3_mprintf ("INSERT INTO "DBTABLE_MEMBER_GROUP" VALUES "
                               "(NULL, '%q', '%q')", group->name, group->cost);
        gbilling_sql_exec (cmd, NULL);
        sqlite3_free (cmd);
        ret = TRUE;
    }
    gbilling_sql_free_table (table);
    return ret;
}

/**
 * gbilling_server_update_member_group:
 * @new:
 * @old_name:
 *
 * Returns:
 *
 */
gboolean
gbilling_server_update_member_group (const GbillingMemberGroup *new,
                                     const gchar               *old_name)
{
    g_return_val_if_fail (new != NULL && old_name != NULL, FALSE);
    
    gchar *cmd, **table;
    gint row;
    gboolean ret = FALSE, test_name = FALSE;
    
    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_MEMBER_GROUP" "
                           "WHERE "DBMEMBER_GROUP_NAME" LIKE '%q'", new->name);
    gbilling_sql_get_table (cmd, &table, &row, NULL);
    sqlite3_free (cmd);
    gbilling_sql_assert_row (row, table, ret);
    if (row == 1 && !g_ascii_strcasecmp (new->name, old_name))
        test_name = TRUE;
    if (row == 0)
        test_name = TRUE;
    gbilling_sql_free_table (table);
    if (test_name)
    {
        cmd = sqlite3_mprintf ("UPDATE "DBTABLE_MEMBER_GROUP" SET "
                               ""DBMEMBER_GROUP_NAME" = '%q',     "
                               ""DBMEMBER_GROUP_COST" = '%q'      "
                               "WHERE "DBMEMBER_GROUP_NAME" = '%q'",
                               new->name, new->cost, old_name);
        gbilling_sql_exec (cmd, NULL);
        sqlite3_free (cmd);
        ret = TRUE;
    }
    return ret;
}

/**
 * gbilling_server_delete_member_group:
 * @group:
 *
 * Returns:
 *
 */
gboolean
gbilling_server_delete_member_group (const gchar *name)
{
    g_return_val_if_fail (name != NULL, FALSE);
    
    gchar *cmd, **table;
    gint row;
    gboolean ret = FALSE;
    
    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_MEMBER_GROUP" WHERE "
                           ""DBMEMBER_GROUP_NAME" = '%q'", name);
    gbilling_sql_get_table (cmd, &table, &row, NULL);
    sqlite3_free (cmd);
    gbilling_sql_assert_row (row, table, ret);
    if (row == 1)
    {
        cmd = sqlite3_mprintf ("DELETE FROM "DBTABLE_MEMBER_GROUP" WHERE "
                               ""DBMEMBER_GROUP_NAME" = '%q'", name);
        gbilling_sql_exec (cmd, NULL);
        sqlite3_free (cmd);
        ret = TRUE;
    }
    gbilling_sql_free_table (table);
    return ret;
}

/**
 * gbilling_server_get_member_group_by_name:
 *
 * Returns:
 *
 */
GbillingMemberGroup*
gbilling_server_get_member_group_by_name (const gchar *name)
{
    g_return_val_if_fail (name != NULL, NULL);

    GbillingMemberGroup *group = NULL;
    gchar *cmd, **table;
    gint row, col, i;
    
    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_MEMBER_GROUP" WHERE "
                           ""DBMEMBER_GROUP_NAME" = '%q'", name);
    gbilling_sql_get_table (cmd, &table, &row, &col);
    sqlite3_free (cmd);
    gbilling_sql_assert_row (row, table, group);
    if (row == 1)
    {
        i = col;
        group = gbilling_member_group_new ();
        group->id = atoi (table[i++]);
        g_snprintf (group->name, sizeof(group->name), table[i++]);
        g_snprintf (group->cost, sizeof(group->cost), table[i++]);
    }
    gbilling_sql_free_table (table);
    return group;
}

/**
 * gbilling_server_get_member_group_list:
 *
 * Returns:
 *
 */
GbillingMemberGroupList*
gbilling_server_get_member_group_list (void)
{
    GbillingMemberGroupList *list = NULL;
    gchar *cmd, **table;
    gint row, col, i, j;
    
    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_MEMBER_GROUP"");
    gbilling_sql_get_table (cmd, &table, &row, &col);
    sqlite3_free (cmd);
    if (row > 0)
    {
        for (i = 0, j = col; i < row; i++)
        {
            GbillingMemberGroup *group;
            group = gbilling_member_group_new ();
            group->id = atoi (table[j++]);
            g_snprintf (group->name, sizeof(group->name), table[j++]);
            g_snprintf (group->cost, sizeof(group->cost), table[j++]);
            list = g_list_prepend (list, group);
         }
         list = g_list_reverse (list);
    }
    gbilling_sql_free_table (table);
    return list;
}

/**
 * gbilling_server_add_member:
 * @member:
 *
 * Returns:
 *
 */
gboolean
gbilling_server_add_member (const GbillingMember *member)
{
    g_return_val_if_fail (member != NULL, FALSE);
    
    gchar *cmd, **table;
    gint row;
    gboolean ret = FALSE;
    
    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_MEMBER" WHERE "
                           ""DBMEMBER_USERNAME" LIKE '%q' AND    "
                           ""DBMEMBER_GROUP" = '%q'              ", 
                           member->username, member->group);
    gbilling_sql_get_table (cmd, &table, &row, NULL);
    sqlite3_free (cmd);
    gbilling_sql_assert_row (row, table, ret);
    if (row == 0)
    {
        cmd = sqlite3_mprintf ("INSERT INTO "DBTABLE_MEMBER" VALUES (NULL, %li, "
                               "%i, '%q', '%q', '%q', '%q', '%q', '%q', '%q', "
                               "'%q')", member->reg, member->status, 
                               member->username, member->pass, member->group,
                               member->fullname, member->address, member->phone, 
                               member->email, member->idcard);
        gbilling_sql_exec (cmd, NULL);
        sqlite3_free (cmd);
        ret = TRUE;
    }
    gbilling_sql_free_table (table);
    return ret;
}

/** 
 * gbilling_server_update_member: 
 * @new:
 * @old:
 *
 * Returns:
 *
 */
gboolean
gbilling_server_update_member (const GbillingMember *new,
                               const GbillingMember *old)
{
    g_return_val_if_fail (new != NULL && old != NULL, FALSE);

    gchar *cmd, **table;
    gint row;
    gboolean ret = FALSE, test_name = FALSE;

    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_MEMBER" WHERE "
                           ""DBMEMBER_USERNAME" LIKE '%q' AND    "
                           ""DBMEMBER_GROUP" = '%q'              ", 
                           new->username, new->group);
    gbilling_sql_get_table (cmd, &table, &row, NULL);
    sqlite3_free (cmd);
    gbilling_sql_assert_row (row, table, ret);
    if (row == 1 && !g_ascii_strcasecmp (new->username, old->username))
        test_name = TRUE;
    if (row == 0)
        test_name = TRUE;
    if (test_name)
    {
        cmd = sqlite3_mprintf ("UPDATE "DBTABLE_MEMBER" SET      "
                               ""DBMEMBER_REGISTER" = %li,       "
                               ""DBMEMBER_STATUS"   = %i,        "
                               ""DBMEMBER_USERNAME" = '%q',      "
                               ""DBMEMBER_PASSWD"   = '%q',      "
                               ""DBMEMBER_GROUP"    = '%q',      "
                               ""DBMEMBER_FULLNAME" = '%q',      "
                               ""DBMEMBER_ADDRESS"  = '%q',      "
                               ""DBMEMBER_PHONE"    = '%q',      "
                               ""DBMEMBER_EMAIL"    = '%q',      "
                               ""DBMEMBER_IDCARD"   = '%q'       "
                               "WHERE "DBMEMBER_USERNAME" = '%q' "
                               "AND "DBMEMBER_GROUP"      = '%q' ",
                               new->reg, new->status, new->username,
                               new->pass, new->group, new->fullname,
                               new->address, new->phone, new->email,
                               new->idcard, old->username, old->group);
        gbilling_sql_exec (cmd, NULL);
        sqlite3_free (cmd);
        ret = TRUE;
    }
    gbilling_sql_free_table (table);
    return ret;
}    

/**
 * gbilling_server_delete_member:
 * @member:
 *
 * Returns:
 *
 */
gboolean
gbilling_server_delete_member (const GbillingMember *member)
{
    g_return_val_if_fail (member != NULL, FALSE);
    
    gchar *cmd, **table;
    gint row;
    gboolean ret = FALSE;
    
    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_MEMBER" WHERE "
                           ""DBMEMBER_USERNAME" = '%q' AND       "
                           ""DBMEMBER_GROUP"    = '%q'", member->username, 
                           member->group);
    gbilling_sql_get_table (cmd, &table, &row, NULL);
    sqlite3_free (cmd);
    gbilling_sql_assert_row (row, table, ret);
    if (row == 1)
    {
        cmd = sqlite3_mprintf ("DELETE * FROM "DBTABLE_MEMBER" WHERE "
                               ""DBMEMBER_USERNAME" = '%q' AND       "
                               ""DBMEMBER_GROUP"    = '%q'", member->username,
                               member->group);
        gbilling_sql_exec (cmd, NULL);
        sqlite3_free (cmd);
        ret = TRUE;
    }
    gbilling_sql_free_table (table);
    return ret;
}

/**
 * gbilling_server_get_member:
 * @name:
 * @group:
 *
 * Returns:
 *
 */
GbillingMember*
gbilling_server_get_member (const gchar *name, 
                            const gchar *group)
{
    g_return_val_if_fail (name != NULL && group != NULL, NULL);

    GbillingMember *member = NULL;
    gchar *cmd, **table;
    gint row, col, i;

    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_MEMBER"   "
                           "WHERE "DBMEMBER_USERNAME" = '%q' "
                           "AND "DBMEMBER_GROUP"      = '%q' ",
                           name, group);
    gbilling_sql_get_table (cmd, &table, &row, &col);
    sqlite3_free (cmd);
    gbilling_sql_assert_row (row, table, member);
    if (row == 1)
    {
        i = col;
        member = gbilling_member_new ();
        i++; /* ID */
        member->reg = atol (table[i++]);
        member->status = atoi (table[i++]);
        member->username = g_strdup (table[i++]);
        member->pass = g_strdup (table[i++]);
        member->group = g_strdup (table[i++]);
        member->fullname = g_strdup (table[i++]);
        member->address = g_strdup (table[i++]);
        member->phone = g_strdup (table[i++]);
        member->email = g_strdup (table[i++]);
        member->idcard = g_strdup (table[i]);
    }
    gbilling_sql_free_table (table);
    return member;
}

/**
 * gbilling_server_get_member_total:
 *
 * @name: Nama grup, atau %NULL.
 *
 * Cari berapa banyak member dalam suatu grup, jika @group = %NULL, maka total
 * seluruh member dari semua grup. 
 * 
 * Returns: Jumlah member. 
 */
gint
gbilling_server_get_member_total (const gchar *group)
{
    gchar *cmd, **table;
    gint ret, row;
    
    if (group)
        cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_MEMBER" WHERE "
                               ""DBMEMBER_GROUP" = '%q'", group);
    else
        cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_MEMBER"");
    gbilling_sql_get_table (cmd, &table, &row, NULL);
    sqlite3_free (cmd);
    ret = row;
    gbilling_sql_free_table (table);
    return ret;
}

/**
 * gbilling_server_get_member_list:
 *
 * Returns:
 *
 */
GbillingMemberList*
gbilling_server_get_member_list (const gchar *stmt)
{
    GbillingMemberList *list = NULL;
    gchar *cmd, **table;
    gint row, col, i, j;
    
    cmd = stmt ? sqlite3_mprintf (stmt) : sqlite3_mprintf ("SELECT * FROM "DBTABLE_MEMBER"");
    gbilling_sql_get_table (cmd, &table, &row, &col);
    sqlite3_free (cmd);
    if (row > 0)
    {
        for (i = 0, j = col; i < row; i++)
        {
            GbillingMember *member;
            member = gbilling_member_new ();
            member->id = atoi (table[j++]);
            member->reg = atol (table[j++]);
            member->status = atoi (table[j++]);
            member->username = g_strdup (table[j++]);
            member->pass = g_strdup (table[j++]);
            member->group = g_strdup (table[j++]);
            member->fullname = g_strdup (table[j++]);
            member->address = g_strdup (table[j++]);
            member->phone = g_strdup (table[j++]);
            member->email = g_strdup (table[j++]);
            member->idcard = g_strdup (table[j++]);
            list = g_list_prepend (list, member);
        }
        list = g_list_reverse (list);
    }
    gbilling_sql_free_table (table);
    return list;
}
        

/**
 * Manajemen group member dalam database.
 * @group: Informasi group yang akan di tambahkan.
 * @old_name: Nama lama group tersebut sebelum perubahan (hanya untuk
 * mode update GBILLING_UPDATE_MODE_EDIT) atau NULL.
 * @mode: Mode update gBilling.
 */
void
gbilling_member_group_update (GbillingMemberGroup *group,
                              const gchar         *old_name,
                              GbillingUpdateMode   mode)
{
    g_return_if_fail (group != NULL);
    g_return_if_fail (mode >= GBILLING_UPDATE_MODE_ADD &&
                      mode <= GBILLING_UPDATE_MODE_DELETE);
    gchar *cmd;
    gint ret;

    switch (mode)
    {
        case GBILLING_UPDATE_MODE_ADD:
            cmd = sqlite3_mprintf ("INSERT INTO "DBTABLE_MEMBER_GROUP" "
                                   "VALUES (NULL, '%q', '%q')          ",
                                   group->name, group->cost);
            break;
        case GBILLING_UPDATE_MODE_EDIT:
            g_return_if_fail (old_name != NULL);
            cmd = sqlite3_mprintf ("UPDATE "DBTABLE_MEMBER_GROUP" SET   "
                                   ""DBMEMBER_GROUP_NAME" = '%q',       "
                                   ""DBMEMBER_GROUP_COST" = '%q'        "
                                   " WHERE "DBMEMBER_GROUP_NAME" = '%q' ",
                                   group->name, group->cost, old_name);
            break;
        default: /* GBILLING_UPDATE_MODE_DELETE */
            cmd = sqlite3_mprintf ("DELETE FROM "DBTABLE_MEMBER_GROUP" "
                                   "WHERE "DBMEMBER_GROUP_NAME" = '%q' ",
                                   group->name);
    }

    ret = sqlite3_exec (db_file, cmd, NULL, NULL, NULL);
    sqlite3_free (cmd);
    if (ret != SQLITE_OK)
        GBILLING_SQLITE3_EXIT_ERROR ("gbilling_member_group_update", NULL);
}

/**
 * Buat tabel member DBTABLE_MEMBER.
 * Returns: 0 jika table berhasil dibuat, -1 jika gagal.
 */
gint
gbilling_create_member_table (void)
{
    gchar *cmd;
    gint ret;

    cmd = sqlite3_mprintf ("CREATE TABLE "DBTABLE_MEMBER"           ("
                           ""DBMEMBER_ID"       INTEGER PRIMARY KEY, "
                           ""DBMEMBER_REGISTER" INTEGER,             "
                           ""DBMEMBER_STATUS"   INTEGER,             "
                           ""DBMEMBER_USERNAME" TEXT,                "
                           ""DBMEMBER_GROUP"    TEXT,                "
                           ""DBMEMBER_PASSWD"   TEXT,                "
                           ""DBMEMBER_FULLNAME" TEXT,                "
                           ""DBMEMBER_ADDRESS"  TEXT,                "
                           ""DBMEMBER_PHONE"    TEXT,                "
                           ""DBMEMBER_EMAIL"    TEXT,                "
                           ""DBMEMBER_IDCARD"   TEXT                )"
                          );
    ret = sqlite3_exec (db_file, cmd, NULL, NULL, NULL);
    sqlite3_free (cmd);
    if (ret != SQLITE_OK)
        return -1;

    return 0;
}

void
gbilling_member_update (GbillingMember     *val,
                        GbillingMember     *old_val,
                        GbillingUpdateMode  mode)
{
    g_return_if_fail (val != NULL);
    g_return_if_fail (mode >= GBILLING_UPDATE_MODE_ADD &&
                      mode <= GBILLING_UPDATE_MODE_DELETE);
    gchar *cmd;
    gint ret;

    switch (mode)
    {
        case GBILLING_UPDATE_MODE_ADD:
            cmd = sqlite3_mprintf ("INSERT INTO "DBTABLE_MEMBER" VALUES     "
                                   "(NULL, %li, %i, '%q', '%q', '%q', '%q', "
                                   " '%q', '%q', '%q', '%q')", val->reg,
                                    val->status, val->username, val->group,
                                    val->pass, val->fullname, val->address,
                                    val->phone, val->email, val->idcard);
            break;

        case GBILLING_UPDATE_MODE_EDIT:
            g_return_if_fail (old_val != NULL);
            cmd = sqlite3_mprintf ("UPDATE "DBTABLE_MEMBER" SET        "
                                   ""DBMEMBER_STATUS"          = %i,   "
                                   ""DBMEMBER_USERNAME"        = '%q', "
                                   ""DBMEMBER_GROUP"           = '%q', "
                                   ""DBMEMBER_PASSWD"          = '%q', "
                                   ""DBMEMBER_FULLNAME"        = '%q', "
                                   ""DBMEMBER_ADDRESS"         = '%q', "
                                   ""DBMEMBER_PHONE"           = '%q', "
                                   ""DBMEMBER_EMAIL"           = '%q', "
                                   ""DBMEMBER_PHONE"           = '%q', "
                                   ""DBMEMBER_IDCARD"          = '%q'  "
                                   "WHERE "DBMEMBER_USERNAME"  = '%q'  "
                                   "AND   "DBMEMBER_GROUP"     = '%q'  ",
                                    val->status, val->username, val->group,
                                    val->pass, val->fullname, val->address,
                                    val->phone, val->email, val->phone,
                                    val->idcard, old_val->username,
                                    old_val->group
                                  );
            break;

        default: /* GBILLING_UPDATE_MODE_DELETE */
            cmd = sqlite3_mprintf ("DELETE FROM "DBTABLE_MEMBER" WHERE "
                                   ""DBMEMBER_USERNAME" = '%q'   AND   "
                                   ""DBMEMBER_GROUP"    = '%q'         ",
                                    val->username, val->group          );
    } /* switch */
    ret = sqlite3_exec (db_file, cmd, NULL, NULL, NULL);
    sqlite3_free (cmd);
    if (ret != SQLITE_OK)
        GBILLING_SQLITE3_EXIT_ERROR ("gbilling_member_update", NULL);
}

GbillingMember*
gbilling_get_member (const gchar *name,
                     const gchar *group)
{
    GbillingMember *member;
    gchar **res, *cmd;
    gint ret, row, col;

    cmd = sqlite3_mprintf ("SELECT * FROM "DBTABLE_MEMBER" WHERE "
                           ""DBMEMBER_USERNAME" = '%q' AND       "
                           ""DBMEMBER_GROUP"    = '%q'           ",
                            name, group                          );
    ret = sqlite3_get_table (db_file, cmd, &res, &row, &col, NULL);
    sqlite3_free (cmd);
    if (ret != SQLITE_OK)
        GBILLING_SQLITE3_EXIT_ERROR ("gbilling_get_member", NULL);
    if (row == 0)
        return NULL;
    col++; /* pass DBMEMBER_ID */
    member = g_new0 (GbillingMember, 1);
    member->reg = atol (res[col++]);
    member->status = atoi (res[col++]);
    member->username = g_strdup (res[col++]);
    member->group = g_strdup (res[col++]);
    member->pass = g_strdup (res[col++]);
    /* opsional, check NULL values */
    member->fullname = g_strdup (res[col++]);
    member->address = g_strdup (res[col++]);
    member->phone = g_strdup (res[col++]);
    member->email = g_strdup (res[col++]);
    member->idcard = g_strdup (res[col]);
    sqlite3_free_table (res);

    return member;
}

