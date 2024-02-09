#include "screen_pointclick.h"
#include "application.h"
#include "badge-communication-protocol.h"
#include "bsp.h"
#include "esp_err.h"
#include "esp_log.h"
#include "events.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"
#include "pax_codecs.h"
#include "pax_gfx.h"
#include "screen_home.h"
#include "screens.h"
#include "textedit.h"
#include <inttypes.h>
#include <stdio.h>
#include <esp_random.h>
#include <string.h>

static const char* TAG = "point & click";

extern const uint8_t town2_s_S[] asm("_binary_town2_s_png_start");
extern const uint8_t town2_s_E[] asm("_binary_town2_s_png_end");
extern const uint8_t town2_n_S[] asm("_binary_town2_n_png_start");
extern const uint8_t town2_n_E[] asm("_binary_town2_n_png_end");
extern const uint8_t town1_w_S[] asm("_binary_town1_w_png_start");
extern const uint8_t town1_w_E[] asm("_binary_town1_w_png_end");
extern const uint8_t town1_s_d1_messageboard_S[] asm("_binary_town1_s_d1_messageboard_png_start");
extern const uint8_t town1_s_d1_messageboard_E[] asm("_binary_town1_s_d1_messageboard_png_end");
extern const uint8_t town1_s_d1_boat_S[] asm("_binary_town1_s_d1_boat_png_start");
extern const uint8_t town1_s_d1_boat_E[] asm("_binary_town1_s_d1_boat_png_end");
extern const uint8_t town1_s_S[] asm("_binary_town1_s_png_start");
extern const uint8_t town1_s_E[] asm("_binary_town1_s_png_end");
extern const uint8_t town1_n_d1_S[] asm("_binary_town1_n_d1_png_start");
extern const uint8_t town1_n_d1_E[] asm("_binary_town1_n_d1_png_end");
extern const uint8_t town1_n_S[] asm("_binary_town1_n_png_start");
extern const uint8_t town1_n_E[] asm("_binary_town1_n_png_end");
extern const uint8_t town1_e_d1_S[] asm("_binary_town1_e_d1_png_start");
extern const uint8_t town1_e_d1_E[] asm("_binary_town1_e_d1_png_end");
extern const uint8_t town1_e_S[] asm("_binary_town1_e_png_start");
extern const uint8_t town1_e_E[] asm("_binary_town1_e_png_end");
extern const uint8_t shop_closeup_d9a_S[] asm("_binary_shop_closeup_d9a_png_start");
extern const uint8_t shop_closeup_d9a_E[] asm("_binary_shop_closeup_d9a_png_end");
extern const uint8_t shop_closeup_d8a_S[] asm("_binary_shop_closeup_d8a_png_start");
extern const uint8_t shop_closeup_d8a_E[] asm("_binary_shop_closeup_d8a_png_end");
extern const uint8_t shop_closeup_d7a_S[] asm("_binary_shop_closeup_d7a_png_start");
extern const uint8_t shop_closeup_d7a_E[] asm("_binary_shop_closeup_d7a_png_end");
extern const uint8_t shop_closeup_d6a_S[] asm("_binary_shop_closeup_d6a_png_start");
extern const uint8_t shop_closeup_d6a_E[] asm("_binary_shop_closeup_d6a_png_end");
extern const uint8_t shop_closeup_d5a_S[] asm("_binary_shop_closeup_d5a_png_start");
extern const uint8_t shop_closeup_d5a_E[] asm("_binary_shop_closeup_d5a_png_end");
extern const uint8_t shop_closeup_d4a_S[] asm("_binary_shop_closeup_d4a_png_start");
extern const uint8_t shop_closeup_d4a_E[] asm("_binary_shop_closeup_d4a_png_end");
extern const uint8_t shop_closeup_d4_S[] asm("_binary_shop_closeup_d4_png_start");
extern const uint8_t shop_closeup_d4_E[] asm("_binary_shop_closeup_d4_png_end");
extern const uint8_t shop_closeup_d3a_S[] asm("_binary_shop_closeup_d3a_png_start");
extern const uint8_t shop_closeup_d3a_E[] asm("_binary_shop_closeup_d3a_png_end");
extern const uint8_t shop_closeup_d3_S[] asm("_binary_shop_closeup_d3_png_start");
extern const uint8_t shop_closeup_d3_E[] asm("_binary_shop_closeup_d3_png_end");
extern const uint8_t shop_closeup_d2a_S[] asm("_binary_shop_closeup_d2a_png_start");
extern const uint8_t shop_closeup_d2a_E[] asm("_binary_shop_closeup_d2a_png_end");
extern const uint8_t shop_closeup_d2_S[] asm("_binary_shop_closeup_d2_png_start");
extern const uint8_t shop_closeup_d2_E[] asm("_binary_shop_closeup_d2_png_end");
extern const uint8_t shop_closeup_d1a_S[] asm("_binary_shop_closeup_d1a_png_start");
extern const uint8_t shop_closeup_d1a_E[] asm("_binary_shop_closeup_d1a_png_end");
extern const uint8_t shop_closeup_d15a_S[] asm("_binary_shop_closeup_d15a_png_start");
extern const uint8_t shop_closeup_d15a_E[] asm("_binary_shop_closeup_d15a_png_end");
extern const uint8_t shop_closeup_d14a_S[] asm("_binary_shop_closeup_d14a_png_start");
extern const uint8_t shop_closeup_d14a_E[] asm("_binary_shop_closeup_d14a_png_end");
extern const uint8_t shop_closeup_d13a_S[] asm("_binary_shop_closeup_d13a_png_start");
extern const uint8_t shop_closeup_d13a_E[] asm("_binary_shop_closeup_d13a_png_end");
extern const uint8_t shop_closeup_d12a_S[] asm("_binary_shop_closeup_d12a_png_start");
extern const uint8_t shop_closeup_d12a_E[] asm("_binary_shop_closeup_d12a_png_end");
extern const uint8_t shop_closeup_d11a_S[] asm("_binary_shop_closeup_d11a_png_start");
extern const uint8_t shop_closeup_d11a_E[] asm("_binary_shop_closeup_d11a_png_end");
extern const uint8_t shop_closeup_d10a_S[] asm("_binary_shop_closeup_d10a_png_start");
extern const uint8_t shop_closeup_d10a_E[] asm("_binary_shop_closeup_d10a_png_end");
extern const uint8_t shop_closeup_d1_S[] asm("_binary_shop_closeup_d1_png_start");
extern const uint8_t shop_closeup_d1_E[] asm("_binary_shop_closeup_d1_png_end");
extern const uint8_t shop_closeup_S[] asm("_binary_shop_closeup_png_start");
extern const uint8_t shop_closeup_E[] asm("_binary_shop_closeup_png_end");
extern const uint8_t road1_w_S[] asm("_binary_road1_w_png_start");
extern const uint8_t road1_w_E[] asm("_binary_road1_w_png_end");
extern const uint8_t road1_n_go_S[] asm("_binary_road1_n_go_png_start");
extern const uint8_t road1_n_go_E[] asm("_binary_road1_n_go_png_end");
extern const uint8_t road1_n_S[] asm("_binary_road1_n_png_start");
extern const uint8_t road1_n_E[] asm("_binary_road1_n_png_end");
extern const uint8_t road1_e_jorge_d7_S[] asm("_binary_road1_e_jorge_d7_png_start");
extern const uint8_t road1_e_jorge_d7_E[] asm("_binary_road1_e_jorge_d7_png_end");
extern const uint8_t road1_e_jorge_d6_S[] asm("_binary_road1_e_jorge_d6_png_start");
extern const uint8_t road1_e_jorge_d6_E[] asm("_binary_road1_e_jorge_d6_png_end");
extern const uint8_t road1_e_jorge_d5_S[] asm("_binary_road1_e_jorge_d5_png_start");
extern const uint8_t road1_e_jorge_d5_E[] asm("_binary_road1_e_jorge_d5_png_end");
extern const uint8_t road1_e_jorge_d4_S[] asm("_binary_road1_e_jorge_d4_png_start");
extern const uint8_t road1_e_jorge_d4_E[] asm("_binary_road1_e_jorge_d4_png_end");
extern const uint8_t road1_e_jorge_d3_S[] asm("_binary_road1_e_jorge_d3_png_start");
extern const uint8_t road1_e_jorge_d3_E[] asm("_binary_road1_e_jorge_d3_png_end");
extern const uint8_t road1_e_jorge_d2_S[] asm("_binary_road1_e_jorge_d2_png_start");
extern const uint8_t road1_e_jorge_d2_E[] asm("_binary_road1_e_jorge_d2_png_end");
extern const uint8_t road1_e_jorge_d1_S[] asm("_binary_road1_e_jorge_d1_png_start");
extern const uint8_t road1_e_jorge_d1_E[] asm("_binary_road1_e_jorge_d1_png_end");
extern const uint8_t road1_e_jorge_S[] asm("_binary_road1_e_jorge_png_start");
extern const uint8_t road1_e_jorge_E[] asm("_binary_road1_e_jorge_png_end");
extern const uint8_t road1_e_S[] asm("_binary_road1_e_png_start");
extern const uint8_t road1_e_E[] asm("_binary_road1_e_png_end");
extern const uint8_t postoffice_closeup_d9_S[] asm("_binary_postoffice_closeup_d9_png_start");
extern const uint8_t postoffice_closeup_d9_E[] asm("_binary_postoffice_closeup_d9_png_end");
extern const uint8_t postoffice_closeup_d8_S[] asm("_binary_postoffice_closeup_d8_png_start");
extern const uint8_t postoffice_closeup_d8_E[] asm("_binary_postoffice_closeup_d8_png_end");
extern const uint8_t postoffice_closeup_d7_S[] asm("_binary_postoffice_closeup_d7_png_start");
extern const uint8_t postoffice_closeup_d7_E[] asm("_binary_postoffice_closeup_d7_png_end");
extern const uint8_t postoffice_closeup_d6_S[] asm("_binary_postoffice_closeup_d6_png_start");
extern const uint8_t postoffice_closeup_d6_E[] asm("_binary_postoffice_closeup_d6_png_end");
extern const uint8_t postoffice_closeup_d5_S[] asm("_binary_postoffice_closeup_d5_png_start");
extern const uint8_t postoffice_closeup_d5_E[] asm("_binary_postoffice_closeup_d5_png_end");
extern const uint8_t postoffice_closeup_d4_S[] asm("_binary_postoffice_closeup_d4_png_start");
extern const uint8_t postoffice_closeup_d4_E[] asm("_binary_postoffice_closeup_d4_png_end");
extern const uint8_t postoffice_closeup_d3_S[] asm("_binary_postoffice_closeup_d3_png_start");
extern const uint8_t postoffice_closeup_d3_E[] asm("_binary_postoffice_closeup_d3_png_end");
extern const uint8_t postoffice_closeup_d2_S[] asm("_binary_postoffice_closeup_d2_png_start");
extern const uint8_t postoffice_closeup_d2_E[] asm("_binary_postoffice_closeup_d2_png_end");
extern const uint8_t postoffice_closeup_d13_S[] asm("_binary_postoffice_closeup_d13_png_start");
extern const uint8_t postoffice_closeup_d13_E[] asm("_binary_postoffice_closeup_d13_png_end");
extern const uint8_t postoffice_closeup_d12_S[] asm("_binary_postoffice_closeup_d12_png_start");
extern const uint8_t postoffice_closeup_d12_E[] asm("_binary_postoffice_closeup_d12_png_end");
extern const uint8_t postoffice_closeup_d11_S[] asm("_binary_postoffice_closeup_d11_png_start");
extern const uint8_t postoffice_closeup_d11_E[] asm("_binary_postoffice_closeup_d11_png_end");
extern const uint8_t postoffice_closeup_d10_S[] asm("_binary_postoffice_closeup_d10_png_start");
extern const uint8_t postoffice_closeup_d10_E[] asm("_binary_postoffice_closeup_d10_png_end");
extern const uint8_t postoffice_closeup_d1_S[] asm("_binary_postoffice_closeup_d1_png_start");
extern const uint8_t postoffice_closeup_d1_E[] asm("_binary_postoffice_closeup_d1_png_end");
extern const uint8_t messageboard_closeup_S[] asm("_binary_messageboard_closeup_png_start");
extern const uint8_t messageboard_closeup_E[] asm("_binary_messageboard_closeup_png_end");
extern const uint8_t lighthouse2_w_S[] asm("_binary_lighthouse2_w_png_start");
extern const uint8_t lighthouse2_w_E[] asm("_binary_lighthouse2_w_png_end");
extern const uint8_t lighthouse2_s_S[] asm("_binary_lighthouse2_s_png_start");
extern const uint8_t lighthouse2_s_E[] asm("_binary_lighthouse2_s_png_end");
extern const uint8_t lighthouse2_n_go_S[] asm("_binary_lighthouse2_n_go_png_start");
extern const uint8_t lighthouse2_n_go_E[] asm("_binary_lighthouse2_n_go_png_end");
extern const uint8_t lighthouse2_n_S[] asm("_binary_lighthouse2_n_png_start");
extern const uint8_t lighthouse2_n_E[] asm("_binary_lighthouse2_n_png_end");
extern const uint8_t lighthouse2_e_S[] asm("_binary_lighthouse2_e_png_start");
extern const uint8_t lighthouse2_e_E[] asm("_binary_lighthouse2_e_png_end");
extern const uint8_t lighthouse1_s_S[] asm("_binary_lighthouse1_s_png_start");
extern const uint8_t lighthouse1_s_E[] asm("_binary_lighthouse1_s_png_end");
extern const uint8_t lighthouse1_n_go_S[] asm("_binary_lighthouse1_n_go_png_start");
extern const uint8_t lighthouse1_n_go_E[] asm("_binary_lighthouse1_n_go_png_end");
extern const uint8_t lighthouse1_n_gc_d2_key_S[] asm("_binary_lighthouse1_n_gc_d2_key_png_start");
extern const uint8_t lighthouse1_n_gc_d2_key_E[] asm("_binary_lighthouse1_n_gc_d2_key_png_end");
extern const uint8_t lighthouse1_n_gc_d1_key_S[] asm("_binary_lighthouse1_n_gc_d1_key_png_start");
extern const uint8_t lighthouse1_n_gc_d1_key_E[] asm("_binary_lighthouse1_n_gc_d1_key_png_end");
extern const uint8_t lighthouse1_n_gc_d1_S[] asm("_binary_lighthouse1_n_gc_d1_png_start");
extern const uint8_t lighthouse1_n_gc_d1_E[] asm("_binary_lighthouse1_n_gc_d1_png_end");
extern const uint8_t lighthouse1_n_gc_S[] asm("_binary_lighthouse1_n_gc_png_start");
extern const uint8_t lighthouse1_n_gc_E[] asm("_binary_lighthouse1_n_gc_png_end");
extern const uint8_t library_closeup_d1_S[] asm("_binary_library_closeup_d1_png_start");
extern const uint8_t library_closeup_d1_E[] asm("_binary_library_closeup_d1_png_end");
extern const uint8_t library_closeup_S[] asm("_binary_library_closeup_png_start");
extern const uint8_t library_closeup_E[] asm("_binary_library_closeup_png_end");
extern const uint8_t land1_w_S[] asm("_binary_land1_w_png_start");
extern const uint8_t land1_w_E[] asm("_binary_land1_w_png_end");
extern const uint8_t land1_s_S[] asm("_binary_land1_s_png_start");
extern const uint8_t land1_s_E[] asm("_binary_land1_s_png_end");
extern const uint8_t land1_n_S[] asm("_binary_land1_n_png_start");
extern const uint8_t land1_n_E[] asm("_binary_land1_n_png_end");
extern const uint8_t land1_e_S[] asm("_binary_land1_e_png_start");
extern const uint8_t land1_e_E[] asm("_binary_land1_e_png_end");
extern const uint8_t dune3_s_S[] asm("_binary_dune3_s_png_start");
extern const uint8_t dune3_s_E[] asm("_binary_dune3_s_png_end");
extern const uint8_t dune3_n_go_up_S[] asm("_binary_dune3_n_go_up_png_start");
extern const uint8_t dune3_n_go_up_E[] asm("_binary_dune3_n_go_up_png_end");
extern const uint8_t dune3_n_go_S[] asm("_binary_dune3_n_go_png_start");
extern const uint8_t dune3_n_go_E[] asm("_binary_dune3_n_go_png_end");
extern const uint8_t dune3_n_gc_up_S[] asm("_binary_dune3_n_gc_up_png_start");
extern const uint8_t dune3_n_gc_up_E[] asm("_binary_dune3_n_gc_up_png_end");
extern const uint8_t dune3_n_gc_d4_S[] asm("_binary_dune3_n_gc_d4_png_start");
extern const uint8_t dune3_n_gc_d4_E[] asm("_binary_dune3_n_gc_d4_png_end");
extern const uint8_t dune3_n_gc_d3_S[] asm("_binary_dune3_n_gc_d3_png_start");
extern const uint8_t dune3_n_gc_d3_E[] asm("_binary_dune3_n_gc_d3_png_end");
extern const uint8_t dune3_n_gc_d2_S[] asm("_binary_dune3_n_gc_d2_png_start");
extern const uint8_t dune3_n_gc_d2_E[] asm("_binary_dune3_n_gc_d2_png_end");
extern const uint8_t dune3_n_gc_d1_S[] asm("_binary_dune3_n_gc_d1_png_start");
extern const uint8_t dune3_n_gc_d1_E[] asm("_binary_dune3_n_gc_d1_png_end");
extern const uint8_t dune3_n_gc_S[] asm("_binary_dune3_n_gc_png_start");
extern const uint8_t dune3_n_gc_E[] asm("_binary_dune3_n_gc_png_end");

