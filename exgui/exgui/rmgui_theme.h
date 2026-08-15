#pragma once

#include "nanovg.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

enum class RmVisualState {
  normal,
  hovered,
  pressed,
  disabled
};

enum class RmThemeMode {
  dark,
  light
};

enum class RmButtonVariant {
  primary,
  secondary,
  outline,
  subtle,
  destructive
};

enum class RmTabVariant {
  document,
  tool,
  segmented,
  underline
};

enum class RmTabPlacement {
  top,
  bottom,
  left,
  right
};

struct RmStateColors {
  NVGcolor normal;
  NVGcolor hovered;
  NVGcolor pressed;
  NVGcolor disabled;

  NVGcolor resolve(RmVisualState state) const noexcept {
    switch (state) {
    case RmVisualState::hovered: return hovered;
    case RmVisualState::pressed: return pressed;
    case RmVisualState::disabled: return disabled;
    case RmVisualState::normal:
    default: return normal;
    }
  }
};

struct RmWindowStyle {
  RmStateColors background;
  RmStateColors border;
  RmStateColors titlebar_background;
  NVGcolor focus_ring;
  float corner_radius = 0.0f;
  float border_width = 0.0f;
  float focus_ring_width = 0.0f;
  float titlebar_height = 0.0f;
  float resize_grip_extent = 0.0f;
};

struct RmTooltipStyle {
  NVGcolor background;
  NVGcolor border;
  NVGcolor text;
  float horizontal_padding = 0.0f;
  float vertical_padding = 0.0f;
  float cursor_offset = 0.0f;
  float corner_radius = 0.0f;
  float border_width = 0.0f;
  float font_size = 0.0f;
  float show_delay = 0.0f;
};

struct RmButtonStyle {
  RmStateColors background;
  RmStateColors border;
  RmStateColors text;
  NVGcolor focus_ring;
  float corner_radius = 0.0f;
  float border_width = 0.0f;
  float focus_ring_width = 0.0f;
  float font_size = 0.0f;
  float icon_size = 0.0f;
  float icon_text_gap = 0.0f;
};

struct RmButtonStyles {
  RmButtonStyle primary;
  RmButtonStyle secondary;
  RmButtonStyle outline;
  RmButtonStyle subtle;
  RmButtonStyle destructive;

  const RmButtonStyle& resolve(RmButtonVariant variant) const noexcept {
    switch (variant) {
    case RmButtonVariant::secondary: return secondary;
    case RmButtonVariant::outline: return outline;
    case RmButtonVariant::subtle: return subtle;
    case RmButtonVariant::destructive: return destructive;
    case RmButtonVariant::primary:
    default: return primary;
    }
  }
};

struct RmTabStyle {
  RmStateColors background;
  RmStateColors border;
  RmStateColors text;
  RmStateColors selected_background;
  RmStateColors selected_border;
  RmStateColors selected_text;
  RmStateColors close_icon;
  NVGcolor bar_background;
  NVGcolor page_background;
  NVGcolor page_border;
  NVGcolor indicator;
  NVGcolor focus_ring;
  float tab_height = 0.0f;
  float vertical_bar_width = 0.0f;
  float horizontal_padding = 0.0f;
  float minimum_width = 0.0f;
  float maximum_width = 0.0f;
  float gap = 0.0f;
  float corner_radius = 0.0f;
  float border_width = 0.0f;
  float indicator_thickness = 0.0f;
  float close_size = 0.0f;
  float font_size = 0.0f;
  float focus_ring_width = 0.0f;
  bool fill_available_width = false;
  bool show_indicator = false;
  bool show_page_border = true;
};

struct RmTabStyles {
  RmTabStyle document;
  RmTabStyle tool;
  RmTabStyle segmented;
  RmTabStyle underline;

  const RmTabStyle& resolve(RmTabVariant variant) const noexcept {
    switch (variant) {
    case RmTabVariant::tool: return tool;
    case RmTabVariant::segmented: return segmented;
    case RmTabVariant::underline: return underline;
    case RmTabVariant::document:
    default: return document;
    }
  }
};

struct RmMenuStyle {
  RmStateColors item_background;
  RmStateColors item_text;
  RmStateColors item_icon;
  NVGcolor bar_background;
  NVGcolor popup_background;
  NVGcolor popup_border;
  NVGcolor separator;
  NVGcolor shadow;
  float bar_height = 0.0f;
  float item_height = 0.0f;
  float horizontal_padding = 0.0f;
  float vertical_padding = 0.0f;
  float popup_minimum_width = 0.0f;
  float border_width = 0.0f;
  float corner_radius = 0.0f;
  float separator_thickness = 0.0f;
  float separator_margin = 0.0f;
  float submenu_indicator_size = 0.0f;
  float shadow_size = 0.0f;
  float font_size = 0.0f;
};

struct RmLabelStyle {
  NVGcolor text;
  NVGcolor disabled_text;
  float font_size = 0.0f;
};

struct RmTextInputStyle {
  RmStateColors background;
  RmStateColors border;
  RmStateColors text;
  NVGcolor selection;
  NVGcolor caret;
  NVGcolor focus_ring;
  float corner_radius = 0.0f;
  float border_width = 0.0f;
  float focus_ring_width = 0.0f;
  float horizontal_padding = 0.0f;
  float vertical_padding = 0.0f;
  float font_size = 0.0f;
  float caret_width = 0.0f;
  float selection_corner_radius = 0.0f;
};

struct RmNumberInputStyle {
  RmStateColors background;
  RmStateColors border;
  RmStateColors text;
  RmStateColors button_background;
  RmStateColors button_icon;
  NVGcolor separator;
  NVGcolor focus_ring;
  float corner_radius = 0.0f;
  float border_width = 0.0f;
  float focus_ring_width = 0.0f;
  float horizontal_padding = 0.0f;
  float button_width = 0.0f;
  float separator_width = 0.0f;
  float icon_size = 0.0f;
  float font_size = 0.0f;
};

struct RmCheckboxStyle {
  RmStateColors background;
  RmStateColors border;
  RmStateColors mark;
  RmStateColors text;
  NVGcolor focus_ring;
  float box_size = 0.0f;
  float corner_radius = 0.0f;
  float border_width = 0.0f;
  float mark_width = 0.0f;
  float text_gap = 0.0f;
  float font_size = 0.0f;
  float focus_ring_width = 0.0f;
};

struct RmRadioButtonStyle {
  RmStateColors background;
  RmStateColors indicator_background;
  RmStateColors indicator_border;
  RmStateColors mark;
  RmStateColors text;
  NVGcolor focus_ring;
  NVGcolor shadow;
  float indicator_size = 0.0f;
  float mark_radius = 0.0f;
  float border_width = 0.0f;
  float horizontal_padding = 0.0f;
  float text_gap = 0.0f;
  float corner_radius = 0.0f;
  float focus_ring_width = 0.0f;
  float shadow_offset = 0.0f;
  float shadow_size = 0.0f;
  float font_size = 0.0f;
};

struct RmComboBoxStyle {
  RmStateColors field_background;
  RmStateColors field_border;
  RmStateColors field_text;
  RmStateColors indicator;
  RmStateColors item_background;
  RmStateColors item_text;
  NVGcolor placeholder_text;
  NVGcolor selected_mark;
  NVGcolor popup_background;
  NVGcolor popup_border;
  NVGcolor focus_ring;
  NVGcolor shadow;
  float corner_radius = 0.0f;
  float border_width = 0.0f;
  float focus_ring_width = 0.0f;
  float horizontal_padding = 0.0f;
  float popup_padding = 0.0f;
  float popup_gap = 0.0f;
  float item_height = 0.0f;
  float indicator_size = 0.0f;
  float selected_mark_width = 0.0f;
  float shadow_size = 0.0f;
  float font_size = 0.0f;
};

struct RmListViewStyle {
  RmStateColors row_background;
  RmStateColors row_text;
  RmStateColors selected_background;
  RmStateColors selected_text;
  NVGcolor background;
  NVGcolor border;
  NVGcolor focus_ring;
  float row_height = 0.0f;
  float horizontal_padding = 0.0f;
  float vertical_padding = 0.0f;
  float corner_radius = 0.0f;
  float row_corner_radius = 0.0f;
  float border_width = 0.0f;
  float focus_ring_width = 0.0f;
  float font_size = 0.0f;
};

struct RmTreeViewStyle {
  RmStateColors row_background;
  RmStateColors row_text;
  RmStateColors expander;
  RmStateColors selected_background;
  RmStateColors selected_text;
  NVGcolor background;
  NVGcolor border;
  NVGcolor guide;
  NVGcolor focus_ring;
  NVGcolor selection_border;
  float row_height = 0.0f;
  float indent = 0.0f;
  float horizontal_padding = 0.0f;
  float selection_horizontal_padding = 0.0f;
  float selection_vertical_padding = 0.0f;
  float icon_size = 0.0f;
  float icon_text_gap = 0.0f;
  float expander_size = 0.0f;
  float expander_stroke_width = 0.0f;
  float corner_radius = 0.0f;
  float row_corner_radius = 0.0f;
  float border_width = 0.0f;
  float focus_ring_width = 0.0f;
  float selection_border_width = 0.0f;
  float font_size = 0.0f;
  bool draw_background = true;
  bool draw_border = true;
  bool show_guides = true;
};

