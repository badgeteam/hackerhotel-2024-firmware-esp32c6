#include "application.h"
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

#define nbmessages   9
#define sizenickname 64
#define sizemessages 64

static char const * TAG = "testscreen";

// char nicknamearray[nbmessages][sizenickname];
// char messagearray[nbmessages][sizemessages];
// int  messagecursor = 0;

// static esp_err_t nvs_get_str_wrapped(char const * namespace, char const * key, char* buffer, size_t buffer_size) {
//     nvs_handle_t handle;
//     esp_err_t    res = nvs_open(namespace, NVS_READWRITE, &handle);
//     if (res == ESP_OK) {
//         size_t size = 0;
//         res         = nvs_get_str(handle, key, NULL, &size);
//         if ((res == ESP_OK) && (size <= buffer_size - 1)) {
//             res = nvs_get_str(handle, key, buffer, &size);
//             if (res != ESP_OK) {
//                 buffer[0] = '\0';
//             }
//         }
//     } else {
//         buffer[0] = '\0';
//     }
//     nvs_close(handle);
//     return res;
// }

// void DisplayBillboard(int _addmessageflag, char* _nickname, char* _message) {
//     // set screen font and buffer
//     pax_font_t const * font = pax_font_sky;
//     pax_buf_t*         gfx  = bsp_get_gfx_buffer();

//     // set infrastructure
//     pax_background(gfx, WHITE);
//     pax_draw_text(gfx, BLACK, font, 18, 80, 0, "The billboard");
//     DisplaySwitchesBox(SWITCH_1);
//     pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 8, 116, "Exit");
//     DisplaySwitchesBox(SWITCH_5);
//     pax_draw_text(gfx, 1, pax_font_sky_mono, 10, 247, 116, "Send");

//     int messageoffsetx = 1;
//     int messageoffsety = 21;

//     int textboxoffestx = 1;
//     int textboxoffesty = 20;

//     int textboxheight = 10;
//     int textboxwidth  = gfx->height - textboxoffestx;

//     int fontsize = 9;

//     char billboardline[64];

//     // if the add message flag is raised
//     if (_addmessageflag) {
//         // clear board if full and reset cursor
//         if (messagecursor >= nbmessages) {
//             for (int i = 0; i < nbmessages; i++) {
//                 strcpy(nicknamearray[i], "");
//                 strcpy(messagearray[i], "");
//                 messagecursor = 0;
//             }
//         }
//         // add message to the board
//         strcpy(nicknamearray[messagecursor], _nickname);
//         strcpy(messagearray[messagecursor], _message);
//         messagecursor++;
//     }


//     for (int i = 0; i < nbmessages; i++) {
//         strcpy(billboardline, nicknamearray[i]);
//         if (strlen(nicknamearray[i]) != 0)
//             strcat(billboardline, ": ");
//         strcat(billboardline, messagearray[i]);

//         pax_draw_text(gfx, BLACK, font, fontsize, messageoffsetx, messageoffsety + textboxheight * i, billboardline);
//         pax_outline_rect(gfx, BLACK, textboxoffestx, textboxoffesty + textboxheight * i, textboxwidth,
//         textboxheight);
//     }
//     bsp_display_flush();
// }

// screen_t screen_test_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {

//     // update the keyboard event handler settings
//     event_t kbsettings = {
//         .type                                     = event_control_keyboard,
//         .args_control_keyboard.enable_typing      = false,
//         .args_control_keyboard.enable_actions     = {true, true, false, false, true},
//         .args_control_keyboard.enable_leds        = true,
//         .args_control_keyboard.enable_relay       = true,
//         kbsettings.args_control_keyboard.capslock = false,
//     };
//     xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);

//     // set screen font and buffer
//     // pax_font_t const * font = pax_font_sky;
//     // pax_buf_t*         gfx  = bsp_get_gfx_buffer();

//     // // memory
//     // nvs_handle_t handle;
//     // esp_err_t    res = nvs_open("owner", NVS_READWRITE, &handle);

