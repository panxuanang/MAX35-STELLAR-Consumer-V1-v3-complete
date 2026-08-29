#include "stellar_max35_display.h"
#include "stellar_todo.h"

#include "application.h"
#include <esp_log.h>
#include <esp_timer.h>
#include <cstring>
#include <ctime>
#include <cstdio>

namespace {
constexpr const char* TAG = "StellarMAX35";
constexpr uint32_t kGreen = 0x7FA37A;
constexpr uint32_t kBlue = 0x6E9DD8;
constexpr uint32_t kGold = 0xC89B5D;
constexpr uint32_t kRed = 0xD36C74;
constexpr int kScrollDelayMs = 1800;
constexpr int kScrollMsPerPixel = 90;
constexpr int64_t kReturnHomeDelayUs = 4500000LL;
constexpr int64_t kAfterScrollDelayUs = 1800000LL;

bool IsConversing(DeviceState s) {
    return s == kDeviceStateConnecting || s == kDeviceStateListening || s == kDeviceStateSpeaking;
}
const char* StateText(DeviceState s) {
    switch (s) {
        case kDeviceStateStarting: return "小智启动中";
        case kDeviceStateWifiConfiguring: return "等待配网";
        case kDeviceStateConnecting: return "正在连接";
        case kDeviceStateListening: return "正在聆听";
        case kDeviceStateSpeaking: return "正在回答";
        case kDeviceStateUpgrading: return "系统升级中";
        case kDeviceStateFatalError: return "系统异常";
        default: return "小智待命中";
    }
}
uint32_t StateColor(DeviceState s) {
    if (s == kDeviceStateListening) return kBlue;
    if (s == kDeviceStateSpeaking) return kGold;
    if (s == kDeviceStateFatalError) return kRed;
    return kGreen;
}
const char* Weekday(int w) {
    static const char* k[] = {"星期日","星期一","星期二","星期三","星期四","星期五","星期六"};
    return (w >= 0 && w < 7) ? k[w] : "";
}
}

StellarMax35Display::~StellarMax35Display() {
    if (shutter_sem_) vSemaphoreDelete(shutter_sem_);
}

void StellarMax35Display::SetupUI() {
    // Preserve native Xiaozhi objects behind opaque product pages so framework internals stay intact.
    LcdDisplay::SetupUI();
    shutter_sem_ = xSemaphoreCreateBinary();
    {
        DisplayLockGuard lock(this);
        auto* screen = lv_screen_active();
        stellar_max35::BuildHomeUi(screen, &home_);
        stellar_max35::BuildChatUi(screen, &chat_);
        stellar_max35::BuildCameraUi(screen, &camera_, ShutterCallback, this);
        page_ = Page::Home;
        conversation_active_ = false;
        ShowPageInternal(Page::Home);
    }
    last_state_ = Application::GetInstance().GetDeviceState();
    {
        DisplayLockGuard lock(this);
        UpdateHomeInfoInternal();
    }
    ESP_LOGI(TAG, "MAX35 consumer UI ready: home/chat/camera separated");
}

