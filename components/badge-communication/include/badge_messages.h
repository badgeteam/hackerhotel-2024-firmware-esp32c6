#pragma once

#include "badge_messages.h"
#include "freertos/FreeRTOS.h"

#define nicknamelength 32
#define messagelength  67
#define BSpayload      25

// please no more userdata than this, else we might not have enough buffer for the message structure
#define BADGE_COMMS_USER_DATA_MAX_LEN (100)

// NOTE: DO NOT MOVE OR REMOVE ITEMS
typedef enum {
    // 0 to F reserved for tbd event messages
    MESSAGE_TYPE_TIMESTAMP  = 10,
    MESSAGE_TYPE_STRING     = 11,
    MESSAGE_TYPE_REPERTOIRE = 12,
    MESSAGE_TYPE_BATTLESHIP = 13,
    MESSAGE_TYPE_MAX        = 0xFF
} badge_comms_message_type_t;

typedef struct _badge_message_timestamp {
    time_t unix_time;
} __attribute__((packed)) badge_message_time_t;

static_assert(sizeof(badge_message_time_t) < BADGE_COMMS_USER_DATA_MAX_LEN, "badge_message_time_t is too big");

typedef struct _badge_message_str {
    char nickname[nicknamelength];
    char payload[messagelength];
} __attribute__((packed)) badge_message_chat_t;

static_assert(sizeof(badge_message_chat_t) < BADGE_COMMS_USER_DATA_MAX_LEN, "badge_message_chat_t is too big");

typedef struct _badge_message_repertoire {
    char nickname[nicknamelength];
} __attribute__((packed)) badge_message_repertoire_t;

static_assert(
    sizeof(badge_message_repertoire_t) < BADGE_COMMS_USER_DATA_MAX_LEN, "badge_message_repertoire_t is too big"
);

typedef struct {
    uint8_t dataBS[BSpayload];
} badge_message_battleship;
static_assert(sizeof(badge_message_battleship) < BADGE_COMMS_USER_DATA_MAX_LEN, "badge_message_timestamp_t is too big");