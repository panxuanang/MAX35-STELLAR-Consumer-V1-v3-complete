#include "camera_gate.h"
#include "stellar_max35_display.h"
#include "board.h"

namespace stellar_max35 {
bool WaitForCameraConsent(const std::string& question, int timeout_ms) {
    auto* display = dynamic_cast<StellarMax35Display*>(Board::GetInstance().GetDisplay());
    if (!display) return true;
    return display->WaitForTouchShutter(question.c_str(), timeout_ms);
}
}
