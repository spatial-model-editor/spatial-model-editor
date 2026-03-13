#include "catch_wrapper.hpp"
#include "gpumemoryusagebarwidget.hpp"
#include "qt_test_utils.hpp"

using namespace sme::test;

TEST_CASE("GpuMemoryUsageBarWidget",
          "[gui/widgets/gpumemoryusagebarwidget][gui/widgets][gui]"
          "[gpumemoryusagebarwidget]") {
  SECTION("valid memory info shows tooltip and widget") {
    GpuMemoryUsageBarWidget widget(
        []() -> std::optional<sme::simulate::CudaMemoryInfo> {
          return sme::simulate::CudaMemoryInfo{100, 30};
        },
        nullptr, 60000);
    widget.show();
    waitFor(&widget);
    widget.refresh();

    REQUIRE(widget.isHidden() == false);
    REQUIRE(widget.toolTip().contains("GPU RAM used: 70 B"));
    REQUIRE(widget.toolTip().contains("GPU RAM total available: 30 B"));
    REQUIRE(widget.toolTip().contains("GPU RAM total: 100 B"));
  }

  SECTION("available memory is capped at total memory") {
    GpuMemoryUsageBarWidget widget(
        []() -> std::optional<sme::simulate::CudaMemoryInfo> {
          return sme::simulate::CudaMemoryInfo{100, 130};
        },
        nullptr, 60000);
    widget.show();
    waitFor(&widget);
    widget.refresh();

    REQUIRE(widget.toolTip().contains("GPU RAM used: 0 B"));
    REQUIRE(widget.toolTip().contains("GPU RAM total available: 100 B"));
  }

  SECTION("missing memory info hides widget and clears tooltip") {
    GpuMemoryUsageBarWidget widget(
        []() -> std::optional<sme::simulate::CudaMemoryInfo> {
          return std::nullopt;
        },
        nullptr, 60000);
    widget.show();
    waitFor(&widget);
    widget.refresh();

    REQUIRE(widget.isHidden());
    REQUIRE(widget.toolTip().isEmpty());
  }
}
