
#ifndef _H_X86
#define _H_X86

#include <cstdio>
#include <fstream>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>
#include "CMOS.h"
#include "KBD.h"

using namespace std;
typedef struct DescriptorTable
{
    int selector = 0;
    int base     = 0;
    int limit    = 0;
    int flags    = 0;
} DescriptorTable;
typedef struct ErrorInfo
{
    int intno      = 0;
    int error_code = 0;
} ErrorInfo;

class PIT;
class Serial;
class PIC_Controller;
class x86 {

  public:
    bool   logcheck        = true;
    string filename        = "log.txt";
    int    filecheck_start = 0;
    int    filecheck_end   = 1000;
    int    fileoffset      = 0;
    bool   stepinfo        = false;

    int count = 0;

    // EAX, EBX, ECX, EDX, ESI, EDI, ESP, EBP  32bit registers
    int regs[8]{0};

    int cycle_count = 0;

    int cpl = 0;    // current privilege level

    int df = 1;    // Direction Flag

    int cr0 = (1 << 0);    // PE-mode ON
    int cr2 = 0;
    int cr3 = 0;
    int cr4 = 0;

    int  tlb_size         = 0x100000;
    int *tlb_read_kernel  = nullptr;
    int *tlb_write_kernel = nullptr;
    int *tlb_read_user    = nullptr;
    int *tlb_write_user   = nullptr;

    int halted     = 0;
    int hard_irq   = 0;
    int eflags     = 0x2;    // EFLAG register
    int hard_intno = -1;

    //   [" ES", " CS", " SS", " DS", " FS", " GS", "LDT", " TR"]
    DescriptorTable segs[7];
    DescriptorTable idt;

    int eip = 0;    // instruction pointer

    int cc_op   = 0;    // current op
    int cc_dst  = 0;    // current dest
    int cc_src  = 0;    // current src
    int cc_op2  = 0;    // current op, byte2
    int cc_dst2 = 0;    // current dest, byte2

    uint8_t  *phys_mem   = nullptr;
    uint8_t  *phys_mem8  = nullptr;
    uint16_t *phys_mem16 = nullptr;
    uint32_t *phys_mem32 = nullptr;

    DescriptorTable gdt;    // The Global Descriptor Table
    DescriptorTable ldt;    // The Local Descriptor Table
    DescriptorTable tr;

    const std::vector<int> parity_LUT = {
        1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1,
        0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0,
        0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0,
        1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1,
        0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1,
        0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1,
        1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1};
    const std::vector<int> shift16_LUT = {0,  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                                          16, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,  10, 11, 12, 13, 14};
    const std::vector<int> shift8_LUT  = {0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 1, 2, 3, 4, 5, 6,
                                          7, 8, 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 1, 2, 3, 4};

    int mem_size     = 16 * 1024 * 1024;
    int new_mem_size = mem_size + ((15 + 3) & ~3);

  private:
    int tlb_pages[2048]{0};
    int tlb_pages_count = 0;

  public:
    x86(int _mem_size);
    virtual ~x86();

    void load(uint8_t *bin, int offset, int size);
    void start(int start_addr, int initrd_size, int cmdline_addr);

    uint8_t ld8_phys(int mem8_loc);
    void    st8_phys(int mem8_loc, uint8_t x);
    int     ld32_phys(int mem8_loc);
    void    st32_phys(int mem8_loc, int x);
    void    write_string(int mem8_loc, string str);
    void    tlb_set_page(int mem8_loc, int page_val, int set_write_tlb, int set_user_tlb);

    void tlb_flush_page(int mem8_loc);
    void tlb_flush_all();
    void tlb_flush_all1(int la);
    void tlb_clear(int i);

    string hex_rep(int x, int n)
    {
        string s = "";
        int    i;
        char   h[] = "0123456789ABCDEF";
        s          = "";
        for (i = n - 1; i >= 0; i--) {
            s = s + h[(x >> (i * 4)) & 15];
        }
        return s;
    }
    string _4_bytes_(int n)
    {
        return hex_rep(n, 8);
    }
    string _2_bytes_(int n)
    {
        return hex_rep(n, 2);
    }
    string _1_byte_(int n)
    {
        return hex_rep(n, 4);
    }
};
class x86Internal : public x86 {

  public:
    x86Internal(int _mem_size);
    ~x86Internal();

