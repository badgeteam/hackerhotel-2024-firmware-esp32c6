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

extern uint8_t const caronl_png_start[] asm("_binary_caronl_png_start");
extern uint8_t const caronl_png_end[] asm("_binary_caronl_png_end");
extern uint8_t const caronv_png_start[] asm("_binary_caronv_png_start");
extern uint8_t const caronv_png_end[] asm("_binary_caronv_png_end");
extern uint8_t const carond_png_start[] asm("_binary_carond_png_start");
extern uint8_t const carond_png_end[] asm("_binary_carond_png_end");

#define nbmessages   9
#define sizenickname 64
#define sizemessages 64

#define water         0
#define boat          1
#define missedshot    2
#define boathit       3
#define boatdestroyed 4

#define victory 1
#define defeat  2

#define player 0
#define ennemy 1

#define invalid -1

static char const * TAG = "testscreen";

int playerboard[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int ennemyboard[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int telegraph_X[20] = {0, -8, 8, -16, 0, 16, -24, -8, 8, 24, -24, -8, 8, 24, -16, 0, 16, -8, 8, 0};
int telegraph_Y[20] = {12, 27, 27, 42, 42, 42, 57, 57, 57, 57, 71, 71, 71, 71, 86, 86, 86, 101, 101, 116};
int playership[6]   = {-1, -1, -1, -1, -1, -1};
int ennemyship[6]   = {3, 2, 0, 16, 18, 19};
int _position[20];

screen_t screen_battleship_placeships(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);
screen_t screen_battleship_battle(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue);
screen_t screen_battleship_victory(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int _victoryflag
);

void Display_battleship_placeships(int _shipplaced, int _flagstart);
void Display_battleship_battle(char _nickname[64], char _ennemyname[64], int _turn);
void debugboardstatus(int board[20]) {
    ESP_LOGE(TAG, "      %d", board[0]);
    ESP_LOGE(TAG, "    %d - %d", board[1], board[2]);
    ESP_LOGE(TAG, "  %d - %d - %d", board[3], board[4], board[5]);
    ESP_LOGE(TAG, "%d - %d - %d - %d", board[6], board[7], board[8], board[9]);
    ESP_LOGE(TAG, "%d - %d - %d - %d", board[10], board[11], board[12], board[13]);
    ESP_LOGE(TAG, "  %d - %d - %d", board[14], board[15], board[16]);
    ESP_LOGE(TAG, "    %d - %d", board[17], board[18]);
    ESP_LOGE(TAG, "      %d", board[19]);
}
void debugshipstatus(int ships[6]) {
    ESP_LOGE(TAG, "Ship 1: %d", ships[0]);
    ESP_LOGE(TAG, "Ship 2: %d - %d", ships[1], ships[2]);
    ESP_LOGE(TAG, "Ship 3: %d - %d - %d", ships[3], ships[4], ships[5]);
}

// either return the possible blocks for the front of the ship, or the back
// _FB for which part of the boat to place, if front (first part) _FB = 1, back is 0
// _blockfrontship show the block off the front of the ship, only relevant if _FB = 0
void CheckforShipPlacement(int _ships[6], int _shipplaced) {

    int validlines[24][4] = {{0, 1, 3, 6},     {2, 4, 7, -1},    {5, 8, -1, -1},   {-1, -1, -1, -1}, {0, 2, 5, 9},
                             {1, 4, 8, -1},    {3, 7, -1, -1},   {-1, -1, -1, -1}, {-1, -1, -1, -1}, {1, 2, -1, -1},
                             {3, 4, 5, -1},    {6, 7, 8, 9},     {-1, -1, -1, -1}, {14, 11, -1, -1}, {17, 15, 12, -1},
                             {19, 18, 16, 13}, {19, 17, 14, 10}, {18, 15, 11, -1}, {16, 12, -1, -1}, {-1, -1, -1, -1},
                             {10, 11, 12, 13}, {14, 15, 16, -1}, {17, 18, -1, -1}, {-1, -1, -1, -1}};
    int shipbase;

    int _blockfrontship = 0;
    int _FB             = 0;
    int _shipsize       = 0;

    switch (_shipplaced) {
        case 0:
            _blockfrontship = 0;
            _FB             = 1;
            break;
        case 1:  // possible blocks for the front of the 2 long ship
            _blockfrontship = 0;
            _FB             = 1;
            _shipsize       = 2;
            break;
        case 2:  // possible blocks for the back of the 2 long ship
            _blockfrontship = _ships[1];
            _FB             = 0;
            _shipsize       = 2;
            break;
        case 3:  // possible blocks for the front of the 3 long ship
            _blockfrontship = 0;
            _FB             = 1;
            _shipsize       = 3;
            break;
        case 5:  // possible blocks for the back of the 2 long ship
            _blockfrontship = _ships[3];
            _FB             = 0;
            _shipsize       = 3;
            break;
        default: break;
    }

    // remove the front of the ship currently being placed from being counted as invalid
    // if (_shipsize == 2 && _FB == 1)
    //     _shipplaced[1] = 30;  // 2 long ship, front (first part)
    // if (_shipsize == 2 && _FB == 0)
    //     _shipplaced[1] = 30;  // 2 long ship, back (2nd part)
    // if (_shipsize == 3 && _FB == 1)
    //     _shipplaced[3] = 30;  // 3 long ship, front (first part)
    // if (_shipsize == 3 && _FB == 0)
    //     _ships[3] = 30;  // 3 long ship, back (2nd part)

    // remove placed ships from possible locations
    for (int k = 0; k < 6; k++) {
        // skips the front of the 2 long ship
        if (_shipplaced == 2 && k == 1)
            k++;
        // skips the front of the 3 long ship
        if (_shipplaced == 5 && k == 3)
            k++;

        for (int j = 0; j < 24; j++) {
            for (int i = 0; i < 4; i++) {
                if (validlines[j][i] == _ships[k])
                    validlines[j][i] = invalid;
            }
        }
    }

    int streakvalid = 0;

    // if placing the front of the boat (first part)
    if (_FB) {
        for (int j = 0; j < 24; j++) {
            streakvalid = 0;
            for (int i = 0; i < 4; i++) {
                if (validlines[j][i] != invalid) {
                    streakvalid++;
                    // if there is a valid ship position, set the first and last position of the streak as valid
                    if (streakvalid >= _shipsize) {
                        // ESP_LOGE(TAG, "validlines j: %d i: %d", j, i);
                        _position[validlines[j][i]]                 = missedshot;
                        _position[validlines[j][i + 1 - _shipsize]] = missedshot;
                    }
                } else
                    streakvalid = 0;
            }
        }
    }

    // if placing the back of the boat (second part)
    if (!_FB) {
        for (int j = 0; j < 24; j++) {
            for (int i = 0; i < 4; i++) {
                // if this is the block of the front of the ship
                if (validlines[j][i] == _blockfrontship) {
                    // check the block before
                    int blocktocheck = i + 1 - _shipsize;
                    if (blocktocheck >= 0 && blocktocheck < 4)  // check if the block is exist
                        if (validlines[j][blocktocheck] != -1)  // check if the block is valid
                        {
                            _position[validlines[j][blocktocheck]] = missedshot;
                            // ESP_LOGE(TAG, "validlines j: %d i: %d", j, i);
                            // check is the ship is 3 and block inbetween front and back is invalid, invalidate the
                            // result
                            if (_shipsize == 3 && validlines[j][i - 1] == -1) {
                                _position[validlines[j][blocktocheck]] = 0;
                                // ESP_LOGE(TAG, "nullified");
                            }
                        }

                    // check the block after
                    blocktocheck = i - 1 + _shipsize;
                    if (blocktocheck >= 0 && blocktocheck < 4)  // check if the block is exist
                        if (validlines[j][blocktocheck] != -1)  // check if the block is valid
                        {
                            _position[validlines[j][blocktocheck]] = missedshot;
                            // ESP_LOGE(TAG, "validlines j: %d i: %d", j, i);

                            // check is the ship is 3 and block inbetween front and back is invalid, invalidate the
                            // result
                            if (_shipsize == 3 && validlines[j][i + 1] == -1) {
                                _position[validlines[j][blocktocheck]] = 0;
                                // ESP_LOGE(TAG, "nullified");
                            }
                        }
                }
            }
        }
    }
}

void CheckforDestroyedShips(void) {
    if (playerboard[playership[0]] == boathit)
        playerboard[playership[0]] = boatdestroyed;
    if (playerboard[playership[1]] == boathit && playerboard[playership[2]] == boathit) {
        playerboard[playership[1]] = boatdestroyed;
        playerboard[playership[2]] = boatdestroyed;
    }
    if (playerboard[playership[3]] == boathit && playerboard[playership[4]] == boathit &&
        playerboard[playership[5]] == boathit) {
        playerboard[playership[3]] = boatdestroyed;
        playerboard[playership[4]] = boatdestroyed;
        playerboard[playership[5]] = boatdestroyed;
    }

    if (ennemyboard[ennemyship[0]] == boathit)
        ennemyboard[ennemyship[0]] = boatdestroyed;
    if (ennemyboard[ennemyship[1]] == boathit && ennemyboard[ennemyship[2]] == boathit) {
        ennemyboard[ennemyship[1]] = boatdestroyed;
        ennemyboard[ennemyship[2]] = boatdestroyed;
    }
    if (ennemyboard[ennemyship[3]] == boathit && ennemyboard[ennemyship[4]] == boathit &&
        ennemyboard[ennemyship[5]] == boathit) {
        ennemyboard[ennemyship[3]] = boatdestroyed;
        ennemyboard[ennemyship[4]] = boatdestroyed;
        ennemyboard[ennemyship[5]] = boatdestroyed;
    }
}

int CheckforVictory(void) {
    // check for player victory
    int flagvictory = 1;
    for (int i = 0; i < 20; i++)
        if (ennemyboard[i] == boat) {
            flagvictory = 0;
        }

    if (flagvictory)
        return 1;

    // check for player defeat
    flagvictory = 1;
    for (int i = 0; i < 20; i++)
        if (playerboard[i] == boat) {
            flagvictory = 0;
        }
    if (flagvictory)
        return 2;

    // if no one lost

    return 0;
}

static void configure_keyboard(QueueHandle_t keyboard_event_queue) {
    // update the keyboard event handler settings
    event_t kbsettings = {
        .type                                     = event_control_keyboard,
        .args_control_keyboard.enable_typing      = false,
        .args_control_keyboard.enable_actions     = {true, true, false, true, true},
        .args_control_keyboard.enable_leds        = true,
        .args_control_keyboard.enable_relay       = true,
        kbsettings.args_control_keyboard.capslock = false,
    };
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);
}

static esp_err_t nvs_get_str_wrapped(char const * namespace, char const * key, char* buffer, size_t buffer_size) {
    nvs_handle_t handle;
    esp_err_t    res = nvs_open(namespace, NVS_READWRITE, &handle);
    if (res == ESP_OK) {
        size_t size = 0;
        res         = nvs_get_str(handle, key, NULL, &size);
        if ((res == ESP_OK) && (size <= buffer_size - 1)) {
            res = nvs_get_str(handle, key, buffer, &size);
            if (res != ESP_OK) {
                buffer[0] = '\0';
            }
        }
    } else {
        buffer[0] = '\0';
    }
    nvs_close(handle);
    return res;
}

int TelegraphtoBlock(char _inputletter) {
    switch (_inputletter) {
        case 'a': return 0; break;
        case 'b': return 1; break;
        case 'd': return 2; break;
        case 'e': return 3; break;
        case 'f': return 4; break;
        case 'g': return 5; break;
        case 'h': return 6; break;
        case 'i': return 7; break;
        case 'k': return 8; break;
        case 'l': return 9; break;
        case 'm': return 10; break;
        case 'n': return 11; break;
        case 'o': return 12; break;
        case 'p': return 13; break;
        case 'r': return 14; break;
        case 's': return 15; break;
        case 't': return 16; break;
        case 'v': return 17; break;
        case 'w': return 18; break;
        case 'y': return 19; break;
        default: return 0; break;
    }
}

void Displayblockstatus(int _offset_x, int _block, int _status) {
    pax_buf_t* gfx = bsp_get_gfx_buffer();
    switch (_status) {
        case water: break;
        case missedshot: pax_outline_circle(gfx, BLACK, _offset_x + telegraph_X[_block], telegraph_Y[_block], 3); break;
        case boat: pax_draw_circle(gfx, BLACK, _offset_x + telegraph_X[_block], telegraph_Y[_block], 3); break;
        case boathit: pax_draw_circle(gfx, RED, _offset_x + telegraph_X[_block], telegraph_Y[_block], 3); break;
        case boatdestroyed: pax_draw_circle(gfx, RED, _offset_x + telegraph_X[_block], telegraph_Y[_block], 5); break;
        default: break;
    }
}

screen_t screen_test_entry(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {

    pax_font_t const * font = pax_font_sky;
    pax_buf_t*         gfx  = bsp_get_gfx_buffer();

    event_t kbsettings = {
        .type                                     = event_control_keyboard,
        .args_control_keyboard.enable_typing      = false,
        .args_control_keyboard.enable_actions     = {true, true, false, true, true},
        .args_control_keyboard.enable_leds        = true,
        .args_control_keyboard.enable_relay       = true,
        kbsettings.args_control_keyboard.capslock = false,
    };
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);

    // configure_keyboard(keyboard_event_queue);

    bsp_apply_lut(lut_1s);
    pax_background(gfx, WHITE);
    pax_insert_png_buf(gfx, caronl_png_start, caronl_png_end - caronl_png_start, 0, 0, 0);
    AddSwitchesBoxtoBuffer(SWITCH_1);
    pax_draw_text(gfx, BLACK, font, 9, 8 + SWITCH_1 * 62, 116, "Exit");
    AddSwitchesBoxtoBuffer(SWITCH_2);
    pax_draw_text(gfx, BLACK, font, 9, 8 + SWITCH_2 * 62, 116, "Offline");
    AddSwitchesBoxtoBuffer(SWITCH_4);
    pax_draw_text(gfx, BLACK, font, 9, 8 + SWITCH_4 * 62, 116, "Replay");
    AddSwitchesBoxtoBuffer(SWITCH_5);
    pax_draw_text(gfx, BLACK, font, 9, 8 + SWITCH_5 * 62, 116, "Online");

    // Addborder2toBuffer();
    bsp_display_flush();



    while (1) {
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1: return screen_home; break;
                        case SWITCH_2:
                            return screen_battleship_placeships(application_event_queue, keyboard_event_queue);
                            break;
                        case SWITCH_3: break;
                        case SWITCH_4: bsp_set_addressable_led(0xFF0000); break;  // replay, to implement
                        case SWITCH_5: break;                                     // online, to implement
                        default: break;
                    }
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

screen_t screen_battleship_placeships(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {

    event_t kbsettings = {
        .type                                     = event_control_keyboard,
        .args_control_keyboard.enable_typing      = true,
        .args_control_keyboard.enable_actions     = {true, false, false, false, false},
        .args_control_keyboard.enable_leds        = true,
        .args_control_keyboard.enable_relay       = true,
        kbsettings.args_control_keyboard.capslock = false,
    };
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);

    int  shipplaced         = 0;
    int  exitconf           = 0;
    char exitprompt[128]    = "Do you want to exit and declare forfeit";
    int  flagstart          = 0;
    int  playershipdummy[6] = {
        4,
        3,
        10,
        11,
        12,
    };
    // for (int i = 0; i < 20; i++) {
    //     _position[i] = 0;
    // }
    //  CheckforShipPlacement(3, playershipdummy, playershipdummy[1], 1);
    // for (int i = 0; i < 20; i++) {
    //     ESP_LOGE(TAG, "_position: %d", _position[i]);
    // }
    Display_battleship_placeships(shipplaced, flagstart);

    while (1) {
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1:
                            //  undo instead of exit
                            if (shipplaced > 0) {
                                shipplaced--;
                                // skip the middle of the 3 long ship
                                if (shipplaced == 4)
                                    shipplaced--;
                                playerboard[playership[shipplaced]] = water;
                                playership[shipplaced]              = -1;
                                // disable the input to press start
                                if (shipplaced < 5) {
                                    flagstart          = 0;
                                    event_t kbsettings = {
                                        .type                                     = event_control_keyboard,
                                        .args_control_keyboard.enable_typing      = true,
                                        .args_control_keyboard.enable_actions     = {true, false, false, false, false},
                                        .args_control_keyboard.enable_leds        = true,
                                        .args_control_keyboard.enable_relay       = true,
                                        kbsettings.args_control_keyboard.capslock = false,
                                    };
                                    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);
                                }
                                Display_battleship_placeships(shipplaced, flagstart);

                            } else {
                                if (exitconf)
                                    return screen_home;
                                else
                                    exitconf = DisplayExitConfirmation(exitprompt, keyboard_event_queue);
                            }
                            break;
                        case SWITCH_2: break;
                        case SWITCH_3: break;
                        case SWITCH_4: break;
                        case SWITCH_5:
                            if (exitconf) {
                                exitconf = 0;
                                Display_battleship_placeships(shipplaced, flagstart);
                            } else
                                return screen_battleship_battle(application_event_queue, keyboard_event_queue);
                            break;
                        default: break;
                    }
                    if (event.args_input_keyboard.character != '\0') {

                        playership[shipplaced]              = TelegraphtoBlock(event.args_input_keyboard.character);
                        playerboard[playership[shipplaced]] = boat;
                        // if (shipplaced < 5)
                        shipplaced++;
                        // skip the middle of the 3 long ship
                        if (shipplaced == 4)
                            shipplaced++;
                        // enable the input to press start
                        Display_battleship_placeships(shipplaced, flagstart);
                        if (shipplaced >= 5) {
                            flagstart          = 1;
                            event_t kbsettings = {
                                .type                                     = event_control_keyboard,
                                .args_control_keyboard.enable_typing      = true,
                                .args_control_keyboard.enable_actions     = {true, false, false, false, true},
                                .args_control_keyboard.enable_leds        = true,
                                .args_control_keyboard.enable_relay       = true,
                                kbsettings.args_control_keyboard.capslock = false,
                            };
                            xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);
                        }
                    }
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}

