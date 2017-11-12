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
 *  setting.c
 *  File ini adalah bagian dari gBilling (http://gbilling.sourceforge.net)
 *  Copyright (C) 2010, Ardhan Madras <ajhwb@knac.com>
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <glib.h>

#include "gbilling.h"
#include "server.h"

#define KEYFILE_NAME "server.conf"


GKeyFile *keyfile = NULL;

gboolean
read_setting (void)
{
    g_return_val_if_fail (keyfile == NULL, FALSE);

    gboolean retval;
    gchar *file, *tmp;
    GError *error = NULL;
    gint i = 0;

    file = g_build_filename (g_get_home_dir (), ".gbilling", KEYFILE_NAME, NULL);

    retval = g_file_test (file, G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS);
    if (!retval)
    {
        g_free (file);
        return retval;
    }

    keyfile = g_key_file_new ();
    retval = g_key_file_load_from_file (keyfile, file, G_KEY_FILE_NONE, &error);
    g_free (file);

    if (!retval)
    {
        gbilling_debug ("%s: %s\n", __func__, error->message);
        g_error_free (error);
        g_key_file_free (keyfile);
        keyfile = NULL;
    }

    server_info = gbilling_server_info_new ();

    tmp = g_key_file_get_string (keyfile, "server", "name", NULL);
    g_snprintf (server_info->name, sizeof(server_info->name), tmp);
    g_free (tmp);
    
    tmp = g_key_file_get_string (keyfile, "server", "description", NULL);
    g_snprintf (server_info->desc, sizeof(server_info)->desc, tmp);
    g_free (tmp);

    tmp = g_key_file_get_string (keyfile, "server", "address", NULL);
    g_snprintf (server_info->addr, sizeof(server_info->addr), tmp);
    g_free (tmp);

    server = g_malloc0 (sizeof(struct _server));

    server->port = g_key_file_get_integer (keyfile, "server", "port", NULL);
    server->autoactive = g_key_file_get_boolean (keyfile, "server", "autoactive", NULL);
    server->display_name = g_key_file_get_boolean (keyfile, "server", "display_name", NULL);
    server->display_desc = g_key_file_get_boolean (keyfile, "server", "display_description", NULL);
    server->display_addr = g_key_file_get_boolean (keyfile, "server", "display_address", NULL);
    server->display_logo = g_key_file_get_boolean (keyfile, "server", "display_logo", NULL);
    server->logo = g_key_file_get_string (keyfile, "server", "logo", NULL);

    server->autodisable = g_key_file_get_boolean (keyfile, "client", "autodisable", NULL);
    server->autorestart = g_key_file_get_boolean (keyfile, "client", "autorestart", NULL);

    server->logshow = g_key_file_get_integer (keyfile, "log", "show", NULL);
    server->autodel = g_key_file_get_boolean (keyfile, "log", "autodel", NULL);
    server->deltime = g_key_file_get_integer (keyfile, "log", "deltime", NULL);

    for (i = 0; i < G_N_ELEMENTS (current_soundopt); i++)
    {
        gchar *key1 = g_strdup_printf ("play_sound%i", i + 1);
        gchar *key2 = g_strdup_printf ("sound%i", i + 1);

        current_soundopt[i].play = g_key_file_get_boolean (keyfile, "sounds", key1, NULL);
        current_soundopt[i].sound = g_key_file_get_string (keyfile, "sounds", key2, NULL);
        g_free (key1);
        g_free (key2);
    }

    return retval;
}

static gboolean
write_setting0 (void)
{
    g_return_val_if_fail (keyfile != NULL, FALSE);

    gchar *data, *file;
    GIOChannel *channel;
    GIOStatus status;
    GError *error = NULL;

    data = g_key_file_to_data (keyfile, NULL, &error);
    if (!data)
    {
err:
        gbilling_debug ("%s: %s\n", __func__, error->message);
        g_error_free (error);
        return FALSE;
    }


    file = g_build_filename (g_get_home_dir (), ".gbilling", KEYFILE_NAME, NULL);
    channel = g_io_channel_new_file (file, "w", &error);
    g_free (file);
    if (!channel)
    {
        g_free (data); 
        goto err;
    }

    status = g_io_channel_write_chars (channel, g_strstrip (data), -1, NULL, &error);
    g_free (data);
    if (status != G_IO_STATUS_NORMAL)
        goto err;

    g_io_channel_shutdown (channel, TRUE, NULL);
    g_io_channel_unref (channel);

    return TRUE;
}

gboolean
write_setting (void)
{
    g_return_val_if_fail (keyfile != NULL, FALSE);
    
    gint i = 0;

    g_key_file_set_integer (keyfile, "server", "port", server->port);
    g_key_file_set_boolean (keyfile, "server", "autoactive", server->autoactive);
    g_key_file_set_boolean (keyfile, "server", "display_name", server->display_name);
    g_key_file_set_string (keyfile, "server", "name", server_info->name);
    g_key_file_set_boolean (keyfile, "server", "display_description", server->display_desc);
    g_key_file_set_string (keyfile, "server", "description", server_info->desc);
    g_key_file_set_boolean (keyfile, "server", "display_address", server->display_addr);
    g_key_file_set_string (keyfile, "server", "address", server_info->addr);
    g_key_file_set_boolean (keyfile, "server", "display_logo", server->display_logo);
    g_key_file_set_string (keyfile, "server", "logo", server->logo);

    g_key_file_set_boolean (keyfile, "client", "autodisable", server->autodisable);
    g_key_file_set_boolean (keyfile, "client", "autorestart", server->autorestart);

    g_key_file_set_integer (keyfile, "log", "show", server->logshow);
    g_key_file_set_boolean (keyfile, "log", "autodel", server->autodel);
    g_key_file_set_integer (keyfile, "log", "deltime", server->deltime);

    do {
        gchar *key1 = g_strdup_printf ("play_sound%i", i + 1);
        gchar *key2 = g_strdup_printf ("sound%i", i + 1);

        g_key_file_set_boolean (keyfile, "sounds", key1, current_soundopt[i].play);
        if (current_soundopt[i].play)
            g_key_file_set_string (keyfile, "sounds", key2, current_soundopt[i].sound);

        g_free (key1);
        g_free (key2);
    } while (++i < G_N_ELEMENTS (current_soundopt));

    return write_setting0 ();
}

static gchar*
get_sound (const gchar *file)
{
    gchar *str;

#ifdef PACKAGE_DATA_DIR
    str = g_build_filename (PACKAGE_DATA_DIR, "sounds", file, NULL);
#else
    gchar *tmp = g_get_current_dir ();
    str = g_build_filename (tmp, "share", "sounds", file, NULL);
    g_free (tmp);
#endif

    return str;
}

gboolean
create_setting (void)
{
    g_return_val_if_fail (keyfile == NULL, FALSE);

    gchar *str;
    gint i = 0;
    gboolean retval;
    keyfile = g_key_file_new ();

    g_key_file_set_integer (keyfile, "server", "port", SERVER_DEFAULT_PORT);
    g_key_file_set_boolean (keyfile, "server", "autoactive", TRUE);
    g_key_file_set_boolean (keyfile, "server", "display_name", FALSE);
    g_key_file_set_string (keyfile, "server", "name", GBILLING_NAME);
    g_key_file_set_boolean (keyfile, "server", "display_description", FALSE);
    g_key_file_set_string (keyfile, "server", "description", GBILLING_DESC);
    g_key_file_set_boolean (keyfile, "server", "display_address", FALSE);
    g_key_file_set_string (keyfile, "server", "address", GBILLING_SITE);

#ifdef PACKAGE_PIXMAPS_DIR
    str = g_build_filename (PACKAGE_PIXMAPS_DIR, "gbilling.png", NULL);
#else
    gchar *tmp = g_get_current_dir ();
    str = g_build_filename (tmp, "share", "pixmaps", "gbilling.png", NULL);
    g_free (tmp);
#endif

    g_key_file_set_boolean (keyfile, "server", "display_logo", FALSE);
    g_key_file_set_string (keyfile, "server", "logo", str);
    g_free (str);

    g_key_file_set_boolean (keyfile, "client", "autodisable", FALSE);
    g_key_file_set_boolean (keyfile, "client", "autorestart", FALSE);

    g_key_file_set_integer (keyfile, "log", "show", 86400);
    g_key_file_set_boolean (keyfile, "log", "autodel", FALSE);
    g_key_file_set_integer (keyfile, "log", "deltime", 0);

    do {
        gchar *key1 = g_strdup_printf ("play_sound%i", i + 1);
        gchar *key2 = g_strdup_printf ("sound%i", i + 1);

        if (default_soundopt[i].play)
            str = get_sound (default_soundopt[i].sound);
        else
            str = g_strdup ("");
        g_key_file_set_boolean (keyfile, "sounds", key1, default_soundopt[i].play);
        g_key_file_set_string (keyfile, "sounds", key2, str);

        g_free (key1);
        g_free (key2);
        g_free (str);
    } while (++i < G_N_ELEMENTS (default_soundopt));

    retval = write_setting0 ();
    g_key_file_free (keyfile);
    keyfile = NULL;

    return retval;
}

