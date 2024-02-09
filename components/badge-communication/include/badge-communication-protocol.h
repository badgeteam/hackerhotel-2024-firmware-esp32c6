// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 Hugo Trippaers

#pragma once

#include "badge-communication-ieee802154.h"
#include "freertos/FreeRTOS.h"

#define BADGE_COMMUNICATION_PAN     0x1337
#define BADGE_COMMUNICATION_CHANNEL 24
#define BADGE_COMMUNICATION_MAGIC   0xBA
#define BADGE_COMMUNICATION_VERSION 0x01

#define nicknamelength 32
#define messagelength  67
#define BSpayload      25

// please no more userdata than this, else we might not have enough buffer for the message structure
#define BADGE_COMMS_USER_DATA_MAX_LEN (100)

// NOTE: DO NOT MOVE OR REMOVE ITEMS
typedef enum {
    // 0 to F reserved for tbd event messages
    MESSAGE_TYPE_TIME       = 10,
    MESSAGE_TYPE_CHAT       = 11,
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
} badge_message_battleship_t;
static_assert(
    sizeof(badge_message_battleship_t) < BADGE_COMMS_USER_DATA_MAX_LEN, "badge_message_timestamp_t is too big"
);

typedef struct _badge_communication_packet {
    uint8_t magic;
    uint8_t version;
    uint8_t repeat;
    uint8_t flags;
    uint8_t type;
    uint8_t content[0];
} __attribute__((packed)) badge_communication_packet_t;

static_assert(sizeof(badge_communication_packet_t) < 128, "badge_communication_packet_t is too big");
