#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <limits>
#include <string>
#include <utility>
#include <vector>

// Renderer-free interaction state machines. These classes intentionally do
// not depend on rm_widget, NanoVG, platform key codes, or style data.

struct RmBehaviourUpdate {
  bool handled = false;
  bool state_changed = false;
  bool activated = false;
};

class RmTextInputBehaviour {
  struct Snapshot {
    std::string text;
    size_t cursor = 0;
    size_t selection_start = 0;
    size_t selection_end = 0;
  };

  std::string m_text;
  size_t m_cursor = 0;
  size_t m_selection_start = 0;
  size_t m_selection_end = 0;
  std::vector<Snapshot> m_undo;
  std::vector<Snapshot> m_redo;
  bool m_enabled = true;
  bool m_active = false;
  bool m_dragging = false;

  static bool is_utf8_continuation(unsigned char value) noexcept {
    return (value & 0xc0u) == 0x80u;
  }

  size_t clamp_boundary(size_t index) const noexcept {
    index = std::min(index, m_text.size());
    while (index > 0 && index < m_text.size() &&
      is_utf8_continuation(static_cast<unsigned char>(m_text[index])))
      --index;
    return index;
  }

  size_t previous_boundary(size_t index) const noexcept {
    index = clamp_boundary(index);
    if (index == 0)
      return 0;
    --index;
    while (index > 0 &&
      is_utf8_continuation(static_cast<unsigned char>(m_text[index])))
      --index;
    return index;
  }

  size_t next_boundary(size_t index) const noexcept {
    index = clamp_boundary(index);
    if (index >= m_text.size())
      return m_text.size();
    ++index;
    while (index < m_text.size() &&
      is_utf8_continuation(static_cast<unsigned char>(m_text[index])))
      ++index;
    return index;
  }

  static size_t utf8_column(const std::string& text, size_t begin,
    size_t end) noexcept
  {
    size_t column = 0;
    for (size_t index = begin; index < end; ++index) {
      if (!is_utf8_continuation(static_cast<unsigned char>(text[index])))
        ++column;
    }
    return column;
  }

  static size_t offset_for_column(const std::string& text, size_t begin,
    size_t end, size_t column) noexcept
  {
    size_t index = begin;
    while (index < end && column > 0) {
      ++index;
      while (index < end &&
        is_utf8_continuation(static_cast<unsigned char>(text[index])))
        ++index;
      --column;
    }
    return index;
  }

  Snapshot snapshot() const {
    return { m_text, m_cursor, m_selection_start, m_selection_end };
  }

  void restore(Snapshot state) {
    m_text = std::move(state.text);
    m_cursor = std::min(state.cursor, m_text.size());
    m_selection_start = std::min(state.selection_start, m_text.size());
    m_selection_end = std::min(state.selection_end, m_text.size());
  }

  void save_undo() {
    m_undo.push_back(snapshot());
    if (m_undo.size() > 100)
      m_undo.erase(m_undo.begin());
    m_redo.clear();
  }

  void erase_selection() {
    if (!has_selection())
      return;
    const size_t first = std::min(m_selection_start, m_selection_end);
    const size_t last = std::max(m_selection_start, m_selection_end);
    m_text.erase(first, last - first);
    m_cursor = first;
    clear_selection();
  }

  RmBehaviourUpdate move_cursor(size_t index) noexcept {
    index = clamp_boundary(index);
    const bool changed = m_cursor != index || has_selection();
    m_cursor = index;
    clear_selection();
    return { changed, changed, false };
  }

public:
  bool is_enabled() const noexcept { return m_enabled; }
  bool is_active() const noexcept { return m_active; }
  bool is_dragging() const noexcept { return m_dragging; }
  const std::string& text() const noexcept { return m_text; }
  size_t cursor() const noexcept { return m_cursor; }
  size_t selection_start() const noexcept { return m_selection_start; }
  size_t selection_end() const noexcept { return m_selection_end; }
  bool has_selection() const noexcept {
    return m_selection_start != m_selection_end;
  }

  RmBehaviourUpdate set_enabled(bool enabled) noexcept {
    const bool changed = m_enabled != enabled || (!enabled && (m_active || m_dragging));
    m_enabled = enabled;
    if (!m_enabled) {
      m_active = false;
      m_dragging = false;
      clear_selection();
    }
    return { false, changed, false };
  }

