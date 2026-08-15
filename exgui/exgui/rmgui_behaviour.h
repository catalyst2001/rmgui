#pragma once

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <limits>

// Renderer-free interaction state machines. These classes intentionally do
// not depend on rm_widget, NanoVG, platform key codes, or style data.

struct RmBehaviourUpdate {
  bool handled = false;
  bool state_changed = false;
  bool activated = false;
};

class RmButtonBehaviour {
  bool enabled_ = true;
  bool hovered_ = false;
  bool pressed_ = false;
  bool keyboard_pressed_ = false;

public:
  bool is_enabled() const noexcept { return enabled_; }
  bool is_hovered() const noexcept { return hovered_; }
  bool is_pressed() const noexcept { return pressed_ || keyboard_pressed_; }

  RmBehaviourUpdate set_enabled(bool enabled) noexcept {
    const bool changed = enabled_ != enabled || (!enabled && is_pressed());
    enabled_ = enabled;
    if (!enabled_) {
      pressed_ = false;
      keyboard_pressed_ = false;
    }
    return { false, changed, false };
  }

  RmBehaviourUpdate pointer_move(bool inside) noexcept {
    const bool changed = hovered_ != inside;
    hovered_ = inside;
    return { enabled_ && pressed_, changed, false };
  }

  RmBehaviourUpdate pointer_down(bool inside) noexcept {
    if (!enabled_ || !inside)
      return {};

    const bool changed = !pressed_ || !hovered_;
    hovered_ = true;
    pressed_ = true;
    return { true, changed, false };
  }

  RmBehaviourUpdate pointer_up(bool inside) noexcept {
    if (!pressed_)
      return {};

    const bool activated = enabled_ && inside;
    pressed_ = false;
    hovered_ = inside;
    return { true, true, activated };
  }

  RmBehaviourUpdate key_down(bool activation_key) noexcept {
    if (!enabled_ || !activation_key)
      return {};

    const bool changed = !keyboard_pressed_;
    keyboard_pressed_ = true;
    return { true, changed, false };
  }

  RmBehaviourUpdate key_up(bool activation_key) noexcept {
    if (!activation_key || !keyboard_pressed_)
      return {};

    keyboard_pressed_ = false;
    return { true, true, enabled_ };
  }

  RmBehaviourUpdate cancel() noexcept {
    const bool changed = pressed_ || keyboard_pressed_;
    pressed_ = false;
    keyboard_pressed_ = false;
    return { changed, changed, false };
  }
};

class RmToggleBehaviour {
  RmButtonBehaviour button_;
  bool checked_ = false;

  RmBehaviourUpdate apply_activation(RmBehaviourUpdate update) noexcept {
    if (update.activated) {
      checked_ = !checked_;
      update.state_changed = true;
    }
    return update;
  }

public:
  explicit RmToggleBehaviour(bool checked = false) noexcept : checked_(checked) {}

  bool is_enabled() const noexcept { return button_.is_enabled(); }
  bool is_hovered() const noexcept { return button_.is_hovered(); }
  bool is_pressed() const noexcept { return button_.is_pressed(); }
  bool is_checked() const noexcept { return checked_; }

  RmBehaviourUpdate set_enabled(bool enabled) noexcept { return button_.set_enabled(enabled); }

  RmBehaviourUpdate set_checked(bool checked) noexcept {
    const bool changed = checked_ != checked;
    checked_ = checked;
    return { false, changed, false };
  }

  RmBehaviourUpdate pointer_move(bool inside) noexcept { return button_.pointer_move(inside); }
  RmBehaviourUpdate pointer_down(bool inside) noexcept { return button_.pointer_down(inside); }
  RmBehaviourUpdate pointer_up(bool inside) noexcept { return apply_activation(button_.pointer_up(inside)); }
  RmBehaviourUpdate key_down(bool activation_key) noexcept { return button_.key_down(activation_key); }
  RmBehaviourUpdate key_up(bool activation_key) noexcept { return apply_activation(button_.key_up(activation_key)); }
  RmBehaviourUpdate cancel() noexcept { return button_.cancel(); }
};

