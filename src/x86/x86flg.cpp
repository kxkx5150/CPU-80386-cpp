#include "x86.h"

bool x86Internal::check_carry()
{
    bool     rval;
    int      Yb, currentcc_op;
    uint32_t reldst;

    if (cc_op >= 25) {
        currentcc_op = cc_op2;
        reldst       = cc_dst2;
    } else {
        currentcc_op = cc_op;
        reldst       = cc_dst;
    }

    switch (currentcc_op) {
        case 0:
            rval = (reldst & 0xff) < (cc_src & 0xff);
            break;
        case 1:
            rval = (reldst & 0xffff) < (cc_src & 0xffff);
            break;
        case 2: {
            rval = (reldst >> 0) < (cc_src >> 0);
        } break;
        case 3:
            rval = (reldst & 0xff) <= (cc_src & 0xff);
            break;
        case 4:
            rval = (reldst & 0xffff) <= (cc_src & 0xffff);
            break;
        case 5:
            rval = (reldst >> 0) <= (cc_src >> 0);
            break;
        case 6:
            rval = ((reldst + cc_src) & 0xff) < (cc_src & 0xff);
            break;
        case 7:
            rval = ((reldst + cc_src) & 0xffff) < (cc_src & 0xffff);
            break;
        case 8:
            rval = ((reldst + cc_src) >> 0) < (cc_src >> 0);
            break;
        case 9:
            Yb   = (reldst + cc_src + 1) & 0xff;
            rval = Yb <= (cc_src & 0xff);
            break;
        case 10:
            Yb   = (reldst + cc_src + 1) & 0xffff;
            rval = Yb <= (cc_src & 0xffff);
            break;
        case 11:
            Yb   = (reldst + cc_src + 1) >> 0;
            rval = Yb <= (cc_src >> 0);
            break;
        case 12:
        case 13:
        case 14:
            rval = 0;
            break;
        case 15:
            rval = (cc_src >> 7) & 1;
            break;
        case 16:
            rval = (cc_src >> 15) & 1;
            break;
        case 17:
            rval = (cc_src >> 31) & 1;
            break;
        case 18:
        case 19:
        case 20:
            rval = cc_src & 1;
            break;
        case 21:
        case 22:
        case 23:
            rval = cc_src != 0;
            break;
        case 24:
            rval = cc_src & 1;
            break;
        default:
            throw &"GET_CARRY: unsupported cccc_op="[cc_op];
    }
    return rval;
}
bool x86Internal::check_overflow()
{
    bool rval;
    int  Yb;

    switch (cc_op) {
        case 0:
            Yb   = (cc_dst - cc_src) >> 0;
            rval = (((Yb ^ cc_src ^ -1) & (Yb ^ cc_dst)) >> 7) & 1;
            break;
        case 1:
            Yb   = (cc_dst - cc_src) >> 0;
            rval = (((Yb ^ cc_src ^ -1) & (Yb ^ cc_dst)) >> 15) & 1;
            break;
        case 2:
            Yb   = (cc_dst - cc_src) >> 0;
            rval = (((Yb ^ cc_src ^ -1) & (Yb ^ cc_dst)) >> 31) & 1;
            break;
        case 3:
            Yb   = (cc_dst - cc_src - 1) >> 0;
            rval = (((Yb ^ cc_src ^ -1) & (Yb ^ cc_dst)) >> 7) & 1;
            break;
        case 4:
            Yb   = (cc_dst - cc_src - 1) >> 0;
            rval = (((Yb ^ cc_src ^ -1) & (Yb ^ cc_dst)) >> 15) & 1;
            break;
        case 5:
            Yb   = (cc_dst - cc_src - 1) >> 0;
            rval = (((Yb ^ cc_src ^ -1) & (Yb ^ cc_dst)) >> 31) & 1;
            break;
        case 6:
            Yb   = (cc_dst + cc_src) >> 0;
            rval = (((Yb ^ cc_src) & (Yb ^ cc_dst)) >> 7) & 1;
            break;
        case 7:
            Yb   = (cc_dst + cc_src) >> 0;
            rval = (((Yb ^ cc_src) & (Yb ^ cc_dst)) >> 15) & 1;
            break;
        case 8:
            Yb   = (cc_dst + cc_src) >> 0;
            rval = (((Yb ^ cc_src) & (Yb ^ cc_dst)) >> 31) & 1;
            break;
        case 9:
            Yb   = (cc_dst + cc_src + 1) >> 0;
            rval = (((Yb ^ cc_src) & (Yb ^ cc_dst)) >> 7) & 1;
            break;
        case 10:
            Yb   = (cc_dst + cc_src + 1) >> 0;
            rval = (((Yb ^ cc_src) & (Yb ^ cc_dst)) >> 15) & 1;
            break;
        case 11:
            Yb   = (cc_dst + cc_src + 1) >> 0;
            rval = (((Yb ^ cc_src) & (Yb ^ cc_dst)) >> 31) & 1;
            break;
        case 12:
        case 13:
        case 14:
            rval = 0;
            break;
        case 15:
        case 18:
            rval = ((cc_src ^ cc_dst) >> 7) & 1;
            break;
        case 16:
        case 19:
            rval = ((cc_src ^ cc_dst) >> 15) & 1;
            break;
        case 17:
        case 20:
            rval = ((cc_src ^ cc_dst) >> 31) & 1;
            break;
        case 21:
        case 22:
        case 23:
            rval = cc_src != 0;
            break;
        case 24:
            rval = (cc_src >> 11) & 1;
            break;
        case 25:
            rval = (cc_dst & 0xff) == 0x80;
            break;
        case 26:
            rval = (cc_dst & 0xffff) == 0x8000;
            break;
        case 27:
            rval = (cc_dst == -2147483648);
            break;
        case 28:
            rval = (cc_dst & 0xff) == 0x7f;
            break;
        case 29:
            rval = (cc_dst & 0xffff) == 0x7fff;
            break;
        case 30:
            rval = cc_dst == 0x7fffffff;
            break;
        default:
            throw &"JO: unsupported cccc_op="[cc_op];
    }
    return rval;
}
bool x86Internal::check_below_or_equal()
{
    bool flg = false;
    switch (cc_op) {
        case 6:
            flg = ((cc_dst + cc_src) & 0xff) <= (cc_src & 0xff);
            break;
        case 7:
            flg = ((cc_dst + cc_src) & 0xffff) <= (cc_src & 0xffff);
            break;
        case 8: {
            uint32_t val = cc_dst + cc_src;
            flg          = (val >> 0) <= (cc_src >> 0);
        } break;
        case 24:
            flg = (cc_src & (0x0040 | 0x0001)) != 0;
            break;
        default:
            flg = check_carry() | (cc_dst == 0);
            break;
    }
    return flg;
}
int x86Internal::check_parity()
{
    if (cc_op == 24) {
        return (cc_src >> 2) & 1;
    } else {
        return parity_LUT[cc_dst & 0xff];
    }
}
int x86Internal::check_less_than()
{
    bool flg;
    switch (cc_op) {
        case 6:
            flg = ((cc_dst + cc_src) << 24) < (cc_src << 24);
            break;
        case 7:
            flg = ((cc_dst + cc_src) << 16) < (cc_src << 16);
            break;
        case 8:
            flg = ((cc_dst + cc_src) >> 0) < cc_src;
            break;
        case 12:
        case 25:
        case 28:
        case 13:
        case 26:
        case 29:
        case 14:
        case 27:
        case 30:
            flg = cc_dst < 0;
            break;
        case 24:
            flg = ((cc_src >> 7) ^ (cc_src >> 11)) & 1;
            break;
        default:
            flg = (cc_op == 24 ? ((cc_src >> 7) & 1) : (cc_dst < 0)) ^ check_overflow();
            break;
    }
    return flg;
}
int x86Internal::check_less_or_equal()
{
    bool flg;
    switch (cc_op) {
        case 6:
            flg = ((cc_dst + cc_src) << 24) <= (cc_src << 24);
            break;
        case 7:
            flg = ((cc_dst + cc_src) << 16) <= (cc_src << 16);
            break;
        case 8:
            flg = ((cc_dst + cc_src) >> 0) <= cc_src;
            break;
        case 12:
        case 25:
        case 28:
        case 13:
        case 26:
        case 29:
        case 14:
        case 27:
        case 30:
            flg = cc_dst <= 0;
            break;
        case 24:
            flg = (((cc_src >> 7) ^ (cc_src >> 11)) | (cc_src >> 6)) & 1;
            break;
        default:
            flg = ((cc_op == 24 ? ((cc_src >> 7) & 1) : (cc_dst < 0)) ^ check_overflow()) | (cc_dst == 0);
            break;
    }
    return flg;
}
int x86Internal::check_adjust_flag()
{
    int Yb;
    int rval;

    switch (cc_op) {
        case 0:
        case 1:
        case 2:
            Yb   = (cc_dst - cc_src) >> 0;
            rval = (cc_dst ^ Yb ^ cc_src) & 0x10;
            break;
        case 3:
        case 4:
        case 5:
            Yb   = (cc_dst - cc_src - 1) >> 0;
            rval = (cc_dst ^ Yb ^ cc_src) & 0x10;
            break;
        case 6:
        case 7:
        case 8:
            Yb   = (cc_dst + cc_src) >> 0;
            rval = (cc_dst ^ Yb ^ cc_src) & 0x10;
            break;
        case 9:
        case 10:
        case 11:
            Yb   = (cc_dst + cc_src + 1) >> 0;
            rval = (cc_dst ^ Yb ^ cc_src) & 0x10;
            break;
        case 12:
        case 13:
        case 14:
            rval = 0;
            break;
        case 15:
        case 18:
        case 16:
        case 19:
        case 17:
        case 20:
        case 21:
        case 22:
        case 23:
            rval = 0;
            break;
        case 24:
            rval = cc_src & 0x10;
            break;
        case 25:
        case 26:
        case 27:
            rval = (cc_dst ^ (cc_dst - 1)) & 0x10;
            break;
        case 28:
        case 29:
        case 30:
            rval = (cc_dst ^ (cc_dst + 1)) & 0x10;
            break;
        default:
            throw &"AF: unsupported cccc_op="[cc_op];
    }
    return rval;
}
int x86Internal::check_status_bits_for_jump(int gd)
{
    bool flg;
    switch (gd >> 1) {
        case 0:
            flg = check_overflow();
            break;
        case 1:
            flg = check_carry();
            break;
        case 2:
            flg = (cc_dst == 0);
            break;
        case 3:
            flg = check_below_or_equal();
            break;
        case 4:
            flg = (cc_op == 24 ? ((cc_src >> 7) & 1) : (cc_dst < 0));
            break;
        case 5:
            flg = check_parity();
            break;
        case 6:
            flg = check_less_than();
            break;
        case 7:
            flg = check_less_or_equal();
            break;
        default:
            throw &"unsupported cond: "[gd];
    }
    return flg ^ (gd & 1);
}
int x86Internal::conditional_flags_for_rot_shiftcc_ops()
{
    return (check_parity() << 2) | ((cc_dst == 0) << 6) | ((cc_op == 24 ? ((cc_src >> 7) & 1) : (cc_dst < 0)) << 7) |
           check_adjust_flag();
}
int x86Internal::get_conditional_flags()
{
    int c0  = (check_carry() << 0);
    int c2  = (check_parity() << 2);
    int c6  = ((cc_dst == 0) << 6);
    int c7  = ((cc_op == 24 ? ((cc_src >> 7) & 1) : (cc_dst < 0)) << 7);
    int c11 = (check_overflow() << 11);
    int c   = check_adjust_flag();
    int val = c0 | c2 | c6 | c7 | c11 | c;
    return val;
}
int x86Internal::get_FLAGS()
{
    int flag_bits = get_conditional_flags();
    flag_bits |= df & 0x00000400;    // direction flag
    flag_bits |= eflags;             // get extended flags
    return flag_bits;
}
void x86Internal::set_FLAGS(int flag_bits, int ld)
{
    cc_src = flag_bits & (0x0800 | 0x0080 | 0x0040 | 0x0010 | 0x0004 | 0x0001);
    cc_dst = ((cc_src >> 6) & 1) ^ 1;
    cc_op  = 24;
    df     = 1 - (2 * ((flag_bits >> 10) & 1));
    eflags = (eflags & ~ld) | (flag_bits & ld);
}