extern const uint8_t dune2_s_S[] asm("_binary_dune2_s_png_start");
extern const uint8_t dune2_s_E[] asm("_binary_dune2_s_png_end");
extern const uint8_t dune2_n_d1_S[] asm("_binary_dune2_n_d1_png_start");
extern const uint8_t dune2_n_d1_E[] asm("_binary_dune2_n_d1_png_end");
extern const uint8_t dune2_n_S[] asm("_binary_dune2_n_png_start");
extern const uint8_t dune2_n_E[] asm("_binary_dune2_n_png_end");
extern const uint8_t dune1_s_S[] asm("_binary_dune1_s_png_start");
extern const uint8_t dune1_s_E[] asm("_binary_dune1_s_png_end");
extern const uint8_t dune1_n_S[] asm("_binary_dune1_n_png_start");
extern const uint8_t dune1_n_E[] asm("_binary_dune1_n_png_end");
extern const uint8_t dock2_w_S[] asm("_binary_dock2_w_png_start");
extern const uint8_t dock2_w_E[] asm("_binary_dock2_w_png_end");
extern const uint8_t dock2_s_S[] asm("_binary_dock2_s_png_start");
extern const uint8_t dock2_s_E[] asm("_binary_dock2_s_png_end");
extern const uint8_t dock2_n_S[] asm("_binary_dock2_n_png_start");
extern const uint8_t dock2_n_E[] asm("_binary_dock2_n_png_end");
extern const uint8_t dock2_e_S[] asm("_binary_dock2_e_png_start");
extern const uint8_t dock2_e_E[] asm("_binary_dock2_e_png_end");
extern const uint8_t dock1_w_d2_S[] asm("_binary_dock1_w_d2_png_start");
extern const uint8_t dock1_w_d2_E[] asm("_binary_dock1_w_d2_png_end");
extern const uint8_t dock1_w_d1_S[] asm("_binary_dock1_w_d1_png_start");
extern const uint8_t dock1_w_d1_E[] asm("_binary_dock1_w_d1_png_end");
extern const uint8_t dock1_w_S[] asm("_binary_dock1_w_png_start");
extern const uint8_t dock1_w_E[] asm("_binary_dock1_w_png_end");
extern const uint8_t dock1_s_S[] asm("_binary_dock1_s_png_start");
extern const uint8_t dock1_s_E[] asm("_binary_dock1_s_png_end");
extern const uint8_t dock1_n_S[] asm("_binary_dock1_n_png_start");
extern const uint8_t dock1_n_E[] asm("_binary_dock1_n_png_end");
extern const uint8_t dock1_e_S[] asm("_binary_dock1_e_png_start");
extern const uint8_t dock1_e_E[] asm("_binary_dock1_e_png_end");

