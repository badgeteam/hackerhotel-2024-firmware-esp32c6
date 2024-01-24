// Copyright [2024] [Hugo Trippaers]
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <stdint.h>

#define FRAME_VERSION_STD_2003 0
#define FRAME_VERSION_STD_2006 1
#define FRAME_VERSION_STD_2015 2

#define FRAME_TYPE_BEACON       (0)
#define FRAME_TYPE_DATA         (1)
#define FRAME_TYPE_ACK          (2)
#define FRAME_TYPE_MAC_COMMAND  (3)
#define FRAME_TYPE_RESERVED     (4)
#define FRAME_TYPE_MULTIPURPOSE (5)
#define FRAME_TYPE_FRAGMENT     (6)
#define FRAME_TYPE_EXTENDED     (7)

#define FRAME_TYPE_BEACON  (0)
#define FRAME_TYPE_DATA    (1)
#define FRAME_TYPE_ACK     (2)
#define FRAME_TYPE_MAC_CMD (3)

#define ADDR_MODE_NONE     (0)  // PAN ID and address fields are not present
#define ADDR_MODE_RESERVED (1)  // Reserved
#define ADDR_MODE_SHORT    (2)  // Short address (16-bit)
#define ADDR_MODE_LONG     (3)  // Extended address (64-bit)

#define SHORT_BROADCAST 0xFFFF


typedef struct mac_fcs {
    uint8_t frameType                  : 3;
    uint8_t secure                     : 1;
    uint8_t framePending               : 1;
    uint8_t ackReqd                    : 1;
    uint8_t panIdCompressed            : 1;
    uint8_t rfu1                       : 1;
    uint8_t sequenceNumberSuppression  : 1;
    uint8_t informationElementsPresent : 1;
    uint8_t destAddrType               : 2;
    uint8_t frameVer                   : 2;
    uint8_t srcAddrType                : 2;
} mac_fcs_t;

typedef struct {
    uint8_t mode;  // ADDR_MODE_NONE || ADDR_MODE_SHORT || ADDR_MODE_LONG
    union {
        uint16_t short_address;
        uint8_t  long_address[8];
    };
} ieee802154_address_t;

uint8_t ieee802154_header(
    uint16_t const *      src_pan,
    ieee802154_address_t* src,
    uint16_t const *      dst_pan,
    ieee802154_address_t* dst,
    uint8_t               ack,
    uint8_t*              header,
    uint8_t               header_length
);
