#include "../exgui/exgui/rmgui_behaviour.h"
#include "../exgui/exgui/smalldelegate.h"

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

void test_text_input_behaviour()
{
  RmTextInputBehaviour input;
  input.set_active(true);
  input.insert_codepoint('A');
  input.insert_codepoint(0x416u);
  input.insert_codepoint('B');
  expect(input.text() == "A\xd0\x96" "B", "text input inserts UTF-8 codepoints");

  input.move_left();
  input.delete_forward();
  expect(input.text() == "A\xd0\x96",
    "Delete removes the codepoint to the right of the caret");
  input.backspace();
  expect(input.text() == "A",
    "Backspace removes the complete UTF-8 codepoint to the left");
  input.undo();
  expect(input.text() == "A\xd0\x96", "text input restores edits through undo");
  input.redo();
  expect(input.text() == "A", "text input reapplies edits through redo");

  input.set_text("hello");
  input.pointer_down(1);
  input.pointer_drag(4);
  input.pointer_up();
  expect(input.selected_text() == "ell", "pointer drag creates a text selection");
  const auto cut = input.cut_selection();
  expect(cut.first == "ell" && input.text() == "ho",
    "cut returns and removes only the selected text");

  input.set_text("ab\nx\nwxyz");
  input.set_cursor(input.text().size());
  input.move_up();
  expect(input.cursor() == 4,
    "vertical navigation clamps the caret to the previous short line");
  input.move_up();
  expect(input.cursor() == 1,
    "vertical navigation preserves the UTF-8 character column");
}

void test_number_input_behaviour()
{
  RmNumberInputBehaviour number(RmNumberInputType::floating_point,
    5.0f, 0.5f, 0.0f, 6.0f);
  expect(number.step_by(1).activated &&
    std::fabs(number.value() - 5.5f) < 1.0e-6f,
    "number input applies its configured step");
  number.step_by(10);
  expect(number.value() == 6.0f, "number input clamps stepped values to its range");

  number.pointer_down(RmNumberInputPart::decrement);
  expect(!number.pointer_up(RmNumberInputPart::increment).activated &&
    number.value() == 6.0f,
    "number input ignores a button release over a different spinner part");
  number.pointer_down(RmNumberInputPart::decrement);
  expect(number.pointer_up(RmNumberInputPart::decrement).activated &&
    std::fabs(number.value() - 5.5f) < 1.0e-6f,
    "number input activates a matched spinner press and release");

  number.set_range(10.0f, -10.0f);
  expect(number.minimum() == -10.0f && number.maximum() == 10.0f,
    "number input normalizes a reversed range");
  number.set_type(RmNumberInputType::integer);
  number.set_value(3.6f);
  expect(number.value() == 4.0f, "integer number input rounds assigned values");

  const RmNumberInputGeometry right = rm_number_input_geometry(100.0f,
    40.0f, 24.0f, RmNumberInputButtonPlacement::vertical_right);
  expect(right.hit_test(90.0f, 8.0f) == RmNumberInputPart::increment &&
    right.hit_test(90.0f, 32.0f) == RmNumberInputPart::decrement &&
    right.hit_test(20.0f, 20.0f) == RmNumberInputPart::field,
    "number input lays vertical spinner buttons out on the right");

  const RmNumberInputGeometry left = rm_number_input_geometry(100.0f,
    40.0f, 24.0f, RmNumberInputButtonPlacement::vertical_left);
  expect(left.hit_test(10.0f, 8.0f) == RmNumberInputPart::increment &&
    left.hit_test(10.0f, 32.0f) == RmNumberInputPart::decrement &&
    left.hit_test(70.0f, 20.0f) == RmNumberInputPart::field,
    "number input lays vertical spinner buttons out on the left");

  const RmNumberInputGeometry sides = rm_number_input_geometry(100.0f,
    40.0f, 24.0f, RmNumberInputButtonPlacement::horizontal_sides);
  expect(sides.hit_test(10.0f, 20.0f) == RmNumberInputPart::decrement &&
    sides.hit_test(90.0f, 20.0f) == RmNumberInputPart::increment &&
    sides.hit_test(50.0f, 20.0f) == RmNumberInputPart::field,
    "number input places decrement and increment around a centered field");
}

