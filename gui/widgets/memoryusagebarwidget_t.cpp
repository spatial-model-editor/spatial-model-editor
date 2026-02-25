#include "catch_wrapper.hpp"
#include "memoryusagebarwidget.hpp"
#include "qt_test_utils.hpp"

using namespace sme::test;

TEST_CASE("MemoryUsageBarWidget", "[gui/widgets/memoryusagebarwidget][gui/"
                                  "widgets][gui][memoryusagebarwidget]") {
  SECTION("valid memory info shows tooltip and widget") {
    MemoryUsageBarWidget widget(
        []() -> std::optional<sme::common::SystemMemoryInfo> {
          return sme::common::SystemMemoryInfo{100, 30};
        },
        []() -> std::optional<std::size_t> { return 40; }, nullptr, 60000);
    widget.show();
    waitFor(&widget);
    widget.refresh();

    REQUIRE(widget.isHidden() == false);
    REQUIRE(widget.toolTip().contains("RAM used by SME: 40 B"));
    REQUIRE(widget.toolTip().contains("RAM used by system: 30 B"));
    REQUIRE(widget.toolTip().contains("RAM total available: 30 B"));
  }

  SECTION("SME usage is capped at total used memory") {
    MemoryUsageBarWidget widget(
        []() -> std::optional<sme::common::SystemMemoryInfo> {
          return sme::common::SystemMemoryInfo{100, 30};
        },
        []() -> std::optional<std::size_t> { return 99; }, nullptr, 60000);
    widget.show();
    waitFor(&widget);
    widget.refresh();

    REQUIRE(widget.toolTip().contains("RAM used by SME: 70 B"));
    REQUIRE(widget.toolTip().contains("RAM used by system: 0 B"));
  }

  SECTION("missing memory info hides widget") {
    MemoryUsageBarWidget widget(
        []() -> std::optional<sme::common::SystemMemoryInfo> {
          return std::nullopt;
        },
        []() -> std::optional<std::size_t> { return 10; }, nullptr, 60000);
    widget.show();
    waitFor(&widget);
    widget.refresh();

    REQUIRE(widget.isHidden());
  }

  SECTION("zero total memory hides widget") {
    MemoryUsageBarWidget widget(
        []() -> std::optional<sme::common::SystemMemoryInfo> {
          return sme::common::SystemMemoryInfo{0, 0};
        },
        []() -> std::optional<std::size_t> { return 10; }, nullptr, 60000);
    widget.show();
    waitFor(&widget);
    widget.refresh();

    REQUIRE(widget.isHidden());
  }
}