screen_t screen_pointclick_dock1(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int cursor[nb_state]
);
screen_t screen_pointclick_dock2(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int cursor[nb_state]
);
screen_t screen_pointclick_land1(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int cursor[nb_state]
);
screen_t screen_pointclick_dune1(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int cursor[nb_state]
);
screen_t screen_pointclick_dune2(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int cursor[nb_state]
);
screen_t screen_pointclick_town1(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int main_cursor[nb_state]
);
screen_t screen_pointclick_town2(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int main_cursor[nb_state]
);
screen_t screen_pointclick_road1(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int main_cursor[nb_state]
);
screen_t screen_pointclick_dune3(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int main_cursor[nb_state]
);
screen_t screen_pointclick_lighthouse1(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int main_cursor[nb_state]
);
screen_t screen_pointclick_lighthouse2(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int main_cursor[nb_state]
);

screen_t screen_postoffice_closeup_d(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int main_cursor[nb_state]
) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, false, false, true, false, false);
    int cursor      = main_cursor[0];
    int displayflag = 1;
    int nbdirection = 14;
    ESP_LOGE(TAG, "dialogue post office");
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    while (1) {
        if (displayflag) {
            pax_background(gfx, WHITE);
            switch (cursor) {
                case 0:
                    {
                        pax_insert_png_buf(
                            gfx,
                            postoffice_closeup_d1_S,
                            postoffice_closeup_d1_E - postoffice_closeup_d1_S,
                            0,
                            0,
                            0
                        );
                        break;
                    }
                case 1:
                    {
                        pax_insert_png_buf(
                            gfx,
                            postoffice_closeup_d2_S,
                            postoffice_closeup_d2_E - postoffice_closeup_d2_S,
                            0,
                            0,
                            0
                        );
                        break;
                    }
                case 2:
                    {
                        pax_insert_png_buf(
                            gfx,
                            postoffice_closeup_d3_S,
                            postoffice_closeup_d3_E - postoffice_closeup_d3_S,
                            0,
                            0,
                            0
                        );
                        break;
                    }
                case 3:
                    {
                        pax_insert_png_buf(
                            gfx,
                            postoffice_closeup_d4_S,
                            postoffice_closeup_d4_E - postoffice_closeup_d4_S,
                            0,
                            0,
                            0
                        );
                        break;
                    }
                case 4:
                    {
                        pax_insert_png_buf(
                            gfx,
                            postoffice_closeup_d5_S,
                            postoffice_closeup_d5_E - postoffice_closeup_d5_S,
                            0,
                            0,
                            0
                        );
                        break;
                    }
                case 5:
                    {
                        pax_insert_png_buf(
                            gfx,
                            postoffice_closeup_d6_S,
                            postoffice_closeup_d6_E - postoffice_closeup_d6_S,
                            0,
                            0,
                            0
                        );
                        break;
                    }
                case 6:
                    {
                        pax_insert_png_buf(
                            gfx,
                            postoffice_closeup_d7_S,
                            postoffice_closeup_d7_E - postoffice_closeup_d7_S,
                            0,
                            0,
                            0
                        );
                        break;
                    }
                case 7:
                    {
                        pax_insert_png_buf(
                            gfx,
                            postoffice_closeup_d8_S,
                            postoffice_closeup_d8_E - postoffice_closeup_d8_S,
                            0,
                            0,
                            0
                        );
                        break;
                    }
                case 8:
                    {
                        pax_insert_png_buf(
                            gfx,
                            postoffice_closeup_d9_S,
                            postoffice_closeup_d9_E - postoffice_closeup_d9_S,
                            0,
                            0,
                            0
                        );
                        break;
                    }
                case 9:
                    {
                        pax_insert_png_buf(
                            gfx,
                            postoffice_closeup_d10_S,
                            postoffice_closeup_d10_E - postoffice_closeup_d10_S,
                            0,
                            0,
                            0
                        );
                        break;
                    }
                case 10:
                    {
                        pax_insert_png_buf(
                            gfx,
                            postoffice_closeup_d11_S,
                            postoffice_closeup_d11_E - postoffice_closeup_d11_S,
                            0,
                            0,
                            0
                        );
                        break;
                    }
                case 11:
                    {
                        pax_insert_png_buf(
                            gfx,
                            postoffice_closeup_d12_S,
                            postoffice_closeup_d12_E - postoffice_closeup_d12_S,
                            0,
                            0,
                            0
                        );
                        break;
                    }
                case 12:
                    {
                        pax_insert_png_buf(
                            gfx,
                            postoffice_closeup_d13_S,
                            postoffice_closeup_d13_E - postoffice_closeup_d13_S,
                            0,
                            0,
                            0
                        );
                        break;
                    }
                case 13:
                    {
                        main_cursor[0] = screen_PC_e;
                        return screen_PC_town1;
                        break;
                    }
                default: break;
            }
            bsp_display_flush();
            displayflag = 0;
        }
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1: break;
                        case SWITCH_2: break;
                        case SWITCH_3: cursor++; break;
                        case SWITCH_4: cursor--; break;
                        case SWITCH_5: break;
                        default: break;
                    }
                    if (cursor < 0)
                        cursor = nbdirection - 1;
                    if (cursor > (nbdirection - 1))
                        cursor = 0;
                    displayflag = 1;
                    ESP_LOGE(TAG, "cursor %d", cursor);
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

