/*
 * transmission-remote-gtk - A GTK RPC client to Transmission
 * Copyright (C) 2011-2013  Alan Fitton

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <rss-glib/rss-glib.h>

#include "trg-rss-window.h"
#include "trg-rss-model.h"
#include "trg-client.h"
#include "util.h"

G_DEFINE_TYPE(TrgRssWindow, trg_rss_window,
              GTK_TYPE_WINDOW)
#define TRG_RSS_WINDOW_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), TRG_TYPE_RSS_WINDOW, TrgRssWindowPrivate))
enum {
	PROP_0, PROP_PARENT, PROP_CLIENT
};

typedef struct _TrgRssWindowPrivate TrgRssWindowPrivate;

struct _TrgRssWindowPrivate {
    TrgMainWindow *parent;
    TrgClient *client;
};

static GObject *instance = NULL;

static void
trg_rss_window_get_property(GObject * object,
                                     guint property_id,
                                     GValue * value, GParamSpec * pspec)
{
    TrgRssWindowPrivate *priv =
        TRG_RSS_WINDOW_GET_PRIVATE(object);
    switch (property_id) {
    case PROP_CLIENT:
        g_value_set_pointer(value, priv->client);
        break;
    case PROP_PARENT:
        g_value_set_object(value, priv->parent);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void
trg_rss_window_set_property(GObject * object,
                                     guint property_id,
                                     const GValue * value,
                                     GParamSpec * pspec)
{
    TrgRssWindowPrivate *priv =
        TRG_RSS_WINDOW_GET_PRIVATE(object);
    switch (property_id) {
    case PROP_PARENT:
        priv->parent = g_value_get_object(value);
        break;
    case PROP_CLIENT:
        priv->client = g_value_get_pointer(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
        break;
    }
}

static void
trg_rss_window_response_cb(GtkDialog * dlg, gint res_id,
                             gpointer data G_GNUC_UNUSED)
{
    TrgRssWindowPrivate *priv =
        TRG_RSS_WINDOW_GET_PRIVATE(dlg);

    gtk_widget_destroy(GTK_WIDGET(dlg));
    instance = NULL;
}

static GObject *trg_rss_window_constructor(GType type,
                                                    guint
                                                    n_construct_properties,
                                                    GObjectConstructParam *
                                                    construct_params)
{
    GObject *object;
    TrgRssWindowPrivate *priv;
    TrgRssModel *model;
    GtkTreeView *view;
    GtkWidget *vbox, *toolbar, *contentbox;
    GtkToolItem *item;

    object = G_OBJECT_CLASS
        (trg_rss_window_parent_class)->constructor(type,
                                                            n_construct_properties,
                                                            construct_params);
    priv = TRG_RSS_WINDOW_GET_PRIVATE(object);

    model = trg_rss_model_new(priv->client);
    trg_rss_model_update(model);

    view = GTK_TREE_VIEW(gtk_tree_view_new());
    gtk_tree_view_set_model(view, GTK_TREE_MODEL(model));
    gtk_tree_view_insert_column_with_attributes(view, -1, "Title", gtk_cell_renderer_text_new(), "text", RSSCOL_TITLE, NULL);

    gtk_window_set_title(GTK_WINDOW(object), _("RSS Viewer"));

    //contentbox = gtk_dialog_get_content_area(GTK_DIALOG(object));

    toolbar = gtk_toolbar_new();

    item = gtk_tool_button_new_from_stock(GTK_STOCK_PREFERENCES);
    gtk_widget_set_sensitive(GTK_WIDGET(item), TRUE);
    gtk_tool_item_set_tooltip_text(item, "Configure");
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), item, 0);

    vbox = trg_vbox_new(FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(toolbar),
                           FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(view),
                           TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(object), vbox);

    /*g_signal_connect(object, "response",
                     G_CALLBACK(trg_rss_window_response_cb), NULL);*/

    return object;
}

static void trg_rss_window_dispose(GObject * object)
{
	instance = NULL;
    G_OBJECT_CLASS(trg_rss_window_parent_class)->dispose(object);
}

static void
trg_rss_window_class_init(TrgRssWindowClass * klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    g_type_class_add_private(klass, sizeof(TrgRssWindowPrivate));

    object_class->constructor = trg_rss_window_constructor;
    object_class->get_property = trg_rss_window_get_property;
    object_class->set_property = trg_rss_window_set_property;
    object_class->dispose = trg_rss_window_dispose;

    g_object_class_install_property(object_class,
                                    PROP_CLIENT,
                                    g_param_spec_pointer("trg-client",
                                                         "TClient",
                                                         "Client",
                                                         G_PARAM_READWRITE
                                                         |
                                                         G_PARAM_CONSTRUCT_ONLY
                                                         |
                                                         G_PARAM_STATIC_NAME
                                                         |
                                                         G_PARAM_STATIC_NICK
                                                         |
                                                         G_PARAM_STATIC_BLURB));

    g_object_class_install_property(object_class,
                                    PROP_PARENT,
                                    g_param_spec_object("parent-window",
                                                        "Parent window",
                                                        "Parent window",
                                                        TRG_TYPE_MAIN_WINDOW,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY
                                                        |
                                                        G_PARAM_STATIC_NAME
                                                        |
                                                        G_PARAM_STATIC_NICK
                                                        |
                                                        G_PARAM_STATIC_BLURB));
}

static void
trg_rss_window_init(TrgRssWindow * self G_GNUC_UNUSED)
{
}

TrgRssWindow *trg_rss_window_get_instance(TrgMainWindow *parent, TrgClient *client)
{
    if (instance == NULL) {
        instance =
            g_object_new(TRG_TYPE_RSS_WINDOW, "parent-window", parent, "trg-client", client, NULL);
    }

    return TRG_RSS_WINDOW(instance);
}
