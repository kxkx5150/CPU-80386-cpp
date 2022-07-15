#include "x86.h"

int x86Internal::__ld_8bits_mem8_read()
{
    do_tlb_set_page(mem8_loc, 0, cpl == 3);
    uint32_t mem8_locu  = mem8_loc;
    int      idx        = mem8_locu >> 12;
    int      tlb_lookup = tlb_read[idx] ^ mem8_loc;
    return phys_mem8[tlb_lookup];
}
int x86Internal::ld_8bits_mem8_read()
{
    int      last_tlb_val;
    uint32_t mem8_locu = mem8_loc;
    return (((last_tlb_val = tlb_read[mem8_locu >> 12]) == -1) ? __ld_8bits_mem8_read()
                                                               : phys_mem8[mem8_loc ^ last_tlb_val]);
}

int x86Internal::__ld_16bits_mem8_read()
{
    int x = ld_8bits_mem8_read();
    mem8_loc++;
    x |= ld_8bits_mem8_read() << 8;
    mem8_loc--;
    return x;
}
int x86Internal::ld_16bits_mem8_read()
{
    int      last_tlb_val;
    uint32_t mem8_locu = mem8_loc;
    return (((last_tlb_val = tlb_read[mem8_locu >> 12]) | mem8_loc) & 1 ? __ld_16bits_mem8_read()
                                                                        : phys_mem16[(mem8_loc ^ last_tlb_val) >> 1]);
}

int x86Internal::__ld_32bits_mem8_read()
{
    int x = ld_8bits_mem8_read();
    mem8_loc++;
    x |= ld_8bits_mem8_read() << 8;
    mem8_loc++;
    x |= ld_8bits_mem8_read() << 16;
    mem8_loc++;
    x |= ld_8bits_mem8_read() << 24;
    mem8_loc -= 3;
    return x;
}
int x86Internal::ld_32bits_mem8_read()
{
    int      last_tlb_val;
    uint32_t mem8_locu = mem8_loc;
    return (((last_tlb_val = tlb_read[mem8_locu >> 12]) | mem8_loc) & 3 ? __ld_32bits_mem8_read()
                                                                        : phys_mem32[(mem8_loc ^ last_tlb_val) >> 2]);
}

int x86Internal::__ld_8bits_mem8_write()
{
    do_tlb_set_page(mem8_loc, 1, cpl == 3);
    uint32_t mem8_locu  = mem8_loc;
    int      tlb_lookup = tlb_write[mem8_locu >> 12] ^ mem8_loc;
    return phys_mem8[tlb_lookup];
}
int x86Internal::ld_8bits_mem8_write()
{
    int      tlb_lookup;
    uint32_t mem8_locu = mem8_loc;
    return ((tlb_lookup = tlb_write[mem8_locu >> 12]) == -1) ? __ld_8bits_mem8_write()
                                                             : phys_mem8[mem8_loc ^ tlb_lookup];
}
int x86Internal::__ld_16bits_mem8_write()
{
    int x = ld_8bits_mem8_write();
    mem8_loc++;
    x |= ld_8bits_mem8_write() << 8;
    mem8_loc--;
    return x;
}
int x86Internal::ld_16bits_mem8_write()
{
    int      tlb_lookup;
    uint32_t mem8_locu = mem8_loc;
    return ((tlb_lookup = tlb_write[mem8_locu >> 12]) | mem8_loc) & 1 ? __ld_16bits_mem8_write()
                                                                      : phys_mem16[(mem8_loc ^ tlb_lookup) >> 1];
}
int x86Internal::__ld_32bits_mem8_write()
{
    int x = ld_8bits_mem8_write();
    mem8_loc++;
    x |= ld_8bits_mem8_write() << 8;
    mem8_loc++;
    x |= ld_8bits_mem8_write() << 16;
    mem8_loc++;
    x |= ld_8bits_mem8_write() << 24;
    mem8_loc -= 3;
    return x;
}
int x86Internal::ld_32bits_mem8_write()
{
    int      tlb_lookup;
    uint32_t mem8_locu = mem8_loc;
    return ((tlb_lookup = tlb_write[mem8_locu >> 12]) | mem8_loc) & 3 ? __ld_32bits_mem8_write()
                                                                      : phys_mem32[(mem8_loc ^ tlb_lookup) >> 2];
}

