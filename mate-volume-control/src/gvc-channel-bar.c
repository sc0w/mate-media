/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 William Jon McCann
 * Copyright (C) 2014 Michal Ratajsky <michal.ratajsky@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include <canberra-gtk.h>
#include <libmatemixer/matemixer.h>

#include "gvc-channel-bar.h"

#define SCALE_SIZE 128
#define GVC_CHANNEL_BAR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GVC_TYPE_CHANNEL_BAR, GvcChannelBarPrivate))

struct _GvcChannelBarPrivate
{
        GtkOrientation       orientation;
        GtkWidget           *scale_box;
        GtkWidget           *start_box;
        GtkWidget           *end_box;
        GtkWidget           *image;
        GtkWidget           *label;
        GtkWidget           *low_image;
        GtkWidget           *scale;
        GtkWidget           *high_image;
        GtkWidget           *mute_box;
        GtkWidget           *mute_button;
        GtkAdjustment       *adjustment;
        gboolean             show_icons;
        gboolean             show_mute;
        gboolean             show_marks;
        gboolean             extended;
        GtkSizeGroup        *size_group;
        gboolean             symmetric;
        gboolean             click_lock;
        MateMixerStream     *stream;
        MateMixerStreamFlags stream_flags;
};

enum {
        PROP_0,
        PROP_STREAM,
        PROP_ORIENTATION,
        PROP_SHOW_ICONS,
        PROP_SHOW_MUTE,
        PROP_SHOW_MARKS,
        PROP_EXTENDED,
        PROP_NAME,
        PROP_ICON_NAME,
        PROP_LOW_ICON_NAME,
        PROP_HIGH_ICON_NAME,
        N_PROPERTIES
};

static GParamSpec *properties[N_PROPERTIES] = { NULL, };

static void     gvc_channel_bar_class_init    (GvcChannelBarClass *klass);
static void     gvc_channel_bar_init          (GvcChannelBar      *bar);

static gboolean on_scale_button_press_event   (GtkWidget          *widget,
                                               GdkEventButton     *event,
                                               GvcChannelBar      *bar);
static gboolean on_scale_button_release_event (GtkWidget          *widget,
                                               GdkEventButton     *event,
                                               GvcChannelBar      *bar);
static gboolean on_scale_scroll_event         (GtkWidget          *widget,
                                               GdkEventScroll     *event,
                                               GvcChannelBar      *bar);

#if GTK_CHECK_VERSION (3, 0, 0)
G_DEFINE_TYPE (GvcChannelBar, gvc_channel_bar, GTK_TYPE_BOX)
#else
G_DEFINE_TYPE (GvcChannelBar, gvc_channel_bar, GTK_TYPE_HBOX)
#endif