class RmRadioButtonBehaviour {
  RmButtonBehaviour m_button;
  bool m_checked = false;
  bool m_allow_uncheck = false;

  RmBehaviourUpdate apply_activation(RmBehaviourUpdate update) noexcept {
    if (!update.activated)
      return update;
    if (m_checked && !m_allow_uncheck) {
      update.activated = false;
      return update;
    }
    m_checked = !m_checked;
    update.state_changed = true;
    return update;
  }

public:
  explicit RmRadioButtonBehaviour(bool checked = false) noexcept :
    m_checked(checked) {}

  bool is_checked() const noexcept { return m_checked; }
  bool is_hovered() const noexcept { return m_button.is_hovered(); }
  bool is_pressed() const noexcept { return m_button.is_pressed(); }
  bool allows_uncheck() const noexcept { return m_allow_uncheck; }

  RmBehaviourUpdate set_enabled(bool enabled) noexcept {
    return m_button.set_enabled(enabled);
  }
  RmBehaviourUpdate set_checked(bool checked) noexcept {
    const bool changed = m_checked != checked;
    m_checked = checked;
    return { false, changed, false };
  }
  RmBehaviourUpdate set_allow_uncheck(bool allow) noexcept {
    const bool changed = m_allow_uncheck != allow;
    m_allow_uncheck = allow;
    return { false, changed, false };
  }
  RmBehaviourUpdate pointer_move(bool inside) noexcept {
    return m_button.pointer_move(inside);
  }
  RmBehaviourUpdate pointer_down(bool inside) noexcept {
    return m_button.pointer_down(inside);
  }
  RmBehaviourUpdate pointer_up(bool inside) noexcept {
    return apply_activation(m_button.pointer_up(inside));
  }
  RmBehaviourUpdate key_down(bool activation_key) noexcept {
    return m_button.key_down(activation_key);
  }
  RmBehaviourUpdate key_up(bool activation_key) noexcept {
    return apply_activation(m_button.key_up(activation_key));
  }
  RmBehaviourUpdate cancel() noexcept { return m_button.cancel(); }
};

class RmListViewBehaviour {
public:
  static constexpr size_t invalid_index = std::numeric_limits<size_t>::max();

private:
  size_t m_count = 0;
  size_t m_hovered = invalid_index;
  size_t m_pressed = invalid_index;
  size_t m_selected = invalid_index;
  bool m_enabled = true;

  bool is_valid(size_t index) const noexcept { return index < m_count; }

public:
  size_t count() const noexcept { return m_count; }
  size_t hovered_index() const noexcept { return m_hovered; }
  size_t pressed_index() const noexcept { return m_pressed; }
  size_t selected_index() const noexcept { return m_selected; }

  RmBehaviourUpdate set_enabled(bool enabled) noexcept {
    const bool changed = m_enabled != enabled || (!enabled && m_pressed != invalid_index);
    m_enabled = enabled;
    if (!m_enabled) {
      m_hovered = invalid_index;
      m_pressed = invalid_index;
    }
    return { false, changed, false };
  }

  RmBehaviourUpdate set_count(size_t count) noexcept {
    m_count = count;
    const size_t old_hovered = m_hovered;
    const size_t old_pressed = m_pressed;
    const size_t old_selected = m_selected;
    if (!is_valid(m_hovered)) m_hovered = invalid_index;
    if (!is_valid(m_pressed)) m_pressed = invalid_index;
    if (!is_valid(m_selected)) m_selected = invalid_index;
    const bool changed = old_hovered != m_hovered || old_pressed != m_pressed ||
      old_selected != m_selected;
    return { false, changed, false };
  }

  RmBehaviourUpdate pointer_move(size_t index) noexcept {
    if (!is_valid(index)) index = invalid_index;
    const bool changed = m_hovered != index;
    m_hovered = index;
    return { m_enabled && m_pressed != invalid_index, changed, false };
  }