  RmBehaviourUpdate set_active(bool active) noexcept {
    active = m_enabled && active;
    const bool changed = m_active != active || (!active && (m_dragging || has_selection()));
    m_active = active;
    if (!m_active) {
      m_dragging = false;
      clear_selection();
    }
    return { false, changed, false };
  }

  RmBehaviourUpdate set_text(std::string text) {
    const bool changed = m_text != text;
    m_text = std::move(text);
    m_cursor = m_text.size();
    clear_selection();
    m_undo.clear();
    m_redo.clear();
    return { false, changed, false };
  }

  RmBehaviourUpdate set_cursor(size_t index) noexcept {
    return move_cursor(index);
  }

  void clear_selection() noexcept {
    m_selection_start = m_selection_end = m_cursor;
  }

  RmBehaviourUpdate select_all() noexcept {
    const bool changed = !m_text.empty() &&
      (m_selection_start != 0 || m_selection_end != m_text.size());
    m_selection_start = 0;
    m_selection_end = m_text.size();
    m_cursor = m_selection_end;
    return { true, changed, false };
  }

  RmBehaviourUpdate pointer_down(size_t index, bool select_everything = false) noexcept {
    if (!m_enabled)
      return {};
    m_active = true;
    m_dragging = true;
    if (select_everything)
      return select_all();
    m_cursor = clamp_boundary(index);
    m_selection_start = m_selection_end = m_cursor;
    return { true, true, false };
  }

  RmBehaviourUpdate pointer_drag(size_t index) noexcept {
    if (!m_enabled || !m_dragging)
      return {};
    index = clamp_boundary(index);
    const bool changed = m_cursor != index || m_selection_end != index;
    m_cursor = index;
    m_selection_end = index;
    return { true, changed, false };
  }

  RmBehaviourUpdate pointer_up() noexcept {
    if (!m_dragging)
      return {};
    m_dragging = false;
    if (!has_selection())
      clear_selection();
    return { true, true, false };
  }

  RmBehaviourUpdate cancel_pointer() noexcept {
    if (!m_dragging)
      return {};
    m_dragging = false;
    return { false, true, false };
  }

  RmBehaviourUpdate insert_codepoint(uint32_t codepoint) {
    if (!m_enabled || !m_active || codepoint > 0x10ffffu ||
      (codepoint >= 0xd800u && codepoint <= 0xdfffu))
      return {};

    std::string encoded;
    if (codepoint < 0x80u)
      encoded.push_back(static_cast<char>(codepoint));
    else if (codepoint < 0x800u) {
      encoded.push_back(static_cast<char>(0xc0u | (codepoint >> 6)));
      encoded.push_back(static_cast<char>(0x80u | (codepoint & 0x3fu)));
    }
    else if (codepoint < 0x10000u) {
      encoded.push_back(static_cast<char>(0xe0u | (codepoint >> 12)));
      encoded.push_back(static_cast<char>(0x80u | ((codepoint >> 6) & 0x3fu)));
      encoded.push_back(static_cast<char>(0x80u | (codepoint & 0x3fu)));
    }
    else {
      encoded.push_back(static_cast<char>(0xf0u | (codepoint >> 18)));
      encoded.push_back(static_cast<char>(0x80u | ((codepoint >> 12) & 0x3fu)));
      encoded.push_back(static_cast<char>(0x80u | ((codepoint >> 6) & 0x3fu)));
      encoded.push_back(static_cast<char>(0x80u | (codepoint & 0x3fu)));
    }
    return insert_text(std::move(encoded));
  }

  RmBehaviourUpdate insert_text(std::string value) {
    if (!m_enabled || !m_active || value.empty())
      return {};
    save_undo();
    erase_selection();
    m_text.insert(m_cursor, value);
    m_cursor += value.size();
    clear_selection();
    return { true, true, false };
  }

  RmBehaviourUpdate backspace() {
    if (!m_enabled || !m_active || (m_cursor == 0 && !has_selection()))
      return {};
    save_undo();
    if (has_selection())
      erase_selection();
    else {
      const size_t previous = previous_boundary(m_cursor);
      m_text.erase(previous, m_cursor - previous);
      m_cursor = previous;
      clear_selection();
    }
    return { true, true, false };
  }

  RmBehaviourUpdate delete_forward() {
    if (!m_enabled || !m_active ||
      (m_cursor >= m_text.size() && !has_selection()))
      return {};
    save_undo();
    if (has_selection())
      erase_selection();
    else {
      const size_t next = next_boundary(m_cursor);
      m_text.erase(m_cursor, next - m_cursor);
      clear_selection();
    }
    return { true, true, false };
  }

