#include "../exgui/exgui/rmgui_behaviour.h"

#include <cmath>
#include <cstdio>

namespace {

int failures = 0;

void expect(bool condition, const char* message)
{
  if (condition)
    return;

  std::fprintf(stderr, "FAILED: %s\n", message);
  ++failures;
}

void test_button_behaviour()
{
  RmButtonBehaviour button;

  expect(button.pointer_down(true).handled, "button handles primary press inside");
  expect(button.is_pressed(), "button enters pressed state");
  expect(!button.pointer_up(false).activated, "button does not activate after release outside");
  expect(!button.is_pressed(), "button clears pressed state after release");

  button.pointer_down(true);
  expect(button.pointer_up(true).activated, "button activates after press and release inside");

  button.set_enabled(false);
  expect(!button.pointer_down(true).handled, "disabled button ignores pointer press");
}

void test_toggle_behaviour()
{
  RmToggleBehaviour toggle;

  toggle.pointer_down(true);
  const auto update = toggle.pointer_up(true);
  expect(update.activated, "toggle reports activation");
  expect(toggle.is_checked(), "toggle changes checked state on activation");

  toggle.key_down(true);
  toggle.key_up(true);
  expect(!toggle.is_checked(), "toggle supports keyboard activation");
}

void test_slider_behaviour()
{
  RmSliderBehaviour slider(0.0f, 100.0f, 50.0f);
  expect(std::fabs(slider.fraction() - 0.5f) < 1.0e-6f, "slider exposes normalized value");

  slider.begin_drag(1.5f);
  expect(std::fabs(slider.value() - 100.0f) < 1.0e-6f, "slider clamps drag fraction");
  expect(slider.is_dragging(), "slider enters dragging state");

  slider.drag_to(0.25f);
  expect(std::fabs(slider.value() - 25.0f) < 1.0e-6f, "slider updates value while dragging");
  slider.end_drag();
  expect(!slider.is_dragging(), "slider leaves dragging state");

  slider.set_range(10.0f, -10.0f);
  expect(slider.minimum() == -10.0f && slider.maximum() == 10.0f,
    "slider normalizes reversed range");
}

void test_progress_behaviour()
{
  RmProgressBehaviour progress(25.0f);
  expect(std::fabs(progress.fraction() - 0.25f) < 1.0e-6f,
    "progress exposes normalized fraction");

  expect(progress.set_percent(150.0f).state_changed,
    "progress reports a clamped value change");
  expect(progress.percent() == 100.0f, "progress clamps values above 100 percent");

  progress.set_percent(-10.0f);
  expect(progress.percent() == 0.0f, "progress clamps values below zero");
}

void test_switch_behaviour()
{
  RmSwitchBehaviour control(false);
  control.pointer_down(true);
  expect(control.pointer_up(true).activated, "switch activates after click");
  expect(control.is_on(), "switch toggles on after activation");

  control.advance(0.1f, 0.2f);
  expect(std::fabs(control.animation_progress() - 0.5f) < 1.0e-6f,
    "switch animation advances independently of rendering");
  control.advance(1.0f, 0.2f);
  expect(control.animation_progress() == 1.0f, "switch animation clamps to its target");

  control.set_on(false, false);
  expect(!control.is_on() && control.animation_progress() == 0.0f,
    "switch supports an immediate programmatic state change");
}

} // namespace

int main()
{
  test_button_behaviour();
  test_toggle_behaviour();
  test_slider_behaviour();
  test_progress_behaviour();
  test_switch_behaviour();

  if (failures != 0) {
    std::fprintf(stderr, "%d behaviour test(s) failed\n", failures);
    return 1;
  }

  std::puts("All behaviour tests passed");
  return 0;
}
