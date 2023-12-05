#include "bt_tik_tiktok.h"
#include <furi.h>
#include <furi_hal_bt_hid.h>
#include <furi_hal_usb_hid.h>
#include <gui/elements.h>

#include "tiktok_remote_icons.h"

struct BtTikTikTok {
    View* view;
};

typedef struct {
    bool left_pressed;
    bool up_pressed;
    bool right_pressed;
    bool down_pressed;
    bool ok_pressed;
    bool connected;
    bool is_cursor_set;
} BtTikTikTokModel;


static void bt_tik_media_draw_arrow(Canvas* canvas, uint8_t x, uint8_t y, CanvasDirection dir) {
    canvas_draw_triangle(canvas, x, y, 5, 3, dir);
    if(dir == CanvasDirectionBottomToTop) {
        canvas_draw_dot(canvas, x, y - 1);
    } else if(dir == CanvasDirectionTopToBottom) {
        canvas_draw_dot(canvas, x, y + 1);
    } else if(dir == CanvasDirectionRightToLeft) {
        canvas_draw_dot(canvas, x - 1, y);
    } else if(dir == CanvasDirectionLeftToRight) {
        canvas_draw_dot(canvas, x + 1, y);
    }
}

static void bt_tik_tiktok_draw_callback(Canvas* canvas, void* context) {
    furi_assert(context);
    BtTikTikTokModel* model = context;

    // Header
    if(model->connected) {
        canvas_draw_icon(canvas, 0, 0, &I_Ble_connected_15x15);
    } else {
        canvas_draw_icon(canvas, 0, 0, &I_Ble_disconnected_15x15);
    }
    canvas_set_font(canvas, FontPrimary);
    elements_multiline_text_aligned(canvas, 17, 3, AlignLeft, AlignTop, "TikTok");
    canvas_set_font(canvas, FontSecondary);

    // Keypad circles
    canvas_draw_icon(canvas, 76, 8, &I_Circles_47x47);

    // Left
    if(model->left_pressed) {
        canvas_set_bitmap_mode(canvas, 1);
        canvas_draw_icon(canvas, 77, 25, &I_Pressed_Button_13x13);
        canvas_set_bitmap_mode(canvas, 0);
        canvas_set_color(canvas, ColorWhite);
    }
    bt_tik_media_draw_arrow(canvas, 82, 31, CanvasDirectionRightToLeft);
    bt_tik_media_draw_arrow(canvas, 86, 31, CanvasDirectionRightToLeft);
    canvas_set_color(canvas, ColorBlack);

    // right
    if(model->right_pressed) {
        canvas_set_bitmap_mode(canvas, 1);
        canvas_draw_icon(canvas, 109, 25, &I_Pressed_Button_13x13);
        canvas_set_bitmap_mode(canvas, 0);
        canvas_set_color(canvas, ColorWhite);
    }
    bt_tik_media_draw_arrow(canvas, 112, 31, CanvasDirectionLeftToRight);
    bt_tik_media_draw_arrow(canvas, 116, 31, CanvasDirectionLeftToRight);
    canvas_set_color(canvas, ColorBlack);
    // Up
    if(model->up_pressed) {
        canvas_set_bitmap_mode(canvas, 1);
        canvas_draw_icon(canvas, 93, 9, &I_Pressed_Button_13x13);
        canvas_set_bitmap_mode(canvas, 0);
        canvas_set_color(canvas, ColorWhite);
    }
    canvas_draw_icon(canvas, 96, 12, &I_Volup_8x6);
    canvas_set_color(canvas, ColorBlack);

    // down
    if(model->down_pressed) {
        canvas_set_bitmap_mode(canvas, 1);
        canvas_draw_icon(canvas, 93, 41, &I_Pressed_Button_13x13);
        canvas_set_bitmap_mode(canvas, 0);
        canvas_set_color(canvas, ColorWhite);
    }
    canvas_draw_icon(canvas, 96, 45, &I_Voldwn_6x6);
    canvas_set_color(canvas, ColorBlack);

    // Ok
    if(model->ok_pressed) {
        canvas_draw_icon(canvas, 93, 25, &I_Pressed_Button_13x13);
        canvas_set_color(canvas, ColorWhite);
    }
    bt_tik_media_draw_arrow(canvas, 96, 31, CanvasDirectionLeftToRight);
    canvas_draw_line(canvas, 100, 29, 100, 33);
    canvas_draw_line(canvas, 102, 29, 102, 33);
    canvas_set_color(canvas, ColorBlack);
    // Exit
    canvas_draw_icon(canvas, 0, 54, &I_Pin_back_arrow_10x8);
    canvas_set_font(canvas, FontSecondary);
    elements_multiline_text_aligned(canvas, 13, 62, AlignLeft, AlignBottom, "Hold to exit");
}

static void bt_tik_tiktok_reset_cursor() {
    // Set cursor to the phone's left up corner
    // Delays to guarantee one packet per connection interval
    for(size_t i = 0; i < 3; i++) {
        furi_hal_bt_hid_mouse_release(HID_MOUSE_BTN_LEFT);
        furi_hal_bt_hid_mouse_move(-127, -127);
        furi_delay_ms(50);
    }
    // Move cursor from the corner
    furi_hal_bt_hid_mouse_move(50, 120);
    furi_delay_ms(50);
}