struct RmPropertyViewStyle {
  RmStateColors row_background;
  RmStateColors value_background;
  NVGcolor background;
  NVGcolor border;
  NVGcolor grid;
  NVGcolor name_text;
  NVGcolor value_text;
  NVGcolor group_background;
  NVGcolor group_text;
  NVGcolor invalid_background;
  NVGcolor invalid_border;
  NVGcolor invalid_text;
  NVGcolor focus_ring;
  NVGcolor choice_indicator;
  float row_height = 0.0f;
  float group_height = 0.0f;
  float name_column_ratio = 0.0f;
  float horizontal_padding = 0.0f;
  float editor_padding = 0.0f;
  float border_width = 0.0f;
  float grid_width = 0.0f;
  float focus_ring_width = 0.0f;
  float corner_radius = 0.0f;
  float font_size = 0.0f;
  float group_font_size = 0.0f;
  float choice_indicator_size = 0.0f;
};

struct RmToolStripStyle {
  RmStateColors button_background;
  RmStateColors button_border;
  RmStateColors button_text;
  NVGcolor background;
  NVGcolor border;
  NVGcolor selected_background;
  NVGcolor selected_border;
  NVGcolor group_text;
  NVGcolor separator;
  float button_extent = 0.0f;
  float button_gap = 0.0f;
  float group_gap = 0.0f;
  float group_label_height = 0.0f;
  float group_padding = 0.0f;
  float corner_radius = 0.0f;
  float border_width = 0.0f;
  float selected_border_width = 0.0f;
  float font_size = 0.0f;
  float group_font_size = 0.0f;
};

struct RmRebarStyle {
  NVGcolor background;
  NVGcolor border;
  NVGcolor separator;
  NVGcolor gripper;
  NVGcolor gripper_hovered;
  NVGcolor gripper_active;
  NVGcolor resize_handle;
  float band_gap = 0.0f;
  float band_padding = 0.0f;
  float row_gap = 0.0f;
  float gripper_extent = 0.0f;
  float resize_handle_extent = 0.0f;
  float border_width = 0.0f;
  float separator_width = 0.0f;
  float corner_radius = 0.0f;
};

struct RmSplitterStyle {
  RmStateColors background;
  NVGcolor grip;
  NVGcolor focus_ring;
  float thickness = 0.0f;
  float grip_extent = 0.0f;
  float grip_width = 0.0f;
  float focus_ring_width = 0.0f;
  float corner_radius = 0.0f;
};

struct RmOutputTextStyle {
  NVGcolor background;
  NVGcolor border;
  NVGcolor text;
  NVGcolor focus_ring;
  float line_height = 0.0f;
  float horizontal_padding = 0.0f;
  float vertical_padding = 0.0f;
  float corner_radius = 0.0f;
  float border_width = 0.0f;
  float focus_ring_width = 0.0f;
  float font_size = 0.0f;
};

struct RmSliderStyle {
  RmStateColors track;
  RmStateColors fill;
  RmStateColors thumb;
  RmStateColors thumb_border;
  NVGcolor focus_ring;
  float track_height = 0.0f;
  float padding = 0.0f;
  float thumb_radius = 0.0f;
  float thumb_border_width = 0.0f;
  float focus_ring_width = 0.0f;
};

struct RmScrollbarStyle {
  RmStateColors track;
  RmStateColors track_border;
  RmStateColors thumb;
  RmStateColors thumb_border;
  NVGcolor focus_ring;
  float thickness = 0.0f;
  float minimum_thumb_length = 0.0f;
  float padding = 0.0f;
  float corner_radius = 0.0f;
  float border_width = 0.0f;
  float thumb_border_width = 0.0f;
  float focus_ring_width = 0.0f;
};

struct RmProgressStyle {
  RmStateColors background;
  RmStateColors fill_start;
  RmStateColors fill_end;
  float corner_radius = 0.0f;
};

struct RmSwitchStyle {
  RmStateColors track_off;
  RmStateColors track_on;
  RmStateColors knob;
  NVGcolor focus_ring;
  NVGcolor shadow;
  float track_height = 0.0f;
  float padding = 0.0f;
  float knob_radius = 0.0f;
  float corner_radius = 0.0f;
  float animation_duration = 0.0f;
  float shadow_offset = 0.0f;
  float shadow_size = 0.0f;
  float focus_ring_width = 0.0f;
};

struct RmColorTokens {
  NVGcolor accent;
  NVGcolor accent_hovered;
  NVGcolor accent_pressed;
  NVGcolor accent_disabled;
  NVGcolor accent_secondary;
  NVGcolor danger;
  NVGcolor danger_hovered;
  NVGcolor danger_pressed;
  NVGcolor danger_disabled;
  NVGcolor surface;
  NVGcolor surface_elevated;
  NVGcolor control;
  NVGcolor control_hovered;
  NVGcolor control_pressed;
  NVGcolor control_disabled;
  NVGcolor border;
  NVGcolor border_hovered;
  NVGcolor border_pressed;
  NVGcolor border_disabled;
  NVGcolor text;
  NVGcolor text_muted;
  NVGcolor text_disabled;
  NVGcolor text_on_accent;
  NVGcolor focus_ring;
  NVGcolor shadow;
};

struct RmSpacingTokens {
  float xsmall = 4.0f;
  float small = 8.0f;
  float medium = 12.0f;
  float large = 16.0f;
  float xlarge = 24.0f;
};

struct RmRadiusTokens {
  float small = 2.0f;
  float medium = 4.0f;
  float large = 5.0f;
  float pill = 999.0f;
};

struct RmTypographyTokens {
  float caption = 13.0f;
  float body = 16.0f;
  float control = 16.0f;
  float title = 20.0f;
};

struct RmAnimationTokens {
  float fast = 0.12f;
  float normal = 0.20f;
  float slow = 0.32f;
};

struct RmControlMetricsTokens {
  float border_width = 1.0f;
  float focus_ring_width = 1.0f;
  float window_titlebar_height = 30.0f;
  float window_resize_grip_extent = 5.0f;
  float button_icon_size = 16.0f;
  float button_icon_text_gap = 6.0f;
  float tooltip_horizontal_padding = 7.0f;
  float tooltip_vertical_padding = 5.0f;
  float tooltip_cursor_offset = 12.0f;
  float tooltip_show_delay = 0.45f;
  float text_input_horizontal_padding = 10.0f;
  float text_input_vertical_padding = 7.0f;
  float text_input_caret_width = 1.5f;
  float text_input_selection_corner_radius = 2.0f;
  float number_input_horizontal_padding = 10.0f;
  float number_input_button_width = 28.0f;
  float number_input_separator_width = 1.0f;
  float number_input_icon_size = 5.0f;
  float checkbox_size = 20.0f;
  float checkbox_mark_width = 2.25f;
  float radiobutton_indicator_size = 20.0f;
  float radiobutton_mark_radius = 5.0f;
  float radiobutton_horizontal_padding = 6.0f;
  float radiobutton_shadow_offset = 2.0f;
  float radiobutton_shadow_size = 5.0f;
  float combobox_horizontal_padding = 10.0f;
  float combobox_popup_padding = 5.0f;
  float combobox_popup_gap = 4.0f;
  float combobox_item_height = 30.0f;
  float combobox_indicator_size = 5.0f;
  float combobox_selected_mark_width = 1.0f;
  float combobox_shadow_size = 10.0f;
  float listview_row_height = 32.0f;
  float listview_horizontal_padding = 10.0f;
  float listview_vertical_padding = 5.0f;
  float treeview_row_height = 30.0f;
  float treeview_indent = 20.0f;
  float treeview_horizontal_padding = 8.0f;
  float treeview_selection_horizontal_padding = 6.0f;
  float treeview_selection_vertical_padding = 3.0f;
  float treeview_icon_size = 16.0f;
  float treeview_icon_text_gap = 4.0f;
  float treeview_expander_size = 7.0f;
  float treeview_expander_stroke_width = 1.0f;
  bool treeview_draw_background = true;
  bool treeview_draw_border = true;
  float propertyview_row_height = 27.0f;
  float propertyview_group_height = 25.0f;
  float propertyview_name_column_ratio = 0.46f;
  float propertyview_horizontal_padding = 7.0f;
  float propertyview_editor_padding = 5.0f;
  float propertyview_grid_width = 1.0f;
  float propertyview_choice_indicator_size = 4.0f;
  float toolstrip_button_extent = 30.0f;
  float toolstrip_button_gap = 2.0f;
  float toolstrip_group_gap = 6.0f;
  float toolstrip_group_label_height = 20.0f;
  float toolstrip_group_padding = 3.0f;
  float rebar_band_gap = 3.0f;
  float rebar_band_padding = 2.0f;
  float rebar_row_gap = 2.0f;
  float rebar_gripper_extent = 6.0f;
  float rebar_resize_handle_extent = 5.0f;
  float splitter_thickness = 6.0f;
  float splitter_grip_extent = 18.0f;
  float splitter_grip_width = 1.0f;
  float output_text_line_height = 20.0f;
  float output_text_horizontal_padding = 10.0f;
  float output_text_vertical_padding = 8.0f;
  float slider_track_height = 4.0f;
  float slider_padding = 8.0f;
  float slider_thumb_radius = 10.0f;
  float slider_thumb_border_width = 1.0f;
  float scrollbar_thickness = 12.0f;
  float scrollbar_minimum_thumb_length = 28.0f;
  float scrollbar_padding = 2.0f;
  float scrollbar_thumb_border_width = 1.0f;
  float progress_corner_radius = 4.0f;
  float switch_track_height = 24.0f;
  float switch_padding = 4.0f;
  float switch_knob_radius = 11.0f;
  float switch_shadow_offset = 5.0f;
  float switch_shadow_size = 7.0f;
  float tab_height = 36.0f;
  float tab_vertical_bar_width = 168.0f;
  float tab_horizontal_padding = 14.0f;
  float tab_minimum_width = 72.0f;
  float tab_maximum_width = 220.0f;
  float tab_gap = 2.0f;
  float tab_indicator_thickness = 3.0f;
  float tab_close_size = 14.0f;
  float menu_bar_height = 30.0f;
  float menu_item_height = 28.0f;
  float menu_horizontal_padding = 12.0f;
  float menu_vertical_padding = 6.0f;
  float menu_popup_minimum_width = 180.0f;
  float menu_separator_thickness = 1.0f;
  float menu_separator_margin = 8.0f;
  float menu_submenu_indicator_size = 6.0f;
  float menu_shadow_size = 10.0f;
};

