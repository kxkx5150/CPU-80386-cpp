#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <thread>
#include "x86.h"

x86::x86(int _mem_size)
{
    mem_size     = _mem_size;
    new_mem_size = mem_size + ((15 + 3) & ~3);

    this->segs[2].flags = (1 << 22);
    this->segs[1].flags = (1 << 22);

    phys_mem = (uint8_t *)malloc(sizeof(uint8_t) * new_mem_size);
    for (int i = 0; i < new_mem_size; i++) {
        phys_mem[i] = 0;
    }
    phys_mem8  = phys_mem;
    phys_mem16 = reinterpret_cast<uint16_t *>(phys_mem8);
    phys_mem32 = reinterpret_cast<uint32_t *>(phys_mem8);

    tlb_read_kernel  = new int[tlb_size];
    tlb_write_kernel = new int[tlb_size];
    tlb_read_user    = new int[tlb_size];
    tlb_write_user   = new int[tlb_size];

    for (int i = 0; i < tlb_size; i++) {
        tlb_clear(i);
    }
}
x86::~x86()
{
    free(phys_mem8);
    delete[] tlb_read_kernel;
    delete[] tlb_write_kernel;
    delete[] tlb_read_user;
    delete[] tlb_write_user;
}
void x86::load(uint8_t *bin, int offset, int size)
{
    printf("x86::load %x\n", offset);
    for (int i = 0; i < size; i++) {
        st8_phys(offset + i, bin[i]);
    }
}
void x86::start(int start_addr, int initrd_size, int cmdline_addr)
{
    eip     = start_addr;
    regs[0] = mem_size;
    regs[3] = initrd_size;
    regs[1] = cmdline_addr;
}
x86Internal::x86Internal(int _mem_size) : x86(_mem_size)
{
    cmos   = new CMOS();
    kbd    = new KBD();
    pic    = new PIC_Controller(this);
    serial = new Serial(pic, 0, 0);
    pit    = new PIT(this, pic);
    file_read();
}
x86Internal::~x86Internal()
{
    delete cmos;
    delete kbd;
    delete pic;
    delete pit;
    delete serial;
}
int x86Internal::file_read()
{
    logcheck = false;
    stepinfo = false;
    filename = "linux_boot_logs/log16.txt";

    if (logcheck) {
        std::string   line;
        std::ifstream input_file(filename);
        if (!input_file.is_open()) {
            logcheck = false;
            stepinfo = false;
        }

        while (getline(input_file, line)) {
            lines.push_back(line);
        }

        input_file.close();

        if (filename == "linux_boot_logs/log0.txt") {
            filecheck_start = 1;
            filecheck_end   = 1000000;
        } else if (filename == "linux_boot_logs/log1.txt") {
            filecheck_start = 1000001;
            filecheck_end   = 2000000;
            fileoffset      = 1000000;
        } else if (filename == "linux_boot_logs/log2.txt") {
            filecheck_start = 2000001;
            filecheck_end   = 3000000;
            fileoffset      = 2000000;
        } else if (filename == "linux_boot_logs/log3.txt") {
            filecheck_start = 3000001;
            filecheck_end   = 4000000;
            fileoffset      = 3000000;
        } else if (filename == "linux_boot_logs/log4.txt") {
            filecheck_start = 4000001;
            filecheck_end   = 5000000;
            fileoffset      = 4000000;
        } else if (filename == "linux_boot_logs/log5.txt") {
            filecheck_start = 5000001;
            filecheck_end   = 6000000;
            fileoffset      = 5000000;
        } else if (filename == "linux_boot_logs/log6.txt") {
            filecheck_start = 6000001;
            filecheck_end   = 7000000;
            fileoffset      = 6000000;
        } else if (filename == "linux_boot_logs/log7.txt") {
            filecheck_start = 7000001;
            filecheck_end   = 8000000;
            fileoffset      = 7000000;
        } else if (filename == "linux_boot_logs/log8.txt") {
            filecheck_start = 8000001;
            filecheck_end   = 9000000;
            fileoffset      = 8000000;
        } else if (filename == "linux_boot_logs/log9.txt") {
            filecheck_start = 9000001;
            filecheck_end   = 10000000;
            fileoffset      = 9000000;
        } else if (filename == "linux_boot_logs/log10.txt") {
            filecheck_start = 10000001;
            filecheck_end   = 11000000;
            fileoffset      = 10000000;
        } else if (filename == "linux_boot_logs/log11.txt") {
            filecheck_start = 11000001;
            filecheck_end   = 12000000;
            fileoffset      = 11000000;
        } else if (filename == "linux_boot_logs/log12.txt") {
            filecheck_start = 12000001;
            filecheck_end   = 13000000;
            fileoffset      = 12000000;
        } else if (filename == "linux_boot_logs/log13.txt") {
            filecheck_start = 13000001;
            filecheck_end   = 14000000;
            fileoffset      = 13000000;
        } else if (filename == "linux_boot_logs/log14.txt") {
            filecheck_start = 14000001;
            filecheck_end   = 15000000;
            fileoffset      = 14000000;
        } else if (filename == "linux_boot_logs/log15.txt") {
            filecheck_start = 15000001;
            filecheck_end   = 16000000;
            fileoffset      = 15000000;
        } else if (filename == "linux_boot_logs/log16.txt") {
            filecheck_start = 16000001;
            filecheck_end   = 16765000;
            fileoffset      = 16000000;
        }
    }
    return EXIT_SUCCESS;
}
void x86Internal::dump(int OPbyte)
{
    count++;

    if (logcheck && filecheck_start <= count && count <= filecheck_end) {
        std::vector<std::string> ta = {"ES", "CS", "SS", "DS", "FS", "GS", "LDT", "TR"};
        char                     buf1[1000];
        sprintf(buf1, "STEPS=%d OPCODE=%d", count, OPbyte);
        // printf("%s", buf1);
        char buf2[1000];
        sprintf(buf2, "EIP=%08X EAX=%08X ECX=%08X EDX=%08X EBX=%08X EFL=%08X ESP=%08X EBP=%08X ESI=%08X EDI=%08X", eip,
                regs[0], regs[1], regs[2], regs[3], eflags, regs[4], regs[5], regs[6], regs[7]);
        // printf("%s", buf2);
        char buf3[1000];
        sprintf(buf3, "TSC=%08X OP=%02X OP2=%02X SRC=%08X DST=%08X DST2=%08X", cycle_count, cc_op, cc_op2, cc_src,
                cc_dst, cc_dst2);
        // printf("%s", buf3);
        char buf4[1000];
        sprintf(buf4, "CPL=%d CR0=%08X CR2=%08X CR3=%08X CR4=%08X", cpl, cr0, cr2, cr3, cr4);
        // printf("%s", buf4);
        char buf5[1000];
        sprintf(buf5, "hard_irq=%d", hard_irq);

        if (stepinfo) {
            printf("\n");
            printf("count : %d\n", count);
            printf("%s\n", lines[count - 1 - fileoffset].c_str());
            printf("%s %s %s %s %s\n", buf1, buf2, buf3, buf4, buf5);
        }

        if (count < filecheck_end) {
            int len = lines[0].size() + 10;

            char *sbf = new char[len];
            sprintf(sbf, "%s", lines[count - 1 - fileoffset].c_str());
            std::string s = sbf;

            char *tbf = new char[len];
            sprintf(tbf, "%s %s %s %s %s", buf1, buf2, buf3, buf4, buf5);
            std::string t = tbf;

            if (std::equal(t.begin(), t.end(), s.begin())) {
                // printf("ok !\n");
            } else {
                printf("\n\n\n***************\n");
                printf("*** Error ! ***\n");
                printf("***************\n\n\n");

                printf("count : %d\n", count);
                printf("OK : %s\n", lines[count - 1 - fileoffset].c_str());
                printf("NG : %s %s %s %s %s\n\n", buf1, buf2, buf3, buf4, buf5);
                exit(1);
            }

            delete[] sbf;
            delete[] tbf;
        } else {
            printf("\n\n\n\n\n-- Compare OK ! ---\n");
            // printf("count : %d\n\n", count);
            printf("\n\n\n\n\n\n");
            exit(1);
        }
    } else {
        if (16765000 < count) {
            // printf("\n\n\n\n\n\n");
            // exit(1);
        }
    }
}
int x86Internal::exec(int N_cycles)
{
    size_t final_cycle_count = cycle_count + N_cycles;
    int    exit_code         = 256;
    interrupt.error_code     = 0;
    interrupt.intno          = 0;

    while (cycle_count < final_cycle_count) {
        try {
            exit_code = instruction(final_cycle_count - cycle_count, interrupt);
            if (exit_code != 256)
                break;
            interrupt.error_code = 0;
            interrupt.intno      = 0;
        } catch (ErrorInfo cpu_exception) {
            interrupt = cpu_exception;
        }
    }

    return exit_code;
}
int x86Internal::init(int _N_cycles)
{
    N_cycles        = _N_cycles;
    cycles_left     = N_cycles;
    CS_flags        = init_CS_flags;
    mem8_loc        = 0;
    last_tlb_val    = 0;
    physmem8_ptr    = 0;
    initial_mem_ptr = 0;
    conditional_var = 0;
    exit_code       = 256;

    change_permission_level(cpl);
    if (check_halted())
        return 257;
    init_segment_local_vars();
    check_interrupt();
    return 0;
}
void x86Internal::init_segment_local_vars()
{
    CS_base = segs[1].base;    // CS
    SS_base = segs[2].base;    // SS
    if (segs[2].flags & (1 << 22))
        SS_mask = -1;
    else
        SS_mask = 0xffff;
    FS_usage_flag = (((CS_base | SS_base | segs[3].base | segs[0].base) == 0) && SS_mask == -1);
    if (segs[1].flags & (1 << 22))
        init_CS_flags = 0;
    else
        init_CS_flags = 0x0100 | 0x0080;
}
void x86Internal::check_opbyte()
{
    eip                  = (eip + physmem8_ptr - initial_mem_ptr) >> 0;
    eip_offset           = (eip + CS_base) >> 0;
    uint32_t eip_offset2 = eip_offset;
    int64_t  eip_tlb_val = tlb_read[eip_offset2 >> 12];

    if (((eip_tlb_val | eip_offset) & 0xfff) >= (4096 - 15 + 1)) {
        if (eip_tlb_val == -1)
            do_tlb_set_page(eip_offset, 0, cpl == 3);

        eip_tlb_val     = tlb_read[eip_offset2 >> 12];
        initial_mem_ptr = physmem8_ptr = eip_offset ^ eip_tlb_val;
        OPbyte                         = phys_mem8[physmem8_ptr++];
        int Cg                         = eip_offset & 0xfff;

        if (Cg >= (4096 - 15 + 1)) {
            x = operation_size_function(eip_offset, OPbyte);
            if ((Cg + x) > 4096) {
                initial_mem_ptr = physmem8_ptr = mem_size;
                for (y = 0; y < x; y++) {
                    mem8_loc           = (eip_offset + y) >> 0;
                    uint32_t mem8_locu = mem8_loc;
                    phys_mem8[physmem8_ptr + y] =
                        (((last_tlb_val = tlb_read[mem8_locu >> 12]) == -1) ? __ld_8bits_mem8_read()
                                                                            : phys_mem8[mem8_loc ^ last_tlb_val]);
                }
                physmem8_ptr++;
            }
        }
    } else {
        initial_mem_ptr = physmem8_ptr = eip_offset ^ eip_tlb_val;
        OPbyte                         = phys_mem8[physmem8_ptr++];
    }
}
int x86Internal::check_halted()
{
    if (halted) {
        if (hard_irq != 0 && (eflags & 0x00000200)) {
            halted = 0;
        } else {
            return 257;
        }
    }
    return 0;
}
void x86Internal::check_interrupt()
{
    if (interrupt.intno != 0) {
        do_interrupt(interrupt.intno, 0, interrupt.error_code, 0, 0);
    }
    if (hard_intno >= 0) {
        do_interrupt(hard_intno, 0, 0, 0, 1);
        hard_intno = -1;
    }
    if (hard_irq != 0 && (eflags & 0x00000200)) {
        hard_intno = pic->get_hard_intno();
        do_interrupt(hard_intno, 0, 0, 0, 1);
        hard_intno = -1;
    }
}

void x86Internal::do_tlb_set_page(int Gd, int Hd, bool ja)
{
    int Id, Jd, error_code, Kd, Ld, Md, Nd, ud, Od;
    if (!(cr0 &
          (1 << 31))) {    // CR0: bit31 PG Paging If 1, enable paging and use the CR3 register, else disable paging
        tlb_set_page(Gd & -4096, Gd & -4096, 1, 0);
    } else {
        Id = (cr3 & -4096) + ((Gd >> 20) & 0xffc);
        Jd = ld32_phys(Id);
        if (!(Jd & 0x00000001)) {
            error_code = 0;
        } else {
            if (!(Jd & 0x00000020)) {
                Jd |= 0x00000020;
                st32_phys(Id, Jd);
            }
            Kd = (Jd & -4096) + ((Gd >> 10) & 0xffc);
            Ld = ld32_phys(Kd);
            if (!(Ld & 0x00000001)) {
                error_code = 0;
            } else {
                Md = Ld & Jd;
                if (ja && !(Md & 0x00000004)) {
                    error_code = 0x01;
                } else if (Hd && !(Md & 0x00000002)) {
                    error_code = 0x01;
                } else {
                    Nd = (Hd && !(Ld & 0x00000040));
                    if (!(Ld & 0x00000020) || Nd) {
                        Ld |= 0x00000020;
                        if (Nd)
                            Ld |= 0x00000040;
                        st32_phys(Kd, Ld);
                    }
                    ud = 0;
                    if ((Ld & 0x00000040) && (Md & 0x00000002))
                        ud = 1;
                    Od = 0;
                    if (Md & 0x00000004)
                        Od = 1;
                    tlb_set_page(Gd & -4096, Ld & -4096, ud, Od);
                    return;
                }
            }
        }
        error_code |= Hd << 1;
        if (ja)
            error_code |= 0x04;
        cr2 = Gd;
        abort_with_error_code(14, error_code);
    }
}

int x86Internal::segment_translation(int mem8)
{
    int base, mem8_loc, Qb, Rb, Sb, Tb;
    if (FS_usage_flag && (CS_flags & (0x000f | 0x0080)) == 0) {
        switch ((mem8 & 7) | ((mem8 >> 3) & 0x18)) {
            case 0x04:
                Qb   = phys_mem8[physmem8_ptr++];
                base = Qb & 7;
                if (base == 5) {
                    {
                        mem8_loc = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) |
                                   (phys_mem8[physmem8_ptr + 2] << 16) | (phys_mem8[physmem8_ptr + 3] << 24);
                        physmem8_ptr += 4;
                    }
                } else {
                    mem8_loc = regs[base];
                }
                Rb = (Qb >> 3) & 7;
                if (Rb != 4) {
                    mem8_loc = (mem8_loc + (regs[Rb] << (Qb >> 6))) >> 0;
                }
                break;
            case 0x0c:
                Qb       = phys_mem8[physmem8_ptr++];
                mem8_loc = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                base     = Qb & 7;
                mem8_loc = (mem8_loc + regs[base]) >> 0;
                Rb       = (Qb >> 3) & 7;
                if (Rb != 4) {
                    mem8_loc = (mem8_loc + (regs[Rb] << (Qb >> 6))) >> 0;
                }
                break;
            case 0x14:
                Qb = phys_mem8[physmem8_ptr++];
                {
                    mem8_loc = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) |
                               (phys_mem8[physmem8_ptr + 2] << 16) | (phys_mem8[physmem8_ptr + 3] << 24);
                    physmem8_ptr += 4;
                }
                base     = Qb & 7;
                mem8_loc = (mem8_loc + regs[base]) >> 0;
                Rb       = (Qb >> 3) & 7;
                if (Rb != 4) {
                    mem8_loc = (mem8_loc + (regs[Rb] << (Qb >> 6))) >> 0;
                }
                break;
            case 0x05: {
                mem8_loc = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) |
                           (phys_mem8[physmem8_ptr + 2] << 16) | (phys_mem8[physmem8_ptr + 3] << 24);
                physmem8_ptr += 4;
            } break;
            case 0x00:
            case 0x01:
            case 0x02:
            case 0x03:
            case 0x06:
            case 0x07:
                base     = mem8 & 7;
                mem8_loc = regs[base];
                break;
            case 0x08:
            case 0x09:
            case 0x0a:
            case 0x0b:
            case 0x0d:
            case 0x0e:
            case 0x0f:
                mem8_loc = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                base     = mem8 & 7;
                mem8_loc = (mem8_loc + regs[base]) >> 0;
                break;
            case 0x10:
            case 0x11:
            case 0x12:
            case 0x13:
            case 0x15:
            case 0x16:
            case 0x17:
            default: {
                mem8_loc = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) |
                           (phys_mem8[physmem8_ptr + 2] << 16) | (phys_mem8[physmem8_ptr + 3] << 24);
                physmem8_ptr += 4;

                base     = mem8 & 7;
                mem8_loc = (mem8_loc + regs[base]) >> 0;
            } break;
        }
        return mem8_loc;
    } else if (CS_flags & 0x0080) {
        if ((mem8 & 0xc7) == 0x06) {
            mem8_loc = ld16_mem8_direct();
            Tb       = 3;
        } else {
            switch (mem8 >> 6) {
                case 0:
                    mem8_loc = 0;
                    break;
                case 1:
                    mem8_loc = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                    break;
                default:
                    mem8_loc = ld16_mem8_direct();
                    break;
            }
            switch (mem8 & 7) {
                case 0:
                    mem8_loc = (mem8_loc + regs[3] + regs[6]) & 0xffff;
                    Tb       = 3;
                    break;
                case 1:
                    mem8_loc = (mem8_loc + regs[3] + regs[7]) & 0xffff;
                    Tb       = 3;
                    break;
                case 2:
                    mem8_loc = (mem8_loc + regs[5] + regs[6]) & 0xffff;
                    Tb       = 2;
                    break;
                case 3:
                    mem8_loc = (mem8_loc + regs[5] + regs[7]) & 0xffff;
                    Tb       = 2;
                    break;
                case 4:
                    mem8_loc = (mem8_loc + regs[6]) & 0xffff;
                    Tb       = 3;
                    break;
                case 5:
                    mem8_loc = (mem8_loc + regs[7]) & 0xffff;
                    Tb       = 3;
                    break;
                case 6:
                    mem8_loc = (mem8_loc + regs[5]) & 0xffff;
                    Tb       = 2;
                    break;
                case 7:
                default:
                    mem8_loc = (mem8_loc + regs[3]) & 0xffff;
                    Tb       = 3;
                    break;
            }
        }
        Sb = CS_flags & 0x000f;
        if (Sb == 0) {
            Sb = Tb;
        } else {
            Sb--;
        }
        mem8_loc = (mem8_loc + segs[Sb].base) >> 0;
        return mem8_loc;
    } else {
        switch ((mem8 & 7) | ((mem8 >> 3) & 0x18)) {
            case 0x04:
                Qb   = phys_mem8[physmem8_ptr++];
                base = Qb & 7;
                if (base == 5) {
                    {
                        mem8_loc = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) |
                                   (phys_mem8[physmem8_ptr + 2] << 16) | (phys_mem8[physmem8_ptr + 3] << 24);
                        physmem8_ptr += 4;
                    }
                    base = 0;
                } else {
                    mem8_loc = regs[base];
                }
                Rb = (Qb >> 3) & 7;
                if (Rb != 4) {
                    mem8_loc = (mem8_loc + (regs[Rb] << (Qb >> 6))) >> 0;
                }
                break;
            case 0x0c:
                Qb       = phys_mem8[physmem8_ptr++];
                mem8_loc = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                base     = Qb & 7;
                mem8_loc = (mem8_loc + regs[base]) >> 0;
                Rb       = (Qb >> 3) & 7;
                if (Rb != 4) {
                    mem8_loc = (mem8_loc + (regs[Rb] << (Qb >> 6))) >> 0;
                }
                break;
            case 0x14:
                Qb = phys_mem8[physmem8_ptr++];

                mem8_loc = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) |
                           (phys_mem8[physmem8_ptr + 2] << 16) | (phys_mem8[physmem8_ptr + 3] << 24);
                physmem8_ptr += 4;

                base     = Qb & 7;
                mem8_loc = (mem8_loc + regs[base]) >> 0;
                Rb       = (Qb >> 3) & 7;
                if (Rb != 4) {
                    mem8_loc = (mem8_loc + (regs[Rb] << (Qb >> 6))) >> 0;
                }
                break;
            case 0x05: {
                mem8_loc = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) |
                           (phys_mem8[physmem8_ptr + 2] << 16) | (phys_mem8[physmem8_ptr + 3] << 24);
                physmem8_ptr += 4;
            }
                base = 0;
                break;
            case 0x00:
            case 0x01:
            case 0x02:
            case 0x03:
            case 0x06:
            case 0x07:
                base     = mem8 & 7;
                mem8_loc = regs[base];
                break;
            case 0x08:
            case 0x09:
            case 0x0a:
            case 0x0b:
            case 0x0d:
            case 0x0e:
            case 0x0f:
                mem8_loc = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                base     = mem8 & 7;
                mem8_loc = (mem8_loc + regs[base]) >> 0;
                break;
            case 0x10:
            case 0x11:
            case 0x12:
            case 0x13:
            case 0x15:
            case 0x16:
            case 0x17:
            default: {
                mem8_loc = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) |
                           (phys_mem8[physmem8_ptr + 2] << 16) | (phys_mem8[physmem8_ptr + 3] << 24);
                physmem8_ptr += 4;

                base     = mem8 & 7;
                mem8_loc = (mem8_loc + regs[base]) >> 0;
            } break;
        }
        Sb = CS_flags & 0x000f;
        if (Sb == 0) {
            if (base == 4 || base == 5)
                Sb = 2;
            else
                Sb = 3;
        } else {
            Sb--;
        }
        mem8_loc = (mem8_loc + segs[Sb].base) >> 0;
        return mem8_loc;
    }

    return 0;
}
int x86Internal::segmented_mem8_loc_for_MOV()
{
    int mem8_loc, Sb;
    if (CS_flags & 0x0080) {
        mem8_loc = ld16_mem8_direct();
    } else {
        mem8_loc = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) | (phys_mem8[physmem8_ptr + 2] << 16) |
                   (phys_mem8[physmem8_ptr + 3] << 24);
        physmem8_ptr += 4;
    }
    Sb = CS_flags & 0x000f;
    if (Sb == 0)
        Sb = 3;
    else
        Sb--;
    mem8_loc = (mem8_loc + segs[Sb].base) >> 0;
    return mem8_loc;
}
void x86Internal::set_word_in_register(int reg_idx1, int x)
{
    if (reg_idx1 & 4)
        regs[reg_idx1 & 3] = (regs[reg_idx1 & 3] & -65281) | ((x & 0xff) << 8);
    else
        regs[reg_idx1 & 3] = (regs[reg_idx1 & 3] & -256) | (x & 0xff);
}
void x86Internal::set_lower_word_in_register(int reg_idx1, int x)
{
    regs[reg_idx1] = (regs[reg_idx1] & -65536) | (x & 0xffff);
}

