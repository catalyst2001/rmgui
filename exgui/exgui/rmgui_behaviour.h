#pragma once

#include <algorithm>
#include <cmath>

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