struct RmThemeTokens {
  RmColorTokens colors;
  RmSpacingTokens spacing;
  RmRadiusTokens radius;
  RmTypographyTokens typography;
  RmAnimationTokens animation;
  RmControlMetricsTokens controls;
};

struct RmThemeDocument {
  std::string name;
  RmThemeMode mode = RmThemeMode::dark;
  RmThemeTokens tokens;

  static RmThemeDocument dark_theme();
  static RmThemeDocument light_theme();
};

struct RmThemeSnapshot {
  std::string name;
  RmThemeMode mode = RmThemeMode::dark;
  RmThemeTokens tokens;
  RmWindowStyle window;
  RmTooltipStyle tooltip;
  RmButtonStyles buttons;
  RmTabStyles tabs;
  RmMenuStyle menu;
  RmLabelStyle label;
  RmTextInputStyle text_input;
  RmNumberInputStyle number_input;
  RmCheckboxStyle checkbox;
  RmRadioButtonStyle radiobutton;
  RmComboBoxStyle combobox;
  RmListViewStyle listview;
  RmTreeViewStyle treeview;
  RmPropertyViewStyle propertyview;
  RmToolStripStyle toolbar;
  RmToolStripStyle toolbox;
  RmRebarStyle rebar;
  RmSplitterStyle splitter;
  RmOutputTextStyle output_text;
  RmSliderStyle slider;
  RmScrollbarStyle scrollbar;
  RmProgressStyle progress;
  RmSwitchStyle switch_control;

  static std::shared_ptr<const RmThemeSnapshot> default_theme();
};

using RmThemeRef = std::shared_ptr<const RmThemeSnapshot>;

enum class RmThemeDiagnosticSeverity {
  warning,
  error
};

struct RmThemeDiagnostic {
  RmThemeDiagnosticSeverity severity = RmThemeDiagnosticSeverity::warning;
  std::string property;
  std::string message;
};

struct RmThemeCompileResult {
  RmThemeRef theme;
  std::vector<RmThemeDiagnostic> diagnostics;

  bool succeeded() const noexcept {
    if (!theme)
      return false;
    for (const RmThemeDiagnostic& diagnostic : diagnostics) {
      if (diagnostic.severity == RmThemeDiagnosticSeverity::error)
        return false;
    }
    return true;
  }
};

class RmThemeCompiler {
  static void sanitize_metric(float& value, float minimum, float maximum,
    const char* p_property, std::vector<RmThemeDiagnostic>& diagnostics)
  {
    const float sanitized = std::clamp(value, minimum, maximum);
    if (sanitized == value)
      return;
    value = sanitized;
    diagnostics.push_back({ RmThemeDiagnosticSeverity::warning, p_property,
      "Value was outside the supported range and has been clamped." });
  }

  static void sanitize_color(NVGcolor& color, const char* p_property,
    std::vector<RmThemeDiagnostic>& diagnostics)
  {
    const NVGcolor original = color;
    color.r = std::clamp(color.r, 0.0f, 1.0f);
    color.g = std::clamp(color.g, 0.0f, 1.0f);
    color.b = std::clamp(color.b, 0.0f, 1.0f);
    color.a = std::clamp(color.a, 0.0f, 1.0f);
    if (original.r != color.r || original.g != color.g ||
      original.b != color.b || original.a != color.a) {
      diagnostics.push_back({ RmThemeDiagnosticSeverity::warning, p_property,
        "Color components were clamped to the normalized RGBA range." });
    }
  }