  RmBehaviourUpdate pointer_down(size_t index) noexcept {
    if (!m_enabled || !is_valid(index))
      return {};
    const bool changed = m_pressed != index || m_hovered != index;
    m_pressed = index;
    m_hovered = index;
    return { true, changed, false };
  }

  RmBehaviourUpdate pointer_up(size_t index) noexcept {
    if (m_pressed == invalid_index)
      return {};
    const size_t pressed = m_pressed;
    m_pressed = invalid_index;
    if (!is_valid(index)) index = invalid_index;
    m_hovered = index;
    const bool activated = m_enabled && pressed == index;
    if (activated)
      m_selected = index;
    return { true, true, activated };
  }

  RmBehaviourUpdate select(size_t index) noexcept {
    if (!is_valid(index))
      return {};
    const bool changed = m_selected != index;
    m_selected = index;
    return { true, changed, false };
  }

  RmBehaviourUpdate clear_selection() noexcept {
    const bool changed = m_selected != invalid_index;
    m_selected = invalid_index;
    return { false, changed, false };
  }

  RmBehaviourUpdate select_relative(int delta, bool wrap = false) noexcept {
    if (!m_enabled || m_count == 0 || delta == 0)
      return {};
    int next = is_valid(m_selected)
      ? static_cast<int>(m_selected) + delta
      : (delta > 0 ? 0 : static_cast<int>(m_count) - 1);
    const int count = static_cast<int>(m_count);
    if (wrap) {
      next %= count;
      if (next < 0) next += count;
    }
    else {
      next = std::clamp(next, 0, count - 1);
    }
    const RmBehaviourUpdate update = select(static_cast<size_t>(next));
    return { update.handled, update.state_changed, update.state_changed };
  }

  RmBehaviourUpdate cancel() noexcept {
    const bool changed = m_pressed != invalid_index;
    m_pressed = invalid_index;
    return { changed, changed, false };
  }
};

class RmComboBoxBehaviour {
public:
  static constexpr size_t invalid_index = std::numeric_limits<size_t>::max();

private:
  size_t m_count = 0;
  size_t m_selected = invalid_index;
  size_t m_highlighted = invalid_index;
  bool m_expanded = false;
  bool m_enabled = true;

  bool is_valid(size_t index) const noexcept { return index < m_count; }

public:
  size_t count() const noexcept { return m_count; }
  size_t selected_index() const noexcept { return m_selected; }
  size_t highlighted_index() const noexcept { return m_highlighted; }
  bool is_expanded() const noexcept { return m_expanded; }

  RmBehaviourUpdate set_enabled(bool enabled) noexcept {
    const bool changed = m_enabled != enabled || (!enabled && m_expanded);
    m_enabled = enabled;
    if (!m_enabled) {
      m_expanded = false;
      m_highlighted = invalid_index;
    }
    return { false, changed, false };
  }

  RmBehaviourUpdate set_count(size_t count) noexcept {
    const size_t previous_selected = m_selected;
    const size_t previous_highlighted = m_highlighted;
    const bool previous_expanded = m_expanded;
    m_count = count;
    if (m_count == 0) {
      m_selected = invalid_index;
      m_highlighted = invalid_index;
      m_expanded = false;
    }
    else {
      if (!is_valid(m_selected))
        m_selected = 0;
      if (!is_valid(m_highlighted))
        m_highlighted = m_selected;
    }
    const bool changed = previous_selected != m_selected ||
      previous_highlighted != m_highlighted || previous_expanded != m_expanded;
    return { false, changed, false };
  }

  RmBehaviourUpdate select(size_t index) noexcept {
    if (!is_valid(index))
      return {};
    const bool changed = m_selected != index || m_highlighted != index;
    m_selected = index;
    m_highlighted = index;
    return { true, changed, false };
  }