int x86Internal::do_32bit_math(int conditional_var, int Yb, int Zb)
{
    int ac;
    switch (conditional_var) {
        case 0:
            cc_src = Zb;
            Yb     = (Yb + Zb) >> 0;
            cc_dst = Yb;
            cc_op  = 2;
            break;
        case 1:
            Yb     = Yb | Zb;
            cc_dst = Yb;
            cc_op  = 14;
            break;
        case 2:
            ac     = check_carry();
            cc_src = Zb;
            Yb     = (Yb + Zb + ac) >> 0;
            cc_dst = Yb;
            cc_op  = ac ? 5 : 2;
            break;
        case 3:
            ac     = check_carry();
            cc_src = Zb;
            Yb     = (Yb - Zb - ac) >> 0;
            cc_dst = Yb;
            cc_op  = ac ? 11 : 8;
            break;
        case 4:
            Yb     = Yb & Zb;
            cc_dst = Yb;
            cc_op  = 14;
            break;
        case 5:
            cc_src = Zb;
            Yb     = (Yb - Zb) >> 0;
            cc_dst = Yb;
            cc_op  = 8;
            break;
        case 6:
            Yb     = Yb ^ Zb;
            cc_dst = Yb;
            cc_op  = 14;
            break;
        case 7:
            cc_src = Zb;
            cc_dst = (Yb - Zb) >> 0;
            cc_op  = 8;
            break;
        default:
            throw "arith: invalid op";
    }
    return Yb;
}
int x86Internal::do_16bit_math(int conditional_var, int Yb, int Zb)
{
    int ac;
    switch (conditional_var) {
        case 0:
            cc_src = Zb;
            Yb     = (((Yb + Zb) << 16) >> 16);
            cc_dst = Yb;
            cc_op  = 1;
            break;
        case 1:
            Yb     = (((Yb | Zb) << 16) >> 16);
            cc_dst = Yb;
            cc_op  = 13;
            break;
        case 2:
            ac     = check_carry();
            cc_src = Zb;
            Yb     = (((Yb + Zb + ac) << 16) >> 16);
            cc_dst = Yb;
            cc_op  = ac ? 4 : 1;
            break;
        case 3:
            ac     = check_carry();
            cc_src = Zb;
            Yb     = (((Yb - Zb - ac) << 16) >> 16);
            cc_dst = Yb;
            cc_op  = ac ? 10 : 7;
            break;
        case 4:
            Yb     = (((Yb & Zb) << 16) >> 16);
            cc_dst = Yb;
            cc_op  = 13;
            break;
        case 5:
            cc_src = Zb;
            Yb     = (((Yb - Zb) << 16) >> 16);
            cc_dst = Yb;
            cc_op  = 7;
            break;
        case 6:
            Yb     = (((Yb ^ Zb) << 16) >> 16);
            cc_dst = Yb;
            cc_op  = 13;
            break;
        case 7:
            cc_src = Zb;
            cc_dst = (((Yb - Zb) << 16) >> 16);
            cc_op  = 7;
            break;
        default:
            throw "arith: invalid op";
    }
    return Yb;
}
int x86Internal::do_8bit_math(int conditional_var, int Yb, int Zb)
{
    int ac;
    switch (conditional_var) {
        case 0:
            cc_src = Zb;
            Yb     = (((Yb + Zb) << 24) >> 24);
            cc_dst = Yb;
            cc_op  = 0;
            break;
        case 1:
            Yb     = (((Yb | Zb) << 24) >> 24);
            cc_dst = Yb;
            cc_op  = 12;
            break;
        case 2:
            ac     = check_carry();
            cc_src = Zb;
            Yb     = (((Yb + Zb + ac) << 24) >> 24);
            cc_dst = Yb;
            cc_op  = ac ? 3 : 0;
            break;
        case 3:
            ac     = check_carry();
            cc_src = Zb;
            Yb     = (((Yb - Zb - ac) << 24) >> 24);
            cc_dst = Yb;
            cc_op  = ac ? 9 : 6;
            break;
        case 4:
            Yb     = (((Yb & Zb) << 24) >> 24);
            cc_dst = Yb;
            cc_op  = 12;
            break;
        case 5:
            cc_src = Zb;
            Yb     = (((Yb - Zb) << 24) >> 24);
            cc_dst = Yb;
            cc_op  = 6;
            break;
        case 6:
            Yb     = (((Yb ^ Zb) << 24) >> 24);
            cc_dst = Yb;
            cc_op  = 12;
            break;
        case 7:
            cc_src = Zb;
            cc_dst = (((Yb - Zb) << 24) >> 24);
            cc_op  = 6;
            break;
        default:
            throw "arith: invalid op";
    }
    return Yb;
}

int x86Internal::increment_16bit(int x)
{
    if (cc_op < 25) {
        cc_op2  = cc_op;
        cc_dst2 = cc_dst;
    }
    cc_dst = (((x + 1) << 16) >> 16);
    cc_op  = 26;
    return cc_dst;
}
int x86Internal::decrement_16bit(int x)
{
    if (cc_op < 25) {
        cc_op2  = cc_op;
        cc_dst2 = cc_dst;
    }
    cc_dst = (((x - 1) << 16) >> 16);
    cc_op  = 29;
    return cc_dst;
}
int x86Internal::increment_8bit(int x)
{
    if (cc_op < 25) {
        cc_op2  = cc_op;
        cc_dst2 = cc_dst;
    }
    cc_dst = (((x + 1) << 24) >> 24);
    cc_op  = 25;
    return cc_dst;
}
int x86Internal::decrement_8bit(int x)
{
    if (cc_op < 25) {
        cc_op2  = cc_op;
        cc_dst2 = cc_dst;
    }
    cc_dst = (((x - 1) << 24) >> 24);
    cc_op  = 28;
    return cc_dst;
}

int x86Internal::shift8(int conditional_var, int Yb, int Zb)
{
    int kc, ac;
    switch (conditional_var) {
        case 0:
            if (Zb & 0x1f) {
                Zb &= 0x7;
                Yb &= 0xff;
                kc     = Yb;
                Yb     = (Yb << Zb) | (Yb >> (8 - Zb));
                cc_src = conditional_flags_for_rot_shiftcc_ops();
                cc_src |= (Yb & 0x0001) | (((kc ^ Yb) << 4) & 0x0800);
                cc_dst = ((cc_src >> 6) & 1) ^ 1;
                cc_op  = 24;
            }
            break;
        case 1:
            if (Zb & 0x1f) {
                Zb &= 0x7;
                Yb &= 0xff;
                kc     = Yb;
                Yb     = (Yb >> Zb) | (Yb << (8 - Zb));
                cc_src = conditional_flags_for_rot_shiftcc_ops();
                cc_src |= ((Yb >> 7) & 0x0001) | (((kc ^ Yb) << 4) & 0x0800);
                cc_dst = ((cc_src >> 6) & 1) ^ 1;
                cc_op  = 24;
            }
            break;
        case 2:
            Zb = shift8_LUT[Zb & 0x1f];
            if (Zb) {
                Yb &= 0xff;
                kc = Yb;
                ac = check_carry();
                Yb = (Yb << Zb) | (ac << (Zb - 1));
                if (Zb > 1)
                    Yb |= kc >> (9 - Zb);
                cc_src = conditional_flags_for_rot_shiftcc_ops();
                cc_src |= (((kc ^ Yb) << 4) & 0x0800) | ((kc >> (8 - Zb)) & 0x0001);
                cc_dst = ((cc_src >> 6) & 1) ^ 1;
                cc_op  = 24;
            }
            break;
        case 3:
            Zb = shift8_LUT[Zb & 0x1f];
            if (Zb) {
                Yb &= 0xff;
                kc = Yb;
                ac = check_carry();
                Yb = (Yb >> Zb) | (ac << (8 - Zb));
                if (Zb > 1)
                    Yb |= kc << (9 - Zb);
                cc_src = conditional_flags_for_rot_shiftcc_ops();
                cc_src |= (((kc ^ Yb) << 4) & 0x0800) | ((kc >> (Zb - 1)) & 0x0001);
                cc_dst = ((cc_src >> 6) & 1) ^ 1;
                cc_op  = 24;
            }
            break;
        case 4:
        case 6:
            Zb &= 0x1f;
            if (Zb) {
                cc_src = Yb << (Zb - 1);
                cc_dst = Yb = (((Yb << Zb) << 24) >> 24);
                cc_op       = 15;
            }
            break;
        case 5:
            Zb &= 0x1f;
            if (Zb) {
                Yb &= 0xff;
                cc_src = Yb >> (Zb - 1);
                cc_dst = Yb = (((Yb >> Zb) << 24) >> 24);
                cc_op       = 18;
            }
            break;
        case 7:
            Zb &= 0x1f;
            if (Zb) {
                Yb     = (Yb << 24) >> 24;
                cc_src = Yb >> (Zb - 1);
                cc_dst = Yb = (((Yb >> Zb) << 24) >> 24);
                cc_op       = 18;
            }
            break;
        default:
            throw &"unsupported shift8="[conditional_var];
    }
    return Yb;
}
int x86Internal::shift16(int conditional_var, int Yb, int Zb)
{
    int kc, ac;
    switch (conditional_var) {
        case 0:
            if (Zb & 0x1f) {
                Zb &= 0xf;
                Yb &= 0xffff;
                kc     = Yb;
                Yb     = (Yb << Zb) | (Yb >> (16 - Zb));
                cc_src = conditional_flags_for_rot_shiftcc_ops();
                cc_src |= (Yb & 0x0001) | (((kc ^ Yb) >> 4) & 0x0800);
                cc_dst = ((cc_src >> 6) & 1) ^ 1;
                cc_op  = 24;
            }
            break;
        case 1:
            if (Zb & 0x1f) {
                Zb &= 0xf;
                Yb &= 0xffff;
                kc     = Yb;
                Yb     = (Yb >> Zb) | (Yb << (16 - Zb));
                cc_src = conditional_flags_for_rot_shiftcc_ops();
                cc_src |= ((Yb >> 15) & 0x0001) | (((kc ^ Yb) >> 4) & 0x0800);
                cc_dst = ((cc_src >> 6) & 1) ^ 1;
                cc_op  = 24;
            }
            break;
        case 2:
            Zb = shift16_LUT[Zb & 0x1f];
            if (Zb) {
                Yb &= 0xffff;
                kc = Yb;
                ac = check_carry();
                Yb = (Yb << Zb) | (ac << (Zb - 1));
                if (Zb > 1)
                    Yb |= kc >> (17 - Zb);
                cc_src = conditional_flags_for_rot_shiftcc_ops();
                cc_src |= (((kc ^ Yb) >> 4) & 0x0800) | ((kc >> (16 - Zb)) & 0x0001);
                cc_dst = ((cc_src >> 6) & 1) ^ 1;
                cc_op  = 24;
            }
            break;
        case 3:
            Zb = shift16_LUT[Zb & 0x1f];
            if (Zb) {
                Yb &= 0xffff;
                kc = Yb;
                ac = check_carry();
                Yb = (Yb >> Zb) | (ac << (16 - Zb));
                if (Zb > 1)
                    Yb |= kc << (17 - Zb);
                cc_src = conditional_flags_for_rot_shiftcc_ops();
                cc_src |= (((kc ^ Yb) >> 4) & 0x0800) | ((kc >> (Zb - 1)) & 0x0001);
                cc_dst = ((cc_src >> 6) & 1) ^ 1;
                cc_op  = 24;
            }
            break;
        case 4:
        case 6:
            Zb &= 0x1f;
            if (Zb) {
                cc_src = Yb << (Zb - 1);
                cc_dst = Yb = (((Yb << Zb) << 16) >> 16);
                cc_op       = 16;
            }
            break;
        case 5:
            Zb &= 0x1f;
            if (Zb) {
                Yb &= 0xffff;
                cc_src = Yb >> (Zb - 1);
                cc_dst = Yb = (((Yb >> Zb) << 16) >> 16);
                cc_op       = 19;
            }
            break;
        case 7:
            Zb &= 0x1f;
            if (Zb) {
                Yb     = (Yb << 16) >> 16;
                cc_src = Yb >> (Zb - 1);
                cc_dst = Yb = (((Yb >> Zb) << 16) >> 16);
                cc_op       = 19;
            }
            break;
        default:
            throw &"unsupported shift16="[conditional_var];
    }
    return Yb;
}
int x86Internal::shift32(int conditional_var, uint32_t Yb, int Zb)
{
    int kc, ac;
    switch (conditional_var) {
        case 0:
            Zb &= 0x1f;
            if (Zb) {
                kc     = Yb;
                Yb     = (Yb << Zb) | (Yb >> (32 - Zb));
                cc_src = conditional_flags_for_rot_shiftcc_ops();
                cc_src |= (Yb & 0x0001) | (((kc ^ Yb) >> 20) & 0x0800);
                cc_dst = ((cc_src >> 6) & 1) ^ 1;
                cc_op  = 24;
            }
            break;
        case 1:
            Zb &= 0x1f;
            if (Zb) {
                kc     = Yb;
                Yb     = (Yb >> Zb) | (Yb << (32 - Zb));
                cc_src = conditional_flags_for_rot_shiftcc_ops();
                cc_src |= ((Yb >> 31) & 0x0001) | (((kc ^ Yb) >> 20) & 0x0800);
                cc_dst = ((cc_src >> 6) & 1) ^ 1;
                cc_op  = 24;
            }
            break;
        case 2:
            Zb &= 0x1f;
            if (Zb) {
                kc = Yb;
                ac = check_carry();
                Yb = (Yb << Zb) | (ac << (Zb - 1));
                if (Zb > 1)
                    Yb |= kc >> (33 - Zb);
                cc_src = conditional_flags_for_rot_shiftcc_ops();
                cc_src |= (((kc ^ Yb) >> 20) & 0x0800) | ((kc >> (32 - Zb)) & 0x0001);
                cc_dst = ((cc_src >> 6) & 1) ^ 1;
                cc_op  = 24;
            }
            break;
        case 3:
            Zb &= 0x1f;
            if (Zb) {
                kc = Yb;
                ac = check_carry();
                Yb = (Yb >> Zb) | (ac << (32 - Zb));
                if (Zb > 1)
                    Yb |= kc << (33 - Zb);
                cc_src = conditional_flags_for_rot_shiftcc_ops();
                cc_src |= (((kc ^ Yb) >> 20) & 0x0800) | ((kc >> (Zb - 1)) & 0x0001);
                cc_dst = ((cc_src >> 6) & 1) ^ 1;
                cc_op  = 24;
            }
            break;
        case 4:
        case 6:
            Zb &= 0x1f;
            if (Zb) {
                cc_src = Yb << (Zb - 1);
                cc_dst = Yb = Yb << Zb;
                cc_op       = 17;
            }
            break;
        case 5:
            Zb &= 0x1f;
            if (Zb) {
                cc_src = Yb >> (Zb - 1);
                cc_dst = Yb = Yb >> Zb;
                cc_op       = 20;
            }
            break;
        case 7:
            Zb &= 0x1f;
            if (Zb) {
                int Ybi = Yb;
                cc_src  = Ybi >> (Zb - 1);
                cc_dst = Yb = Ybi >> Zb;
                cc_op       = 20;
            }
            break;
        default:
            throw &"unsupported shift32="[conditional_var];
    }
    return Yb;
}