void Display_battleship_placeships(int _shipplaced, int _flagstart) {
    int telegraph_x      = 100;
    int text_x           = 150;
    int text_y           = 20;
    int gapinstruction_y = 50;
    int fontsize         = 9;

    pax_font_t const * font = pax_font_sky;
    pax_buf_t*         gfx  = bsp_get_gfx_buffer();

    ESP_LOGE(TAG, "ship placed: %d", _shipplaced);
    debugboardstatus(playerboard);
    debugshipstatus(playership);

    for (int i = 0; i < 20; i++) _position[i] = 0;  // reset position
    CheckforShipPlacement(playership, _shipplaced);
    // switch (_shipplaced) {
    //     case 0: CheckforShipPlacement(2, playership, 0, 1); break;
    //     case 1: CheckforShipPlacement(2, playership, 0, 1); break;
    //     case 2: CheckforShipPlacement(2, playership, playership[1], 0); break;
    //     case 3: CheckforShipPlacement(3, playership, 0, 1); break;
    //     case 5: CheckforShipPlacement(3, playership, playership[3], 0); break;
    //     default: break;
    // }

    ESP_LOGE(TAG, "position");
    debugboardstatus(_position);

    bsp_apply_lut(lut_1s);
    pax_background(gfx, WHITE);
    DisplayTelegraph(BLACK, telegraph_x);
    AddSwitchesBoxtoBuffer(SWITCH_1);

    // display exit or undo
    if (_shipplaced == 0)
        pax_draw_text(gfx, BLACK, font, 9, 8 + SWITCH_1 * 62, 116, "Exit");
    else
        pax_draw_text(gfx, BLACK, font, 9, 8 + SWITCH_1 * 62, 116, "Undo");

    if (_flagstart) {
        AddSwitchesBoxtoBuffer(SWITCH_5);
        pax_draw_text(gfx, BLACK, font, 9, 8 + SWITCH_5 * 62, 116, "Start");
    }

    // instructions
    pax_draw_text(gfx, BLACK, font, fontsize, text_x + 3, text_y, "Place your ships using");
    pax_draw_text(gfx, BLACK, font, fontsize, text_x, text_y + fontsize, "the telegraph to enter");
    pax_draw_text(gfx, BLACK, font, fontsize, text_x + 15, text_y + fontsize * 2, "its coordinates");

    // additional contextual instructions
    // if (_shipplaced >= 2) {
    pax_outline_circle(gfx, BLACK, text_x + 10, text_y + 8 + gapinstruction_y, 3);
    pax_outline_circle(gfx, BLACK, text_x + 10, text_y + 8 + gapinstruction_y, 6);
    pax_draw_text(gfx, BLACK, font, fontsize, text_x + 20, text_y + gapinstruction_y, "Shows where the ");
    pax_draw_text(gfx, BLACK, font, fontsize, text_x + 20, text_y + gapinstruction_y + fontsize, "ships can be placed");
    // }

    for (int i = 0; i < 20; i++) {
        Displayblockstatus(telegraph_x, i, playerboard[i]);
        Displayblockstatus(telegraph_x, i, _position[i]);
    }

    // Addborder2toBuffer();
    bsp_display_flush();
}