static void
create_scale_box (GvcChannelBar *bar)
{
#if GTK_CHECK_VERSION (3, 0, 0)
        bar->priv->scale_box = gtk_box_new (bar->priv->orientation, 6);
        bar->priv->start_box = gtk_box_new (bar->priv->orientation, 6);
        bar->priv->end_box   = gtk_box_new (bar->priv->orientation, 6);
        bar->priv->scale     = gtk_scale_new (bar->priv->orientation,
                                              bar->priv->adjustment);
#else
        if (bar->priv->orientation == GTK_ORIENTATION_VERTICAL) {
                bar->priv->scale_box = gtk_vbox_new (FALSE, 6);
                bar->priv->start_box = gtk_vbox_new (FALSE, 6);
                bar->priv->end_box   = gtk_vbox_new (FALSE, 6);
                bar->priv->scale     = gtk_vscale_new (bar->priv->adjustment);
        } else {
                bar->priv->scale_box = gtk_hbox_new (FALSE, 6);
                bar->priv->start_box = gtk_hbox_new (FALSE, 6);
                bar->priv->end_box   = gtk_hbox_new (FALSE, 6);
                bar->priv->scale     = gtk_hscale_new (bar->priv->adjustment);
        }
#endif
        if (bar->priv->orientation == GTK_ORIENTATION_VERTICAL) {
                gtk_widget_set_size_request (bar->priv->scale, -1, SCALE_SIZE);

                gtk_range_set_inverted (GTK_RANGE (bar->priv->scale), TRUE);

                gtk_box_pack_start (GTK_BOX (bar->priv->scale_box),
                                    bar->priv->start_box,
                                    FALSE, FALSE, 0);

                gtk_box_pack_start (GTK_BOX (bar->priv->start_box),
                                    bar->priv->image,
                                    FALSE, FALSE, 0);
                gtk_box_pack_start (GTK_BOX (bar->priv->start_box),
                                    bar->priv->label,
                                    FALSE, FALSE, 0);
                gtk_box_pack_start (GTK_BOX (bar->priv->start_box),
                                    bar->priv->high_image,
                                    FALSE, FALSE, 0);

                gtk_box_pack_start (GTK_BOX (bar->priv->scale_box),
                                    bar->priv->scale,
                                    TRUE, TRUE, 0);
                gtk_box_pack_start (GTK_BOX (bar->priv->scale_box),
                                    bar->priv->end_box,
                                    FALSE, FALSE, 0);

                gtk_box_pack_start (GTK_BOX (bar->priv->end_box),
                                    bar->priv->low_image,
                                    FALSE, FALSE, 0);
                gtk_box_pack_start (GTK_BOX (bar->priv->end_box),
                                    bar->priv->mute_box,
                                    FALSE, FALSE, 0);
        } else {
                gtk_widget_set_size_request (bar->priv->scale, SCALE_SIZE, -1);

                gtk_box_pack_start (GTK_BOX (bar->priv->scale_box),
                                    bar->priv->image,
                                    FALSE, FALSE, 0);
                gtk_box_pack_start (GTK_BOX (bar->priv->scale_box),
                                    bar->priv->start_box,
                                    FALSE, FALSE, 0);

                gtk_box_pack_end (GTK_BOX (bar->priv->start_box),
                                  bar->priv->low_image,
                                  FALSE, FALSE, 0);

                gtk_box_pack_start (GTK_BOX (bar->priv->start_box),
                                    bar->priv->label,
                                    TRUE, TRUE, 0);

                gtk_box_pack_start (GTK_BOX (bar->priv->scale_box),
                                    bar->priv->scale,
                                    TRUE, TRUE, 0);
                gtk_box_pack_start (GTK_BOX (bar->priv->scale_box),
                                    bar->priv->end_box,
                                    FALSE, FALSE, 0);

                gtk_box_pack_start (GTK_BOX (bar->priv->end_box),
                                    bar->priv->high_image,
                                    FALSE, FALSE, 0);
                gtk_box_pack_start (GTK_BOX (bar->priv->end_box),
                                    bar->priv->mute_box,
                                    FALSE, FALSE, 0);
        }

        if (bar->priv->show_icons) {
                gtk_widget_show (bar->priv->low_image);
                gtk_widget_show (bar->priv->high_image);
        } else {
                gtk_widget_hide (bar->priv->low_image);
                gtk_widget_hide (bar->priv->high_image);
        }

        ca_gtk_widget_disable_sounds (bar->priv->scale, FALSE);

        gtk_widget_add_events (bar->priv->scale, GDK_SCROLL_MASK);

        g_signal_connect (G_OBJECT (bar->priv->scale),
                          "button-press-event",
                          G_CALLBACK (on_scale_button_press_event),
                          bar);
        g_signal_connect (G_OBJECT (bar->priv->scale),
                          "button-release-event",
                          G_CALLBACK (on_scale_button_release_event),
                          bar);
        g_signal_connect (G_OBJECT (bar->priv->scale),
                          "scroll-event",
                          G_CALLBACK (on_scale_scroll_event),
                          bar);

        if (bar->priv->size_group != NULL) {
                gtk_size_group_add_widget (bar->priv->size_group,
                                           bar->priv->start_box);

                if (bar->priv->symmetric)
                        gtk_size_group_add_widget (bar->priv->size_group,
                                                   bar->priv->end_box);
        }

        gtk_scale_set_draw_value (GTK_SCALE (bar->priv->scale), FALSE);
}

static void
on_adjustment_value_changed (GtkAdjustment *adjustment,
                             GvcChannelBar *bar)
{
        gdouble value;
        gdouble lower;

        if (bar->priv->stream == NULL || bar->priv->click_lock == TRUE)
                return;

        value = gtk_adjustment_get_value (bar->priv->adjustment);
        lower = gtk_adjustment_get_lower (bar->priv->adjustment);

        if (bar->priv->stream_flags & MATE_MIXER_STREAM_HAS_MUTE)
                mate_mixer_stream_set_mute (bar->priv->stream, (value <= lower));

        if (bar->priv->stream_flags & MATE_MIXER_STREAM_CAN_SET_VOLUME)
                mate_mixer_stream_set_volume (bar->priv->stream, (guint) value);
}

static void
update_layout (GvcChannelBar *bar)
{
        GtkWidget *frame;

        if (bar->priv->scale == NULL)
                return;

        frame = gtk_widget_get_parent (bar->priv->scale_box);

        g_object_ref (bar->priv->image);
        g_object_ref (bar->priv->label);
        g_object_ref (bar->priv->mute_box);
        g_object_ref (bar->priv->low_image);
        g_object_ref (bar->priv->high_image);

        // XXX this is not the opposite of what is done above
        gtk_container_remove (GTK_CONTAINER (bar->priv->start_box),
                              bar->priv->image);
        gtk_container_remove (GTK_CONTAINER (bar->priv->start_box),
                              bar->priv->label);
        gtk_container_remove (GTK_CONTAINER (bar->priv->end_box),
                              bar->priv->mute_box);

        if (bar->priv->orientation == GTK_ORIENTATION_VERTICAL) {
                gtk_container_remove (GTK_CONTAINER (bar->priv->start_box),
                                      bar->priv->low_image);
                gtk_container_remove (GTK_CONTAINER (bar->priv->end_box),
                                      bar->priv->high_image);
        } else {
                gtk_container_remove (GTK_CONTAINER (bar->priv->end_box),
                                      bar->priv->low_image);
                gtk_container_remove (GTK_CONTAINER (bar->priv->start_box),
                                      bar->priv->high_image);
        }

        gtk_container_remove (GTK_CONTAINER (bar->priv->scale_box),
                              bar->priv->start_box);
        gtk_container_remove (GTK_CONTAINER (bar->priv->scale_box),
                              bar->priv->scale);
        gtk_container_remove (GTK_CONTAINER (bar->priv->scale_box),
                              bar->priv->end_box);
        gtk_container_remove (GTK_CONTAINER (frame),
                              bar->priv->scale_box);

        create_scale_box (bar);
        gtk_container_add (GTK_CONTAINER (frame), bar->priv->scale_box);

        g_object_unref (bar->priv->image);
        g_object_unref (bar->priv->label);
        g_object_unref (bar->priv->mute_box);
        g_object_unref (bar->priv->low_image);
        g_object_unref (bar->priv->high_image);

        gtk_widget_show_all (frame);
}