int x86Internal::__ld8_mem8_kernel_read()
{
    do_tlb_set_page(mem8_loc, 0, 0);
    uint32_t mem8_locu  = mem8_loc;
    int      tlb_lookup = tlb_read_kernel[mem8_locu >> 12] ^ mem8_loc;
    return phys_mem8[tlb_lookup];
}
int x86Internal::ld8_mem8_kernel_read()
{
    int      tlb_lookup;
    uint32_t mem8_locu = mem8_loc;
    return ((tlb_lookup = tlb_read_kernel[mem8_locu >> 12]) == -1) ? __ld8_mem8_kernel_read()
                                                                   : phys_mem8[mem8_loc ^ tlb_lookup];
}
int x86Internal::__ld16_mem8_kernel_read()
{
    int x = ld8_mem8_kernel_read();
    mem8_loc++;
    x |= ld8_mem8_kernel_read() << 8;
    mem8_loc--;
    return x;
}
int x86Internal::ld16_mem8_kernel_read()
{
    int      tlb_lookup;
    uint32_t mem8_locu = mem8_loc;
    return ((tlb_lookup = tlb_read_kernel[mem8_locu >> 12]) | mem8_loc) & 1 ? __ld16_mem8_kernel_read()
                                                                            : phys_mem16[(mem8_loc ^ tlb_lookup) >> 1];
}
int x86Internal::__ld32_mem8_kernel_read()
{
    int x = ld8_mem8_kernel_read();
    mem8_loc++;
    x |= ld8_mem8_kernel_read() << 8;
    mem8_loc++;
    x |= ld8_mem8_kernel_read() << 16;
    mem8_loc++;
    x |= ld8_mem8_kernel_read() << 24;
    mem8_loc -= 3;
    return x;
}
int x86Internal::ld32_mem8_kernel_read()
{
    int      tlb_lookup;
    uint32_t mem8_locu = mem8_loc;
    return ((tlb_lookup = tlb_read_kernel[mem8_locu >> 12]) | mem8_loc) & 3 ? __ld32_mem8_kernel_read()
                                                                            : phys_mem32[(mem8_loc ^ tlb_lookup) >> 2];
}
int x86Internal::ld16_mem8_direct()
{
    int x = phys_mem8[physmem8_ptr++];
    int y = phys_mem8[physmem8_ptr++];
    return x | (y << 8);
}

void x86Internal::__st8_mem8_write(int x)
{
    do_tlb_set_page(mem8_loc, 1, cpl == 3);
    uint32_t mem8_locu    = mem8_loc;
    int      tlb_lookup   = tlb_write[mem8_locu >> 12] ^ mem8_loc;
    phys_mem8[tlb_lookup] = x;
}
void x86Internal::st8_mem8_write(int x)
{
    uint32_t mem8_locu    = mem8_loc;
    int      idx          = mem8_locu >> 12;
    int      last_tlb_val = tlb_write[idx];
    if (last_tlb_val == -1) {
        __st8_mem8_write(x);
    } else {
        phys_mem8[mem8_loc ^ last_tlb_val] = x;
    }
}
void x86Internal::__st16_mem8_write(int x)
{
    st8_mem8_write(x);
    mem8_loc++;
    st8_mem8_write(x >> 8);
    mem8_loc--;
}
void x86Internal::st16_mem8_write(int x)
{
    uint32_t mem8_locu    = mem8_loc;
    int      last_tlb_val = tlb_write[mem8_locu >> 12];
    if ((last_tlb_val | mem8_loc) & 1) {
        __st16_mem8_write(x);
    } else {
        phys_mem16[(mem8_loc ^ last_tlb_val) >> 1] = x;
    }
}
void x86Internal::__st32_mem8_write(int x)
{
    st8_mem8_write(x);
    mem8_loc++;
    st8_mem8_write(x >> 8);
    mem8_loc++;
    st8_mem8_write(x >> 16);
    mem8_loc++;
    st8_mem8_write(x >> 24);
    mem8_loc -= 3;
}
void x86Internal::st32_mem8_write(int x)
{
    uint32_t mem8_locu    = mem8_loc;
    int      last_tlb_val = tlb_write[mem8_locu >> 12];
    if ((last_tlb_val | mem8_loc) & 3) {
        __st32_mem8_write(x);
    } else {
        int idx         = (mem8_loc ^ last_tlb_val) >> 2;
        phys_mem32[idx] = x;
    }
}