screen_t screen_battleship_battle(QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue) {
    event_t kbsettings = {
        .type                                     = event_control_keyboard,
        .args_control_keyboard.enable_typing      = true,
        .args_control_keyboard.enable_actions     = {true, false, false, false, false},
        .args_control_keyboard.enable_leds        = true,
        .args_control_keyboard.enable_relay       = true,
        kbsettings.args_control_keyboard.capslock = false,
    };
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);

    int  exitconf        = 0;
    char exitprompt[128] = "Do you want to exit and declare forfeit";
    int  turn            = ennemy;
    char nickname[64]    = "";
    char ennemyname[64]  = "AI";

    for (int i = 0; i < 6; i++) {
        ennemyboard[ennemyship[i]] = boat;
    }

    nvs_get_str_wrapped("owner", "nickname", nickname, sizeof(nickname));

    Display_battleship_battle(nickname, ennemyname, turn);

    while (1) {
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            switch (event.type) {
                // view game states
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1:
                            if (exitconf)
                                return screen_home;
                            else
                                exitconf = DisplayExitConfirmation(exitprompt, keyboard_event_queue);
                            break;
                        case SWITCH_2: break;
                        case SWITCH_3: break;
                        case SWITCH_4: break;
                        case SWITCH_5:
                            if (exitconf) {
                                exitconf = 0;
                                Display_battleship_battle(nickname, ennemyname, turn);
                            } else
                                screen_battleship_battle(application_event_queue, keyboard_event_queue);
                            break;
                        default: break;
                    }
                    if (event.args_input_keyboard.character != '\0') {
                        turn            = player;
                        int hitlocation = TelegraphtoBlock(event.args_input_keyboard.character);
                        if (ennemyboard[hitlocation] == water)
                            ennemyboard[hitlocation] = missedshot;
                        if (ennemyboard[hitlocation] == boat)
                            ennemyboard[hitlocation] = boathit;
                        ESP_LOGE(TAG, "player shot");
                        CheckforDestroyedShips();
                        Display_battleship_battle(nickname, ennemyname, turn);
                        for (int i = 0; i < 20; i++) {
                            ESP_LOGE(TAG, "ennemyboard: %d is %d", i, ennemyboard[i]);
                        }
                        for (int i = 0; i < 20; i++) {
                            ESP_LOGE(TAG, "playerboard: %d is %d", i, playerboard[i]);
                        }

                        if (CheckforVictory()) {
                            ESP_LOGE(TAG, "enter victroy");
                            vTaskDelay(pdMS_TO_TICKS(1000));
                            return screen_battleship_victory(
                                application_event_queue,
                                keyboard_event_queue,
                                CheckforVictory()
                            );
                        }

                        vTaskDelay(pdMS_TO_TICKS(1000));


                        turn = ennemy;
                        do hitlocation = esp_random() % 20;
                        while ((playerboard[hitlocation] == missedshot) || (playerboard[hitlocation] == boathit) ||
                               (playerboard[hitlocation] == boatdestroyed));
                        if (playerboard[hitlocation] == water)
                            playerboard[hitlocation] = missedshot;
                        if (playerboard[hitlocation] == boat)
                            playerboard[hitlocation] = boathit;
                        ESP_LOGE(TAG, "ennemy shot");
                        for (int i = 0; i < 20; i++) {
                            ESP_LOGE(TAG, "ennemyboard: %d is %d", i, ennemyboard[i]);
                        }
                        for (int i = 0; i < 20; i++) {
                            ESP_LOGE(TAG, "playerboard: %d is %d", i, playerboard[i]);
                        }


                        CheckforDestroyedShips();
                        Display_battleship_battle(nickname, ennemyname, turn);
                        if (CheckforVictory()) {
                            ESP_LOGE(TAG, "enter victroy");
                            vTaskDelay(pdMS_TO_TICKS(1000));
                            return screen_battleship_victory(
                                application_event_queue,
                                keyboard_event_queue,
                                CheckforVictory()
                            );
                        }
                    }
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
            ESP_LOGE(TAG, "exitconf: %d", exitconf);
        }
    }
}