    int  ioport_read(int mem8_loc);
    void ioport_write(int mem8_loc, int data);

    CMOS           *cmos     = nullptr;
    KBD            *kbd      = nullptr;
    PIC_Controller *pic      = nullptr;
    PIT            *pit      = nullptr;
    Serial         *serial   = nullptr;
    int             CS_flags = 0;

  private:
    int N_cycles    = 0;
    int cycles_left = 0;
    int mem8_loc;
    int last_tlb_val;
    int physmem8_ptr    = 0;
    int initial_mem_ptr = 0;
    int conditional_var = 0;
    int exit_code       = 256;

    int  CS_base, SS_base, iopl;
    bool FS_usage_flag = false;
    int  mem8, reg_idx0, reg_idx1, x, y, z, v;
    int  SS_mask       = -1;
    int  init_CS_flags = 0;

    int _src  = 0;
    int _dst  = 0;
    int _op   = 0;
    int _op2  = 0;
    int _dst2 = 0;

    int      *tlb_read, *tlb_write;
    ErrorInfo interrupt;

    int dpl = 0;

  public:
    int exec(int _N_cycles);
    int Instruction(int _N_cycles, ErrorInfo interrupt);

    int ld8_port(int port_num);
    int ld16_port(int port_num);
    int ld32_port(int port_num);

    void st8_port(int port_num, int x);
    void st16_port(int port_num, int x);
    void st32_port(int port_num, int x);

  private:
    void init_segment_local_vars();
    void check_interrupt();

    int __ld8_mem8_kernel_read();
    int ld8_mem8_kernel_read();
    int __ld16_mem8_kernel_read();
    int ld16_mem8_kernel_read();
    int __ld32_mem8_kernel_read();
    int ld32_mem8_kernel_read();
    int ld16_mem8_direct();

    int __ld_8bits_mem8_read();
    int ld_8bits_mem8_read();
    int __ld_16bits_mem8_read();
    int ld_16bits_mem8_read();
    int __ld_32bits_mem8_read();
    int ld_32bits_mem8_read();

    void do_tlb_set_page(int Gd, int Hd, bool ja);

    int __ld_8bits_mem8_write();
    int ld_8bits_mem8_write();
    int __ld_16bits_mem8_write();
    int ld_16bits_mem8_write();
    int __ld_32bits_mem8_write();
    int ld_32bits_mem8_write();

    void __st8_mem8_write(int x);
    void st8_mem8_write(int x);
    void __st16_mem8_write(int x);
    void st16_mem8_write(int x);
    void __st32_mem8_write(int x);
    void st32_mem8_write(int x);

    void __st8_mem8_kernel_write(int x);
    void st8_mem8_kernel_write(int x);
    void __st16_mem8_kernel_write(int x);
    void st16_mem8_kernel_write(int x);
    void __st32_mem8_kernel_write(int x);
    void st32_mem8_kernel_write(int x);

    int  segment_translation(int mem8);
    int  segmented_mem8_loc_for_MOV();
    void set_word_in_register(int reg_idx1, int x);
    void set_lower_word_in_register(int reg_idx1, int x);

    int do_32bit_math(int conditional_var, int Yb, int Zb);
    int do_16bit_math(int conditional_var, int Yb, int Zb);
    int do_8bit_math(int conditional_var, int Yb, int Zb);

    int increment_16bit(int x);
    int decrement_16bit(int x);
    int increment_8bit(int x);
    int decrement_8bit(int x);

    int shift8(int conditional_var, int Yb, int Zb);
    int shift16(int conditional_var, int Yb, int Zb);
    int shift32(int conditional_var, uint32_t Yb, int Zb);

    int  op_16_SHRD_SHLD(int conditional_var, int Yb, int Zb, int pc);
    int  op_SHLD(int Yb, int Zb, int pc);
    int  op_SHRD(int Yb, int Zb, int pc);
    void op_16_BT(int Yb, int Zb);
    void op_BT(int Yb, int Zb);
    int  op_16_BTS_BTR_BTC(int conditional_var, int Yb, int Zb);
    int  op_BTS_BTR_BTC(int conditional_var, int Yb, int Zb);
    int  op_16_BSF(int Yb, int Zb);
    int  op_BSF(int Yb, int Zb);
    int  op_16_BSR(int Yb, int Zb);
    int  op_BSR(int Yb, int Zb);
    void op_DIV(int OPbyte);
    void op_IDIV(int OPbyte);
    void op_16_DIV(int OPbyte);
    void op_16_IDIV(int OPbyte);
    int  op_DIV32(uint32_t Ic, uint32_t Jc, uint32_t OPbyte);
    int  op_IDIV32(int Ic, int Jc, int OPbyte);
    int  op_MUL(int a, int OPbyte);
    int  op_IMUL(int a, int OPbyte);
    int  op_16_MUL(int a, int OPbyte);
    int  op_16_IMUL(int a, int OPbyte);
    int  do_multiply32(int _a, int _OPbyte);
    int  op_MUL32(int a, int OPbyte);
    int  op_IMUL32(int a, int OPbyte);

