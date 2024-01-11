#include <furi.h>
#include <gui/gui.h>
#include <gui/icon_i.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <gui/modules/menu.h>
#include <gui/modules/popup.h>

#define TAG "ble-friend-app"

// Referenced guide and code from https://instantiator.dev/post/flipper-zero-app-tutorial-02/

/* this will be generated from the ".png" files in the "images" folder */
#include <ble_friend_app_icons.h>

/** scene ids that will be used by the app */
typedef enum {
    ble_friend_app_scene_main_menu,
    ble_friend_app_scene_connect,
    ble_friend_app_scene_terminal,
    ble_friend_app_scene_count
} ble_friend_app_scene;

/** ids for the 2 types of view used by the app */
typedef enum { ble_friend_app_view_menu, ble_friend_app_view_popup } ble_friend_app_view;

/** The app context struct */
typedef struct {
    SceneManager* scene_manager;
    ViewDispatcher* view_dispatcher;
    Menu* menu;
    Popup* popup;
} ble_friend_app;

/** all custom events */
typedef enum {
    ble_friend_app_show_connect_popup,
    ble_friend_app_show_terminal_popup
} ble_friend_app_event;

/* ---------- main menu scene ---------- */

/** indices for menu items */
typedef enum {
    ble_friend_app_connect_selection,
    ble_friend_terminal_selection
} ble_friend_app_selection;

/** main menu callback - sends a custom event to the scene manager based on the menu selection */
void ble_friend_app_menu_callback_main_menu(void* context, uint32_t index) {
    FURI_LOG_T(TAG, "ble_friend_app_menu_callback_main_menu");
    ble_friend_app* app = context;
    switch(index) {
    case ble_friend_app_connect_selection:
        scene_manager_handle_custom_event(app->scene_manager, ble_friend_app_show_connect_popup);
        break;
    case ble_friend_terminal_selection:
        scene_manager_handle_custom_event(app->scene_manager, ble_friend_app_show_terminal_popup);
        break;
    }
}

/** resets the menu, gives it content, callbacks and selection enums */
void ble_friend_app_scene_on_enter_main_menu(void* context) {
    FURI_LOG_T(TAG, "ble_friend_app_scene_on_enter_main_menu");
    ble_friend_app* app = context;
    menu_reset(app->menu);

    menu_add_item(
        app->menu,
        "Connect popup",
        NULL,
        ble_friend_app_show_connect_popup,
        ble_friend_app_menu_callback_main_menu,
        app);
    menu_add_item(
        app->menu,
        "Terminal popup",
        NULL,
        ble_friend_app_show_terminal_popup,
        ble_friend_app_menu_callback_main_menu,
        app);
    view_dispatcher_switch_to_view(app->view_dispatcher, ble_friend_app_view_menu);
}

/** main menu event handler - switches scene based on the event */
bool ble_friend_app_scene_on_event_main_menu(void* context, SceneManagerEvent event) {
    FURI_LOG_T(TAG, "ble_friend_app_scene_on_event_main_menu");
    ble_friend_app* app = context;
    bool consumed = false;
    switch(event.type) {
    case SceneManagerEventTypeCustom:
        switch(event.event) {
        case ble_friend_app_show_connect_popup:
            scene_manager_next_scene(app->scene_manager, ble_friend_app_scene_connect);
            consumed = true;
            break;
        case ble_friend_app_show_terminal_popup:
            scene_manager_next_scene(app->scene_manager, ble_friend_app_scene_terminal);
            consumed = true;
            break;
        }
        break;
    default: // eg. SceneManagerEventTypeBack, SceneManagerEventTypeTick
        consumed = false;
        break;
    }
    return consumed;
}

void ble_friend_app_scene_on_exit_main_menu(void* context) {
    FURI_LOG_T(TAG, "ble_friend_app_scene_on_exit_main_menu");
    ble_friend_app* app = context;
    menu_reset(app->menu);
}

/* Connection popup scene */

