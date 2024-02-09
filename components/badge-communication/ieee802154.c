// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2024 Hugo Trippaers

#include "badge-communication-ieee802154.h"
#include "esp_log.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

void reverse_memcpy(uint8_t* restrict dst, const uint8_t* restrict src, size_t n) {
    size_t i;

    for (i = 0; i < n; ++i) {
        dst[n - 1 - i] = src[i];
    }
}

uint8_t ieee802154_header(
    const uint16_t*       src_pan,
    ieee802154_address_t* src,
    const uint16_t*       dst_pan,
    ieee802154_address_t* dst,
    uint8_t               ack,
    uint8_t*              header,
    uint8_t               header_length
) {
    uint8_t   frame_header_len = 2;
    mac_fcs_t frame_header     = {
            .frameType                  = FRAME_TYPE_DATA,
            .secure                     = false,
            .framePending               = false,
            .ackReqd                    = ack,
            .panIdCompressed            = false,
            .rfu1                       = false,
            .sequenceNumberSuppression  = false,
            .informationElementsPresent = false,
            .destAddrType               = dst->mode,
            .frameVer                   = FRAME_VERSION_STD_2003,
            .srcAddrType                = src->mode
    };

    bool src_present     = src != NULL && src->mode != ADDR_MODE_NONE;
    bool dst_present     = dst != NULL && dst->mode != ADDR_MODE_NONE;
    bool src_pan_present = src_pan != NULL;
    bool dst_pan_present = dst_pan != NULL;

    if (src_pan_present && dst_pan_present && src_present && dst_present) {
        if (*src_pan == *dst_pan) {
            frame_header.panIdCompressed = true;
        }
    }

    if (!frame_header.sequenceNumberSuppression) {
        frame_header_len += 1;
    }

    if (dst_pan_present) {
        frame_header_len += 2;
    }

    if (frame_header.destAddrType == ADDR_MODE_SHORT) {
        frame_header_len += 2;
    } else if (frame_header.destAddrType == ADDR_MODE_LONG) {
        frame_header_len += 8;
    }

    if (src_pan_present && !frame_header.panIdCompressed) {
        frame_header_len += 2;
    }

    if (frame_header.srcAddrType == ADDR_MODE_SHORT) {
        frame_header_len += 2;
    } else if (frame_header.srcAddrType == ADDR_MODE_LONG) {
        frame_header_len += 8;
    }

    if (header_length < frame_header_len) {
        return 0;
    }

    uint8_t position = 0;
    memcpy(&header[position], &frame_header, sizeof frame_header);
    position += 2;

    if (!frame_header.sequenceNumberSuppression) {
        header[position++] = 0;
    }

    if (dst_pan != NULL) {
        memcpy(&header[position], dst_pan, sizeof(uint16_t));
        position += 2;
    }

    if (frame_header.destAddrType == ADDR_MODE_SHORT) {
        memcpy(&header[position], &dst->short_address, sizeof dst->short_address);
        position += 2;
    } else if (frame_header.destAddrType == ADDR_MODE_LONG) {
        reverse_memcpy(&header[position], (uint8_t*)&dst->long_address, sizeof dst->long_address);
        position += 8;
    }

    if (src_pan != NULL && !frame_header.panIdCompressed) {
        memcpy(&header[position], src_pan, sizeof(uint16_t));
        position += 2;
    }

    if (frame_header.srcAddrType == ADDR_MODE_SHORT) {
        memcpy(&header[position], &src->short_address, sizeof src->short_address);
        position += 2;
    } else if (frame_header.srcAddrType == ADDR_MODE_LONG) {
        reverse_memcpy(&header[position], (uint8_t*)&src->long_address, sizeof src->long_address);
        position += 8;
    }

    return position;
}

uint8_t ieee802154_header_parse(
    uint8_t*              header,
    mac_fcs_t*            fcs_out,
    uint16_t*             src_pan,
    ieee802154_address_t* src,
    uint16_t*             dst_pan,
    ieee802154_address_t* dst,
    bool*                 ack
) {

    uint8_t    position  = 0;
    mac_fcs_t* fcs       = (mac_fcs_t*)&header[position];
    *fcs_out             = *fcs;
    uint16_t* fcsint     = (uint16_t*)&header[position];
    // printf("FCS @ %u: %04x\r\n", position, *fcsint);
    position            += sizeof(mac_fcs_t);

    /*printf("frameType: %02x\n", fcs->frameType);
    printf("secure   : %02x\n", fcs->secure);
    printf("framepend: %02x\n", fcs->framePending);
    printf("ackreqd  : %02x\n", fcs->ackReqd);
    printf("pancompr : %02x\n", fcs->panIdCompressed);
    printf("rfu1     : %02x\n", fcs->rfu1);
    printf("seqnrsupr: %02x\n", fcs->sequenceNumberSuppression);
    printf("infelepre: %02x\n", fcs->informationElementsPresent);
    printf("destaddrt: %02x\n", fcs->destAddrType);
    printf("framever : %02x\n", fcs->frameVer);
    printf("srcaddrt : %02x\n", fcs->srcAddrType);*/

    uint8_t seq  = header[position];
    // printf("SEQ @ %u: %02x\r\n", position, seq);
    position    += sizeof(uint8_t);

    *dst_pan  = *((uint16_t*)&header[position]);
    // printf("DST @ %u: %04x\r\n", position, (uint16_t)*dst_pan);
    position += sizeof(uint16_t);

    dst->mode = fcs->destAddrType;
    if (fcs->destAddrType == ADDR_MODE_SHORT) {
        dst->short_address  = *((uint16_t*)&header[position]);
        position           += sizeof(uint16_t);
    } else if (fcs->destAddrType == ADDR_MODE_LONG) {
        uint8_t* address  = (uint8_t*)&header[position];
        position         += 8;
        memcpy(dst->long_address, address, 8);
    }

    if (!fcs->panIdCompressed) {
        *src_pan = *((uint16_t*)&header[position]);
    } else {
        *src_pan = *dst_pan;
    }

    src->mode = fcs->srcAddrType;
    if (fcs->srcAddrType == ADDR_MODE_SHORT) {
        uint16_t* address   = (uint16_t*)&header[position];
        position           += sizeof(uint16_t);
        src->short_address  = *address;
    } else if (fcs->srcAddrType == ADDR_MODE_LONG) {
        uint8_t* address  = (uint8_t*)&header[position];
        position         += 8;
        memcpy(src->long_address, address, 8);
    }

    *ack = fcs->ackReqd;

    return position;
}