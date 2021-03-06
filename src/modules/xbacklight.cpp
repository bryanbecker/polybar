#include "modules/xbacklight.hpp"
#include "drawtypes/label.hpp"
#include "drawtypes/progressbar.hpp"
#include "drawtypes/ramp.hpp"
#include "utils/math.hpp"
#include "x11/connection.hpp"
#include "x11/graphics.hpp"
#include "x11/xutils.hpp"

#include "modules/meta/base.inl"

POLYBAR_NS

namespace modules {
  template class module<xbacklight_module>;

  /**
   * Construct module
   */
  xbacklight_module::xbacklight_module(const bar_settings& bar, string name_)
      : static_module<xbacklight_module>(bar, move(name_)), m_connection(connection::make()) {
    auto output = m_conf.get(name(), "output", m_bar.monitor->name);
    auto strict = m_conf.get(name(), "monitor-strict", false);

    // Grab a list of all outputs and try to find the one defined in the config
    for (auto&& mon : randr_util::get_monitors(m_connection, m_connection.root(), strict)) {
      if (mon->match(output, strict)) {
        m_output.swap(mon);
        break;
      }
    }

    // If we didn't get a match we stop the module
    if (!m_output) {
      throw module_error("No matching output found for \"" + output + "\", stopping module...");
    }

    // Get flag to check if we should add scroll handlers for changing value
    m_scroll = m_conf.get(name(), "enable-scroll", m_scroll);

    // Query randr for the backlight max and min value
    try {
      auto& backlight = m_output->backlight;
      randr_util::get_backlight_range(m_connection, m_output, backlight);
      randr_util::get_backlight_value(m_connection, m_output, backlight);
    } catch (const exception& err) {
      throw module_error("No backlight data found for \"" + output + "\", stopping module...");
    }

    // Create window that will proxy all RandR notify events
    if (!graphics_util::create_window(m_connection, &m_proxy, -1, -1, 1, 1)) {
      throw module_error("Failed to create event proxy");
    }

    // Connect with the event registry and make sure we get
    // notified when a RandR output property gets modified
    m_connection.select_input_checked(m_proxy, XCB_RANDR_NOTIFY_MASK_OUTPUT_PROPERTY);

    // Add formats and elements
    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_LABEL, TAG_BAR, TAG_RAMP});

    if (m_formatter->has(TAG_LABEL)) {
      m_label = load_optional_label(m_conf, name(), TAG_LABEL, "%percentage%");
    }
    if (m_formatter->has(TAG_BAR)) {
      m_progressbar = load_progressbar(m_bar, m_conf, name(), TAG_BAR);
    }
    if (m_formatter->has(TAG_RAMP)) {
      m_ramp = load_ramp(m_conf, name(), TAG_RAMP);
    }
  }

  /**
   * Handler for XCB_RANDR_NOTIFY events
   */
  void xbacklight_module::handle(const evt::randr_notify& evt) {
    if (evt->subCode != XCB_RANDR_NOTIFY_OUTPUT_PROPERTY) {
      return;
    } else if (evt->u.op.status != XCB_PROPERTY_NEW_VALUE) {
      return;
    } else if (evt->u.op.window != m_proxy) {
      return;
    } else if (evt->u.op.output != m_output->output) {
      return;
    } else if (evt->u.op.atom != m_output->backlight.atom) {
      return;
    } else {
      update();
    }
  }

  /**
   * Query the RandR extension for the new values
   */
  void xbacklight_module::update() {
    // Query for the new backlight value
    auto& bl = m_output->backlight;
    randr_util::get_backlight_value(m_connection, m_output, bl);
    m_percentage = math_util::percentage(bl.val, bl.min, bl.max);

    // Update label tokens
    if (m_label) {
      m_label->reset_tokens();
      m_label->replace_token("%percentage%", to_string(m_percentage) + "%");
    }

    // Emit a broadcast notification so that
    // the new data will be drawn to the bar
    broadcast();
  }

  /**
   * Generate the module output
   */
  string xbacklight_module::get_output() {
    // Get the module output early so that
    // the format prefix/suffix also gets wrapped
    // with the cmd handlers
    string output{module::get_output()};
    bool scroll_up{m_scroll && m_percentage < 100};
    bool scroll_down{m_scroll && m_percentage > 0};

    m_builder->cmd(mousebtn::SCROLL_UP, string{EVENT_SCROLLUP}, scroll_up);
    m_builder->cmd(mousebtn::SCROLL_DOWN, string{EVENT_SCROLLDOWN}, scroll_down);

    m_builder->append(output);

    m_builder->cmd_close(scroll_down);
    m_builder->cmd_close(scroll_up);

    return m_builder->flush();
  }

  /**
   * Output content as defined in the config
   */
  bool xbacklight_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_BAR) {
      builder->node(m_progressbar->output(m_percentage));
    } else if (tag == TAG_RAMP) {
      builder->node(m_ramp->get_by_percentage(m_percentage));
    } else if (tag == TAG_LABEL) {
      builder->node(m_label);
    } else {
      return false;
    }
    return true;
  }

  /**
   * Process scroll events by changing backlight value
   */
  bool xbacklight_module::input(string&& cmd) {
    int value_mod = 0;

    if (cmd == EVENT_SCROLLUP) {
      value_mod = 10;
      m_log.info("%s: Increasing value by %i%", name(), value_mod);
    } else if (cmd == EVENT_SCROLLDOWN) {
      value_mod = -10;
      m_log.info("%s: Decreasing value by %i%", name(), -value_mod);
    } else {
      return false;
    }

    try {
      const int new_perc = math_util::cap(m_percentage + value_mod, 0, 100);
      const int new_value = math_util::percentage_to_value<int>(new_perc, m_output->backlight.max);
      const int values[1]{new_value};

      m_connection.change_output_property_checked(
          m_output->output, m_output->backlight.atom, XCB_ATOM_INTEGER, 32, XCB_PROP_MODE_REPLACE, 1, values);
    } catch (const exception& err) {
      m_log.err("%s: %s", name(), err.what());
    }

    return true;
  }
}

POLYBAR_NS_END
