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

void test_tab_behaviour()
{
  RmTabBehaviour tabs;
  tabs.set_count(3);
  expect(tabs.selected_index() == 0, "tabs select the first item when populated");

  tabs.pointer_down(2);
  expect(tabs.pointer_up(2).activated, "tabs activate after press and release on one item");
  expect(tabs.selected_index() == 2, "tab pointer activation updates selection");

  tabs.select_relative(1);
  expect(tabs.selected_index() == 0, "tab keyboard navigation wraps forward");
  tabs.select_relative(-1);
  expect(tabs.selected_index() == 2, "tab keyboard navigation wraps backward");

  tabs.remove(1);
  expect(tabs.count() == 2 && tabs.selected_index() == 1,
    "removing an earlier tab preserves the selected item");
  tabs.remove(1);
  expect(tabs.selected_index() == 0, "removing the selected last tab selects its neighbour");
  tabs.remove(0);
  expect(tabs.selected_index() == RmTabBehaviour::invalid_index,
    "empty tabs have no selected index");
}

void test_menu_behaviour()
{
  RmMenuBehaviour menu;
  menu.set_count(4);
  menu.pointer_move(1);
  expect(menu.highlighted_index() == 1, "menu tracks the highlighted item");

  menu.pointer_down(1);
  expect(menu.pointer_up(1).activated, "menu activates on matching press and release");
  menu.open(1);
  expect(menu.opened_index() == 1, "menu tracks the opened submenu");
  menu.pointer_down(1);
  menu.cancel_press();
  expect(menu.opened_index() == 1,
    "losing pointer capture does not close an opened submenu");

  menu.select_relative(1);
  expect(menu.highlighted_index() == 2, "menu supports forward keyboard navigation");
  menu.select_relative(-3);
  expect(menu.highlighted_index() == 3, "menu keyboard navigation wraps");

  menu.close();
  expect(!menu.has_open_item(), "menu close clears opened state");
  menu.set_enabled(false);
  expect(!menu.pointer_down(0).handled, "disabled menu ignores pointer input");
}

} // namespace

int main()
{
  test_button_behaviour();
  test_toggle_behaviour();
  test_slider_behaviour();
  test_progress_behaviour();
  test_switch_behaviour();
  test_tab_behaviour();
  test_menu_behaviour();

  if (failures != 0) {
    std::fprintf(stderr, "%d behaviour test(s) failed\n", failures);
    return 1;
  }

  std::puts("All behaviour tests passed");
  return 0;
}
