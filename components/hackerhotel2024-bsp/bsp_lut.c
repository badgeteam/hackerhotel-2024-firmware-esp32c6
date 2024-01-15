#include "bsp_lut.h"
#include "epaper.h"
#include "esp_err.h"

void LUTset(int LUTconfiguration)
{
    switch (LUTconfiguration)
    {
    case LutFull: // uses all groups
        lut->groups[0].repeat = 0x01;
        lut->groups[1].repeat = 0x13;
        lut->groups[2].repeat = 0x04;
        lut->groups[3].repeat = 0x03;
        lut->groups[4].repeat = 0x04;
        lut->groups[5].repeat = 0x03;

        lut->groups[0].tp[0] = 0x36;
        lut->groups[0].tp[1] = 0x30;
        lut->groups[0].tp[2] = 0x33;
        lut->groups[0].tp[3] = 0x00;

        lut->groups[1].tp[0] = 0x02;
        lut->groups[1].tp[1] = 0x02;
        lut->groups[1].tp[2] = 0x02;
        lut->groups[1].tp[3] = 0x02;

        lut->groups[2].tp[0] = 0x01;
        lut->groups[2].tp[1] = 0x16;
        lut->groups[2].tp[2] = 0x01;
        lut->groups[2].tp[3] = 0x16;

        lut->groups[3].tp[0] = 0x02;
        lut->groups[3].tp[1] = 0x0b;
        lut->groups[3].tp[2] = 0x10;
        lut->groups[3].tp[3] = 0x00;

        lut->groups[4].tp[0] = 0x06;
        lut->groups[4].tp[1] = 0x04;
        lut->groups[4].tp[2] = 0x02;
        lut->groups[4].tp[3] = 0x2b;

        lut->groups[5].tp[0] = 0x14;
        lut->groups[5].tp[1] = 0x04;
        lut->groups[5].tp[2] = 0x23;
        lut->groups[5].tp[3] = 0x02;
        break;

    case Lut8s: // uses all groups without repeat
        lut->groups[0].repeat = 0;
        lut->groups[1].repeat = 0;
        lut->groups[2].repeat = 0;
        lut->groups[3].repeat = 0;
        lut->groups[4].repeat = 0;
        lut->groups[5].repeat = 0;

        lut->groups[0].tp[0] = 0x36;
        lut->groups[0].tp[1] = 0x30;
        lut->groups[0].tp[2] = 0x33;
        lut->groups[0].tp[3] = 0x00;

        lut->groups[1].tp[0] = 0x02;
        lut->groups[1].tp[1] = 0x02;
        lut->groups[1].tp[2] = 0x02;
        lut->groups[1].tp[3] = 0x02;

        lut->groups[2].tp[0] = 0x01;
        lut->groups[2].tp[1] = 0x16;
        lut->groups[2].tp[2] = 0x01;
        lut->groups[2].tp[3] = 0x16;

        lut->groups[3].tp[0] = 0x02;
        lut->groups[3].tp[1] = 0x0b;
        lut->groups[3].tp[2] = 0x10;
        lut->groups[3].tp[3] = 0x00;

        lut->groups[4].tp[0] = 0x06;
        lut->groups[4].tp[1] = 0x04;
        lut->groups[4].tp[2] = 0x02;
        lut->groups[4].tp[3] = 0x2b;

        lut->groups[5].tp[0] = 0x14;
        lut->groups[5].tp[1] = 0x04;
        lut->groups[5].tp[2] = 0x23;
        lut->groups[5].tp[3] = 0x02;
        break;

    case Lut4s: // uses groups 3,4 and 5 without repeat
        lut->groups[0].repeat = 0;
        lut->groups[1].repeat = 0;
        lut->groups[2].repeat = 0;
        lut->groups[3].repeat = 0;
        lut->groups[4].repeat = 0;
        lut->groups[5].repeat = 0;

        lut->groups[0].tp[0] = 0;
        lut->groups[0].tp[1] = 0;
        lut->groups[0].tp[2] = 0;
        lut->groups[0].tp[3] = 0;

        lut->groups[1].tp[0] = 0;
        lut->groups[1].tp[1] = 0;
        lut->groups[1].tp[2] = 0;
        lut->groups[1].tp[3] = 0;

        lut->groups[2].tp[0] = 0;
        lut->groups[2].tp[1] = 0;
        lut->groups[2].tp[2] = 0;
        lut->groups[2].tp[3] = 0;

        lut->groups[3].tp[0] = 0x02;
        lut->groups[3].tp[1] = 0x0b;
        lut->groups[3].tp[2] = 0x10;
        lut->groups[3].tp[3] = 0x00;

        lut->groups[4].tp[0] = 0x06;
        lut->groups[4].tp[1] = 0x04;
        lut->groups[4].tp[2] = 0x02;
        lut->groups[4].tp[3] = 0x2b;

        lut->groups[5].tp[0] = 0x14;
        lut->groups[5].tp[1] = 0x04;
        lut->groups[5].tp[2] = 0x23;
        lut->groups[5].tp[3] = 0x02;
        break;

    case Lut1s: // use group 3 and 4 without repeat, takes ~1400ms
        lut->groups[0].repeat = 0;
        lut->groups[1].repeat = 0;
        lut->groups[2].repeat = 0;
        lut->groups[3].repeat = 0;
        lut->groups[4].repeat = 0;
        lut->groups[5].repeat = 0;

        lut->groups[0].tp[0] = 0;
        lut->groups[0].tp[1] = 0;
        lut->groups[0].tp[2] = 0;
        lut->groups[0].tp[3] = 0;

        lut->groups[1].tp[0] = 0;
        lut->groups[1].tp[1] = 0;
        lut->groups[1].tp[2] = 0;
        lut->groups[1].tp[3] = 0;

        lut->groups[2].tp[0] = 0;
        lut->groups[2].tp[1] = 0;
        lut->groups[2].tp[2] = 0;
        lut->groups[2].tp[3] = 0;

        lut->groups[3].tp[0] = 0x02;
        lut->groups[3].tp[1] = 0x0b;
        lut->groups[3].tp[2] = 0x10;
        lut->groups[3].tp[3] = 0x00;

        lut->groups[4].tp[0] = 0;
        lut->groups[4].tp[1] = 0;
        lut->groups[4].tp[2] = 0;
        lut->groups[4].tp[3] = 0;

        lut->groups[5].tp[0] = 0;
        lut->groups[5].tp[1] = 0;
        lut->groups[5].tp[2] = 0;
        lut->groups[5].tp[3] = 0;
        break;

    case LutWhite: // NOT WORKING ATM - uses groups 3,4 and 5 without repeat
        lut->groups[0].repeat = 0;
        lut->groups[1].repeat = 0;
        lut->groups[2].repeat = 0;
        lut->groups[3].repeat = 0;
        lut->groups[4].repeat = 0;
        lut->groups[5].repeat = 0;

        lut->groups[0].tp[0] = 0;
        lut->groups[0].tp[1] = 0;
        lut->groups[0].tp[2] = 0;
        lut->groups[0].tp[3] = 0;

        lut->groups[1].tp[0] = 0;
        lut->groups[1].tp[1] = 0;
        lut->groups[1].tp[2] = 0;
        lut->groups[1].tp[3] = 0;

        lut->groups[2].tp[0] = 0;
        lut->groups[2].tp[1] = 0;
        lut->groups[2].tp[2] = 0;
        lut->groups[2].tp[3] = 0;

        lut->groups[3].tp[0] = 0x02;
        lut->groups[3].tp[1] = 0x0b;
        lut->groups[3].tp[2] = 0;
        lut->groups[3].tp[3] = 0;

        lut->groups[4].tp[0] = 0;
        lut->groups[4].tp[1] = 0;
        lut->groups[4].tp[2] = 0;
        lut->groups[4].tp[3] = 0;

        lut->groups[5].tp[0] = 0;
        lut->groups[5].tp[1] = 0;
        lut->groups[5].tp[2] = 0;
        lut->groups[5].tp[3] = 0;
        break;

            case LutBlack: // use the second half of group 3, takes ~2s
        lut->groups[0].repeat = 0;
        lut->groups[1].repeat = 0;
        lut->groups[2].repeat = 0;
        lut->groups[3].repeat = 0x03;
        lut->groups[4].repeat = 0;
        lut->groups[5].repeat = 0;

        lut->groups[0].tp[0] = 0;
        lut->groups[0].tp[1] = 0;
        lut->groups[0].tp[2] = 0;
        lut->groups[0].tp[3] = 0;

        lut->groups[1].tp[0] = 0;
        lut->groups[1].tp[1] = 0;
        lut->groups[1].tp[2] = 0;
        lut->groups[1].tp[3] = 0;

        lut->groups[2].tp[0] = 0;
        lut->groups[2].tp[1] = 0;
        lut->groups[2].tp[2] = 0;
        lut->groups[2].tp[3] = 0;

        lut->groups[3].tp[0] = 0;
        lut->groups[3].tp[1] = 0;
        lut->groups[3].tp[2] = 0x10;
        lut->groups[3].tp[3] = 0x00;

        lut->groups[4].tp[0] = 0;
        lut->groups[4].tp[1] = 0;
        lut->groups[4].tp[2] = 0;
        lut->groups[4].tp[3] = 0;

        lut->groups[5].tp[0] = 0;
        lut->groups[5].tp[1] = 0;
        lut->groups[5].tp[2] = 0;
        lut->groups[5].tp[3] = 0;
        break;

        case LutRed: // use the second half of group 3, and full group 4 and 5, takes ~13s
        lut->groups[0].repeat = 0;
        lut->groups[1].repeat = 0;
        lut->groups[2].repeat = 0;
        lut->groups[3].repeat = 0x03;
        lut->groups[4].repeat = 0x04;
        lut->groups[5].repeat = 0x03;

        lut->groups[0].tp[0] = 0;
        lut->groups[0].tp[1] = 0;
        lut->groups[0].tp[2] = 0;
        lut->groups[0].tp[3] = 0;

        lut->groups[1].tp[0] = 0;
        lut->groups[1].tp[1] = 0;
        lut->groups[1].tp[2] = 0;
        lut->groups[1].tp[3] = 0;

        lut->groups[2].tp[0] = 0;
        lut->groups[2].tp[1] = 0;
        lut->groups[2].tp[2] = 0;
        lut->groups[2].tp[3] = 0;

        lut->groups[3].tp[0] = 0;
        lut->groups[3].tp[1] = 0;
        lut->groups[3].tp[2] = 0x10;
        lut->groups[3].tp[3] = 0x00;

        lut->groups[4].tp[0] = 0x06;
        lut->groups[4].tp[1] = 0x04;
        lut->groups[4].tp[2] = 0x02;
        lut->groups[4].tp[3] = 0x2b;

        lut->groups[5].tp[0] = 0x14;
        lut->groups[5].tp[1] = 0x04;
        lut->groups[5].tp[2] = 0x23;
        lut->groups[5].tp[3] = 0x02;
        break;

        //     case Lut1s: // Fastest found, 900ms, but doesn't refresh the whole screen
        // lut->groups[0].repeat = 0;
        // lut->groups[1].repeat = 0;
        // lut->groups[2].repeat = 0;
        // lut->groups[3].repeat = 0;
        // lut->groups[4].repeat = 0;
        // lut->groups[5].repeat = 0;

        // lut->groups[0].tp[0] = 0;
        // lut->groups[0].tp[1] = 0;
        // lut->groups[0].tp[2] = 0;
        // lut->groups[0].tp[3] = 0;

        // lut->groups[1].tp[0] = 0;
        // lut->groups[1].tp[1] = 0;
        // lut->groups[1].tp[2] = 0;
        // lut->groups[1].tp[3] = 0;

        // lut->groups[2].tp[0] = 0;
        // lut->groups[2].tp[1] = 0;
        // lut->groups[2].tp[2] = 0;
        // lut->groups[2].tp[3] = 0;

        // lut->groups[3].tp[0] = 0x02;
        // lut->groups[3].tp[1] = 0x0b;
        // lut->groups[3].tp[2] = 0x10;
        // lut->groups[3].tp[3] = 0x00;

        // lut->groups[4].tp[0] = 0;
        // lut->groups[4].tp[1] = 0;
        // lut->groups[4].tp[2] = 0;
        // lut->groups[4].tp[3] = 0;

        // lut->groups[5].tp[0] = 0;
        // lut->groups[5].tp[1] = 0;
        // lut->groups[5].tp[2] = 0;
        // lut->groups[5].tp[3] = 0;
        // break;
    }
    hink_set_lut(bsp_get_epaper(), (uint8_t *)lut);
}