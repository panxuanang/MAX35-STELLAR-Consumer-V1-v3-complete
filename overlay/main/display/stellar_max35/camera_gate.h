#pragma once
#include <string>
namespace stellar_max35 {
// Returns true when capture may proceed. For non-Stellar displays it returns true immediately.
bool WaitForCameraConsent(const std::string& question, int timeout_ms = 30000);
}