int x86Internal::op_16_SHRD_SHLD(int conditional_var, int Yb, int Zb, int pc)
{
    bool flg;
    pc &= 0x1f;
    if (pc) {
        if (conditional_var == 0) {
            Zb &= 0xffff;
            flg    = Zb | (Yb << 16);
            cc_src = flg >> (32 - pc);
            flg <<= pc;
            if (pc > 16)
                flg |= Zb << (pc - 16);
            Yb = cc_dst = flg >> 16;
            cc_op       = 19;
        } else {
            flg    = (Yb & 0xffff) | (Zb << 16);
            cc_src = flg >> (pc - 1);
            flg >>= pc;
            if (pc > 16)
                flg |= Zb << (32 - pc);
            Yb = cc_dst = (((flg) << 16) >> 16);
            cc_op       = 19;
        }
    }
    return Yb;
}
int x86Internal::op_SHLD(int Yb, int Zb, int pc)
{
    pc &= 0x1f;
    if (pc) {
        cc_src        = Yb << (pc - 1);
        uint32_t Zbu  = Zb;
        uint32_t lval = (Yb << pc);
        uint32_t rval = (Zbu >> (32 - pc));
        cc_dst = Yb = lval | rval;
        cc_op       = 17;
    }
    return Yb;
}
int x86Internal::op_SHRD(int Yb, int Zb, int pc)
{
    pc &= 0x1f;
    if (pc) {
        cc_src        = Yb >> (pc - 1);
        uint32_t Zbu  = Zb;
        uint32_t Ybu  = Yb;
        uint32_t lval = (Ybu >> pc);
        uint32_t rval = (Zbu << (32 - pc));
        cc_dst = Yb = lval | rval;
        cc_op       = 20;
    }
    return Yb;
}
void x86Internal::op_16_BT(int Yb, int Zb)
{
    Zb &= 0xf;
    cc_src = Yb >> Zb;
    cc_op  = 19;
}
void x86Internal::op_BT(int Yb, int Zb)
{
    Zb &= 0x1f;
    cc_src = Yb >> Zb;
    cc_op  = 20;
}
int x86Internal::op_16_BTS_BTR_BTC(int conditional_var, int Yb, int Zb)
{
    int wc;
    Zb &= 0xf;
    cc_src = Yb >> Zb;
    wc     = 1 << Zb;
    switch (conditional_var) {
        case 1:
            Yb |= wc;
            break;
        case 2:
            Yb &= ~wc;
            break;
        case 3:
        default:
            Yb ^= wc;
            break;
    }
    cc_op = 19;
    return Yb;
}
int x86Internal::op_BTS_BTR_BTC(int conditional_var, int Yb, int Zb)
{
    int wc;
    Zb &= 0x1f;
    cc_src = Yb >> Zb;
    wc     = 1 << Zb;
    switch (conditional_var) {
        case 1:
            Yb |= wc;
            break;
        case 2:
            Yb &= ~wc;
            break;
        case 3:
        default:
            Yb ^= wc;
            break;
    }
    cc_op = 20;
    return Yb;
}
int x86Internal::op_16_BSF(int Yb, int Zb)
{
    Zb &= 0xffff;
    if (Zb) {
        Yb = 0;
        while ((Zb & 1) == 0) {
            Yb++;
            Zb >>= 1;
        }
        cc_dst = 1;
    } else {
        cc_dst = 0;
    }
    cc_op = 14;
    return Yb;
}
int x86Internal::op_BSF(int Yb, int Zb)
{
    if (Zb) {
        Yb = 0;
        while ((Zb & 1) == 0) {
            Yb++;
            Zb >>= 1;
        }
        cc_dst = 1;
    } else {
        cc_dst = 0;
    }
    cc_op = 14;
    return Yb;
}
int x86Internal::op_16_BSR(int Yb, int Zb)
{
    Zb &= 0xffff;
    if (Zb) {
        Yb = 15;
        while ((Zb & 0x8000) == 0) {
            Yb--;
            Zb <<= 1;
        }
        cc_dst = 1;
    } else {
        cc_dst = 0;
    }
    cc_op = 14;
    return Yb;
}
int x86Internal::op_BSR(int Yb, int Zb)
{
    if (Zb) {
        Yb = 31;
        while (Zb >= 0) {
            Yb--;
            Zb <<= 1;
        }
        cc_dst = 1;
    } else {
        cc_dst = 0;
    }
    cc_op = 14;
    return Yb;
}
void x86Internal::op_DIV(int OPbyte)
{
    int a, q, r;
    a = regs[0] & 0xffff;
    OPbyte &= 0xff;
    if ((a >> 8) >= OPbyte)
        abort(0);
    q = (a / OPbyte) >> 0;
    r = (a % OPbyte);
    set_lower_word_in_register(0, (q & 0xff) | (r << 8));
}
void x86Internal::op_IDIV(int OPbyte)
{
    int a, q, r;
    a      = (regs[0] << 16) >> 16;
    OPbyte = (OPbyte << 24) >> 24;
    if (OPbyte == 0)
        abort(0);
    q = (a / OPbyte) >> 0;
    if (((q << 24) >> 24) != q)
        abort(0);
    r = (a % OPbyte);
    set_lower_word_in_register(0, (q & 0xff) | (r << 8));
}
void x86Internal::op_16_DIV(int OPbyte)
{
    int a, q, r;
    a = (regs[2] << 16) | (regs[0] & 0xffff);
    OPbyte &= 0xffff;
    uint32_t au = a;
    if ((au >> 16) >= OPbyte)
        abort(0);
    q = (a / OPbyte) >> 0;
    r = (a % OPbyte);
    set_lower_word_in_register(0, q);
    set_lower_word_in_register(2, r);
}
void x86Internal::op_16_IDIV(int OPbyte)
{
    int a, q, r;
    a      = (regs[2] << 16) | (regs[0] & 0xffff);
    OPbyte = (OPbyte << 16) >> 16;
    if (OPbyte == 0)
        abort(0);
    q = (a / OPbyte) >> 0;
    if (((q << 16) >> 16) != q)
        abort(0);
    r = (a % OPbyte);
    set_lower_word_in_register(0, q);
    set_lower_word_in_register(2, r);
}
int x86Internal::op_DIV32(uint32_t Ic, uint32_t Jc, uint32_t OPbyte)
{
    uint64_t a;
    uint32_t i, Kc;
    Ic     = Ic >> 0;
    Jc     = Jc >> 0;
    OPbyte = OPbyte >> 0;
    if (Ic >= OPbyte) {
        abort(0);
    }
    if (Ic >= 0 && Ic <= 0x200000) {
        a = Ic * 4294967296 + Jc;
        v = (a % OPbyte) >> 0;
        return (a / OPbyte) >> 0;
    } else {
        for (i = 0; i < 32; i++) {
            Kc = Ic >> 31;
            Ic = ((Ic << 1) | (Jc >> 31)) >> 0;
            if (Kc || Ic >= OPbyte) {
                Ic = Ic - OPbyte;
                Jc = (Jc << 1) | 1;
            } else {
                Jc = Jc << 1;
            }
        }
        v = Ic >> 0;
        return Jc;
    }
}
int x86Internal::op_IDIV32(int Ic, int Jc, int OPbyte)
{
    int Mc, Nc, q;
    if (Ic < 0) {
        Mc = 1;
        Ic = ~Ic;
        Jc = (-Jc) >> 0;
        if (Jc == 0)
            Ic = (Ic + 1) >> 0;
    } else {
        Mc = 0;
    }
    if (OPbyte < 0) {
        OPbyte = (-OPbyte) >> 0;
        Nc     = 1;
    } else {
        Nc = 0;
    }
    q = op_DIV32(Ic, Jc, OPbyte);
    Nc ^= Mc;
    if (Nc) {
        if ((q >> 0) > 0x80000000)
            abort(0);
        q = (-q) >> 0;
    } else {
        if ((q >> 0) >= 0x80000000)
            abort(0);
    }
    if (Mc) {
        v = (-v) >> 0;
    }
    return q;
}
int x86Internal::op_MUL(int a, int OPbyte)
{
    bool flg;
    a &= 0xff;
    OPbyte &= 0xff;
    flg    = (regs[0] & 0xff) * (OPbyte & 0xff);
    cc_src = flg >> 8;
    cc_dst = (((flg) << 24) >> 24);
    cc_op  = 21;
    return flg;
}
int x86Internal::op_IMUL(int a, int OPbyte)
{
    bool flg;
    a      = (((a) << 24) >> 24);
    OPbyte = (((OPbyte) << 24) >> 24);
    flg    = (a * OPbyte) >> 0;
    cc_dst = (((flg) << 24) >> 24);
    cc_src = (flg != cc_dst) >> 0;
    cc_op  = 21;
    return flg;
}
int x86Internal::op_16_MUL(int a, int OPbyte)
{
    bool flg;
    flg    = ((a & 0xffff) * (OPbyte & 0xffff)) >> 0;
    cc_src = flg >> 16;
    cc_dst = (((flg) << 16) >> 16);
    cc_op  = 22;
    return flg;
}
int x86Internal::op_16_IMUL(int a, int OPbyte)
{
    bool flg;
    a      = (a << 16) >> 16;
    OPbyte = (OPbyte << 16) >> 16;
    flg    = (a * OPbyte) >> 0;
    cc_dst = (((flg) << 16) >> 16);
    cc_src = (flg != cc_dst) >> 0;
    cc_op  = 22;
    return flg;
}
int x86Internal::do_multiply32(int _a, int cc_opbyte)
{
    uint32_t Jc, Ic, Tc, Uc, m;
    uint64_t a      = _a;
    uint32_t au     = _a;
    uint32_t OPbyte = cc_opbyte;
    uint64_t r      = a * OPbyte;
    if (r <= 0xffffffff) {
        v = 0;
        r &= -1;
    } else {
        Jc = a & 0xffff;
        Ic = au >> 16;
        Tc = OPbyte & 0xffff;
        Uc = OPbyte >> 16;
        r  = Jc * Tc;
        v  = Ic * Uc;
        m  = Jc * Uc;
        r += (((m & 0xffff) << 16) >> 0);
        v += (m >> 16);
        if (r >= 4294967296) {
            r -= 4294967296;
            v++;
        }
        m = Ic * Tc;
        r += (((m & 0xffff) << 16) >> 0);
        v += (m >> 16);
        if (r >= 4294967296) {
            r -= 4294967296;
            v++;
        }
        r &= -1;
        v &= -1;
    }
    return r;
}
int x86Internal::op_MUL32(int a, int OPbyte)
{
    cc_dst = do_multiply32(a, OPbyte);
    cc_src = v;
    cc_op  = 23;
    return cc_dst;
}
int x86Internal::op_IMUL32(int a, int OPbyte)
{
    int s, r;
    s = 0;
    if (a < 0) {
        a = -a;
        s = 1;
    }
    if (OPbyte < 0) {
        OPbyte = -OPbyte;
        s ^= 1;
    }
    r = do_multiply32(a, OPbyte);
    if (s) {
        v = ~v;
        r = (-r) >> 0;
        if (r == 0) {
            v = (v + 1) >> 0;
        }
    }
    cc_dst = r;
    cc_src = (v - (r >> 31)) >> 0;
    cc_op  = 23;
    return r;
}

void x86Internal::change_permission_level(int sd)
{
    cpl = sd;
    if (cpl == 3) {
        tlb_read  = tlb_read_user;
        tlb_write = tlb_write_user;
    } else {
        tlb_read  = tlb_read_kernel;
        tlb_write = tlb_write_kernel;
    }
}
int x86Internal::do_tlb_lookup(int mem8_loc, int ud)
{
    int      tlb_lookup;
    uint32_t mem8_locu = mem8_loc;
    if (ud) {
        tlb_lookup = tlb_write[mem8_locu >> 12];
    } else {
        tlb_lookup = tlb_read[mem8_locu >> 12];
    }
    if (tlb_lookup == -1) {
        do_tlb_set_page(mem8_loc, ud, cpl == 3);
        if (ud) {
            tlb_lookup = tlb_write[mem8_locu >> 12];
        } else {
            tlb_lookup = tlb_read[mem8_locu >> 12];
        }
    }
    return tlb_lookup ^ mem8_loc;
}