    bool check_carry();
    bool check_overflow();
    bool check_below_or_equal();
    int  check_parity();
    int  check_less_than();
    int  check_less_or_equal();
    int  check_adjust_flag();
    int  check_status_bits_for_jump(int gd);
    int  conditional_flags_for_rot_shift_ops();
    int  get_conditional_flags();
    int  get_FLAGS();
    void set_FLAGS(int flag_bits, int ld);
    void abort_with_error_code(int intno, int error_code);
    void abort(int intno);
    void change_permission_level(int sd);
    int  do_tlb_lookup(int mem8_loc, int ud);

    void push_word_to_stack(int x);
    void push_dword_to_stack(int x);
    int  pop_word_from_stack_read();
    void pop_word_from_stack_incr_ptr();
    int  pop_dword_from_stack_read();
    void pop_dword_from_stack_incr_ptr();
    int  operation_size_function(int eip_offset, int OPbyte);

    void set_CR0(int Qd);
    void set_CR3(int new_pdb);
    void set_CR4(int newval);

    int  SS_mask_from_flags(int descriptor_high4bytes);
    void load_from_descriptor_table(int selector, int *desary);
    int  calculate_descriptor_limit(int descriptor_low4bytes, int descriptor_high4bytes);
    int  calculate_descriptor_base(int descriptor_low4bytes, int descriptor_high4bytes);
    void set_descriptor_register(DescriptorTable *descriptor_table, int descriptor_low4bytes,
                                 int descriptor_high4bytes);
    void set_segment_vars(int ee, int selector, int base, int limit, int flags);
    void init_segment_vars_with_selector(int Sb, int selector);

    void load_from_TR(int he, int *desary);
    void do_interrupt_protected_mode(int intno, int ne, int error_code, int oe, int pe);
    void do_interrupt_not_protected_mode(int intno, int ne, int error_code, int oe, int pe);
    void do_interrupt(int intno, int ne, int error_code, int oe, int pe);

    void op_LDTR(int selector);
    void op_LTR(int selector);
    void set_protected_mode_segment_register(int reg, int selector);
    void set_segment_register(int reg, int selector);
    void do_JMPF_virtual_mode(int selector, int Le);
    void do_JMPF(int selector, int Le);
    void op_JMPF(int selector, int Le);
    void Pe(int reg, int cpl_var);

    void op_CALLF_not_protected_mode(bool is_32_bit, int selector, int Le, int oe);
    void op_CALLF_protected_mode(bool is_32_bit, int selector, int Le, int oe);
    void op_CALLF(bool is_32_bit, int selector, int Le, int oe);
    void do_return_not_protected_mode(bool is_32_bit, bool is_iret, int imm16);
    void do_return_protected_mode(bool is_32_bit, bool is_iret, int imm16);

    void op_IRET(bool is_32_bit);
    void op_RETF(bool is_32_bit, int imm16);

    int  of(int selector, bool is_lsl);
    void op_LAR_LSL(bool is_32_bit, bool is_lsl);
    int  segment_isnt_accessible(int selector, bool is_verw);
    void op_VERR_VERW(int selector, bool is_verw);
    void op_ARPL();
    void op_CPUID();
    void op_AAM(int base);
    void op_AAD(int base);
    void op_AAA();
    void op_AAS();
    void op_DAA();
    void op_DAS();
    void checkOp_BOUND();
    void op_16_BOUND();
    void op_16_PUSHA();
    void op_PUSHA();
    void op_16_POPA();
    void op_POPA();
    void op_16_LEAVE();
    void op_LEAVE();
    void op_16_ENTER();
    void op_ENTER();