static void bt_tik_tiktok_process_press(BtTikTikTokModel* model, InputEvent* event) {
    if(event->key == InputKeyUp) {
        model->up_pressed = true;
        furi_hal_bt_hid_consumer_key_press(HID_CONSUMER_VOLUME_INCREMENT);
    } else if(event->key == InputKeyDown) {
        model->down_pressed = true;
        furi_hal_bt_hid_consumer_key_press(HID_CONSUMER_VOLUME_DECREMENT);
    } else if(event->key == InputKeyLeft) {
        model->left_pressed = true;
    } else if(event->key == InputKeyRight) {
        model->right_pressed = true;

    } else if(event->key == InputKeyOk) {
        model->ok_pressed = true;
    }
}

static void bt_tik_tiktok_process_release(BtTikTikTokModel* model, InputEvent* event) {
    if(event->key == InputKeyUp) {
        model->up_pressed = false;
        furi_hal_bt_hid_consumer_key_release(HID_CONSUMER_VOLUME_INCREMENT);
    } else if(event->key == InputKeyDown) {
        model->down_pressed = false;
        furi_hal_bt_hid_consumer_key_release(HID_CONSUMER_VOLUME_DECREMENT);
    } else if(event->key == InputKeyLeft) {
        model->left_pressed = false;
    } else if(event->key == InputKeyRight) {
        model->right_pressed = false;
    } else if(event->key == InputKeyOk) {
        model->ok_pressed = false;
    }
}

static bool bt_tik_tiktok_input_callback(InputEvent* event, void* context) {
    furi_assert(context);
    BtTikTikTok* bt_tik_tiktok = context;
    bool consumed = false;

    with_view_model(
        bt_tik_tiktok->view,
        BtTikTikTokModel * model,
        {
            if(event->type == InputTypePress) {
                bt_tik_tiktok_process_press(model, event);
                if(model->connected && !model->is_cursor_set) {
                    bt_tik_tiktok_reset_cursor();
                    model->is_cursor_set = true;
                }
                consumed = true;
            } else if(event->type == InputTypeRelease) {
                bt_tik_tiktok_process_release(model, event);
                consumed = true;
                //play or pause
            } else if(event->type == InputTypeShort) {
                if(event->key == InputKeyOk) {
                    furi_hal_bt_hid_mouse_press(HID_MOUSE_BTN_LEFT);
                    furi_delay_ms(50);
                    furi_hal_bt_hid_mouse_release(HID_MOUSE_BTN_LEFT);

                    consumed = true;
                } else if(event->key == InputKeyLeft) {
                    // Emulate up swipe
                    furi_hal_bt_hid_mouse_scroll(-6);
                    furi_hal_bt_hid_mouse_scroll(-12);
                    furi_hal_bt_hid_mouse_scroll(-19);
                    furi_hal_bt_hid_mouse_scroll(-12);
                    furi_hal_bt_hid_mouse_scroll(-6);
                    consumed = true;
                } else if(event->key == InputKeyRight) {
                    // Emulate down swipe
                    furi_hal_bt_hid_mouse_scroll(6);
                    furi_hal_bt_hid_mouse_scroll(12);
                    furi_hal_bt_hid_mouse_scroll(19);
                    furi_hal_bt_hid_mouse_scroll(12);
                    furi_hal_bt_hid_mouse_scroll(6);
                    consumed = true;
                } else if(event->key == InputKeyBack) {
                    furi_hal_bt_hid_consumer_key_release_all();
                    consumed = true;
                }
            } else if(event->type == InputTypeLong) {
                if(event->key == InputKeyOk) {
                    furi_hal_bt_hid_mouse_press(HID_MOUSE_BTN_LEFT);
                    furi_delay_ms(50);
                    furi_hal_bt_hid_mouse_release(HID_MOUSE_BTN_LEFT);
                    furi_delay_ms(50);
                    furi_hal_bt_hid_mouse_press(HID_MOUSE_BTN_LEFT);
                    furi_delay_ms(50);
                    furi_hal_bt_hid_mouse_release(HID_MOUSE_BTN_LEFT);
                    model->is_cursor_set = false;
                    consumed = false;
                }else if (event->key == InputKeyBack) {
                    furi_hal_bt_hid_consumer_key_release_all();
                    model->is_cursor_set = false;
                    consumed = false;
                }

            }
        },
        true);

    return consumed;
}

BtTikTikTok* bt_tik_tiktok_alloc() {
    BtTikTikTok* bt_tik_tiktok = malloc(sizeof(BtTikTikTok));
    bt_tik_tiktok->view = view_alloc();
    view_set_context(bt_tik_tiktok->view, bt_tik_tiktok);
    view_allocate_model(bt_tik_tiktok->view, ViewModelTypeLocking, sizeof(BtTikTikTokModel));
    view_set_draw_callback(bt_tik_tiktok->view, bt_tik_tiktok_draw_callback);
    view_set_input_callback(bt_tik_tiktok->view, bt_tik_tiktok_input_callback);

    return bt_tik_tiktok;
}

void bt_tik_tiktok_free(BtTikTikTok* bt_tik_tiktok) {
    furi_assert(bt_tik_tiktok);
    view_free(bt_tik_tiktok->view);
    free(bt_tik_tiktok);
}

View* bt_tik_tiktok_get_view(BtTikTikTok* bt_tik_tiktok) {
    furi_assert(bt_tik_tiktok);
    return bt_tik_tiktok->view;
}

void bt_tik_tiktok_set_connected_status(BtTikTikTok* bt_tik_tiktok, bool connected) {
    furi_assert(bt_tik_tiktok);
    with_view_model(
        bt_tik_tiktok->view,
        BtTikTikTokModel * model,
        {
            model->connected = connected;
            model->is_cursor_set = false;
        },
        true);
}