void ble_friend_app_scene_on_enter_connect_popup(void* context) {
    FURI_LOG_T(TAG, "ble_friend_app_scene_on_enter_connect_popup");
    ble_friend_app* app = context;
    popup_reset(app->popup);
    popup_set_context(app->popup, app);
    popup_set_header(app->popup, "Connect Popup", 64, 10, AlignCenter, AlignTop);
    popup_set_icon(app->popup, 10, 10, &I_connect);
    popup_set_text(app->popup, "Connect popup! (press back)  ", 64, 20, AlignLeft, AlignTop);
    view_dispatcher_switch_to_view(app->view_dispatcher, ble_friend_app_view_popup);
}

bool ble_friend_app_scene_on_event_connect_popup(void* context, SceneManagerEvent event) {
    FURI_LOG_T(TAG, "ble_friend_app_scene_on_event_connect_popup");
    UNUSED(context);
    UNUSED(event);
    return false; // don't handle any events
}

void ble_friend_app_scene_on_exit_connect_popup(void* context) {
    FURI_LOG_T(TAG, "ble_friend_app_scene_on_exit_connect_popup");
    ble_friend_app* app = context;
    popup_reset(app->popup);
}

/* Terminal popup scene */

void ble_friend_app_scene_on_enter_terminal_popup(void* context) {
    FURI_LOG_T(TAG, "ble_friend_app_scene_on_enter_terminal_popup");
    ble_friend_app* app = context;
    popup_reset(app->popup);
    popup_set_context(app->popup, app);
    popup_set_header(app->popup, "Terminal Popup", 64, 10, AlignCenter, AlignTop);
    popup_set_icon(app->popup, 10, 10, &I_terminal);
    popup_set_text(
        app->popup, "Terminal popup. (press back)", 64, 20, AlignLeft, AlignTop);
    view_dispatcher_switch_to_view(app->view_dispatcher, ble_friend_app_view_popup);
}

bool ble_friend_app_scene_on_event_terminal_popup(void* context, SceneManagerEvent event) {
    FURI_LOG_T(TAG, "ble_friend_app_scene_on_event_terminal_popup");
    UNUSED(context);
    UNUSED(event);
    return false; // don't handle any events
}

void ble_friend_app_scene_on_exit_terminal_popup(void* context) {
    FURI_LOG_T(TAG, "ble_friend_app_scene_on_exit_terminal_popup");
    ble_friend_app* app = context;
    popup_reset(app->popup);
}

/** collection of all scene on_enter handlers - in the same order as their enum */
void (*const ble_friend_app_scene_on_enter_handlers[])(void*) = {
    ble_friend_app_scene_on_enter_main_menu,
    ble_friend_app_scene_on_enter_connect_popup,
    ble_friend_app_scene_on_enter_terminal_popup};

/** collection of all scene on event handlers - in the same order as their enum */
bool (*const ble_friend_app_scene_on_event_handlers[])(void*, SceneManagerEvent) = {
    ble_friend_app_scene_on_event_main_menu,
    ble_friend_app_scene_on_event_connect_popup,
    ble_friend_app_scene_on_event_terminal_popup};

/** collection of all scene on exit handlers - in the same order as their enum */
void (*const ble_friend_app_scene_on_exit_handlers[])(void*) = {
    ble_friend_app_scene_on_exit_main_menu,
    ble_friend_app_scene_on_exit_connect_popup,
    ble_friend_app_scene_on_exit_terminal_popup};

/** collection of all on_enter, on_event, on_exit handlers */
const SceneManagerHandlers ble_friend_app_scene_event_handlers = {
    .on_enter_handlers = ble_friend_app_scene_on_enter_handlers,
    .on_event_handlers = ble_friend_app_scene_on_event_handlers,
    .on_exit_handlers = ble_friend_app_scene_on_exit_handlers,
    .scene_num = ble_friend_app_scene_count};

/** custom event handler - passes the event to the scene manager */
bool ble_friend_app_scene_manager_custom_event_callback(void* context, uint32_t custom_event) {
    FURI_LOG_T(TAG, "ble_friend_app_scene_manager_custom_event_callback");
    furi_assert(context);
    ble_friend_app* app = context;
    return scene_manager_handle_custom_event(app->scene_manager, custom_event);
}

