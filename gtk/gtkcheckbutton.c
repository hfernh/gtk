/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#include "config.h"

#include "gtkactionhelperprivate.h"
#include "gtkboxlayout.h"
#include "gtkbuiltiniconprivate.h"
#include "gtkcheckbuttonprivate.h"
#include "gtkcssnodeprivate.h"
#include "gtkcssnumbervalueprivate.h"
#include "gtkgestureclick.h"
#include "gtkintl.h"
#include "gtklabel.h"
#include "gtkprivate.h"
#include "gtkradiobutton.h"
#include "gtkstylecontextprivate.h"
#include "gtkwidgetprivate.h"

/**
 * SECTION:gtkcheckbutton
 * @Short_description: Create widgets with a discrete toggle button
 * @Title: GtkCheckButton
 * @See_also: #GtkButton, #GtkToggleButton, #GtkRadioButton
 *
 * A #GtkCheckButton places a discrete #GtkToggleButton next to a widget,
 * (usually a #GtkLabel). See the section on #GtkToggleButton widgets for
 * more information about toggle/check buttons.
 *
 * The important signal ( #GtkToggleButton::toggled ) is also inherited from
 * #GtkToggleButton.
 *
 * # CSS nodes
 *
 * |[<!-- language="plain" -->
 * checkbutton
 * ├── check
 * ╰── <child>
 * ]|
 *
 * A GtkCheckButton with indicator (see gtk_check_button_set_draw_indicator()) has a
 * main CSS node with name checkbutton and a subnode with name check.
 *
 * # Accessibility
 *
 * GtkCheckButton uses the #GTK_ACCESSIBLE_ROLE_CHECKBOX role.
 */

typedef struct {
  GtkWidget *indicator_widget;
  GtkWidget *label_widget;

  guint draw_indicator: 1;
  guint inconsistent: 1;
  guint active: 1;
  guint use_underline: 1;

  GtkCheckButton *group_next;
  GtkCheckButton *group_prev;

  GtkActionHelper *action_helper;
} GtkCheckButtonPrivate;

enum {
  PROP_0,
  PROP_ACTIVE,
  PROP_GROUP,
  PROP_LABEL,
  PROP_DRAW_INDICATOR,
  PROP_INCONSISTENT,
  PROP_USE_UNDERLINE,

  /* actionable properties */
  PROP_ACTION_NAME,
  PROP_ACTION_TARGET,
  LAST_PROP = PROP_ACTION_NAME
};

enum {
  TOGGLED,
  LAST_SIGNAL
};

static void gtk_check_button_actionable_iface_init (GtkActionableInterface *iface);

static guint signals[LAST_SIGNAL] = { 0 };
static GParamSpec *props[LAST_PROP] = { NULL, };

G_DEFINE_TYPE_WITH_CODE (GtkCheckButton, gtk_check_button, GTK_TYPE_WIDGET,
                         G_ADD_PRIVATE (GtkCheckButton)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_ACTIONABLE, gtk_check_button_actionable_iface_init))

static void
gtk_check_button_dispose (GObject *object)
{
  GtkCheckButtonPrivate *priv = gtk_check_button_get_instance_private (GTK_CHECK_BUTTON (object));

  g_clear_pointer (&priv->indicator_widget, gtk_widget_unparent);
  g_clear_pointer (&priv->label_widget, gtk_widget_unparent);

  gtk_check_button_set_group (GTK_CHECK_BUTTON (object), NULL);

  G_OBJECT_CLASS (gtk_check_button_parent_class)->dispose (object);
}

static void
gtk_check_button_activate_action (GtkCheckButton *self)
{
  GtkCheckButtonPrivate *priv = gtk_check_button_get_instance_private (self);

  if (priv->action_helper)
    gtk_action_helper_activate (priv->action_helper);
}

static void
gtk_check_button_set_action_name (GtkActionable *actionable,
                                  const char    *action_name)
{
  GtkCheckButton *self = GTK_CHECK_BUTTON (actionable);
  GtkCheckButtonPrivate *priv = gtk_check_button_get_instance_private (self);

  if (!priv->action_helper)
    priv->action_helper = gtk_action_helper_new (actionable);

  gtk_action_helper_set_action_name (priv->action_helper, action_name);
}