screen_t screen_shop_closeup_d(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int main_cursor[nb_state]
) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, false, false, true, false, false);
    int cursor      = main_cursor[0];
    int displayflag = 1;
    int nbdirection = 5;
    ESP_LOGE(TAG, "dialogue post office");
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    while (1) {
        if (displayflag) {
            pax_background(gfx, WHITE);
            switch (cursor) {
                case 0:
                    {
                        pax_insert_png_buf(gfx, shop_closeup_d1_S, shop_closeup_d1_E - shop_closeup_d1_S, 0, 0, 0);
                        break;
                    }
                case 1:
                    {
                        pax_insert_png_buf(gfx, shop_closeup_d2_S, shop_closeup_d2_E - shop_closeup_d2_S, 0, 0, 0);
                        break;
                    }
                case 2:
                    {
                        pax_insert_png_buf(gfx, shop_closeup_d3_S, shop_closeup_d3_E - shop_closeup_d3_S, 0, 0, 0);
                        break;
                    }
                case 3:
                    {
                        pax_insert_png_buf(gfx, shop_closeup_d4_S, shop_closeup_d4_E - shop_closeup_d4_S, 0, 0, 0);
                        break;
                    }
                case 4:
                    {
                        main_cursor[0] = screen_PC_w;
                        return screen_PC_town1;
                        break;
                    }
                default: break;
            }
            bsp_display_flush();
            displayflag = 0;
        }
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1: break;
                        case SWITCH_2: break;
                        case SWITCH_3: cursor++; break;
                        case SWITCH_4: cursor--; break;
                        case SWITCH_5: break;
                        default: break;
                    }
                    if (cursor < 0)
                        cursor = nbdirection - 1;
                    if (cursor > (nbdirection - 1))
                        cursor = 0;
                    displayflag = 1;
                    ESP_LOGE(TAG, "cursor %d", cursor);
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

screen_t screen_shop_closeup_da(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int main_cursor[nb_state]
) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, false, false, true, false, false);
    int cursor      = main_cursor[0];
    int displayflag = 1;
    int nbdirection = 16;
    ESP_LOGE(TAG, "dialogue post office");
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    while (1) {
        if (displayflag) {
            pax_background(gfx, WHITE);
            switch (cursor) {
                case 0:
                    {
                        pax_insert_png_buf(gfx, shop_closeup_d1a_S, shop_closeup_d1a_E - shop_closeup_d1a_S, 0, 0, 0);
                        break;
                    }
                case 1:
                    {
                        pax_insert_png_buf(gfx, shop_closeup_d2a_S, shop_closeup_d2a_E - shop_closeup_d2a_S, 0, 0, 0);

                        break;
                    }
                case 2:
                    {
                        pax_insert_png_buf(gfx, shop_closeup_d3a_S, shop_closeup_d3a_E - shop_closeup_d3a_S, 0, 0, 0);

                        break;
                    }
                case 3:
                    {
                        pax_insert_png_buf(gfx, shop_closeup_d4a_S, shop_closeup_d4a_E - shop_closeup_d4a_S, 0, 0, 0);
                        break;
                    }
                case 4:
                    {
                        pax_insert_png_buf(gfx, shop_closeup_d5a_S, shop_closeup_d5a_E - shop_closeup_d5a_S, 0, 0, 0);
                        break;
                    }
                case 5:
                    {
                        pax_insert_png_buf(gfx, shop_closeup_d6a_S, shop_closeup_d6a_E - shop_closeup_d6a_S, 0, 0, 0);
                        break;
                    }
                case 6:
                    {
                        pax_insert_png_buf(gfx, shop_closeup_d7a_S, shop_closeup_d7a_E - shop_closeup_d7a_S, 0, 0, 0);
                        break;
                    }
                case 7:
                    {
                        pax_insert_png_buf(gfx, shop_closeup_d8a_S, shop_closeup_d8a_E - shop_closeup_d8a_S, 0, 0, 0);
                        break;
                    }
                case 8:
                    {
                        pax_insert_png_buf(gfx, shop_closeup_d9a_S, shop_closeup_d9a_E - shop_closeup_d9a_S, 0, 0, 0);
                        break;
                    }
                case 9:
                    {
                        pax_insert_png_buf(
                            gfx,
                            shop_closeup_d10a_S,
                            shop_closeup_d10a_E - shop_closeup_d10a_S,
                            0,
                            0,
                            0
                        );
                        break;
                    }
                case 10:
                    {
                        pax_insert_png_buf(
                            gfx,
                            shop_closeup_d11a_S,
                            shop_closeup_d11a_E - shop_closeup_d11a_S,
                            0,
                            0,
                            0
                        );
                        break;
                    }
                case 11:
                    {
                        pax_insert_png_buf(
                            gfx,
                            shop_closeup_d12a_S,
                            shop_closeup_d12a_E - shop_closeup_d12a_S,
                            0,
                            0,
                            0
                        );
                        break;
                    }
                case 12:
                    {
                        pax_insert_png_buf(
                            gfx,
                            shop_closeup_d13a_S,
                            shop_closeup_d13a_E - shop_closeup_d13a_S,
                            0,
                            0,
                            0
                        );
                        break;
                    }
                case 13:
                    {
                        pax_insert_png_buf(
                            gfx,
                            shop_closeup_d14a_S,
                            shop_closeup_d14a_E - shop_closeup_d14a_S,
                            0,
                            0,
                            0
                        );
                        break;
                    }
                case 14:
                    {
                        pax_insert_png_buf(
                            gfx,
                            shop_closeup_d15a_S,
                            shop_closeup_d15a_E - shop_closeup_d15a_S,
                            0,
                            0,
                            0
                        );
                        break;
                    }
                case 15:
                    {
                        main_cursor[0] = screen_PC_e;  // return to shop_closeup.png
                        return screen_PC_town1;
                        break;
                    }
                default: break;
            }
            bsp_display_flush();
            displayflag = 0;
        }
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1: break;
                        case SWITCH_2: break;
                        case SWITCH_3: cursor++; break;
                        case SWITCH_4: cursor--; break;
                        case SWITCH_5: break;
                        default: break;
                    }
                    if (cursor < 0)
                        cursor = nbdirection - 1;
                    if (cursor > (nbdirection - 1))
                        cursor = 0;
                    displayflag = 1;
                    ESP_LOGE(TAG, "cursor %d", cursor);
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

screen_t screen_road1_e_jorge_d(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int main_cursor[nb_state]
) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, false, false, true, false, false);
    int cursor      = main_cursor[0];
    int displayflag = 1;
    int nbdirection = 8;
    ESP_LOGE(TAG, "dialogue post office");
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    while (1) {
        if (displayflag) {
            pax_background(gfx, WHITE);
            switch (cursor) {
                case 0:
                    {
                        pax_insert_png_buf(gfx, road1_e_jorge_d1_S, road1_e_jorge_d1_E - road1_e_jorge_d1_S, 0, 0, 0);
                        break;
                    }
                case 1:
                    {
                        pax_insert_png_buf(gfx, road1_e_jorge_d2_S, road1_e_jorge_d2_E - road1_e_jorge_d2_S, 0, 0, 0);
                        break;
                    }
                case 2:
                    {
                        pax_insert_png_buf(gfx, road1_e_jorge_d3_S, road1_e_jorge_d3_E - road1_e_jorge_d3_S, 0, 0, 0);
                        break;
                    }
                case 3:
                    {
                        pax_insert_png_buf(gfx, road1_e_jorge_d4_S, road1_e_jorge_d4_E - road1_e_jorge_d4_S, 0, 0, 0);
                        break;
                    }
                case 4:
                    {
                        pax_insert_png_buf(gfx, road1_e_jorge_d5_S, road1_e_jorge_d5_E - road1_e_jorge_d5_S, 0, 0, 0);
                        break;
                    }
                case 5:
                    {
                        pax_insert_png_buf(gfx, road1_e_jorge_d6_S, road1_e_jorge_d6_E - road1_e_jorge_d6_S, 0, 0, 0);
                        break;
                    }
                case 6:
                    {
                        pax_insert_png_buf(gfx, road1_e_jorge_d7_S, road1_e_jorge_d7_E - road1_e_jorge_d7_S, 0, 0, 0);
                        break;
                    }
                case 7:
                    {
                        main_cursor[0] = screen_PC_e;  // tbd
                        return screen_PC_town1;        // tbd
                        break;
                    }
                default: break;
            }
            bsp_display_flush();
            displayflag = 0;
        }
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1: break;
                        case SWITCH_2: break;
                        case SWITCH_3: cursor++; break;
                        case SWITCH_4: cursor--; break;
                        case SWITCH_5: break;
                        default: break;
                    }
                    if (cursor < 0)
                        cursor = nbdirection - 1;
                    if (cursor > (nbdirection - 1))
                        cursor = 0;
                    displayflag = 1;
                    ESP_LOGE(TAG, "cursor %d", cursor);
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