    void op_16_load_far_pointer32(int Sb);
    void op_16_load_far_pointer16(int Sb);

    void stringOp_INSB();
    void stringOp_OUTSB();
    void stringOp_MOVSB();
    void stringOp_STOSB();
    void stringOp_CMPSB();
    void stringOp_LODSB();
    void stringOp_SCASB();

    void op_16_INS();
    void op_16_OUTS();
    void op_16_MOVS();
    void op_16_STOS();
    void op_16_CMPS();
    void op_16_LODS();
    void op_16_SCAS();

    void stringOp_INSD();
    void stringOp_OUTSD();
    void stringOp_MOVSD();
    void stringOp_STOSD();
    void stringOp_CMPSD();
    void stringOp_LODSD();
    void stringOp_SCASD();

    vector<string> lines;
    void           cpu_dump(int OPbyte);
    int            file_read();

    int current_cycle_count()
    {
        return cycle_count + (N_cycles - cycles_left);
    }
    void cpu_abort(string str)
    {
        throw "CPU abort: " + str;
    }
};

//
//
//

class PIC {
    int  last_irr           = 0;
    int  irr                = 0;    // Interrupt Request Register
    int  imr                = 0;    // Interrupt Mask Register
    int  isr                = 0;    // In-Service Register
    int  priority_add       = 0;
    int  read_reg_select    = 0;
    int  special_mask       = 0;
    int  init_state         = 0;
    int  auto_eoi           = 0;
    int  rotate_on_autoeoi  = 0;
    int  init4              = 0;
    int  elcr               = 0;    // Edge/Level Control Register
    bool rotate_on_auto_eoi = false;

    PIC_Controller *ppic = nullptr;

  public:
    int elcr_mask = 0;
    int irq_base  = 0;

  public:
    PIC(PIC_Controller *_ppic)
    {
        ppic = _ppic;
        reset();
    }

    ~PIC()
    {
    }

    void init()
    {
    }

    void reset()
    {
        last_irr          = 0;
        irr               = 0;    // Interrupt Request Register
        imr               = 0;    // Interrupt Mask Register
        isr               = 0;    // In-Service Register
        priority_add      = 0;
        irq_base          = 0;
        read_reg_select   = 0;
        special_mask      = 0;
        init_state        = 0;
        auto_eoi          = 0;
        rotate_on_autoeoi = 0;
        init4             = 0;
        elcr              = 0;    // Edge/Level Control Register
        elcr_mask         = 0;
    }

    void set_irq1(int irq, bool Qf)
    {
        int ir_register = 1 << irq;
        if (Qf) {
            if ((last_irr & ir_register) == 0)
                irr |= ir_register;
            last_irr |= ir_register;
        } else {
            last_irr &= ~ir_register;
        }
    }

    int get_priority(int ir_register)
    {
        if (ir_register == 0)
            return -1;
        int priority = 7;
        while ((ir_register & (1 << ((priority + priority_add) & 7))) == 0)
            priority--;
        return priority;
    }

    int get_irq()
    {
        int ir_register, in_service_priority, priority;
        ir_register = irr & ~imr;
        priority    = get_priority(ir_register);
        if (priority < 0)
            return -1;
        in_service_priority = get_priority(isr);
        if (priority > in_service_priority) {
            return priority;
        } else {
            return -1;
        }
    }

    void intack(int irq)
    {
        if (auto_eoi) {
            if (rotate_on_auto_eoi)
                priority_add = (irq + 1) & 7;
        } else {
            isr |= (1 << irq);
        }
        if (!(elcr & (1 << irq)))
            irr &= ~(1 << irq);
    }