  RmBehaviourUpdate open() noexcept {
    if (!m_enabled || m_count == 0)
      return {};
    const bool changed = !m_expanded || m_highlighted != m_selected;
    m_expanded = true;
    m_highlighted = m_selected;
    return { true, changed, false };
  }

  RmBehaviourUpdate close() noexcept {
    const bool changed = m_expanded;
    m_expanded = false;
    m_highlighted = m_selected;
    return { changed, changed, false };
  }

  RmBehaviourUpdate toggle() noexcept {
    return m_expanded ? close() : open();
  }

  RmBehaviourUpdate highlight(size_t index) noexcept {
    if (!m_enabled || !m_expanded)
      return {};
    if (!is_valid(index))
      index = invalid_index;
    const bool changed = m_highlighted != index;
    m_highlighted = index;
    return { true, changed, false };
  }

  RmBehaviourUpdate highlight_relative(int delta, bool wrap = true) noexcept {
    if (!m_enabled || !m_expanded || m_count == 0 || delta == 0)
      return {};
    int next = is_valid(m_highlighted)
      ? static_cast<int>(m_highlighted) + delta
      : (delta > 0 ? 0 : static_cast<int>(m_count) - 1);
    const int count = static_cast<int>(m_count);
    if (wrap) {
      next %= count;
      if (next < 0)
        next += count;
    }
    else {
      next = std::clamp(next, 0, count - 1);
    }
    return highlight(static_cast<size_t>(next));
  }

  RmBehaviourUpdate select_relative(int delta, bool wrap = true) noexcept {
    if (!m_enabled || m_count == 0 || delta == 0)
      return {};
    int next = is_valid(m_selected)
      ? static_cast<int>(m_selected) + delta
      : (delta > 0 ? 0 : static_cast<int>(m_count) - 1);
    const int count = static_cast<int>(m_count);
    if (wrap) {
      next %= count;
      if (next < 0)
        next += count;
    }
    else {
      next = std::clamp(next, 0, count - 1);
    }
    const RmBehaviourUpdate update = select(static_cast<size_t>(next));
    return { update.handled, update.state_changed, update.state_changed };
  }

  RmBehaviourUpdate commit_highlighted() noexcept {
    if (!m_enabled || !m_expanded || !is_valid(m_highlighted))
      return {};
    const bool changed = m_selected != m_highlighted;
    m_selected = m_highlighted;
    m_expanded = false;
    return { true, true, changed };
  }
};

class RmSliderBehaviour {
  float minimum_ = 0.0f;
  float maximum_ = 1.0f;
  float value_ = 0.0f;
  bool enabled_ = true;
  bool dragging_ = false;

  static bool different(float lhs, float rhs) noexcept {
    return std::fabs(lhs - rhs) > 1.0e-6f;
  }

public:
  RmSliderBehaviour(float minimum, float maximum, float value) noexcept {
    set_range(minimum, maximum);
    set_value(value);
  }

  float minimum() const noexcept { return minimum_; }
  float maximum() const noexcept { return maximum_; }
  float value() const noexcept { return value_; }
  bool is_enabled() const noexcept { return enabled_; }
  bool is_dragging() const noexcept { return dragging_; }

  float fraction() const noexcept {
    const float range = maximum_ - minimum_;
    return range > 1.0e-6f ? (value_ - minimum_) / range : 0.0f;
  }

  RmBehaviourUpdate set_enabled(bool enabled) noexcept {
    const bool changed = enabled_ != enabled || (!enabled && dragging_);
    enabled_ = enabled;
    if (!enabled_)
      dragging_ = false;
    return { false, changed, false };
  }

  RmBehaviourUpdate set_range(float minimum, float maximum) noexcept {
    if (maximum < minimum)
      std::swap(minimum, maximum);

    const bool range_changed = different(minimum_, minimum) || different(maximum_, maximum);
    minimum_ = minimum;
    maximum_ = maximum;
    const float clamped = std::clamp(value_, minimum_, maximum_);
    const bool value_changed = different(value_, clamped);
    value_ = clamped;
    return { false, range_changed || value_changed, false };
  }