//     // // read nickname from memory
//     // char   nickname[64] = {0};
//     // size_t size         = 0;
//     // res                 = nvs_get_str(handle, "nickname", NULL, &size);
//     // if ((res == ESP_OK) && (size <= sizeof(nickname) - 1)) {
//     //     res = nvs_get_str(handle, "nickname", nickname, &size);
//     //     if (res != ESP_OK || strlen(nickname) < 1) {
//     //         sprintf(nickname, "No nickname configured");
//     //     }
//     // }

//     // nvs_close(handle);

//     // // scale the nickame to fit the screen and display(I think)
//     // pax_vec1_t dims = {
//     //     .x = 999,
//     //     .y = 999,
//     // };
//     // float scale = 100;
//     // while (scale > 1) {
//     //     dims = pax_text_size(font, scale, nickname);
//     //     if (dims.x <= 296 && dims.y <= 100)
//     //         break;
//     //     scale -= 0.2;
//     // }
//     // ESP_LOGW(TAG, "Scale: %f", scale);
//     // pax_draw_text(gfx, BLACK, font, 18, 80, 50, "The cake is a lie The cake is a lie The cake is a lie");
//     //  pax_insert_png_buf(gfx, homescreen_png_start, homescreen_png_end - homescreen_png_start, 0, gfx->width - 24,
//     0);


//     // pax_draw_text(gfx, BLACK, font, fontsize, messageoffsetx, messageoffsety, billboardline);
//     // pax_draw_text(gfx, BLACK, font, fontsize, messageoffsetx, messageoffsety + textboxheight, nicknamearray[1]);
//     // pax_draw_text(gfx, BLACK, font, fontsize, messageoffsetx, messageoffsety + textboxheight * 2,
//     nicknamearray[2]);

//     // pax_outline_rect(gfx, BLACK, textboxoffestx, textboxoffesty + textboxheight * 0, textboxwidth, textboxheight);
//     // pax_outline_rect(gfx, BLACK, textboxoffestx, textboxoffesty + textboxheight * 1, textboxwidth, textboxheight);
//     // pax_outline_rect(gfx, BLACK, textboxoffestx, textboxoffesty + textboxheight * 2, textboxwidth, textboxheight);
//     for (int i = 0; i < nbmessages; i++) {
//         strcpy(nicknamearray[i], "");
//         strcpy(messagearray[i], "");
//     }
//     DisplayBillboard(0, "", "");
//     while (1) {
//         event_t event = {0};
//         if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
//             switch (event.type) {
//                 case event_input_button: break;  // Ignore raw button input
//                 case event_input_keyboard:
//                     switch (event.args_input_keyboard.action) {
//                         case SWITCH_1: return screen_home; break;

//                         case SWITCH_2:
//                             char dummynickname[3][sizenickname] = {"Zero Cool", "Acid Burn", "Cereal Killer"};
//                             char dummymessage[3][sizemessages]  = {
//                                 "Hack the Planet!",
//                                 "Mess with the best, die like the rest",
//                                 "1337"
//                             };
//                             int i = esp_random() % 3;
//                             DisplayBillboard(1, dummynickname[i], dummymessage[i]);

//                             break;
//                         case SWITCH_3: break;
//                         case SWITCH_4: break;
//                         case SWITCH_5:
//                             char playermessage[64];
//                             strcpy(playermessage, "Banana bread");
//                             // Using textedit here works until you press "ok", then crashes
//                             // textedit(
//                             //     "Type your message to broadcast:",
//                             //     application_event_queue,
//                             //     keyboard_event_queue,
//                             //     playermessage,
//                             //     sizeof(playermessage)
//                             // );
//                             char nickname[64] = "";
//                             nvs_get_str_wrapped("owner", "nickname", nickname, sizeof(nickname));
//                             DisplayBillboard(1, nickname, playermessage);
//                             break;
//                         default: break;
//                     }
//                     break;
//                 default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
//             }
//         }
//     }
// }