screen_t screen_dock1_w_d(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int main_cursor[nb_state]
) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, false, false, true, false, false);
    int cursor      = main_cursor[0];
    int displayflag = 1;
    int nbdirection = 3;
    ESP_LOGE(TAG, "dialogue post office");
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    while (1) {
        if (displayflag) {
            pax_background(gfx, WHITE);
            switch (cursor) {
                case 0:
                    {
                        configure_keyboard_presses(keyboard_event_queue, false, false, true, false, false);
                        pax_insert_png_buf(gfx, dock1_w_d1_S, dock1_w_d1_E - dock1_w_d1_S, 0, 0, 0);
                        break;
                    }
                case 1:
                    {
                        configure_keyboard_presses(keyboard_event_queue, false, false, true, true, false);
                        pax_insert_png_buf(gfx, dock1_w_d2_S, dock1_w_d2_E - dock1_w_d2_S, 0, 0, 0);
                        break;
                    }
                case 2:
                    {
                        main_cursor[0]                   = screen_PC_w;
                        main_cursor[state_tutorial_dock] = tutorial_complete;
                        return screen_PC_dock2;
                        break;
                    }
                default: break;
            }
            bsp_display_flush();
            displayflag = 0;
        }
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1: break;
                        case SWITCH_2: break;
                        case SWITCH_3: cursor++; break;
                        case SWITCH_4: cursor--; break;
                        case SWITCH_5: break;
                        default: break;
                    }
                    if (cursor < 0)
                        cursor = nbdirection - 1;
                    if (cursor > (nbdirection - 1))
                        cursor = 0;
                    displayflag = 1;
                    ESP_LOGE(TAG, "cursor %d", cursor);
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}
screen_t screen_dune2_n_d(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int main_cursor[nb_state]
) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, false, false, true, false, false);
    int cursor      = main_cursor[0];
    int displayflag = 1;
    int nbdirection = 2;
    ESP_LOGE(TAG, "dialogue post office");
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    while (1) {
        if (displayflag) {
            pax_background(gfx, WHITE);
            switch (cursor) {
                case 0:
                    {
                        pax_insert_png_buf(gfx, dune2_n_d1_S, dune2_n_d1_E - dune2_n_d1_S, 0, 0, 0);
                        break;
                    }
                case 1:
                    {
                        main_cursor[state_tutorial_dune] = tutorial_complete;
                        main_cursor[0]                   = screen_PC_n;
                        return screen_PC_town1;
                        break;
                    }
                default: break;
            }
            bsp_display_flush();
            displayflag = 0;
        }
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1: break;
                        case SWITCH_2: break;
                        case SWITCH_3: cursor++; break;
                        case SWITCH_4: cursor--; break;
                        case SWITCH_5: break;
                        default: break;
                    }
                    if (cursor < 0)
                        cursor = nbdirection - 1;
                    if (cursor > (nbdirection - 1))
                        cursor = 0;
                    displayflag = 1;
                    ESP_LOGE(TAG, "cursor %d", cursor);
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}
screen_t screen_dune3_n_gc_d(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int main_cursor[nb_state]
) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, false, false, true, false, false);
    int cursor      = main_cursor[0];
    int displayflag = 1;
    int nbdirection = 4;
    ESP_LOGE(TAG, "dialogue post office");
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    while (1) {
        if (displayflag) {
            pax_background(gfx, WHITE);
            switch (cursor) {
                case 0:
                    {
                        pax_insert_png_buf(gfx, dune3_n_gc_d1_S, dune3_n_gc_d1_E - dune3_n_gc_d1_S, 0, 0, 0);
                        break;
                    }
                case 1:
                    {
                        pax_insert_png_buf(gfx, dune3_n_gc_d2_S, dune3_n_gc_d2_E - dune3_n_gc_d2_S, 0, 0, 0);
                        break;
                    }
                case 2:
                    {
                        pax_insert_png_buf(gfx, dune3_n_gc_d3_S, dune3_n_gc_d3_E - dune3_n_gc_d3_S, 0, 0, 0);
                        break;
                    }
                case 3:
                    {
                        pax_insert_png_buf(gfx, dune3_n_gc_d4_S, dune3_n_gc_d4_E - dune3_n_gc_d4_S, 0, 0, 0);
                        break;
                    }
                case 4:
                    {
                        main_cursor[0] = screen_PC_n2;
                        return screen_PC_lighthouse1;
                        break;
                    }
                default: break;
            }
            bsp_display_flush();
            displayflag = 0;
        }
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1: break;
                        case SWITCH_2: break;
                        case SWITCH_3: cursor++; break;
                        case SWITCH_4: cursor--; break;
                        case SWITCH_5: break;
                        default: break;
                    }
                    if (cursor < 0)
                        cursor = nbdirection - 1;
                    if (cursor > (nbdirection - 1))
                        cursor = 0;
                    displayflag = 1;
                    ESP_LOGE(TAG, "cursor %d", cursor);
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

screen_t screen_pointclick_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    screen_t current_screen_PC = screen_PC_dock1;
    int      main_cursor[nb_state];
    for (int i = 0; i < nb_state; i++) {
        main_cursor[i] = 0;
    }
    main_cursor[state_cursor]         = screen_PC_s;
    main_cursor[state_locklighthouse] = 0;

    bsp_apply_lut(lut_4s);
    while (1) {
        switch (current_screen_PC) {
            case screen_PC_dock1:
                {
                    current_screen_PC =
                        screen_pointclick_dock1(application_event_queue, keyboard_event_queue, main_cursor);
                    break;
                }
            case screen_PC_dock2:
                {
                    current_screen_PC =
                        screen_pointclick_dock2(application_event_queue, keyboard_event_queue, main_cursor);
                    break;
                }
            case screen_PC_land1:
                {
                    current_screen_PC =
                        screen_pointclick_land1(application_event_queue, keyboard_event_queue, main_cursor);
                    break;
                }
            case screen_PC_dune1:
                {
                    current_screen_PC =
                        screen_pointclick_dune1(application_event_queue, keyboard_event_queue, main_cursor);
                    break;
                }
            case screen_PC_dune2:
                {
                    current_screen_PC =
                        screen_pointclick_dune2(application_event_queue, keyboard_event_queue, main_cursor);
                    break;
                }
            case screen_PC_town1:
                {
                    current_screen_PC =
                        screen_pointclick_town1(application_event_queue, keyboard_event_queue, main_cursor);
                    break;
                }
            case screen_PC_town2:
                {
                    current_screen_PC =
                        screen_pointclick_town2(application_event_queue, keyboard_event_queue, main_cursor);
                    break;
                }
            case screen_PC_road1:
                {
                    current_screen_PC =
                        screen_pointclick_road1(application_event_queue, keyboard_event_queue, main_cursor);
                    break;
                }
            case screen_PC_dune3:
                {
                    current_screen_PC =
                        screen_pointclick_dune3(application_event_queue, keyboard_event_queue, main_cursor);
                    break;
                }
            case screen_PC_lighthouse1:
                {
                    current_screen_PC =
                        screen_pointclick_lighthouse1(application_event_queue, keyboard_event_queue, main_cursor);
                    break;
                }
            case screen_PC_lighthouse2:
                {
                    current_screen_PC =
                        screen_pointclick_lighthouse2(application_event_queue, keyboard_event_queue, main_cursor);
                    break;
                }
            case screen_PC_postoffice_closeup_d:
                {
                    current_screen_PC =
                        screen_postoffice_closeup_d(application_event_queue, keyboard_event_queue, main_cursor);
                    break;
                }
            case screen_PC_shop_closeup_d:
                {
                    current_screen_PC =
                        screen_shop_closeup_d(application_event_queue, keyboard_event_queue, main_cursor);
                    break;
                }
            case screen_PC_shop_closeup_da:
                {
                    current_screen_PC =
                        screen_shop_closeup_da(application_event_queue, keyboard_event_queue, main_cursor);
                    break;
                }
            case screen_PC_road1_e_jorge_d:
                {
                    current_screen_PC =
                        screen_road1_e_jorge_d(application_event_queue, keyboard_event_queue, main_cursor);
                    break;
                }
            case screen_PC_dock1_w_d:
                {
                    current_screen_PC = screen_dock1_w_d(application_event_queue, keyboard_event_queue, main_cursor);
                    break;
                }
            case screen_PC_dune2_n_d:
                {
                    current_screen_PC = screen_dune2_n_d(application_event_queue, keyboard_event_queue, main_cursor);
                    break;
                }
            case screen_PC_dune3_n_gc_d:
                {
                    current_screen_PC = screen_dune3_n_gc_d(application_event_queue, keyboard_event_queue, main_cursor);
                    break;
                }
            case screen_home:
            default: return screen_home; break;
        }
    }
}