int x86Internal::operation_size_function(int eip_offset, int OPbyte)
{
    int CS_flags, l, mem8, localcc_opbyte_var, base, conditional_var, stride;
    int n = 1;

    CS_flags = init_CS_flags;
    if (CS_flags & 0x0100)    // are we in 16bit compatibility mode?
        stride = 2;
    else
        stride = 4;

    while (true) {
        switch (OPbyte) {
            case 0x66:    //   Operand-size override prefix
                if (init_CS_flags & 0x0100) {
                    stride = 4;
                    CS_flags &= ~0x0100;
                } else {
                    stride = 2;
                    CS_flags |= 0x0100;
                }
            case 0xf0:    // LOCK   Assert LOCK# Signal Prefix
            case 0xf2:    // REPNZ  eCX Repeat String Operation Prefix
            case 0xf3:    // REPZ  eCX Repeat String Operation Prefix
            case 0x26:    // ES ES  ES segment override prefix
            case 0x2e:    // CS CS  CS segment override prefix
            case 0x36:    // SS SS  SS segment override prefix
            case 0x3e:    // DS DS  DS segment override prefix
            case 0x64:    // FS FS  FS segment override prefix
            case 0x65:    // GS GS  GS segment override prefix
            {
                if ((n + 1) > 15)
                    abort(6);
                mem8_loc           = (eip_offset + (n++)) >> 0;
                uint32_t mem8_locu = mem8_loc;
                OPbyte             = (((last_tlb_val = tlb_read[mem8_locu >> 12]) == -1) ? __ld_8bits_mem8_read()
                                                                                         : phys_mem8[mem8_loc ^ last_tlb_val]);
            } break;
            case 0x67:    //   Address-size override prefix
            {
                if (init_CS_flags & 0x0080) {
                    CS_flags &= ~0x0080;
                } else {
                    CS_flags |= 0x0080;
                }

                if ((n + 1) > 15)
                    abort(6);
                mem8_loc           = (eip_offset + (n++)) >> 0;
                uint32_t mem8_locu = mem8_loc;
                OPbyte             = (((last_tlb_val = tlb_read[mem8_locu >> 12]) == -1) ? __ld_8bits_mem8_read()
                                                                                         : phys_mem8[mem8_loc ^ last_tlb_val]);
            } break;
            case 0x91:
            case 0x92:
            case 0x93:
            case 0x94:
            case 0x95:
            case 0x96:
            case 0x97:
            case 0x40:    // INC  Zv Increment by 1
            case 0x41:    // REX.B   Extension of r/m field, base field, or opcode reg field
            case 0x42:    // REX.X   Extension of SIB index field
            case 0x43:    // REX.XB   REX.X and REX.B combination
            case 0x44:    // REX.R   Extension of ModR/M reg field
            case 0x45:    // REX.RB   REX.R and REX.B combination
            case 0x46:    // REX.RX   REX.R and REX.X combination
            case 0x47:    // REX.RXB   REX.R, REX.X and REX.B combination
            case 0x48:    // DEC  Zv Decrement by 1
            case 0x49:    // REX.WB   REX.W and REX.B combination
            case 0x4a:    // REX.WX   REX.W and REX.X combination
            case 0x4b:    // REX.WXB   REX.W, REX.X and REX.B combination
            case 0x4c:    // REX.WR   REX.W and REX.R combination
            case 0x4d:    // REX.WRB   REX.W, REX.R and REX.B combination
            case 0x4e:    // REX.WRX   REX.W, REX.R and REX.X combination
            case 0x4f:    // REX.WRXB   REX.W, REX.R, REX.X and REX.B combination
            case 0x50:    // PUSH Zv SS:[rSP] Push Word, Doubleword or Quadword Onto the Stack
            case 0x51:
            case 0x52:
            case 0x53:
            case 0x54:
            case 0x55:
            case 0x56:
            case 0x57:
            case 0x58:    // POP SS:[rSP] Zv Pop a Value from the Stack
            case 0x59:
            case 0x5a:
            case 0x5b:
            case 0x5c:
            case 0x5d:
            case 0x5e:
            case 0x5f:
            case 0x98:    // CBW AL AX Convert Byte to Word
            case 0x99:    // CWD AX DX Convert Word to Doubleword
            case 0xc9:    // LEAVE SS:[rSP] eBP High Level Procedure Exit
            case 0x9c:    // PUSHF Flags SS:[rSP] Push FLAGS Register onto the Stack
            case 0x9d:    // POPF SS:[rSP] Flags Pop Stack into FLAGS Register
            case 0x06:    // PUSH ES SS:[rSP] Push Word, Doubleword or Quadword Onto the Stack
            case 0x0e:    // PUSH CS SS:[rSP] Push Word, Doubleword or Quadword Onto the Stack
            case 0x16:    // PUSH SS SS:[rSP] Push Word, Doubleword or Quadword Onto the Stack
            case 0x1e:    // PUSH DS SS:[rSP] Push Word, Doubleword or Quadword Onto the Stack
            case 0x07:    // POP SS:[rSP] ES Pop a Value from the Stack
            case 0x17:    // POP SS:[rSP] SS Pop a Value from the Stack
            case 0x1f:    // POP SS:[rSP] DS Pop a Value from the Stack
            case 0xc3:    // RETN SS:[rSP]  Return from procedure
            case 0xcb:    // RETF SS:[rSP]  Return from procedure
            case 0x90:    // XCHG  Zvqp Exchange Register/Memory with Register
            case 0xcc:    // INT 3 SS:[rSP] Call to Interrupt Procedure
            case 0xce:    // INTO eFlags SS:[rSP] Call to Interrupt Procedure
            case 0xcf:    // IRET SS:[rSP] Flags Interrupt Return
            case 0xf5:    // CMC   Complement Carry Flag
            case 0xf8:    // CLC   Clear Carry Flag
            case 0xf9:    // STC   Set Carry Flag
            case 0xfc:    // CLD   Clear Direction Flag
            case 0xfd:    // STD   Set Direction Flag
            case 0xfa:    // CLI   Clear Interrupt Flag
            case 0xfb:    // STI   Set Interrupt Flag
            case 0x9e:    // SAHF AH  Store AH into Flags
            case 0x9f:    // LAHF  AH Load Status Flags into AH Register
            case 0xf4:    // HLT   Halt
            case 0xa4:    // MOVS (DS:)[rSI] (ES:)[rDI] Move Data from String to String
            case 0xa5:    // MOVS DS:[SI] ES:[DI] Move Data from String to String
            case 0xaa:    // STOS AL (ES:)[rDI] Store String
            case 0xab:    // STOS AX ES:[DI] Store String
            case 0xa6:    // CMPS (ES:)[rDI]  Compare String Operands
            case 0xa7:    // CMPS ES:[DI]  Compare String Operands
            case 0xac:    // LODS (DS:)[rSI] AL Load String
            case 0xad:    // LODS DS:[SI] AX Load String
            case 0xae:    // SCAS (ES:)[rDI]  Scan String
            case 0xaf:    // SCAS ES:[DI]  Scan String
            case 0x9b:    // FWAIT   Check pending unmasked floating-point exceptions
            case 0xec:    // IN DX AL Input from Port
            case 0xed:    // IN DX eAX Input from Port
            case 0xee:    // OUT AL DX Output to Port
            case 0xef:    // OUT eAX DX Output to Port
            case 0xd7:    // XLAT (DS:)[rBX+AL] AL Table Look-up Translation
            case 0x27:    // DAA  AL Decimal Adjust AL after Addition
            case 0x2f:    // DAS  AL Decimal Adjust AL after Subtraction
            case 0x37:    // AAA  AL ASCII Adjust After Addition
            case 0x3f:    // AAS  AL ASCII Adjust AL After Subtraction
            case 0x60:    // PUSHA AX SS:[rSP] Push All General-Purpose Registers
            case 0x61:    // POPA SS:[rSP] DI Pop All General-Purpose Registers
            case 0x6c:    // INS DX (ES:)[rDI] Input from Port to String
            case 0x6d:    // INS DX ES:[DI] Input from Port to String
            case 0x6e:    // OUTS (DS):[rSI] DX Output String to Port
            case 0x6f:    // OUTS DS:[SI] DX Output String to Port
                goto EXEC_LOOP;
            case 0xb0:    // MOV Ib Zb Move
            case 0xb1:
            case 0xb2:
            case 0xb3:
            case 0xb4:
            case 0xb5:
            case 0xb6:
            case 0xb7:
            case 0x04:    // ADD Ib AL Add
            case 0x0c:    // OR Ib AL Logical Inclusive OR
            case 0x14:    // ADC Ib AL Add with Carry
            case 0x1c:    // SBB Ib AL Integer Subtraction with Borrow
            case 0x24:    // AND Ib AL Logical AND
            case 0x2c:    // SUB Ib AL Subtract
            case 0x34:    // XOR Ib AL Logical Exclusive OR
            case 0x3c:    // CMP AL  Compare Two Operands
            case 0xa8:    // TEST AL  Logical Compare
            case 0x6a:    // PUSH Ibss SS:[rSP] Push Word, Doubleword or Quadword Onto the Stack
            case 0xeb:    // JMP Jbs  Jump
            case 0x70:    // JO Jbs  Jump short if overflow (OF=1)
            case 0x71:    // JNO Jbs  Jump short if not overflow (OF=0)
            case 0x72:    // JB Jbs  Jump short if below/not above or equal/carry (CF=1)
            case 0x73:    // JNB Jbs  Jump short if not below/above or equal/not carry (CF=0)
            case 0x76:    // JBE Jbs  Jump short if below or equal/not above (CF=1 AND ZF=1)
            case 0x77:    // JNBE Jbs  Jump short if not below or equal/above (CF=0 AND ZF=0)
            case 0x78:    // JS Jbs  Jump short if sign (SF=1)
            case 0x79:    // JNS Jbs  Jump short if not sign (SF=0)
            case 0x7a:    // JP Jbs  Jump short if parity/parity even (PF=1)
            case 0x7b:    // JNP Jbs  Jump short if not parity/parity odd
            case 0x7c:    // JL Jbs  Jump short if less/not greater (SF!=OF)
            case 0x7d:    // JNL Jbs  Jump short if not less/greater or equal (SF=OF)
            case 0x7e:    // JLE Jbs  Jump short if less or equal/not greater ((ZF=1) OR (SF!=OF))
            case 0x7f:    // JNLE Jbs  Jump short if not less nor equal/greater ((ZF=0) AND (SF=OF))
            case 0x74:    // JZ Jbs  Jump short if zero/equal (ZF=0)
            case 0x75:    // JNZ Jbs  Jump short if not zero/not equal (ZF=1)
            case 0xe0:    // LOOPNZ Jbs eCX Decrement count; Jump short if count!=0 and ZF=0
            case 0xe1:    // LOOPZ Jbs eCX Decrement count; Jump short if count!=0 and ZF=1
            case 0xe2:    // LOOP Jbs eCX Decrement count; Jump short if count!=0
            case 0xe3:    // JCXZ Jbs  Jump short if eCX register is 0
            case 0xcd:    // INT Ib SS:[rSP] Call to Interrupt Procedure
            case 0xe4:    // IN Ib AL Input from Port
            case 0xe5:    // IN Ib eAX Input from Port
            case 0xe6:    // OUT AL Ib Output to Port
            case 0xe7:    // OUT eAX Ib Output to Port
            case 0xd4:    // AAM  AL ASCII Adjust AX After Multiply
            case 0xd5:    // AAD  AL ASCII Adjust AX Before Division
                n++;
                if (n > 15)
                    abort(6);
                goto EXEC_LOOP;
            case 0xb8:    // MOV Ivqp Zvqp Move
            case 0xb9:
            case 0xba:
            case 0xbb:
            case 0xbc:
            case 0xbd:
            case 0xbe:
            case 0xbf:
            case 0x05:    // ADD Ivds rAX Add
            case 0x0d:    // OR Ivds rAX Logical Inclusive OR
            case 0x15:    // ADC Ivds rAX Add with Carry
            case 0x1d:    // SBB Ivds rAX Integer Subtraction with Borrow
            case 0x25:    // AND Ivds rAX Logical AND
            case 0x2d:    // SUB Ivds rAX Subtract
            case 0x35:    // XOR Ivds rAX Logical Exclusive OR
            case 0x3d:    // CMP rAX  Compare Two Operands
            case 0xa9:    // TEST rAX  Logical Compare
            case 0x68:    // PUSH Ivs SS:[rSP] Push Word, Doubleword or Quadword Onto the Stack
            case 0xe9:    // JMP Jvds  Jump
            case 0xe8:    // CALL Jvds SS:[rSP] Call Procedure
                n += stride;
                if (n > 15)
                    abort(6);
                goto EXEC_LOOP;
            case 0x88:    // MOV Gb Eb Move
            case 0x89:    // MOV Gvqp Evqp Move
            case 0x8a:    // MOV Eb Gb Move
            case 0x8b:    // MOV Evqp Gvqp Move
            case 0x86:    // XCHG  Gb Exchange Register/Memory with Register
            case 0x87:    // XCHG  Gvqp Exchange Register/Memory with Register
            case 0x8e:    // MOV Ew Sw Move
            case 0x8c:    // MOV Sw Mw Move
            case 0xc4:    // LES Mp ES Load Far Pointer
            case 0xc5:    // LDS Mp DS Load Far Pointer
            case 0x00:    // ADD Gb Eb Add
            case 0x08:    // OR Gb Eb Logical Inclusive OR
            case 0x10:    // ADC Gb Eb Add with Carry
            case 0x18:    // SBB Gb Eb Integer Subtraction with Borrow
            case 0x20:    // AND Gb Eb Logical AND
            case 0x28:    // SUB Gb Eb Subtract
            case 0x30:    // XOR Gb Eb Logical Exclusive OR
            case 0x38:    // CMP Eb  Compare Two Operands
            case 0x01:    // ADD Gvqp Evqp Add
            case 0x09:    // OR Gvqp Evqp Logical Inclusive OR
            case 0x11:    // ADC Gvqp Evqp Add with Carry
            case 0x19:    // SBB Gvqp Evqp Integer Subtraction with Borrow
            case 0x21:    // AND Gvqp Evqp Logical AND
            case 0x29:    // SUB Gvqp Evqp Subtract
            case 0x31:    // XOR Gvqp Evqp Logical Exclusive OR
            case 0x39:    // CMP Evqp  Compare Two Operands
            case 0x02:    // ADD Eb Gb Add
            case 0x0a:    // OR Eb Gb Logical Inclusive OR
            case 0x12:    // ADC Eb Gb Add with Carry
            case 0x1a:    // SBB Eb Gb Integer Subtraction with Borrow
            case 0x22:    // AND Eb Gb Logical AND
            case 0x2a:    // SUB Eb Gb Subtract
            case 0x32:    // XOR Eb Gb Logical Exclusive OR
            case 0x3a:    // CMP Gb  Compare Two Operands
            case 0x03:    // ADD Evqp Gvqp Add
            case 0x0b:    // OR Evqp Gvqp Logical Inclusive OR
            case 0x13:    // ADC Evqp Gvqp Add with Carry
            case 0x1b:    // SBB Evqp Gvqp Integer Subtraction with Borrow
            case 0x23:    // AND Evqp Gvqp Logical AND
            case 0x2b:    // SUB Evqp Gvqp Subtract
            case 0x33:    // XOR Evqp Gvqp Logical Exclusive OR
            case 0x3b:    // CMP Gvqp  Compare Two Operands
            case 0x84:    // TEST Eb  Logical Compare
            case 0x85:    // TEST Evqp  Logical Compare
            case 0xd0:    // ROL 1 Eb Rotate
            case 0xd1:    // ROL 1 Evqp Rotate
            case 0xd2:    // ROL CL Eb Rotate
            case 0xd3:    // ROL CL Evqp Rotate
            case 0x8f:    // POP SS:[rSP] Ev Pop a Value from the Stack
            case 0x8d:    // LEA M Gvqp Load Effective Address
            case 0xfe:    // INC  Eb Increment by 1
            case 0xff:    // INC  Evqp Increment by 1
            case 0xd8:    // FADD Msr ST Add
            case 0xd9:    // FLD ESsr ST Load Floating Point Value
            case 0xda:    // FIADD Mdi ST Add
            case 0xdb:    // FILD Mdi ST Load Integer
            case 0xdc:    // FADD Mdr ST Add
            case 0xdd:    // FLD Mdr ST Load Floating Point Value
            case 0xde:    // FIADD Mwi ST Add
            case 0xdf:    // FILD Mwi ST Load Integer
            case 0x62:    // BOUND Gv SS:[rSP] Check Array Index Against Bounds
            case 0x63:    // ARPL Ew  Adjust RPL Field of Segment Selector
            {

                if ((n + 1) > 15)
                    abort(6);
                mem8_loc           = (eip_offset + (n++)) >> 0;
                uint32_t mem8_locu = mem8_loc;
                mem8               = (((last_tlb_val = tlb_read[mem8_locu >> 12]) == -1) ? __ld_8bits_mem8_read()
                                                                                         : phys_mem8[mem8_loc ^ last_tlb_val]);

                if (CS_flags & 0x0080) {
                    switch (mem8 >> 6) {
                        case 0:
                            if ((mem8 & 7) == 6)
                                n += 2;
                            break;
                        case 1:
                            n++;
                            break;
                        default:
                            n += 2;
                            break;
                    }
                } else {
                    switch ((mem8 & 7) | ((mem8 >> 3) & 0x18)) {
                        case 0x04: {
                            if ((n + 1) > 15)
                                abort(6);
                            mem8_loc           = (eip_offset + (n++)) >> 0;
                            uint32_t mem8_locu = mem8_loc;
                            localcc_opbyte_var = (((last_tlb_val = tlb_read[mem8_locu >> 12]) == -1)
                                                      ? __ld_8bits_mem8_read()
                                                      : phys_mem8[mem8_loc ^ last_tlb_val]);

                            if ((localcc_opbyte_var & 7) == 5) {
                                n += 4;
                            }
                        } break;
                        case 0x0c:
                            n += 2;
                            break;
                        case 0x14:
                            n += 5;
                            break;
                        case 0x05:
                            n += 4;
                            break;
                        case 0x00:
                        case 0x01:
                        case 0x02:
                        case 0x03:
                        case 0x06:
                        case 0x07:
                            break;
                        case 0x08:
                        case 0x09:
                        case 0x0a:
                        case 0x0b:
                        case 0x0d:
                        case 0x0e:
                        case 0x0f:
                            n++;
                            break;
                        case 0x10:
                        case 0x11:
                        case 0x12:
                        case 0x13:
                        case 0x15:
                        case 0x16:
                        case 0x17:
                            n += 4;
                            break;
                    }
                }
                if (n > 15)
                    abort(6);
            }
                goto EXEC_LOOP;
            case 0xa0:    // MOV Ob AL Move
            case 0xa1:    // MOV Ovqp rAX Move
            case 0xa2:    // MOV AL Ob Move
            case 0xa3:    // MOV rAX Ovqp Move
                if (CS_flags & 0x0100)
                    n += 2;
                else
                    n += 4;
                if (n > 15)
                    abort(6);
                goto EXEC_LOOP;
            case 0xc6:    // MOV Ib Eb Move
            case 0x80:    // ADD Ib Eb Add
            case 0x82:    // ADD Ib Eb Add
            case 0x83:    // ADD Ibs Evqp Add
            case 0x6b:    // IMUL Evqp Gvqp Signed Multiply
            case 0xc0:    // ROL Ib Eb Rotate
            case 0xc1:    // ROL Ib Evqp Rotate
            {

                if ((n + 1) > 15)
                    abort(6);
                mem8_loc           = (eip_offset + (n++)) >> 0;
                uint32_t mem8_locu = mem8_loc;
                mem8               = (((last_tlb_val = tlb_read[mem8_locu >> 12]) == -1) ? __ld_8bits_mem8_read()
                                                                                         : phys_mem8[mem8_loc ^ last_tlb_val]);

                if (CS_flags & 0x0080) {
                    switch (mem8 >> 6) {
                        case 0:
                            if ((mem8 & 7) == 6)
                                n += 2;
                            break;
                        case 1:
                            n++;
                            break;
                        default:
                            n += 2;
                            break;
                    }
                } else {
                    switch ((mem8 & 7) | ((mem8 >> 3) & 0x18)) {
                        case 0x04: {
                            if ((n + 1) > 15)
                                abort(6);
                            mem8_loc           = (eip_offset + (n++)) >> 0;
                            uint32_t mem8_locu = mem8_loc;
                            localcc_opbyte_var = (((last_tlb_val = tlb_read[mem8_locu >> 12]) == -1)
                                                      ? __ld_8bits_mem8_read()
                                                      : phys_mem8[mem8_loc ^ last_tlb_val]);
                        }
                            if ((localcc_opbyte_var & 7) == 5) {
                                n += 4;
                            }
                            break;
                        case 0x0c:
                            n += 2;
                            break;
                        case 0x14:
                            n += 5;
                            break;
                        case 0x05:
                            n += 4;
                            break;
                        case 0x00:
                        case 0x01:
                        case 0x02:
                        case 0x03:
                        case 0x06:
                        case 0x07:
                            break;
                        case 0x08:
                        case 0x09:
                        case 0x0a:
                        case 0x0b:
                        case 0x0d:
                        case 0x0e:
                        case 0x0f:
                            n++;
                            break;
                        case 0x10:
                        case 0x11:
                        case 0x12:
                        case 0x13:
                        case 0x15:
                        case 0x16:
                        case 0x17:
                            n += 4;
                            break;
                    }
                }
                if (n > 15)
                    abort(6);
            }
                n++;
                if (n > 15)
                    abort(6);
                goto EXEC_LOOP;
            case 0xc7:    // MOV Ivds Evqp Move
            case 0x81:    // ADD Ivds Evqp Add
            case 0x69:    // IMUL Evqp Gvqp Signed Multiply
            {

                if ((n + 1) > 15)
                    abort(6);
                mem8_loc = (eip_offset + (n++)) >> 0;
                mem8     = (((last_tlb_val = tlb_read[mem8_loc >> 12]) == -1) ? __ld_8bits_mem8_read()
                                                                              : phys_mem8[mem8_loc ^ last_tlb_val]);

                if (CS_flags & 0x0080) {
                    switch (mem8 >> 6) {
                        case 0:
                            if ((mem8 & 7) == 6)
                                n += 2;
                            break;
                        case 1:
                            n++;
                            break;
                        default:
                            n += 2;
                            break;
                    }
                } else {
                    switch ((mem8 & 7) | ((mem8 >> 3) & 0x18)) {
                        case 0x04: {
                            if ((n + 1) > 15)
                                abort(6);
                            mem8_loc           = (eip_offset + (n++)) >> 0;
                            localcc_opbyte_var = (((last_tlb_val = tlb_read[mem8_loc >> 12]) == -1)
                                                      ? __ld_8bits_mem8_read()
                                                      : phys_mem8[mem8_loc ^ last_tlb_val]);
                        }
                            if ((localcc_opbyte_var & 7) == 5) {
                                n += 4;
                            }
                            break;
                        case 0x0c:
                            n += 2;
                            break;
                        case 0x14:
                            n += 5;
                            break;
                        case 0x05:
                            n += 4;
                            break;
                        case 0x00:
                        case 0x01:
                        case 0x02:
                        case 0x03:
                        case 0x06:
                        case 0x07:
                            break;
                        case 0x08:
                        case 0x09:
                        case 0x0a:
                        case 0x0b:
                        case 0x0d:
                        case 0x0e:
                        case 0x0f:
                            n++;
                            break;
                        case 0x10:
                        case 0x11:
                        case 0x12:
                        case 0x13:
                        case 0x15:
                        case 0x16:
                        case 0x17:
                            n += 4;
                            break;
                    }
                }
                if (n > 15)
                    abort(6);
            }
                n += stride;
                if (n > 15)
                    abort(6);
                goto EXEC_LOOP;
            case 0xf6:    // TEST Eb  Logical Compare
            {

                if ((n + 1) > 15)
                    abort(6);
                mem8_loc           = (eip_offset + (n++)) >> 0;
                uint32_t mem8_locu = mem8_loc;
                mem8               = (((last_tlb_val = tlb_read[mem8_locu >> 12]) == -1) ? __ld_8bits_mem8_read()
                                                                                         : phys_mem8[mem8_loc ^ last_tlb_val]);

                if (CS_flags & 0x0080) {
                    switch (mem8 >> 6) {
                        case 0:
                            if ((mem8 & 7) == 6)
                                n += 2;
                            break;
                        case 1:
                            n++;
                            break;
                        default:
                            n += 2;
                            break;
                    }
                } else {
                    switch ((mem8 & 7) | ((mem8 >> 3) & 0x18)) {
                        case 0x04: {
                            if ((n + 1) > 15)
                                abort(6);
                            mem8_loc           = (eip_offset + (n++)) >> 0;
                            uint32_t mem8_locu = mem8_loc;
                            localcc_opbyte_var = (((last_tlb_val = tlb_read[mem8_locu >> 12]) == -1)
                                                      ? __ld_8bits_mem8_read()
                                                      : phys_mem8[mem8_loc ^ last_tlb_val]);

                            if ((localcc_opbyte_var & 7) == 5) {
                                n += 4;
                            }
                        } break;
                        case 0x0c:
                            n += 2;
                            break;
                        case 0x14:
                            n += 5;
                            break;
                        case 0x05:
                            n += 4;
                            break;
                        case 0x00:
                        case 0x01:
                        case 0x02:
                        case 0x03:
                        case 0x06:
                        case 0x07:
                            break;
                        case 0x08:
                        case 0x09:
                        case 0x0a:
                        case 0x0b:
                        case 0x0d:
                        case 0x0e:
                        case 0x0f:
                            n++;
                            break;
                        case 0x10:
                        case 0x11:
                        case 0x12:
                        case 0x13:
                        case 0x15:
                        case 0x16:
                        case 0x17:
                            n += 4;
                            break;
                    }
                }
                if (n > 15)
                    abort(6);
            }
                conditional_var = (mem8 >> 3) & 7;
                if (conditional_var == 0) {
                    n++;
                    if (n > 15)
                        abort(6);
                }
                goto EXEC_LOOP;
            case 0xf7:    // TEST Evqp  Logical Compare
            {

                if ((n + 1) > 15)
                    abort(6);
                mem8_loc           = (eip_offset + (n++)) >> 0;
                uint32_t mem8_locu = mem8_loc;
                mem8               = (((last_tlb_val = tlb_read[mem8_locu >> 12]) == -1) ? __ld_8bits_mem8_read()
                                                                                         : phys_mem8[mem8_loc ^ last_tlb_val]);

                if (CS_flags & 0x0080) {
                    switch (mem8 >> 6) {
                        case 0:
                            if ((mem8 & 7) == 6)
                                n += 2;
                            break;
                        case 1:
                            n++;
                            break;
                        default:
                            n += 2;
                            break;
                    }
                } else {
                    switch ((mem8 & 7) | ((mem8 >> 3) & 0x18)) {
                        case 0x04: {
                            if ((n + 1) > 15)
                                abort(6);
                            mem8_loc           = (eip_offset + (n++)) >> 0;
                            uint32_t mem8_locu = mem8_loc;
                            localcc_opbyte_var = (((last_tlb_val = tlb_read[mem8_locu >> 12]) == -1)
                                                      ? __ld_8bits_mem8_read()
                                                      : phys_mem8[mem8_loc ^ last_tlb_val]);

                            if ((localcc_opbyte_var & 7) == 5) {
                                n += 4;
                            }
                        } break;
                        case 0x0c:
                            n += 2;
                            break;
                        case 0x14:
                            n += 5;
                            break;
                        case 0x05:
                            n += 4;
                            break;
                        case 0x00:
                        case 0x01:
                        case 0x02:
                        case 0x03:
                        case 0x06:
                        case 0x07:
                            break;
                        case 0x08:
                        case 0x09:
                        case 0x0a:
                        case 0x0b:
                        case 0x0d:
                        case 0x0e:
                        case 0x0f:
                            n++;
                            break;
                        case 0x10:
                        case 0x11:
                        case 0x12:
                        case 0x13:
                        case 0x15:
                        case 0x16:
                        case 0x17:
                            n += 4;
                            break;
                    }
                }
                if (n > 15)
                    abort(6);
            }
                conditional_var = (mem8 >> 3) & 7;
                if (conditional_var == 0) {
                    n += stride;
                    if (n > 15)
                        abort(6);
                }
                goto EXEC_LOOP;
            case 0xea:    // JMPF Ap  Jump
            case 0x9a:    // CALLF Ap SS:[rSP] Call Procedure
                n += 2 + stride;
                if (n > 15)
                    abort(6);
                goto EXEC_LOOP;
            case 0xc2:    // RETN SS:[rSP]  Return from procedure
            case 0xca:    // RETF Iw  Return from procedure
                n += 2;
                if (n > 15)
                    abort(6);
                goto EXEC_LOOP;
            case 0xc8:    // ENTER Iw SS:[rSP] Make Stack Frame for Procedure Parameters
                n += 3;
                if (n > 15)
                    abort(6);
                goto EXEC_LOOP;
            case 0xd6:    // SALC   Undefined and Reserved; Does not Generate #UD
            case 0xf1:    // INT1   Undefined and Reserved; Does not Generate #UD
            default:
                abort(6);
            case 0x0f:    // two-op instruction prefix
            {
                if ((n + 1) > 15)
                    abort(6);
                mem8_loc = (eip_offset + (n++)) >> 0;
                OPbyte   = (((last_tlb_val = tlb_read[mem8_loc >> 12]) == -1) ? __ld_8bits_mem8_read()
                                                                              : phys_mem8[mem8_loc ^ last_tlb_val]);

                switch (OPbyte) {
                    case 0x06:    // CLTS  CR0 Clear Task-Switched Flag in CR0
                    case 0xa2:    // CPUID  IA32_BIOS_SIGN_ID CPU Identification
                    case 0x31:    // RDTSC IA32_TIME_STAMP_COUNTER EAX Read Time-Stamp Counter
                    case 0xa0:    // PUSH FS SS:[rSP] Push Word, Doubleword or Quadword Onto the Stack
                    case 0xa8:    // PUSH GS SS:[rSP] Push Word, Doubleword or Quadword Onto the Stack
                    case 0xa1:    // POP SS:[rSP] FS Pop a Value from the Stack
                    case 0xa9:    // POP SS:[rSP] GS Pop a Value from the Stack
                    case 0xc8:    // BSWAP  Zvqp Byte Swap
                    case 0xc9:
                    case 0xca:
                    case 0xcb:
                    case 0xcc:
                    case 0xcd:
                    case 0xce:
                    case 0xcf:
                        goto EXEC_LOOP;
                    case 0x80:    // JO Jvds  Jump short if overflow (OF=1)
                    case 0x81:    // JNO Jvds  Jump short if not overflow (OF=0)
                    case 0x82:    // JB Jvds  Jump short if below/not above or equal/carry (CF=1)
                    case 0x83:    // JNB Jvds  Jump short if not below/above or equal/not carry (CF=0)
                    case 0x84:    // JZ Jvds  Jump short if zero/equal (ZF=0)
                    case 0x85:    // JNZ Jvds  Jump short if not zero/not equal (ZF=1)
                    case 0x86:    // JBE Jvds  Jump short if below or equal/not above (CF=1 AND ZF=1)
                    case 0x87:    // JNBE Jvds  Jump short if not below or equal/above (CF=0 AND ZF=0)
                    case 0x88:    // JS Jvds  Jump short if sign (SF=1)
                    case 0x89:    // JNS Jvds  Jump short if not sign (SF=0)
                    case 0x8a:    // JP Jvds  Jump short if parity/parity even (PF=1)
                    case 0x8b:    // JNP Jvds  Jump short if not parity/parity odd
                    case 0x8c:    // JL Jvds  Jump short if less/not greater (SF!=OF)
                    case 0x8d:    // JNL Jvds  Jump short if not less/greater or equal (SF=OF)
                    case 0x8e:    // JLE Jvds  Jump short if less or equal/not greater ((ZF=1) OR (SF!=OF))
                    case 0x8f:    // JNLE Jvds  Jump short if not less nor equal/greater ((ZF=0) AND (SF=OF))
                        n += stride;
                        if (n > 15)
                            abort(6);
                        goto EXEC_LOOP;
                    case 0x90:    // SETO  Eb Set Byte on Condition - overflow (OF=1)
                    case 0x91:    // SETNO  Eb Set Byte on Condition - not overflow (OF=0)
                    case 0x92:    // SETB  Eb Set Byte on Condition - below/not above or equal/carry (CF=1)
                    case 0x93:    // SETNB  Eb Set Byte on Condition - not below/above or equal/not carry (CF=0)
                    case 0x94:    // SETZ  Eb Set Byte on Condition - zero/equal (ZF=0)
                    case 0x95:    // SETNZ  Eb Set Byte on Condition - not zero/not equal (ZF=1)
                    case 0x96:    // SETBE  Eb Set Byte on Condition - below or equal/not above (CF=1 AND ZF=1)
                    case 0x97:    // SETNBE  Eb Set Byte on Condition - not below or equal/above (CF=0 AND ZF=0)
                    case 0x98:    // SETS  Eb Set Byte on Condition - sign (SF=1)
                    case 0x99:    // SETNS  Eb Set Byte on Condition - not sign (SF=0)
                    case 0x9a:    // SETP  Eb Set Byte on Condition - parity/parity even (PF=1)
                    case 0x9b:    // SETNP  Eb Set Byte on Condition - not parity/parity odd
                    case 0x9c:    // SETL  Eb Set Byte on Condition - less/not greater (SF!=OF)
                    case 0x9d:    // SETNL  Eb Set Byte on Condition - not less/greater or equal (SF=OF)
                    case 0x9e:    // SETLE  Eb Set Byte on Condition - less or equal/not greater ((ZF=1) OR (SF!=OF))
                    case 0x9f:    // SETNLE  Eb Set Byte on Condition - not less nor equal/greater ((ZF=0) AND (SF=OF))
                    case 0x40:    // CMOVO Evqp Gvqp Conditional Move - overflow (OF=1)
                    case 0x41:    // CMOVNO Evqp Gvqp Conditional Move - not overflow (OF=0)
                    case 0x42:    // CMOVB Evqp Gvqp Conditional Move - below/not above or equal/carry (CF=1)
                    case 0x43:    // CMOVNB Evqp Gvqp Conditional Move - not below/above or equal/not carry (CF=0)
                    case 0x44:    // CMOVZ Evqp Gvqp Conditional Move - zero/equal (ZF=0)
                    case 0x45:    // CMOVNZ Evqp Gvqp Conditional Move - not zero/not equal (ZF=1)
                    case 0x46:    // CMOVBE Evqp Gvqp Conditional Move - below or equal/not above (CF=1 AND ZF=1)
                    case 0x47:    // CMOVNBE Evqp Gvqp Conditional Move - not below or equal/above (CF=0 AND ZF=0)
                    case 0x48:    // CMOVS Evqp Gvqp Conditional Move - sign (SF=1)
                    case 0x49:    // CMOVNS Evqp Gvqp Conditional Move - not sign (SF=0)
                    case 0x4a:    // CMOVP Evqp Gvqp Conditional Move - parity/parity even (PF=1)
                    case 0x4b:    // CMOVNP Evqp Gvqp Conditional Move - not parity/parity odd
                    case 0x4c:    // CMOVL Evqp Gvqp Conditional Move - less/not greater (SF!=OF)
                    case 0x4d:    // CMOVNL Evqp Gvqp Conditional Move - not less/greater or equal (SF=OF)
                    case 0x4e:    // CMOVLE Evqp Gvqp Conditional Move - less or equal/not greater ((ZF=1) OR (SF!=OF))
                    case 0x4f:    // CMOVNLE Evqp Gvqp Conditional Move - not less nor equal/greater ((ZF=0) AND
                                  // (SF=OF))
                    case 0xb6:    // MOVZX Eb Gvqp Move with Zero-Extend
                    case 0xb7:    // MOVZX Ew Gvqp Move with Zero-Extend
                    case 0xbe:    // MOVSX Eb Gvqp Move with Sign-Extension
                    case 0xbf:    // MOVSX Ew Gvqp Move with Sign-Extension
                    case 0x00:    // SLDT LDTR Mw Store Local Descriptor Table Register
                    case 0x01:    // SGDT GDTR Ms Store Global Descriptor Table Register
                    case 0x02:    // LAR Mw Gvqp Load Access Rights Byte
                    case 0x03:    // LSL Mw Gvqp Load Segment Limit
                    case 0x20:    // MOV Cd Rd Move to/from Control Registers
                    case 0x22:    // MOV Rd Cd Move to/from Control Registers
                    case 0x23:    // MOV Rd Dd Move to/from Debug Registers
                    case 0xb2:    // LSS Mptp SS Load Far Pointer
                    case 0xb4:    // LFS Mptp FS Load Far Pointer
                    case 0xb5:    // LGS Mptp GS Load Far Pointer
                    case 0xa5:    // SHLD Gvqp Evqp Double Precision Shift Left
                    case 0xad:    // SHRD Gvqp Evqp Double Precision Shift Right
                    case 0xa3:    // BT Evqp  Bit Test
                    case 0xab:    // BTS Gvqp Evqp Bit Test and Set
                    case 0xb3:    // BTR Gvqp Evqp Bit Test and Reset
                    case 0xbb:    // BTC Gvqp Evqp Bit Test and Complement
                    case 0xbc:    // BSF Evqp Gvqp Bit Scan Forward
                    case 0xbd:    // BSR Evqp Gvqp Bit Scan Reverse
                    case 0xaf:    // IMUL Evqp Gvqp Signed Multiply
                    case 0xc0:    // XADD  Eb Exchange and Add
                    case 0xc1:    // XADD  Evqp Exchange and Add
                    case 0xb0:    // CMPXCHG Gb Eb Compare and Exchange
                    case 0xb1:    // CMPXCHG Gvqp Evqp Compare and Exchange
                    {

                        if ((n + 1) > 15)
                            abort(6);
                        mem8_loc           = (eip_offset + (n++)) >> 0;
                        uint32_t mem8_locu = mem8_loc;
                        mem8 =
                            (((last_tlb_val = tlb_read[mem8_locu >> 12]) == -1) ? __ld_8bits_mem8_read()
                                                                                : phys_mem8[mem8_loc ^ last_tlb_val]);

                        if (CS_flags & 0x0080) {
                            switch (mem8 >> 6) {
                                case 0:
                                    if ((mem8 & 7) == 6)
                                        n += 2;
                                    break;
                                case 1:
                                    n++;
                                    break;
                                default:
                                    n += 2;
                                    break;
                            }
                        } else {
                            switch ((mem8 & 7) | ((mem8 >> 3) & 0x18)) {
                                case 0x04: {
                                    if ((n + 1) > 15)
                                        abort(6);
                                    mem8_loc           = (eip_offset + (n++)) >> 0;
                                    uint32_t mem8_locu = mem8_loc;
                                    localcc_opbyte_var = (((last_tlb_val = tlb_read[mem8_locu >> 12]) == -1)
                                                              ? __ld_8bits_mem8_read()
                                                              : phys_mem8[mem8_loc ^ last_tlb_val]);

                                    if ((localcc_opbyte_var & 7) == 5) {
                                        n += 4;
                                    }
                                } break;
                                case 0x0c:
                                    n += 2;
                                    break;
                                case 0x14:
                                    n += 5;
                                    break;
                                case 0x05:
                                    n += 4;
                                    break;
                                case 0x00:
                                case 0x01:
                                case 0x02:
                                case 0x03:
                                case 0x06:
                                case 0x07:
                                    break;
                                case 0x08:
                                case 0x09:
                                case 0x0a:
                                case 0x0b:
                                case 0x0d:
                                case 0x0e:
                                case 0x0f:
                                    n++;
                                    break;
                                case 0x10:
                                case 0x11:
                                case 0x12:
                                case 0x13:
                                case 0x15:
                                case 0x16:
                                case 0x17:
                                    n += 4;
                                    break;
                            }
                        }
                        if (n > 15)
                            abort(6);
                    }
                        goto EXEC_LOOP;
                    case 0xa4:    // SHLD Gvqp Evqp Double Precision Shift Left
                    case 0xac:    // SHRD Gvqp Evqp Double Precision Shift Right
                    case 0xba:    // BT Evqp  Bit Test
                    {

                        if ((n + 1) > 15)
                            abort(6);
                        mem8_loc           = (eip_offset + (n++)) >> 0;
                        uint32_t mem8_locu = mem8_loc;
                        mem8 =
                            (((last_tlb_val = tlb_read[mem8_locu >> 12]) == -1) ? __ld_8bits_mem8_read()
                                                                                : phys_mem8[mem8_loc ^ last_tlb_val]);

                        if (CS_flags & 0x0080) {
                            switch (mem8 >> 6) {
                                case 0:
                                    if ((mem8 & 7) == 6)
                                        n += 2;
                                    break;
                                case 1:
                                    n++;
                                    break;
                                default:
                                    n += 2;
                                    break;
                            }
                        } else {
                            switch ((mem8 & 7) | ((mem8 >> 3) & 0x18)) {
                                case 0x04: {
                                    if ((n + 1) > 15)
                                        abort(6);
                                    mem8_loc           = (eip_offset + (n++)) >> 0;
                                    uint32_t mem8_locu = mem8_loc;
                                    localcc_opbyte_var = (((last_tlb_val = tlb_read[mem8_locu >> 12]) == -1)
                                                              ? __ld_8bits_mem8_read()
                                                              : phys_mem8[mem8_loc ^ last_tlb_val]);

                                    if ((localcc_opbyte_var & 7) == 5) {
                                        n += 4;
                                    }
                                } break;
                                case 0x0c:
                                    n += 2;
                                    break;
                                case 0x14:
                                    n += 5;
                                    break;
                                case 0x05:
                                    n += 4;
                                    break;
                                case 0x00:
                                case 0x01:
                                case 0x02:
                                case 0x03:
                                case 0x06:
                                case 0x07:
                                    break;
                                case 0x08:
                                case 0x09:
                                case 0x0a:
                                case 0x0b:
                                case 0x0d:
                                case 0x0e:
                                case 0x0f:
                                    n++;
                                    break;
                                case 0x10:
                                case 0x11:
                                case 0x12:
                                case 0x13:
                                case 0x15:
                                case 0x16:
                                case 0x17:
                                    n += 4;
                                    break;
                            }
                        }
                        if (n > 15)
                            abort(6);
                    }
                        n++;
                        if (n > 15)
                            abort(6);
                        goto EXEC_LOOP;
                    case 0x04:
                    case 0x05:    // LOADALL  AX Load All of the CPU Registers
                    case 0x07:    // LOADALL  EAX Load All of the CPU Registers
                    case 0x08:    // INVD   Invalidate Internal Caches
                    case 0x09:    // WBINVD   Write Back and Invalidate Cache
                    case 0x0a:
                    case 0x0b:    // UD2   Undefined Instruction
                    case 0x0c:
                    case 0x0d:    // NOP Ev  No Operation
                    case 0x0e:
                    case 0x0f:
                    case 0x10:    // MOVUPS Wps Vps Move Unaligned Packed Single-FP Values
                    case 0x11:    // MOVUPS Vps Wps Move Unaligned Packed Single-FP Values
                    case 0x12:    // MOVHLPS Uq Vq Move Packed Single-FP Values High to Low
                    case 0x13:    // MOVLPS Vq Mq Move Low Packed Single-FP Values
                    case 0x14:    // UNPCKLPS Wq Vps Unpack and Interleave Low Packed Single-FP Values
                    case 0x15:    // UNPCKHPS Wq Vps Unpack and Interleave High Packed Single-FP Values
                    case 0x16:    // MOVLHPS Uq Vq Move Packed Single-FP Values Low to High
                    case 0x17:    // MOVHPS Vq Mq Move High Packed Single-FP Values
                    case 0x18:    // HINT_NOP Ev  Hintable NOP
                    case 0x19:    // HINT_NOP Ev  Hintable NOP
                    case 0x1a:    // HINT_NOP Ev  Hintable NOP
                    case 0x1b:    // HINT_NOP Ev  Hintable NOP
                    case 0x1c:    // HINT_NOP Ev  Hintable NOP
                    case 0x1d:    // HINT_NOP Ev  Hintable NOP
                    case 0x1e:    // HINT_NOP Ev  Hintable NOP
                    case 0x1f:    // HINT_NOP Ev  Hintable NOP
                    case 0x21:    // MOV Dd Rd Move to/from Debug Registers
                    case 0x24:    // MOV Td Rd Move to/from Test Registers
                    case 0x25:
                    case 0x26:    // MOV Rd Td Move to/from Test Registers
                    case 0x27:
                    case 0x28:    // MOVAPS Wps Vps Move Aligned Packed Single-FP Values
                    case 0x29:    // MOVAPS Vps Wps Move Aligned Packed Single-FP Values
                    case 0x2a:    // CVTPI2PS Qpi Vps Convert Packed DW Integers to1.11 PackedSingle-FP Values
                    case 0x2b:    // MOVNTPS Vps Mps Store Packed Single-FP Values Using Non-Temporal Hint
                    case 0x2c:    // CVTTPS2PI Wpsq Ppi Convert with Trunc. Packed Single-FP Values to1.11 PackedDW
                                  // Integers
                    case 0x2d:    // CVTPS2PI Wpsq Ppi Convert Packed Single-FP Values to1.11 PackedDW Integers
                    case 0x2e:    // UCOMISS Vss  Unordered Compare Scalar Single-FP Values and Set EFLAGS
                    case 0x2f:    // COMISS Vss  Compare Scalar Ordered Single-FP Values and Set EFLAGS
                    case 0x30:    // WRMSR rCX MSR Write to Model Specific Register
                    case 0x32:    // RDMSR rCX rAX Read from Model Specific Register
                    case 0x33:    // RDPMC PMC EAX Read Performance-Monitoring Counters
                    case 0x34:    // SYSENTER IA32_SYSENTER_CS SS Fast System Call
                    case 0x35:    // SYSEXIT IA32_SYSENTER_CS SS Fast Return from Fast System Call
                    case 0x36:
                    case 0x37:    // GETSEC EAX  GETSEC Leaf Functions
                    case 0x38:    // PSHUFB Qq Pq Packed Shuffle Bytes
                    case 0x39:
                    case 0x3a:    // ROUNDPS Wps Vps Round Packed Single-FP Values
                    case 0x3b:
                    case 0x3c:
                    case 0x3d:
                    case 0x3e:
                    case 0x3f:
                    case 0x50:    // MOVMSKPS Ups Gdqp Extract Packed Single-FP Sign Mask
                    case 0x51:    // SQRTPS Wps Vps Compute Square Roots of Packed Single-FP Values
                    case 0x52:    // RSQRTPS Wps Vps Compute Recipr. of Square Roots of Packed Single-FP Values
                    case 0x53:    // RCPPS Wps Vps Compute Reciprocals of Packed Single-FP Values
                    case 0x54:    // ANDPS Wps Vps Bitwise Logical AND of Packed Single-FP Values
                    case 0x55:    // ANDNPS Wps Vps Bitwise Logical AND NOT of Packed Single-FP Values
                    case 0x56:    // ORPS Wps Vps Bitwise Logical OR of Single-FP Values
                    case 0x57:    // XORPS Wps Vps Bitwise Logical XOR for Single-FP Values
                    case 0x58:    // ADDPS Wps Vps Add Packed Single-FP Values
                    case 0x59:    // MULPS Wps Vps Multiply Packed Single-FP Values
                    case 0x5a:    // CVTPS2PD Wps Vpd Convert Packed Single-FP Values to1.11 PackedDouble-FP Values
                    case 0x5b:    // CVTDQ2PS Wdq Vps Convert Packed DW Integers to1.11 PackedSingle-FP Values
                    case 0x5c:    // SUBPS Wps Vps Subtract Packed Single-FP Values
                    case 0x5d:    // MINPS Wps Vps Return Minimum Packed Single-FP Values
                    case 0x5e:    // DIVPS Wps Vps Divide Packed Single-FP Values
                    case 0x5f:    // MAXPS Wps Vps Return Maximum Packed Single-FP Values
                    case 0x60:    // PUNPCKLBW Qd Pq Unpack Low Data
                    case 0x61:    // PUNPCKLWD Qd Pq Unpack Low Data
                    case 0x62:    // PUNPCKLDQ Qd Pq Unpack Low Data
                    case 0x63:    // PACKSSWB Qd Pq Pack with Signed Saturation
                    case 0x64:    // PCMPGTB Qd Pq Compare Packed Signed Integers for Greater Than
                    case 0x65:    // PCMPGTW Qd Pq Compare Packed Signed Integers for Greater Than
                    case 0x66:    // PCMPGTD Qd Pq Compare Packed Signed Integers for Greater Than
                    case 0x67:    // PACKUSWB Qq Pq Pack with Unsigned Saturation
                    case 0x68:    // PUNPCKHBW Qq Pq Unpack High Data
                    case 0x69:    // PUNPCKHWD Qq Pq Unpack High Data
                    case 0x6a:    // PUNPCKHDQ Qq Pq Unpack High Data
                    case 0x6b:    // PACKSSDW Qq Pq Pack with Signed Saturation
                    case 0x6c:    // PUNPCKLQDQ Wdq Vdq Unpack Low Data
                    case 0x6d:    // PUNPCKHQDQ Wdq Vdq Unpack High Data
                    case 0x6e:    // MOVD Ed Pq Move Doubleword
                    case 0x6f:    // MOVQ Qq Pq Move Quadword
                    case 0x70:    // PSHUFW Qq Pq Shuffle Packed Words
                    case 0x71:    // PSRLW Ib Nq Shift Packed Data Right Logical
                    case 0x72:    // PSRLD Ib Nq Shift Double Quadword Right Logical
                    case 0x73:    // PSRLQ Ib Nq Shift Packed Data Right Logical
                    case 0x74:    // PCMPEQB Qq Pq Compare Packed Data for Equal
                    case 0x75:    // PCMPEQW Qq Pq Compare Packed Data for Equal
                    case 0x76:    // PCMPEQD Qq Pq Compare Packed Data for Equal
                    case 0x77:    // EMMS   Empty MMX Technology State
                    case 0x78:    // VMREAD Gd Ed Read Field from Virtual-Machine Control Structure
                    case 0x79:    // VMWRITE Gd  Write Field to Virtual-Machine Control Structure
                    case 0x7a:
                    case 0x7b:
                    case 0x7c:    // HADDPD Wpd Vpd Packed Double-FP Horizontal Add
                    case 0x7d:    // HSUBPD Wpd Vpd Packed Double-FP Horizontal Subtract
                    case 0x7e:    // MOVD Pq Ed Move Doubleword
                    case 0x7f:    // MOVQ Pq Qq Move Quadword
                    case 0xa6:
                    case 0xa7:
                    case 0xaa:    // RSM  Flags Resume from System Management Mode
                    case 0xae:    // FXSAVE ST Mstx Save x87 FPU, MMX, XMM, and MXCSR State
                    case 0xb8:    // JMPE   Jump to IA-64 Instruction Set
                    case 0xb9:    // UD G  Undefined Instruction
                    case 0xc2:    // CMPPS Wps Vps Compare Packed Single-FP Values
                    case 0xc3:    // MOVNTI Gdqp Mdqp Store Doubleword Using Non-Temporal Hint
                    case 0xc4:    // PINSRW Rdqp Pq Insert Word
                    case 0xc5:    // PEXTRW Nq Gdqp Extract Word
                    case 0xc6:    // SHUFPS Wps Vps Shuffle Packed Single-FP Values
                    case 0xc7:    // CMPXCHG8B EBX Mq Compare and Exchange Bytes
                    default:
                        abort(6);
                }
            } break;
        }
    }
EXEC_LOOP:;
    return n;
}
void x86Internal::set_CR0(int Qd)
{
    if (!(Qd & (1 << 0)))    // 0th bit protected or real, real not supported!
        cpu_abort("real mode not supported");
    // if changing flags 31, 16, or 0 must flush tlb
    if ((Qd & ((1 << 31) | (1 << 16) | (1 << 0))) != (cr0 & ((1 << 31) | (1 << 16) | (1 << 0)))) {
        tlb_flush_all();
    }
    cr0 = Qd | (1 << 4);    // keep bit 4 set to 1
}
void x86Internal::set_CR3(int new_pdb)
{
    cr3 = new_pdb;
    if (cr0 & (1 << 31)) {    // if in paging mode must reset tables
        tlb_flush_all();
    }
}
void x86Internal::set_CR4(int newval)
{
    cr4 = newval;
}
int x86Internal::SS_mask_from_flags(int desp_high4)
{
    if (desp_high4 & (1 << 22))
        return -1;
    else
        return 0xffff;
}
void x86Internal::load_from_descriptor_table(int selector, int *desary)
{
    DescriptorTable descriptor_table;
    int             Rb, desp_low4, desp_high4;

    if (selector & 0x4)
        descriptor_table = ldt;
    else
        descriptor_table = gdt;
    Rb = selector & ~7;

    if ((Rb + 7) > descriptor_table.limit)
        return;

    mem8_loc  = descriptor_table.base + Rb;
    desp_low4 = ld32_mem8_kernel_read();
    mem8_loc += 4;
    desp_high4 = ld32_mem8_kernel_read();

    desary[0] = desp_low4;
    desary[1] = desp_high4;
    return;
}
int x86Internal::calc_desp_limit(int desp_low4, int desp_high4)
{
    int limit = (desp_low4 & 0xffff) | (desp_high4 & 0x000f0000);
    if (desp_high4 & (1 << 23))
        limit = (limit << 12) | 0xfff;
    return limit;
}
int x86Internal::calc_desp_base(int desp_low4, int desp_high4)
{
    return (((desp_low4 >> 16) | ((desp_high4 & 0xff) << 16) | (desp_high4 & 0xff000000))) & -1;
}
void x86Internal::set_descriptor_register(DescriptorTable *descriptor_table, int desp_low4, int desp_high4)
{
    descriptor_table->base  = calc_desp_base(desp_low4, desp_high4);
    descriptor_table->limit = calc_desp_limit(desp_low4, desp_high4);
    descriptor_table->flags = desp_high4;
}
void x86Internal::set_segment_vars(int ee, int selector, int base, int limit, int flags)
{
    segs[ee] = {.selector = selector, .base = base, .limit = limit, .flags = flags};
    init_segment_local_vars();
}
void x86Internal::init_segment_vars_with_selector(int Sb, int selector)
{
    set_segment_vars(Sb, selector, (selector << 4), 0xffff,
                     (1 << 15) | (3 << 13) | (1 << 12) | (1 << 8) | (1 << 12) | (1 << 9));
}