void test_window_behaviour()
{
  expect(rm_window_resize_edges(2.0f, 98.0f, 120.0f, 100.0f,
    5.0f, WCF_RESIZABLE) == (WCF_LRESIZE | WCF_BRESIZE),
    "window detects resize corners from allowed edge flags");

  RmWindowBehaviour window;
  window.begin_drag(30.0f, 30.0f, { 10.0f, 10.0f, 100.0f, 80.0f });
  expect(window.pointer_move(70.0f, 50.0f, 200.0f, 160.0f,
    50.0f, 40.0f, 0.0f, 0.0f).state_changed &&
    window.geometry().x == 50.0f && window.geometry().y == 30.0f,
    "window dragging updates geometry inside its parent");
  window.end_interaction();

  window.begin_resize(WCF_LRESIZE | WCF_BRESIZE, 50.0f, 30.0f,
    { 50.0f, 30.0f, 100.0f, 80.0f });
  window.pointer_move(80.0f, 70.0f, 200.0f, 160.0f,
    60.0f, 40.0f, 0.0f, 0.0f);
  expect(window.geometry().x == 80.0f && window.geometry().width == 70.0f &&
    window.geometry().height == 120.0f,
    "window resizing preserves the opposite edge and applies constraints");
  window.end_interaction();
}

void test_treeview_behaviour()
{
  RmTreeViewBehaviour tree;
  tree.set_count(5);
  tree.pointer_move(2);
  tree.pointer_down(2);
  expect(tree.pointer_up(2).activated && tree.selected_index() == 2,
    "tree view selects a row after a matched press and release");
  expect(tree.select_relative(1).activated && tree.selected_index() == 3,
    "tree view supports keyboard navigation through visible rows");
  tree.select_relative(10);
  expect(tree.selected_index() == 4,
    "tree view keyboard navigation clamps at the last visible row");
  tree.set_count(2);
  expect(tree.selected_index() == RmTreeViewBehaviour::invalid_index,
    "tree view clears a selection removed by a visible-row rebuild");
  tree.set_enabled(false);
  expect(!tree.pointer_down(0).handled,
    "disabled tree view ignores pointer selection");
}

void test_propertyview_behaviour()
{
  RmPropertyViewBehaviour propertyview;
  propertyview.set_count(4);
  propertyview.pointer_move(1);
  propertyview.pointer_down(1);
  expect(propertyview.pointer_up(1).activated &&
    propertyview.selected_index() == 1,
    "property view selects a table row after a matched click");
  propertyview.select_relative(1);
  expect(propertyview.selected_index() == 2,
    "property view supports keyboard row navigation");
  propertyview.set_enabled(false);
  expect(!propertyview.pointer_down(0).handled,
    "disabled property view ignores pointer input");
}

int delegate_free_result = 0;

void delegate_free_function(int value)
{
  delegate_free_result = value;
}

struct DelegateReceiver {
  int result = 0;

  void receive(int value) { result = value; }
  int offset(int value) const { return result + value; }
};

void test_small_delegate()
{
  Delegate<void, int> free_delegate(delegate_free_function);
  free_delegate(7);
  expect(delegate_free_result == 7,
    "small delegate invokes a free function without allocation");

  DelegateReceiver receiver;
  Delegate<void, int> method_delegate;
  method_delegate.Bind<DelegateReceiver, &DelegateReceiver::receive>(&receiver);
  method_delegate(11);
  expect(receiver.result == 11,
    "small delegate invokes a bound member function");

  Delegate<int, int> const_delegate;
  const_delegate.Bind<DelegateReceiver, &DelegateReceiver::offset>(&receiver);
  expect(const_delegate(4) == 15,
    "small delegate invokes a bound const member function");
}