  RmBehaviourUpdate set_value(float value) noexcept {
    const float clamped = std::clamp(value, minimum_, maximum_);
    const bool changed = different(value_, clamped);
    value_ = clamped;
    return { false, changed, false };
  }

  RmBehaviourUpdate begin_drag(float fraction) noexcept {
    if (!enabled_)
      return {};

    dragging_ = true;
    auto update = set_fraction(fraction);
    update.handled = true;
    update.state_changed = true;
    return update;
  }

  RmBehaviourUpdate drag_to(float fraction) noexcept {
    if (!enabled_ || !dragging_)
      return {};

    auto update = set_fraction(fraction);
    update.handled = true;
    return update;
  }

  RmBehaviourUpdate end_drag() noexcept {
    if (!dragging_)
      return {};

    dragging_ = false;
    return { true, true, false };
  }

  RmBehaviourUpdate cancel() noexcept { return end_drag(); }

private:
  RmBehaviourUpdate set_fraction(float fraction) noexcept {
    fraction = std::clamp(fraction, 0.0f, 1.0f);
    return set_value(minimum_ + (maximum_ - minimum_) * fraction);
  }
};

class RmScrollbarBehaviour {
  float m_position = 0.0f;
  float m_viewport_fraction = 1.0f;
  float m_drag_offset = 0.0f;
  bool m_dragging = false;
  bool m_enabled = true;

public:
  explicit RmScrollbarBehaviour(float position = 0.0f) noexcept {
    set_position(position);
  }

  float position() const noexcept { return m_position; }
  float viewport_fraction() const noexcept { return m_viewport_fraction; }
  bool is_dragging() const noexcept { return m_dragging; }

  float thumb_length(float track_length, float minimum_length) const noexcept {
    track_length = std::max(0.0f, track_length);
    return std::min(track_length, std::max(minimum_length,
      track_length * m_viewport_fraction));
  }

  float thumb_offset(float track_length, float minimum_length) const noexcept {
    const float available = std::max(0.0f,
      track_length - thumb_length(track_length, minimum_length));
    return available * m_position;
  }

  RmBehaviourUpdate set_enabled(bool enabled) noexcept {
    const bool changed = m_enabled != enabled || (!enabled && m_dragging);
    m_enabled = enabled;
    if (!m_enabled)
      m_dragging = false;
    return { false, changed, false };
  }

  RmBehaviourUpdate set_position(float position) noexcept {
    const float clamped = std::clamp(position, 0.0f, 1.0f);
    const bool changed = std::fabs(m_position - clamped) > 1.0e-6f;
    m_position = clamped;
    return { false, changed, false };
  }

  RmBehaviourUpdate set_viewport_fraction(float fraction) noexcept {
    const float clamped = std::clamp(fraction, 0.0f, 1.0f);
    const bool changed = std::fabs(m_viewport_fraction - clamped) > 1.0e-6f;
    m_viewport_fraction = clamped;
    if (m_viewport_fraction >= 1.0f)
      m_position = 0.0f;
    return { false, changed, false };
  }

  RmBehaviourUpdate begin_drag(float pointer, float track_length,
    float minimum_length) noexcept {
    if (!m_enabled || track_length <= 0.0f || m_viewport_fraction >= 1.0f)
      return {};
    const float length = thumb_length(track_length, minimum_length);
    const float offset = thumb_offset(track_length, minimum_length);
    if (pointer >= offset && pointer <= offset + length) {
      m_drag_offset = pointer - offset;
    }
    else {
      m_drag_offset = length * 0.5f;
      const float available = track_length - length;
      if (available > 0.0f)
        set_position((pointer - m_drag_offset) / available);
    }
    m_dragging = true;
    return { true, true, false };
  }

  RmBehaviourUpdate drag_to(float pointer, float track_length,
    float minimum_length) noexcept {
    if (!m_enabled || !m_dragging)
      return {};
    const float available = track_length - thumb_length(track_length, minimum_length);
    const RmBehaviourUpdate update = available > 0.0f
      ? set_position((pointer - m_drag_offset) / available)
      : set_position(0.0f);
    return { true, update.state_changed, false };
  }