void x86Internal::load_from_TR(int he, int *desary)
{
    int tr_type, Rb, is_32_bit, ke, le;
    if (!(tr.flags & (1 << 15)))
        cpu_abort("invalid tss");    // task state segment

    tr_type = (tr.flags >> 8) & 0xf;
    if ((tr_type & 7) != 1)
        cpu_abort("invalid tss type");

    is_32_bit = tr_type >> 3;
    Rb        = (he * 4 + 2) << is_32_bit;

    if (Rb + (4 << is_32_bit) - 1 > tr.limit)
        abort_with_error_code(10, tr.selector & 0xfffc);

    mem8_loc = (tr.base + Rb) & -1;
    if (is_32_bit == 0) {
        le = ld16_mem8_kernel_read();
        mem8_loc += 2;
    } else {
        le = ld32_mem8_kernel_read();
        mem8_loc += 4;
    }
    ke = ld16_mem8_kernel_read();

    desary[0] = ke;
    desary[1] = le;
}
void x86Internal::do_interrupt_protected_mode(int intno, int ne, int error_code, int oe, int pe)
{
    DescriptorTable descriptor_table;
    int             qe, descriptor_type, he, selector, re, cpl_var;
    int             te, ue, is_32_bit;
    int             desp_low4, desp_high4, ve, ke, le, we, xe;
    int             ye, SS_mask;
    int             e[2];
    te = 0;

    if (!ne && !pe) {
        switch (intno) {
            case 8:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 17:
                te = 1;
                break;
        }
    }

    if (ne)
        ye = oe;
    else
        ye = eip;

    descriptor_table = idt;
    if (intno * 8 + 7 > descriptor_table.limit)
        abort_with_error_code(13, intno * 8 + 2);

    mem8_loc  = (descriptor_table.base + intno * 8) & -1;
    desp_low4 = ld32_mem8_kernel_read();
    mem8_loc += 4;
    desp_high4      = ld32_mem8_kernel_read();
    descriptor_type = (desp_high4 >> 8) & 0x1f;

    switch (descriptor_type) {
        case 5:
        case 7:
        case 6:
            throw "unsupported task gate";
        case 14:
        case 15:
            break;
        default:
            abort_with_error_code(13, intno * 8 + 2);
            break;
    }
    dpl = (desp_high4 >> 13) & 3;

    cpl_var = cpl;
    if (ne && dpl < cpl_var)
        abort_with_error_code(13, intno * 8 + 2);
    if (!(desp_high4 & (1 << 15)))
        abort_with_error_code(11, intno * 8 + 2);

    selector = desp_low4 >> 16;
    ve       = (desp_high4 & -65536) | (desp_low4 & 0x0000ffff);

    if ((selector & 0xfffc) == 0)
        abort_with_error_code(13, 0);

    load_from_descriptor_table(selector, e);
    if (e[0] == 0 && e[1] == 0)
        abort_with_error_code(13, selector & 0xfffc);

    desp_low4  = e[0];
    desp_high4 = e[1];
    if (!(desp_high4 & (1 << 12)) || !(desp_high4 & ((1 << 11))))
        abort_with_error_code(13, selector & 0xfffc);

    dpl = (desp_high4 >> 13) & 3;
    if (dpl > cpl_var)
        abort_with_error_code(13, selector & 0xfffc);

    if (!(desp_high4 & (1 << 15)))
        abort_with_error_code(11, selector & 0xfffc);

    if (!(desp_high4 & (1 << 10)) && dpl < cpl_var) {
        load_from_TR(dpl, e);
        ke = e[0];
        le = e[1];
        if ((ke & 0xfffc) == 0)
            abort_with_error_code(10, ke & 0xfffc);

        if ((ke & 3) != dpl)
            abort_with_error_code(10, ke & 0xfffc);

        load_from_descriptor_table(ke, e);
        if (e[0] == 0 && e[1] == 0)
            abort_with_error_code(10, ke & 0xfffc);

        we = e[0];
        xe = e[1];
        re = (xe >> 13) & 3;
        if (re != dpl)
            abort_with_error_code(10, ke & 0xfffc);

        if (!(xe & (1 << 12)) || (xe & (1 << 11)) || !(xe & (1 << 9)))
            abort_with_error_code(10, ke & 0xfffc);

        if (!(xe & (1 << 15)))
            abort_with_error_code(10, ke & 0xfffc);

        ue      = 1;
        SS_mask = SS_mask_from_flags(xe);
        qe      = calc_desp_base(we, xe);
    } else if ((desp_high4 & (1 << 10)) || dpl == cpl_var) {
        if (eflags & 0x00020000)
            abort_with_error_code(13, selector & 0xfffc);
        ue      = 0;
        SS_mask = SS_mask_from_flags(segs[2].flags);
        qe      = segs[2].base;
        le      = regs[4];
        dpl     = cpl_var;
    } else {
        abort_with_error_code(13, selector & 0xfffc);
        ue      = 0;
        SS_mask = 0;
        qe      = 0;
        le      = 0;
    }

    is_32_bit = descriptor_type >> 3;

    if (is_32_bit == 1) {
        if (ue) {
            if (eflags & 0x00020000) {

                le       = (le - 4) & -1;
                mem8_loc = (qe + (le & SS_mask)) & -1;
                st32_mem8_kernel_write(segs[5].selector);

                le       = (le - 4) & -1;
                mem8_loc = (qe + (le & SS_mask)) & -1;
                st32_mem8_kernel_write(segs[4].selector);

                le       = (le - 4) & -1;
                mem8_loc = (qe + (le & SS_mask)) & -1;
                st32_mem8_kernel_write(segs[3].selector);

                le       = (le - 4) & -1;
                mem8_loc = (qe + (le & SS_mask)) & -1;
                st32_mem8_kernel_write(segs[0].selector);
            }

            le       = (le - 4) & -1;
            mem8_loc = (qe + (le & SS_mask)) & -1;
            st32_mem8_kernel_write(segs[2].selector);

            le       = (le - 4) & -1;
            mem8_loc = (qe + (le & SS_mask)) & -1;
            st32_mem8_kernel_write(regs[4]);
        }

        le       = (le - 4) & -1;
        mem8_loc = (qe + (le & SS_mask)) & -1;
        st32_mem8_kernel_write(get_FLAGS());

        le       = (le - 4) & -1;
        mem8_loc = (qe + (le & SS_mask)) & -1;
        st32_mem8_kernel_write(segs[1].selector);

        le       = (le - 4) & -1;
        mem8_loc = (qe + (le & SS_mask)) & -1;
        st32_mem8_kernel_write(ye);

        if (te) {

            le       = (le - 4) & -1;
            mem8_loc = (qe + (le & SS_mask)) & -1;
            st32_mem8_kernel_write(error_code);
        }
    } else {
        if (ue) {
            if (eflags & 0x00020000) {

                le       = (le - 2) & -1;
                mem8_loc = (qe + (le & SS_mask)) & -1;
                st16_mem8_kernel_write(segs[5].selector);

                le       = (le - 2) & -1;
                mem8_loc = (qe + (le & SS_mask)) & -1;
                st16_mem8_kernel_write(segs[4].selector);

                le       = (le - 2) & -1;
                mem8_loc = (qe + (le & SS_mask)) & -1;
                st16_mem8_kernel_write(segs[3].selector);

                le       = (le - 2) & -1;
                mem8_loc = (qe + (le & SS_mask)) & -1;
                st16_mem8_kernel_write(segs[0].selector);
            }

            le       = (le - 2) & -1;
            mem8_loc = (qe + (le & SS_mask)) & -1;
            st16_mem8_kernel_write(segs[2].selector);

            le       = (le - 2) & -1;
            mem8_loc = (qe + (le & SS_mask)) & -1;
            st16_mem8_kernel_write(regs[4]);
        }

        le       = (le - 2) & -1;
        mem8_loc = (qe + (le & SS_mask)) & -1;
        st16_mem8_kernel_write(get_FLAGS());

        le       = (le - 2) & -1;
        mem8_loc = (qe + (le & SS_mask)) & -1;
        st16_mem8_kernel_write(segs[1].selector);

        le       = (le - 2) & -1;
        mem8_loc = (qe + (le & SS_mask)) & -1;
        st16_mem8_kernel_write(ye);

        if (te) {

            le       = (le - 2) & -1;
            mem8_loc = (qe + (le & SS_mask)) & -1;
            st16_mem8_kernel_write(error_code);
        }
    }

    if (ue) {
        if (eflags & 0x00020000) {
            set_segment_vars(0, 0, 0, 0, 0);
            set_segment_vars(3, 0, 0, 0, 0);
            set_segment_vars(4, 0, 0, 0, 0);
            set_segment_vars(5, 0, 0, 0, 0);
        }
        ke = (ke & ~3) | dpl;
        set_segment_vars(2, ke, qe, calc_desp_limit(we, xe), xe);
    }
    regs[4]  = (regs[4] & ~SS_mask) | ((le)&SS_mask);
    selector = (selector & ~3) | dpl;
    set_segment_vars(1, selector, calc_desp_base(desp_low4, desp_high4), calc_desp_limit(desp_low4, desp_high4),
                     desp_high4);
    change_permission_level(dpl);
    eip = ve, physmem8_ptr = initial_mem_ptr = 0;
    if ((descriptor_type & 1) == 0) {
        eflags &= ~0x00000200;
    }
    eflags &= ~(0x00000100 | 0x00020000 | 0x00010000 | 0x00004000);
}
void x86Internal::do_interrupt_not_protected_mode(int intno, int ne, int error_code, int oe, int pe)
{
    DescriptorTable descriptor_table;
    int             qe, selector, ve, le, ye;
    descriptor_table = idt;

    if (intno * 4 + 3 > descriptor_table.limit)
        abort_with_error_code(13, intno * 8 + 2);

    mem8_loc = (descriptor_table.base + (intno << 2)) >> 0;
    ve       = ld16_mem8_kernel_read();
    mem8_loc = (mem8_loc + 2) >> 0;
    selector = ld16_mem8_kernel_read();
    le       = regs[4];

    if (ne)
        ye = oe;
    else
        ye = eip;

    le       = (le - 2) >> 0;
    mem8_loc = ((le & SS_mask) + SS_base) >> 0;
    st16_mem8_write(get_FLAGS());

    le       = (le - 2) >> 0;
    mem8_loc = ((le & SS_mask) + SS_base) >> 0;
    st16_mem8_write(segs[1].selector);

    le       = (le - 2) >> 0;
    mem8_loc = ((le & SS_mask) + SS_base) >> 0;
    st16_mem8_write(ye);

    regs[4] = (regs[4] & ~SS_mask) | ((le)&SS_mask);
    eip = ve, physmem8_ptr = initial_mem_ptr = 0;
    segs[1].selector = selector;
    segs[1].base     = (selector << 4);
    eflags &= ~(0x00000200 | 0x00000100 | 0x00040000 | 0x00010000);
}
void x86Internal::do_interrupt(int intno, int ne, int error_code, int oe, int pe)
{
    if (intno == 0x06) {
        int         eip_tmp = eip;
        int         eip_offset;
        std::string str = "do_interrupt: intno=" + _2_bytes_(intno) + " error_code=" + _4_bytes_(error_code) +
                          " EIP=" + _4_bytes_(eip_tmp) + " ESP=" + _4_bytes_(regs[4]) + " EAX=" + _4_bytes_(regs[0]) +
                          " EBX=" + _4_bytes_(regs[3]) + " ECX=" + _4_bytes_(regs[1]);
        if (intno == 0x0e) {
            str += " CR2=" + _4_bytes_(cr2);
        }

        printf("%s\n", str.c_str());

        if (intno == 0x06) {
            int i, n;
            str        = "Code:";
            eip_offset = (eip_tmp + CS_base) >> 0;
            n          = 4096 - (eip_offset & 0xfff);

            if (n > 15)
                n = 15;
            for (i = 0; i < n; i++) {
                mem8_loc = (eip_offset + i) & -1;
                str += " " + _2_bytes_(ld_8bits_mem8_read());
            }

            printf("%s\n", str.c_str());
        }
    }
    if (cr0 & (1 << 0)) {
        do_interrupt_protected_mode(intno, ne, error_code, oe, pe);
    } else {
        do_interrupt_not_protected_mode(intno, ne, error_code, oe, pe);
    }
}
void x86Internal::op_LDTR(int selector)
{
    DescriptorTable descriptor_table;
    int             desp_low4, desp_high4, Rb, De;
    selector &= 0xffff;
    if ((selector & 0xfffc) == 0) {
        ldt.base  = 0;
        ldt.limit = 0;
    } else {
        if (selector & 0x4)
            abort_with_error_code(13, selector & 0xfffc);
        descriptor_table = gdt;
        Rb               = selector & ~7;
        De               = 7;
        if ((Rb + De) > descriptor_table.limit)
            abort_with_error_code(13, selector & 0xfffc);
        mem8_loc  = (descriptor_table.base + Rb) & -1;
        desp_low4 = ld32_mem8_kernel_read();
        mem8_loc += 4;
        desp_high4 = ld32_mem8_kernel_read();
        if ((desp_high4 & (1 << 12)) || ((desp_high4 >> 8) & 0xf) != 2)
            abort_with_error_code(13, selector & 0xfffc);
        if (!(desp_high4 & (1 << 15)))
            abort_with_error_code(11, selector & 0xfffc);
        set_descriptor_register(&ldt, desp_low4, desp_high4);
    }
    ldt.selector = selector;
}
void x86Internal::op_LTR(int selector)
{
    DescriptorTable descriptor_table;
    int             desp_low4, desp_high4, Rb, descriptor_type, De;
    selector &= 0xffff;
    if ((selector & 0xfffc) == 0) {
        tr.base  = 0;
        tr.limit = 0;
        tr.flags = 0;
    } else {
        if (selector & 0x4)
            abort_with_error_code(13, selector & 0xfffc);
        descriptor_table = gdt;
        Rb               = selector & ~7;
        De               = 7;
        if ((Rb + De) > descriptor_table.limit)
            abort_with_error_code(13, selector & 0xfffc);
        mem8_loc  = (descriptor_table.base + Rb) & -1;
        desp_low4 = ld32_mem8_kernel_read();
        mem8_loc += 4;
        desp_high4      = ld32_mem8_kernel_read();
        descriptor_type = (desp_high4 >> 8) & 0xf;
        if ((desp_high4 & (1 << 12)) || (descriptor_type != 1 && descriptor_type != 9))
            abort_with_error_code(13, selector & 0xfffc);
        if (!(desp_high4 & (1 << 15)))
            abort_with_error_code(11, selector & 0xfffc);
        set_descriptor_register(&tr, desp_low4, desp_high4);
        desp_high4 |= (1 << 9);
        st32_mem8_kernel_write(desp_high4);
    }
    tr.selector = selector;
}
void x86Internal::set_protected_mode_segment_register(int reg, int selector)
{
    DescriptorTable descriptor_table;
    int             desp_low4, desp_high4, cpl_var, dpl, rpl, selector_index;
    cpl_var = cpl;
    if ((selector & 0xfffc) == 0) {    // null selector
        if (reg == 2)                  //(SS == null) => #GP(0)
            abort_with_error_code(13, 0);
        set_segment_vars(reg, selector, 0, 0, 0);
    } else {
        if (selector & 0x4)
            descriptor_table = ldt;
        else
            descriptor_table = gdt;
        selector_index = selector & ~7;
        if ((selector_index + 7) > descriptor_table.limit)
            abort_with_error_code(13, selector & 0xfffc);
        mem8_loc  = (descriptor_table.base + selector_index) & -1;
        desp_low4 = ld32_mem8_kernel_read();
        mem8_loc += 4;
        desp_high4 = ld32_mem8_kernel_read();
        if (!(desp_high4 & (1 << 12)))
            abort_with_error_code(13, selector & 0xfffc);
        rpl = selector & 3;
        dpl = (desp_high4 >> 13) & 3;
        if (reg == 2) {
            if ((desp_high4 & (1 << 11)) || !(desp_high4 & (1 << 9)))
                abort_with_error_code(13, selector & 0xfffc);
            if (rpl != cpl_var || dpl != cpl_var)
                abort_with_error_code(13, selector & 0xfffc);
        } else {
            if ((desp_high4 & ((1 << 11) | (1 << 9))) == (1 << 11))
                abort_with_error_code(13, selector & 0xfffc);
            if (!(desp_high4 & (1 << 11)) || !(desp_high4 & (1 << 10))) {
                if (dpl < cpl_var || dpl < rpl)
                    abort_with_error_code(13, selector & 0xfffc);
            }
        }
        if (!(desp_high4 & (1 << 15))) {
            if (reg == 2)
                abort_with_error_code(12, selector & 0xfffc);
            else
                abort_with_error_code(11, selector & 0xfffc);
        }
        if (!(desp_high4 & (1 << 8))) {
            desp_high4 |= (1 << 8);
            st32_mem8_kernel_write(desp_high4);
        }
        set_segment_vars(reg, selector, calc_desp_base(desp_low4, desp_high4), calc_desp_limit(desp_low4, desp_high4),
                         desp_high4);
    }
}
void x86Internal::set_segment_register(int reg, int selector)
{
    DescriptorTable descriptor_table;
    selector &= 0xffff;
    if (!(cr0 & (1 << 0))) {    // CR0.PE (0 == real mode)
        descriptor_table          = segs[reg];
        descriptor_table.selector = selector;
        descriptor_table.base     = selector << 4;
    } else if (eflags & 0x00020000) {    // EFLAGS.VM (1 == v86 mode)
        init_segment_vars_with_selector(reg, selector);
    } else {    // protected mode
        set_protected_mode_segment_register(reg, selector);
    }
}
void x86Internal::do_JMPF_virtual_mode(int selector, int Le)
{
    eip = Le, physmem8_ptr = initial_mem_ptr = 0;
    segs[1].selector = selector;
    segs[1].base     = (selector << 4);
    init_segment_local_vars();
}
void x86Internal::do_JMPF(int selector, int Le)
{
    int      Ne, ie, desp_low4, desp_high4, cpl_var, dpl, rpl;
    uint32_t limit;

    if ((selector & 0xfffc) == 0)
        abort_with_error_code(13, 0);

    int e[2];
    load_from_descriptor_table(selector, e);

    if (e[0] == 0 && e[1] == 0)
        abort_with_error_code(13, selector & 0xfffc);

    desp_low4  = e[0];
    desp_high4 = e[1];
    cpl_var    = cpl;
    if (desp_high4 & (1 << 12)) {
        if (!(desp_high4 & (1 << 11)))
            abort_with_error_code(13, selector & 0xfffc);
        dpl = (desp_high4 >> 13) & 3;
        if (desp_high4 & (1 << 10)) {
            if (dpl > cpl_var)
                abort_with_error_code(13, selector & 0xfffc);
        } else {
            rpl = selector & 3;
            if (rpl > cpl_var)
                abort_with_error_code(13, selector & 0xfffc);
            if (dpl != cpl_var)
                abort_with_error_code(13, selector & 0xfffc);
        }
        if (!(desp_high4 & (1 << 15)))
            abort_with_error_code(11, selector & 0xfffc);
        limit = calc_desp_limit(desp_low4, desp_high4);
        if ((Le >> 0) > (limit >> 0))
            abort_with_error_code(13, selector & 0xfffc);
        set_segment_vars(1, (selector & 0xfffc) | cpl_var, calc_desp_base(desp_low4, desp_high4), limit, desp_high4);
        eip = Le, physmem8_ptr = initial_mem_ptr = 0;
    } else {
        cpu_abort("unsupported jump to call or task gate");
    }
}
void x86Internal::op_JMPF(int selector, int Le)
{
    if (!(cr0 & (1 << 0)) || (eflags & 0x00020000)) {
        do_JMPF_virtual_mode(selector, Le);
    } else {
        do_JMPF(selector, Le);
    }
}
void x86Internal::Pe(int reg, int cpl_var)
{
    int dpl, desp_high4;
    if ((reg == 4 || reg == 5) && (segs[reg].selector & 0xfffc) == 0)
        return;
    desp_high4 = segs[reg].flags;
    dpl        = (desp_high4 >> 13) & 3;
    if (!(desp_high4 & (1 << 11)) || !(desp_high4 & (1 << 10))) {
        if (dpl < cpl_var) {
            set_segment_vars(reg, 0, 0, 0, 0);
        }
    }
}
void x86Internal::op_CALLF_not_protected_mode(bool is_32_bit, int selector, int Le, int oe)
{
    int le;
    le = regs[4];
    if (is_32_bit) {

        le       = (le - 4) >> 0;
        mem8_loc = ((le & SS_mask) + SS_base) >> 0;
        st32_mem8_write(segs[1].selector);

        le       = (le - 4) >> 0;
        mem8_loc = ((le & SS_mask) + SS_base) >> 0;
        st32_mem8_write(oe);

    } else {

        le       = (le - 2) >> 0;
        mem8_loc = ((le & SS_mask) + SS_base) >> 0;
        st16_mem8_write(segs[1].selector);

        le       = (le - 2) >> 0;
        mem8_loc = ((le & SS_mask) + SS_base) >> 0;
        st16_mem8_write(oe);
    }
    regs[4] = (regs[4] & ~SS_mask) | ((le)&SS_mask);
    eip = Le, physmem8_ptr = initial_mem_ptr = 0;
    segs[1].selector = selector;
    segs[1].base     = (selector << 4);
    init_segment_local_vars();
}
void x86Internal::op_CALLF_protected_mode(bool is_32_bit, int selector, int Le, int oe)
{
    int ue, i;
    int desp_low4, desp_high4, cpl_var, dpl, rpl, ve, Se;
    int ke, we, xe, esp, descriptor_type, re, SS_mask;
    int x, limit, Ue;
    int qe, Ve, We;

    if ((selector & 0xfffc) == 0)
        abort_with_error_code(13, 0);

    int e[2];
    load_from_descriptor_table(selector, e);

    if (e[0] == 0 && e[1] == 0)
        abort_with_error_code(13, selector & 0xfffc);

    desp_low4  = e[0];
    desp_high4 = e[1];
    cpl_var    = cpl;
    We         = regs[4];
    if (desp_high4 & (1 << 12)) {
        if (!(desp_high4 & (1 << 11)))
            abort_with_error_code(13, selector & 0xfffc);
        dpl = (desp_high4 >> 13) & 3;
        if (desp_high4 & (1 << 10)) {
            if (dpl > cpl_var)
                abort_with_error_code(13, selector & 0xfffc);
        } else {
            rpl = selector & 3;
            if (rpl > cpl_var)
                abort_with_error_code(13, selector & 0xfffc);
            if (dpl != cpl_var)
                abort_with_error_code(13, selector & 0xfffc);
        }
        if (!(desp_high4 & (1 << 15)))
            abort_with_error_code(11, selector & 0xfffc);

        esp     = We;
        SS_mask = SS_mask_from_flags(segs[2].flags);
        qe      = segs[2].base;
        if (is_32_bit) {

            esp      = (esp - 4) & -1;
            mem8_loc = (qe + (esp & SS_mask)) & -1;
            st32_mem8_kernel_write(segs[1].selector);

            esp      = (esp - 4) & -1;
            mem8_loc = (qe + (esp & SS_mask)) & -1;
            st32_mem8_kernel_write(oe);

        } else {

            esp      = (esp - 2) & -1;
            mem8_loc = (qe + (esp & SS_mask)) & -1;
            st16_mem8_kernel_write(segs[1].selector);

            esp      = (esp - 2) & -1;
            mem8_loc = (qe + (esp & SS_mask)) & -1;
            st16_mem8_kernel_write(oe);
        }
        limit = calc_desp_limit(desp_low4, desp_high4);
        if (Le > limit)
            abort_with_error_code(13, selector & 0xfffc);
        regs[4] = (regs[4] & ~SS_mask) | ((esp)&SS_mask);
        set_segment_vars(1, (selector & 0xfffc) | cpl_var, calc_desp_base(desp_low4, desp_high4), limit, desp_high4);
        eip = Le, physmem8_ptr = initial_mem_ptr = 0;

    } else {
        descriptor_type = (desp_high4 >> 8) & 0x1f;
        dpl             = (desp_high4 >> 13) & 3;
        rpl             = selector & 3;
        switch (descriptor_type) {
            case 1:
            case 9:
            case 5:
                throw "unsupported task gate";
                return;
            case 4:
            case 12:
                break;
            default:
                abort_with_error_code(13, selector & 0xfffc);
                break;
        }
        is_32_bit = descriptor_type >> 3;
        if (dpl < cpl_var || dpl < rpl)
            abort_with_error_code(13, selector & 0xfffc);
        if (!(desp_high4 & (1 << 15)))
            abort_with_error_code(11, selector & 0xfffc);
        selector = desp_low4 >> 16;
        ve       = (desp_high4 & 0xffff0000) | (desp_low4 & 0x0000ffff);
        Se       = desp_high4 & 0x1f;
        if ((selector & 0xfffc) == 0)
            abort_with_error_code(13, 0);
        load_from_descriptor_table(selector, e);
        if (e[0] == 0 && e[1] == 0)
            abort_with_error_code(13, selector & 0xfffc);
        desp_low4  = e[0];
        desp_high4 = e[1];
        if (!(desp_high4 & (1 << 12)) || !(desp_high4 & ((1 << 11))))
            abort_with_error_code(13, selector & 0xfffc);
        dpl = (desp_high4 >> 13) & 3;
        if (dpl > cpl_var)
            abort_with_error_code(13, selector & 0xfffc);
        if (!(desp_high4 & (1 << 15)))
            abort_with_error_code(11, selector & 0xfffc);
        if (!(desp_high4 & (1 << 10)) && dpl < cpl_var) {
            load_from_TR(dpl, e);
            ke  = e[0];
            esp = e[1];
            if ((ke & 0xfffc) == 0)
                abort_with_error_code(10, ke & 0xfffc);
            if ((ke & 3) != dpl)
                abort_with_error_code(10, ke & 0xfffc);

            load_from_descriptor_table(ke, e);

            if (e[0] == 0 && e[1] == 0)
                abort_with_error_code(10, ke & 0xfffc);
            we = e[0];
            xe = e[1];
            re = (xe >> 13) & 3;
            if (re != dpl)
                abort_with_error_code(10, ke & 0xfffc);
            if (!(xe & (1 << 12)) || (xe & (1 << 11)) || !(xe & (1 << 9)))
                abort_with_error_code(10, ke & 0xfffc);
            if (!(xe & (1 << 15)))
                abort_with_error_code(10, ke & 0xfffc);
            Ue      = SS_mask_from_flags(segs[2].flags);
            Ve      = segs[2].base;
            SS_mask = SS_mask_from_flags(xe);
            qe      = calc_desp_base(we, xe);
            if (is_32_bit) {

                esp      = (esp - 4) & -1;
                mem8_loc = (qe + (esp & SS_mask)) & -1;
                st32_mem8_kernel_write(segs[2].selector);

                esp      = (esp - 4) & -1;
                mem8_loc = (qe + (esp & SS_mask)) & -1;
                st32_mem8_kernel_write(We);

                for (i = Se - 1; i >= 0; i--) {
                    // x = Xe(Ve + ((We + i * 4) & Ue));

                    esp      = (esp - 4) & -1;
                    mem8_loc = (qe + (esp & SS_mask)) & -1;
                    st32_mem8_kernel_write(x);
                }
            } else {

                esp      = (esp - 2) & -1;
                mem8_loc = (qe + (esp & SS_mask)) & -1;
                st16_mem8_kernel_write(segs[2].selector);

                esp      = (esp - 2) & -1;
                mem8_loc = (qe + (esp & SS_mask)) & -1;
                st16_mem8_kernel_write(We);

                for (i = Se - 1; i >= 0; i--) {
                    // x = Ye(Ve + ((We + i * 2) & Ue));

                    esp      = (esp - 2) & -1;
                    mem8_loc = (qe + (esp & SS_mask)) & -1;
                    st16_mem8_kernel_write(x);
                }
            }
            ue = 1;
        } else {
            esp     = We;
            SS_mask = SS_mask_from_flags(segs[2].flags);
            qe      = segs[2].base;
            ue      = 0;
        }
        if (is_32_bit) {

            esp      = (esp - 4) & -1;
            mem8_loc = (qe + (esp & SS_mask)) & -1;
            st32_mem8_kernel_write(segs[1].selector);

            esp      = (esp - 4) & -1;
            mem8_loc = (qe + (esp & SS_mask)) & -1;
            st32_mem8_kernel_write(oe);

        } else {

            esp      = (esp - 2) & -1;
            mem8_loc = (qe + (esp & SS_mask)) & -1;
            st16_mem8_kernel_write(segs[1].selector);

            esp      = (esp - 2) & -1;
            mem8_loc = (qe + (esp & SS_mask)) & -1;
            st16_mem8_kernel_write(oe);
        }
        if (ue) {
            ke = (ke & ~3) | dpl;
            set_segment_vars(2, ke, qe, calc_desp_limit(we, xe), xe);
        }
        selector = (selector & ~3) | dpl;
        set_segment_vars(1, selector, calc_desp_base(desp_low4, desp_high4), calc_desp_limit(desp_low4, desp_high4),
                         desp_high4);
        change_permission_level(dpl);
        regs[4] = (regs[4] & ~SS_mask) | ((esp)&SS_mask);
        eip = ve, physmem8_ptr = initial_mem_ptr = 0;
    }
}
void x86Internal::op_CALLF(bool is_32_bit, int selector, int Le, int oe)
{
    if (!(cr0 & (1 << 0)) || (eflags & 0x00020000)) {
        op_CALLF_not_protected_mode(is_32_bit, selector, Le, oe);
    } else {
        op_CALLF_protected_mode(is_32_bit, selector, Le, oe);
    }
}
void x86Internal::do_return_not_protected_mode(bool is_32_bit, bool is_iret, int imm16)
{
    int esp, selector, stack_eip, stack_eflags, SS_mask, qe, ef;
    SS_mask = 0xffff;
    esp     = regs[4];
    qe      = segs[2].base;
    if (is_32_bit == 1) {

        mem8_loc  = (qe + (esp & SS_mask)) & -1;
        stack_eip = ld32_mem8_kernel_read();
        esp       = (esp + 4) & -1;

        mem8_loc = (qe + (esp & SS_mask)) & -1;
        selector = ld32_mem8_kernel_read();
        esp      = (esp + 4) & -1;

        selector &= 0xffff;
        if (is_iret) {
            mem8_loc     = (qe + (esp & SS_mask)) & -1;
            stack_eflags = ld32_mem8_kernel_read();
            esp          = (esp + 4) & -1;
        }
    } else {

        mem8_loc  = (qe + (esp & SS_mask)) & -1;
        stack_eip = ld16_mem8_kernel_read();
        esp       = (esp + 2) & -1;

        mem8_loc = (qe + (esp & SS_mask)) & -1;
        selector = ld16_mem8_kernel_read();
        esp      = (esp + 2) & -1;

        if (is_iret) {
            mem8_loc     = (qe + (esp & SS_mask)) & -1;
            stack_eflags = ld16_mem8_kernel_read();
            esp          = (esp + 2) & -1;
        }
    }
    regs[4]          = (regs[4] & ~SS_mask) | ((esp + imm16) & SS_mask);
    segs[1].selector = selector;
    segs[1].base     = (selector << 4);
    eip = stack_eip, physmem8_ptr = initial_mem_ptr = 0;
    if (is_iret) {
        if (eflags & 0x00020000)
            ef = 0x00000100 | 0x00040000 | 0x00200000 | 0x00000200 | 0x00010000 | 0x00004000;
        else
            ef = 0x00000100 | 0x00040000 | 0x00200000 | 0x00000200 | 0x00003000 | 0x00010000 | 0x00004000;
        if (is_32_bit == 0)
            ef &= 0xffff;
        set_FLAGS(stack_eflags, ef);
    }
    init_segment_local_vars();
}
void x86Internal::do_return_protected_mode(bool is_32_bit, bool is_iret, int imm16)
{
    int selector, stack_eflags, gf;
    int hf, jf, kf, lf;
    int desp_low4, desp_high4, we, xe;
    int cpl_var, dpl, rpl, ef, iopl;
    int qe, esp, stack_eip, wd, SS_mask;
    int e[2];

    SS_mask      = SS_mask_from_flags(segs[2].flags);
    esp          = regs[4];
    qe           = segs[2].base;
    stack_eflags = 0;
    if (is_32_bit == 1) {

        mem8_loc  = (qe + (esp & SS_mask)) & -1;
        stack_eip = ld32_mem8_kernel_read();
        esp       = (esp + 4) & -1;

        mem8_loc = (qe + (esp & SS_mask)) & -1;
        selector = ld32_mem8_kernel_read();    // CS selector
        esp      = (esp + 4) & -1;

        selector &= 0xffff;
        if (is_iret) {

            mem8_loc     = (qe + (esp & SS_mask)) & -1;
            stack_eflags = ld32_mem8_kernel_read();
            esp          = (esp + 4) & -1;

            if (stack_eflags & 0x00020000) {    // eflags.VM (return to v86 mode)

                mem8_loc = (qe + (esp & SS_mask)) & -1;
                wd       = ld32_mem8_kernel_read();
                esp      = (esp + 4) & -1;

                // pop segment selectors from stack

                mem8_loc = (qe + (esp & SS_mask)) & -1;
                gf       = ld32_mem8_kernel_read();
                esp      = (esp + 4) & -1;

                mem8_loc = (qe + (esp & SS_mask)) & -1;
                hf       = ld32_mem8_kernel_read();
                esp      = (esp + 4) & -1;

                mem8_loc = (qe + (esp & SS_mask)) & -1;
                jf       = ld32_mem8_kernel_read();
                esp      = (esp + 4) & -1;

                mem8_loc = (qe + (esp & SS_mask)) & -1;
                kf       = ld32_mem8_kernel_read();
                esp      = (esp + 4) & -1;

                mem8_loc = (qe + (esp & SS_mask)) & -1;
                lf       = ld32_mem8_kernel_read();
                esp      = (esp + 4) & -1;

                set_FLAGS(stack_eflags, 0x00000100 | 0x00040000 | 0x00200000 | 0x00000200 | 0x00003000 | 0x00020000 |
                                            0x00004000 | 0x00080000 | 0x00100000);
                init_segment_vars_with_selector(1, selector & 0xffff);
                change_permission_level(3);
                init_segment_vars_with_selector(2, gf & 0xffff);
                init_segment_vars_with_selector(0, hf & 0xffff);
                init_segment_vars_with_selector(3, jf & 0xffff);
                init_segment_vars_with_selector(4, kf & 0xffff);
                init_segment_vars_with_selector(5, lf & 0xffff);
                eip = stack_eip & 0xffff, physmem8_ptr = initial_mem_ptr = 0;
                regs[4] = (regs[4] & ~SS_mask) | ((wd)&SS_mask);
                return;
            }
        }
    } else {

        mem8_loc  = (qe + (esp & SS_mask)) & -1;
        stack_eip = ld16_mem8_kernel_read();
        esp       = (esp + 2) & -1;

        mem8_loc = (qe + (esp & SS_mask)) & -1;
        selector = ld16_mem8_kernel_read();
        esp      = (esp + 2) & -1;

        if (is_iret) {
            mem8_loc     = (qe + (esp & SS_mask)) & -1;
            stack_eflags = ld16_mem8_kernel_read();
            esp          = (esp + 2) & -1;
        }
    }
    if ((selector & 0xfffc) == 0)
        abort_with_error_code(13, selector & 0xfffc);

    load_from_descriptor_table(selector, e);
    if (e[0] == 0 && e[1] == 0)
        abort_with_error_code(13, selector & 0xfffc);

    desp_low4  = e[0];
    desp_high4 = e[1];
    if (!(desp_high4 & (1 << 12)) || !(desp_high4 & (1 << 11)))
        abort_with_error_code(13, selector & 0xfffc);
    cpl_var = cpl;
    rpl     = selector & 3;
    if (rpl < cpl_var)
        abort_with_error_code(13, selector & 0xfffc);
    dpl = (desp_high4 >> 13) & 3;
    if (desp_high4 & (1 << 10)) {
        if (dpl > rpl)
            abort_with_error_code(13, selector & 0xfffc);
    } else {
        if (dpl != rpl)
            abort_with_error_code(13, selector & 0xfffc);
    }
    if (!(desp_high4 & (1 << 15)))
        abort_with_error_code(11, selector & 0xfffc);
    esp = (esp + imm16) & -1;
    if (rpl == cpl_var) {
        set_segment_vars(1, selector, calc_desp_base(desp_low4, desp_high4), calc_desp_limit(desp_low4, desp_high4),
                         desp_high4);
    } else {
        if (is_32_bit == 1) {

            mem8_loc = (qe + (esp & SS_mask)) & -1;
            wd       = ld32_mem8_kernel_read();
            esp      = (esp + 4) & -1;

            mem8_loc = (qe + (esp & SS_mask)) & -1;
            gf       = ld32_mem8_kernel_read();
            esp      = (esp + 4) & -1;

            gf &= 0xffff;
        } else {

            mem8_loc = (qe + (esp & SS_mask)) & -1;
            wd       = ld16_mem8_kernel_read();
            esp      = (esp + 2) & -1;

            mem8_loc = (qe + (esp & SS_mask)) & -1;
            gf       = ld16_mem8_kernel_read();
            esp      = (esp + 2) & -1;
        }
        if ((gf & 0xfffc) == 0) {
            abort_with_error_code(13, 0);
        } else {
            if ((gf & 3) != rpl)
                abort_with_error_code(13, gf & 0xfffc);

            load_from_descriptor_table(gf, e);
            if (e[0] == 0 && e[1] == 0)
                abort_with_error_code(13, gf & 0xfffc);

            we = e[0];
            xe = e[1];
            if (!(xe & (1 << 12)) || (xe & (1 << 11)) || !(xe & (1 << 9)))
                abort_with_error_code(13, gf & 0xfffc);

            dpl = (xe >> 13) & 3;
            if (dpl != rpl)
                abort_with_error_code(13, gf & 0xfffc);
            if (!(xe & (1 << 15)))
                abort_with_error_code(11, gf & 0xfffc);

            set_segment_vars(2, gf, calc_desp_base(we, xe), calc_desp_limit(we, xe), xe);
        }
        set_segment_vars(1, selector, calc_desp_base(desp_low4, desp_high4), calc_desp_limit(desp_low4, desp_high4),
                         desp_high4);
        change_permission_level(rpl);
        esp     = wd;
        SS_mask = SS_mask_from_flags(xe);
        Pe(0, rpl);
        Pe(3, rpl);
        Pe(4, rpl);
        Pe(5, rpl);
        esp = (esp + imm16) & -1;
    }
    regs[4] = (regs[4] & ~SS_mask) | ((esp)&SS_mask);
    eip = stack_eip, physmem8_ptr = initial_mem_ptr = 0;
    if (is_iret) {
        ef = 0x00000100 | 0x00040000 | 0x00200000 | 0x00010000 | 0x00004000;
        if (cpl_var == 0)
            ef |= 0x00003000;
        iopl = (eflags >> 12) & 3;
        if (cpl_var <= iopl)
            ef |= 0x00000200;
        if (is_32_bit == 0)
            ef &= 0xffff;
        set_FLAGS(stack_eflags, ef);
    }
}
void x86Internal::op_IRET(bool is_32_bit)
{
    int iopl;
    if (!(cr0 & (1 << 0)) || (eflags & 0x00020000)) {
        if (eflags & 0x00020000) {
            iopl = (eflags >> 12) & 3;
            if (iopl != 3)
                abort(13);
        }
        do_return_not_protected_mode(is_32_bit, 1, 0);
    } else {
        if (eflags & 0x00004000) {
            throw "unsupported task gate";
        } else {
            do_return_protected_mode(is_32_bit, 1, 0);
        }
    }
}
void x86Internal::op_RETF(bool is_32_bit, int imm16)
{
    if (!(cr0 & (1 << 0)) || (eflags & 0x00020000)) {
        do_return_not_protected_mode(is_32_bit, 0, imm16);
    } else {
        do_return_protected_mode(is_32_bit, 0, imm16);
    }
}
int x86Internal::of(int selector, bool is_lsl)
{
    int desp_low4, desp_high4, rpl, dpl, cpl_var, descriptor_type;
    int e[2];

    if ((selector & 0xfffc) == 0)
        return -1;

    load_from_descriptor_table(selector, e);
    if (e[0] == 0 && e[1] == 0)
        return -1;

    desp_low4  = e[0];
    desp_high4 = e[1];
    rpl        = selector & 3;
    dpl        = (desp_high4 >> 13) & 3;
    cpl_var    = cpl;
    if (desp_high4 & (1 << 12)) {
        if ((desp_high4 & (1 << 11)) && (desp_high4 & (1 << 10))) {
        } else {
            if (dpl < cpl_var || dpl < rpl)
                return -1;
        }
    } else {
        descriptor_type = (desp_high4 >> 8) & 0xf;
        // Valid descriptors fall through this switch. Invalid descriptors return null
        switch (descriptor_type) {
            case 1:
            case 2:
            case 3:
            case 9:
            case 11:
                break;
            case 4:
            case 5:
            case 12:
                if (is_lsl)
                    return -1;
                break;
            default:
                return -1;
        }
        if (dpl < cpl_var || dpl < rpl)
            return -1;
    }
    if (is_lsl) {
        return calc_desp_limit(desp_low4, desp_high4);
    } else {
        return desp_high4 & 0x00f0ff00;
    }
}
void x86Internal::op_LAR_LSL(bool is_32_bit, bool is_lsl)
{
    int x, mem8, reg_idx1, selector;
    if (!(cr0 & (1 << 0)) || (eflags & 0x00020000))
        abort(6);

    mem8     = phys_mem8[physmem8_ptr++];
    reg_idx1 = (mem8 >> 3) & 7;
    if ((mem8 >> 6) == 3) {
        selector = regs[mem8 & 7] & 0xffff;
    } else {
        mem8_loc = segment_translation(mem8);
        selector = ld_16bits_mem8_read();
    }

    x      = of(selector, is_lsl);
    cc_src = get_conditional_flags();

    if (x == -1) {
        cc_src &= ~0x0040;
    } else {
        cc_src |= 0x0040;
        if (is_32_bit)
            regs[reg_idx1] = x;
        else
            set_lower_word_in_register(reg_idx1, x);
    }
    cc_dst = ((cc_src >> 6) & 1) ^ 1;
    cc_op  = 24;
}
int x86Internal::segment_isnt_accessible(int selector, bool is_verw)
{
    int desp_low4, desp_high4, rpl, dpl, cpl_var;
    int e[2];

    if ((selector & 0xfffc) == 0)
        return 0;

    load_from_descriptor_table(selector, e);
    if (e[0] == 0 && e[1] == 0)
        return 0;

    desp_low4  = e[0];
    desp_high4 = e[1];
    if (!(desp_high4 & (1 << 12)))    // s bit (system == 0)
        return 0;
    rpl     = selector & 3;
    dpl     = (desp_high4 >> 13) & 3;
    cpl_var = cpl;
    if (desp_high4 & (1 << 11)) {    // code == 1, data == 0
        if (is_verw) {               // code segments are never writable
            return 0;
        } else {
            if (!(desp_high4 & (1 << 9)))
                return 1;
            if (!(desp_high4 & (1 << 10))) {
                if (dpl < cpl_var || dpl < rpl)
                    return 0;
            }
        }
    } else {
        if (dpl < cpl_var || dpl < rpl)    // data segments are always readable, if privilege is sufficient
            return 0;
        if (is_verw && !(desp_high4 & (1 << 9)))    // writable data segment
            return 0;
    }
    return 1;
}
void x86Internal::op_VERR_VERW(int selector, bool is_verw)
{
    int z;
    z      = segment_isnt_accessible(selector, is_verw);
    cc_src = get_conditional_flags();

    // clear eflags.zf if selector is accessible and (readable (for VERR) or writable (for VERW))
    if (z)
        cc_src |= 0x0040;
    else
        cc_src &= ~0x0040;
    cc_dst = ((cc_src >> 6) & 1) ^ 1;
    cc_op  = 24;
}
void x86Internal::op_ARPL()
{
    int mem8, x, y, reg_idx0;
    if (!(cr0 & (1 << 0)) || (eflags & 0x00020000))
        abort(6);
    mem8 = phys_mem8[physmem8_ptr++];
    if ((mem8 >> 6) == 3) {
        reg_idx0 = mem8 & 7;
        x        = regs[reg_idx0] & 0xffff;
    } else {
        mem8_loc = segment_translation(mem8);
        x        = ld_16bits_mem8_write();
    }
    y      = regs[(mem8 >> 3) & 7];
    cc_src = get_conditional_flags();
    if ((x & 3) < (y & 3)) {
        x = (x & ~3) | (y & 3);
        if ((mem8 >> 6) == 3) {
            set_lower_word_in_register(reg_idx0, x);
        } else {
            st16_mem8_write(x);
        }
        cc_src |= 0x0040;
    } else {
        cc_src &= ~0x0040;
    }
    cc_dst = ((cc_src >> 6) & 1) ^ 1;
    cc_op  = 24;
}
void x86Internal::op_CPUID()
{
    int eax;
    eax = regs[0];
    switch (eax) {
        case 0:    // eax == 0: vendor ID
            regs[0] = 1;
            regs[3] = 0x756e6547 & -1;
            regs[2] = 0x49656e69 & -1;
            regs[1] = 0x6c65746e & -1;
            break;
        case 1:    // eax == 1: processor info and feature flags
        default:
            regs[0] = (5 << 8) | (4 << 4) | 3;    // family | model | stepping
            regs[3] = 8 << 8;    // danluu: This is a mystery to me. This bit now indicates clflush line size, but must
                                 // have meant something else in the past.
            regs[1] = 0;
            regs[2] = (1 << 4);    // rdtsc support
            break;
    }
}
void x86Internal::op_AAM(int base)
{
    int wf, xf;
    if (base == 0)
        abort(0);
    wf      = regs[0] & 0xff;
    xf      = (wf / base) & -1;
    wf      = (wf % base);
    regs[0] = (regs[0] & ~0xffff) | wf | (xf << 8);
    cc_dst  = (((wf) << 24) >> 24);
    cc_op   = 12;
}
void x86Internal::op_AAD(int base)
{
    int wf, xf;
    wf      = regs[0] & 0xff;
    xf      = (regs[0] >> 8) & 0xff;
    wf      = (xf * base + wf) & 0xff;
    regs[0] = (regs[0] & ~0xffff) | wf;
    cc_dst  = (((wf) << 24) >> 24);
    cc_op   = 12;
}
void x86Internal::op_AAA()
{
    int Af, wf, xf, Bf, flag_bits;
    flag_bits = get_conditional_flags();
    Bf        = flag_bits & 0x0010;
    wf        = regs[0] & 0xff;
    xf        = (regs[0] >> 8) & 0xff;
    Af        = (wf > 0xf9);
    if (((wf & 0x0f) > 9) || Bf) {
        wf = (wf + 6) & 0x0f;
        xf = (xf + 1 + Af) & 0xff;
        flag_bits |= 0x0001 | 0x0010;
    } else {
        flag_bits &= ~(0x0001 | 0x0010);
        wf &= 0x0f;
    }
    regs[0] = (regs[0] & ~0xffff) | wf | (xf << 8);
    cc_src  = flag_bits;
    cc_dst  = ((cc_src >> 6) & 1) ^ 1;
    cc_op   = 24;
}
void x86Internal::op_AAS()
{
    int Af, wf, xf, Bf, flag_bits;
    flag_bits = get_conditional_flags();
    Bf        = flag_bits & 0x0010;
    wf        = regs[0] & 0xff;
    xf        = (regs[0] >> 8) & 0xff;
    Af        = (wf < 6);
    if (((wf & 0x0f) > 9) || Bf) {
        wf = (wf - 6) & 0x0f;
        xf = (xf - 1 - Af) & 0xff;
        flag_bits |= 0x0001 | 0x0010;
    } else {
        flag_bits &= ~(0x0001 | 0x0010);
        wf &= 0x0f;
    }
    regs[0] = (regs[0] & ~0xffff) | wf | (xf << 8);
    cc_src  = flag_bits;
    cc_dst  = ((cc_src >> 6) & 1) ^ 1;
    cc_op   = 24;
}
void x86Internal::op_DAA()
{
    int wf, Bf, Ef, flag_bits;
    flag_bits = get_conditional_flags();
    Ef        = flag_bits & 0x0001;
    Bf        = flag_bits & 0x0010;
    wf        = regs[0] & 0xff;
    flag_bits = 0;
    if (((wf & 0x0f) > 9) || Bf) {
        wf = (wf + 6) & 0xff;
        flag_bits |= 0x0010;
    }
    if ((wf > 0x9f) || Ef) {
        wf = (wf + 0x60) & 0xff;
        flag_bits |= 0x0001;
    }
    regs[0] = (regs[0] & ~0xff) | wf;
    flag_bits |= (wf == 0) << 6;
    flag_bits |= parity_LUT[wf] << 2;
    flag_bits |= (wf & 0x80);
    cc_src = flag_bits;
    cc_dst = ((cc_src >> 6) & 1) ^ 1;
    cc_op  = 24;
}
void x86Internal::op_DAS()
{
    int wf, Gf, Bf, Ef, flag_bits;
    flag_bits = get_conditional_flags();
    Ef        = flag_bits & 0x0001;
    Bf        = flag_bits & 0x0010;
    wf        = regs[0] & 0xff;
    flag_bits = 0;
    Gf        = wf;
    if (((wf & 0x0f) > 9) || Bf) {
        flag_bits |= 0x0010;
        if (wf < 6 || Ef)
            flag_bits |= 0x0001;
        wf = (wf - 6) & 0xff;
    }
    if ((Gf > 0x99) || Ef) {
        wf = (wf - 0x60) & 0xff;
        flag_bits |= 0x0001;
    }
    regs[0] = (regs[0] & ~0xff) | wf;
    flag_bits |= (wf == 0) << 6;
    flag_bits |= parity_LUT[wf] << 2;
    flag_bits |= (wf & 0x80);
    cc_src = flag_bits;
    cc_dst = ((cc_src >> 6) & 1) ^ 1;
    cc_op  = 24;
}
void x86Internal::checkOp_BOUND()
{
    int mem8, x, y, z;
    mem8 = phys_mem8[physmem8_ptr++];
    if ((mem8 >> 3) == 3)
        abort(6);
    mem8_loc = segment_translation(mem8);
    x        = ld_32bits_mem8_read();
    mem8_loc = (mem8_loc + 4) & -1;
    y        = ld_32bits_mem8_read();
    reg_idx1 = (mem8 >> 3) & 7;
    z        = regs[reg_idx1];
    if (z < x || z > y)
        abort(5);
}
void x86Internal::op_16_BOUND()
{
    int mem8, x, y, z;
    mem8 = phys_mem8[physmem8_ptr++];
    if ((mem8 >> 3) == 3)
        abort(6);
    mem8_loc = segment_translation(mem8);
    x        = (ld_16bits_mem8_read() << 16) >> 16;
    mem8_loc = (mem8_loc + 2) & -1;
    y        = (ld_16bits_mem8_read() << 16) >> 16;
    reg_idx1 = (mem8 >> 3) & 7;
    z        = (regs[reg_idx1] << 16) >> 16;
    if (z < x || z > y)
        abort(5);
}
void x86Internal::op_16_PUSHA()
{
    int x, y, reg_idx1;
    y        = (regs[4] - 16) >> 0;
    mem8_loc = ((y & SS_mask) + SS_base) >> 0;
    for (reg_idx1 = 7; reg_idx1 >= 0; reg_idx1--) {
        x = regs[reg_idx1];
        st16_mem8_write(x);
        mem8_loc = (mem8_loc + 2) >> 0;
    }
    regs[4] = (regs[4] & ~SS_mask) | ((y)&SS_mask);
}
void x86Internal::op_PUSHA()
{
    int x, y, reg_idx1;
    y        = (regs[4] - 32) >> 0;
    mem8_loc = ((y & SS_mask) + SS_base) >> 0;
    for (reg_idx1 = 7; reg_idx1 >= 0; reg_idx1--) {
        x = regs[reg_idx1];
        st32_mem8_write(x);
        mem8_loc = (mem8_loc + 4) >> 0;
    }
    regs[4] = (regs[4] & ~SS_mask) | ((y)&SS_mask);
}
void x86Internal::op_16_POPA()
{
    int reg_idx1;
    mem8_loc = ((regs[4] & SS_mask) + SS_base) >> 0;
    for (reg_idx1 = 7; reg_idx1 >= 0; reg_idx1--) {
        if (reg_idx1 != 4) {
            set_lower_word_in_register(reg_idx1, ld_16bits_mem8_read());
        }
        mem8_loc = (mem8_loc + 2) >> 0;
    }
    regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 16) & SS_mask);
}
void x86Internal::op_POPA()
{
    int reg_idx1;
    mem8_loc = ((regs[4] & SS_mask) + SS_base) >> 0;
    for (reg_idx1 = 7; reg_idx1 >= 0; reg_idx1--) {
        if (reg_idx1 != 4) {
            regs[reg_idx1] = ld_32bits_mem8_read();
        }
        mem8_loc = (mem8_loc + 4) >> 0;
    }
    regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 32) & SS_mask);
}
void x86Internal::op_16_LEAVE()
{
    int x, y;
    y        = regs[5];
    mem8_loc = ((y & SS_mask) + SS_base) >> 0;
    x        = ld_16bits_mem8_read();
    set_lower_word_in_register(5, x);
    regs[4] = (regs[4] & ~SS_mask) | ((y + 2) & SS_mask);
}
void x86Internal::op_LEAVE()
{
    int x, y;
    y        = regs[5];
    mem8_loc = ((y & SS_mask) + SS_base) >> 0;
    x        = ld_32bits_mem8_read();
    regs[5]  = x;
    regs[4]  = (regs[4] & ~SS_mask) | ((y + 4) & SS_mask);
}
void x86Internal::op_16_ENTER()
{
    int cf, Qf, le, Rf, x, Sf;
    cf = ld16_mem8_direct();
    Qf = phys_mem8[physmem8_ptr++];
    Qf &= 0x1f;
    le = regs[4];
    Rf = regs[5];

    le       = (le - 2) >> 0;
    mem8_loc = ((le & SS_mask) + SS_base) >> 0;
    st16_mem8_write(Rf);

    Sf = le;
    if (Qf != 0) {
        while (Qf > 1) {
            Rf       = (Rf - 2) >> 0;
            mem8_loc = ((Rf & SS_mask) + SS_base) >> 0;
            x        = ld_16bits_mem8_read();

            le       = (le - 2) >> 0;
            mem8_loc = ((le & SS_mask) + SS_base) >> 0;
            st16_mem8_write(x);

            Qf--;
        }

        le       = (le - 2) >> 0;
        mem8_loc = ((le & SS_mask) + SS_base) >> 0;
        st16_mem8_write(Sf);
    }
    le       = (le - cf) >> 0;
    mem8_loc = ((le & SS_mask) + SS_base) >> 0;
    ld_16bits_mem8_write();
    regs[5] = (regs[5] & ~SS_mask) | (Sf & SS_mask);
    regs[4] = le;
}
void x86Internal::op_ENTER()
{
    int cf, Qf, le, Rf, x, Sf;
    cf = ld16_mem8_direct();
    Qf = phys_mem8[physmem8_ptr++];
    Qf &= 0x1f;
    le = regs[4];
    Rf = regs[5];

    le       = (le - 4) >> 0;
    mem8_loc = ((le & SS_mask) + SS_base) >> 0;
    st32_mem8_write(Rf);

    Sf = le;
    if (Qf != 0) {
        while (Qf > 1) {
            Rf       = (Rf - 4) >> 0;
            mem8_loc = ((Rf & SS_mask) + SS_base) >> 0;
            x        = ld_32bits_mem8_read();

            le       = (le - 4) >> 0;
            mem8_loc = ((le & SS_mask) + SS_base) >> 0;
            st32_mem8_write(x);

            Qf--;
        }

        le       = (le - 4) >> 0;
        mem8_loc = ((le & SS_mask) + SS_base) >> 0;
        st32_mem8_write(Sf);
    }
    le       = (le - cf) >> 0;
    mem8_loc = ((le & SS_mask) + SS_base) >> 0;
    ld_32bits_mem8_write();
    regs[5] = (regs[5] & ~SS_mask) | (Sf & SS_mask);
    regs[4] = (regs[4] & ~SS_mask) | ((le)&SS_mask);
}
void x86Internal::op_16_load_far_pointer32(int Sb)
{
    int x, y, mem8;
    mem8 = phys_mem8[physmem8_ptr++];
    if ((mem8 >> 3) == 3)
        abort(6);
    mem8_loc = segment_translation(mem8);
    x        = ld_32bits_mem8_read();
    mem8_loc += 4;
    y = ld_16bits_mem8_read();
    set_segment_register(Sb, y);
    regs[(mem8 >> 3) & 7] = x;
}
void x86Internal::op_16_load_far_pointer16(int Sb)
{
    int x, y, mem8;
    mem8 = phys_mem8[physmem8_ptr++];
    if ((mem8 >> 3) == 3)
        abort(6);
    mem8_loc = segment_translation(mem8);
    x        = ld_16bits_mem8_read();
    mem8_loc += 2;
    y = ld_16bits_mem8_read();
    set_segment_register(Sb, y);
    set_lower_word_in_register((mem8 >> 3) & 7, x);
}