  RmBehaviourUpdate move_left() noexcept {
    return move_cursor(has_selection()
      ? std::min(m_selection_start, m_selection_end)
      : previous_boundary(m_cursor));
  }

  RmBehaviourUpdate move_right() noexcept {
    return move_cursor(has_selection()
      ? std::max(m_selection_start, m_selection_end)
      : next_boundary(m_cursor));
  }

  RmBehaviourUpdate move_home() noexcept {
    const size_t newline = m_cursor == 0
      ? std::string::npos : m_text.rfind('\n', m_cursor - 1);
    return move_cursor(newline == std::string::npos ? 0 : newline + 1);
  }

  RmBehaviourUpdate move_end() noexcept {
    const size_t newline = m_text.find('\n', m_cursor);
    return move_cursor(newline == std::string::npos ? m_text.size() : newline);
  }

  RmBehaviourUpdate move_up() noexcept {
    const size_t current_start = m_cursor == 0 ? 0 :
      (m_text.rfind('\n', m_cursor - 1) == std::string::npos ? 0 :
        m_text.rfind('\n', m_cursor - 1) + 1);
    if (current_start == 0)
      return {};
    const size_t previous_end = current_start - 1;
    const size_t previous_break = previous_end == 0 ? std::string::npos :
      m_text.rfind('\n', previous_end - 1);
    const size_t previous_start = previous_break == std::string::npos
      ? 0 : previous_break + 1;
    const size_t column = utf8_column(m_text, current_start, m_cursor);
    return move_cursor(offset_for_column(m_text, previous_start, previous_end, column));
  }

  RmBehaviourUpdate move_down() noexcept {
    const size_t current_start = m_cursor == 0 ? 0 :
      (m_text.rfind('\n', m_cursor - 1) == std::string::npos ? 0 :
        m_text.rfind('\n', m_cursor - 1) + 1);
    const size_t current_end = m_text.find('\n', m_cursor);
    if (current_end == std::string::npos)
      return {};
    const size_t next_start = current_end + 1;
    const size_t next_break = m_text.find('\n', next_start);
    const size_t next_end = next_break == std::string::npos
      ? m_text.size() : next_break;
    const size_t column = utf8_column(m_text, current_start, m_cursor);
    return move_cursor(offset_for_column(m_text, next_start, next_end, column));
  }

  std::string selected_text() const {
    if (!has_selection())
      return {};
    const size_t first = std::min(m_selection_start, m_selection_end);
    const size_t last = std::max(m_selection_start, m_selection_end);
    return m_text.substr(first, last - first);
  }

  std::pair<std::string, RmBehaviourUpdate> cut_selection() {
    std::string selected = selected_text();
    if (selected.empty())
      return { {}, {} };
    save_undo();
    erase_selection();
    return { std::move(selected), { true, true, false } };
  }

  RmBehaviourUpdate undo() {
    if (!m_enabled || !m_active || m_undo.empty())
      return {};
    m_redo.push_back(snapshot());
    Snapshot state = std::move(m_undo.back());
    m_undo.pop_back();
    restore(std::move(state));
    return { true, true, false };
  }

  RmBehaviourUpdate redo() {
    if (!m_enabled || !m_active || m_redo.empty())
      return {};
    m_undo.push_back(snapshot());
    Snapshot state = std::move(m_redo.back());
    m_redo.pop_back();
    restore(std::move(state));
    return { true, true, false };
  }
};

enum class RmNumberInputType {
  integer,
  floating_point
};

enum class RmNumberInputPart {
  none,
  field,
  decrement,
  increment
};

class RmNumberInputBehaviour {
  RmNumberInputType m_type = RmNumberInputType::floating_point;
  RmNumberInputPart m_hovered = RmNumberInputPart::none;
  RmNumberInputPart m_pressed = RmNumberInputPart::none;
  float m_value = 0.0f;
  float m_minimum = 0.0f;
  float m_maximum = 100.0f;
  float m_step = 0.1f;
  bool m_enabled = true;

  float normalize(float value) const noexcept {
    value = std::clamp(value, m_minimum, m_maximum);
    if (m_type == RmNumberInputType::integer)
      value = std::round(value);
    return std::clamp(value, m_minimum, m_maximum);
  }

public:
  RmNumberInputBehaviour(RmNumberInputType type = RmNumberInputType::floating_point,
    float value = 0.0f, float step = 0.1f, float minimum = 0.0f,
    float maximum = 100.0f) noexcept : m_type(type)
  {
    if (minimum > maximum)
      std::swap(minimum, maximum);
    m_minimum = minimum;
    m_maximum = maximum;
    m_step = std::max(std::fabs(step), std::numeric_limits<float>::epsilon());
    m_value = normalize(value);
  }