screen_t screen_pointclick_dock1(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int main_cursor[nb_state]
) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_1, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_5, true);
    int cursor       = main_cursor[0];
    int displayflag  = 1;
    int nbdirection  = 4;
    int move_forward = 0;
    ESP_LOGE(TAG, "dock 1");
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    while (1) {
        if (displayflag) {
            pax_background(gfx, WHITE);
            switch (cursor) {
                case screen_PC_n:
                    {
                        pax_insert_png_buf(gfx, dock1_n_S, dock1_n_E - dock1_n_S, 0, 0, 0);
                        break;
                    }
                case screen_PC_e:
                    {
                        pax_insert_png_buf(gfx, dock1_e_S, dock1_e_E - dock1_e_S, 0, 0, 0);
                        break;
                    }
                case screen_PC_s:
                    {
                        pax_insert_png_buf(gfx, dock1_s_S, dock1_s_E - dock1_s_S, 0, 0, 0);
                        break;
                    }
                case screen_PC_w:
                    {
                        if (move_forward) {
                            if (main_cursor[state_tutorial_dock] == tutorial_uncomplete) {
                                main_cursor[0] = 0;
                                return screen_PC_dock1_w_d;
                            }
                            main_cursor[0] = screen_PC_w;
                            return screen_PC_dock2;
                        }
                        pax_insert_png_buf(gfx, dock1_w_S, dock1_w_E - dock1_w_S, 0, 0, 0);
                        break;
                    }
                default: break;
            }
            bsp_display_flush();
            displayflag = 0;
        }
        move_forward  = 0;
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_L1: cursor--; break;
                        case SWITCH_R1: cursor++; break;
                        case SWITCH_L5: cursor--; break;
                        case SWITCH_R5: cursor++; break;
                        case SWITCH_1: return screen_home; break;
                        case SWITCH_2: break;
                        case SWITCH_3: move_forward++; break;
                        case SWITCH_4: break;
                        case SWITCH_5: break;
                        default: break;
                    }
                    if (cursor < 0)
                        cursor = nbdirection - 1;
                    if (cursor > (nbdirection - 1))
                        cursor = 0;
                    displayflag = 1;
                    ESP_LOGE(TAG, "cursor %d", cursor);
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

screen_t screen_pointclick_dock2(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int main_cursor[nb_state]
) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_1, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_5, true);
    int cursor       = main_cursor[0];
    int displayflag  = 1;
    int nbdirection  = 4;
    int move_forward = 0;
    ESP_LOGE(TAG, "dock 2");
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    while (1) {
        if (displayflag) {
            pax_background(gfx, WHITE);
            switch (cursor) {
                case screen_PC_n:
                    {
                        pax_insert_png_buf(gfx, dock2_n_S, dock2_n_E - dock2_n_S, 0, 0, 0);
                        break;
                    }
                case screen_PC_e:
                    {
                        if (move_forward) {
                            main_cursor[0] = screen_PC_e;
                            return screen_PC_dock1;
                        }
                        pax_insert_png_buf(gfx, dock2_e_S, dock2_e_E - dock2_e_S, 0, 0, 0);
                        break;
                    }
                case screen_PC_s:
                    {
                        pax_insert_png_buf(gfx, dock2_s_S, dock2_s_E - dock2_s_S, 0, 0, 0);
                        break;
                    }
                case screen_PC_w:
                    {
                        if (move_forward) {
                            main_cursor[0] = screen_PC_w;
                            return screen_PC_land1;
                        }
                        pax_insert_png_buf(gfx, dock2_w_S, dock2_w_E - dock2_w_S, 0, 0, 0);
                        break;
                    }
                default: break;
            }
            bsp_display_flush();
            displayflag = 0;
        }
        move_forward  = 0;
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_L1: cursor--; break;
                        case SWITCH_R1: cursor++; break;
                        case SWITCH_L5: cursor--; break;
                        case SWITCH_R5: cursor++; break;
                        case SWITCH_1: return screen_home; break;
                        case SWITCH_2: break;
                        case SWITCH_3: move_forward++; break;
                        case SWITCH_4: break;
                        case SWITCH_5: break;
                        default: break;
                    }
                    ESP_LOGE(TAG, "cursor %d", cursor);
                    if (cursor < 0)
                        cursor = nbdirection - 1;
                    if (cursor > (nbdirection - 1))
                        cursor = 0;
                    displayflag = 1;
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

screen_t screen_pointclick_land1(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int main_cursor[nb_state]
) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_1, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_5, true);
    int cursor       = main_cursor[0];
    int displayflag  = 1;
    int nbdirection  = 4;
    int move_forward = 0;
    ESP_LOGE(TAG, "land1");
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    while (1) {
        if (displayflag) {
            pax_background(gfx, WHITE);
            switch (cursor) {
                case screen_PC_n:
                    {
                        if (move_forward) {
                            main_cursor[0] = screen_PC_n2;
                            return screen_PC_dune1;
                        }
                        pax_insert_png_buf(gfx, land1_n_S, land1_n_E - land1_n_S, 0, 0, 0);
                        break;
                    }
                case screen_PC_e:
                    {
                        if (move_forward) {
                            main_cursor[0] = screen_PC_e;
                            return screen_PC_dock2;
                        }
                        pax_insert_png_buf(gfx, land1_e_S, land1_e_E - land1_e_S, 0, 0, 0);
                        break;
                    }
                case screen_PC_s:
                    {
                        pax_insert_png_buf(gfx, land1_s_S, land1_s_E - land1_s_S, 0, 0, 0);
                        break;
                    }
                case screen_PC_w:
                    {
                        pax_insert_png_buf(gfx, land1_w_S, land1_w_E - land1_w_S, 0, 0, 0);
                        break;
                    }
                default: break;
            }
            bsp_display_flush();
            displayflag = 0;
        }
        move_forward  = 0;
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_L1: cursor--; break;
                        case SWITCH_R1: cursor++; break;
                        case SWITCH_L5: cursor--; break;
                        case SWITCH_R5: cursor++; break;
                        case SWITCH_1: return screen_home; break;
                        case SWITCH_2: break;
                        case SWITCH_3: move_forward++; break;
                        case SWITCH_4: break;
                        case SWITCH_5: break;
                        default: break;
                    }
                    if (cursor < 0)
                        cursor = nbdirection - 1;
                    if (cursor > (nbdirection - 1))
                        cursor = 0;
                    displayflag = 1;
                    ESP_LOGE(TAG, "cursor %d", cursor);
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