static void
gtk_check_button_set_action_target_value (GtkActionable *actionable,
                                          GVariant      *action_target)
{
  GtkCheckButton *self = GTK_CHECK_BUTTON (actionable);
  GtkCheckButtonPrivate *priv = gtk_check_button_get_instance_private (self);

  if (!priv->action_helper)
    priv->action_helper = gtk_action_helper_new (actionable);

  gtk_action_helper_set_action_target_value (priv->action_helper, action_target);
}

static void
gtk_check_button_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  switch (prop_id)
    {
    case PROP_ACTIVE:
      gtk_check_button_set_active (GTK_CHECK_BUTTON (object), g_value_get_boolean (value));
      break;
    case PROP_GROUP:
      gtk_check_button_set_group (GTK_CHECK_BUTTON (object), g_value_get_object (value));
      break;
    case PROP_LABEL:
      gtk_check_button_set_label (GTK_CHECK_BUTTON (object), g_value_get_string (value));
      break;
    case PROP_DRAW_INDICATOR:
      gtk_check_button_set_draw_indicator (GTK_CHECK_BUTTON (object), g_value_get_boolean (value));
      break;
    case PROP_INCONSISTENT:
      gtk_check_button_set_inconsistent (GTK_CHECK_BUTTON (object), g_value_get_boolean (value));
      break;
    case PROP_USE_UNDERLINE:
      gtk_check_button_set_use_underline (GTK_CHECK_BUTTON (object), g_value_get_boolean (value));
      break;
    case PROP_ACTION_NAME:
      gtk_check_button_set_action_name (GTK_ACTIONABLE (object), g_value_get_string (value));
      break;
    case PROP_ACTION_TARGET:
      gtk_check_button_set_action_target_value (GTK_ACTIONABLE (object), g_value_get_variant (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk_check_button_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GtkCheckButtonPrivate *priv = gtk_check_button_get_instance_private (GTK_CHECK_BUTTON (object));

  switch (prop_id)
    {
    case PROP_ACTIVE:
      g_value_set_boolean (value, gtk_check_button_get_active (GTK_CHECK_BUTTON (object)));
      break;
    case PROP_LABEL:
      g_value_set_string (value, gtk_check_button_get_label (GTK_CHECK_BUTTON (object)));
      break;
    case PROP_DRAW_INDICATOR:
      g_value_set_boolean (value, gtk_check_button_get_draw_indicator (GTK_CHECK_BUTTON (object)));
      break;
    case PROP_INCONSISTENT:
      g_value_set_boolean (value, gtk_check_button_get_inconsistent (GTK_CHECK_BUTTON (object)));
      break;
    case PROP_USE_UNDERLINE:
      g_value_set_boolean (value, gtk_check_button_get_use_underline (GTK_CHECK_BUTTON (object)));
      break;
    case PROP_ACTION_NAME:
      g_value_set_string (value, gtk_action_helper_get_action_name (priv->action_helper));
      break;
    case PROP_ACTION_TARGET:
      g_value_set_variant (value, gtk_action_helper_get_action_target_value (priv->action_helper));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static const char *
gtk_check_button_get_action_name (GtkActionable *actionable)
{
  GtkCheckButton *self = GTK_CHECK_BUTTON (actionable);
  GtkCheckButtonPrivate *priv = gtk_check_button_get_instance_private (self);

  return gtk_action_helper_get_action_name (priv->action_helper);
}

static GVariant *
gtk_check_button_get_action_target_value (GtkActionable *actionable)
{
  GtkCheckButton *self = GTK_CHECK_BUTTON (actionable);
  GtkCheckButtonPrivate *priv = gtk_check_button_get_instance_private (self);

  return gtk_action_helper_get_action_target_value (priv->action_helper);
}

static void
gtk_check_button_actionable_iface_init (GtkActionableInterface *iface)
{
  iface->get_action_name = gtk_check_button_get_action_name;
  iface->set_action_name = gtk_check_button_set_action_name;
  iface->get_action_target_value = gtk_check_button_get_action_target_value;
  iface->set_action_target_value = gtk_check_button_set_action_target_value;
}

static void
click_pressed_cb (GtkGestureClick *gesture,
                  guint            n_press,
                  double           x,
                  double           y,
                  GtkWidget       *widget)
{
  if (gtk_widget_get_focus_on_click (widget) && !gtk_widget_has_focus (widget))
    gtk_widget_grab_focus (widget);

  gtk_gesture_set_state (GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_CLAIMED);
}

static void
click_released_cb (GtkGestureClick *gesture,
                   guint            n_press,
                   double           x,
                   double           y,
                   GtkWidget       *widget)
{
  GtkCheckButton *self = GTK_CHECK_BUTTON (widget);
  GtkCheckButtonPrivate *priv = gtk_check_button_get_instance_private (self);

  gtk_check_button_set_active (self, !priv->active);

  g_message (__FUNCTION__);

  if (priv->action_helper)
    gtk_action_helper_activate (priv->action_helper);
}

static void
update_accessible_state (GtkCheckButton *check_button)
{
  GtkCheckButtonPrivate *priv = gtk_check_button_get_instance_private (check_button);

  GtkAccessibleTristate checked_state;

  if (priv->inconsistent)
    checked_state = GTK_ACCESSIBLE_TRISTATE_MIXED;
  else if (priv->active)
    checked_state = GTK_ACCESSIBLE_TRISTATE_TRUE;
  else
    checked_state = GTK_ACCESSIBLE_TRISTATE_FALSE;

  gtk_accessible_update_state (GTK_ACCESSIBLE (check_button),
                               GTK_ACCESSIBLE_STATE_CHECKED, checked_state,
                               -1);
}

static void
gtk_check_button_class_init (GtkCheckButtonClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  object_class->dispose = gtk_check_button_dispose;
  object_class->set_property = gtk_check_button_set_property;
  object_class->get_property = gtk_check_button_get_property;

  props[PROP_ACTIVE] =
      g_param_spec_boolean ("active",
                            P_("Active"),
                            P_("If the toggle button should be pressed in"),
                            FALSE,
                            GTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);
  props[PROP_GROUP] =
      g_param_spec_object ("group",
                           P_("Group"),
                           P_("The check button whose group this widget belongs to."),
                           GTK_TYPE_CHECK_BUTTON,
                           GTK_PARAM_WRITABLE);
  props[PROP_LABEL] =
    g_param_spec_string ("label",
                         P_("Label"),
                         P_("Text of the label widget inside the button, if the button contains a label widget"),
                         NULL,
                         GTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_DRAW_INDICATOR] =
      g_param_spec_boolean ("draw-indicator",
                            P_("Draw Indicator"),
                            P_("If the indicator part of the button is displayed"),
                            TRUE,
                            GTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_INCONSISTENT] =
      g_param_spec_boolean ("inconsistent",
                            P_("Inconsistent"),
                            P_("If the check button is in an “in between” state"),
                            FALSE,
                            GTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  props[PROP_USE_UNDERLINE] =
      g_param_spec_boolean ("use-underline",
                            P_("Use underline"),
                            P_("If set, an underline in the text indicates the next character should be used for the mnemonic accelerator key"),
                            FALSE,
                            GTK_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  g_object_class_override_property (object_class, PROP_ACTION_NAME, "action-name");
  g_object_class_override_property (object_class, PROP_ACTION_TARGET, "action-target");


  /**
   * GtkCheckButton::toggled:
   */
  signals[TOGGLED] =
    g_signal_new (I_("toggled"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GtkCheckButtonClass, toggled),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BOX_LAYOUT);
  gtk_widget_class_set_css_name (widget_class, I_("checkbutton"));
  gtk_widget_class_set_accessible_role (widget_class, GTK_ACCESSIBLE_ROLE_CHECKBOX);
}

static void
gtk_check_button_init (GtkCheckButton *check_button)
{
  GtkCheckButtonPrivate *priv = gtk_check_button_get_instance_private (check_button);
  GtkGesture *gesture;

  gtk_widget_set_receives_default (GTK_WIDGET (check_button), FALSE);
  priv->draw_indicator = TRUE;
  priv->indicator_widget = gtk_builtin_icon_new ("check");
  gtk_widget_set_halign (priv->indicator_widget, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (priv->indicator_widget, GTK_ALIGN_CENTER);
  gtk_widget_set_parent (priv->indicator_widget, GTK_WIDGET (check_button));

  priv->label_widget = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (priv->label_widget), 0.0f);
  gtk_widget_set_parent (priv->label_widget, GTK_WIDGET (check_button));
  gtk_widget_set_hexpand (priv->label_widget, TRUE);

  update_accessible_state (check_button);

  gesture = gtk_gesture_click_new ();
  gtk_gesture_single_set_touch_only (GTK_GESTURE_SINGLE (gesture), FALSE);
  gtk_gesture_single_set_exclusive (GTK_GESTURE_SINGLE (gesture), TRUE);
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (gesture), GDK_BUTTON_PRIMARY);
  g_signal_connect (gesture, "pressed", G_CALLBACK (click_pressed_cb), check_button);
  g_signal_connect (gesture, "released", G_CALLBACK (click_released_cb), check_button);
  gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (gesture), GTK_PHASE_CAPTURE);
  gtk_widget_add_controller (GTK_WIDGET (check_button), GTK_EVENT_CONTROLLER (gesture));
}

/**
 * gtk_check_button_new:
 *
 * Creates a new #GtkCheckButton.
 *
 * Returns: a #GtkWidget.
 */
GtkWidget*
gtk_check_button_new (void)
{
  return g_object_new (GTK_TYPE_CHECK_BUTTON, NULL);
}


/**
 * gtk_check_button_new_with_label:
 * @label: the text for the check button.
 *
 * Creates a new #GtkCheckButton with a #GtkLabel to the right of it.
 *
 * Returns: a #GtkWidget.
 */
GtkWidget*
gtk_check_button_new_with_label (const char *label)
{
  return g_object_new (GTK_TYPE_CHECK_BUTTON, "label", label, NULL);
}

/**
 * gtk_check_button_new_with_mnemonic:
 * @label: The text of the button, with an underscore in front of the
 *   mnemonic character
 *
 * Creates a new #GtkCheckButton containing a label. The label
 * will be created using gtk_label_new_with_mnemonic(), so underscores
 * in @label indicate the mnemonic for the check button.
 *
 * Returns: a new #GtkCheckButton
 */
GtkWidget*
gtk_check_button_new_with_mnemonic (const char *label)
{
  return g_object_new (GTK_TYPE_CHECK_BUTTON,
                       "label", label,
                       "use-underline", TRUE,
                       NULL);
}

GtkCssNode *
gtk_check_button_get_indicator_node (GtkCheckButton *check_button)
{
  GtkCheckButtonPrivate *priv = gtk_check_button_get_instance_private (check_button);

  return gtk_widget_get_css_node (priv->indicator_widget);
}

/**
 * gtk_check_button_set_draw_indicator:
 * @check_button: a #GtkCheckButton
 * @draw_indicator: Whether or not to draw the indicator part of the button
 *
 * Sets whether the indicator part of the button is drawn. This is important for
 * cases where the check button should have the functionality of a check button,
 * but the visuals of a regular button, like in a #GtkStackSwitcher.
 */
void
gtk_check_button_set_draw_indicator (GtkCheckButton *check_button,
                                     gboolean        draw_indicator)
{
  GtkCheckButtonPrivate *priv = gtk_check_button_get_instance_private (check_button);

  g_return_if_fail (GTK_IS_CHECK_BUTTON (check_button));

  g_warning (__FUNCTION__);

  draw_indicator = !!draw_indicator;

  if (draw_indicator != priv->draw_indicator)
    {
      priv->draw_indicator = draw_indicator;
      gtk_widget_queue_resize (GTK_WIDGET (check_button));
      g_object_notify_by_pspec (G_OBJECT (check_button), props[PROP_DRAW_INDICATOR]);
    }
}

/**
 * gtk_check_button_get_draw_indicator:
 * @check_button: a #GtkCheckButton
 *
 * Returns Whether or not the indicator part of the button gets drawn.
 *
 * Returns: The value of the GtkCheckButton:draw-indicator property.
 */
gboolean
gtk_check_button_get_draw_indicator (GtkCheckButton *check_button)
{
  GtkCheckButtonPrivate *priv = gtk_check_button_get_instance_private (check_button);

  g_return_val_if_fail (GTK_IS_CHECK_BUTTON (check_button), FALSE);

  return priv->draw_indicator;
}

/**
 * gtk_check_button_set_inconsistent:
 * @check_button: a #GtkCheckButton
 * @inconsistent: %TRUE if state is inconsistent
 *
 * If the user has selected a range of elements (such as some text or
 * spreadsheet cells) that are affected by a check button, and the
 * current values in that range are inconsistent, you may want to
 * display the toggle in an "in between" state. Normally you would
 * turn off the inconsistent state again if the user checks the
 * check button. This has to be done manually,
 * gtk_check_button_set_inconsistent only affects visual appearance,
 * not the semantics of the button.
 */
void
gtk_check_button_set_inconsistent (GtkCheckButton *check_button,
                                   gboolean        inconsistent)
{
  GtkCheckButtonPrivate *priv = gtk_check_button_get_instance_private (check_button);

  g_return_if_fail (GTK_IS_CHECK_BUTTON (check_button));

  inconsistent = !!inconsistent;
  if (priv->inconsistent != inconsistent)
    {
      priv->inconsistent = inconsistent;

      if (inconsistent)
        {
          gtk_widget_set_state_flags (GTK_WIDGET (check_button), GTK_STATE_FLAG_INCONSISTENT, FALSE);
          gtk_widget_set_state_flags (priv->indicator_widget, GTK_STATE_FLAG_INCONSISTENT, FALSE);
        }
      else
        {
          gtk_widget_unset_state_flags (GTK_WIDGET (check_button), GTK_STATE_FLAG_INCONSISTENT);
          gtk_widget_unset_state_flags (priv->indicator_widget, GTK_STATE_FLAG_INCONSISTENT);
        }

      update_accessible_state (check_button);

      g_object_notify_by_pspec (G_OBJECT (check_button), props[PROP_INCONSISTENT]);
    }
}

/**
 * gtk_check_button_get_inconsistent:
 * @check_button: a #GtkCheckButton
 *
 * Returns whether the check button is in an inconsistent state.
 *
 * Returns: %TRUE if @check_button is currently in an 'in between' state, %FALSE otherwise.
 */
gboolean
gtk_check_button_get_inconsistent (GtkCheckButton *check_button)
{
  GtkCheckButtonPrivate *priv = gtk_check_button_get_instance_private (check_button);

  g_return_val_if_fail (GTK_IS_CHECK_BUTTON (check_button), FALSE);

  return priv->inconsistent;
}

/**
 * gtk_check_button_get_active:
 * @self: a #GtkCheckButton
 *
 * Returns the current value of the #GtkCheckButton:active property.
 *
 * Returns: The value of the #GtkCheckButton:active property.
 *   See gtk_check_button_set_active() for details on how to set a new value.
 */
gboolean
gtk_check_button_get_active (GtkCheckButton *self)
{
  GtkCheckButtonPrivate *priv = gtk_check_button_get_instance_private (self);

  g_return_val_if_fail (GTK_IS_CHECK_BUTTON (self), FALSE);

  return priv->active;
}

static GtkCheckButton *
get_group_next (GtkCheckButton *self)
{
  return ((GtkCheckButtonPrivate *)gtk_check_button_get_instance_private (self))->group_next;
}

static GtkCheckButton *
get_group_prev (GtkCheckButton *self)
{
  return ((GtkCheckButtonPrivate *)gtk_check_button_get_instance_private (self))->group_prev;
}

/**
 * gtk_check_button_set_active:
 * @self: a #GtkCheckButton
 * @setting: the new value to set
 *
 * Sets the new value of the #GtkCheckButton:active property.
 * See also gtk_check_button_get_active()
 */
void
gtk_check_button_set_active (GtkCheckButton *self,
                             gboolean       setting)
{
  GtkCheckButtonPrivate *priv = gtk_check_button_get_instance_private (self);

  g_return_if_fail (GTK_IS_CHECK_BUTTON (self));

  setting = !!setting;

  if (setting == priv->active)
    return;

  if (setting)
    {
      gtk_widget_set_state_flags (GTK_WIDGET (self), GTK_STATE_FLAG_CHECKED, FALSE);
      gtk_widget_set_state_flags (priv->indicator_widget, GTK_STATE_FLAG_CHECKED, FALSE);
    }
  else
    {
      gtk_widget_unset_state_flags (GTK_WIDGET (self), GTK_STATE_FLAG_CHECKED);
      gtk_widget_unset_state_flags (priv->indicator_widget, GTK_STATE_FLAG_CHECKED);
    }

  if (setting && (priv->group_prev || priv->group_next))
    {
      GtkCheckButton *group_first = NULL;
      GtkCheckButton *iter;

      /* Find first in group */
      iter = self;
      while (iter)
        {
          group_first = iter;

          iter = get_group_prev (iter);
          if (!iter)
            break;
        }

      g_assert (group_first);

      /* Set all buttons in group to !active */
      for (iter = group_first; iter; iter = get_group_next (iter))
        gtk_check_button_set_active (iter, FALSE);

      /* ... and the next code block will set this one to active */
    }

  update_accessible_state (self);
  priv->active = setting;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ACTIVE]);
  g_signal_emit (self, signals[TOGGLED], 0);
}

const char *
gtk_check_button_get_label (GtkCheckButton *self)
{
  GtkCheckButtonPrivate *priv = gtk_check_button_get_instance_private (self);

  g_return_val_if_fail (GTK_IS_CHECK_BUTTON (self), "");

  return gtk_label_get_label (GTK_LABEL (priv->label_widget));
}

void
gtk_check_button_set_label (GtkCheckButton *self,
                            const char     *label)
{
  GtkCheckButtonPrivate *priv = gtk_check_button_get_instance_private (self);

  g_return_if_fail (GTK_IS_CHECK_BUTTON (self));

  gtk_label_set_label (GTK_LABEL (priv->label_widget), label);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_LABEL]);
}

