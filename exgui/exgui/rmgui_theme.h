#pragma once

#include "nanovg.h"

#include <memory>

enum class RmVisualState {
  normal,
  hovered,
  pressed,
  disabled
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

struct RmButtonStyle {
  RmStateColors background{
    NVGcolor::RGBA(70, 92, 210, 255),
    NVGcolor::RGBA(88, 112, 235, 255),
    NVGcolor::RGBA(51, 69, 173, 255),
    NVGcolor::RGBA(74, 76, 88, 255)
  };
  RmStateColors border{
    NVGcolor::RGBA(105, 127, 238, 255),
    NVGcolor::RGBA(135, 153, 255, 255),
    NVGcolor::RGBA(76, 91, 191, 255),
    NVGcolor::RGBA(95, 96, 103, 255)
  };
  RmStateColors text{
    NVGcolor::RGBA(255, 255, 255, 255),
    NVGcolor::RGBA(255, 255, 255, 255),
    NVGcolor::RGBA(245, 246, 255, 255),
    NVGcolor::RGBA(168, 169, 177, 255)
  };
  NVGcolor focus_ring = NVGcolor::RGBA(148, 190, 255, 230);
  float corner_radius = 4.0f;
  float border_width = 1.0f;
  float focus_ring_width = 2.0f;
  float font_size = 20.0f;
};

struct RmLabelStyle {
  NVGcolor text = NVGcolor::RGBA(244, 244, 247, 255);
  NVGcolor disabled_text = NVGcolor::RGBA(145, 145, 151, 255);
  float font_size = 18.0f;
};

struct RmCheckboxStyle {
  RmStateColors background{
    NVGcolor::RGBA(255, 255, 255, 255),
    NVGcolor::RGBA(247, 249, 255, 255),
    NVGcolor::RGBA(225, 231, 255, 255),
    NVGcolor::RGBA(105, 106, 113, 255)
  };
  RmStateColors border{
    NVGcolor::RGBA(52, 58, 78, 255),
    NVGcolor::RGBA(91, 116, 231, 255),
    NVGcolor::RGBA(57, 76, 195, 255),
    NVGcolor::RGBA(87, 88, 96, 255)
  };
  RmStateColors mark{
    NVGcolor::RGBA(42, 55, 150, 255),
    NVGcolor::RGBA(42, 55, 150, 255),
    NVGcolor::RGBA(31, 42, 121, 255),
    NVGcolor::RGBA(159, 160, 168, 255)
  };
  RmStateColors text{
    NVGcolor::RGBA(244, 244, 247, 255),
    NVGcolor::RGBA(255, 255, 255, 255),
    NVGcolor::RGBA(244, 244, 247, 255),
    NVGcolor::RGBA(145, 145, 151, 255)
  };
  NVGcolor focus_ring = NVGcolor::RGBA(148, 190, 255, 230);
  float box_size = 20.0f;
  float corner_radius = 3.0f;
  float border_width = 1.0f;
  float mark_width = 2.25f;
  float text_gap = 5.0f;
  float font_size = 18.0f;
  float focus_ring_width = 2.0f;
};

struct RmSliderStyle {
  RmStateColors track{
    NVGcolor::RGBA(78, 80, 89, 255),
    NVGcolor::RGBA(86, 89, 100, 255),
    NVGcolor::RGBA(86, 89, 100, 255),
    NVGcolor::RGBA(66, 67, 73, 255)
  };
  RmStateColors fill{
    NVGcolor::RGBA(70, 92, 210, 255),
    NVGcolor::RGBA(88, 112, 235, 255),
    NVGcolor::RGBA(51, 69, 173, 255),
    NVGcolor::RGBA(93, 95, 106, 255)
  };
  RmStateColors thumb{
    NVGcolor::RGBA(255, 255, 255, 255),
    NVGcolor::RGBA(255, 255, 255, 255),
    NVGcolor::RGBA(225, 231, 255, 255),
    NVGcolor::RGBA(154, 155, 163, 255)
  };
  RmStateColors thumb_border{
    NVGcolor::RGBA(70, 92, 210, 255),
    NVGcolor::RGBA(106, 130, 244, 255),
    NVGcolor::RGBA(51, 69, 173, 255),
    NVGcolor::RGBA(93, 95, 106, 255)
  };
  NVGcolor focus_ring = NVGcolor::RGBA(148, 190, 255, 230);
  float track_height = 4.0f;
  float padding = 8.0f;
  float thumb_radius = 10.0f;
  float thumb_border_width = 2.0f;
  float focus_ring_width = 2.0f;
};

struct RmProgressStyle {
  RmStateColors background{
    NVGcolor::RGBA(24, 25, 30, 255),
    NVGcolor::RGBA(24, 25, 30, 255),
    NVGcolor::RGBA(24, 25, 30, 255),
    NVGcolor::RGBA(47, 48, 53, 255)
  };
  RmStateColors fill_start{
    NVGcolor::RGBA(232, 43, 231, 255),
    NVGcolor::RGBA(232, 43, 231, 255),
    NVGcolor::RGBA(232, 43, 231, 255),
    NVGcolor::RGBA(112, 85, 112, 255)
  };
  RmStateColors fill_end{
    NVGcolor::RGBA(40, 5, 229, 255),
    NVGcolor::RGBA(40, 5, 229, 255),
    NVGcolor::RGBA(40, 5, 229, 255),
    NVGcolor::RGBA(68, 65, 111, 255)
  };
  float corner_radius = 4.0f;
};

struct RmSwitchStyle {
  RmStateColors track_off{
    NVGcolor::RGBA(200, 200, 200, 128),
    NVGcolor::RGBA(210, 210, 214, 160),
    NVGcolor::RGBA(180, 181, 188, 180),
    NVGcolor::RGBA(83, 84, 90, 180)
  };
  RmStateColors track_on{
    NVGcolor::RGBA(57, 76, 195, 255),
    NVGcolor::RGBA(77, 99, 224, 255),
    NVGcolor::RGBA(43, 59, 163, 255),
    NVGcolor::RGBA(75, 81, 119, 255)
  };
  RmStateColors knob{
    NVGcolor::RGBA(255, 255, 255, 255),
    NVGcolor::RGBA(255, 255, 255, 255),
    NVGcolor::RGBA(232, 234, 244, 255),
    NVGcolor::RGBA(155, 156, 164, 255)
  };
  NVGcolor focus_ring = NVGcolor::RGBA(148, 190, 255, 230);
  NVGcolor shadow = NVGcolor::RGBAf(0.0f, 0.0f, 0.0f, 0.25f);
  float track_height = 24.0f;
  float padding = 4.0f;
  float knob_radius = 11.0f;
  float corner_radius = 9.6f;
  float animation_duration = 0.2f;
  float shadow_offset = 5.0f;
  float shadow_size = 7.0f;
  float focus_ring_width = 2.0f;
};

struct RmTheme {
  RmButtonStyle button;
  RmLabelStyle label;
  RmCheckboxStyle checkbox;
  RmSliderStyle slider;
  RmProgressStyle progress;
  RmSwitchStyle switch_control;

  static std::shared_ptr<const RmTheme> default_theme() {
    static const std::shared_ptr<const RmTheme> theme = std::make_shared<const RmTheme>();
    return theme;
  }
};

using RmThemeRef = std::shared_ptr<const RmTheme>;