static void
update_marks (GvcChannelBar *bar)
{
        gdouble  base;
        gdouble  normal;
        gboolean has_mark = FALSE;

        gtk_scale_clear_marks (GTK_SCALE (bar->priv->scale));

        if (bar->priv->stream == NULL || bar->priv->show_marks == FALSE)
                return;
        if (!(bar->priv->stream_flags & MATE_MIXER_STREAM_HAS_VOLUME))
                return;

        /* Base volume represents unamplified volume, normal volume is the 100%
         * volume, in many cases they are the same as unamplified volume is unknown */
        base   = mate_mixer_stream_get_base_volume (bar->priv->stream);
        normal = mate_mixer_stream_get_normal_volume (bar->priv->stream);

        if (normal <= gtk_adjustment_get_lower (bar->priv->adjustment))
                return;

        if (base < normal) {
                gchar *str = g_strdup_printf ("<small>%s</small>", C_("volume", "Unamplified"));

                gtk_scale_add_mark (GTK_SCALE (bar->priv->scale),
                                    base,
                                    GTK_POS_BOTTOM,
                                    str);
                has_mark = TRUE;
                g_free (str);
        }

        /* Only show 100% mark if the scale is extended beyond 100% and
         * there is no unamplified mark or it is below the normal volume */
        if (bar->priv->extended && (base == normal || base < normal)) {
                gchar *str = g_strdup_printf ("<small>%s</small>", C_("volume", "100%"));

                gtk_scale_add_mark (GTK_SCALE (bar->priv->scale),
                                    normal,
                                    GTK_POS_BOTTOM,
                                    str);
                has_mark = TRUE;
                g_free (str);
        }

        if (has_mark) {
                gtk_alignment_set (GTK_ALIGNMENT (bar->priv->mute_box), 0.5, 0, 0, 0);

                gtk_misc_set_alignment (GTK_MISC (bar->priv->low_image), 0.5, 0);
                gtk_misc_set_alignment (GTK_MISC (bar->priv->high_image), 0.5, 0);
                gtk_misc_set_alignment (GTK_MISC (bar->priv->label), 0, 0);
        } else {
                gtk_alignment_set (GTK_ALIGNMENT (bar->priv->mute_box), 0.5, 0.5, 0, 0);

                gtk_misc_set_alignment (GTK_MISC (bar->priv->low_image), 0.5, 0.5);
                gtk_misc_set_alignment (GTK_MISC (bar->priv->high_image), 0.5, 0.5);
                gtk_misc_set_alignment (GTK_MISC (bar->priv->label), 0, 0.5);
        }
}

static void
update_adjustment_value (GvcChannelBar *bar)
{
        gdouble  value;
        gboolean set_lower = FALSE;

        /* Move the slider to the minimal value if the stream is muted or
         * volume is unavailable */
        if (!(bar->priv->stream_flags & MATE_MIXER_STREAM_HAS_VOLUME))
                set_lower = TRUE;
        else if (bar->priv->stream == NULL ||
                 bar->priv->stream_flags & MATE_MIXER_STREAM_HAS_MUTE)
                set_lower = mate_mixer_stream_get_mute (bar->priv->stream);

        if (set_lower)
                value = gtk_adjustment_get_lower (bar->priv->adjustment);
        else
                value = mate_mixer_stream_get_volume (bar->priv->stream);

        g_signal_handlers_block_by_func (G_OBJECT (bar->priv->adjustment),
                                         on_adjustment_value_changed,
                                         bar);

        gtk_adjustment_set_value (bar->priv->adjustment, value);

        g_signal_handlers_unblock_by_func (G_OBJECT (bar->priv->adjustment),
                                           on_adjustment_value_changed,
                                           bar);
}

static void
update_adjustment_limits (GvcChannelBar *bar)
{
        gdouble minimum = 0.0;
        gdouble maximum = 0.0;

        if (bar->priv->stream_flags & MATE_MIXER_STREAM_HAS_VOLUME) {
                minimum = mate_mixer_stream_get_min_volume (bar->priv->stream);
                if (bar->priv->extended)
                        maximum = mate_mixer_stream_get_max_volume (bar->priv->stream);
                else
                        maximum = mate_mixer_stream_get_normal_volume (bar->priv->stream);
        }

        gtk_adjustment_configure (bar->priv->adjustment,
                                  gtk_adjustment_get_value (bar->priv->adjustment),
                                  minimum,
                                  maximum,
                                  (maximum - minimum) / 100.0,
                                  (maximum - minimum) / 15.0,
                                  0.0);
}

