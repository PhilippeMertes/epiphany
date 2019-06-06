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

  g_sequence_free (self->pvd_list);

  G_OBJECT_CLASS (ephy_pvd_manager_parent_class)->finalize (object);
}

static void
ephy_pvd_manager_class_init (EphyPvdManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = ephy_pvd_manager_finalize;
}

static void
ephy_pvd_manager_init (EphyPvdManager *self)
{
  EphyPvd *pvd;

  self->pvd_list = g_sequence_new (g_free);

  // collect Pvd names from pvdd
  t_pvd_connection *conn = pvd_connect (-1);
  t_pvd_list *pvd_list = g_malloc (sizeof (t_pvd_list));

  if (pvd_get_pvd_list_sync (conn, pvd_list)) {
    // error
    pvd_disconnect (conn);
    g_free (pvd_list);
    return;
  }

  // store pvd names into sequence
  for (int i = 0; i < pvd_list->npvd; ++i) {
    if (g_strcmp0 (pvd_list->pvdnames[i], "video.mpvd.io.") == 0)
      pvd = ephy_pvd_new ("Video-Stream PvD");
    else
      pvd = ephy_pvd_new (pvd_list->pvdnames[i]);
    g_sequence_append (self->pvd_list, pvd);
    g_free (pvd_list->pvdnames[i]);
  }

  pvd_disconnect(conn);
  g_free(pvd_list); //TODO: find out why there is a duplicate free
}

EphyPvdManager *
ephy_pvd_manager_new (void)
{
  return EPHY_PVD_MANAGER (g_object_new (EPHY_TYPE_PVD_MANAGER, NULL));
}

GSequence *
ephy_pvd_manager_get_pvd_list (EphyPvdManager *self)
{
  g_assert (EPHY_IS_PVD_MANAGER (self));

  return self->pvd_list;
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