screen_t screen_pointclick_dune1(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int main_cursor[nb_state]
) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_1, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_5, true);
    int cursor       = main_cursor[0];
    int displayflag  = 1;
    int nbdirection  = 2;
    int move_forward = 0;
    int move_back    = 0;
    ESP_LOGE(TAG, "dune 1");
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    while (1) {
        if (displayflag) {
            pax_background(gfx, WHITE);
            switch (cursor) {
                case screen_PC_n2:  // North
                    {
                        if (move_forward) {
                            main_cursor[0] = screen_PC_e;
                            return screen_PC_land1;
                        }
                        pax_insert_png_buf(gfx, dune1_n_S, dune1_n_E - dune1_n_S, 0, 0, 0);
                        break;
                    }
                case screen_PC_s2:  // South
                    {
                        if (move_forward) {
                            main_cursor[0] = screen_PC_n2;
                            return screen_PC_dune2;
                        }
                        pax_insert_png_buf(gfx, dune1_s_S, dune1_s_E - dune1_s_S, 0, 0, 0);
                        break;
                    }
                default: break;
            }
            bsp_display_flush();
            displayflag = 0;
        }
        move_forward  = 0;
        move_back     = 0;
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_L1: cursor--; break;
                        case SWITCH_R1: cursor++; break;
                        case SWITCH_L5: cursor--; break;
                        case SWITCH_R5: cursor++; break;
                        case SWITCH_1: return screen_home; break;
                        case SWITCH_2: move_back++; break;
                        case SWITCH_3: move_forward++; break;
                        case SWITCH_4: break;
                        case SWITCH_5: break;
                        default: break;
                    }
                    ESP_LOGE(TAG, "cursor %d", cursor);
                    if (cursor < 0)
                        cursor = nbdirection - 1;
                    if (cursor > (nbdirection - 1))
                        cursor = 0;
                    displayflag = 1;
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

screen_t screen_pointclick_dune2(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int main_cursor[nb_state]
) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_1, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_5, true);
    int cursor       = main_cursor[0];
    int displayflag  = 1;
    int nbdirection  = 2;
    int move_forward = 0;
    int move_back    = 0;
    ESP_LOGE(TAG, "dune 2");
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    while (1) {
        if (displayflag) {
            pax_background(gfx, WHITE);
            switch (cursor) {
                case screen_PC_n2:  // North
                    {
                        if (move_forward) {
                            if (main_cursor[state_tutorial_dune] == tutorial_uncomplete) {
                                main_cursor[0] = 0;
                                return screen_PC_dune2_n_d;
                            }
                            main_cursor[0] = screen_PC_n;
                            return screen_PC_town1;
                        }
                        pax_insert_png_buf(gfx, dune2_n_S, dune2_n_E - dune2_n_S, 0, 0, 0);
                        break;
                    }
                case screen_PC_s2:  // South
                    {
                        if (move_forward) {
                            main_cursor[0] = screen_PC_s2;
                            return screen_PC_dune1;
                        }
                        pax_insert_png_buf(gfx, dune2_s_S, dune2_s_E - dune2_s_S, 0, 0, 0);
                        break;
                    }
                default: break;
            }
            bsp_display_flush();
            displayflag = 0;
        }
        move_forward  = 0;
        move_back     = 0;
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_L1: cursor--; break;
                        case SWITCH_R1: cursor++; break;
                        case SWITCH_L5: cursor--; break;
                        case SWITCH_R5: cursor++; break;
                        case SWITCH_1: return screen_home; break;
                        case SWITCH_2: move_back++; break;
                        case SWITCH_3: move_forward++; break;
                        case SWITCH_4: break;
                        case SWITCH_5: break;
                        default: break;
                    }
                    if (cursor < 0)
                        cursor = nbdirection - 1;
                    if (cursor > (nbdirection - 1))
                        cursor = 0;
                    displayflag = 1;
                    ESP_LOGE(TAG, "cursor %d", cursor);
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

screen_t screen_pointclick_town1(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int main_cursor[nb_state]
) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_1, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_5, true);
    int cursor       = main_cursor[0];
    int displayflag  = 1;
    int nbdirection  = 4;
    int move_forward = 0;
    int move_back    = 0;
    ESP_LOGE(TAG, "town1");
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    while (1) {
        if (displayflag) {
            pax_background(gfx, WHITE);
            switch (cursor) {
                case screen_PC_n:
                    {
                        pax_insert_png_buf(gfx, town1_n_S, town1_n_E - town1_n_S, 0, 0, 0);
                        break;
                    }
                case screen_PC_e:
                    {
                        if (move_back) {
                            main_cursor[0] = screen_PC_s2;
                            return screen_PC_dune2;
                        }
                        pax_insert_png_buf(gfx, town1_e_S, town1_e_E - town1_e_S, 0, 0, 0);
                        break;
                    }
                case screen_PC_s:
                    {
                        if (move_forward) {
                            main_cursor[0] = screen_PC_s2;
                            return screen_PC_dune2;
                        }
                        pax_insert_png_buf(gfx, town1_s_S, town1_s_E - town1_s_S, 0, 0, 0);
                        break;
                    }
                case screen_PC_w:
                    {
                        if (move_forward) {
                            main_cursor[0] = screen_PC_n2;
                            return screen_PC_town2;
                        }
                        pax_insert_png_buf(gfx, town1_w_S, town1_w_E - town1_w_S, 0, 0, 0);
                        break;
                    }
                default: break;
            }
            bsp_display_flush();
            displayflag = 0;
        }
        move_forward  = 0;
        move_back     = 0;
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_L1: cursor--; break;
                        case SWITCH_R1: cursor++; break;
                        case SWITCH_L5: cursor--; break;
                        case SWITCH_R5: cursor++; break;
                        case SWITCH_1: return screen_home; break;
                        case SWITCH_2: move_back++; break;
                        case SWITCH_3: move_forward++; break;
                        case SWITCH_4: break;
                        case SWITCH_5: break;
                        default: break;
                    }
                    if (cursor < 0)
                        cursor = nbdirection - 1;
                    if (cursor > (nbdirection - 1))
                        cursor = 0;
                    displayflag = 1;
                    ESP_LOGE(TAG, "cursor %d", cursor);
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

screen_t screen_pointclick_town2(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int main_cursor[nb_state]
) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_1, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_5, true);
    int cursor       = main_cursor[0];
    int displayflag  = 1;
    int nbdirection  = 2;
    int move_forward = 0;
    int move_back    = 0;
    ESP_LOGE(TAG, "town2");
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    while (1) {
        if (displayflag) {
            pax_background(gfx, WHITE);
            switch (cursor) {
                case screen_PC_n2:  // North
                    {
                        if (move_forward) {
                            main_cursor[0] = screen_PC_road1_w;
                            return screen_PC_road1;
                        }
                        pax_insert_png_buf(gfx, town2_n_S, town2_n_E - town2_n_S, 0, 0, 0);
                        break;
                    }
                case screen_PC_s2:  // South
                    {
                        if (move_forward) {
                            main_cursor[0] = screen_PC_e;
                            return screen_PC_town1;
                        }
                        pax_insert_png_buf(gfx, town2_s_S, town2_s_E - town2_s_S, 0, 0, 0);
                        break;
                    }
                default: break;
            }
            bsp_display_flush();
            displayflag = 0;
        }
        move_forward  = 0;
        move_back     = 0;
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_L1: cursor--; break;
                        case SWITCH_R1: cursor++; break;
                        case SWITCH_L5: cursor--; break;
                        case SWITCH_R5: cursor++; break;
                        case SWITCH_1: return screen_home; break;
                        case SWITCH_2: move_back++; break;
                        case SWITCH_3: move_forward++; break;
                        case SWITCH_4: break;
                        case SWITCH_5: break;
                        default: break;
                    }
                    if (cursor < 0)
                        cursor = nbdirection - 1;
                    if (cursor > (nbdirection - 1))
                        cursor = 0;
                    displayflag = 1;
                    ESP_LOGE(TAG, "cursor %d", cursor);
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

screen_t screen_pointclick_road1(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int main_cursor[nb_state]
) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_1, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_5, true);
    int cursor       = main_cursor[0];
    int displayflag  = 1;
    int nbdirection  = 3;
    int move_forward = 0;
    int move_back    = 0;
    ESP_LOGE(TAG, "road1");
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    while (1) {
        if (displayflag) {
            pax_background(gfx, WHITE);
            switch (cursor) {
                case screen_PC_road1_n:  // North
                    {
                        if (move_forward) {
                            main_cursor[0] = screen_PC_dune3_gate;
                            return screen_PC_dune3;
                        }
                        pax_insert_png_buf(gfx, road1_n_S, road1_n_E - road1_n_S, 0, 0, 0);
                        break;
                    }
                case screen_PC_road1_e:  // East
                    {
                        if (move_forward) {
                            main_cursor[0] = screen_PC_s2;
                            return screen_PC_town2;
                        }
                        pax_insert_png_buf(gfx, road1_e_S, road1_e_E - road1_e_S, 0, 0, 0);
                        break;
                    }
                case screen_PC_road1_w:  // West
                    {
                        pax_insert_png_buf(gfx, road1_w_S, road1_w_E - road1_w_S, 0, 0, 0);
                        break;
                    }
                default: break;
            }
            bsp_display_flush();
            displayflag = 0;
        }
        move_forward  = 0;
        move_back     = 0;
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_L1: cursor--; break;
                        case SWITCH_R1: cursor++; break;
                        case SWITCH_L5: cursor--; break;
                        case SWITCH_R5: cursor++; break;
                        case SWITCH_1: return screen_home; break;
                        case SWITCH_2: move_back++; break;
                        case SWITCH_3: move_forward++; break;
                        case SWITCH_4: break;
                        case SWITCH_5: break;
                        default: break;
                    }
                    if (cursor < 0)
                        cursor = nbdirection - 1;
                    if (cursor > (nbdirection - 1))
                        cursor = 0;
                    displayflag = 1;
                    ESP_LOGE(TAG, "cursor %d", cursor);
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