/** navigation event handler - passes the event to the scene manager */
bool ble_friend_app_scene_manager_navigation_event_callback(void* context) {
    FURI_LOG_T(TAG, "ble_friend_app_scene_manager_navigation_event_callback");
    furi_assert(context);
    ble_friend_app* app = context;
    return scene_manager_handle_back_event(app->scene_manager);
}

/** initialise the scene manager with all handlers */
void ble_friend_app_scene_manager_init(ble_friend_app* app) {
    FURI_LOG_T(TAG, "ble_friend_app_scene_manager_init");
    app->scene_manager = scene_manager_alloc(&ble_friend_app_scene_event_handlers, app);
}

/** initialise the views, and initialise the view dispatcher with all views */
void ble_friend_app_view_dispatcher_init(ble_friend_app* app) {
    FURI_LOG_T(TAG, "ble_friend_app_view_dispatcher_init");
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue(app->view_dispatcher);

    // allocate each view
    FURI_LOG_D(TAG, "ble_friend_app_view_dispatcher_init allocating views");
    app->menu = menu_alloc();
    app->popup = popup_alloc();

    // assign callback that pass events from views to the scene manager
    FURI_LOG_D(TAG, "ble_friend_app_view_dispatcher_init setting callbacks");
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(
        app->view_dispatcher, ble_friend_app_scene_manager_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(
        app->view_dispatcher, ble_friend_app_scene_manager_navigation_event_callback);

    // add views to the dispatcher, indexed by their enum value
    FURI_LOG_D(TAG, "ble_friend_app_view_dispatcher_init adding view menu");
    view_dispatcher_add_view(
        app->view_dispatcher, ble_friend_app_view_menu, menu_get_view(app->menu));

    FURI_LOG_D(TAG, "ble_friend_app_view_dispatcher_init adding view popup");
    view_dispatcher_add_view(
        app->view_dispatcher, ble_friend_app_view_popup, popup_get_view(app->popup));
}

/** initialise app data, scene manager, and view dispatcher */
ble_friend_app* ble_friend_app_init() {
    FURI_LOG_T(TAG, "ble_friend_app_init");
    ble_friend_app* app = malloc(sizeof(ble_friend_app));
    ble_friend_app_scene_manager_init(app);
    ble_friend_app_view_dispatcher_init(app);
    return app;
}

/** free all app data, scene manager, and view dispatcher */
void ble_friend_app_free(ble_friend_app* app) {
    FURI_LOG_T(TAG, "ble_friend_app_free");
    scene_manager_free(app->scene_manager);
    view_dispatcher_remove_view(app->view_dispatcher, ble_friend_app_view_menu);
    view_dispatcher_remove_view(app->view_dispatcher, ble_friend_app_view_popup);
    view_dispatcher_free(app->view_dispatcher);
    menu_free(app->menu);
    popup_free(app->popup);
    free(app);
}

/** go to trace log level in the dev environment */
void ble_friend_app_set_log_level() {
#ifdef FURI_DEBUG
    furi_log_set_level(FuriLogLevelTrace);
#else
    furi_log_set_level(FuriLogLevelInfo);
#endif
}

/** entrypoint */
int32_t ble_friend_app_app(void* p) {
    UNUSED(p);
    ble_friend_app_set_log_level();

    // create the app context struct, scene manager, and view dispatcher
    FURI_LOG_I(TAG, "BLE Friend app starting...");
    // ble_friend_app* app = ble_friend_app_app();
    ble_friend_app* app = ble_friend_app_init();

    // set the scene and launch the main loop
    Gui* gui = furi_record_open(RECORD_GUI);
    view_dispatcher_attach_to_gui(app->view_dispatcher, gui, ViewDispatcherTypeFullscreen);
    scene_manager_next_scene(app->scene_manager, ble_friend_app_scene_main_menu);
    FURI_LOG_D(TAG, "Starting dispatcher...");
    view_dispatcher_run(app->view_dispatcher);

    // free all memory
    FURI_LOG_I(TAG, "BLE Friend app finishing...");
    furi_record_close(RECORD_GUI);
    ble_friend_app_free(app);
    return 0;
}