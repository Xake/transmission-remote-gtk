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
#include <json-glib/json-glib.h>
#include <rss-glib/rss-glib.h>

#include "config.h"
#include "torrent.h"
#include "trg-client.h"
#include "trg-model.h"
#include "trg-rss-model.h"

enum {
	PROP_0, PROP_CLIENT
};

G_DEFINE_TYPE(TrgRssModel, trg_rss_model, GTK_TYPE_LIST_STORE)
#define TRG_RSS_MODEL_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), TRG_TYPE_RSS_MODEL, TrgRssModelPrivate))
typedef struct _TrgRssModelPrivate TrgRssModelPrivate;

struct _TrgRssModelPrivate {
	TrgClient *client;
	GHashTable *table;
};

typedef struct {
	TrgRssModel *model;
	gchar *feed_id;
	gchar *feed_url;
	gint status;
	RssParser *parser;
	gboolean success;
	GError *error;
} feed_update;

static void feed_update_free(feed_update *update) {
	if (update->error)
		g_error_free(update->error);

	g_free(update->feed_id);
	g_free(update->feed_url);

	if (update->parser)
		g_object_unref(update->parser);

	g_free(update);
}

static gboolean on_rss_receive_idle(gpointer data) {
	feed_update *update = (feed_update*) data;
	TrgRssModel *model = update->model;
	TrgRssModelPrivate *priv = TRG_RSS_MODEL_GET_PRIVATE(model);

	if (update->success) {
		RssDocument *doc = rss_parser_get_document(update->parser);
		GtkTreeIter iter;
		GList *list, *tmp;
		gchar *title;

		list = rss_document_get_items(doc);

		for (tmp = list; tmp != NULL; tmp = tmp->next) {
			RssItem *item = (RssItem*) tmp->data;
			const gchar *guid = rss_item_get_guid(item);
			if (g_hash_table_lookup(priv->table, guid) != (void*) 1) {
				gtk_list_store_append(GTK_LIST_STORE(model), &iter);
				gtk_list_store_set(GTK_LIST_STORE(model), &iter, RSSCOL_ID,
						guid, RSSCOL_TITLE, rss_item_get_title(item),
						RSSCOL_LINK, rss_item_get_link(item), -1);
				g_hash_table_insert(priv->table, g_strdup(guid), (void*) 1);
			}
		}

		g_list_free(list);
		g_object_unref(doc);
	}

	feed_update_free(update);

	return FALSE;
}

static gboolean on_rss_receive(gpointer data) {
	trg_response *response = (trg_response *) data;
	feed_update *update = (feed_update*) response->cb_data;

	update->status = response->status;

	if (response->status == CURLE_OK) {
		update->parser = rss_parser_new();
		update->success = rss_parser_load_from_data(update->parser,
				response->raw, response->size, &(update->error));
	}

	g_idle_add(on_rss_receive_idle, update);

	trg_response_free(response);

	return FALSE;
}

void trg_rss_model_update(TrgRssModel * model) {
	TrgRssModelPrivate *priv = TRG_RSS_MODEL_GET_PRIVATE(model);
	TrgPrefs *prefs = trg_client_get_prefs(priv->client);
	JsonArray *feeds = trg_prefs_get_rss(prefs);
	GList *li;

	if (!feeds)
		return;

	for (li = json_array_get_elements(feeds); li != NULL; li = g_list_next(li)) {
		JsonObject *feed = json_node_get_object((JsonNode *) li->data);
		const gchar *url = json_object_get_string_member(feed, "url");
		const gchar *id = json_object_get_string_member(feed, "id");

		if (!url || !id)
			continue;

		feed_update *update = g_new0(feed_update, 1);
		update->feed_url = g_strdup(url);
		update->feed_id = g_strdup(id);
		update->model = model;

		async_http_request(priv->client, update->feed_url, on_rss_receive,
				update);
	}

	/*trg_model_remove_removed(GTK_LIST_STORE(model),
	 RSSCOL_UPDATESERIAL, updateSerial);*/
}

static void trg_rss_model_set_property(GObject * object, guint prop_id,
		const GValue * value, GParamSpec * pspec G_GNUC_UNUSED) {
	TrgRssModelPrivate *priv = TRG_RSS_MODEL_GET_PRIVATE(object);

	switch (prop_id) {
	case PROP_CLIENT:
		priv->client = g_value_get_pointer(value);
		break;
	}
}

static void trg_rss_model_get_property(GObject * object, guint prop_id,
		GValue * value, GParamSpec * pspec G_GNUC_UNUSED) {
}

static GObject *trg_rss_model_constructor(GType type,
		guint n_construct_properties, GObjectConstructParam * construct_params) {
	GObject *obj = G_OBJECT_CLASS
	(trg_rss_model_parent_class)->constructor(type,
			n_construct_properties, construct_params);
	TrgRssModelPrivate *priv = TRG_RSS_MODEL_GET_PRIVATE(obj);

	priv->table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	return obj;
}

static void trg_rss_model_dispose(GObject * object)
{
	TrgRssModelPrivate *priv = TRG_RSS_MODEL_GET_PRIVATE(object);
	g_hash_table_destroy(priv->table);
    G_OBJECT_CLASS(trg_rss_model_parent_class)->dispose(object);
}

static void trg_rss_model_class_init(TrgRssModelClass * klass) {
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	g_type_class_add_private(klass, sizeof(TrgRssModelPrivate));

	object_class->set_property = trg_rss_model_set_property;
	object_class->get_property = trg_rss_model_get_property;
	object_class->constructor = trg_rss_model_constructor;
	object_class->dispose = trg_rss_model_dispose;

	g_object_class_install_property(object_class, PROP_CLIENT,
			g_param_spec_pointer("client", "client", "client",
					G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY
							| G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK
							| G_PARAM_STATIC_BLURB));
}

static void trg_rss_model_init(TrgRssModel * self) {
	//TrgRssModelPrivate *priv = TRG_RSS_MODEL_GET_PRIVATE(self);
	GType column_types[RSSCOL_COLUMNS];

	column_types[RSSCOL_ID] = G_TYPE_STRING;
	column_types[RSSCOL_TITLE] = G_TYPE_STRING;
	column_types[RSSCOL_LINK] = G_TYPE_STRING;

	gtk_list_store_set_column_types(GTK_LIST_STORE(self), RSSCOL_COLUMNS,
			column_types);
}

TrgRssModel *trg_rss_model_new(TrgClient *client) {
	return g_object_new(TRG_TYPE_RSS_MODEL, "client", client, NULL);
}
