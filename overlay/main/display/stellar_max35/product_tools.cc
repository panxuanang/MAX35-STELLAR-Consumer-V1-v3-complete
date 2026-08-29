#include "product_tools.h"
#include "stellar_max35_display.h"
#include "stellar_todo.h"

#include "application.h"
#include "board.h"
#include "mcp_server.h"
#include <string>

namespace stellar_max35 {
namespace {
StellarMax35Display* Display() {
    return dynamic_cast<StellarMax35Display*>(Board::GetInstance().GetDisplay());
}
}

void RegisterProductTools() {
    RegisterTodoTools();
    auto& m = McpServer::GetInstance();

    // Voice-only navigation: no permanent on-screen home button is needed.
    m.AddTool("self.stellar.ui.home",
              "Return the MAX35 product display to its desktop/home screen when the user asks to go back to the desktop.",
              PropertyList(),
              [](const PropertyList&) -> ReturnValue {
                  auto* d = Display();
                  if (!d) return false;
                  Application::GetInstance().Schedule([d]() { d->ShowHome(); });
                  return true;
              });

    // Weather data is deliberately injected, not invented locally. A cloud weather MCP or
    // application service can call this tool after it has obtained trustworthy data.
    m.AddTool("self.stellar.weather.set",
              "Update the desktop weather after obtaining real weather data. value example: '晴 26°C'; detail example: '空气优'.",
              PropertyList({Property("value", kPropertyTypeString), Property("detail", kPropertyTypeString)}),
              [](const PropertyList& p) -> ReturnValue {
                  auto* d = Display();
                  if (!d) return false;
                  const auto value = p["value"].value<std::string>();
                  const auto detail = p["detail"].value<std::string>();
                  Application::GetInstance().Schedule([d, value, detail]() {
                      d->SetWeather(value.c_str(), detail.c_str());
                  });
                  return true;
              });
}
}  // namespace stellar_max35