void x86Internal::__st8_mem8_kernel_write(int x)
{
    do_tlb_set_page(mem8_loc, 1, 0);
    uint32_t mem8_locu    = mem8_loc;
    int      tlb_lookup   = tlb_write_kernel[mem8_locu >> 12] ^ mem8_loc;
    phys_mem8[tlb_lookup] = x;
}
void x86Internal::st8_mem8_kernel_write(int x)
{
    uint32_t mem8_locu  = mem8_loc;
    int      tlb_lookup = tlb_write_kernel[mem8_locu >> 12];
    if (tlb_lookup == -1) {
        __st8_mem8_kernel_write(x);
    } else {
        phys_mem8[mem8_loc ^ tlb_lookup] = x;
    }
}
void x86Internal::__st16_mem8_kernel_write(int x)
{
    st8_mem8_kernel_write(x);
    mem8_loc++;
    st8_mem8_kernel_write(x >> 8);
    mem8_loc--;
}
void x86Internal::st16_mem8_kernel_write(int x)
{
    uint32_t mem8_locu  = mem8_loc;
    int      tlb_lookup = tlb_write_kernel[mem8_locu >> 12];
    if ((tlb_lookup | mem8_loc) & 1) {
        __st16_mem8_kernel_write(x);
    } else {
        phys_mem16[(mem8_loc ^ tlb_lookup) >> 1] = x;
    }
}
void x86Internal::__st32_mem8_kernel_write(int x)
{
    st8_mem8_kernel_write(x);
    mem8_loc++;
    st8_mem8_kernel_write(x >> 8);
    mem8_loc++;
    st8_mem8_kernel_write(x >> 16);
    mem8_loc++;
    st8_mem8_kernel_write(x >> 24);
    mem8_loc -= 3;
}
void x86Internal::st32_mem8_kernel_write(int x)
{
    uint32_t mem8_locu  = mem8_loc;
    int      tlb_lookup = tlb_write_kernel[mem8_locu >> 12];
    if ((tlb_lookup | mem8_loc) & 3) {
        __st32_mem8_kernel_write(x);
    } else {
        phys_mem32[(mem8_loc ^ tlb_lookup) >> 2] = x;
    }
}

void x86Internal::push_word_to_stack(int x)
{
    int wd;
    wd       = regs[4] - 2;
    mem8_loc = ((wd & SS_mask) + SS_base) >> 0;
    st16_mem8_write(x);
    regs[4] = (regs[4] & ~SS_mask) | ((wd)&SS_mask);
}
void x86Internal::push_dword_to_stack(int x)
{
    int wd;
    wd       = regs[4] - 4;
    mem8_loc = ((wd & SS_mask) + SS_base) >> 0;
    st32_mem8_write(x);
    regs[4] = (regs[4] & ~SS_mask) | ((wd)&SS_mask);
}
int x86Internal::pop_word_from_stack_read()
{
    mem8_loc = ((regs[4] & SS_mask) + SS_base) >> 0;
    return ld_16bits_mem8_read();
}
void x86Internal::pop_word_from_stack_incr_ptr()
{
    regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 2) & SS_mask);
}
int x86Internal::pop_dword_from_stack_read()
{
    mem8_loc = ((regs[4] & SS_mask) + SS_base) >> 0;
    return ld_32bits_mem8_read();
}
void x86Internal::pop_dword_from_stack_incr_ptr()
{
    regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 4) & SS_mask);
}
