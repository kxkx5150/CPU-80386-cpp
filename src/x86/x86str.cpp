#include "x86.h"

void x86Internal::stringOp_INSB()
{
    int Xf, Yf, Zf, ag, iopl, x;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl)
        abort(13);
    if (CS_flags & 0x0080)
        Xf = 0xffff;
    else
        Xf = -1;
    Yf = regs[7];
    Zf = regs[2] & 0xffff;
    if (CS_flags & (0x0010 | 0x0020)) {
        ag = regs[1];
        if ((ag & Xf) == 0)
            return;
        x        = ld8_port(Zf);
        mem8_loc = ((Yf & Xf) + segs[0].base) >> 0;
        st8_mem8_write(x);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 0)) & Xf);
        regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
        if (ag & Xf)
            physmem8_ptr = initial_mem_ptr;
    } else {
        x        = ld8_port(Zf);
        mem8_loc = ((Yf & Xf) + segs[0].base) >> 0;
        st8_mem8_write(x);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 0)) & Xf);
    }
}
void x86Internal::stringOp_OUTSB()
{
    int Xf, cg, Sb, ag, Zf, iopl, x;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl)
        abort(13);
    if (CS_flags & 0x0080)
        Xf = 0xffff;
    else
        Xf = -1;
    Sb = CS_flags & 0x000f;
    if (Sb == 0)
        Sb = 3;
    else
        Sb--;
    cg = regs[6];
    Zf = regs[2] & 0xffff;
    if (CS_flags & (0x0010 | 0x0020)) {
        ag = regs[1];
        if ((ag & Xf) == 0)
            return;
        mem8_loc = ((cg & Xf) + segs[Sb].base) >> 0;
        x        = ld_8bits_mem8_read();
        st8_port(Zf, x);
        regs[6] = (cg & ~Xf) | ((cg + (df << 0)) & Xf);
        regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
        if (ag & Xf)
            physmem8_ptr = initial_mem_ptr;
    } else {
        mem8_loc = ((cg & Xf) + segs[Sb].base) >> 0;
        x        = ld_8bits_mem8_read();
        st8_port(Zf, x);
        regs[6] = (cg & ~Xf) | ((cg + (df << 0)) & Xf);
    }
}
void x86Internal::stringOp_MOVSB()
{
    int Xf, Yf, cg, ag, Sb, eg;
    if (CS_flags & 0x0080)
        Xf = 0xffff;
    else
        Xf = -1;
    Sb = CS_flags & 0x000f;
    if (Sb == 0)
        Sb = 3;
    else
        Sb--;
    cg       = regs[6];
    Yf       = regs[7];
    mem8_loc = ((cg & Xf) + segs[Sb].base) >> 0;
    eg       = ((Yf & Xf) + segs[0].base) >> 0;
    if (CS_flags & (0x0010 | 0x0020)) {
        ag = regs[1];
        if ((ag & Xf) == 0)
            return;

        x        = ld_8bits_mem8_read();
        mem8_loc = eg;
        st8_mem8_write(x);
        regs[6] = (cg & ~Xf) | ((cg + (df << 0)) & Xf);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 0)) & Xf);
        regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
        if (ag & Xf)
            physmem8_ptr = initial_mem_ptr;

    } else {
        x        = ld_8bits_mem8_read();
        mem8_loc = eg;
        st8_mem8_write(x);
        regs[6] = (cg & ~Xf) | ((cg + (df << 0)) & Xf);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 0)) & Xf);
    }
}
void x86Internal::stringOp_STOSB()
{
    int Xf, Yf, ag;
    if (CS_flags & 0x0080)
        Xf = 0xffff;
    else
        Xf = -1;
    Yf       = regs[7];
    mem8_loc = ((Yf & Xf) + segs[0].base) >> 0;
    if (CS_flags & (0x0010 | 0x0020)) {
        ag = regs[1];
        if ((ag & Xf) == 0)
            return;

        st8_mem8_write(regs[0]);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 0)) & Xf);
        regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
        if (ag & Xf)
            physmem8_ptr = initial_mem_ptr;

    } else {
        st8_mem8_write(regs[0]);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 0)) & Xf);
    }
}
void x86Internal::stringOp_CMPSB()
{
    int Xf, Yf, cg, ag, Sb, eg;
    if (CS_flags & 0x0080)
        Xf = 0xffff;
    else
        Xf = -1;
    Sb = CS_flags & 0x000f;
    if (Sb == 0)
        Sb = 3;
    else
        Sb--;
    cg       = regs[6];
    Yf       = regs[7];
    mem8_loc = ((cg & Xf) + segs[Sb].base) >> 0;
    eg       = ((Yf & Xf) + segs[0].base) >> 0;
    if (CS_flags & (0x0010 | 0x0020)) {
        ag = regs[1];
        if ((ag & Xf) == 0)
            return;
        x        = ld_8bits_mem8_read();
        mem8_loc = eg;
        y        = ld_8bits_mem8_read();
        do_8bit_math(7, x, y);
        regs[6] = (cg & ~Xf) | ((cg + (df << 0)) & Xf);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 0)) & Xf);
        regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
        if (CS_flags & 0x0010) {
            if (!(cc_dst == 0))
                return;
        } else {
            if (cc_dst == 0)
                return;
        }
        if (ag & Xf)
            physmem8_ptr = initial_mem_ptr;
    } else {
        x        = ld_8bits_mem8_read();
        mem8_loc = eg;
        y        = ld_8bits_mem8_read();
        do_8bit_math(7, x, y);
        regs[6] = (cg & ~Xf) | ((cg + (df << 0)) & Xf);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 0)) & Xf);
    }
}
void x86Internal::stringOp_LODSB()
{
    int Xf, cg, Sb, ag, x;
    if (CS_flags & 0x0080)
        Xf = 0xffff;
    else
        Xf = -1;
    Sb = CS_flags & 0x000f;
    if (Sb == 0)
        Sb = 3;
    else
        Sb--;
    cg       = regs[6];
    mem8_loc = ((cg & Xf) + segs[Sb].base) >> 0;
    if (CS_flags & (0x0010 | 0x0020)) {
        ag = regs[1];
        if ((ag & Xf) == 0)
            return;
        x       = ld_8bits_mem8_read();
        regs[0] = (regs[0] & -256) | x;
        regs[6] = (cg & ~Xf) | ((cg + (df << 0)) & Xf);
        regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
        if (ag & Xf)
            physmem8_ptr = initial_mem_ptr;
    } else {
        x       = ld_8bits_mem8_read();
        regs[0] = (regs[0] & -256) | x;
        regs[6] = (cg & ~Xf) | ((cg + (df << 0)) & Xf);
    }
}
void x86Internal::stringOp_SCASB()
{
    int Xf, Yf, ag, x;
    if (CS_flags & 0x0080)
        Xf = 0xffff;
    else
        Xf = -1;
    Yf       = regs[7];
    mem8_loc = ((Yf & Xf) + segs[0].base) >> 0;
    if (CS_flags & (0x0010 | 0x0020)) {
        ag = regs[1];
        if ((ag & Xf) == 0)
            return;
        x = ld_8bits_mem8_read();
        do_8bit_math(7, regs[0], x);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 0)) & Xf);
        regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
        if (CS_flags & 0x0010) {
            if (!(cc_dst == 0))
                return;
        } else {
            if (cc_dst == 0)
                return;
        }
        if (ag & Xf)
            physmem8_ptr = initial_mem_ptr;
    } else {
        x = ld_8bits_mem8_read();
        do_8bit_math(7, regs[0], x);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 0)) & Xf);
    }
}
void x86Internal::stringOp_INSD()
{
    int Xf, Yf, Zf, ag, iopl, x;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl)
        abort(13);
    if (CS_flags & 0x0080)
        Xf = 0xffff;
    else
        Xf = -1;
    Yf = regs[7];
    Zf = regs[2] & 0xffff;
    if (CS_flags & (0x0010 | 0x0020)) {
        ag = regs[1];
        if ((ag & Xf) == 0)
            return;
        x        = ld32_port(Zf);
        mem8_loc = ((Yf & Xf) + segs[0].base) >> 0;
        st32_mem8_write(x);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 2)) & Xf);
        regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
        if (ag & Xf)
            physmem8_ptr = initial_mem_ptr;
    } else {
        x        = ld32_port(Zf);
        mem8_loc = ((Yf & Xf) + segs[0].base) >> 0;
        st32_mem8_write(x);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 2)) & Xf);
    }
}
void x86Internal::stringOp_OUTSD()
{
    int Xf, cg, Sb, ag, Zf, iopl, x;
    iopl = (eflags >> 12) & 3;
    if (cpl > iopl)
        abort(13);
    if (CS_flags & 0x0080)
        Xf = 0xffff;
    else
        Xf = -1;
    Sb = CS_flags & 0x000f;
    if (Sb == 0)
        Sb = 3;
    else
        Sb--;
    cg = regs[6];
    Zf = regs[2] & 0xffff;
    if (CS_flags & (0x0010 | 0x0020)) {
        ag = regs[1];
        if ((ag & Xf) == 0)
            return;
        mem8_loc = ((cg & Xf) + segs[Sb].base) >> 0;
        x        = ld_32bits_mem8_read();
        st32_port(Zf, x);
        regs[6] = (cg & ~Xf) | ((cg + (df << 2)) & Xf);
        regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
        if (ag & Xf)
            physmem8_ptr = initial_mem_ptr;
    } else {
        mem8_loc = ((cg & Xf) + segs[Sb].base) >> 0;
        x        = ld_32bits_mem8_read();
        st32_port(Zf, x);
        regs[6] = (cg & ~Xf) | ((cg + (df << 2)) & Xf);
    }
}
void x86Internal::stringOp_MOVSD()
{
    int Xf, Yf, cg, ag, Sb, eg;
    if (CS_flags & 0x0080)
        Xf = 0xffff;
    else
        Xf = -1;

    Sb = CS_flags & 0x000f;
    if (Sb == 0)
        Sb = 3;
    else
        Sb--;

    cg       = regs[6];
    Yf       = regs[7];
    mem8_loc = ((cg & Xf) + segs[Sb].base) >> 0;
    eg       = ((Yf & Xf) + segs[0].base) >> 0;

    if (CS_flags & (0x0010 | 0x0020)) {
        ag = regs[1];
        if ((ag & Xf) == 0)
            return;

        if (Xf == -1 && df == 1 && ((mem8_loc | eg) & 3) == 0) {
            int len, l, ug, vg, i, wg;
            len = ag >> 0;
            l   = (4096 - (mem8_loc & 0xfff)) >> 2;
            if (len > l)
                len = l;

            l = (4096 - (eg & 0xfff)) >> 2;
            if (len > l)
                len = l;

            ug = do_tlb_lookup(mem8_loc, 0);
            vg = do_tlb_lookup(eg, 1);
            wg = len << 2;
            vg >>= 2;
            ug >>= 2;
            for (i = 0; i < len; i++)
                phys_mem32[vg + i] = phys_mem32[ug + i];
            regs[6] = (cg + wg) >> 0;
            regs[7] = (Yf + wg) >> 0;
            regs[1] = ag = (ag - len) >> 0;
            if (ag)
                physmem8_ptr = initial_mem_ptr;
        } else {
            x        = ld_32bits_mem8_read();
            mem8_loc = eg;
            st32_mem8_write(x);
            regs[6] = (cg & ~Xf) | ((cg + (df << 2)) & Xf);
            regs[7] = (Yf & ~Xf) | ((Yf + (df << 2)) & Xf);
            regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
            if (ag & Xf)
                physmem8_ptr = initial_mem_ptr;
        }
    } else {
        x        = ld_32bits_mem8_read();
        mem8_loc = eg;
        st32_mem8_write(x);
        regs[6] = (cg & ~Xf) | ((cg + (df << 2)) & Xf);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 2)) & Xf);
    }
}
void x86Internal::stringOp_STOSD()
{
    int Xf, Yf, ag;
    if (CS_flags & 0x0080)
        Xf = 0xffff;
    else
        Xf = -1;
    Yf       = regs[7];
    mem8_loc = ((Yf & Xf) + segs[0].base) >> 0;
    if (CS_flags & (0x0010 | 0x0020)) {
        ag = regs[1];
        if ((ag & Xf) == 0)
            return;
        if (Xf == -1 && df == 1 && (mem8_loc & 3) == 0) {
            int len, l, vg, i, wg, x;
            len = ag >> 0;
            l   = (4096 - (mem8_loc & 0xfff)) >> 2;
            if (len > l)
                len = l;
            vg = do_tlb_lookup(regs[7], 1);
            x  = regs[0];
            vg >>= 2;
            for (i = 0; i < len; i++)
                phys_mem32[vg + i] = x;
            wg      = len << 2;
            regs[7] = (Yf + wg) >> 0;
            regs[1] = ag = (ag - len) >> 0;
            if (ag)
                physmem8_ptr = initial_mem_ptr;
        } else {
            st32_mem8_write(regs[0]);
            regs[7] = (Yf & ~Xf) | ((Yf + (df << 2)) & Xf);
            regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
            if (ag & Xf)
                physmem8_ptr = initial_mem_ptr;
        }
    } else {
        st32_mem8_write(regs[0]);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 2)) & Xf);
    }
}
void x86Internal::stringOp_CMPSD()
{
    int Xf, Yf, cg, ag, Sb, eg;
    if (CS_flags & 0x0080)
        Xf = 0xffff;
    else
        Xf = -1;
    Sb = CS_flags & 0x000f;
    if (Sb == 0)
        Sb = 3;
    else
        Sb--;
    cg       = regs[6];
    Yf       = regs[7];
    mem8_loc = ((cg & Xf) + segs[Sb].base) >> 0;
    eg       = ((Yf & Xf) + segs[0].base) >> 0;
    if (CS_flags & (0x0010 | 0x0020)) {
        ag = regs[1];
        if ((ag & Xf) == 0)
            return;
        x        = ld_32bits_mem8_read();
        mem8_loc = eg;
        y        = ld_32bits_mem8_read();
        do_32bit_math(7, x, y);
        regs[6] = (cg & ~Xf) | ((cg + (df << 2)) & Xf);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 2)) & Xf);
        regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
        if (CS_flags & 0x0010) {
            if (!(cc_dst == 0))
                return;
        } else {
            if (cc_dst == 0)
                return;
        }
        if (ag & Xf)
            physmem8_ptr = initial_mem_ptr;
    } else {
        x        = ld_32bits_mem8_read();
        mem8_loc = eg;
        y        = ld_32bits_mem8_read();
        do_32bit_math(7, x, y);
        regs[6] = (cg & ~Xf) | ((cg + (df << 2)) & Xf);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 2)) & Xf);
    }
}
void x86Internal::stringOp_LODSD()
{
    int Xf, cg, Sb, ag, x;
    if (CS_flags & 0x0080)
        Xf = 0xffff;
    else
        Xf = -1;
    Sb = CS_flags & 0x000f;
    if (Sb == 0)
        Sb = 3;
    else
        Sb--;
    cg       = regs[6];
    mem8_loc = ((cg & Xf) + segs[Sb].base) >> 0;
    if (CS_flags & (0x0010 | 0x0020)) {
        ag = regs[1];
        if ((ag & Xf) == 0)
            return;
        x       = ld_32bits_mem8_read();
        regs[0] = x;
        regs[6] = (cg & ~Xf) | ((cg + (df << 2)) & Xf);
        regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
        if (ag & Xf)
            physmem8_ptr = initial_mem_ptr;
    } else {
        x       = ld_32bits_mem8_read();
        regs[0] = x;
        regs[6] = (cg & ~Xf) | ((cg + (df << 2)) & Xf);
    }
}
void x86Internal::stringOp_SCASD()
{
    int Xf, Yf, ag, x;
    if (CS_flags & 0x0080)
        Xf = 0xffff;
    else
        Xf = -1;
    Yf       = regs[7];
    mem8_loc = ((Yf & Xf) + segs[0].base) >> 0;
    if (CS_flags & (0x0010 | 0x0020)) {
        ag = regs[1];
        if ((ag & Xf) == 0)
            return;
        x = ld_32bits_mem8_read();
        do_32bit_math(7, regs[0], x);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 2)) & Xf);
        regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
        if (CS_flags & 0x0010) {
            if (!(cc_dst == 0))
                return;
        } else {
            if (cc_dst == 0)
                return;
        }
        if (ag & Xf)
            physmem8_ptr = initial_mem_ptr;
    } else {
        x = ld_32bits_mem8_read();
        do_32bit_math(7, regs[0], x);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 2)) & Xf);
    }
}