  RmBehaviourUpdate end_drag() noexcept {
    if (!m_dragging)
      return {};
    m_dragging = false;
    return { true, true, false };
  }

  RmBehaviourUpdate step(float delta) noexcept {
    if (!m_enabled)
      return {};
    const RmBehaviourUpdate update = set_position(m_position + delta);
    return { true, update.state_changed, update.state_changed };
  }

  RmBehaviourUpdate cancel() noexcept { return end_drag(); }
};

class RmProgressBehaviour {
  float percent_ = 0.0f;

public:
  explicit RmProgressBehaviour(float percent = 0.0f) noexcept {
    set_percent(percent);
  }

  float percent() const noexcept { return percent_; }
  float fraction() const noexcept { return percent_ / 100.0f; }

  RmBehaviourUpdate set_percent(float percent) noexcept {
    const float clamped = std::clamp(percent, 0.0f, 100.0f);
    const bool changed = std::fabs(percent_ - clamped) > 1.0e-6f;
    percent_ = clamped;
    return { false, changed, false };
  }
};

class RmSwitchBehaviour {
  RmToggleBehaviour toggle_;
  float animation_progress_ = 0.0f;
  float animation_target_ = 0.0f;

  RmBehaviourUpdate sync_activation(RmBehaviourUpdate update) noexcept {
    if (update.activated)
      animation_target_ = toggle_.is_checked() ? 1.0f : 0.0f;
    return update;
  }

public:
  explicit RmSwitchBehaviour(bool on = false) noexcept
    : toggle_(on), animation_progress_(on ? 1.0f : 0.0f),
      animation_target_(animation_progress_) {}

  bool is_enabled() const noexcept { return toggle_.is_enabled(); }
  bool is_hovered() const noexcept { return toggle_.is_hovered(); }
  bool is_pressed() const noexcept { return toggle_.is_pressed(); }
  bool is_on() const noexcept { return toggle_.is_checked(); }
  float animation_progress() const noexcept { return animation_progress_; }

  RmBehaviourUpdate set_enabled(bool enabled) noexcept {
    return toggle_.set_enabled(enabled);
  }

  RmBehaviourUpdate set_on(bool on, bool animate = true) noexcept {
    auto update = toggle_.set_checked(on);
    animation_target_ = on ? 1.0f : 0.0f;
    if (!animate)
      animation_progress_ = animation_target_;
    return update;
  }

  RmBehaviourUpdate pointer_move(bool inside) noexcept {
    return toggle_.pointer_move(inside);
  }
  RmBehaviourUpdate pointer_down(bool inside) noexcept {
    return toggle_.pointer_down(inside);
  }
  RmBehaviourUpdate pointer_up(bool inside) noexcept {
    return sync_activation(toggle_.pointer_up(inside));
  }
  RmBehaviourUpdate key_down(bool activation_key) noexcept {
    return toggle_.key_down(activation_key);
  }
  RmBehaviourUpdate key_up(bool activation_key) noexcept {
    return sync_activation(toggle_.key_up(activation_key));
  }
  RmBehaviourUpdate cancel() noexcept { return toggle_.cancel(); }

  RmBehaviourUpdate advance(float delta_seconds, float duration_seconds) noexcept {
    if (std::fabs(animation_progress_ - animation_target_) <= 1.0e-6f)
      return {};

    duration_seconds = std::max(duration_seconds, 1.0e-4f);
    const float step = std::max(delta_seconds, 0.0f) / duration_seconds;
    if (animation_progress_ < animation_target_)
      animation_progress_ = std::min(animation_progress_ + step, animation_target_);
    else
      animation_progress_ = std::max(animation_progress_ - step, animation_target_);
    return { false, true, false };
  }
};