static void
update_mute_button (GvcChannelBar *bar)
{
        if (bar->priv->show_mute == TRUE) {
                gboolean enable = FALSE;

                if (bar->priv->stream != NULL &&
                    bar->priv->stream_flags & MATE_MIXER_STREAM_HAS_MUTE)
                        enable = TRUE;

                if (enable == TRUE) {
                        gboolean mute = mate_mixer_stream_get_mute (bar->priv->stream);

                        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (bar->priv->mute_button),
                                                      mute);
                } else {
                        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (bar->priv->mute_button),
                                                      FALSE);
                }

                gtk_widget_set_sensitive (bar->priv->mute_button, enable);
                gtk_widget_show (bar->priv->mute_button);
        } else
                gtk_widget_hide (bar->priv->mute_button);
}

static gboolean
on_scale_button_press_event (GtkWidget      *widget,
                             GdkEventButton *event,
                             GvcChannelBar  *bar)
{

#if !GTK_CHECK_VERSION (3, 6, 0)
        /* Up to GTK 3.4 the slider selection only moves in increments when
         * clicking in the slider with the left button and it moves directly
         * to the clicked position with the middle mouse button.
         * Change this behaviour to also jump to the clicked position with the
         * left mouse button. */
        if (event->button == 1)
                event->button = 2;
#endif

        /* Muting the stream when volume is non-zero moves the slider to zero,
         * but the volume remains the same. In this case delay unmuting and
         * changing volume until user releases the mouse button. */
        if (bar->priv->stream_flags & MATE_MIXER_STREAM_HAS_MUTE &&
            bar->priv->stream_flags & MATE_MIXER_STREAM_HAS_VOLUME) {
                if (mate_mixer_stream_get_mute (bar->priv->stream) == TRUE) {
                        guint minimum = (guint) gtk_adjustment_get_lower (bar->priv->adjustment);

                        if (mate_mixer_stream_get_volume (bar->priv->stream) > minimum)
                                bar->priv->click_lock = TRUE;
                }
        }
        return FALSE;
}

static gboolean
on_scale_button_release_event (GtkWidget      *widget,
                               GdkEventButton *event,
                               GvcChannelBar  *bar)
{
#if !GTK_CHECK_VERSION (3, 6, 0)
        if (event->button == 1)
                event->button = 2;
#endif

        if (bar->priv->click_lock == TRUE) {
                /* The volume change is not reflected while the lock is
                 * held, propagate the change now that user has released
                 * the mouse button */
                bar->priv->click_lock = FALSE;
                on_adjustment_value_changed (bar->priv->adjustment, bar);
        }

        /* Play a sound */
        ca_gtk_play_for_widget (GTK_WIDGET (bar), 0,
                                CA_PROP_EVENT_ID, "audio-volume-change",
                                CA_PROP_EVENT_DESCRIPTION, "Volume change",
                                CA_PROP_APPLICATION_ID, "org.mate.VolumeControl",
                                CA_PROP_APPLICATION_NAME, _("Volume Control"),
                                CA_PROP_APPLICATION_VERSION, VERSION,
                                CA_PROP_APPLICATION_ICON_NAME, "multimedia-volume-control",
                                NULL);
        return FALSE;
}

static gboolean
on_scale_scroll_event (GtkWidget      *widget,
                       GdkEventScroll *event,
                       GvcChannelBar  *bar)
{
        GdkScrollDirection direction = event->direction;

#if GTK_CHECK_VERSION (3, 4, 0)
        if (direction == GDK_SCROLL_SMOOTH) {
                gdouble dx = 0.0;
                gdouble dy = 0.0;

                gdk_event_get_scroll_deltas ((const GdkEvent *) event, &dx, &dy);
                if (dy > 0.0)
                        direction = GDK_SCROLL_DOWN;
                else if (dy < 0.0)
                        direction = GDK_SCROLL_UP;
                else
                        return FALSE;
        }
#endif
        return gvc_channel_bar_scroll (bar, direction);
}

static void
on_stream_volume_notify (MateMixerStream *stream,
                         GParamSpec      *pspec,
                         GvcChannelBar   *bar)
{
        update_adjustment_value (bar);
}

static void
on_stream_mute_notify (MateMixerStream *stream,
                       GParamSpec      *pspec,
                       GvcChannelBar   *bar)
{
        if (bar->priv->show_mute == TRUE) {
                gboolean mute = mate_mixer_stream_get_mute (stream);

                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (bar->priv->mute_button), mute);
        }
        update_adjustment_value (bar);
}

MateMixerStream *
gvc_channel_bar_get_stream (GvcChannelBar *bar)
{
        g_return_val_if_fail (GVC_IS_CHANNEL_BAR (bar), NULL);

        return bar->priv->stream;
}

