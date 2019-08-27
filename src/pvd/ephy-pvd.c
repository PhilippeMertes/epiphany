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
#include "ephy-pvd.h"

struct _EphyPvd {
    GObject parent_instance;

    char *name;
    GHashTable *attributes;
    GHashTable *extra_attributes;
    JsonParser *parser;
};

G_DEFINE_TYPE (EphyPvd, ephy_pvd, G_TYPE_OBJECT)

static void ephy_pvd_set_extra_attributes (EphyPvd *self,
                                           JsonNode *node);
static gboolean ephy_pvd_set_attribute (EphyPvd *self,
                                        const char *name,
                                        JsonNode *value);

enum {
    PROP_0,
    PROP_NAME,
    LAST_PROP
};

static GParamSpec *obj_properties[LAST_PROP];

static void
ephy_pvd_set_property (GObject      *object,
                       guint         prop_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  EphyPvd *self = EPHY_PVD (object);

  switch (prop_id) {
    case PROP_NAME:
      ephy_pvd_set_name (self, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
ephy_pvd_get_property (GObject    *object,
                       guint       prop_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  EphyPvd *self = EPHY_PVD (object);

  switch (prop_id) {
    case PROP_NAME:
      g_value_set_string (value, ephy_pvd_get_name (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
ephy_pvd_finalize (GObject *object)
{
  EphyPvd *self = EPHY_PVD (object);

  g_free (self->name);

  g_object_unref (self->parser);

  g_hash_table_destroy (self->attributes);

  g_hash_table_destroy (self->extra_attributes);

  G_OBJECT_CLASS (ephy_pvd_parent_class)->finalize (object);
}

static void
ephy_pvd_class_init (EphyPvdClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ephy_pvd_set_property;
  object_class->get_property = ephy_pvd_get_property;
  object_class->finalize = ephy_pvd_finalize;

  obj_properties[PROP_NAME] =
    g_param_spec_string ("name",
      "Name",
      "The PvD's name",
      "Default PvD name",
      G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, obj_properties);
}

static void
ephy_pvd_init (EphyPvd *self)
{
  self->name = NULL;
  self->attributes = g_hash_table_new (g_str_hash, g_str_equal);
  self->extra_attributes = g_hash_table_new (g_str_hash, g_str_equal);
  self->parser = json_parser_new ();
}

EphyPvd *
ephy_pvd_new (const char *name)
{
  return g_object_new (EPHY_TYPE_PVD,
                       "name", name,
                       NULL);
}

/**
 * ephy_pvd_get_name:
 * @self: an #EphyPvd
 *
 * Returns the name (FQDN) of the PvD.
 *
 * Return value: a constant string
 **/
const char *
ephy_pvd_get_name (EphyPvd *self)
{
  g_assert (EPHY_IS_PVD (self));

  return self->name;
}

/**
 * ephy_pvd_set_name:
 * @self: an #EphyPvd
 * @name: constant string FQDN
 *
 * Sets the name (FQDN) of the PvD.
 **/
void
ephy_pvd_set_name (EphyPvd* self, const char *name)
{
  g_assert (EPHY_IS_PVD (self));

  g_free (self->name);
  self->name = g_strdup (name);
  g_object_notify_by_pspec (G_OBJECT (self), obj_properties[PROP_NAME]);
}

/**
 * ephy_pvd_get_attributes:
 * @self: an #EphyPvd
 *
 * Returns the attributes of the PvD (excluding extra attributes).
 *
 * Return value: hash table containing <name(string)<->value(JsonNode)> pairs
 **/
GHashTable *
ephy_pvd_get_attributes (EphyPvd *self)
{
  g_assert (EPHY_IS_PVD (self));

  return self->attributes;
}

/**
 * ephy_pvd_set_extra_attribute:
 * @self: an #EphyPvd
 * @name: constant string
 * @value: a JsonNode
 *
 * Adds or replaces an extra attribute to the #EphyPvd object.
 *
 * Return value: Boolean, which takes the value
 *               TRUE: if a new key has been inserted
 *               FALSE: if a key's value has been replaced.
 **/
static gboolean
ephy_pvd_set_extra_attribute (EphyPvd *self,
                              const char *name,
                              JsonNode *value)
{
  return g_hash_table_insert (self->extra_attributes, (char *)name, (gpointer)value);
}

/**
 * ephy_pvd_set_attributes:
 * @self: an #EphyPvd
 * @json_str: constant JSON string
 *
 * Sets the attributes of the #EphyPvd object from a JSON string
 * (including extra attributes).
 *
 * Return value: Boolean indicating if the string could be
 *               well parsed (TRUE) or not (FALSE).
 **/
gboolean
ephy_pvd_set_attributes (EphyPvd *self,
                         const char *json_str)
{
  JsonNode *root;
  JsonObject *object;
  JsonObjectIter iter;
  const char *type;
  const char *attribute_name;
  JsonNode *attribute_node;
  GError *error;

  g_assert (EPHY_IS_PVD (self));

  // if the attributes table contains already some values, destroy the table and create a new one
  if (g_hash_table_size (self->attributes)) {
    g_hash_table_destroy (self->attributes);
    self->attributes = g_hash_table_new (g_str_hash, g_str_equal);
  }

  // configure JSON parser
  error = NULL;
  json_parser_load_from_data (self->parser, json_str, strlen (json_str), &error);
  if (error) {
    g_warning ("Error while parsing JSON string: %s\nError message: %s",
               json_str, error->message);
    return FALSE;
  }

  // retrieve root node and check its type
  root = json_parser_get_root (self->parser);
  type = json_node_type_name (root);
  if (strcmp (type, "JsonObject")) {
    g_warning ("Error while parsing JSON string: %s\n"
               "The root node is not a JsonObject.\n", json_str);
    return FALSE;
  }

  object = json_node_get_object (root);
  // iterate through the key-value pairs
  json_object_iter_init (&iter, object);
  while (json_object_iter_next (&iter, &attribute_name, &attribute_node)) {
    if (strcmp (attribute_name, "extraInfo") == 0)
      ephy_pvd_set_extra_attributes (self, attribute_node);
    else
      ephy_pvd_set_attribute (self, attribute_name, attribute_node);
  }

  return TRUE;
}

/**
 * ephy_pvd_set_extra_attributes:
 * @self: an #EphyPvd
 * @noode: a JsonNode
 *
 * Sets the extra attributes of the #EphyPvd object from a JSON node.
 **/
static void
ephy_pvd_set_extra_attributes (EphyPvd *self,
                               JsonNode *node)
{
  const char *type;
  JsonObject *object;
  JsonObjectIter iter;
  const char *attribute_name;
  JsonNode *attribute_node;

  g_assert (EPHY_IS_PVD (self));

  type = json_node_type_name (node);

  if (strcmp (type, "JsonObject")) {
    g_warning ("The extra information is not a \"JsonObject\" type, as it should.\n"
               "Thus it won't be added to the EphyPvd object \"%s\".", self->name);
    return;
  }

  object = json_node_get_object (node);

  // iterate through the key-value pairs
  json_object_iter_init (&iter, object);
  while (json_object_iter_next (&iter, &attribute_name, &attribute_node)) {
    ephy_pvd_set_extra_attribute (self, attribute_name, attribute_node);
  }
}

/**
 * ephy_pvd_get_attribute:
 * @self: an #EphyPvd
 * @name: constant string
 *
 * Returns the value corresponding to the specified attribute.
 *
 * Return value: JsonNode object
 **/
JsonNode *
ephy_pvd_get_attribute (EphyPvd *self,
                        const char *name)
{
  return (JsonNode *) g_hash_table_lookup (self->attributes, name);
}

/**
 * ephy_pvd_set_attribute:
 * @self: an #EphyPvd
 * @name: constant string
 * @value: a JsonNode
 *
 * Adds or replaces a (non-extra) attribute to the #EphyPvd object.
 *
 * Return value: Boolean, which takes the value
 *               TRUE: if a new key has been inserted
 *               FALSE: if a key's value has been replaced or the key is forbidden.
 **/
static gboolean
ephy_pvd_set_attribute (EphyPvd *self,
                        const char *name,
                        JsonNode *value)
{
  if (g_strcmp0 (name, "extraInfo") == 0) {
    g_warning ("Trying to set a normal attribute with the identifier of the "
               "extra attribute, i.e. \"extraInfo\"\nThis is not permitted.");
    return FALSE;
  }

  return g_hash_table_insert (self->attributes, (char *)name, (gpointer)value);
}

/**
 * ephy_pvd_has_extra_attributes:
 * @self: an #EphyPvd
 *
 * Indicates whether or not extra attributes are known for the PvD.
 *
 * Return value: Boolean (TRUE, if extra attributes present)
 */
gboolean
ephy_pvd_has_extra_attributes (EphyPvd *self)
{
  return g_hash_table_size (self->extra_attributes) > 0;
}

/**
 * ephy_pvd_get_extra_attributes:
 * @self: an #EphyPvd
 *
 * Returns the extra attributes of the PvD
 *
 * Return value: hash table containing <name(string)<->value(JsonNode)> pairs
 */
GHashTable *
ephy_pvd_get_extra_attributes (EphyPvd *self)
{
  g_assert (EPHY_IS_PVD (self));

  return self->extra_attributes;
}

/**
 * ephy_pvd_get_extra_attribute_name:
 * @self: an #EphyPvd
 *
 * Returns the value of the "name" extra attribute.
 *
 * Return value: constant string
 */
const char *
ephy_pvd_get_extra_attribute_name (EphyPvd *self)
{
  JsonNode *name_node;

  g_assert (EPHY_IS_PVD (self));

  name_node = (JsonNode *)g_hash_table_lookup (self->extra_attributes, "name");

  return g_strcmp0 (json_node_type_name (name_node), "String") == 0 ?
         json_node_get_string (name_node) : NULL;
}