void Display_battleship_battle(char _nickname[64], char _ennemyname[64], int _turn) {
    pax_font_t const * font = pax_font_sky;
    pax_buf_t*         gfx  = bsp_get_gfx_buffer();

    int telegraphplayer_x = 100;
    int telegraphennemy_x = gfx->height - telegraphplayer_x;
    int text_x            = 20;
    int text_y            = 35;
    int text_fontsize     = 9;

    bsp_apply_lut(lut_1s);
    pax_background(gfx, WHITE);

    DisplayTelegraph(BLACK, telegraphplayer_x);
    DisplayTelegraph(BLACK, telegraphennemy_x);

    AddSwitchesBoxtoBuffer(SWITCH_1);
    pax_draw_text(gfx, BLACK, font, 9, 8 + SWITCH_1 * 62, 116, "Exit");

    pax_draw_text(gfx, BLACK, font, 14, 0, 0, _nickname);
    if (_turn == ennemy)  // counter intuitive, but it is waiting on next player so it's inverted
        pax_draw_text(gfx, BLACK, font, text_fontsize, 0, 15, "your turn");
    Justify_right_text(gfx, BLACK, font, 14, 0, 0, _ennemyname);

    if (_turn == player)
        Justify_right_text(gfx, BLACK, font, text_fontsize, 0, 15, "your turn");


    // instructions
    pax_draw_text(gfx, BLACK, font, text_fontsize, text_x, text_y, "Miss");
    pax_outline_circle(gfx, BLACK, text_x - 10, text_y, 3);
    pax_outline_circle(gfx, BLACK, text_x - 10, text_y, 6);
    pax_draw_text(gfx, BLACK, font, text_fontsize, text_x, text_y + text_fontsize * 2, "Hit");
    pax_draw_circle(gfx, RED, text_x - 10, text_y + text_fontsize * 2, 3);
    pax_outline_circle(gfx, BLACK, text_x - 10, text_y + text_fontsize * 2, 6);
    pax_draw_text(gfx, BLACK, font, text_fontsize, text_x, text_y + text_fontsize * 4, "Sunken");
    pax_draw_circle(gfx, RED, text_x - 10, text_y + text_fontsize * 4, 5);
    pax_outline_circle(gfx, BLACK, text_x - 10, text_y + text_fontsize * 4, 6);


    for (int i = 0; i < 20; i++) {
        Displayblockstatus(telegraphplayer_x, i, playerboard[i]);
        if (ennemyboard[i] != boat)
            Displayblockstatus(telegraphennemy_x, i, ennemyboard[i]);
    }

    bsp_display_flush();
}