void
gtk_check_button_set_group (GtkCheckButton *self,
                            GtkCheckButton *group)
{
  GtkCheckButtonPrivate *priv = gtk_check_button_get_instance_private (self);
  GtkCheckButtonPrivate *group_priv = gtk_check_button_get_instance_private (group);

  g_return_if_fail (GTK_IS_CHECK_BUTTON (self));

  if (priv->group_next == group)
    return;

  if (!group)
    {
      if (priv->group_prev)
        {
          GtkCheckButtonPrivate *p = gtk_check_button_get_instance_private (priv->group_prev);
          p->group_next = priv->group_next;
        }
      if (priv->group_next)
        {
          GtkCheckButtonPrivate *p = gtk_check_button_get_instance_private (priv->group_next);
          p->group_prev = priv->group_prev;
        }

      priv->group_next = NULL;
      priv->group_prev = NULL;
      g_object_notify_by_pspec (G_OBJECT (self), props[PROP_GROUP]);

      if (priv->indicator_widget)
        gtk_css_node_set_name (gtk_widget_get_css_node (priv->indicator_widget),
                               g_quark_from_static_string("check"));

      return;
    }

  priv->group_prev = NULL;
  if (group_priv->group_prev)
    {
      GtkCheckButtonPrivate *prev = gtk_check_button_get_instance_private (group_priv->group_prev);

      prev->group_next = self;
      priv->group_prev = group_priv->group_prev;
    }

  group_priv->group_prev = self;
  priv->group_next = group;

  if (priv->indicator_widget)
    gtk_css_node_set_name (gtk_widget_get_css_node (priv->indicator_widget),
                           g_quark_from_static_string("radio"));

  gtk_css_node_set_name (gtk_widget_get_css_node (group_priv->indicator_widget),
                         g_quark_from_static_string("radio"));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_GROUP]);
}