    void ioport_write(int mem8_loc, int x)
    {
        int priority;
        mem8_loc &= 1;
        if (mem8_loc == 0) {
            if (x & 0x10) {
                reset();
                init_state = 1;
                init4      = x & 1;
                if (x & 0x02)
                    throw "single mode not supported";
                if (x & 0x08)
                    throw "level sensitive irq not supported";
            } else if (x & 0x08) {
                if (x & 0x02)
                    read_reg_select = x & 1;
                if (x & 0x40)
                    special_mask = (x >> 5) & 1;
            } else {
                switch (x) {
                    case 0x00:
                    case 0x80:
                        rotate_on_autoeoi = x >> 7;
                        break;
                    case 0x20:
                    case 0xa0:
                        priority = get_priority(isr);
                        if (priority >= 0) {
                            isr &= ~(1 << ((priority + priority_add) & 7));
                        }
                        if (x == 0xa0)
                            priority_add = (priority_add + 1) & 7;
                        break;
                    case 0x60:
                    case 0x61:
                    case 0x62:
                    case 0x63:
                    case 0x64:
                    case 0x65:
                    case 0x66:
                    case 0x67:
                        priority = x & 7;
                        isr &= ~(1 << priority);
                        break;
                    case 0xc0:
                    case 0xc1:
                    case 0xc2:
                    case 0xc3:
                    case 0xc4:
                    case 0xc5:
                    case 0xc6:
                    case 0xc7:
                        priority_add = (x + 1) & 7;
                        break;
                    case 0xe0:
                    case 0xe1:
                    case 0xe2:
                    case 0xe3:
                    case 0xe4:
                    case 0xe5:
                    case 0xe6:
                    case 0xe7:
                        priority = x & 7;
                        isr &= ~(1 << priority);
                        priority_add = (priority + 1) & 7;
                        break;
                }
            }
        } else {
            switch (init_state) {
                case 0:
                    imr = x;
                    update_irq();
                    break;
                case 1:
                    irq_base   = x & 0xf8;
                    init_state = 2;
                    break;
                case 2:
                    if (init4) {
                        init_state = 3;
                    } else {
                        init_state = 0;
                    }
                    break;
                case 3:
                    auto_eoi   = (x >> 1) & 1;
                    init_state = 0;
                    break;
            }
        }
    }

    int ioport_read(int Ug)
    {
        int mem8_loc, return_register;
        mem8_loc = Ug & 1;
        if (mem8_loc == 0) {
            if (read_reg_select)
                return_register = isr;
            else
                return_register = irr;
        } else {
            return_register = imr;
        }
        return return_register;
    }
    void update_irq();
};
class PIC_Controller {
    int          irq_requested = 0;
    x86Internal *cpu;

  public:
    PIC *pics[2];

  public:
    PIC_Controller(x86Internal *_cpu)
    {
        cpu     = _cpu;
        pics[0] = new PIC(this);
        pics[1] = new PIC(this);

        pics[0]->elcr_mask = 0xf8;
        pics[1]->elcr_mask = 0xde;
    }
    ~PIC_Controller()
    {
        delete pics[0];
        delete pics[1];
    }

    void set_irq(int irq, int Qf)
    {
        pics[irq >> 3]->set_irq1(irq & 7, Qf);
        update_irq();
    }

    int get_hard_intno()
    {
        int intno = 0;
        int irq   = pics[0]->get_irq();
        if (irq >= 0) {
            pics[0]->intack(irq);
            if (irq == 2) {
                int slave_irq = pics[1]->get_irq();
                if (slave_irq >= 0) {
                    pics[1]->intack(slave_irq);
                } else {
                    slave_irq = 7;
                }
                intno = pics[1]->irq_base + slave_irq;
                irq   = slave_irq + 8;
            } else {
                intno = pics[0]->irq_base + irq;
            }
        } else {
            irq   = 7;
            intno = pics[0]->irq_base + irq;
        }
        update_irq();
        return intno;
    }

    void update_irq();
};
inline void PIC_Controller::update_irq()
{
    int slave_irq = pics[1]->get_irq();
    if (slave_irq >= 0) {
        pics[0]->set_irq1(2, 1);
        pics[0]->set_irq1(2, 0);
    }
    int irq = pics[0]->get_irq();
    if (irq >= 0) {
        cpu->hard_irq = 1;
    } else {
        cpu->hard_irq = 0;
    }
}
inline void PIC::update_irq()
{
    ppic->update_irq();
}
class Serial {

    int         divider = 0;
    int         rbr     = 0;
    int         ier     = 0;
    int         iir     = 0x01;
    int         lcr     = 0;
    int         mcr;
    int         lsr          = 0x40 | 0x20;
    int         msr          = 0;
    int         scr          = 0;
    int         set_irq_func = 0;
    int         write_func   = 0;
    std::string receive_fifo = "";

    PIC_Controller *pic;

  public:
    Serial(PIC_Controller *_pic, int kh, int lh)
    {
        pic          = _pic;
        set_irq_func = kh;
        write_func   = lh;
    }