screen_t screen_battleship_victory(
    QueueHandle_t application_event_queue, QueueHandle_t keyboard_event_queue, int _victoryflag
) {
    event_t kbsettings = {
        .type                                     = event_control_keyboard,
        .args_control_keyboard.enable_typing      = true,
        .args_control_keyboard.enable_actions     = {true, true, true, true, true},
        .args_control_keyboard.enable_leds        = true,
        .args_control_keyboard.enable_relay       = true,
        kbsettings.args_control_keyboard.capslock = false,
    };
    xQueueSend(keyboard_event_queue, &kbsettings, portMAX_DELAY);

    pax_font_t const * font = pax_font_sky;
    pax_buf_t*         gfx  = bsp_get_gfx_buffer();

    bsp_apply_lut(lut_4s);
    pax_background(gfx, WHITE);
    if (_victoryflag == victory)
        pax_insert_png_buf(gfx, caronv_png_start, caronv_png_end - caronv_png_start, 0, 0, 0);
    if (_victoryflag == defeat)
        pax_insert_png_buf(gfx, carond_png_start, carond_png_end - carond_png_start, 0, 0, 0);
    bsp_display_flush();
    bsp_apply_lut(lut_1s);

    while (1) {
        event_t event = {0};
        if (xQueueReceive(application_event_queue, &event, 5000) == pdTRUE) {
            switch (event.type) {
                case event_input_button: break;  // Ignore raw button input
                case event_input_keyboard:
                    switch (event.args_input_keyboard.action) {
                        case SWITCH_1: return screen_home; break;
                        case SWITCH_2: return screen_home; break;
                        case SWITCH_3: return screen_home; break;
                        case SWITCH_4: return screen_home; break;
                        case SWITCH_5: return screen_home; break;
                        default: break;
                    }
                    break;
                default: ESP_LOGE(TAG, "Unhandled event type %u", event.type);
            }
        }
    }
}