  static void sanitize_tokens(RmThemeTokens& tokens,
    std::vector<RmThemeDiagnostic>& diagnostics)
  {
    NVGcolor* colors[] = {
      &tokens.colors.accent, &tokens.colors.accent_hovered,
      &tokens.colors.accent_pressed, &tokens.colors.accent_disabled,
      &tokens.colors.accent_secondary, &tokens.colors.danger,
      &tokens.colors.danger_hovered, &tokens.colors.danger_pressed,
      &tokens.colors.danger_disabled, &tokens.colors.surface,
      &tokens.colors.surface_elevated, &tokens.colors.control,
      &tokens.colors.control_hovered, &tokens.colors.control_pressed,
      &tokens.colors.control_disabled, &tokens.colors.border,
      &tokens.colors.border_hovered, &tokens.colors.border_pressed,
      &tokens.colors.border_disabled, &tokens.colors.text,
      &tokens.colors.text_muted, &tokens.colors.text_disabled,
      &tokens.colors.text_on_accent, &tokens.colors.focus_ring,
      &tokens.colors.shadow
    };
    for (NVGcolor* p_color : colors)
      sanitize_color(*p_color, "tokens.colors", diagnostics);

    sanitize_metric(tokens.spacing.xsmall, 0.0f, 128.0f, "tokens.spacing.xsmall", diagnostics);
    sanitize_metric(tokens.spacing.small, 0.0f, 128.0f, "tokens.spacing.small", diagnostics);
    sanitize_metric(tokens.spacing.medium, 0.0f, 128.0f, "tokens.spacing.medium", diagnostics);
    sanitize_metric(tokens.spacing.large, 0.0f, 128.0f, "tokens.spacing.large", diagnostics);
    sanitize_metric(tokens.spacing.xlarge, 0.0f, 128.0f, "tokens.spacing.xlarge", diagnostics);
    sanitize_metric(tokens.radius.small, 0.0f, 1024.0f, "tokens.radius.small", diagnostics);
    sanitize_metric(tokens.radius.medium, 0.0f, 1024.0f, "tokens.radius.medium", diagnostics);
    sanitize_metric(tokens.radius.large, 0.0f, 1024.0f, "tokens.radius.large", diagnostics);
    sanitize_metric(tokens.radius.pill, 0.0f, 4096.0f, "tokens.radius.pill", diagnostics);
    sanitize_metric(tokens.typography.caption, 1.0f, 256.0f, "tokens.typography.caption", diagnostics);
    sanitize_metric(tokens.typography.body, 1.0f, 256.0f, "tokens.typography.body", diagnostics);
    sanitize_metric(tokens.typography.control, 1.0f, 256.0f, "tokens.typography.control", diagnostics);
    sanitize_metric(tokens.typography.title, 1.0f, 256.0f, "tokens.typography.title", diagnostics);
    sanitize_metric(tokens.animation.fast, 0.0001f, 10.0f, "tokens.animation.fast", diagnostics);
    sanitize_metric(tokens.animation.normal, 0.0001f, 10.0f, "tokens.animation.normal", diagnostics);
    sanitize_metric(tokens.animation.slow, 0.0001f, 10.0f, "tokens.animation.slow", diagnostics);
    sanitize_metric(tokens.controls.border_width, 0.0f, 32.0f, "tokens.controls.border_width", diagnostics);
    sanitize_metric(tokens.controls.focus_ring_width, 0.0f, 32.0f, "tokens.controls.focus_ring_width", diagnostics);
    sanitize_metric(tokens.controls.window_titlebar_height, 0.0f, 256.0f, "tokens.controls.window_titlebar_height", diagnostics);
    sanitize_metric(tokens.controls.window_resize_grip_extent, 1.0f, 64.0f, "tokens.controls.window_resize_grip_extent", diagnostics);
    sanitize_metric(tokens.controls.button_icon_size, 1.0f, 256.0f, "tokens.controls.button_icon_size", diagnostics);
    sanitize_metric(tokens.controls.button_icon_text_gap, 0.0f, 256.0f, "tokens.controls.button_icon_text_gap", diagnostics);
    sanitize_metric(tokens.controls.tooltip_horizontal_padding, 0.0f, 256.0f, "tokens.controls.tooltip_horizontal_padding", diagnostics);
    sanitize_metric(tokens.controls.tooltip_vertical_padding, 0.0f, 256.0f, "tokens.controls.tooltip_vertical_padding", diagnostics);
    sanitize_metric(tokens.controls.tooltip_cursor_offset, 0.0f, 256.0f, "tokens.controls.tooltip_cursor_offset", diagnostics);
    sanitize_metric(tokens.controls.tooltip_show_delay, 0.0f, 10.0f, "tokens.controls.tooltip_show_delay", diagnostics);
    sanitize_metric(tokens.controls.text_input_horizontal_padding, 0.0f, 256.0f, "tokens.controls.text_input_horizontal_padding", diagnostics);
    sanitize_metric(tokens.controls.text_input_vertical_padding, 0.0f, 256.0f, "tokens.controls.text_input_vertical_padding", diagnostics);
    sanitize_metric(tokens.controls.text_input_caret_width, 0.0f, 32.0f, "tokens.controls.text_input_caret_width", diagnostics);
    sanitize_metric(tokens.controls.text_input_selection_corner_radius, 0.0f, 128.0f, "tokens.controls.text_input_selection_corner_radius", diagnostics);
    sanitize_metric(tokens.controls.number_input_horizontal_padding, 0.0f, 256.0f, "tokens.controls.number_input_horizontal_padding", diagnostics);
    sanitize_metric(tokens.controls.number_input_button_width, 8.0f, 256.0f, "tokens.controls.number_input_button_width", diagnostics);
    sanitize_metric(tokens.controls.number_input_separator_width, 0.0f, 32.0f, "tokens.controls.number_input_separator_width", diagnostics);
    sanitize_metric(tokens.controls.number_input_icon_size, 1.0f, 64.0f, "tokens.controls.number_input_icon_size", diagnostics);
    sanitize_metric(tokens.controls.checkbox_size, 1.0f, 256.0f, "tokens.controls.checkbox_size", diagnostics);
    sanitize_metric(tokens.controls.checkbox_mark_width, 0.0f, 32.0f, "tokens.controls.checkbox_mark_width", diagnostics);
    sanitize_metric(tokens.controls.radiobutton_indicator_size, 1.0f, 256.0f, "tokens.controls.radiobutton_indicator_size", diagnostics);
    sanitize_metric(tokens.controls.radiobutton_mark_radius, 0.0f, 128.0f, "tokens.controls.radiobutton_mark_radius", diagnostics);
    sanitize_metric(tokens.controls.radiobutton_horizontal_padding, 0.0f, 256.0f, "tokens.controls.radiobutton_horizontal_padding", diagnostics);
    sanitize_metric(tokens.controls.radiobutton_shadow_offset, 0.0f, 256.0f, "tokens.controls.radiobutton_shadow_offset", diagnostics);
    sanitize_metric(tokens.controls.radiobutton_shadow_size, 0.0f, 256.0f, "tokens.controls.radiobutton_shadow_size", diagnostics);
    sanitize_metric(tokens.controls.combobox_horizontal_padding, 0.0f, 256.0f, "tokens.controls.combobox_horizontal_padding", diagnostics);
    sanitize_metric(tokens.controls.combobox_popup_padding, 0.0f, 256.0f, "tokens.controls.combobox_popup_padding", diagnostics);
    sanitize_metric(tokens.controls.combobox_popup_gap, 0.0f, 256.0f, "tokens.controls.combobox_popup_gap", diagnostics);
    sanitize_metric(tokens.controls.combobox_item_height, 16.0f, 256.0f, "tokens.controls.combobox_item_height", diagnostics);
    sanitize_metric(tokens.controls.combobox_indicator_size, 1.0f, 64.0f, "tokens.controls.combobox_indicator_size", diagnostics);
    sanitize_metric(tokens.controls.combobox_selected_mark_width, 0.0f, 32.0f, "tokens.controls.combobox_selected_mark_width", diagnostics);
    sanitize_metric(tokens.controls.combobox_shadow_size, 0.0f, 256.0f, "tokens.controls.combobox_shadow_size", diagnostics);
    sanitize_metric(tokens.controls.listview_row_height, 16.0f, 256.0f, "tokens.controls.listview_row_height", diagnostics);
    sanitize_metric(tokens.controls.listview_horizontal_padding, 0.0f, 256.0f, "tokens.controls.listview_horizontal_padding", diagnostics);
    sanitize_metric(tokens.controls.listview_vertical_padding, 0.0f, 256.0f, "tokens.controls.listview_vertical_padding", diagnostics);
    sanitize_metric(tokens.controls.treeview_row_height, 16.0f, 256.0f, "tokens.controls.treeview_row_height", diagnostics);
    sanitize_metric(tokens.controls.treeview_indent, 0.0f, 256.0f, "tokens.controls.treeview_indent", diagnostics);
    sanitize_metric(tokens.controls.treeview_horizontal_padding, 0.0f, 256.0f, "tokens.controls.treeview_horizontal_padding", diagnostics);
    sanitize_metric(tokens.controls.treeview_selection_horizontal_padding, 0.0f, 256.0f, "tokens.controls.treeview_selection_horizontal_padding", diagnostics);
    sanitize_metric(tokens.controls.treeview_selection_vertical_padding, 0.0f, 256.0f, "tokens.controls.treeview_selection_vertical_padding", diagnostics);
    sanitize_metric(tokens.controls.treeview_icon_size, 1.0f, 256.0f, "tokens.controls.treeview_icon_size", diagnostics);
    sanitize_metric(tokens.controls.treeview_icon_text_gap, 0.0f, 256.0f, "tokens.controls.treeview_icon_text_gap", diagnostics);
    sanitize_metric(tokens.controls.treeview_expander_size, 1.0f, 64.0f, "tokens.controls.treeview_expander_size", diagnostics);
    sanitize_metric(tokens.controls.treeview_expander_stroke_width, 0.0f, 32.0f, "tokens.controls.treeview_expander_stroke_width", diagnostics);
    sanitize_metric(tokens.controls.propertyview_row_height, 16.0f, 256.0f, "tokens.controls.propertyview_row_height", diagnostics);
    sanitize_metric(tokens.controls.propertyview_group_height, 16.0f, 256.0f, "tokens.controls.propertyview_group_height", diagnostics);
    sanitize_metric(tokens.controls.propertyview_name_column_ratio, 0.1f, 0.9f, "tokens.controls.propertyview_name_column_ratio", diagnostics);
    sanitize_metric(tokens.controls.propertyview_horizontal_padding, 0.0f, 256.0f, "tokens.controls.propertyview_horizontal_padding", diagnostics);
    sanitize_metric(tokens.controls.propertyview_editor_padding, 0.0f, 256.0f, "tokens.controls.propertyview_editor_padding", diagnostics);
    sanitize_metric(tokens.controls.propertyview_grid_width, 0.0f, 32.0f, "tokens.controls.propertyview_grid_width", diagnostics);
    sanitize_metric(tokens.controls.propertyview_choice_indicator_size, 1.0f, 64.0f, "tokens.controls.propertyview_choice_indicator_size", diagnostics);
    sanitize_metric(tokens.controls.toolstrip_button_extent, 12.0f, 256.0f, "tokens.controls.toolstrip_button_extent", diagnostics);
    sanitize_metric(tokens.controls.toolstrip_button_gap, 0.0f, 64.0f, "tokens.controls.toolstrip_button_gap", diagnostics);
    sanitize_metric(tokens.controls.toolstrip_group_gap, 0.0f, 256.0f, "tokens.controls.toolstrip_group_gap", diagnostics);
    sanitize_metric(tokens.controls.toolstrip_group_label_height, 0.0f, 128.0f, "tokens.controls.toolstrip_group_label_height", diagnostics);
    sanitize_metric(tokens.controls.toolstrip_group_padding, 0.0f, 64.0f, "tokens.controls.toolstrip_group_padding", diagnostics);
    sanitize_metric(tokens.controls.rebar_band_gap, 0.0f, 128.0f, "tokens.controls.rebar_band_gap", diagnostics);
    sanitize_metric(tokens.controls.rebar_band_padding, 0.0f, 128.0f, "tokens.controls.rebar_band_padding", diagnostics);
    sanitize_metric(tokens.controls.rebar_row_gap, 0.0f, 128.0f, "tokens.controls.rebar_row_gap", diagnostics);
    sanitize_metric(tokens.controls.rebar_gripper_extent, 0.0f, 64.0f, "tokens.controls.rebar_gripper_extent", diagnostics);
    sanitize_metric(tokens.controls.rebar_resize_handle_extent, 1.0f, 64.0f, "tokens.controls.rebar_resize_handle_extent", diagnostics);
    sanitize_metric(tokens.controls.splitter_thickness, 2.0f, 64.0f, "tokens.controls.splitter_thickness", diagnostics);
    sanitize_metric(tokens.controls.splitter_grip_extent, 1.0f, 128.0f, "tokens.controls.splitter_grip_extent", diagnostics);
    sanitize_metric(tokens.controls.splitter_grip_width, 0.0f, 16.0f, "tokens.controls.splitter_grip_width", diagnostics);
    sanitize_metric(tokens.controls.output_text_line_height, 1.0f, 256.0f, "tokens.controls.output_text_line_height", diagnostics);
    sanitize_metric(tokens.controls.output_text_horizontal_padding, 0.0f, 256.0f, "tokens.controls.output_text_horizontal_padding", diagnostics);
    sanitize_metric(tokens.controls.output_text_vertical_padding, 0.0f, 256.0f, "tokens.controls.output_text_vertical_padding", diagnostics);
    sanitize_metric(tokens.controls.slider_track_height, 1.0f, 256.0f, "tokens.controls.slider_track_height", diagnostics);
    sanitize_metric(tokens.controls.slider_padding, 0.0f, 256.0f, "tokens.controls.slider_padding", diagnostics);
    sanitize_metric(tokens.controls.slider_thumb_radius, 1.0f, 256.0f, "tokens.controls.slider_thumb_radius", diagnostics);
    sanitize_metric(tokens.controls.slider_thumb_border_width, 0.0f, 32.0f, "tokens.controls.slider_thumb_border_width", diagnostics);
    sanitize_metric(tokens.controls.scrollbar_thickness, 4.0f, 256.0f, "tokens.controls.scrollbar_thickness", diagnostics);
    sanitize_metric(tokens.controls.scrollbar_minimum_thumb_length, 4.0f, 1024.0f, "tokens.controls.scrollbar_minimum_thumb_length", diagnostics);
    sanitize_metric(tokens.controls.scrollbar_padding, 0.0f, 128.0f, "tokens.controls.scrollbar_padding", diagnostics);
    sanitize_metric(tokens.controls.scrollbar_thumb_border_width, 0.0f, 32.0f, "tokens.controls.scrollbar_thumb_border_width", diagnostics);
    sanitize_metric(tokens.controls.progress_corner_radius, 0.0f, 1024.0f, "tokens.controls.progress_corner_radius", diagnostics);
    sanitize_metric(tokens.controls.switch_track_height, 1.0f, 256.0f, "tokens.controls.switch_track_height", diagnostics);
    sanitize_metric(tokens.controls.switch_padding, 0.0f, 256.0f, "tokens.controls.switch_padding", diagnostics);
    sanitize_metric(tokens.controls.switch_knob_radius, 1.0f, 256.0f, "tokens.controls.switch_knob_radius", diagnostics);
    sanitize_metric(tokens.controls.switch_shadow_offset, 0.0f, 256.0f, "tokens.controls.switch_shadow_offset", diagnostics);
    sanitize_metric(tokens.controls.switch_shadow_size, 0.0f, 256.0f, "tokens.controls.switch_shadow_size", diagnostics);
    sanitize_metric(tokens.controls.tab_height, 16.0f, 256.0f, "tokens.controls.tab_height", diagnostics);
    sanitize_metric(tokens.controls.tab_vertical_bar_width, 32.0f, 1024.0f, "tokens.controls.tab_vertical_bar_width", diagnostics);
    sanitize_metric(tokens.controls.tab_horizontal_padding, 0.0f, 256.0f, "tokens.controls.tab_horizontal_padding", diagnostics);
    sanitize_metric(tokens.controls.tab_minimum_width, 1.0f, 1024.0f, "tokens.controls.tab_minimum_width", diagnostics);
    sanitize_metric(tokens.controls.tab_maximum_width, 1.0f, 4096.0f, "tokens.controls.tab_maximum_width", diagnostics);
    if (tokens.controls.tab_maximum_width < tokens.controls.tab_minimum_width) {
      tokens.controls.tab_maximum_width = tokens.controls.tab_minimum_width;
      diagnostics.push_back({ RmThemeDiagnosticSeverity::warning,
        "tokens.controls.tab_maximum_width",
        "Maximum tab width was smaller than minimum tab width and has been corrected." });
    }
    sanitize_metric(tokens.controls.tab_gap, 0.0f, 64.0f, "tokens.controls.tab_gap", diagnostics);
    sanitize_metric(tokens.controls.tab_indicator_thickness, 0.0f, 32.0f, "tokens.controls.tab_indicator_thickness", diagnostics);
    sanitize_metric(tokens.controls.tab_close_size, 4.0f, 128.0f, "tokens.controls.tab_close_size", diagnostics);
    sanitize_metric(tokens.controls.menu_bar_height, 16.0f, 256.0f, "tokens.controls.menu_bar_height", diagnostics);
    sanitize_metric(tokens.controls.menu_item_height, 16.0f, 256.0f, "tokens.controls.menu_item_height", diagnostics);
    sanitize_metric(tokens.controls.menu_horizontal_padding, 0.0f, 256.0f, "tokens.controls.menu_horizontal_padding", diagnostics);
    sanitize_metric(tokens.controls.menu_vertical_padding, 0.0f, 256.0f, "tokens.controls.menu_vertical_padding", diagnostics);
    sanitize_metric(tokens.controls.menu_popup_minimum_width, 32.0f, 2048.0f, "tokens.controls.menu_popup_minimum_width", diagnostics);
    sanitize_metric(tokens.controls.menu_separator_thickness, 0.0f, 32.0f, "tokens.controls.menu_separator_thickness", diagnostics);
    sanitize_metric(tokens.controls.menu_separator_margin, 0.0f, 256.0f, "tokens.controls.menu_separator_margin", diagnostics);
    sanitize_metric(tokens.controls.menu_submenu_indicator_size, 1.0f, 64.0f, "tokens.controls.menu_submenu_indicator_size", diagnostics);
    sanitize_metric(tokens.controls.menu_shadow_size, 0.0f, 256.0f, "tokens.controls.menu_shadow_size", diagnostics);
  }

public:
  static RmThemeCompileResult compile(const RmThemeDocument& document)
  {
    RmThemeCompileResult result;
    auto p_theme = std::make_shared<RmThemeSnapshot>();
    p_theme->name = document.name.empty() ? "Unnamed theme" : document.name;
    p_theme->mode = document.mode;
    p_theme->tokens = document.tokens;
    if (document.name.empty()) {
      result.diagnostics.push_back({ RmThemeDiagnosticSeverity::warning,
        "name", "An unnamed theme was compiled with a generated display name." });
    }
    sanitize_tokens(p_theme->tokens, result.diagnostics);

    const RmThemeTokens& tokens = p_theme->tokens;
    const RmColorTokens& colors = tokens.colors;
    const RmControlMetricsTokens& controls = tokens.controls;

    const NVGcolor transparent = NVGcolor::RGBA(0, 0, 0, 0);
    p_theme->window.background = { colors.surface_elevated,
      colors.surface_elevated, colors.surface_elevated,
      colors.control_disabled };
    p_theme->window.border = { colors.border, colors.border_hovered,
      colors.border_pressed, colors.border_disabled };
    p_theme->window.titlebar_background = { colors.control,
      colors.control_hovered, colors.control_pressed, colors.control_disabled };
    p_theme->window.focus_ring = colors.focus_ring;
    p_theme->window.corner_radius = tokens.radius.medium;
    p_theme->window.border_width = controls.border_width;
    p_theme->window.focus_ring_width = controls.focus_ring_width;
    p_theme->window.titlebar_height = controls.window_titlebar_height;
    p_theme->window.resize_grip_extent = controls.window_resize_grip_extent;

    p_theme->tooltip.background = colors.control;
    p_theme->tooltip.border = colors.border_hovered;
    p_theme->tooltip.text = colors.text;
    p_theme->tooltip.horizontal_padding = controls.tooltip_horizontal_padding;
    p_theme->tooltip.vertical_padding = controls.tooltip_vertical_padding;
    p_theme->tooltip.cursor_offset = controls.tooltip_cursor_offset;
    p_theme->tooltip.corner_radius = tokens.radius.small;
    p_theme->tooltip.border_width = controls.border_width;
    p_theme->tooltip.font_size = tokens.typography.caption;
    p_theme->tooltip.show_delay = controls.tooltip_show_delay;

    const auto configure_button = [&](RmButtonStyle& style,
      const RmStateColors& background, const RmStateColors& border,
      const RmStateColors& text, float border_width) {
      style.background = background;
      style.border = border;
      style.text = text;
      style.focus_ring = colors.focus_ring;
      style.corner_radius = tokens.radius.medium;
      style.border_width = border_width;
      style.focus_ring_width = controls.focus_ring_width;
      style.font_size = tokens.typography.control;
      style.icon_size = controls.button_icon_size;
      style.icon_text_gap = controls.button_icon_text_gap;
    };

    configure_button(p_theme->buttons.primary,
      { colors.accent, colors.accent_hovered, colors.accent_pressed,
        colors.accent_disabled },
      { colors.accent_hovered, colors.focus_ring, colors.accent_pressed,
        colors.border_disabled },
      { colors.text_on_accent, colors.text_on_accent, colors.text_on_accent,
        colors.text_disabled }, controls.border_width);
    configure_button(p_theme->buttons.secondary,
      { colors.control, colors.control_hovered, colors.control_pressed,
        colors.control_disabled },
      { colors.border, colors.border_hovered, colors.border_pressed,
        colors.border_disabled },
      { colors.text, colors.text, colors.text, colors.text_disabled },
      controls.border_width);
    configure_button(p_theme->buttons.outline,
      { transparent, colors.control_hovered, colors.control_pressed, transparent },
      { colors.accent, colors.accent_hovered, colors.accent_pressed,
        colors.border_disabled },
      { colors.accent, colors.accent_hovered, colors.accent_pressed,
        colors.text_disabled }, controls.border_width);
    configure_button(p_theme->buttons.subtle,
      { transparent, colors.control_hovered, colors.control_pressed, transparent },
      { transparent, transparent, transparent, transparent },
      { colors.text, colors.text, colors.text, colors.text_disabled }, 0.0f);
    configure_button(p_theme->buttons.destructive,
      { colors.danger, colors.danger_hovered, colors.danger_pressed,
        colors.danger_disabled },
      { colors.danger_hovered, colors.focus_ring, colors.danger_pressed,
        colors.border_disabled },
      { colors.text_on_accent, colors.text_on_accent, colors.text_on_accent,
        colors.text_disabled }, controls.border_width);

    const auto configure_tab = [&](RmTabStyle& style) {
      style.bar_background = colors.surface;
      style.page_background = colors.surface_elevated;
      style.page_border = colors.border;
      style.indicator = colors.accent;
      style.focus_ring = colors.focus_ring;
      style.tab_height = controls.tab_height;
      style.vertical_bar_width = controls.tab_vertical_bar_width;
      style.horizontal_padding = controls.tab_horizontal_padding;
      style.minimum_width = controls.tab_minimum_width;
      style.maximum_width = controls.tab_maximum_width;
      style.gap = controls.tab_gap;
      style.corner_radius = tokens.radius.medium;
      style.border_width = controls.border_width;
      style.indicator_thickness = controls.tab_indicator_thickness;
      style.close_size = controls.tab_close_size;
      style.font_size = tokens.typography.control;
      style.focus_ring_width = controls.focus_ring_width;
    };

    configure_tab(p_theme->tabs.document);
    p_theme->tabs.document.background = { colors.control, colors.control_hovered,
      colors.control_pressed, colors.control_disabled };
    p_theme->tabs.document.border = { colors.border, colors.border_hovered,
      colors.border_pressed, colors.border_disabled };
    p_theme->tabs.document.text = { colors.text_muted, colors.text,
      colors.text, colors.text_disabled };
    p_theme->tabs.document.selected_background = { colors.surface_elevated,
      colors.surface_elevated, colors.surface_elevated, colors.control_disabled };
    p_theme->tabs.document.selected_border = { colors.border, colors.border_hovered,
      colors.border_pressed, colors.border_disabled };
    p_theme->tabs.document.selected_text = { colors.text, colors.text,
      colors.text, colors.text_disabled };
    p_theme->tabs.document.close_icon = { colors.text_muted, colors.text,
      colors.text, colors.text_disabled };

    p_theme->tabs.tool = p_theme->tabs.document;
    p_theme->tabs.tool.background = { transparent, colors.control_hovered,
      colors.control_pressed, transparent };
    p_theme->tabs.tool.border = { transparent, transparent, transparent, transparent };
    p_theme->tabs.tool.selected_background = { colors.control_pressed,
      colors.control_pressed, colors.control_pressed, colors.control_disabled };
    p_theme->tabs.tool.selected_border = p_theme->tabs.tool.border;
    p_theme->tabs.tool.tab_height = std::max(12.0f, controls.tab_height - 4.0f);
    p_theme->tabs.tool.minimum_width = std::max(1.0f, controls.tab_minimum_width - 8.0f);
    p_theme->tabs.tool.show_page_border = false;

    p_theme->tabs.segmented = p_theme->tabs.document;
    p_theme->tabs.segmented.bar_background = colors.control;
    p_theme->tabs.segmented.background = { transparent, colors.control_hovered,
      colors.control_pressed, transparent };
    p_theme->tabs.segmented.selected_background = { colors.accent,
      colors.accent_hovered, colors.accent_pressed, colors.accent_disabled };
    p_theme->tabs.segmented.selected_text = { colors.text_on_accent,
      colors.text_on_accent, colors.text_on_accent, colors.text_disabled };
    p_theme->tabs.segmented.fill_available_width = true;
    p_theme->tabs.segmented.show_page_border = false;

    p_theme->tabs.underline = p_theme->tabs.tool;
    p_theme->tabs.underline.selected_background = { transparent, colors.control_hovered,
      colors.control_pressed, transparent };
    p_theme->tabs.underline.selected_text = { colors.text, colors.text,
      colors.text, colors.text_disabled };
    p_theme->tabs.underline.show_indicator = true;

    p_theme->menu.item_background = { transparent, colors.control_hovered,
      colors.control_pressed, transparent };
    p_theme->menu.item_text = { colors.text, colors.text, colors.text,
      colors.text_disabled };
    p_theme->menu.item_icon = { colors.text_muted, colors.text, colors.text,
      colors.text_disabled };
    p_theme->menu.bar_background = colors.surface_elevated;
    p_theme->menu.popup_background = colors.surface_elevated;
    p_theme->menu.popup_border = colors.border;
    p_theme->menu.separator = colors.border;
    p_theme->menu.shadow = colors.shadow;
    p_theme->menu.bar_height = controls.menu_bar_height;
    p_theme->menu.item_height = controls.menu_item_height;
    p_theme->menu.horizontal_padding = controls.menu_horizontal_padding;
    p_theme->menu.vertical_padding = controls.menu_vertical_padding;
    p_theme->menu.popup_minimum_width = controls.menu_popup_minimum_width;
    p_theme->menu.border_width = controls.border_width;
    p_theme->menu.corner_radius = tokens.radius.medium;
    p_theme->menu.separator_thickness = controls.menu_separator_thickness;
    p_theme->menu.separator_margin = controls.menu_separator_margin;
    p_theme->menu.submenu_indicator_size = controls.menu_submenu_indicator_size;
    p_theme->menu.shadow_size = controls.menu_shadow_size;
    p_theme->menu.font_size = tokens.typography.control;

    p_theme->label.text = colors.text;
    p_theme->label.disabled_text = colors.text_disabled;
    p_theme->label.font_size = tokens.typography.body;

    p_theme->text_input.background = { colors.control, colors.control_hovered,
      colors.control_pressed, colors.control_disabled };
    p_theme->text_input.border = { colors.border, colors.border_hovered,
      colors.border_pressed, colors.border_disabled };
    p_theme->text_input.text = { colors.text, colors.text, colors.text,
      colors.text_disabled };
    p_theme->text_input.selection = colors.accent;
    p_theme->text_input.selection.a = 0.55f;
    p_theme->text_input.caret = colors.text;
    p_theme->text_input.focus_ring = colors.focus_ring;
    p_theme->text_input.corner_radius = tokens.radius.medium;
    p_theme->text_input.border_width = controls.border_width;
    p_theme->text_input.focus_ring_width = controls.focus_ring_width;
    p_theme->text_input.horizontal_padding = controls.text_input_horizontal_padding;
    p_theme->text_input.vertical_padding = controls.text_input_vertical_padding;
    p_theme->text_input.font_size = tokens.typography.control;
    p_theme->text_input.caret_width = controls.text_input_caret_width;
    p_theme->text_input.selection_corner_radius =
      controls.text_input_selection_corner_radius;

    p_theme->number_input.background = p_theme->text_input.background;
    p_theme->number_input.border = p_theme->text_input.border;
    p_theme->number_input.text = p_theme->text_input.text;
    p_theme->number_input.button_background = { colors.control,
      colors.control_hovered, colors.control_pressed, colors.control_disabled };
    p_theme->number_input.button_icon = { colors.text_muted, colors.text,
      colors.text, colors.text_disabled };
    p_theme->number_input.separator = colors.border;
    p_theme->number_input.focus_ring = colors.focus_ring;
    p_theme->number_input.corner_radius = tokens.radius.medium;
    p_theme->number_input.border_width = controls.border_width;
    p_theme->number_input.focus_ring_width = controls.focus_ring_width;
    p_theme->number_input.horizontal_padding =
      controls.number_input_horizontal_padding;
    p_theme->number_input.button_width = controls.number_input_button_width;
    p_theme->number_input.separator_width = controls.number_input_separator_width;
    p_theme->number_input.icon_size = controls.number_input_icon_size;
    p_theme->number_input.font_size = tokens.typography.control;

    p_theme->checkbox.background = { colors.control, colors.control_hovered,
      colors.control_pressed, colors.control_disabled };
    p_theme->checkbox.border = { colors.border, colors.border_hovered,
      colors.border_pressed, colors.border_disabled };
    p_theme->checkbox.mark = { colors.accent, colors.accent_hovered,
      colors.accent_pressed, colors.accent_disabled };
    p_theme->checkbox.text = { colors.text, colors.text, colors.text, colors.text_disabled };
    p_theme->checkbox.focus_ring = colors.focus_ring;
    p_theme->checkbox.box_size = controls.checkbox_size;
    p_theme->checkbox.corner_radius = tokens.radius.small;
    p_theme->checkbox.border_width = controls.border_width;
    p_theme->checkbox.mark_width = controls.checkbox_mark_width;
    p_theme->checkbox.text_gap = tokens.spacing.small;
    p_theme->checkbox.font_size = tokens.typography.control;
    p_theme->checkbox.focus_ring_width = controls.focus_ring_width;

    p_theme->radiobutton.background = { transparent, colors.control_hovered,
      colors.control_pressed, transparent };
    p_theme->radiobutton.indicator_background = { colors.control,
      colors.control_hovered, colors.control_pressed, colors.control_disabled };
    p_theme->radiobutton.indicator_border = { colors.border, colors.border_hovered,
      colors.border_pressed, colors.border_disabled };
    p_theme->radiobutton.mark = { colors.accent, colors.accent_hovered,
      colors.accent_pressed, colors.accent_disabled };
    p_theme->radiobutton.text = { colors.text, colors.text, colors.text,
      colors.text_disabled };
    p_theme->radiobutton.focus_ring = colors.focus_ring;
    p_theme->radiobutton.shadow = colors.shadow;
    p_theme->radiobutton.indicator_size = controls.radiobutton_indicator_size;
    p_theme->radiobutton.mark_radius = controls.radiobutton_mark_radius;
    p_theme->radiobutton.border_width = controls.border_width;
    p_theme->radiobutton.horizontal_padding = controls.radiobutton_horizontal_padding;
    p_theme->radiobutton.text_gap = tokens.spacing.small;
    p_theme->radiobutton.corner_radius = tokens.radius.medium;
    p_theme->radiobutton.focus_ring_width = controls.focus_ring_width;
    p_theme->radiobutton.shadow_offset = controls.radiobutton_shadow_offset;
    p_theme->radiobutton.shadow_size = controls.radiobutton_shadow_size;
    p_theme->radiobutton.font_size = tokens.typography.control;

    p_theme->combobox.field_background = { colors.control, colors.control_hovered,
      colors.control_pressed, colors.control_disabled };
    p_theme->combobox.field_border = { colors.border, colors.border_hovered,
      colors.border_pressed, colors.border_disabled };
    p_theme->combobox.field_text = { colors.text, colors.text, colors.text,
      colors.text_disabled };
    p_theme->combobox.indicator = { colors.text_muted, colors.text,
      colors.text, colors.text_disabled };
    p_theme->combobox.item_background = { transparent, colors.control_hovered,
      colors.control_pressed, transparent };
    p_theme->combobox.item_text = { colors.text, colors.text, colors.text,
      colors.text_disabled };
    p_theme->combobox.placeholder_text = colors.text_muted;
    p_theme->combobox.selected_mark = colors.accent;
    p_theme->combobox.popup_background = colors.surface_elevated;
    p_theme->combobox.popup_border = colors.border;
    p_theme->combobox.focus_ring = colors.focus_ring;
    p_theme->combobox.shadow = colors.shadow;
    p_theme->combobox.corner_radius = tokens.radius.medium;
    p_theme->combobox.border_width = controls.border_width;
    p_theme->combobox.focus_ring_width = controls.focus_ring_width;
    p_theme->combobox.horizontal_padding = controls.combobox_horizontal_padding;
    p_theme->combobox.popup_padding = controls.combobox_popup_padding;
    p_theme->combobox.popup_gap = controls.combobox_popup_gap;
    p_theme->combobox.item_height = controls.combobox_item_height;
    p_theme->combobox.indicator_size = controls.combobox_indicator_size;
    p_theme->combobox.selected_mark_width = controls.combobox_selected_mark_width;
    p_theme->combobox.shadow_size = controls.combobox_shadow_size;
    p_theme->combobox.font_size = tokens.typography.control;

    p_theme->listview.row_background = { transparent, colors.control_hovered,
      colors.control_pressed, transparent };
    p_theme->listview.row_text = { colors.text, colors.text, colors.text,
      colors.text_disabled };
    p_theme->listview.selected_background = { colors.accent,
      colors.accent_hovered, colors.accent_pressed, colors.accent_disabled };
    p_theme->listview.selected_text = { colors.text_on_accent,
      colors.text_on_accent, colors.text_on_accent, colors.text_disabled };
    p_theme->listview.background = colors.surface_elevated;
    p_theme->listview.border = colors.border;
    p_theme->listview.focus_ring = colors.focus_ring;
    p_theme->listview.row_height = controls.listview_row_height;
    p_theme->listview.horizontal_padding = controls.listview_horizontal_padding;
    p_theme->listview.vertical_padding = controls.listview_vertical_padding;
    p_theme->listview.corner_radius = tokens.radius.medium;
    p_theme->listview.row_corner_radius = tokens.radius.small;
    p_theme->listview.border_width = controls.border_width;
    p_theme->listview.focus_ring_width = controls.focus_ring_width;
    p_theme->listview.font_size = tokens.typography.control;

    p_theme->treeview.row_background = { transparent, colors.control_hovered,
      colors.control_pressed, transparent };
    p_theme->treeview.row_text = { colors.text, colors.text, colors.text,
      colors.text_disabled };
    p_theme->treeview.expander = { colors.text_muted, colors.text,
      colors.text, colors.text_disabled };
    p_theme->treeview.selected_background = { colors.control_pressed,
      colors.control_hovered, colors.control_pressed, colors.control_disabled };
    p_theme->treeview.selected_text = { colors.text, colors.text,
      colors.text, colors.text_disabled };
    p_theme->treeview.background = colors.surface_elevated;
    p_theme->treeview.border = colors.border;
    p_theme->treeview.guide = colors.border;
    p_theme->treeview.focus_ring = colors.focus_ring;
    p_theme->treeview.selection_border = colors.focus_ring;
    p_theme->treeview.row_height = controls.treeview_row_height;
    p_theme->treeview.indent = controls.treeview_indent;
    p_theme->treeview.horizontal_padding = controls.treeview_horizontal_padding;
    p_theme->treeview.selection_horizontal_padding =
      controls.treeview_selection_horizontal_padding;
    p_theme->treeview.selection_vertical_padding =
      controls.treeview_selection_vertical_padding;
    p_theme->treeview.icon_size = controls.treeview_icon_size;
    p_theme->treeview.icon_text_gap = controls.treeview_icon_text_gap;
    p_theme->treeview.expander_size = controls.treeview_expander_size;
    p_theme->treeview.expander_stroke_width =
      controls.treeview_expander_stroke_width;
    p_theme->treeview.corner_radius = tokens.radius.medium;
    p_theme->treeview.row_corner_radius = tokens.radius.small;
    p_theme->treeview.border_width = controls.border_width;
    p_theme->treeview.focus_ring_width = controls.focus_ring_width;
    p_theme->treeview.selection_border_width = controls.focus_ring_width;
    p_theme->treeview.font_size = tokens.typography.control;
    p_theme->treeview.draw_background = controls.treeview_draw_background;
    p_theme->treeview.draw_border = controls.treeview_draw_border;

    p_theme->propertyview.row_background = { colors.surface_elevated,
      colors.control_hovered, colors.control_pressed, colors.control_disabled };
    p_theme->propertyview.value_background = { colors.control,
      colors.control_hovered, colors.control_pressed, colors.control_disabled };
    p_theme->propertyview.background = colors.surface_elevated;
    p_theme->propertyview.border = colors.border;
    p_theme->propertyview.grid = colors.border;
    p_theme->propertyview.name_text = colors.text_muted;
    p_theme->propertyview.value_text = colors.text;
    p_theme->propertyview.group_background = colors.control_pressed;
    p_theme->propertyview.group_text = colors.text;
    p_theme->propertyview.invalid_background = NVGcolor::RGBAf(
      colors.danger.r, colors.danger.g, colors.danger.b, 0.16f);
    p_theme->propertyview.invalid_border = colors.danger;
    p_theme->propertyview.invalid_text = colors.danger_hovered;
    p_theme->propertyview.focus_ring = colors.focus_ring;
    p_theme->propertyview.choice_indicator = colors.text_muted;
    p_theme->propertyview.row_height = controls.propertyview_row_height;
    p_theme->propertyview.group_height = controls.propertyview_group_height;
    p_theme->propertyview.name_column_ratio =
      controls.propertyview_name_column_ratio;
    p_theme->propertyview.horizontal_padding =
      controls.propertyview_horizontal_padding;
    p_theme->propertyview.editor_padding = controls.propertyview_editor_padding;
    p_theme->propertyview.border_width = controls.border_width;
    p_theme->propertyview.grid_width = controls.propertyview_grid_width;
    p_theme->propertyview.focus_ring_width = controls.focus_ring_width;
    p_theme->propertyview.corner_radius = tokens.radius.small;
    p_theme->propertyview.font_size = tokens.typography.caption;
    p_theme->propertyview.group_font_size = tokens.typography.caption;
    p_theme->propertyview.choice_indicator_size =
      controls.propertyview_choice_indicator_size;

    auto compile_toolstrip = [&](RmToolStripStyle& style, bool exclusive) {
      style.button_background = { colors.control, colors.control_hovered,
        colors.control_pressed, colors.control_disabled };
      style.button_border = { colors.border, colors.border_hovered,
        colors.border_pressed, colors.border_disabled };
      style.button_text = { colors.text, colors.text, colors.text,
        colors.text_disabled };
      style.background = colors.surface_elevated;
      style.border = colors.border;
      style.selected_background = exclusive ? colors.control_pressed : colors.accent;
      style.selected_border = colors.focus_ring;
      style.group_text = colors.text_muted;
      style.separator = colors.border;
      style.button_extent = controls.toolstrip_button_extent;
      style.button_gap = controls.toolstrip_button_gap;
      style.group_gap = controls.toolstrip_group_gap;
      style.group_label_height = controls.toolstrip_group_label_height;
      style.group_padding = controls.toolstrip_group_padding;
      style.corner_radius = tokens.radius.small;
      style.border_width = controls.border_width;
      style.selected_border_width = controls.focus_ring_width;
      style.font_size = tokens.typography.caption;
      style.group_font_size = tokens.typography.caption;
    };
    compile_toolstrip(p_theme->toolbar, false);
    compile_toolstrip(p_theme->toolbox, true);

    p_theme->rebar.background = colors.surface_elevated;
    p_theme->rebar.border = colors.border;
    p_theme->rebar.separator = colors.border;
    p_theme->rebar.gripper = colors.text_muted;
    p_theme->rebar.gripper_hovered = colors.control_hovered;
    p_theme->rebar.gripper_active = colors.accent;
    p_theme->rebar.resize_handle = colors.border_hovered;
    p_theme->rebar.band_gap = controls.rebar_band_gap;
    p_theme->rebar.band_padding = controls.rebar_band_padding;
    p_theme->rebar.row_gap = controls.rebar_row_gap;
    p_theme->rebar.gripper_extent = controls.rebar_gripper_extent;
    p_theme->rebar.resize_handle_extent = controls.rebar_resize_handle_extent;
    p_theme->rebar.border_width = controls.border_width;
    p_theme->rebar.separator_width = controls.border_width;
    p_theme->rebar.corner_radius = tokens.radius.small;

    p_theme->splitter.background = { colors.border, colors.border_hovered,
      colors.border_pressed, colors.border_disabled };
    p_theme->splitter.grip = colors.text_muted;
    p_theme->splitter.focus_ring = colors.focus_ring;
    p_theme->splitter.thickness = controls.splitter_thickness;
    p_theme->splitter.grip_extent = controls.splitter_grip_extent;
    p_theme->splitter.grip_width = controls.splitter_grip_width;
    p_theme->splitter.focus_ring_width = controls.focus_ring_width;
    p_theme->splitter.corner_radius = tokens.radius.small;

    p_theme->output_text.background = colors.surface_elevated;
    p_theme->output_text.border = colors.border;
    p_theme->output_text.text = colors.text_muted;
    p_theme->output_text.focus_ring = colors.focus_ring;
    p_theme->output_text.line_height = controls.output_text_line_height;
    p_theme->output_text.horizontal_padding =
      controls.output_text_horizontal_padding;
    p_theme->output_text.vertical_padding = controls.output_text_vertical_padding;
    p_theme->output_text.corner_radius = tokens.radius.medium;
    p_theme->output_text.border_width = controls.border_width;
    p_theme->output_text.focus_ring_width = controls.focus_ring_width;
    p_theme->output_text.font_size = tokens.typography.caption;

    p_theme->slider.track = { colors.control, colors.control_hovered,
      colors.control_pressed, colors.control_disabled };
    p_theme->slider.fill = { colors.accent, colors.accent_hovered,
      colors.accent_pressed, colors.accent_disabled };
    p_theme->slider.thumb = { colors.text_on_accent, colors.text_on_accent,
      colors.control_pressed, colors.text_disabled };
    p_theme->slider.thumb_border = { colors.accent, colors.accent_hovered,
      colors.accent_pressed, colors.border_disabled };
    p_theme->slider.focus_ring = colors.focus_ring;
    p_theme->slider.track_height = controls.slider_track_height;
    p_theme->slider.padding = controls.slider_padding;
    p_theme->slider.thumb_radius = controls.slider_thumb_radius;
    p_theme->slider.thumb_border_width = controls.slider_thumb_border_width;
    p_theme->slider.focus_ring_width = controls.focus_ring_width;

    p_theme->scrollbar.track = { colors.control, colors.control_hovered,
      colors.control_pressed, colors.control_disabled };
    p_theme->scrollbar.track_border = { colors.border, colors.border_hovered,
      colors.border_pressed, colors.border_disabled };
    p_theme->scrollbar.thumb = { colors.text_muted, colors.text,
      colors.accent_pressed, colors.text_disabled };
    p_theme->scrollbar.thumb_border = { colors.border_hovered, colors.focus_ring,
      colors.accent_pressed, colors.border_disabled };
    p_theme->scrollbar.focus_ring = colors.focus_ring;
    p_theme->scrollbar.thickness = controls.scrollbar_thickness;
    p_theme->scrollbar.minimum_thumb_length = controls.scrollbar_minimum_thumb_length;
    p_theme->scrollbar.padding = controls.scrollbar_padding;
    p_theme->scrollbar.corner_radius = tokens.radius.small;
    p_theme->scrollbar.border_width = controls.border_width;
    p_theme->scrollbar.thumb_border_width = controls.scrollbar_thumb_border_width;
    p_theme->scrollbar.focus_ring_width = controls.focus_ring_width;

    p_theme->progress.background = { colors.control, colors.control,
      colors.control, colors.control_disabled };
    p_theme->progress.fill_start = { colors.accent, colors.accent,
      colors.accent, colors.accent_disabled };
    p_theme->progress.fill_end = { colors.accent_secondary, colors.accent_secondary,
      colors.accent_secondary, colors.accent_disabled };
    p_theme->progress.corner_radius = controls.progress_corner_radius;

    p_theme->switch_control.track_off = { colors.control, colors.control_hovered,
      colors.control_pressed, colors.control_disabled };
    p_theme->switch_control.track_on = { colors.accent, colors.accent_hovered,
      colors.accent_pressed, colors.accent_disabled };
    p_theme->switch_control.knob = { colors.text_on_accent, colors.text_on_accent,
      colors.control_pressed, colors.text_disabled };
    p_theme->switch_control.focus_ring = colors.focus_ring;
    p_theme->switch_control.shadow = colors.shadow;
    p_theme->switch_control.track_height = controls.switch_track_height;
    p_theme->switch_control.padding = controls.switch_padding;
    p_theme->switch_control.knob_radius = controls.switch_knob_radius;
    p_theme->switch_control.corner_radius = controls.switch_track_height * 0.5f;
    p_theme->switch_control.animation_duration = tokens.animation.normal;
    p_theme->switch_control.shadow_offset = controls.switch_shadow_offset;
    p_theme->switch_control.shadow_size = controls.switch_shadow_size;
    p_theme->switch_control.focus_ring_width = controls.focus_ring_width;

    result.theme = std::move(p_theme);
    return result;
  }
};