  RmNumberInputType type() const noexcept { return m_type; }
  RmNumberInputPart hovered_part() const noexcept { return m_hovered; }
  RmNumberInputPart pressed_part() const noexcept { return m_pressed; }
  float value() const noexcept { return m_value; }
  float minimum() const noexcept { return m_minimum; }
  float maximum() const noexcept { return m_maximum; }
  float step() const noexcept { return m_step; }
  bool is_enabled() const noexcept { return m_enabled; }

  RmBehaviourUpdate set_enabled(bool enabled) noexcept {
    const bool changed = m_enabled != enabled || (!enabled &&
      (m_hovered != RmNumberInputPart::none ||
        m_pressed != RmNumberInputPart::none));
    m_enabled = enabled;
    if (!m_enabled) {
      m_hovered = RmNumberInputPart::none;
      m_pressed = RmNumberInputPart::none;
    }
    return { false, changed, false };
  }

  RmBehaviourUpdate set_type(RmNumberInputType type) noexcept {
    const float previous = m_value;
    const bool changed = m_type != type;
    m_type = type;
    m_value = normalize(m_value);
    return { false, changed || previous != m_value, false };
  }

  RmBehaviourUpdate set_range(float minimum, float maximum) noexcept {
    if (minimum > maximum)
      std::swap(minimum, maximum);
    const float previous_value = m_value;
    const bool changed = m_minimum != minimum || m_maximum != maximum;
    m_minimum = minimum;
    m_maximum = maximum;
    m_value = normalize(m_value);
    return { false, changed || previous_value != m_value, false };
  }

  RmBehaviourUpdate set_step(float step) noexcept {
    step = std::max(std::fabs(step), std::numeric_limits<float>::epsilon());
    const bool changed = m_step != step;
    m_step = step;
    return { false, changed, false };
  }

  RmBehaviourUpdate set_value(float value) noexcept {
    value = normalize(value);
    const bool changed = m_value != value;
    m_value = value;
    return { false, changed, false };
  }

  RmBehaviourUpdate step_by(int steps) noexcept {
    if (!m_enabled || steps == 0)
      return {};
    RmBehaviourUpdate update = set_value(m_value + m_step * static_cast<float>(steps));
    update.handled = true;
    update.activated = update.state_changed;
    return update;
  }

  RmBehaviourUpdate pointer_move(RmNumberInputPart part) noexcept {
    part = m_enabled ? part : RmNumberInputPart::none;
    const bool changed = m_hovered != part;
    m_hovered = part;
    return { m_pressed != RmNumberInputPart::none, changed, false };
  }

  RmBehaviourUpdate pointer_down(RmNumberInputPart part) noexcept {
    if (!m_enabled || (part != RmNumberInputPart::increment &&
      part != RmNumberInputPart::decrement))
      return {};
    const bool changed = m_pressed != part || m_hovered != part;
    m_hovered = part;
    m_pressed = part;
    return { true, changed, false };
  }

  RmBehaviourUpdate pointer_up(RmNumberInputPart part) noexcept {
    if (m_pressed == RmNumberInputPart::none)
      return {};
    const RmNumberInputPart pressed = m_pressed;
    m_pressed = RmNumberInputPart::none;
    m_hovered = m_enabled ? part : RmNumberInputPart::none;
    RmBehaviourUpdate update{ true, true, false };
    if (m_enabled && pressed == part) {
      const RmBehaviourUpdate stepped = step_by(
        part == RmNumberInputPart::increment ? 1 : -1);
      update.state_changed = true;
      update.activated = stepped.activated;
    }
    return update;
  }

  RmBehaviourUpdate cancel() noexcept {
    if (m_pressed == RmNumberInputPart::none)
      return {};
    m_pressed = RmNumberInputPart::none;
    return { false, true, false };
  }
};

class RmTreeViewBehaviour {
public:
  static constexpr size_t invalid_index = std::numeric_limits<size_t>::max();

private:
  size_t m_count = 0;
  size_t m_selected = invalid_index;
  size_t m_hovered = invalid_index;
  size_t m_pressed = invalid_index;
  bool m_enabled = true;

public:
  size_t count() const noexcept { return m_count; }
  size_t selected_index() const noexcept { return m_selected; }
  size_t hovered_index() const noexcept { return m_hovered; }
  size_t pressed_index() const noexcept { return m_pressed; }
  bool is_enabled() const noexcept { return m_enabled; }

