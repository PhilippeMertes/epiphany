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

#include "ephy-pvd-popover.h"

#include "ephy-pvd.h"
#include "ephy-pvd-row.h"
#include "ephy-pvd-manager.h"
#include "ephy-debug.h"
#include "ephy-shell.h"
#include "ephy-window.h"

#include <glib/gi18n.h>

struct _EphyPvdPopover {
    GtkPopover             parent_instance;

    GtkWidget             *toplevel_stack;
    GtkWidget             *pvd_list_box;

    EphyPvdManager        *manager;
};

G_DEFINE_TYPE (EphyPvdPopover, ephy_pvd_popover, GTK_TYPE_POPOVER)

static void
ephy_pvd_popover_list_box_row_activated_cb (EphyPvdPopover *self,
                                            GtkListBoxRow *row,
                                            GtkListBox *box)
{
  GtkWidget *window;
  const char *pvd_name;
  EphyPvd *pvd;
  GActionGroup *action_group;
  GAction *action;

  g_assert (EPHY_IS_PVD_POPOVER (self));
  g_assert (EPHY_IS_PVD_ROW (row));
  g_assert (GTK_IS_LIST_BOX (box));

  // get row's PvD
  pvd = ephy_pvd_row_get_pvd ((EphyPvdRow *) row);
  pvd_name = ephy_pvd_get_name (pvd);

  // get pvd attributes dialog opening action
  window = gtk_widget_get_ancestor (GTK_WIDGET (self), EPHY_TYPE_WINDOW);
  g_assert (EPHY_IS_WINDOW (window));
  action_group = gtk_widget_get_action_group (window, "popup");
  g_assert (action_group != NULL);
  action = g_action_map_lookup_action (G_ACTION_MAP (action_group), "pvd-attributes");
  g_assert (action != NULL);

  // open dialog
  g_action_activate (action, g_variant_new_string (pvd_name));
}

static void
ephy_pvd_popover_finalize (GObject *object)
{
  G_OBJECT_CLASS (ephy_pvd_popover_parent_class)->finalize (object);
}

static void
ephy_pvd_popover_class_init (EphyPvdPopoverClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = ephy_pvd_popover_finalize;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/epiphany/gtk/pvd-popover.ui");
  gtk_widget_class_bind_template_child (widget_class, EphyPvdPopover, toplevel_stack);
  gtk_widget_class_bind_template_child (widget_class, EphyPvdPopover, pvd_list_box);
}

static const GActionEntry entries[] = {
        //{ "tag-detail-back", ephy_pvd_popover_actions_tag_detail_back }
};

static void
ephy_pvd_popover_init (EphyPvdPopover *self)
{
  GSimpleActionGroup *group;

  gtk_widget_init_template (GTK_WIDGET (self));

  self->manager = ephy_shell_get_pvd_manager (ephy_shell_get_default ());

  group = g_simple_action_group_new ();
  g_action_map_add_action_entries (G_ACTION_MAP (group), entries,
                                   G_N_ELEMENTS (entries), self);
  gtk_widget_insert_action_group (GTK_WIDGET (self), "popover",
                                  G_ACTION_GROUP (group));
  g_object_unref (group);

  gtk_list_box_bind_model (GTK_LIST_BOX (self->pvd_list_box),
                           G_LIST_MODEL (self->manager),
                           ephy_pvd_row_create,
                           self, NULL);

  if (g_list_model_get_n_items (G_LIST_MODEL (self->manager)) == 0)
    gtk_stack_set_visible_child_name (GTK_STACK (self->toplevel_stack), "empty-state");

  g_signal_connect_object (self->pvd_list_box, "row-activated",
                           G_CALLBACK (ephy_pvd_popover_list_box_row_activated_cb),
                           self, G_CONNECT_SWAPPED);
}

EphyPvdPopover *
ephy_pvd_popover_new (void)
{
  return g_object_new (EPHY_TYPE_PVD_POPOVER,
                       NULL);
}
