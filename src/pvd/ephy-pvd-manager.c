/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 *  Copyright © 2019 Philippe Mertes <pmertes@student.uliege.be>
 *
 *  This file is part of Epiphany.
 *
 *  Epiphany is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Epiphany is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Epiphany.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"
#include "ephy-pvd-manager.h"

#include <libpvd.h>
#include <string.h>
#include <json-glib/json-glib.h>

//TODO: If no PvDs retrieved, show corresponding popover and don't search in the pvd_list sequence
// => provokes g_sequence_get: assertion '!is_end(iter)' failed error

struct _EphyPvdManager {
    GObject     parent_instance;

    GSequence  *pvd_list;
};

static void list_model_iface_init (GListModelInterface *iface);

// define class to be an instance of GListModel
G_DEFINE_TYPE_WITH_CODE (EphyPvdManager, ephy_pvd_manager, G_TYPE_OBJECT,
  G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, list_model_iface_init))

static void
ephy_pvd_manager_finalize (GObject *object)
{
  EphyPvdManager *self = EPHY_PVD_MANAGER (object);

  g_sequence_free (self->pvd_list); // TODO: free memory allocated by PvDs

  G_OBJECT_CLASS (ephy_pvd_manager_parent_class)->finalize (object);
}

static void
ephy_pvd_manager_class_init (EphyPvdManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ephy_pvd_manager_finalize;
}

static attribute_t *
create_attribute (JsonNode *node)
{
  const char *type = json_node_type_name (node);
  attribute_t *attr = g_malloc (sizeof(attribute_t));

  attr->type = strdup (type);

  if (strcmp (type, "Boolean") == 0)
    attr->val.b = json_node_get_boolean (node);
  else if (strcmp (type, "Integer") == 0)
    attr->val.i = json_node_get_int (node);
  else if (strcmp (type, "String") == 0)
    attr->val.str = json_node_dup_string (node);
  else {
    // unknown type => free memory and return NULL
    g_free (attr->type);
    g_free (attr);
    return NULL;
  }

  return attr;
}

static void
ephy_pvd_manager_retrieve_pvd_attributes (EphyPvd *pvd,
                                          t_pvd_connection *conn)
{
  const char *pvd_name = ephy_pvd_get_name (pvd);
  char *attributes;
  JsonParser *parser;
  JsonNode *root;
  JsonObject *object;
  JsonObjectIter iter;
  const char *type;
  const char *attribute_name;
  JsonNode *attribute_node;
  GError *error;
  attribute_t *attr;

  // retrieve PvD attributes from pvdd
  if (pvd_get_attributes_sync (conn, (char *) pvd_name, &attributes)) {
    return; // error
  }

  // create JSON data parser
  parser = json_parser_new ();
  error = NULL;
  json_parser_load_from_data (parser, attributes, strlen (attributes), &error);
  if (error) {
    g_free (&attributes);
    g_object_unref (parser);
    return; //TODO: add error message (logging)
  }

  // retrieve root node and check its type
  root = json_parser_get_root (parser);
  type = json_node_type_name (root);
  if (strcmp (type, "JsonObject")) {
    // not a JSON object => exit
    g_free (&attributes);
    g_object_unref (parser);
  }

  object = json_node_get_object (root);
  // iterate through the key-value pairs
  json_object_iter_init (&iter, object);
  while (json_object_iter_next (&iter, &attribute_name, &attribute_node)) {
    type = json_node_type_name (attribute_node);

    printf ("type = %s\n", type);

    attr = create_attribute (attribute_node);

    ephy_pvd_add_attribute (pvd, attribute_name, attr);
  }

  g_object_unref (parser);
}

static void
ephy_pvd_manager_init (EphyPvdManager *self)
{
  EphyPvd *pvd;

  self->pvd_list = g_sequence_new (g_object_unref);

  // collect PvD names from pvdd
  t_pvd_connection *conn = pvd_connect (-1);
  t_pvd_list *pvd_list = g_malloc (sizeof (t_pvd_list));

  if (pvd_get_pvd_list_sync (conn, pvd_list)) {
    // error
    pvd_disconnect (conn);
    g_free (pvd_list);
    return;
  }

  // store PvDs in sequence
  for (int i = 0; i < pvd_list->npvd; ++i) {
    pvd = ephy_pvd_new (strdup (pvd_list->pvdnames[i]));
    ephy_pvd_manager_retrieve_pvd_attributes(pvd, conn);
    g_sequence_append (self->pvd_list, pvd);
    g_free (pvd_list->pvdnames[i]);
  }

  pvd_disconnect (conn);
  g_free(pvd_list);
}

EphyPvdManager *
ephy_pvd_manager_new (void)
{
  return EPHY_PVD_MANAGER (g_object_new (EPHY_TYPE_PVD_MANAGER, NULL));
}

/*GSequence *
ephy_pvd_manager_get_pvd_list (EphyPvdManager *self)
{
  g_assert (EPHY_IS_PVD_MANAGER (self));

  return self->pvd_list;
}*/

static gint
compare_pvd_names (gconstpointer pvd_ptr,
                   gconstpointer name_ptr,
                   gpointer user_data)
{
  // cast void pointers to arguments expected types
  EphyPvd *pvd = EPHY_PVD ((void *) pvd_ptr);
  const char *name = (const char *) name_ptr;
  g_assert (EPHY_IS_PVD (pvd));
  const char *pvd_name = ephy_pvd_get_name (pvd);

  printf ("name = %s, pvd_name = %s\n", name, pvd_name);

  return strcmp (name, pvd_name);
}

EphyPvd *
ephy_pvd_manager_get_pvd (EphyPvdManager *self,
                          const char *pvd_name)
{
  g_assert (EPHY_IS_PVD_MANAGER (self));

  GSequenceIter *iter = g_sequence_lookup (self->pvd_list, (char *) pvd_name, compare_pvd_names, NULL);
  return (EphyPvd *) g_sequence_get (iter);
}



static GType
ephy_pvd_manager_list_model_get_item_type (GListModel *model)
{
  return EPHY_TYPE_PVD;
}

static guint
ephy_pvd_manager_list_model_get_n_items (GListModel *model)
{
  EphyPvdManager *self = EPHY_PVD_MANAGER (model);

  return g_sequence_get_length (self->pvd_list);
}

static gpointer
ephy_pvd_manager_list_model_get_item (GListModel *model,
                                      guint       position)
{
  EphyPvdManager *self = EPHY_PVD_MANAGER (model);
  GSequenceIter *iter;

  iter = g_sequence_get_iter_at_pos (self->pvd_list, position);

  return g_object_ref (g_sequence_get (iter));
}

static void
list_model_iface_init (GListModelInterface *iface)
{
  iface->get_item_type = ephy_pvd_manager_list_model_get_item_type;
  iface->get_n_items = ephy_pvd_manager_list_model_get_n_items;
  iface->get_item = ephy_pvd_manager_list_model_get_item;
}