  RmBehaviourUpdate set_enabled(bool enabled) noexcept {
    const bool changed = m_enabled != enabled || (!enabled &&
      (m_hovered != invalid_index || m_pressed != invalid_index));
    m_enabled = enabled;
    if (!m_enabled) {
      m_hovered = invalid_index;
      m_pressed = invalid_index;
    }
    return { false, changed, false };
  }

  RmBehaviourUpdate set_count(size_t count) noexcept {
    const bool changed = m_count != count ||
      (m_selected != invalid_index && m_selected >= count);
    m_count = count;
    if (m_selected >= m_count)
      m_selected = invalid_index;
    if (m_hovered >= m_count)
      m_hovered = invalid_index;
    if (m_pressed >= m_count)
      m_pressed = invalid_index;
    return { false, changed, false };
  }

  RmBehaviourUpdate pointer_move(size_t index) noexcept {
    if (!m_enabled)
      index = invalid_index;
    if (index >= m_count)
      index = invalid_index;
    const bool changed = m_hovered != index;
    m_hovered = index;
    return { m_pressed != invalid_index, changed, false };
  }

  RmBehaviourUpdate pointer_down(size_t index) noexcept {
    if (!m_enabled || index >= m_count)
      return {};
    const bool changed = m_pressed != index || m_hovered != index;
    m_pressed = index;
    m_hovered = index;
    return { true, changed, false };
  }

  RmBehaviourUpdate pointer_up(size_t index) noexcept {
    if (m_pressed == invalid_index)
      return {};
    const bool activated = m_enabled && index < m_count && m_pressed == index;
    m_pressed = invalid_index;
    m_hovered = index < m_count ? index : invalid_index;
    if (!activated)
      return { true, true, false };
    m_selected = index;
    return { true, true, true };
  }

  RmBehaviourUpdate select(size_t index) noexcept {
    if (!m_enabled || index >= m_count)
      return {};
    const bool changed = m_selected != index;
    m_selected = index;
    return { true, changed, changed };
  }

  RmBehaviourUpdate select_relative(int delta) noexcept {
    if (!m_enabled || m_count == 0 || delta == 0)
      return {};
    const int current = m_selected == invalid_index
      ? (delta > 0 ? -1 : static_cast<int>(m_count))
      : static_cast<int>(m_selected);
    const size_t next = static_cast<size_t>(std::clamp(current + delta,
      0, static_cast<int>(m_count) - 1));
    return select(next);
  }

  RmBehaviourUpdate clear_selection() noexcept {
    const bool changed = m_selected != invalid_index;
    m_selected = invalid_index;
    return { false, changed, false };
  }

  RmBehaviourUpdate cancel() noexcept {
    const bool changed = m_pressed != invalid_index;
    m_pressed = invalid_index;
    return { false, changed, false };
  }
};

class RmOutputTextBehaviour {
  std::vector<std::string> m_lines;
  size_t m_capacity = 16;

  void trim() {
    if (m_lines.size() > m_capacity)
      m_lines.erase(m_lines.begin(),
        m_lines.begin() + static_cast<std::ptrdiff_t>(m_lines.size() - m_capacity));
  }

public:
  explicit RmOutputTextBehaviour(size_t capacity = 16) noexcept
    : m_capacity(std::max<size_t>(1, capacity)) {}

  const std::vector<std::string>& lines() const noexcept { return m_lines; }
  size_t capacity() const noexcept { return m_capacity; }

  RmBehaviourUpdate set_capacity(size_t capacity) {
    capacity = std::max<size_t>(1, capacity);
    const bool changed = m_capacity != capacity || m_lines.size() > capacity;
    m_capacity = capacity;
    trim();
    return { false, changed, false };
  }

  RmBehaviourUpdate append_text(const std::string& text) {
    if (text.empty())
      return {};
    size_t start = 0;
    do {
      const size_t newline = text.find('\n', start);
      const size_t end = newline == std::string::npos ? text.size() : newline;
      m_lines.push_back(text.substr(start, end - start));
      if (newline == std::string::npos)
        break;
      start = newline + 1;
    } while (start <= text.size());
    trim();
    return { true, true, false };
  }

  RmBehaviourUpdate clear() noexcept {
    const bool changed = !m_lines.empty();
    m_lines.clear();
    return { false, changed, false };
  }
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
