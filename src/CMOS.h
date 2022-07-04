#ifndef _H_CMOS
#define _H_CMOS

#include <fstream>
#include <time.h>

class CMOS {
    time_t     t;
    struct tm *utcTm;
    uint8_t    cmos_data[128];
    int        cmos_index = 0;

  public:
    CMOS()
    {
        time(&t);
        utcTm           = gmtime(&t);
        cmos_data[0]    = 3;    // bin_to_bcd(utcTm->tm_sec);
        cmos_data[2]    = 3;    // bin_to_bcd(utcTm->tm_min);
        cmos_data[4]    = 3;    // bin_to_bcd(utcTm->tm_hour);
        cmos_data[6]    = 3;    // bin_to_bcd(utcTm->tm_wday);
        cmos_data[7]    = 3;    // bin_to_bcd(utcTm->tm_mday);
        cmos_data[8]    = 3;    // bin_to_bcd(utcTm->tm_mon + 1);
        cmos_data[9]    = 3;    // bin_to_bcd(utcTm->tm_year);
        cmos_data[10]   = 0x26;
        cmos_data[11]   = 0x02;
        cmos_data[12]   = 0x00;
        cmos_data[13]   = 0x80;
        cmos_data[0x14] = 0x02;
    }

    void ioport_write(int mem8_loc, int data)
    {
        if (mem8_loc == 0x70) {
            cmos_index = data & 0x7f;
        }
    }

    int ioport_read(int mem8_loc)
    {
        int data;
        if (mem8_loc == 0x70) {
            return 0xff;
        } else {
            data = cmos_data[cmos_index];
            if (cmos_index == 10)
                cmos_data[10] ^= 0x80;
            else if (cmos_index == 12)
                cmos_data[12] = 0x00;
            return data;
        }
    }

  private:
    int bin_to_bcd(int a)
    {
        return ((a / 10) << 4) | (a % 10);
    }
};

#endif