void
gvc_channel_bar_set_stream (GvcChannelBar *bar, MateMixerStream *stream)
{
        g_return_if_fail (GVC_IS_CHANNEL_BAR (bar));

        if (bar->priv->stream == stream)
                return;

        if (stream != NULL)
                g_object_ref (stream);

        if (bar->priv->stream != NULL) {
                g_signal_handlers_disconnect_by_func (G_OBJECT (bar->priv->stream),
                                                      G_CALLBACK (on_stream_volume_notify),
                                                      bar);
                g_signal_handlers_disconnect_by_func (G_OBJECT (bar->priv->stream),
                                                      G_CALLBACK (on_stream_mute_notify),
                                                      bar);
                g_object_unref (bar->priv->stream);
        }

        bar->priv->stream = stream;

        if (stream != NULL)
                bar->priv->stream_flags = mate_mixer_stream_get_flags (stream);
        else
                bar->priv->stream_flags = MATE_MIXER_STREAM_NO_FLAGS;

        if (bar->priv->stream_flags & MATE_MIXER_STREAM_HAS_VOLUME)
                g_signal_connect (G_OBJECT (stream),
                                  "notify::volume",
                                  G_CALLBACK (on_stream_volume_notify),
                                  bar);
        if (bar->priv->stream_flags & MATE_MIXER_STREAM_HAS_MUTE)
                g_signal_connect (G_OBJECT (stream),
                                  "notify::mute",
                                  G_CALLBACK (on_stream_mute_notify),
                                  bar);

        update_marks (bar);
        update_mute_button (bar);
        update_adjustment_limits (bar);
        update_adjustment_value (bar);
}

GtkOrientation
gvc_channel_bar_get_orientation (GvcChannelBar *bar)
{
        g_return_val_if_fail (GVC_IS_CHANNEL_BAR (bar), 0);

        return bar->priv->orientation;
}

void
gvc_channel_bar_set_orientation (GvcChannelBar  *bar,
                                 GtkOrientation  orientation)
{
        g_return_if_fail (GVC_IS_CHANNEL_BAR (bar));

        if (orientation == bar->priv->orientation)
                return;

        bar->priv->orientation = orientation;
        update_layout (bar);

        g_object_notify_by_pspec (G_OBJECT (bar), properties[PROP_ORIENTATION]);
}

gboolean
gvc_channel_bar_get_show_icons (GvcChannelBar *bar)
{
        g_return_val_if_fail (GVC_IS_CHANNEL_BAR (bar), FALSE);

        return bar->priv->show_icons;
}

void
gvc_channel_bar_set_show_icons (GvcChannelBar *bar, gboolean show_icons)
{
        g_return_if_fail (GVC_IS_CHANNEL_BAR (bar));

        if (show_icons == bar->priv->show_icons)
                return;

        bar->priv->show_icons = show_icons;

        if (bar->priv->show_icons == TRUE) {
                gtk_widget_show (bar->priv->low_image);
                gtk_widget_show (bar->priv->high_image);
        } else {
                gtk_widget_hide (bar->priv->low_image);
                gtk_widget_hide (bar->priv->high_image);
        }

        g_object_notify_by_pspec (G_OBJECT (bar), properties[PROP_SHOW_ICONS]);
}

gboolean
gvc_channel_bar_get_show_mute (GvcChannelBar *bar)
{
        g_return_val_if_fail (GVC_IS_CHANNEL_BAR (bar), FALSE);

        return bar->priv->show_mute;
}

void
gvc_channel_bar_set_show_mute (GvcChannelBar *bar, gboolean show_mute)
{
        g_return_if_fail (GVC_IS_CHANNEL_BAR (bar));

        if (show_mute == bar->priv->show_mute)
                return;

        bar->priv->show_mute = show_mute;
        update_mute_button (bar);

        g_object_notify_by_pspec (G_OBJECT (bar), properties[PROP_SHOW_MUTE]);
}

gboolean
gvc_channel_bar_get_show_marks (GvcChannelBar *bar)
{
        g_return_val_if_fail (GVC_IS_CHANNEL_BAR (bar), FALSE);

        return bar->priv->show_marks;
}

void
gvc_channel_bar_set_show_marks (GvcChannelBar *bar, gboolean show_marks)
{
        g_return_if_fail (GVC_IS_CHANNEL_BAR (bar));

        if (show_marks == bar->priv->show_marks)
                return;

        bar->priv->show_marks = show_marks;
        update_marks (bar);

        g_object_notify_by_pspec (G_OBJECT (bar), properties[PROP_SHOW_MARKS]);
}

gboolean
gvc_channel_bar_get_extended (GvcChannelBar *bar)
{
        g_return_val_if_fail (GVC_IS_CHANNEL_BAR (bar), FALSE);

        return bar->priv->extended;
}

void
gvc_channel_bar_set_extended (GvcChannelBar *bar, gboolean extended)
{
        g_return_if_fail (GVC_IS_CHANNEL_BAR (bar));

        if (extended == bar->priv->extended)
                return;

        bar->priv->extended = extended;

        /* Update displayed marks as non-extended scales do not show the 100%
         * limit at the end of the scale */
        update_marks (bar);
        update_adjustment_limits (bar);

        g_object_notify_by_pspec (G_OBJECT (bar), properties[PROP_EXTENDED]);
}