void x86Internal::op_16_INS()
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
        x        = ld16_port(Zf);
        mem8_loc = ((Yf & Xf) + segs[0].base) >> 0;
        st16_mem8_write(x);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 1)) & Xf);
        regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
        if (ag & Xf)
            physmem8_ptr = initial_mem_ptr;
    } else {
        x        = ld16_port(Zf);
        mem8_loc = ((Yf & Xf) + segs[0].base) >> 0;
        st16_mem8_write(x);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 1)) & Xf);
    }
}
void x86Internal::op_16_OUTS()
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
        x        = ld_16bits_mem8_read();
        st16_port(Zf, x);
        regs[6] = (cg & ~Xf) | ((cg + (df << 1)) & Xf);
        regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
        if (ag & Xf)
            physmem8_ptr = initial_mem_ptr;
    } else {
        mem8_loc = ((cg & Xf) + segs[Sb].base) >> 0;
        x        = ld_16bits_mem8_read();
        st16_port(Zf, x);
        regs[6] = (cg & ~Xf) | ((cg + (df << 1)) & Xf);
    }
}
void x86Internal::op_16_MOVS()
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

        x        = ld_16bits_mem8_read();
        mem8_loc = eg;
        st16_mem8_write(x);
        regs[6] = (cg & ~Xf) | ((cg + (df << 1)) & Xf);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 1)) & Xf);
        regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
        if (ag & Xf)
            physmem8_ptr = initial_mem_ptr;

    } else {
        x        = ld_16bits_mem8_read();
        mem8_loc = eg;
        st16_mem8_write(x);
        regs[6] = (cg & ~Xf) | ((cg + (df << 1)) & Xf);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 1)) & Xf);
    }
}
void x86Internal::op_16_STOS()
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

        st16_mem8_write(regs[0]);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 1)) & Xf);
        regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
        if (ag & Xf)
            physmem8_ptr = initial_mem_ptr;

    } else {
        st16_mem8_write(regs[0]);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 1)) & Xf);
    }
}
void x86Internal::op_16_CMPS()
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
        x        = ld_16bits_mem8_read();
        mem8_loc = eg;
        y        = ld_16bits_mem8_read();
        do_16bit_math(7, x, y);
        regs[6] = (cg & ~Xf) | ((cg + (df << 1)) & Xf);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 1)) & Xf);
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
        x        = ld_16bits_mem8_read();
        mem8_loc = eg;
        y        = ld_16bits_mem8_read();
        do_16bit_math(7, x, y);
        regs[6] = (cg & ~Xf) | ((cg + (df << 1)) & Xf);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 1)) & Xf);
    }
}
void x86Internal::op_16_LODS()
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
        x       = ld_16bits_mem8_read();
        regs[0] = (regs[0] & -65536) | x;
        regs[6] = (cg & ~Xf) | ((cg + (df << 1)) & Xf);
        regs[1] = ag = (ag & ~Xf) | ((ag - 1) & Xf);
        if (ag & Xf)
            physmem8_ptr = initial_mem_ptr;
    } else {
        x       = ld_16bits_mem8_read();
        regs[0] = (regs[0] & -65536) | x;
        regs[6] = (cg & ~Xf) | ((cg + (df << 1)) & Xf);
    }
}
void x86Internal::op_16_SCAS()
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
        x = ld_16bits_mem8_read();
        do_16bit_math(7, regs[0], x);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 1)) & Xf);
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
        x = ld_16bits_mem8_read();
        do_16bit_math(7, regs[0], x);
        regs[7] = (Yf & ~Xf) | ((Yf + (df << 1)) & Xf);
    }
}