    void update_irq()
    {
        if ((lsr & 0x01) && (ier & 0x01)) {
            iir = 0x04;
        } else if ((lsr & 0x20) && (ier & 0x02)) {
            iir = 0x02;
        } else {
            iir = 0x01;
        }
        if (iir != 0x01) {
            set_irq(1);
        } else {
            set_irq(0);
        }
    }

    void ioport_write(int mem8_loc, int x)
    {
        mem8_loc &= 7;
        switch (mem8_loc) {
            default:
            case 0:
                if (lcr & 0x80) {
                    divider = (divider & 0xff00) | x;
                } else {
                    lsr &= ~0x20;
                    update_irq();
                    // write_func(String.fromCharCode(x));
                    printf("%c", x);
                    lsr |= 0x20;
                    lsr |= 0x40;
                    update_irq();
                }
                break;
            case 1:
                if (lcr & 0x80) {
                    divider = (divider & 0x00ff) | (x << 8);
                } else {
                    ier = x;
                    update_irq();
                }
                break;
            case 2:
                break;
            case 3:
                lcr = x;
                break;
            case 4:
                mcr = x;
                break;
            case 5:
                break;
            case 6:
                msr = x;
                break;
            case 7:
                scr = x;
                break;
        }
    }

    int ioport_read(int mem8_loc)
    {
        int Pg;
        mem8_loc &= 7;
        switch (mem8_loc) {
            default:
            case 0:
                if (lcr & 0x80) {
                    Pg = divider & 0xff;
                } else {
                    Pg = rbr;
                    lsr &= ~(0x01 | 0x10);
                    update_irq();
                    // send_char_from_fifo();
                }
                break;
            case 1:
                if (lcr & 0x80) {
                    Pg = (divider >> 8) & 0xff;
                } else {
                    Pg = ier;
                }
                break;
            case 2:
                Pg = iir;
                break;
            case 3:
                Pg = lcr;
                break;
            case 4:
                Pg = mcr;
                break;
            case 5:
                Pg = lsr;
                break;
            case 6:
                Pg = msr;
                break;
            case 7:
                Pg = scr;
                break;
        }
        return Pg;
    }

    void send_break()
    {
        rbr = 0;
        lsr |= 0x10 | 0x01;
        update_irq();
    }

    void send_char(int mh)
    {
        rbr = mh;
        lsr |= 0x01;
        update_irq();
    }

    void send_char_from_fifo()
    {
        std::string nh = receive_fifo;
        if (nh != "" && !(lsr & 0x01)) {
            // send_char(nh.charCodeAt(0));
            // receive_fifo = nh.substr(1, nh.length - 1);
        }
    }

    void send_chars(std::string na)
    {
        receive_fifo += na;
        send_char_from_fifo();
    }

    void set_irq(int x);
};
inline void Serial::set_irq(int x)
{
    pic->set_irq(4, x);
}
class IRQCH {
  public:
    int last_irr        = 0;
    int count           = 0;
    int latched_count   = 0;
    int rw_state        = 0;
    int mode            = 0;
    int bcd             = 0;
    int gate            = 0;
    int count_load_time = 0;
    // float        pit_time_unit   = 0.596591;
    x86Internal *cpu;

  public:
    IRQCH(x86Internal *_cpu)
    {
        cpu = _cpu;
    }

    int get_time()
    {
        return std::floor(cpu->cycle_count * 0.596591);
    }

    int pit_get_count()
    {
        int d, dh;
        d = get_time() - count_load_time;
        switch (mode) {
            case 0:
            case 1:
            case 4:
            case 5:
                dh = (count - d) & 0xffff;
                break;
            default:
                dh = count - (d % count);
                break;
        }
        return dh;
    }

    int pit_get_out()
    {
        int d, eh;
        d = get_time() - count_load_time;
        switch (mode) {
            default:
            case 0:    // Interrupt on terminal count
                eh = (d >= count) >> 0;
                break;
            case 1:    // One shot
                eh = (d < count) >> 0;
                break;
            case 2:    // Frequency divider
                if ((d % count) == 0 && d != 0)
                    eh = 1;
                else
                    eh = 0;
                break;
            case 3:    // Square wave
                eh = ((d % count) < (count >> 1)) >> 0;
                break;
            case 4:    // SW strobe
            case 5:    // HW strobe
                eh = (d == count) >> 0;
                break;
        }
        return eh;
    }