class RmTabBehaviour {
public:
  static constexpr size_t invalid_index = std::numeric_limits<size_t>::max();

private:
  size_t m_count = 0;
  size_t m_selected = invalid_index;
  size_t m_hovered = invalid_index;
  size_t m_pressed = invalid_index;
  bool m_enabled = true;

  bool is_valid(size_t index) const noexcept { return index < m_count; }

public:
  size_t count() const noexcept { return m_count; }
  size_t selected_index() const noexcept { return m_selected; }
  size_t hovered_index() const noexcept { return m_hovered; }
  size_t pressed_index() const noexcept { return m_pressed; }
  bool is_enabled() const noexcept { return m_enabled; }

  RmBehaviourUpdate set_enabled(bool enabled) noexcept {
    const bool changed = m_enabled != enabled || (!enabled && m_pressed != invalid_index);
    m_enabled = enabled;
    if (!m_enabled)
      m_pressed = invalid_index;
    return { false, changed, false };
  }

  RmBehaviourUpdate set_count(size_t count) noexcept {
    const size_t old_selected = m_selected;
    m_count = count;
    if (m_count == 0)
      m_selected = invalid_index;
    else if (!is_valid(m_selected))
      m_selected = 0;
    if (!is_valid(m_hovered))
      m_hovered = invalid_index;
    if (!is_valid(m_pressed))
      m_pressed = invalid_index;
    return { false, old_selected != m_selected, false };
  }

  RmBehaviourUpdate select(size_t index) noexcept {
    if (!m_enabled || !is_valid(index))
      return {};
    const bool changed = m_selected != index;
    m_selected = index;
    return { true, changed, changed };
  }

  RmBehaviourUpdate remove(size_t index) noexcept {
    if (!is_valid(index))
      return {};

    const size_t old_selected = m_selected;
    --m_count;
    if (m_count == 0) {
      m_selected = invalid_index;
    }
    else if (old_selected == index) {
      m_selected = std::min(index, m_count - 1);
    }
    else if (old_selected > index) {
      m_selected = old_selected - 1;
    }

    const auto adjust_index = [index](size_t value) noexcept {
      if (value == invalid_index || value == index)
        return invalid_index;
      return value > index ? value - 1 : value;
    };
    m_hovered = adjust_index(m_hovered);
    m_pressed = adjust_index(m_pressed);
    return { false, old_selected == index || old_selected != m_selected, false };
  }

  RmBehaviourUpdate pointer_move(size_t index) noexcept {
    if (!is_valid(index))
      index = invalid_index;
    const bool changed = m_hovered != index;
    m_hovered = index;
    return { m_enabled && m_pressed != invalid_index, changed, false };
  }

  RmBehaviourUpdate pointer_down(size_t index) noexcept {
    if (!m_enabled || !is_valid(index))
      return {};
    const bool changed = m_pressed != index || m_hovered != index;
    m_pressed = index;
    m_hovered = index;
    return { true, changed, false };
  }

  RmBehaviourUpdate pointer_up(size_t index) noexcept {
    if (m_pressed == invalid_index)
      return {};
    const size_t pressed = m_pressed;
    m_pressed = invalid_index;
    if (!is_valid(index))
      index = invalid_index;
    m_hovered = index;
    if (!m_enabled || pressed != index)
      return { true, true, false };
    const bool changed = m_selected != index;
    m_selected = index;
    return { true, true, changed };
  }

  RmBehaviourUpdate select_relative(int delta, bool wrap = true) noexcept {
    if (!m_enabled || m_count == 0 || delta == 0)
      return {};
    if (m_selected == invalid_index)
      return select(0);

    const int count = static_cast<int>(m_count);
    int next = static_cast<int>(m_selected) + delta;
    if (wrap) {
      next %= count;
      if (next < 0)
        next += count;
    }
    else {
      next = std::clamp(next, 0, count - 1);
    }
    return select(static_cast<size_t>(next));
  }