const gchar *
gvc_channel_bar_get_name (GvcChannelBar *bar)
{
        g_return_val_if_fail (GVC_IS_CHANNEL_BAR (bar), NULL);

        return gtk_label_get_text (GTK_LABEL (bar->priv->label));
}

void
gvc_channel_bar_set_name (GvcChannelBar *bar, const gchar *name)
{
        g_return_if_fail (GVC_IS_CHANNEL_BAR (bar));

        if (name != NULL) {
                gtk_label_set_text_with_mnemonic (GTK_LABEL (bar->priv->label), name);
                gtk_label_set_mnemonic_widget (GTK_LABEL (bar->priv->label),
                                               bar->priv->scale);

                gtk_widget_show (bar->priv->label);
        } else {
                gtk_label_set_text (GTK_LABEL (bar->priv->label), NULL);
                gtk_widget_hide (bar->priv->label);
        }

        g_object_notify_by_pspec (G_OBJECT (bar), properties[PROP_NAME]);
}

const gchar *
gvc_channel_bar_get_icon_name (GvcChannelBar *bar)
{
        const gchar *name = NULL;

        g_return_val_if_fail (GVC_IS_CHANNEL_BAR (bar), NULL);

        gtk_image_get_icon_name (GTK_IMAGE (bar->priv->image), &name, NULL);
        return name;
}

void
gvc_channel_bar_set_icon_name (GvcChannelBar *bar, const gchar *name)
{
        g_return_if_fail (GVC_IS_CHANNEL_BAR (bar));

        gtk_image_set_from_icon_name (GTK_IMAGE (bar->priv->image),
                                      name,
                                      GTK_ICON_SIZE_DIALOG);
        if (name != NULL)
                gtk_widget_show (bar->priv->image);
        else
                gtk_widget_hide (bar->priv->image);

        g_object_notify_by_pspec (G_OBJECT (bar), properties[PROP_ICON_NAME]);
}

const gchar *
gvc_channel_bar_get_low_icon_name (GvcChannelBar *bar)
{
        const gchar *name = NULL;

        g_return_val_if_fail (GVC_IS_CHANNEL_BAR (bar), NULL);

        gtk_image_get_icon_name (GTK_IMAGE (bar->priv->low_image), &name, NULL);
        return name;
}

void
gvc_channel_bar_set_low_icon_name (GvcChannelBar *bar, const gchar *name)
{
        g_return_if_fail (GVC_IS_CHANNEL_BAR (bar));

        gtk_image_set_from_icon_name (GTK_IMAGE (bar->priv->low_image),
                                      name,
                                      GTK_ICON_SIZE_BUTTON);

        g_object_notify_by_pspec (G_OBJECT (bar), properties[PROP_LOW_ICON_NAME]);
}

const gchar *
gvc_channel_bar_get_high_icon_name (GvcChannelBar *bar)
{
        const gchar *name = NULL;

        g_return_val_if_fail (GVC_IS_CHANNEL_BAR (bar), NULL);

        gtk_image_get_icon_name (GTK_IMAGE (bar->priv->high_image), &name, NULL);
        return name;
}

void
gvc_channel_bar_set_high_icon_name (GvcChannelBar *bar, const gchar *name)
{
        g_return_if_fail (GVC_IS_CHANNEL_BAR (bar));

        gtk_image_set_from_icon_name (GTK_IMAGE (bar->priv->high_image),
                                      name,
                                      GTK_ICON_SIZE_BUTTON);

        g_object_notify_by_pspec (G_OBJECT (bar), properties[PROP_HIGH_ICON_NAME]);
}

gboolean
gvc_channel_bar_scroll (GvcChannelBar *bar, GdkScrollDirection direction)
{
        gdouble value;
        gdouble minimum;
        gdouble maximum;
        gdouble scrollstep;

        g_return_val_if_fail (GVC_IS_CHANNEL_BAR (bar), FALSE);

        if (bar->priv->orientation == GTK_ORIENTATION_VERTICAL) {
                if (direction != GDK_SCROLL_UP && direction != GDK_SCROLL_DOWN)
                        return FALSE;
        } else {
                /* Switch direction for RTL */
                if (gtk_widget_get_direction (GTK_WIDGET (bar)) == GTK_TEXT_DIR_RTL) {
                        if (direction == GDK_SCROLL_RIGHT)
                                direction = GDK_SCROLL_LEFT;
                        else if (direction == GDK_SCROLL_LEFT)
                                direction = GDK_SCROLL_RIGHT;
                }

                /* Switch side scroll to vertical */
                if (direction == GDK_SCROLL_RIGHT)
                        direction = GDK_SCROLL_UP;
                else if (direction == GDK_SCROLL_LEFT)
                        direction = GDK_SCROLL_DOWN;
        }

        value   = gtk_adjustment_get_value (bar->priv->adjustment);
        minimum = gtk_adjustment_get_lower (bar->priv->adjustment);
        maximum = gtk_adjustment_get_upper (bar->priv->adjustment);

        scrollstep = maximum / 100.0 * 5.0;

        if (direction == GDK_SCROLL_UP) {
                if (value + scrollstep > maximum)
                        value = maximum;
                else
                        value = value + scrollstep;
        } else if (direction == GDK_SCROLL_DOWN) {
                if (value - scrollstep < minimum)
                        value = minimum;
                else
                        value = value - scrollstep;
        }

        gtk_adjustment_set_value (bar->priv->adjustment, value);
        return TRUE;
}