    int get_next_transition_time()
    {
        int d, fh, base, gh;
        d = get_time() - count_load_time;
        switch (mode) {
            default:
            case 0:    // Interrupt on terminal count
            case 1:    // One shot
                if (d < count)
                    fh = count;
                else
                    return -1;
                break;
            case 2:    // Frequency divider
                base = (d / count) * count;
                if ((d - base) == 0 && d != 0)
                    fh = base + count;
                else
                    fh = base + count + 1;
                break;
            case 3:    // Square wave
                base = (d / count) * count;
                gh   = ((count + 1) >> 1);
                if ((d - base) < gh)
                    fh = base + gh;
                else
                    fh = base + count;
                break;
            case 4:    // SW strobe
            case 5:    // HW strobe
                if (d < count)
                    fh = count;
                else if (d == count)
                    fh = count + 1;
                else
                    return -1;
                break;
        }

        fh = count_load_time + fh;
        return fh;
    }

    void pit_load_count(int x)
    {
        if (x == 0) {
            x = 0x10000;
        }
        count_load_time = get_time();
        count           = x;
    }
};
class PIT {

    IRQCH          *pit_channels[3];
    x86Internal    *cpu;
    PIC_Controller *pic;
    int             speaker_data_on = 0;

  public:
    PIT(x86Internal *_cpu, PIC_Controller *_pic)
    {
        cpu = _cpu;
        pic = _pic;

        for (int i = 0; i < 3; i++) {
            pit_channels[i]       = new IRQCH(cpu);
            pit_channels[i]->mode = 3;
            pit_channels[i]->gate = (i != 2) >> 0;
            pit_channels[i]->pit_load_count(0);
        }
        // set_irq         = set_irq_callback;
    }

    void ioport_write(int mem8_loc, int x)
    {
        int hh, ih;
        mem8_loc &= 3;
        if (mem8_loc == 3) {
            hh = x >> 6;
            if (hh == 3)
                return;

            auto s = pit_channels[hh];
            ih     = (x >> 4) & 3;
            switch (ih) {
                case 0: {
                    int val          = s->pit_get_count();
                    s->latched_count = val;
                    s->rw_state      = 4;
                } break;
                default:
                    s->mode     = (x >> 1) & 7;
                    s->bcd      = x & 1;
                    s->rw_state = ih - 1 + 0;
                    break;
            }
        } else {
            auto s = pit_channels[mem8_loc];
            switch (s->rw_state) {
                case 0:
                    s->pit_load_count(x);
                    break;
                case 1:
                    s->pit_load_count(x << 8);
                    break;
                case 2:
                case 3:
                    if (s->rw_state & 1) {
                        s->pit_load_count((s->latched_count & 0xff) | (x << 8));
                    } else {
                        s->latched_count = x;
                    }
                    s->rw_state ^= 1;
                    break;
            }
        }
    }

    int ioport_read(int mem8_loc)
    {
        int Pg, ma;
        mem8_loc &= 3;
        auto s = pit_channels[mem8_loc];
        switch (s->rw_state) {
            case 0:
            case 1:
            case 2:
            case 3:
                ma = s->pit_get_count();
                if (s->rw_state & 1)
                    Pg = (ma >> 8) & 0xff;
                else
                    Pg = ma & 0xff;
                if (s->rw_state & 2)
                    s->rw_state ^= 1;
                break;
            default:
            case 4:
            case 5:
                if (s->rw_state & 1)
                    Pg = s->latched_count >> 8;
                else
                    Pg = s->latched_count & 0xff;
                s->rw_state ^= 1;
                break;
        }
        return Pg;
    }

    void speaker_ioport_write(int mem8_loc, int x)
    {
        speaker_data_on       = (x >> 1) & 1;
        pit_channels[2]->gate = x & 1;
    }

    int speaker_ioport_read(int mem8_loc)
    {
        int  eh, x;
        auto s = pit_channels[2];
        eh     = s->pit_get_out();
        x      = (speaker_data_on << 1) | s->gate | (eh << 5);
        return x;
    }

    void speaker_ioport_write()
    {
        set_irq(1);
        set_irq(0);
    }

    void set_irq(int x);
    void update_irq();
};
inline void PIT::set_irq(int x)
{
    pic->set_irq(0, x);
}
inline void PIT::update_irq()
{
    set_irq(1);
    set_irq(0);
}

#endif