int x86Internal::ld8_port(int port_num)
{
    return ioport_read(port_num);
}
int x86Internal::ld16_port(int port_num)
{
    return ioport_read(port_num);
}
int x86Internal::ld32_port(int port_num)
{
    return ioport_read(port_num);
}
void x86Internal::st8_port(int port_num, int x)
{
    ioport_write(port_num, x);
}
void x86Internal::st16_port(int port_num, int x)
{
    ioport_write(port_num, x);
}
void x86Internal::st32_port(int port_num, int x)
{
    ioport_write(port_num, x);
}
int x86Internal::ioport_read(int mem8_loc)
{
    int port = mem8_loc & (1024 - 1);
    switch (port) {
        case 0x80:
            // (function(mem8_loc, data) {})(mem8_loc);
            return 0;
        case 0x70:
        case 0x71:
            return cmos->ioport_read(mem8_loc);
        case 0x64:
            return kbd->read_status(mem8_loc);
        case 0x20:
        case 0x21:
            return pic->pics[0]->ioport_read(mem8_loc);
        case 0xa0:
        case 0xa1:
            return pic->pics[1]->ioport_read(mem8_loc);
        case 0x40:
        case 0x41:
        case 0x42:
        case 0x43:
            return pit->ioport_read(mem8_loc);
        case 0x61:
            return pit->speaker_ioport_read(mem8_loc);
        case 0x3f8:
        case 0x3f9:
        case 0x3fa:
        case 0x3fb:
        case 0x3fc:
        case 0x3fd:
        case 0x3fe:
        case 0x3ff:
            return serial->ioport_read(mem8_loc);
        default: {
            return 0xff;
        }
    }
}
void x86Internal::ioport_write(int mem8_loc, int data)
{
    int port = mem8_loc & (1024 - 1);
    switch (port) {
        case 0x80:
            // (function(mem8_loc, data) {})(mem8_loc, data);
            break;
        case 0x70:
        case 0x71:
            cmos->ioport_write(mem8_loc, data);
            break;
        case 0x64:
            kbd->write_command(mem8_loc, data);
            break;
        case 0x20:
        case 0x21:
            pic->pics[0]->ioport_write(mem8_loc, data);
            break;
        case 0xa0:
        case 0xa1:
            pic->pics[1]->ioport_write(mem8_loc, data);
            break;
        case 0x40:
        case 0x41:
        case 0x42:
        case 0x43:
            pit->ioport_write(mem8_loc, data);
            break;
        case 0x61:
            pit->speaker_ioport_write(mem8_loc, data);
            break;
        case 0x3f8:
        case 0x3f9:
        case 0x3fa:
        case 0x3fb:
        case 0x3fc:
        case 0x3fd:
        case 0x3fe:
        case 0x3ff:
            serial->ioport_write(mem8_loc, data);
            break;
    }
}
void x86Internal::abort_with_error_code(int intno, int error_code)
{
    cycle_count += (N_cycles - cycles_left);
    ErrorInfo errinf;
    errinf.intno      = intno;
    errinf.error_code = error_code;
    throw errinf;
}
void x86Internal::abort(int intno)
{
    abort_with_error_code(intno, 0);
}
