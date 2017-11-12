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
 *  sound.c
 *  This file is part of gBilling (http://gbilling.sourceforge.net)
 *  Copyright (C) 2010, Ardhan Madras <ajhwb@knac.com>
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef _WIN32
# include <windows.h>
#else
# include <gst/gst.h>
#endif

#include <glib.h>

#include "gbilling.h"
#include "server.h"


static gboolean init = FALSE;

#ifndef _WIN32
static gboolean
bus_watch_func (GstBus     *bus,
                GstMessage *msg,
                gpointer    data)
{
    gchar *debug;
    GError *error;
    gint type = GST_MESSAGE_TYPE (msg);

    if (type == GST_MESSAGE_EOS)
        g_main_loop_quit ((GMainLoop *) data);
    else if (type == GST_MESSAGE_ERROR)
    {
        gst_message_parse_error (msg, &error, &debug);

        gbilling_debug ("%s: %s\n", __func__, error->message);
        g_error_free (error);
        g_free (debug);
    }

    return TRUE;
}
#endif

#ifndef _WIN32
static void
on_pad_added (GstElement *element,
              GstPad     *pad,
              gpointer    data)
{
    GstPad *sinkpad;

    sinkpad = gst_element_get_static_pad ((GstElement *) data, "sink");

    gst_pad_link (pad, sinkpad);

    gst_object_unref (sinkpad);
}
#endif


#ifndef _WIN32
/**
 * sound_play_gst:
 * @event: Event.
 *
 * Returns: TRUE jika berhasil, FALSE jika gagal.
 *
 * Putar file suara dengan backend GStreamer.
 */
static gboolean
sound_play_gst (GbillingEvent event)
{
    GstElement *pipeline, *source, *demuxer, *conv, *sink;
    GstBus *bus;
    GMainLoop *loop;

    pipeline = gst_pipeline_new ("gbilling-player");
    source = gst_element_factory_make ("filesrc", "file-source");
    demuxer = gst_element_factory_make ("wavparse", "wav-parser");
    conv = gst_element_factory_make ("audioconvert", "converter");
    sink = gst_element_factory_make ("autoaudiosink", "audio-output");

    if (!pipeline || !source || !demuxer || !conv || !sink)
    {
        gbilling_debug ("%s: some elements could not be created\n", __func__);
        return FALSE;
    }

    g_object_set ((GObject *) source, "location", current_soundopt[event].sound, NULL);

    loop = g_main_loop_new (NULL, FALSE);
    bus = gst_pipeline_get_bus ((GstPipeline *) pipeline);
    gst_bus_add_watch (bus, bus_watch_func, loop);
    gst_object_unref (bus);

    gst_bin_add_many ((GstBin *) pipeline, source, demuxer, conv, sink, NULL);

    gst_element_link (source, demuxer);
    gst_element_link_many (conv, sink, NULL);

    g_signal_connect (demuxer, "pad-added", (GCallback) on_pad_added, conv);

    gst_element_set_state (pipeline, GST_STATE_PLAYING);
    g_main_loop_run (loop);
    gst_element_set_state (pipeline, GST_STATE_NULL);

    gst_object_unref ((GstObject *) pipeline);

    return TRUE;
}
#endif

/*
 * Gunakan thread di Win32 karena SND_SYNC blocking sampai suara selesai, 
 * saya tidak tahu mengapa dengan flag SND_ASYNC suara tidak dimainkan.
 */
#ifdef _WIN32

static GCond *win32_cond;

static GbillingEvent win32_event;

static gpointer
sound_play_win32_func (gpointer data)
{
    GMutex *mutex = g_mutex_new ();
    win32_cond = g_cond_new ();

    g_mutex_lock (mutex);

    while (1)
    {
        g_cond_wait (win32_cond, mutex);

        PlaySound (current_soundopt[win32_event].sound, NULL, SND_NODEFAULT | SND_SYNC);
    }

    /* Tidak pernah kesini */
    g_mutex_unlock (mutex);
    g_mutex_free (mutex);
    g_cond_free (win32_cond);

    return NULL;
}

/**
 * sound_play_win32_func:
 * @event: Event.
 *
 * Returns: Selalu TRUE.
 *
 * Putar file suara di Win32.
 */
static gboolean
sound_play_win32 (GbillingEvent event)
{
    win32_event = event;

    g_cond_signal (win32_cond);
}
#endif

gboolean
sound_play (GbillingEvent event)
{
    g_return_val_if_fail (init == TRUE, FALSE);

#ifndef _WIN32
    return sound_play_gst (event);
#else
    return sound_play_win32 (event);
#endif
}

void
sound_init (gint    *argc,
            gchar ***argv)
{
    g_return_if_fail (init == FALSE);

#ifndef _WIN32
    gst_init (NULL, NULL);
#else
    g_thread_create ((GThreadFunc) sound_play_win32_func, NULL, FALSE, NULL);
#endif

    init = TRUE;
}