inline RmThemeDocument RmThemeDocument::dark_theme()
{
  RmThemeDocument document;
  document.name = "RmGUI CAD Dark";
  document.mode = RmThemeMode::dark;
  RmColorTokens& colors = document.tokens.colors;
  colors.accent = NVGcolor::RGBA(24, 126, 188, 255);
  colors.accent_hovered = NVGcolor::RGBA(39, 145, 207, 255);
  colors.accent_pressed = NVGcolor::RGBA(17, 99, 153, 255);
  colors.accent_disabled = NVGcolor::RGBA(73, 102, 119, 255);
  colors.accent_secondary = NVGcolor::RGBA(151, 89, 232, 255);
  colors.danger = NVGcolor::RGBA(196, 54, 75, 255);
  colors.danger_hovered = NVGcolor::RGBA(220, 68, 89, 255);
  colors.danger_pressed = NVGcolor::RGBA(161, 41, 60, 255);
  colors.danger_disabled = NVGcolor::RGBA(111, 67, 75, 255);
  colors.surface = NVGcolor::RGBA(37, 38, 40, 255);
  colors.surface_elevated = NVGcolor::RGBA(51, 52, 54, 255);
  colors.control = NVGcolor::RGBA(63, 64, 66, 255);
  colors.control_hovered = NVGcolor::RGBA(75, 76, 79, 255);
  colors.control_pressed = NVGcolor::RGBA(54, 55, 57, 255);
  colors.control_disabled = NVGcolor::RGBA(57, 58, 60, 255);
  colors.border = NVGcolor::RGBA(91, 93, 96, 255);
  colors.border_hovered = NVGcolor::RGBA(121, 151, 169, 255);
  colors.border_pressed = NVGcolor::RGBA(39, 129, 183, 255);
  colors.border_disabled = NVGcolor::RGBA(73, 74, 77, 255);
  colors.text = NVGcolor::RGBA(232, 233, 235, 255);
  colors.text_muted = NVGcolor::RGBA(178, 180, 183, 255);
  colors.text_disabled = NVGcolor::RGBA(126, 128, 131, 255);
  colors.text_on_accent = NVGcolor::RGBA(255, 255, 255, 255);
  colors.focus_ring = NVGcolor::RGBA(82, 181, 230, 255);
  colors.shadow = NVGcolor::RGBA(0, 0, 0, 90);
  return document;
}