  RmBehaviourUpdate cancel() noexcept {
    const bool changed = m_pressed != invalid_index;
    m_pressed = invalid_index;
    return { changed, changed, false };
  }
};

class RmMenuBehaviour {
public:
  static constexpr size_t invalid_index = std::numeric_limits<size_t>::max();

private:
  size_t m_count = 0;
  size_t m_highlighted = invalid_index;
  size_t m_pressed = invalid_index;
  size_t m_opened = invalid_index;
  bool m_enabled = true;

  bool is_valid(size_t index) const noexcept { return index < m_count; }

public:
  size_t count() const noexcept { return m_count; }
  size_t highlighted_index() const noexcept { return m_highlighted; }
  size_t pressed_index() const noexcept { return m_pressed; }
  size_t opened_index() const noexcept { return m_opened; }
  bool has_open_item() const noexcept { return is_valid(m_opened); }

  RmBehaviourUpdate set_enabled(bool enabled) noexcept {
    const bool changed = m_enabled != enabled || (!enabled &&
      (m_pressed != invalid_index || m_opened != invalid_index));
    m_enabled = enabled;
    if (!m_enabled) {
      m_pressed = invalid_index;
      m_opened = invalid_index;
    }
    return { false, changed, false };
  }

  RmBehaviourUpdate set_count(size_t count) noexcept {
    m_count = count;
    const bool changed = !is_valid(m_highlighted) && m_highlighted != invalid_index;
    if (!is_valid(m_highlighted))
      m_highlighted = invalid_index;
    if (!is_valid(m_pressed))
      m_pressed = invalid_index;
    if (!is_valid(m_opened))
      m_opened = invalid_index;
    return { false, changed, false };
  }

  RmBehaviourUpdate pointer_move(size_t index) noexcept {
    if (!is_valid(index))
      index = invalid_index;
    const bool changed = m_highlighted != index;
    m_highlighted = index;
    return { m_enabled && m_pressed != invalid_index, changed, false };
  }

  RmBehaviourUpdate pointer_down(size_t index) noexcept {
    if (!m_enabled || !is_valid(index))
      return {};
    const bool changed = m_pressed != index || m_highlighted != index;
    m_pressed = index;
    m_highlighted = index;
    return { true, changed, false };
  }

  RmBehaviourUpdate pointer_up(size_t index) noexcept {
    if (m_pressed == invalid_index)
      return {};
    const size_t pressed = m_pressed;
    m_pressed = invalid_index;
    if (!is_valid(index))
      index = invalid_index;
    m_highlighted = index;
    return { true, true, m_enabled && pressed == index };
  }

  RmBehaviourUpdate open(size_t index) noexcept {
    if (!m_enabled || !is_valid(index))
      return {};
    const bool changed = m_opened != index;
    m_opened = index;
    m_highlighted = index;
    return { true, changed, false };
  }

  RmBehaviourUpdate close() noexcept {
    const bool changed = m_opened != invalid_index || m_pressed != invalid_index;
    m_opened = invalid_index;
    m_pressed = invalid_index;
    return { changed, changed, false };
  }

  RmBehaviourUpdate cancel_press() noexcept {
    const bool changed = m_pressed != invalid_index;
    m_pressed = invalid_index;
    return { changed, changed, false };
  }

  RmBehaviourUpdate select_relative(int delta, bool wrap = true) noexcept {
    if (!m_enabled || m_count == 0 || delta == 0)
      return {};
    int next = m_highlighted == invalid_index
      ? (delta > 0 ? 0 : static_cast<int>(m_count) - 1)
      : static_cast<int>(m_highlighted) + delta;
    const int count = static_cast<int>(m_count);
    if (wrap) {
      next %= count;
      if (next < 0)
        next += count;
    }
    else {
      next = std::clamp(next, 0, count - 1);
    }
    const bool changed = m_highlighted != static_cast<size_t>(next);
    m_highlighted = static_cast<size_t>(next);
    return { true, changed, false };
  }

  RmBehaviourUpdate cancel() noexcept { return close(); }
};