screen_t screen_pointclick_dune3(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int main_cursor[nb_state]
) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_1, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_5, true);
    int cursor       = main_cursor[0];
    int displayflag  = 1;
    int nbdirection  = 3;
    int move_forward = 0;
    int move_back    = 0;
    ESP_LOGE(TAG, "dune3");
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    while (1) {
        if (displayflag) {
            pax_background(gfx, WHITE);
            switch (cursor) {
                case screen_PC_dune3_gate:  // East - but not really to sort - gate
                    {
                        if (move_forward) {
                            if (main_cursor[state_locklighthouse] == locklighthouse_locked) {
                                main_cursor[0] = 0;
                                return screen_PC_dune3_n_gc_d;
                            }
                            main_cursor[0] = screen_PC_n2;
                            return screen_PC_lighthouse1;
                        }
                        if (main_cursor[state_locklighthouse] == locklighthouse_locked) {
                            pax_insert_png_buf(gfx, dune3_n_gc_S, dune3_n_gc_E - dune3_n_gc_S, 0, 0, 0);
                        } else if (main_cursor[state_locklighthouse] == locklighthouse_unlocked) {
                            pax_insert_png_buf(gfx, dune3_n_go_S, dune3_n_go_E - dune3_n_go_S, 0, 0, 0);
                        }
                        break;
                    }
                case screen_PC_dune3_s:  // South
                    {
                        if (move_forward) {
                            main_cursor[0] = screen_PC_road1_e;
                            return screen_PC_road1;
                        }
                        pax_insert_png_buf(gfx, dune3_s_S, dune3_s_E - dune3_s_S, 0, 0, 0);

                        break;
                    }
                case screen_PC_dune3_up:  // West- but not really to sort- up
                    {
                        if (main_cursor[state_locklighthouse] == locklighthouse_locked)
                            pax_insert_png_buf(gfx, dune3_n_gc_up_S, dune3_n_gc_up_E - dune3_n_gc_up_S, 0, 0, 0);
                        else if (main_cursor[state_locklighthouse] == locklighthouse_unlocked)
                            pax_insert_png_buf(gfx, dune3_n_go_up_S, dune3_n_go_up_E - dune3_n_go_up_S, 0, 0, 0);
                        break;
                    }
                default: break;
            }
            bsp_display_flush();
            displayflag = 0;
        }
        move_forward  = 0;
        move_back     = 0;
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_L1: cursor--; break;
                        case SWITCH_R1: cursor++; break;
                        case SWITCH_L5: cursor--; break;
                        case SWITCH_R5: cursor++; break;
                        case SWITCH_1: return screen_home; break;
                        case SWITCH_2: move_back++; break;
                        case SWITCH_3: move_forward++; break;
                        case SWITCH_4: break;
                        case SWITCH_5: break;
                        default: break;
                    }
                    if (cursor < 0)
                        cursor = nbdirection - 1;
                    if (cursor > (nbdirection - 1))
                        cursor = 0;
                    displayflag = 1;
                    ESP_LOGE(TAG, "cursor %d", cursor);
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

screen_t screen_pointclick_lighthouse1(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int main_cursor[nb_state]
) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_1, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_5, true);
    int cursor       = main_cursor[0];
    int displayflag  = 1;
    int nbdirection  = 2;
    int move_forward = 0;
    int move_back    = 0;
    ESP_LOGE(TAG, "lighthouse1");
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    while (1) {
        if (displayflag) {
            pax_background(gfx, WHITE);
            switch (cursor) {
                case screen_PC_n2:  // North
                    {
                        if (main_cursor[state_locklighthouse] == locklighthouse_locked)
                            pax_insert_png_buf(
                                gfx,
                                lighthouse1_n_gc_S,
                                lighthouse1_n_gc_E - lighthouse1_n_gc_S,
                                0,
                                0,
                                0
                            );
                        else if (main_cursor[state_locklighthouse] == locklighthouse_unlocked) {
                            if (move_forward) {
                                main_cursor[0] = screen_PC_n;
                                return screen_PC_lighthouse2;
                            }
                            pax_insert_png_buf(
                                gfx,
                                lighthouse1_n_go_S,
                                lighthouse1_n_go_E - lighthouse1_n_go_S,
                                0,
                                0,
                                0
                            );
                        }
                        break;
                    }
                case screen_PC_s2:  // South
                    {
                        if (move_forward) {
                            main_cursor[0] = screen_PC_dune3_s;
                            return screen_PC_dune3;
                        }
                        pax_insert_png_buf(gfx, lighthouse1_s_S, lighthouse1_s_E - lighthouse1_s_S, 0, 0, 0);
                        break;
                    }
                default: break;
            }
            bsp_display_flush();
            displayflag = 0;
        }
        move_forward  = 0;
        move_back     = 0;
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_L1: cursor--; break;
                        case SWITCH_R1: cursor++; break;
                        case SWITCH_L5: cursor--; break;
                        case SWITCH_R5: cursor++; break;
                        case SWITCH_1: return screen_home; break;
                        case SWITCH_2: move_back++; break;
                        case SWITCH_3: move_forward++; break;
                        case SWITCH_4: break;
                        case SWITCH_5: break;
                        default: break;
                    }
                    if (cursor < 0)
                        cursor = nbdirection - 1;
                    if (cursor > (nbdirection - 1))
                        cursor = 0;
                    displayflag = 1;
                    ESP_LOGE(TAG, "cursor %d", cursor);
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

screen_t screen_pointclick_lighthouse2(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int main_cursor[nb_state]
) {
    InitKeyboard(keyboard_event_queue);
    configure_keyboard_presses(keyboard_event_queue, true, true, true, true, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_1, true);
    configure_keyboard_rotate_both(keyboard_event_queue, SWITCH_5, true);
    int cursor       = main_cursor[0];
    int displayflag  = 1;
    int nbdirection  = 4;
    int move_forward = 0;
    int move_back    = 0;
    ESP_LOGE(TAG, "lighthouse2");
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    while (1) {
        if (displayflag) {
            pax_background(gfx, WHITE);
            switch (cursor) {
                case screen_PC_n:
                    {
                        pax_insert_png_buf(gfx, lighthouse2_n_S, lighthouse2_n_E - lighthouse2_n_S, 0, 0, 0);
                        break;
                    }
                case screen_PC_e:
                    {
                        pax_insert_png_buf(gfx, lighthouse2_e_S, lighthouse2_e_E - lighthouse2_e_S, 0, 0, 0);
                        break;
                    }
                case screen_PC_s:
                    {
                        if (move_forward) {
                            main_cursor[0] = screen_PC_s2;
                            return screen_PC_lighthouse1;
                        }
                        pax_insert_png_buf(gfx, lighthouse2_s_S, lighthouse2_s_E - lighthouse2_s_S, 0, 0, 0);
                        break;
                    }
                case screen_PC_w:
                    {
                        pax_insert_png_buf(gfx, lighthouse2_w_S, lighthouse2_w_E - lighthouse2_w_S, 0, 0, 0);
                        break;
                    }
                default: break;
            }
            bsp_display_flush();
            displayflag = 0;
        }
        move_forward  = 0;
        move_back     = 0;
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_L1: cursor--; break;
                        case SWITCH_R1: cursor++; break;
                        case SWITCH_L5: cursor--; break;
                        case SWITCH_R5: cursor++; break;
                        case SWITCH_1: return screen_home; break;
                        case SWITCH_2: move_back++; break;
                        case SWITCH_3: move_forward++; break;
                        case SWITCH_4: break;
                        case SWITCH_5: break;
                        default: break;
                    }
                    if (cursor < 0)
                        cursor = nbdirection - 1;
                    if (cursor > (nbdirection - 1))
                        cursor = 0;
                    displayflag = 1;
                    ESP_LOGE(TAG, "cursor %d", cursor);
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}
