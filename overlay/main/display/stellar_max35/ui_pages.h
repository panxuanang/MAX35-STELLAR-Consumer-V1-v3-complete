#pragma once

#include <lvgl.h>
#include <cstdint>

namespace stellar_max35 {

using CameraShutterCallback = void (*)(void* user_data);

struct HomeUi {
    lv_obj_t* root = nullptr;
    lv_obj_t* time = nullptr;
    lv_obj_t* date = nullptr;
    lv_obj_t* weather = nullptr;
    lv_obj_t* weather_detail = nullptr;
    lv_obj_t* memo = nullptr;
    lv_obj_t* status = nullptr;
};

struct ChatUi {
    lv_obj_t* root = nullptr;
    lv_obj_t* user = nullptr;
    lv_obj_t* answer_box = nullptr;
    lv_obj_t* answer = nullptr;
    lv_obj_t* state = nullptr;
};

struct CameraUi {
    lv_obj_t* root = nullptr;
    lv_obj_t* title = nullptr;
    lv_obj_t* hint = nullptr;
    lv_obj_t* shutter = nullptr;
    lv_obj_t* shutter_icon = nullptr;
    lv_obj_t* status = nullptr;
    CameraShutterCallback callback = nullptr;
    void* callback_user_data = nullptr;
};

// Each page is intentionally isolated in one .cc file. For later visual redesigns,
// replace only ui_home.cc, ui_chat.cc or ui_camera.cc and keep the controller intact.
void BuildHomeUi(lv_obj_t* screen, HomeUi* ui);
void HomeUiSetClock(HomeUi* ui, const char* time_text, const char* date_text);
void HomeUiSetWeather(HomeUi* ui, const char* value, const char* detail);
void HomeUiSetMemo(HomeUi* ui, const char* memo_text);
void HomeUiSetStatus(HomeUi* ui, const char* state_text, uint32_t color);

void BuildChatUi(lv_obj_t* screen, ChatUi* ui);
void ChatUiSetUser(ChatUi* ui, const char* text);
void ChatUiSetAnswer(ChatUi* ui, const char* text);
void ChatUiSetState(ChatUi* ui, const char* text, uint32_t color);
void ChatUiStopScroll(ChatUi* ui);
int64_t ChatUiStartReadableScroll(ChatUi* ui, int delay_ms, int ms_per_pixel);

void BuildCameraUi(lv_obj_t* screen, CameraUi* ui,
                   CameraShutterCallback callback, void* user_data);
void CameraUiSetPrompt(CameraUi* ui, const char* question);
void CameraUiSetState(CameraUi* ui, const char* text, bool busy);
void CameraUiSetEnabled(CameraUi* ui, bool enabled);

}  // namespace stellar_max35