void StellarMax35Display::ShowPageInternal(Page page) {
    if (!home_.root || !chat_.root || !camera_.root) return;
    lv_obj_add_flag(home_.root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(chat_.root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(camera_.root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t* target = nullptr;
    if (page == Page::Home) target = home_.root;
    if (page == Page::Chat) target = chat_.root;
    if (page == Page::Camera) target = camera_.root;
    if (target) {
        lv_obj_remove_flag(target, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(target);
    }
    page_ = page;
    if (page != Page::Chat) stellar_max35::ChatUiStopScroll(&chat_);
}

void StellarMax35Display::ShowHome() {
    DisplayLockGuard lock(this);
    const DeviceState state = Application::GetInstance().GetDeviceState();
    suppress_chat_until_idle_ = IsConversing(state);
    conversation_active_ = false;
    camera_waiting_ = false;
    return_home_us_ = 0;
    scroll_finish_us_ = 0;
    ShowPageInternal(Page::Home);
    UpdateHomeInfoInternal();
}

void StellarMax35Display::ShowChat() {
    DisplayLockGuard lock(this);
    conversation_active_ = true;
    return_home_us_ = 0;
    ShowPageInternal(Page::Chat);
}

void StellarMax35Display::SetWeather(const char* value, const char* detail) {
    DisplayLockGuard lock(this);
    last_weather_value_ = (value && value[0]) ? value : "天气 --°C";
    last_weather_detail_ = (detail && detail[0]) ? detail : "等待天气数据";
    if (page_ == Page::Home) {
        stellar_max35::HomeUiSetWeather(&home_, last_weather_value_.c_str(), last_weather_detail_.c_str());
    }
}

void StellarMax35Display::ShowCamera(const char* question) {
    DisplayLockGuard lock(this);
    conversation_active_ = true;
    camera_waiting_ = true;
    return_home_us_ = 0;
    stellar_max35::CameraUiSetPrompt(&camera_, question);
    stellar_max35::CameraUiSetState(&camera_, "后置摄像头已就绪", false);
    stellar_max35::CameraUiSetEnabled(&camera_, true);
    ShowPageInternal(Page::Camera);
}

void StellarMax35Display::ShutterCallback(void* user_data) {
    auto* self = static_cast<StellarMax35Display*>(user_data);
    if (self) self->OnShutter();
}

void StellarMax35Display::OnShutter() {
    if (!camera_waiting_ || !shutter_sem_) return;
    stellar_max35::CameraUiSetEnabled(&camera_, false);
    stellar_max35::CameraUiSetState(&camera_, "已拍照，正在识别题目…", true);
    xSemaphoreGive(shutter_sem_);
}

bool StellarMax35Display::WaitForTouchShutter(const char* question, int timeout_ms) {
    if (!shutter_sem_) return true;  // Fail-open preserves camera function if UI init was skipped.
    while (xSemaphoreTake(shutter_sem_, 0) == pdTRUE) {}
    ShowCamera(question);
    const bool clicked = xSemaphoreTake(shutter_sem_, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
    {
        DisplayLockGuard lock(this);
        camera_waiting_ = false;
        if (clicked) {
            stellar_max35::ChatUiSetUser(&chat_, "拍照搜题");
            stellar_max35::ChatUiSetAnswer(&chat_, "照片已获取，正在识别并整理答案…");
            stellar_max35::ChatUiSetState(&chat_, "正在识别题目", kGold);
            ShowPageInternal(Page::Chat);
        } else {
            ShowPageInternal(Page::Home);
        }
    }
    return clicked;
}

void StellarMax35Display::SetChatMessage(const char* role, const char* content) {
    if (!setup_ui_called_ || !role || !content || !content[0]) return;
    DisplayLockGuard lock(this);
    if (std::strcmp(role, "system") == 0) {
        // Startup/network notifications must not hijack the desktop.
        return;
    }
    if (suppress_chat_until_idle_) return;
    if (std::strcmp(role, "user") == 0) {
        conversation_active_ = true;
        ShowPageInternal(Page::Chat);
        stellar_max35::ChatUiSetUser(&chat_, content);
        stellar_max35::ChatUiSetAnswer(&chat_, "我听到了，正在思考…");
        stellar_max35::ChatUiSetState(&chat_, "正在思考", kBlue);
        scroll_finish_us_ = 0;
        return_home_us_ = 0;
        return;
    }
    if (std::strcmp(role, "assistant") == 0) {
        if (!conversation_active_) return;
        ShowPageInternal(Page::Chat);
        stellar_max35::ChatUiSetAnswer(&chat_, content);
        stellar_max35::ChatUiSetState(&chat_, "小智正在回答", kGold);
        scroll_finish_us_ = stellar_max35::ChatUiStartReadableScroll(&chat_, kScrollDelayMs, kScrollMsPerPixel);
    }
}

void StellarMax35Display::ClearChatMessages() {
    // Keep the final answer visible until speech/scroll is actually finished.
}

void StellarMax35Display::UpdateHomeInfoInternal() {
    if (!home_.root) return;
    std::time_t now = std::time(nullptr);
    std::tm tm{};
    localtime_r(&now, &tm);
    char t[16]; char d[64];
    if (now > 1700000000) {
        std::snprintf(t, sizeof(t), "%02d:%02d", tm.tm_hour, tm.tm_min);
        std::snprintf(d, sizeof(d), "%d月%d日  %s", tm.tm_mon + 1, tm.tm_mday, Weekday(tm.tm_wday));
    } else {
        std::snprintf(t, sizeof(t), "--:--");
        std::snprintf(d, sizeof(d), "时间同步中");
    }
    stellar_max35::HomeUiSetClock(&home_, t, d);
    stellar_max35::HomeUiSetWeather(&home_, last_weather_value_.c_str(), last_weather_detail_.c_str());
    const auto memo = stellar_max35::GetTodayMemoText();
    stellar_max35::HomeUiSetMemo(&home_, memo.c_str());
    stellar_max35::HomeUiSetStatus(&home_, StateText(last_state_), StateColor(last_state_));
}

void StellarMax35Display::UpdateConversationState(DeviceState state) {
    if (page_ != Page::Chat) return;
    if (state == kDeviceStateListening) stellar_max35::ChatUiSetState(&chat_, "我在听，请说话", kBlue);
    else if (state == kDeviceStateConnecting) stellar_max35::ChatUiSetState(&chat_, "正在连接 AI", kBlue);
    else if (state == kDeviceStateSpeaking) stellar_max35::ChatUiSetState(&chat_, "小智正在回答", kGold);
}

void StellarMax35Display::UpdateStatusBar(bool update_all) {
    LvglDisplay::UpdateStatusBar(update_all);
    const DeviceState state = Application::GetInstance().GetDeviceState();
    const int64_t now = esp_timer_get_time();
    {
        DisplayLockGuard lock(this);
        if (state != last_state_) {
            if (state == kDeviceStateIdle && suppress_chat_until_idle_) suppress_chat_until_idle_ = false;
            // Match the proven STELLAR interaction: wake-word/BOOT Listening opens chat immediately.
            if (!suppress_chat_until_idle_ && state == kDeviceStateListening && page_ == Page::Home) {
                conversation_active_ = true;
                return_home_us_ = 0;
                ShowPageInternal(Page::Chat);
                stellar_max35::ChatUiSetUser(&chat_, "正在聆听…");
                stellar_max35::ChatUiSetAnswer(&chat_, "请说，我在听。");
            }
            UpdateConversationState(state);
            last_state_ = state;
        }
        if (page_ == Page::Home) UpdateHomeInfoInternal();
        if (page_ == Page::Chat && conversation_active_) {
            if (state == kDeviceStateIdle) {
                if (scroll_finish_us_ > now) return_home_us_ = scroll_finish_us_ + kAfterScrollDelayUs;
                else if (return_home_us_ == 0) return_home_us_ = now + kReturnHomeDelayUs;
            } else if (IsConversing(state)) {
                return_home_us_ = 0;
            }
            if (return_home_us_ > 0 && now >= return_home_us_) {
                conversation_active_ = false;
                return_home_us_ = 0;
                scroll_finish_us_ = 0;
                ShowPageInternal(Page::Home);
                UpdateHomeInfoInternal();
            }
        }
    }
}
