#pragma once

#include "badge_messages.h"
#include "freertos/FreeRTOS.h"

#define nicknamelenght 32
#define messagelenght  67

// please no more userdata than this, else we might not have enough buffer for the message structure
#define BADGE_COMMS_USER_DATA_MAX_LEN (100)

// NOTE: DO NOT MOVE OR REMOVE ITEMS
typedef enum {
    // 0 to F reserved for tbd event messages
    MESSAGE_TYPE_TIMESTAMP  = 10,
    MESSAGE_TYPE_STRING     = 11,
    MESSAGE_TYPE_REPERTOIRE = 12,
    MESSAGE_TYPE_MAX        = 0xFF
} badge_comms_message_type_t;

typedef struct {
    time_t unix_time;
} badge_message_timestamp_t;
static_assert(
    sizeof(badge_message_timestamp_t) < BADGE_COMMS_USER_DATA_MAX_LEN, "badge_message_timestamp_t is too big"
);

typedef struct {
    char nickname[nicknamelenght];
    char payload[messagelenght];
} badge_message_str;
static_assert(sizeof(badge_message_str) < BADGE_COMMS_USER_DATA_MAX_LEN, "badge_message_timestamp_t is too big");

typedef struct {
    char nickname[nicknamelenght];
} badge_message_repertoire;
static_assert(sizeof(badge_message_repertoire) < BADGE_COMMS_USER_DATA_MAX_LEN, "badge_message_timestamp_t is too big");