void test_output_text_behaviour()
{
  RmOutputTextBehaviour output(3);
  output.append_text("first\nsecond");
  output.append_text("third");
  output.append_text("fourth");
  expect(output.lines().size() == 3 && output.lines()[0] == "second" &&
    output.lines()[2] == "fourth",
    "output text retains only the newest lines within its capacity");
  output.append_text("tail\n");
  expect(output.lines().back().empty(),
    "output text preserves an explicit trailing empty line");
  output.clear();
  expect(output.lines().empty(), "output text can clear its renderer-free model");
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

void test_combobox_behaviour()
{
  RmComboBoxBehaviour combo;
  expect(!combo.open().handled, "an empty combobox does not open");

  combo.set_count(3);
  expect(combo.selected_index() == 0, "combobox selects its first item when populated");
  expect(combo.open().handled && combo.is_expanded(), "combobox opens when populated");
  combo.highlight_relative(1);
  expect(combo.highlighted_index() == 1, "combobox navigates its popup highlight");
  expect(combo.commit_highlighted().activated,
    "combobox commits a changed highlighted item");
  expect(combo.selected_index() == 1 && !combo.is_expanded(),
    "committing a combobox item updates selection and closes the popup");

  expect(combo.select_relative(-1).activated && combo.selected_index() == 0,
    "closed combobox supports keyboard selection");
  combo.open();
  combo.set_enabled(false);
  expect(!combo.is_expanded() && !combo.open().handled,
    "disabled combobox closes and ignores open requests");
}

void test_radiobutton_behaviour()
{
  RmRadioButtonBehaviour radio;
  radio.pointer_down(true);
  expect(radio.pointer_up(true).activated && radio.is_checked(),
    "radio button selects itself on activation");

  radio.pointer_down(true);
  expect(!radio.pointer_up(true).activated && radio.is_checked(),
    "selected radio button stays selected by default");
  radio.set_allow_uncheck(true);
  radio.key_down(true);
  expect(radio.key_up(true).activated && !radio.is_checked(),
    "optional radio button uncheck works with keyboard activation");
}

void test_listview_behaviour()
{
  RmListViewBehaviour list;
  list.set_count(4);
  list.pointer_move(2);
  list.pointer_down(2);
  expect(list.pointer_up(2).activated && list.selected_index() == 2,
    "list view selects the pressed row on matching release");
  expect(list.select_relative(1).activated && list.selected_index() == 3,
    "list view supports keyboard selection");
  list.set_count(2);
  expect(list.selected_index() == RmListViewBehaviour::invalid_index,
    "list view clears selection removed by a model resize");
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

void test_scrollbar_behaviour()
{
  RmScrollbarBehaviour scrollbar(0.25f);
  scrollbar.set_viewport_fraction(0.2f);
  expect(std::fabs(scrollbar.thumb_length(100.0f, 12.0f) - 20.0f) < 1.0e-6f,
    "scrollbar derives thumb length from viewport fraction");
  expect(std::fabs(scrollbar.thumb_offset(100.0f, 12.0f) - 20.0f) < 1.0e-6f,
    "scrollbar maps normalized position into the available track");
  scrollbar.begin_drag(25.0f, 100.0f, 12.0f);
  scrollbar.drag_to(65.0f, 100.0f, 12.0f);
  expect(std::fabs(scrollbar.position() - 0.75f) < 1.0e-6f,
    "scrollbar drag preserves the pointer offset inside the thumb");
  scrollbar.end_drag();
  scrollbar.set_viewport_fraction(1.0f);
  expect(scrollbar.position() == 0.0f &&
    !scrollbar.begin_drag(10.0f, 100.0f, 12.0f).handled,
    "scrollbar disables dragging when all content is visible");
}

void test_toolstrip_behaviour()
{
  RmToolbarBehaviour toolbar;
  toolbar.set_count(3);
  toolbar.pointer_down(1);
  expect(toolbar.pointer_up(1).activated,
    "toolbar reports a momentary command activation");
  expect(toolbar.selected_index() == RmToolStripBehaviour::invalid_index,
    "toolbar does not retain a selected command");

  RmToolboxBehaviour toolbox;
  toolbox.set_count(4);
  toolbox.pointer_down(2);
  expect(toolbox.pointer_up(2).activated && toolbox.selected_index() == 2,
    "toolbox retains the current tool after pointer activation");
  expect(toolbox.select(3).activated && toolbox.selected_index() == 3,
    "toolbox supports programmatic current-tool selection");
  toolbox.pointer_down(1);
  toolbox.pointer_up(0);
  expect(toolbox.selected_index() == 3,
    "toolbox does not change selection after a mismatched release");
}

void test_splitter_behaviour()
{
  RmSplitterBehaviour splitter;
  splitter.set_limits(0.2f, 0.75f);
  splitter.set_fraction(0.9f);
  expect(std::fabs(splitter.fraction() - 0.75f) < 1.0e-6f,
    "splitter clamps its fraction to the second pane minimum");
  expect(splitter.begin_drag().handled && splitter.is_dragging(),
    "splitter begins pointer dragging");
  splitter.drag_to(0.1f);
  expect(std::fabs(splitter.fraction() - 0.2f) < 1.0e-6f,
    "splitter clamps a drag to the first pane minimum");
  splitter.end_drag();
  expect(!splitter.is_dragging(), "splitter ends pointer dragging");
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
  test_text_input_behaviour();
  test_number_input_behaviour();
  test_window_behaviour();
  test_treeview_behaviour();
  test_propertyview_behaviour();
  test_small_delegate();
  test_output_text_behaviour();
  test_toggle_behaviour();
  test_combobox_behaviour();
  test_radiobutton_behaviour();
  test_listview_behaviour();
  test_slider_behaviour();
  test_scrollbar_behaviour();
  test_toolstrip_behaviour();
  test_splitter_behaviour();
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