inline RmThemeDocument RmThemeDocument::light_theme()
{
  RmThemeDocument document;
  document.name = "RmGUI Light";
  document.mode = RmThemeMode::light;
  RmColorTokens& colors = document.tokens.colors;
  colors.accent = NVGcolor::RGBA(50, 83, 210, 255);
  colors.accent_hovered = NVGcolor::RGBA(65, 101, 225, 255);
  colors.accent_pressed = NVGcolor::RGBA(39, 64, 169, 255);
  colors.accent_disabled = NVGcolor::RGBA(145, 155, 197, 255);
  colors.accent_secondary = NVGcolor::RGBA(125, 69, 190, 255);
  colors.danger = NVGcolor::RGBA(190, 42, 61, 255);
  colors.danger_hovered = NVGcolor::RGBA(211, 52, 72, 255);
  colors.danger_pressed = NVGcolor::RGBA(151, 30, 47, 255);
  colors.danger_disabled = NVGcolor::RGBA(199, 144, 151, 255);
  colors.surface = NVGcolor::RGBA(245, 247, 251, 255);
  colors.surface_elevated = NVGcolor::RGBA(255, 255, 255, 255);
  colors.control = NVGcolor::RGBA(255, 255, 255, 255);
  colors.control_hovered = NVGcolor::RGBA(246, 248, 255, 255);
  colors.control_pressed = NVGcolor::RGBA(230, 235, 252, 255);
  colors.control_disabled = NVGcolor::RGBA(232, 234, 239, 255);
  colors.border = NVGcolor::RGBA(164, 170, 184, 255);
  colors.border_hovered = NVGcolor::RGBA(77, 105, 215, 255);
  colors.border_pressed = NVGcolor::RGBA(50, 83, 210, 255);
  colors.border_disabled = NVGcolor::RGBA(195, 198, 205, 255);
  colors.text = NVGcolor::RGBA(31, 34, 42, 255);
  colors.text_muted = NVGcolor::RGBA(91, 96, 109, 255);
  colors.text_disabled = NVGcolor::RGBA(142, 146, 156, 255);
  colors.text_on_accent = NVGcolor::RGBA(255, 255, 255, 255);
  colors.focus_ring = NVGcolor::RGBA(74, 124, 237, 220);
  colors.shadow = NVGcolor::RGBA(21, 27, 43, 55);
  return document;
}

inline RmThemeRef RmThemeSnapshot::default_theme()
{
  static const RmThemeRef theme = RmThemeCompiler::compile(
    RmThemeDocument::dark_theme()).theme;
  return theme;
}
