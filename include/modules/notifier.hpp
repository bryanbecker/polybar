#pragma once

#include "config.hpp"
#include "modules/meta/timer_module.hpp"
#include "utils/http.hpp"

POLYBAR_NS

namespace modules {
  /**
   * Module used to query Oauth for something
   */
  class notifier_module : public timer_module<notifier_module> {
   public:
    explicit notifier_module(const bar_settings&, string);

    bool update();
    bool build(builder* builder, const string& tag) const;

   private:
    static constexpr auto TAG_LABEL = "<label>";

    label_t m_label{};
    string m_accesstoken{};
    unique_ptr<http_downloader> m_http{};
    bool m_empty_notifications{false};
  };
}

POLYBAR_NS_END
