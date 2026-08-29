#pragma once

#include "display/lcd_display.h"
#include "device_state.h"
#include "ui_pages.h"
#include <cstdint>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <string>

class StellarMax35Display : public SpiLcdDisplay {
public:
    using SpiLcdDisplay::SpiLcdDisplay;
    ~StellarMax35Display() override;

    void SetupUI() override;
    void SetChatMessage(const char* role, const char* content) override;
    void ClearChatMessages() override;
    void UpdateStatusBar(bool update_all = false) override;

    // Camera privacy gate used by the MCP camera patch.
    bool WaitForTouchShutter(const char* question, int timeout_ms);
    void ShowCamera(const char* question);
    void ShowHome();
    void ShowChat();
    void SetWeather(const char* value, const char* detail);

private:
    enum class Page { Home, Chat, Camera };
    stellar_max35::HomeUi home_;
    stellar_max35::ChatUi chat_;
    stellar_max35::CameraUi camera_;
    Page page_ = Page::Home;
    DeviceState last_state_ = kDeviceStateUnknown;
    bool conversation_active_ = false;
    bool camera_waiting_ = false;
    bool suppress_chat_until_idle_ = false;
    int64_t scroll_finish_us_ = 0;
    int64_t return_home_us_ = 0;
    SemaphoreHandle_t shutter_sem_ = nullptr;
    std::string last_weather_value_ = "天气 --°C";
    std::string last_weather_detail_ = "等待天气数据";

    void ShowPageInternal(Page page);
    void UpdateHomeInfoInternal();
    void UpdateConversationState(DeviceState state);
    void OnShutter();
    static void ShutterCallback(void* user_data);
};
