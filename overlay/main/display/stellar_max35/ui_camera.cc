#include "ui_pages.h"

namespace stellar_max35 {
namespace {
constexpr uint32_t kInk = 0xF8FAFC;
constexpr uint32_t kMuted = 0xD5DEE8;
constexpr uint32_t kAccent = 0xA7B98A;

void StyleGuide(lv_obj_t* p, int x, int y, int w, int h) {
    auto* o = lv_obj_create(p);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(o, lv_color_white(), 0);
    lv_obj_set_style_border_width(o, 2, 0);
    lv_obj_set_style_border_opa(o, LV_OPA_70, 0);
    lv_obj_set_style_radius(o, 14, 0);
    lv_obj_set_style_pad_all(o, 0, 0);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
}

void ShutterEvent(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    auto* ui = static_cast<CameraUi*>(lv_event_get_user_data(e));
    if (ui && ui->callback) ui->callback(ui->callback_user_data);
}
}

void BuildCameraUi(lv_obj_t* screen, CameraUi* ui,
                   CameraShutterCallback callback, void* user_data) {
    ui->root = lv_obj_create(screen);
    lv_obj_set_pos(ui->root, 0, 0);
    lv_obj_set_size(ui->root, 480, 320);
    lv_obj_set_style_bg_color(ui->root, lv_color_hex(0x30363B), 0);
    lv_obj_set_style_bg_grad_color(ui->root, lv_color_hex(0x55504B), 0);
    lv_obj_set_style_bg_grad_dir(ui->root, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_border_width(ui->root, 0, 0);
    lv_obj_set_style_radius(ui->root, 0, 0);
    lv_obj_set_style_pad_all(ui->root, 0, 0);
    lv_obj_remove_flag(ui->root, LV_OBJ_FLAG_SCROLLABLE);

    ui->title = lv_label_create(ui->root);
    lv_label_set_text(ui->title, "拍照搜题");
    lv_obj_set_pos(ui->title, 18, 12);
    lv_obj_set_width(ui->title, 150);
    lv_obj_set_style_text_color(ui->title, lv_color_hex(kInk), 0);

    ui->hint = lv_label_create(ui->root);
    lv_label_set_text(ui->hint, "将题目放入取景框，点一下快门");
    lv_obj_set_pos(ui->hint, 170, 12);
    lv_obj_set_width(ui->hint, 290);
    lv_obj_set_style_text_color(ui->hint, lv_color_hex(kMuted), 0);
    lv_obj_set_style_text_align(ui->hint, LV_TEXT_ALIGN_RIGHT, 0);

    // V1 deliberately uses a framing overlay instead of assuming a live-preview API
    // that the vendor's xiaozhi camera abstraction may not expose.
    StyleGuide(ui->root, 28, 50, 424, 186);
    auto* center = lv_label_create(ui->root);
    lv_label_set_text(center, "+");
    lv_obj_set_pos(center, 228, 124);
    lv_obj_set_width(center, 24);
    lv_obj_set_style_text_color(center, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(center, LV_TEXT_ALIGN_CENTER, 0);

    ui->status = lv_label_create(ui->root);
    lv_label_set_text(ui->status, "后置摄像头已就绪");
    lv_obj_set_pos(ui->status, 30, 246);
    lv_obj_set_width(ui->status, 275);
    lv_obj_set_style_text_color(ui->status, lv_color_hex(kMuted), 0);

    ui->shutter = lv_obj_create(ui->root);
    lv_obj_set_pos(ui->shutter, 374, 245);
    lv_obj_set_size(ui->shutter, 72, 60);
    lv_obj_set_style_radius(ui->shutter, 30, 0);
    lv_obj_set_style_bg_color(ui->shutter, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(ui->shutter, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ui->shutter, lv_color_hex(kAccent), 0);
    lv_obj_set_style_border_width(ui->shutter, 4, 0);
    lv_obj_set_style_pad_all(ui->shutter, 0, 0);
    lv_obj_remove_flag(ui->shutter, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui->shutter, LV_OBJ_FLAG_CLICKABLE);
    ui->shutter_icon = lv_label_create(ui->shutter);
    lv_label_set_text(ui->shutter_icon, "●");
    lv_obj_center(ui->shutter_icon);
    lv_obj_set_style_text_color(ui->shutter_icon, lv_color_hex(kAccent), 0);

    // CameraUi has the same lifetime as the display controller, so no heap allocation
    // is needed for the touch callback context.
    ui->callback = callback;
    ui->callback_user_data = user_data;
    lv_obj_add_event_cb(ui->shutter, ShutterEvent, LV_EVENT_CLICKED, ui);
}

void CameraUiSetPrompt(CameraUi* ui, const char* question) {
    if (!ui || !ui->hint) return;
    if (question && question[0]) {
        lv_label_set_text(ui->hint, "对准题目后点击快门");
    }
}

void CameraUiSetState(CameraUi* ui, const char* text, bool busy) {
    if (ui && ui->status) {
        lv_label_set_text(ui->status, text ? text : "");
        lv_obj_set_style_text_color(ui->status,
                                    lv_color_hex(busy ? 0xF3D7A1 : kMuted), 0);
    }
}

void CameraUiSetEnabled(CameraUi* ui, bool enabled) {
    if (!ui || !ui->shutter) return;
    if (enabled) {
        lv_obj_add_flag(ui->shutter, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_opa(ui->shutter, LV_OPA_COVER, 0);
    } else {
        lv_obj_remove_flag(ui->shutter, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_opa(ui->shutter, LV_OPA_50, 0);
    }
}

}  // namespace stellar_max35