/**
 * gtk_check_button_get_use_underline:
 * @self: a #GtkCheckButton
 *
 * Returns the current value of the #GtkCheckButton:use-underline property.
 *
 * Returns: The value of the #GtkCheckButton:use-underline property.
 *   See gtk_check_button_set_use_underline() for details on how to set a new value.
 */
gboolean
gtk_check_button_get_use_underline (GtkCheckButton *self)
{
  GtkCheckButtonPrivate *priv = gtk_check_button_get_instance_private (self);

  g_return_val_if_fail (GTK_IS_CHECK_BUTTON (self), FALSE);

  return priv->use_underline;
}

/**
 * gtk_check_button_set_use_underline:
 * @self: a #GtkCheckButton
 * @setting: the new value to set
 *
 * Sets the new value of the #GtkCheckButton:use-underline property.
 * See also gtk_check_button_get_use_underline()
 */
void
gtk_check_button_set_use_underline (GtkCheckButton *self,
                                    gboolean       setting)
{
  GtkCheckButtonPrivate *priv = gtk_check_button_get_instance_private (self);

  g_return_if_fail (GTK_IS_CHECK_BUTTON (self));

  setting = !!setting;

  if (setting == priv->use_underline)
    return;

  priv->use_underline = setting;
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_USE_UNDERLINE]);
}