void
gvc_channel_bar_set_size_group (GvcChannelBar *bar,
                                GtkSizeGroup  *group,
                                gboolean       symmetric)
{
        g_return_if_fail (GVC_IS_CHANNEL_BAR (bar));
        g_return_if_fail (GTK_IS_SIZE_GROUP (group));

        bar->priv->size_group = group;
        bar->priv->symmetric = symmetric;

        if (bar->priv->size_group != NULL) {
                gtk_size_group_add_widget (bar->priv->size_group,
                                           bar->priv->start_box);

                if (bar->priv->symmetric)
                        gtk_size_group_add_widget (bar->priv->size_group,
                                                   bar->priv->end_box);
        }
        gtk_widget_queue_draw (GTK_WIDGET (bar));
}

static void
gvc_channel_bar_set_property (GObject       *object,
                              guint          prop_id,
                              const GValue  *value,
                              GParamSpec    *pspec)
{
        GvcChannelBar *self = GVC_CHANNEL_BAR (object);

        switch (prop_id) {
        case PROP_STREAM:
                gvc_channel_bar_set_stream (self, g_value_get_object (value));
                break;
        case PROP_ORIENTATION:
                gvc_channel_bar_set_orientation (self, g_value_get_enum (value));
                break;
        case PROP_SHOW_MUTE:
                gvc_channel_bar_set_show_mute (self, g_value_get_boolean (value));
                break;
        case PROP_SHOW_ICONS:
                gvc_channel_bar_set_show_icons (self, g_value_get_boolean (value));
                break;
        case PROP_SHOW_MARKS:
                gvc_channel_bar_set_show_marks (self, g_value_get_boolean (value));
                break;
        case PROP_EXTENDED:
                gvc_channel_bar_set_extended (self, g_value_get_boolean (value));
                break;
        case PROP_NAME:
                gvc_channel_bar_set_name (self, g_value_get_string (value));
                break;
        case PROP_ICON_NAME:
                gvc_channel_bar_set_icon_name (self, g_value_get_string (value));
                break;
        case PROP_LOW_ICON_NAME:
                gvc_channel_bar_set_low_icon_name (self, g_value_get_string (value));
                break;
        case PROP_HIGH_ICON_NAME:
                gvc_channel_bar_set_high_icon_name (self, g_value_get_string (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gvc_channel_bar_get_property (GObject     *object,
                              guint        prop_id,
                              GValue      *value,
                              GParamSpec  *pspec)
{
        GvcChannelBar *self = GVC_CHANNEL_BAR (object);

        switch (prop_id) {
        case PROP_STREAM:
                g_value_set_object (value, self->priv->stream);
                break;
        case PROP_ORIENTATION:
                g_value_set_enum (value, self->priv->orientation);
                break;
        case PROP_SHOW_MUTE:
                g_value_set_boolean (value, self->priv->show_mute);
                break;
        case PROP_SHOW_ICONS:
                g_value_set_boolean (value, self->priv->show_icons);
                break;
        case PROP_SHOW_MARKS:
                g_value_set_boolean (value, self->priv->show_marks);
                break;
        case PROP_EXTENDED:
                g_value_set_boolean (value, self->priv->extended);
                break;
        case PROP_NAME:
                g_value_set_string (value, gtk_label_get_text (GTK_LABEL (self->priv->label)));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gvc_channel_bar_class_init (GvcChannelBarClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->set_property = gvc_channel_bar_set_property;
        object_class->get_property = gvc_channel_bar_get_property;

        properties[PROP_STREAM] =
                g_param_spec_object ("stream",
                                     "Stream",
                                     "MateMixer stream",
                                     MATE_MIXER_TYPE_STREAM,
                                     G_PARAM_READWRITE |
                                     G_PARAM_CONSTRUCT |
                                     G_PARAM_STATIC_STRINGS);

        properties[PROP_ORIENTATION] =
                g_param_spec_enum ("orientation",
                                   "Orientation",
                                   "The orientation of the scale",
                                   GTK_TYPE_ORIENTATION,
                                   GTK_ORIENTATION_VERTICAL,
                                   G_PARAM_READWRITE |
                                   G_PARAM_STATIC_STRINGS);

        properties[PROP_SHOW_MUTE] =
                g_param_spec_boolean ("show-mute",
                                      "show mute",
                                      "Whether stream is muted",
                                      FALSE,
                                      G_PARAM_READWRITE |
                                      G_PARAM_CONSTRUCT |
                                      G_PARAM_STATIC_STRINGS);

        properties[PROP_SHOW_ICONS] =
                g_param_spec_boolean ("show-icons",
                                      "show mute",
                                      "Whether to show low and high icons",
                                      FALSE,
                                      G_PARAM_READWRITE |
                                      G_PARAM_CONSTRUCT |
                                      G_PARAM_STATIC_STRINGS);

        properties[PROP_SHOW_MARKS] =
                g_param_spec_boolean ("show-marks",
                                      "Show marks",
                                      "Whether to show scale marks",
                                      FALSE,
                                      G_PARAM_READWRITE |
                                      G_PARAM_CONSTRUCT |
                                      G_PARAM_STATIC_STRINGS);

        properties[PROP_EXTENDED] =
                g_param_spec_boolean ("extended",
                                      "Extended",
                                      "Allow the scale to be extended above normal volume",
                                      FALSE,
                                      G_PARAM_READWRITE |
                                      G_PARAM_STATIC_STRINGS);

        properties[PROP_NAME] =
                g_param_spec_string ("name",
                                     "Name",
                                     "Name to display for this stream",
                                     NULL,
                                     G_PARAM_READWRITE |
                                     G_PARAM_CONSTRUCT |
                                     G_PARAM_STATIC_STRINGS);

        properties[PROP_ICON_NAME] =
                g_param_spec_string ("icon-name",
                                     "Icon name",
                                     "Name of icon to display for this stream",
                                     NULL,
                                     G_PARAM_WRITABLE |
                                     G_PARAM_CONSTRUCT |
                                     G_PARAM_STATIC_STRINGS);

        properties[PROP_LOW_ICON_NAME] =
                g_param_spec_string ("low-icon-name",
                                     "Low icon name",
                                     "Name of low volume icon to display for this stream",
                                     "audio-volume-low",
                                     G_PARAM_WRITABLE |
                                     G_PARAM_CONSTRUCT |
                                     G_PARAM_STATIC_STRINGS);

        properties[PROP_HIGH_ICON_NAME] =
                g_param_spec_string ("high-icon-name",
                                     "High icon name",
                                     "Name of high volume icon to display for this stream",
                                     "audio-volume-high",
                                     G_PARAM_WRITABLE |
                                     G_PARAM_CONSTRUCT |
                                     G_PARAM_STATIC_STRINGS);

        g_object_class_install_properties (object_class, N_PROPERTIES, properties);

        g_type_class_add_private (klass, sizeof (GvcChannelBarPrivate));
}

static void
on_mute_button_toggled (GtkToggleButton *button, GvcChannelBar *bar)
{
        gboolean mute;

        if (G_UNLIKELY (bar->priv->stream == NULL))
                g_warn_if_reached ();

        mute = gtk_toggle_button_get_active (button);

        mate_mixer_stream_set_mute (bar->priv->stream, mute);
}

static void
gvc_channel_bar_init (GvcChannelBar *bar)
{
        GtkWidget *frame;

        bar->priv = GVC_CHANNEL_BAR_GET_PRIVATE (bar);

        /* Mute button */
        bar->priv->mute_button = gtk_check_button_new_with_label (_("Mute"));
        gtk_widget_set_no_show_all (bar->priv->mute_button, TRUE);

        g_signal_connect (bar->priv->mute_button,
                          "toggled",
                          G_CALLBACK (on_mute_button_toggled),
                          bar);

        bar->priv->mute_box = gtk_alignment_new (0.5, 0.5, 0, 0);
        gtk_container_add (GTK_CONTAINER (bar->priv->mute_box), bar->priv->mute_button);

        bar->priv->image = gtk_image_new ();
        gtk_widget_set_no_show_all (bar->priv->image, TRUE);

        /* Low/high icons */
        bar->priv->low_image = gtk_image_new ();
        gtk_widget_set_no_show_all (bar->priv->low_image, TRUE);

        bar->priv->high_image = gtk_image_new ();
        gtk_widget_set_no_show_all (bar->priv->high_image, TRUE);

        bar->priv->label = gtk_label_new (NULL);
        gtk_misc_set_alignment (GTK_MISC (bar->priv->label), 0.0, 0.5);
        gtk_widget_set_no_show_all (bar->priv->label, TRUE);

        /* Frame */
        frame = gtk_frame_new (NULL);
        gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
        gtk_box_pack_start (GTK_BOX (bar), frame, TRUE, TRUE, 0);

        gtk_widget_show_all (frame);

        /* Create a default adjustment */
        bar->priv->adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 0, 0, 0, 0));

        g_object_ref_sink (bar->priv->adjustment);

        g_signal_connect (bar->priv->adjustment,
                          "value-changed",
                          G_CALLBACK (on_adjustment_value_changed),
                          bar);

        /* Initially create a vertical scale box */
        bar->priv->orientation = GTK_ORIENTATION_VERTICAL;

        create_scale_box (bar);

        gtk_container_add (GTK_CONTAINER (frame), bar->priv->scale_box);
}

GtkWidget *
gvc_channel_bar_new (MateMixerStream *stream)
{
        return g_object_new (GVC_TYPE_CHANNEL_BAR,
                             "stream", stream,
#if GTK_CHECK_VERSION (3, 0, 0)
                             "orientation", GTK_ORIENTATION_HORIZONTAL,
#endif
                             NULL);
}
