#include "x86.h"

int x86Internal::instruction(int _N_cycles, ErrorInfo interrupt)
{
    if (init(_N_cycles))
        return 257;

    do {
        check_opbyte();
        CS_flags = init_CS_flags;
        OPbyte |= CS_flags & 0x0100;

        while (true) {
            dump(OPbyte);
            switch (OPbyte) {
                case 0x66:    //   Operand-size override prefix
                    if (CS_flags == init_CS_flags)
                        operation_size_function(eip_offset, OPbyte);
                    if (init_CS_flags & 0x0100)
                        CS_flags &= ~0x0100;
                    else
                        CS_flags |= 0x0100;
                    OPbyte = phys_mem8[physmem8_ptr++];
                    OPbyte |= (CS_flags & 0x0100);
                    break;
                case 0x67:    //   Address-size override prefix
                    if (CS_flags == init_CS_flags)
                        operation_size_function(eip_offset, OPbyte);
                    if (init_CS_flags & 0x0080)
                        CS_flags &= ~0x0080;
                    else
                        CS_flags |= 0x0080;
                    OPbyte = phys_mem8[physmem8_ptr++];
                    OPbyte |= (CS_flags & 0x0100);
                    break;
                case 0xf0:    // LOCK   Assert LOCK# Signal Prefix
                    if (CS_flags == init_CS_flags)
                        operation_size_function(eip_offset, OPbyte);
                    CS_flags |= 0x0040;
                    OPbyte = phys_mem8[physmem8_ptr++];
                    OPbyte |= (CS_flags & 0x0100);
                    break;
                case 0xf2:    // REPNZ  eCX Repeat String Operation Prefix
                    if (CS_flags == init_CS_flags)
                        operation_size_function(eip_offset, OPbyte);
                    CS_flags |= 0x0020;
                    OPbyte = phys_mem8[physmem8_ptr++];
                    OPbyte |= (CS_flags & 0x0100);
                    break;
                case 0xf3:    // REPZ  eCX Repeat String Operation Prefix
                    if (CS_flags == init_CS_flags)
                        operation_size_function(eip_offset, OPbyte);
                    CS_flags |= 0x0010;
                    OPbyte = phys_mem8[physmem8_ptr++];
                    OPbyte |= (CS_flags & 0x0100);
                    break;
                case 0x26:    // ES ES  ES segment override prefix
                case 0x2e:    // CS CS  CS segment override prefix
                case 0x36:    // SS SS  SS segment override prefix
                case 0x3e:    // DS DS  DS segment override prefix
                    if (CS_flags == init_CS_flags)
                        operation_size_function(eip_offset, OPbyte);
                    CS_flags = (CS_flags & ~0x000f) | (((OPbyte >> 3) & 3) + 1);
                    OPbyte   = phys_mem8[physmem8_ptr++];
                    OPbyte |= (CS_flags & 0x0100);
                    break;
                case 0x64:    // FS FS  FS segment override prefix
                case 0x65:    // GS GS  GS segment override prefix
                    if (CS_flags == init_CS_flags)
                        operation_size_function(eip_offset, OPbyte);
                    CS_flags = (CS_flags & ~0x000f) | ((OPbyte & 7) + 1);
                    OPbyte   = phys_mem8[physmem8_ptr++];
                    OPbyte |= (CS_flags & 0x0100);
                    break;
                case 0xb0:    // MOV Ib Zb Move
                case 0xb1:
                case 0xb2:
                case 0xb3:
                case 0xb4:
                case 0xb5:
                case 0xb6:
                case 0xb7:
                    x = phys_mem8[physmem8_ptr++];    // r8
                    OPbyte &= 7;                      // last bits
                    last_tlb_val     = (OPbyte & 4) << 1;
                    regs[OPbyte & 3] = (regs[OPbyte & 3] & ~(0xff << last_tlb_val)) | (((x)&0xff) << last_tlb_val);
                    goto EXEC_LOOP;
                case 0xb8:    // MOV Ivqp Zvqp Move
                case 0xb9:
                case 0xba:
                case 0xbb:
                case 0xbc:
                case 0xbd:
                case 0xbe:
                case 0xbf: {
                    x = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) |
                        (phys_mem8[physmem8_ptr + 2] << 16) | (phys_mem8[physmem8_ptr + 3] << 24);
                    physmem8_ptr += 4;
                }
                    regs[OPbyte & 7] = x;
                    goto EXEC_LOOP;
                case 0x88:    // MOV Gb Eb Move
                    mem8     = phys_mem8[physmem8_ptr++];
                    reg_idx1 = (mem8 >> 3) & 7;
                    x        = (regs[reg_idx1 & 3] >> ((reg_idx1 & 4) << 1));
                    if ((mem8 >> 6) == 3) {
                        reg_idx0     = mem8 & 7;
                        last_tlb_val = (reg_idx0 & 4) << 1;
                        regs[reg_idx0 & 3] =
                            (regs[reg_idx0 & 3] & ~(0xff << last_tlb_val)) | (((x)&0xff) << last_tlb_val);
                    } else {
                        mem8_loc = segment_translation(mem8);

                        uint32_t mem8_locu = mem8_loc;
                        last_tlb_val       = tlb_write[mem8_locu >> 12];
                        if (last_tlb_val == -1) {
                            __st8_mem8_write(x);
                        } else {
                            phys_mem8[mem8_loc ^ last_tlb_val] = x;
                        }
                    }
                    goto EXEC_LOOP;
                case 0x89:    // MOV Gvqp Evqp Move
                    mem8 = phys_mem8[physmem8_ptr++];
                    x    = regs[(mem8 >> 3) & 7];
                    if ((mem8 >> 6) == 3) {
                        regs[mem8 & 7] = x;
                    } else {
                        mem8_loc = segment_translation(mem8);

                        uint32_t mem8_locu = mem8_loc;
                        last_tlb_val       = tlb_write[mem8_locu >> 12];
                        if ((last_tlb_val | mem8_loc) & 3) {
                            __st32_mem8_write(x);
                        } else {
                            phys_mem32[(mem8_loc ^ last_tlb_val) >> 2] = x;
                        }
                    }
                    goto EXEC_LOOP;
                case 0x8a:    // MOV Eb Gb Move
                    mem8 = phys_mem8[physmem8_ptr++];
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        x        = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1));
                    } else {
                        mem8_loc           = segment_translation(mem8);
                        uint32_t mem8_locu = mem8_loc;
                        int      idx       = mem8_locu >> 12;
                        x                  = (((last_tlb_val = tlb_read[idx]) == -1) ? __ld_8bits_mem8_read()
                                                                                     : phys_mem8[mem8_loc ^ last_tlb_val]);
                    }
                    reg_idx1           = (mem8 >> 3) & 7;
                    last_tlb_val       = (reg_idx1 & 4) << 1;
                    regs[reg_idx1 & 3] = (regs[reg_idx1 & 3] & ~(0xff << last_tlb_val)) | (((x)&0xff) << last_tlb_val);
                    goto EXEC_LOOP;
                case 0x8b:    // MOV Evqp Gvqp Move
                    mem8 = phys_mem8[physmem8_ptr++];
                    if ((mem8 >> 6) == 3) {
                        x = regs[mem8 & 7];
                    } else {
                        mem8_loc           = segment_translation(mem8);
                        uint32_t mem8_locu = mem8_loc;
                        int      idx       = mem8_locu >> 12;
                        last_tlb_val       = tlb_read[idx];
                        x                  = ((last_tlb_val | mem8_loc) & 3 ? __ld_32bits_mem8_read()
                                                                            : phys_mem32[(mem8_loc ^ last_tlb_val) >> 2]);
                    }
                    regs[(mem8 >> 3) & 7] = x;
                    goto EXEC_LOOP;
                case 0xa0:    // MOV Ob AL Move byte at (seg:offset) to AL
                    mem8_loc = segmented_mem8_loc_for_MOV();
                    x        = ld_8bits_mem8_read();
                    regs[0]  = (regs[0] & -256) | x;
                    goto EXEC_LOOP;
                case 0xa1:    // MOV Ovqp rAX Move dword at (seg:offset) to EAX
                    mem8_loc = segmented_mem8_loc_for_MOV();
                    x        = ld_32bits_mem8_read();
                    regs[0]  = x;
                    goto EXEC_LOOP;
                case 0xa2:    // MOV AL Ob Move AL to (seg:offset)
                    mem8_loc = segmented_mem8_loc_for_MOV();
                    st8_mem8_write(regs[0]);
                    goto EXEC_LOOP;
                case 0xa3:    // MOV rAX Ovqp Move EAX to (seg:offset)
                    mem8_loc = segmented_mem8_loc_for_MOV();
                    st32_mem8_write(regs[0]);
                    goto EXEC_LOOP;
                case 0xd7:    // XLAT (DS:)[rBX+AL] AL Table Look-up Translation
                    mem8_loc = (regs[3] + (regs[0] & 0xff)) >> 0;
                    if (CS_flags & 0x0080)
                        mem8_loc &= 0xffff;
                    reg_idx1 = CS_flags & 0x000f;
                    if (reg_idx1 == 0)
                        reg_idx1 = 3;
                    else
                        reg_idx1--;
                    mem8_loc = (mem8_loc + segs[reg_idx1].base) >> 0;
                    x        = ld_8bits_mem8_read();
                    set_word_in_register(0, x);
                    goto EXEC_LOOP;
                case 0xc6:    // MOV Ib Eb Move
                    mem8 = phys_mem8[physmem8_ptr++];
                    if ((mem8 >> 6) == 3) {
                        x = phys_mem8[physmem8_ptr++];
                        set_word_in_register(mem8 & 7, x);
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x        = phys_mem8[physmem8_ptr++];
                        st8_mem8_write(x);
                    }
                    goto EXEC_LOOP;
                case 0xc7:    // MOV Ivds Evqp Move
                    mem8 = phys_mem8[physmem8_ptr++];
                    if ((mem8 >> 6) == 3) {

                        x = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) |
                            (phys_mem8[physmem8_ptr + 2] << 16) | (phys_mem8[physmem8_ptr + 3] << 24);
                        physmem8_ptr += 4;

                        regs[mem8 & 7] = x;
                    } else {
                        mem8_loc = segment_translation(mem8);

                        x = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) |
                            (phys_mem8[physmem8_ptr + 2] << 16) | (phys_mem8[physmem8_ptr + 3] << 24);
                        physmem8_ptr += 4;

                        st32_mem8_write(x);
                    }
                    goto EXEC_LOOP;
                case 0x91:    //(90+r)  XCHG  r16/32  eAX     Exchange Register/Memory with Register
                case 0x92:
                case 0x93:
                case 0x94:
                case 0x95:
                case 0x96:
                case 0x97:
                    reg_idx1       = OPbyte & 7;
                    x              = regs[0];
                    regs[0]        = regs[reg_idx1];
                    regs[reg_idx1] = x;
                    goto EXEC_LOOP;
                case 0x86:    // XCHG  Gb Exchange Register/Memory with Register
                    mem8     = phys_mem8[physmem8_ptr++];
                    reg_idx1 = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        x        = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1));
                        set_word_in_register(reg_idx0, (regs[reg_idx1 & 3] >> ((reg_idx1 & 4) << 1)));
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x        = ld_8bits_mem8_write();
                        st8_mem8_write((regs[reg_idx1 & 3] >> ((reg_idx1 & 4) << 1)));
                    }
                    set_word_in_register(reg_idx1, x);
                    goto EXEC_LOOP;
                case 0x87:    // XCHG  Gvqp Exchange Register/Memory with Register
                    mem8     = phys_mem8[physmem8_ptr++];
                    reg_idx1 = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        reg_idx0       = mem8 & 7;
                        x              = regs[reg_idx0];
                        regs[reg_idx0] = regs[reg_idx1];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x        = ld_32bits_mem8_write();
                        st32_mem8_write(regs[reg_idx1]);
                    }
                    regs[reg_idx1] = x;
                    goto EXEC_LOOP;
                case 0x8e:    // MOV Ew Sw Move
                    mem8     = phys_mem8[physmem8_ptr++];
                    reg_idx1 = (mem8 >> 3) & 7;
                    if (reg_idx1 >= 6 || reg_idx1 == 1)
                        abort(6);
                    if ((mem8 >> 6) == 3) {
                        x = regs[mem8 & 7] & 0xffff;
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x        = ld_16bits_mem8_read();
                    }
                    set_segment_register(reg_idx1, x);
                    goto EXEC_LOOP;
                case 0x8c:    // MOV Sw Mw Move
                    mem8     = phys_mem8[physmem8_ptr++];
                    reg_idx1 = (mem8 >> 3) & 7;
                    if (reg_idx1 >= 6)
                        abort(6);
                    x = segs[reg_idx1].selector;
                    if ((mem8 >> 6) == 3) {
                        if ((((CS_flags >> 8) & 1) ^ 1)) {
                            regs[mem8 & 7] = x;
                        } else {
                            set_lower_word_in_register(mem8 & 7, x);
                        }
                    } else {
                        mem8_loc = segment_translation(mem8);
                        st16_mem8_write(x);
                    }
                    goto EXEC_LOOP;
                case 0xc4:    // LES Mp ES Load Far Pointer
                    op_16_load_far_pointer32(0);
                    goto EXEC_LOOP;
                case 0xc5:    // LDS Mp DS Load Far Pointer
                    op_16_load_far_pointer32(3);
                    goto EXEC_LOOP;
                case 0x00:    // ADD Gb Eb Add
                case 0x08:    // OR Gb Eb Logical Inclusive OR
                case 0x10:    // ADC Gb Eb Add with Carry
                case 0x18:    // SBB Gb Eb Integer Subtraction with Borrow
                case 0x20:    // AND Gb Eb Logical AND
                case 0x28:    // SUB Gb Eb Subtract
                case 0x30:    // XOR Gb Eb Logical Exclusive OR
                case 0x38:    // CMP Eb  Compare Two Operands
                    mem8            = phys_mem8[physmem8_ptr++];
                    conditional_var = OPbyte >> 3;
                    reg_idx1        = (mem8 >> 3) & 7;
                    y               = (regs[reg_idx1 & 3] >> ((reg_idx1 & 4) << 1));
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        set_word_in_register(
                            reg_idx0, do_8bit_math(conditional_var, (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1)), y));
                    } else {
                        mem8_loc = segment_translation(mem8);
                        if (conditional_var != 7) {
                            x = ld_8bits_mem8_write();
                            x = do_8bit_math(conditional_var, x, y);
                            st8_mem8_write(x);
                        } else {
                            x = ld_8bits_mem8_read();
                            do_8bit_math(7, x, y);
                        }
                    }
                    goto EXEC_LOOP;
                case 0x01:    // ADD Gvqp Evqp Add
                    mem8 = phys_mem8[physmem8_ptr++];
                    y    = regs[(mem8 >> 3) & 7];
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;

                        cc_src = y;
                        cc_dst = regs[reg_idx0] = (regs[reg_idx0] + cc_src) >> 0;
                        cc_op                   = 2;

                    } else {
                        mem8_loc = segment_translation(mem8);
                        x        = ld_32bits_mem8_write();

                        cc_src = y;
                        cc_dst = x = (x + cc_src) >> 0;
                        cc_op      = 2;

                        st32_mem8_write(x);
                    }
                    goto EXEC_LOOP;
                case 0x09:    // OR Gvqp Evqp Logical Inclusive OR
                case 0x11:    // ADC Gvqp Evqp Add with Carry
                case 0x19:    // SBB Gvqp Evqp Integer Subtraction with Borrow
                case 0x21:    // AND Gvqp Evqp Logical AND
                case 0x29:    // SUB Gvqp Evqp Subtract
                case 0x31:    // XOR Gvqp Evqp Logical Exclusive OR
                    mem8            = phys_mem8[physmem8_ptr++];
                    conditional_var = OPbyte >> 3;
                    y               = regs[(mem8 >> 3) & 7];
                    if ((mem8 >> 6) == 3) {
                        reg_idx0       = mem8 & 7;
                        regs[reg_idx0] = do_32bit_math(conditional_var, regs[reg_idx0], y);
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x        = ld_32bits_mem8_write();
                        x        = do_32bit_math(conditional_var, x, y);
                        st32_mem8_write(x);
                    }
                    goto EXEC_LOOP;
                case 0x39:    // CMP Evqp  Compare Two Operands
                    mem8            = phys_mem8[physmem8_ptr++];
                    conditional_var = OPbyte >> 3;
                    y               = regs[(mem8 >> 3) & 7];
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;

                        cc_src = y;
                        cc_dst = (regs[reg_idx0] - cc_src) >> 0;
                        cc_op  = 8;

                    } else {
                        mem8_loc = segment_translation(mem8);
                        x        = ld_32bits_mem8_read();

                        cc_src = y;
                        cc_dst = (x - cc_src) >> 0;
                        cc_op  = 8;
                    }
                    goto EXEC_LOOP;
                case 0x02:    // ADD Eb Gb Add
                case 0x0a:    // OR Eb Gb Logical Inclusive OR
                case 0x12:    // ADC Eb Gb Add with Carry
                case 0x1a:    // SBB Eb Gb Integer Subtraction with Borrow
                case 0x22:    // AND Eb Gb Logical AND
                case 0x2a:    // SUB Eb Gb Subtract
                case 0x32:    // XOR Eb Gb Logical Exclusive OR
                case 0x3a:    // CMP Gb  Compare Two Operands
                    mem8            = phys_mem8[physmem8_ptr++];
                    conditional_var = OPbyte >> 3;
                    reg_idx1        = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        y        = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1));
                    } else {
                        mem8_loc = segment_translation(mem8);
                        y        = ld_8bits_mem8_read();
                    }
                    set_word_in_register(
                        reg_idx1, do_8bit_math(conditional_var, (regs[reg_idx1 & 3] >> ((reg_idx1 & 4) << 1)), y));
                    goto EXEC_LOOP;
                case 0x03:    // ADD Evqp Gvqp Add
                    mem8     = phys_mem8[physmem8_ptr++];
                    reg_idx1 = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        y = regs[mem8 & 7];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        y        = ld_32bits_mem8_read();
                    }

                    cc_src = y;
                    cc_dst = regs[reg_idx1] = (regs[reg_idx1] + cc_src) >> 0;
                    cc_op                   = 2;

                    goto EXEC_LOOP;
                case 0x0b:    // OR Evqp Gvqp Logical Inclusive OR
                case 0x13:    // ADC Evqp Gvqp Add with Carry
                case 0x1b:    // SBB Evqp Gvqp Integer Subtraction with Borrow
                case 0x23:    // AND Evqp Gvqp Logical AND
                case 0x2b:    // SUB Evqp Gvqp Subtract
                case 0x33:    // XOR Evqp Gvqp Logical Exclusive OR
                    mem8            = phys_mem8[physmem8_ptr++];
                    conditional_var = OPbyte >> 3;
                    reg_idx1        = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        y = regs[mem8 & 7];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        y        = ld_32bits_mem8_read();
                    }
                    regs[reg_idx1] = do_32bit_math(conditional_var, regs[reg_idx1], y);
                    goto EXEC_LOOP;
                case 0x3b:    // CMP Gvqp  Compare Two Operands
                    mem8            = phys_mem8[physmem8_ptr++];
                    conditional_var = OPbyte >> 3;
                    reg_idx1        = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        y = regs[mem8 & 7];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        y        = ld_32bits_mem8_read();
                    }

                    cc_src = y;
                    cc_dst = (regs[reg_idx1] - cc_src) >> 0;
                    cc_op  = 8;

                    goto EXEC_LOOP;
                case 0x04:    // ADD Ib AL Add
                case 0x0c:    // OR Ib AL Logical Inclusive OR
                case 0x14:    // ADC Ib AL Add with Carry
                case 0x1c:    // SBB Ib AL Integer Subtraction with Borrow
                case 0x24:    // AND Ib AL Logical AND
                case 0x2c:    // SUB Ib AL Subtract
                case 0x34:    // XOR Ib AL Logical Exclusive OR
                case 0x3c:    // CMP AL  Compare Two Operands
                    y               = phys_mem8[physmem8_ptr++];
                    conditional_var = OPbyte >> 3;
                    set_word_in_register(0, do_8bit_math(conditional_var, regs[0] & 0xff, y));
                    goto EXEC_LOOP;
                case 0x05:    // ADD Ivds rAX Add
                {
                    y = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) |
                        (phys_mem8[physmem8_ptr + 2] << 16) | (phys_mem8[physmem8_ptr + 3] << 24);
                    physmem8_ptr += 4;

                    cc_src = y;
                    cc_dst = regs[0] = (regs[0] + cc_src) >> 0;
                    cc_op            = 2;
                }
                    goto EXEC_LOOP;
                case 0x0d:    // OR Ivds rAX Logical Inclusive OR
                case 0x15:    // ADC Ivds rAX Add with Carry
                case 0x1d:    // SBB Ivds rAX Integer Subtraction with Borrow
                case 0x25:    // AND Ivds rAX Logical AND
                case 0x2d:    // SUB Ivds rAX Subtract
                {
                    y = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) |
                        (phys_mem8[physmem8_ptr + 2] << 16) | (phys_mem8[physmem8_ptr + 3] << 24);
                    physmem8_ptr += 4;

                    conditional_var = OPbyte >> 3;
                    regs[0]         = do_32bit_math(conditional_var, regs[0], y);
                }
                    goto EXEC_LOOP;
                case 0x35:    // XOR Ivds rAX Logical Exclusive OR
                {
                    y = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) |
                        (phys_mem8[physmem8_ptr + 2] << 16) | (phys_mem8[physmem8_ptr + 3] << 24);
                    physmem8_ptr += 4;

                    cc_dst = regs[0] = regs[0] ^ y;
                    cc_op            = 14;
                }
                    goto EXEC_LOOP;
                case 0x3d:    // CMP rAX  Compare Two Operands
                {
                    y = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) |
                        (phys_mem8[physmem8_ptr + 2] << 16) | (phys_mem8[physmem8_ptr + 3] << 24);
                    physmem8_ptr += 4;

                    cc_src = y;
                    cc_dst = (regs[0] - cc_src) >> 0;
                    cc_op  = 8;
                }
                    goto EXEC_LOOP;
                case 0x80:    // ADD Ib Eb Add
                case 0x82:    // ADD Ib Eb Add
                    mem8            = phys_mem8[physmem8_ptr++];
                    conditional_var = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        y        = phys_mem8[physmem8_ptr++];
                        set_word_in_register(
                            reg_idx0, do_8bit_math(conditional_var, (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1)), y));
                    } else {
                        mem8_loc = segment_translation(mem8);
                        y        = phys_mem8[physmem8_ptr++];
                        if (conditional_var != 7) {
                            x = ld_8bits_mem8_write();
                            x = do_8bit_math(conditional_var, x, y);
                            st8_mem8_write(x);
                        } else {
                            x = ld_8bits_mem8_read();
                            do_8bit_math(7, x, y);
                        }
                    }
                    goto EXEC_LOOP;
                case 0x81:    // ADD Ivds Evqp Add
                    mem8            = phys_mem8[physmem8_ptr++];
                    conditional_var = (mem8 >> 3) & 7;
                    if (conditional_var == 7) {
                        if ((mem8 >> 6) == 3) {
                            x = regs[mem8 & 7];
                        } else {
                            mem8_loc = segment_translation(mem8);
                            x        = ld_32bits_mem8_read();
                        }

                        y = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) |
                            (phys_mem8[physmem8_ptr + 2] << 16) | (phys_mem8[physmem8_ptr + 3] << 24);
                        physmem8_ptr += 4;

                        cc_src = y;
                        cc_dst = (x - cc_src) >> 0;
                        cc_op  = 8;

                    } else {
                        if ((mem8 >> 6) == 3) {
                            reg_idx0 = mem8 & 7;

                            y = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) |
                                (phys_mem8[physmem8_ptr + 2] << 16) | (phys_mem8[physmem8_ptr + 3] << 24);
                            physmem8_ptr += 4;

                            regs[reg_idx0] = do_32bit_math(conditional_var, regs[reg_idx0], y);
                        } else {
                            mem8_loc = segment_translation(mem8);

                            y = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) |
                                (phys_mem8[physmem8_ptr + 2] << 16) | (phys_mem8[physmem8_ptr + 3] << 24);
                            physmem8_ptr += 4;

                            x = ld_32bits_mem8_write();
                            x = do_32bit_math(conditional_var, x, y);
                            st32_mem8_write(x);
                        }
                    }
                    goto EXEC_LOOP;
                case 0x83:    // ADD Ibs Evqp Add
                    mem8            = phys_mem8[physmem8_ptr++];
                    conditional_var = (mem8 >> 3) & 7;
                    if (conditional_var == 7) {
                        if ((mem8 >> 6) == 3) {
                            x = regs[mem8 & 7];
                        } else {
                            mem8_loc = segment_translation(mem8);
                            x        = ld_32bits_mem8_read();
                        }
                        y = ((phys_mem8[physmem8_ptr++] << 24) >> 24);

                        cc_src = y;
                        cc_dst = (x - cc_src) >> 0;
                        cc_op  = 8;

                    } else {
                        if ((mem8 >> 6) == 3) {
                            reg_idx0       = mem8 & 7;
                            y              = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                            regs[reg_idx0] = do_32bit_math(conditional_var, regs[reg_idx0], y);
                        } else {
                            mem8_loc = segment_translation(mem8);
                            y        = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                            x        = ld_32bits_mem8_write();
                            x        = do_32bit_math(conditional_var, x, y);
                            st32_mem8_write(x);
                        }
                    }
                    goto EXEC_LOOP;
                case 0x40:    // INC  Zv Increment by 1
                case 0x41:    // REX.B   Extension of r/m field, base field, or opcode reg field
                case 0x42:    // REX.X   Extension of SIB index field
                case 0x43:    // REX.XB   REX.X and REX.B combination
                case 0x44:    // REX.R   Extension of ModR/M reg field
                case 0x45:    // REX.RB   REX.R and REX.B combination
                case 0x46:    // REX.RX   REX.R and REX.X combination
                case 0x47:    // REX.RXB   REX.R, REX.X and REX.B combination
                {
                    reg_idx1 = OPbyte & 7;

                    if (cc_op < 25) {
                        cc_op2  = cc_op;
                        cc_dst2 = cc_dst;
                    }
                    regs[reg_idx1] = cc_dst = (regs[reg_idx1] + 1) >> 0;
                    cc_op                   = 27;
                }
                    goto EXEC_LOOP;
                case 0x48:    // DEC  Zv Decrement by 1
                case 0x49:    // REX.WB   REX.W and REX.B combination
                case 0x4a:    // REX.WX   REX.W and REX.X combination
                case 0x4b:    // REX.WXB   REX.W, REX.X and REX.B combination
                case 0x4c:    // REX.WR   REX.W and REX.R combination
                case 0x4d:    // REX.WRB   REX.W, REX.R and REX.B combination
                case 0x4e:    // REX.WRX   REX.W, REX.R and REX.X combination
                case 0x4f:    // REX.WRXB   REX.W, REX.R, REX.X and REX.B combination
                {
                    reg_idx1 = OPbyte & 7;

                    if (cc_op < 25) {
                        cc_op2  = cc_op;
                        cc_dst2 = cc_dst;
                    }
                    regs[reg_idx1] = cc_dst = (regs[reg_idx1] - 1) >> 0;
                    cc_op                   = 30;
                }
                    goto EXEC_LOOP;
                case 0x6b:    // IMUL Evqp Gvqp Signed Multiply
                    mem8     = phys_mem8[physmem8_ptr++];
                    reg_idx1 = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        y = regs[mem8 & 7];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        y        = ld_32bits_mem8_read();
                    }
                    z              = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                    regs[reg_idx1] = op_IMUL32(y, z);
                    goto EXEC_LOOP;
                case 0x69:    // IMUL Evqp Gvqp Signed Multiply
                    mem8     = phys_mem8[physmem8_ptr++];
                    reg_idx1 = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        y = regs[mem8 & 7];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        y        = ld_32bits_mem8_read();
                    }

                    z = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) |
                        (phys_mem8[physmem8_ptr + 2] << 16) | (phys_mem8[physmem8_ptr + 3] << 24);
                    physmem8_ptr += 4;

                    regs[reg_idx1] = op_IMUL32(y, z);
                    goto EXEC_LOOP;
                case 0x84:    // TEST Eb  Logical Compare
                    mem8 = phys_mem8[physmem8_ptr++];
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        x        = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1));
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x        = ld_8bits_mem8_read();
                    }
                    reg_idx1 = (mem8 >> 3) & 7;
                    y        = (regs[reg_idx1 & 3] >> ((reg_idx1 & 4) << 1));

                    cc_dst = (((x & y) << 24) >> 24);
                    cc_op  = 12;

                    goto EXEC_LOOP;
                case 0x85:    // TEST Evqp  Logical Compare
                    mem8 = phys_mem8[physmem8_ptr++];
                    if ((mem8 >> 6) == 3) {
                        x = regs[mem8 & 7];
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x        = ld_32bits_mem8_read();
                    }
                    y = regs[(mem8 >> 3) & 7];

                    cc_dst = x & y;
                    cc_op  = 14;

                    goto EXEC_LOOP;
                case 0xa8:    // TEST AL  Logical Compare
                    y = phys_mem8[physmem8_ptr++];

                    cc_dst = (((regs[0] & y) << 24) >> 24);
                    cc_op  = 12;

                    goto EXEC_LOOP;
                case 0xa9:    // TEST rAX  Logical Compare
                {
                    y = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) |
                        (phys_mem8[physmem8_ptr + 2] << 16) | (phys_mem8[physmem8_ptr + 3] << 24);
                    physmem8_ptr += 4;

                    cc_dst = regs[0] & y;
                    cc_op  = 14;
                }
                    goto EXEC_LOOP;
                case 0xf6:    // TEST Eb  Logical Compare
                    mem8            = phys_mem8[physmem8_ptr++];
                    conditional_var = (mem8 >> 3) & 7;
                    switch (conditional_var) {
                        case 0:
                            if ((mem8 >> 6) == 3) {
                                reg_idx0 = mem8 & 7;
                                x        = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1));
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_8bits_mem8_read();
                            }
                            y = phys_mem8[physmem8_ptr++];

                            cc_dst = (((x & y) << 24) >> 24);
                            cc_op  = 12;

                            break;
                        case 2:
                            if ((mem8 >> 6) == 3) {
                                reg_idx0 = mem8 & 7;
                                set_word_in_register(reg_idx0, ~(regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1)));
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_8bits_mem8_write();
                                x        = ~x;
                                st8_mem8_write(x);
                            }
                            break;
                        case 3:
                            if ((mem8 >> 6) == 3) {
                                reg_idx0 = mem8 & 7;
                                set_word_in_register(reg_idx0,
                                                     do_8bit_math(5, 0, (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1))));
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_8bits_mem8_write();
                                x        = do_8bit_math(5, 0, x);
                                st8_mem8_write(x);
                            }
                            break;
                        case 4:
                            if ((mem8 >> 6) == 3) {
                                reg_idx0 = mem8 & 7;
                                x        = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1));
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_8bits_mem8_read();
                            }
                            set_lower_word_in_register(0, op_MUL(regs[0], x));
                            break;
                        case 5:
                            if ((mem8 >> 6) == 3) {
                                reg_idx0 = mem8 & 7;
                                x        = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1));
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_8bits_mem8_read();
                            }
                            set_lower_word_in_register(0, op_IMUL(regs[0], x));
                            break;
                        case 6:
                            if ((mem8 >> 6) == 3) {
                                reg_idx0 = mem8 & 7;
                                x        = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1));
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_8bits_mem8_read();
                            }
                            op_DIV(x);
                            break;
                        case 7:
                            if ((mem8 >> 6) == 3) {
                                reg_idx0 = mem8 & 7;
                                x        = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1));
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_8bits_mem8_read();
                            }
                            op_IDIV(x);
                            break;
                        default:
                            abort(6);
                    }
                    goto EXEC_LOOP;
                case 0xf7:    // TEST Evqp  Logical Compare
                    mem8            = phys_mem8[physmem8_ptr++];
                    conditional_var = (mem8 >> 3) & 7;
                    switch (conditional_var) {
                        case 0:
                            if ((mem8 >> 6) == 3) {
                                x = regs[mem8 & 7];
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_32bits_mem8_read();
                            }

                            y = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) |
                                (phys_mem8[physmem8_ptr + 2] << 16) | (phys_mem8[physmem8_ptr + 3] << 24);
                            physmem8_ptr += 4;

                            cc_dst = x & y;
                            cc_op  = 14;

                            break;
                        case 2:
                            if ((mem8 >> 6) == 3) {
                                reg_idx0       = mem8 & 7;
                                regs[reg_idx0] = ~regs[reg_idx0];
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_32bits_mem8_write();
                                x        = ~x;
                                st32_mem8_write(x);
                            }
                            break;
                        case 3:
                            if ((mem8 >> 6) == 3) {
                                reg_idx0       = mem8 & 7;
                                regs[reg_idx0] = do_32bit_math(5, 0, regs[reg_idx0]);
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_32bits_mem8_write();
                                x        = do_32bit_math(5, 0, x);
                                st32_mem8_write(x);
                            }
                            break;
                        case 4:
                            if ((mem8 >> 6) == 3) {
                                x = regs[mem8 & 7];
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_32bits_mem8_read();
                            }
                            regs[0] = op_MUL32(regs[0], x);
                            regs[2] = v;
                            break;
                        case 5:
                            if ((mem8 >> 6) == 3) {
                                x = regs[mem8 & 7];
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_32bits_mem8_read();
                            }
                            regs[0] = op_IMUL32(regs[0], x);
                            regs[2] = v;
                            break;
                        case 6:
                            if ((mem8 >> 6) == 3) {
                                x = regs[mem8 & 7];
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_32bits_mem8_read();
                            }
                            regs[0] = op_DIV32(regs[2], regs[0], x);
                            regs[2] = v;
                            break;
                        case 7:
                            if ((mem8 >> 6) == 3) {
                                x = regs[mem8 & 7];
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_32bits_mem8_read();
                            }
                            regs[0] = op_IDIV32(regs[2], regs[0], x);
                            regs[2] = v;
                            break;
                        default:
                            abort(6);
                    }
                    goto EXEC_LOOP;
                // Rotate and Shift ops ---------------------------------------------------------------
                case 0xc0:    // ROL Ib Eb Rotate
                    mem8            = phys_mem8[physmem8_ptr++];
                    conditional_var = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        y        = phys_mem8[physmem8_ptr++];
                        reg_idx0 = mem8 & 7;
                        set_word_in_register(reg_idx0,
                                             shift8(conditional_var, (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1)), y));
                    } else {
                        mem8_loc = segment_translation(mem8);
                        y        = phys_mem8[physmem8_ptr++];
                        x        = ld_8bits_mem8_write();
                        x        = shift8(conditional_var, x, y);
                        st8_mem8_write(x);
                    }
                    goto EXEC_LOOP;
                case 0xc1:    // ROL Ib Evqp Rotate
                    mem8            = phys_mem8[physmem8_ptr++];
                    conditional_var = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        y              = phys_mem8[physmem8_ptr++];
                        reg_idx0       = mem8 & 7;
                        regs[reg_idx0] = shift32(conditional_var, regs[reg_idx0], y);
                    } else {
                        mem8_loc = segment_translation(mem8);
                        y        = phys_mem8[physmem8_ptr++];
                        x        = ld_32bits_mem8_write();
                        x        = shift32(conditional_var, x, y);
                        st32_mem8_write(x);
                    }
                    goto EXEC_LOOP;
                case 0xd0:    // ROL 1 Eb Rotate
                    mem8            = phys_mem8[physmem8_ptr++];
                    conditional_var = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        set_word_in_register(reg_idx0,
                                             shift8(conditional_var, (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1)), 1));
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x        = ld_8bits_mem8_write();
                        x        = shift8(conditional_var, x, 1);
                        st8_mem8_write(x);
                    }
                    goto EXEC_LOOP;
                case 0xd1:    // ROL 1 Evqp Rotate
                    mem8            = phys_mem8[physmem8_ptr++];
                    conditional_var = (mem8 >> 3) & 7;
                    if ((mem8 >> 6) == 3) {
                        reg_idx0       = mem8 & 7;
                        regs[reg_idx0] = shift32(conditional_var, regs[reg_idx0], 1);
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x        = ld_32bits_mem8_write();
                        x        = shift32(conditional_var, x, 1);
                        st32_mem8_write(x);
                    }
                    goto EXEC_LOOP;
                case 0xd2:    // ROL CL Eb Rotate
                    mem8            = phys_mem8[physmem8_ptr++];
                    conditional_var = (mem8 >> 3) & 7;
                    y               = regs[1] & 0xff;
                    if ((mem8 >> 6) == 3) {
                        reg_idx0 = mem8 & 7;
                        set_word_in_register(reg_idx0,
                                             shift8(conditional_var, (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1)), y));
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x        = ld_8bits_mem8_write();
                        x        = shift8(conditional_var, x, y);
                        st8_mem8_write(x);
                    }
                    goto EXEC_LOOP;
                case 0xd3:    // ROL CL Evqp Rotate
                    mem8            = phys_mem8[physmem8_ptr++];
                    conditional_var = (mem8 >> 3) & 7;
                    y               = regs[1] & 0xff;
                    if ((mem8 >> 6) == 3) {
                        reg_idx0       = mem8 & 7;
                        regs[reg_idx0] = shift32(conditional_var, regs[reg_idx0], y);
                    } else {
                        mem8_loc = segment_translation(mem8);
                        x        = ld_32bits_mem8_write();
                        x        = shift32(conditional_var, x, y);
                        st32_mem8_write(x);
                    }
                    goto EXEC_LOOP;
                case 0x98:    // CBW AL AX Convert Byte to Word
                    regs[0] = (regs[0] << 16) >> 16;
                    goto EXEC_LOOP;
                case 0x99:    // CWD AX DX Convert Word to Doubleword
                    regs[2] = regs[0] >> 31;
                    goto EXEC_LOOP;
                case 0x50:    // PUSH Zv SS:[rSP] Push Word, Doubleword or Quadword Onto the Stack
                case 0x51:
                case 0x52:
                case 0x53:
                case 0x54:
                case 0x55:
                case 0x56:
                case 0x57:
                    x = regs[OPbyte & 7];
                    if (FS_usage_flag) {
                        mem8_loc           = (regs[4] - 4) >> 0;
                        uint32_t mem8_locu = mem8_loc;
                        {
                            last_tlb_val = tlb_write[mem8_locu >> 12];
                            if ((last_tlb_val | mem8_loc) & 3) {
                                __st32_mem8_write(x);
                            } else {
                                phys_mem32[(mem8_loc ^ last_tlb_val) >> 2] = x;
                            }
                        }
                        regs[4] = mem8_loc;
                    } else {
                        push_dword_to_stack(x);
                    }
                    goto EXEC_LOOP;
                case 0x58:    // POP SS:[rSP] Zv Pop a Value from the Stack
                case 0x59:
                case 0x5a:
                case 0x5b:
                case 0x5c:
                case 0x5d:
                case 0x5e:
                case 0x5f:
                    if (FS_usage_flag) {
                        mem8_loc           = regs[4];
                        uint32_t mem8_locu = mem8_loc;
                        last_tlb_val       = tlb_read[mem8_locu >> 12];
                        bool     flg       = (last_tlb_val | mem8_loc) & 3;
                        uint32_t midx      = (mem8_loc ^ last_tlb_val) >> 2;

                        x       = (flg ? __ld_32bits_mem8_read() : phys_mem32[midx]);
                        regs[4] = (mem8_loc + 4) >> 0;
                    } else {
                        x = pop_dword_from_stack_read();
                        pop_dword_from_stack_incr_ptr();
                    }
                    regs[OPbyte & 7] = x;
                    goto EXEC_LOOP;

                case 0x60:    // PUSHA AX SS:[rSP] Push All General-Purpose Registers
                    op_PUSHA();
                    goto EXEC_LOOP;
                case 0x61:    // POPA SS:[rSP] DI Pop All General-Purpose Registers
                    op_POPA();
                    goto EXEC_LOOP;
                case 0x8f:    // POP SS:[rSP] Ev Pop a Value from the Stack
                    mem8 = phys_mem8[physmem8_ptr++];
                    if ((mem8 >> 6) == 3) {
                        x = pop_dword_from_stack_read();
                        pop_dword_from_stack_incr_ptr();
                        regs[mem8 & 7] = x;
                    } else {
                        x = pop_dword_from_stack_read();
                        y = regs[4];
                        pop_dword_from_stack_incr_ptr();
                        z        = regs[4];
                        mem8_loc = segment_translation(mem8);
                        regs[4]  = y;
                        st32_mem8_write(x);
                        regs[4] = z;
                    }
                    goto EXEC_LOOP;
                case 0x68:    // PUSH Ivs SS:[rSP] Push Word, Doubleword or Quadword Onto the Stack
                {
                    x = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) |
                        (phys_mem8[physmem8_ptr + 2] << 16) | (phys_mem8[physmem8_ptr + 3] << 24);
                    physmem8_ptr += 4;

                    if (FS_usage_flag) {
                        mem8_loc = (regs[4] - 4) >> 0;
                        st32_mem8_write(x);
                        regs[4] = mem8_loc;
                    } else {
                        push_dword_to_stack(x);
                    }
                }
                    goto EXEC_LOOP;
                case 0x6a:    // PUSH Ibss SS:[rSP] Push Word, Doubleword or Quadword Onto the Stack
                    x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                    if (FS_usage_flag) {
                        mem8_loc = (regs[4] - 4) >> 0;
                        st32_mem8_write(x);
                        regs[4] = mem8_loc;
                    } else {
                        push_dword_to_stack(x);
                    }
                    goto EXEC_LOOP;
                case 0xc8:    // ENTER Iw SS:[rSP] Make Stack Frame for Procedure Parameters
                    op_ENTER();
                    goto EXEC_LOOP;
                case 0xc9:    // LEAVE SS:[rSP] eBP High Level Procedure Exit
                    if (FS_usage_flag) {
                        mem8_loc = regs[5];
                        x        = ld_32bits_mem8_read();
                        regs[5]  = x;
                        regs[4]  = (mem8_loc + 4) >> 0;
                    } else {
                        op_LEAVE();
                    }
                    goto EXEC_LOOP;
                case 0x9c:    // PUSHF Flags SS:[rSP] Push FLAGS Register onto the Stack
                    iopl = (eflags >> 12) & 3;
                    if ((eflags & 0x00020000) && iopl != 3)
                        abort(13);
                    x = get_FLAGS() & ~(0x00020000 | 0x00010000);
                    if ((((CS_flags >> 8) & 1) ^ 1)) {
                        push_dword_to_stack(x);
                    } else {
                        push_word_to_stack(x);
                    }
                    goto EXEC_LOOP;
                case 0x9d:    // POPF SS:[rSP] Flags Pop Stack into FLAGS Register
                    iopl = (eflags >> 12) & 3;
                    if ((eflags & 0x00020000) && iopl != 3)
                        abort(13);
                    if ((((CS_flags >> 8) & 1) ^ 1)) {
                        x = pop_dword_from_stack_read();
                        pop_dword_from_stack_incr_ptr();
                        y = -1;
                    } else {
                        x = pop_word_from_stack_read();
                        pop_word_from_stack_incr_ptr();
                        y = 0xffff;
                    }
                    z = (0x00000100 | 0x00040000 | 0x00200000 | 0x00004000);
                    if (cpl == 0) {
                        z |= 0x00000200 | 0x00003000;
                    } else {
                        if (cpl <= iopl)
                            z |= 0x00000200;
                    }
                    set_FLAGS(x, z & y);

                    if (hard_irq != 0 && (eflags & 0x00000200))
                        goto OUTER_LOOP;

                    goto EXEC_LOOP;
                case 0x06:    // PUSH ES SS:[rSP] Push Word, Doubleword or Quadword Onto the Stack
                case 0x0e:    // PUSH CS SS:[rSP] Push Word, Doubleword or Quadword Onto the Stack
                case 0x16:    // PUSH SS SS:[rSP] Push Word, Doubleword or Quadword Onto the Stack
                case 0x1e:    // PUSH DS SS:[rSP] Push Word, Doubleword or Quadword Onto the Stack
                    push_dword_to_stack(segs[OPbyte >> 3].selector);
                    goto EXEC_LOOP;
                case 0x07:    // POP SS:[rSP] ES Pop a Value from the Stack
                case 0x17:    // POP SS:[rSP] SS Pop a Value from the Stack
                case 0x1f:    // POP SS:[rSP] DS Pop a Value from the Stack
                    set_segment_register(OPbyte >> 3, pop_dword_from_stack_read() & 0xffff);
                    pop_dword_from_stack_incr_ptr();
                    goto EXEC_LOOP;
                case 0x8d:    // LEA M Gvqp Load Effective Address
                    mem8 = phys_mem8[physmem8_ptr++];
                    if ((mem8 >> 6) == 3)
                        abort(6);
                    CS_flags              = (CS_flags & ~0x000f) | (6 + 1);
                    regs[(mem8 >> 3) & 7] = segment_translation(mem8);
                    goto EXEC_LOOP;
                case 0xfe:    // INC  Eb Increment by 1
                    mem8            = phys_mem8[physmem8_ptr++];
                    conditional_var = (mem8 >> 3) & 7;
                    switch (conditional_var) {
                        case 0:
                            if ((mem8 >> 6) == 3) {
                                reg_idx0 = mem8 & 7;
                                set_word_in_register(reg_idx0,
                                                     increment_8bit((regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1))));
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_8bits_mem8_write();
                                x        = increment_8bit(x);
                                st8_mem8_write(x);
                            }
                            break;
                        case 1:
                            if ((mem8 >> 6) == 3) {
                                reg_idx0 = mem8 & 7;
                                set_word_in_register(reg_idx0,
                                                     decrement_8bit((regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1))));
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_8bits_mem8_write();
                                x        = decrement_8bit(x);
                                st8_mem8_write(x);
                            }
                            break;
                        default:
                            abort(6);
                    }
                    goto EXEC_LOOP;
                case 0xff:    // INC DEC CALL CALLF JMP JMPF PUSH
                    mem8            = phys_mem8[physmem8_ptr++];
                    conditional_var = (mem8 >> 3) & 7;
                    switch (conditional_var) {
                        case 0:    // INC  Evqp Increment by 1
                            if ((mem8 >> 6) == 3) {
                                reg_idx0 = mem8 & 7;

                                if (cc_op < 25) {
                                    cc_op2  = cc_op;
                                    cc_dst2 = cc_dst;
                                }
                                regs[reg_idx0] = cc_dst = (regs[reg_idx0] + 1) >> 0;
                                cc_op                   = 27;

                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_32bits_mem8_write();

                                if (cc_op < 25) {
                                    cc_op2  = cc_op;
                                    cc_dst2 = cc_dst;
                                }
                                x = cc_dst = (x + 1) >> 0;
                                cc_op      = 27;

                                st32_mem8_write(x);
                            }
                            break;
                        case 1:    // DEC
                            if ((mem8 >> 6) == 3) {
                                reg_idx0 = mem8 & 7;

                                if (cc_op < 25) {
                                    cc_op2  = cc_op;
                                    cc_dst2 = cc_dst;
                                }
                                regs[reg_idx0] = cc_dst = (regs[reg_idx0] - 1) >> 0;
                                cc_op                   = 30;

                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_32bits_mem8_write();

                                if (cc_op < 25) {
                                    cc_op2  = cc_op;
                                    cc_dst2 = cc_dst;
                                }
                                x = cc_dst = (x - 1) >> 0;
                                cc_op      = 30;

                                st32_mem8_write(x);
                            }
                            break;
                        case 2:    // CALL
                            if ((mem8 >> 6) == 3) {
                                x = regs[mem8 & 7];
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_32bits_mem8_read();
                            }
                            y = (eip + physmem8_ptr - initial_mem_ptr);
                            if (FS_usage_flag) {
                                mem8_loc = (regs[4] - 4) >> 0;
                                st32_mem8_write(y);
                                regs[4] = mem8_loc;
                            } else {
                                push_dword_to_stack(y);
                            }
                            eip = x, physmem8_ptr = initial_mem_ptr = 0;
                            break;
                        case 4:    // JMP
                            if ((mem8 >> 6) == 3) {
                                x = regs[mem8 & 7];
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_32bits_mem8_read();
                            }
                            eip = x, physmem8_ptr = initial_mem_ptr = 0;
                            break;
                        case 6:    // PUSH
                            if ((mem8 >> 6) == 3) {
                                x = regs[mem8 & 7];
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_32bits_mem8_read();
                            }
                            if (FS_usage_flag) {
                                mem8_loc = (regs[4] - 4) >> 0;
                                st32_mem8_write(x);
                                regs[4] = mem8_loc;
                            } else {
                                push_dword_to_stack(x);
                            }
                            break;
                        case 3:    // CALLF
                        case 5:    // JMPF
                            if ((mem8 >> 6) == 3)
                                abort(6);
                            mem8_loc = segment_translation(mem8);
                            x        = ld_32bits_mem8_read();
                            mem8_loc = (mem8_loc + 4) >> 0;
                            y        = ld_16bits_mem8_read();
                            if (conditional_var == 3)
                                op_CALLF(1, y, x, (eip + physmem8_ptr - initial_mem_ptr));
                            else
                                op_JMPF(y, x);
                            break;
                        default:
                            abort(6);
                    }
                    goto EXEC_LOOP;
                case 0xeb:    // JMP Jbs  Jump
                    x            = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                    physmem8_ptr = (physmem8_ptr + x) >> 0;
                    goto EXEC_LOOP;
                case 0xe9:    // JMP Jvds  Jump
                {
                    x = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) |
                        (phys_mem8[physmem8_ptr + 2] << 16) | (phys_mem8[physmem8_ptr + 3] << 24);
                    physmem8_ptr += 4;

                    physmem8_ptr = (physmem8_ptr + x) >> 0;
                }
                    goto EXEC_LOOP;
                case 0xea:    // JMPF Ap  Jump
                    if ((((CS_flags >> 8) & 1) ^ 1)) {
                        {
                            x = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) |
                                (phys_mem8[physmem8_ptr + 2] << 16) | (phys_mem8[physmem8_ptr + 3] << 24);
                            physmem8_ptr += 4;
                        }
                    } else {
                        x = ld16_mem8_direct();
                    }
                    y = ld16_mem8_direct();
                    op_JMPF(y, x);
                    goto EXEC_LOOP;
                case 0x70:    // JO Jbs  Jump short if overflow (OF=1)
                    if (check_overflow()) {
                        x            = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                        physmem8_ptr = (physmem8_ptr + x) >> 0;
                    } else {
                        physmem8_ptr = (physmem8_ptr + 1) >> 0;
                    }
                    goto EXEC_LOOP;
                case 0x71:    // JNO Jbs  Jump short if not overflow (OF=0)
                    if (!check_overflow()) {
                        x            = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                        physmem8_ptr = (physmem8_ptr + x) >> 0;
                    } else {
                        physmem8_ptr = (physmem8_ptr + 1) >> 0;
                    }
                    goto EXEC_LOOP;
                case 0x72:    // JB Jbs  Jump short if below/not above or equal/carry (CF=1)
                    if (check_carry()) {
                        x            = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                        physmem8_ptr = (physmem8_ptr + x) >> 0;
                    } else {
                        physmem8_ptr = (physmem8_ptr + 1) >> 0;
                    }
                    goto EXEC_LOOP;
                case 0x73:    // JNB Jbs  Jump short if not below/above or equal/not carry (CF=0)
                    if (!check_carry()) {
                        x            = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                        physmem8_ptr = (physmem8_ptr + x) >> 0;
                    } else {
                        physmem8_ptr = (physmem8_ptr + 1) >> 0;
                    }
                    goto EXEC_LOOP;
                case 0x74:    // JZ Jbs  Jump short if zero/equal (ZF=0)
                    if (cc_dst == 0) {
                        x            = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                        physmem8_ptr = (physmem8_ptr + x) >> 0;
                    } else {
                        physmem8_ptr = (physmem8_ptr + 1) >> 0;
                    }
                    goto EXEC_LOOP;
                case 0x75:    // JNZ Jbs  Jump short if not zero/not equal (ZF=1)
                    if (!(cc_dst == 0)) {
                        x            = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                        physmem8_ptr = (physmem8_ptr + x) >> 0;
                    } else {
                        physmem8_ptr = (physmem8_ptr + 1) >> 0;
                    }
                    goto EXEC_LOOP;
                case 0x76:    // JBE Jbs  Jump short if below or equal/not above (CF=1 AND ZF=1)
                    if (check_below_or_equal()) {
                        x            = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                        physmem8_ptr = (physmem8_ptr + x) >> 0;
                    } else {
                        physmem8_ptr = (physmem8_ptr + 1) >> 0;
                    }
                    goto EXEC_LOOP;
                case 0x77:    // JNBE Jbs  Jump short if not below or equal/above (CF=0 AND ZF=0)
                    if (!check_below_or_equal()) {
                        x            = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                        physmem8_ptr = (physmem8_ptr + x) >> 0;
                    } else {
                        physmem8_ptr = (physmem8_ptr + 1) >> 0;
                    }
                    goto EXEC_LOOP;
                case 0x78:    // JS Jbs  Jump short if sign (SF=1)
                    if ((cc_op == 24 ? ((cc_src >> 7) & 1) : (cc_dst < 0))) {
                        x            = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                        physmem8_ptr = (physmem8_ptr + x) >> 0;
                    } else {
                        physmem8_ptr = (physmem8_ptr + 1) >> 0;
                    }
                    goto EXEC_LOOP;
                case 0x79:    // JNS Jbs  Jump short if not sign (SF=0)
                    if (!(cc_op == 24 ? ((cc_src >> 7) & 1) : (cc_dst < 0))) {
                        x            = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                        physmem8_ptr = (physmem8_ptr + x) >> 0;
                    } else {
                        physmem8_ptr = (physmem8_ptr + 1) >> 0;
                    }
                    goto EXEC_LOOP;
                case 0x7a:    // JP Jbs  Jump short if parity/parity even (PF=1)
                    if (check_parity()) {
                        x            = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                        physmem8_ptr = (physmem8_ptr + x) >> 0;
                    } else {
                        physmem8_ptr = (physmem8_ptr + 1) >> 0;
                    }
                    goto EXEC_LOOP;
                case 0x7b:    // JNP Jbs  Jump short if not parity/parity odd
                    if (!check_parity()) {
                        x            = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                        physmem8_ptr = (physmem8_ptr + x) >> 0;
                    } else {
                        physmem8_ptr = (physmem8_ptr + 1) >> 0;
                    }
                    goto EXEC_LOOP;
                case 0x7c:    // JL Jbs  Jump short if less/not greater (SF!=OF)
                    if (check_less_than()) {
                        x            = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                        physmem8_ptr = (physmem8_ptr + x) >> 0;
                    } else {
                        physmem8_ptr = (physmem8_ptr + 1) >> 0;
                    }
                    goto EXEC_LOOP;
                case 0x7d:    // JNL Jbs  Jump short if not less/greater or equal (SF=OF)
                    if (!check_less_than()) {
                        x            = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                        physmem8_ptr = (physmem8_ptr + x) >> 0;
                    } else {
                        physmem8_ptr = (physmem8_ptr + 1) >> 0;
                    }
                    goto EXEC_LOOP;
                case 0x7e:    // JLE Jbs  Jump short if less or equal/not greater ((ZF=1) OR (SF!=OF))
                    if (check_less_or_equal()) {
                        x            = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                        physmem8_ptr = (physmem8_ptr + x) >> 0;
                    } else {
                        physmem8_ptr = (physmem8_ptr + 1) >> 0;
                    }
                    goto EXEC_LOOP;
                case 0x7f:    // JNLE Jbs  Jump short if not less nor equal/greater ((ZF=0) AND (SF=OF))
                    if (!check_less_or_equal()) {
                        x            = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                        physmem8_ptr = (physmem8_ptr + x) >> 0;
                    } else {
                        physmem8_ptr = (physmem8_ptr + 1) >> 0;
                    }
                    goto EXEC_LOOP;
                case 0xe0:    // LOOPNZ Jbs eCX Decrement count; Jump short if count!=0 and ZF=0
                case 0xe1:    // LOOPZ Jbs eCX Decrement count; Jump short if count!=0 and ZF=1
                case 0xe2:    // LOOP Jbs eCX Decrement count; Jump short if count!=0
                    x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                    if (CS_flags & 0x0080)
                        conditional_var = 0xffff;
                    else
                        conditional_var = -1;
                    y       = (regs[1] - 1) & conditional_var;
                    regs[1] = (regs[1] & ~conditional_var) | y;
                    OPbyte &= 3;
                    if (OPbyte == 0)
                        z = !(cc_dst == 0);
                    else if (OPbyte == 1)
                        z = (cc_dst == 0);
                    else
                        z = 1;
                    if (y && z) {
                        if (CS_flags & 0x0100) {
                            eip          = (eip + physmem8_ptr - initial_mem_ptr + x) & 0xffff,
                            physmem8_ptr = initial_mem_ptr = 0;
                        } else {
                            physmem8_ptr = (physmem8_ptr + x) >> 0;
                        }
                    }
                    goto EXEC_LOOP;
                case 0xe3:    // JCXZ Jbs  Jump short if eCX register is 0
                    x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                    if (CS_flags & 0x0080)
                        conditional_var = 0xffff;
                    else
                        conditional_var = -1;
                    if ((regs[1] & conditional_var) == 0) {
                        if (CS_flags & 0x0100) {
                            eip          = (eip + physmem8_ptr - initial_mem_ptr + x) & 0xffff,
                            physmem8_ptr = initial_mem_ptr = 0;
                        } else {
                            physmem8_ptr = (physmem8_ptr + x) >> 0;
                        }
                    }
                    goto EXEC_LOOP;
                case 0xc2:    // RETN SS:[rSP]  Return from procedure
                    y       = (ld16_mem8_direct() << 16) >> 16;
                    x       = pop_dword_from_stack_read();
                    regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 4 + y) & SS_mask);
                    eip = x, physmem8_ptr = initial_mem_ptr = 0;
                    goto EXEC_LOOP;
                case 0xc3:    // RETN SS:[rSP]  Return from procedure
                    if (FS_usage_flag) {
                        mem8_loc = regs[4];
                        x        = ld_32bits_mem8_read();
                        regs[4]  = (regs[4] + 4) >> 0;
                    } else {
                        x = pop_dword_from_stack_read();
                        pop_dword_from_stack_incr_ptr();
                    }
                    eip = x, physmem8_ptr = initial_mem_ptr = 0;
                    goto EXEC_LOOP;
                case 0xe8:    // CALL Jvds SS:[rSP] Call Procedure
                {
                    x = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) |
                        (phys_mem8[physmem8_ptr + 2] << 16) | (phys_mem8[physmem8_ptr + 3] << 24);
                    physmem8_ptr += 4;

                    y = (eip + physmem8_ptr - initial_mem_ptr);
                    if (FS_usage_flag) {
                        mem8_loc = (regs[4] - 4) >> 0;
                        st32_mem8_write(y);
                        regs[4] = mem8_loc;
                    } else {
                        push_dword_to_stack(y);
                    }
                    physmem8_ptr = (physmem8_ptr + x) >> 0;
                }
                    goto EXEC_LOOP;
                case 0x9a:    // CALLF Ap SS:[rSP] Call Procedure
                    z = (((CS_flags >> 8) & 1) ^ 1);
                    if (z) {

                        x = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) |
                            (phys_mem8[physmem8_ptr + 2] << 16) | (phys_mem8[physmem8_ptr + 3] << 24);
                        physmem8_ptr += 4;

                    } else {
                        x = ld16_mem8_direct();
                    }
                    y = ld16_mem8_direct();
                    op_CALLF(z, y, x, (eip + physmem8_ptr - initial_mem_ptr));

                    if (hard_irq != 0 && (eflags & 0x00000200))
                        goto OUTER_LOOP;

                    goto EXEC_LOOP;
                case 0xca:                                   // RETF Iw  Return from procedure
                    y = (ld16_mem8_direct() << 16) >> 16;    // 16 bit immediate field
                    op_RETF((((CS_flags >> 8) & 1) ^ 1), y);

                    if (hard_irq != 0 && (eflags & 0x00000200))
                        goto OUTER_LOOP;

                    goto EXEC_LOOP;
                case 0xcb:    // RETF SS:[rSP]  Return from procedure
                    op_RETF((((CS_flags >> 8) & 1) ^ 1), 0);

                    if (hard_irq != 0 && (eflags & 0x00000200))
                        goto OUTER_LOOP;

                    goto EXEC_LOOP;
                case 0xcf:    // IRET SS:[rSP] Flags Interrupt Return
                    op_IRET((((CS_flags >> 8) & 1) ^ 1));

                    if (hard_irq != 0 && (eflags & 0x00000200))
                        goto OUTER_LOOP;

                    goto EXEC_LOOP;
                case 0x90:    // XCHG  Zvqp Exchange Register/Memory with Register
                    goto EXEC_LOOP;
                case 0xcc:    // INT 3 SS:[rSP] Call to Interrupt Procedure
                    y = (eip + physmem8_ptr - initial_mem_ptr);
                    do_interrupt(3, 1, 0, y, 0);
                    goto EXEC_LOOP;
                case 0xcd:    // INT Ib SS:[rSP] Call to Interrupt Procedure
                    x = phys_mem8[physmem8_ptr++];
                    if ((eflags & 0x00020000) && ((eflags >> 12) & 3) != 3)
                        abort(13);
                    y = (eip + physmem8_ptr - initial_mem_ptr);
                    do_interrupt(x, 1, 0, y, 0);
                    goto EXEC_LOOP;
                case 0xce:    // INTO eFlags SS:[rSP] Call to Interrupt Procedure
                    if (check_overflow()) {
                        y = (eip + physmem8_ptr - initial_mem_ptr);
                        do_interrupt(4, 1, 0, y, 0);
                    }
                    goto EXEC_LOOP;
                case 0x62:    // BOUND Gv SS:[rSP] Check Array Index Against Bounds
                    checkOp_BOUND();
                    goto EXEC_LOOP;
                case 0xf5:    // CMC   Complement Carry Flag
                    cc_src = get_conditional_flags() ^ 0x0001;
                    cc_dst = ((cc_src >> 6) & 1) ^ 1;
                    cc_op  = 24;
                    goto EXEC_LOOP;
                case 0xf8:    // CLC   Clear Carry Flag
                    cc_src = get_conditional_flags() & ~0x0001;
                    cc_dst = ((cc_src >> 6) & 1) ^ 1;
                    cc_op  = 24;
                    goto EXEC_LOOP;
                case 0xf9:    // STC   Set Carry Flag
                    cc_src = get_conditional_flags() | 0x0001;
                    cc_dst = ((cc_src >> 6) & 1) ^ 1;
                    cc_op  = 24;
                    goto EXEC_LOOP;
                case 0xfc:    // CLD   Clear Direction Flag
                    df = 1;
                    goto EXEC_LOOP;
                case 0xfd:    // STD   Set Direction Flag
                    df = -1;
                    goto EXEC_LOOP;
                case 0xfa:    // CLI   Clear Interrupt Flag
                    iopl = (eflags >> 12) & 3;
                    if (cpl > iopl)
                        abort(13);
                    eflags &= ~0x00000200;
                    goto EXEC_LOOP;
                case 0xfb:    // STI   Set Interrupt Flag
                    iopl = (eflags >> 12) & 3;
                    if (cpl > iopl)
                        abort(13);
                    eflags |= 0x00000200;

                    if (hard_irq != 0 && (eflags & 0x00000200))
                        goto OUTER_LOOP;

                    goto EXEC_LOOP;
                case 0x9e:    // SAHF AH  Store AH into Flags
                    cc_src = ((regs[0] >> 8) & (0x0080 | 0x0040 | 0x0010 | 0x0004 | 0x0001)) | (check_overflow() << 11);
                    cc_dst = ((cc_src >> 6) & 1) ^ 1;
                    cc_op  = 24;
                    goto EXEC_LOOP;
                case 0x9f:    // LAHF  AH Load Status Flags into AH Register
                    x = get_FLAGS();
                    set_word_in_register(4, x);
                    goto EXEC_LOOP;
                case 0xf4:    // HLT   Halt
                    if (cpl != 0)
                        abort(13);
                    halted    = 1;
                    exit_code = 257;
                    goto OUTER_LOOP;
                case 0xa4:    // MOVS (DS:)[rSI] (ES:)[rDI] Move Data from String to String
                    stringOp_MOVSB();
                    goto EXEC_LOOP;
                case 0xa5:    // MOVS DS:[SI] ES:[DI] Move Data from String to String
                    stringOp_MOVSD();
                    goto EXEC_LOOP;
                case 0xaa:    // STOS AL (ES:)[rDI] Store String
                    stringOp_STOSB();
                    goto EXEC_LOOP;
                case 0xab:    // STOS AX ES:[DI] Store String
                    stringOp_STOSD();
                    goto EXEC_LOOP;
                case 0xa6:    // CMPS (ES:)[rDI]  Compare String Operands
                    stringOp_CMPSB();
                    goto EXEC_LOOP;
                case 0xa7:    // CMPS ES:[DI]  Compare String Operands
                    stringOp_CMPSD();
                    goto EXEC_LOOP;
                case 0xac:    // LODS (DS:)[rSI] AL Load String
                    stringOp_LODSB();
                    goto EXEC_LOOP;
                case 0xad:    // LODS DS:[SI] AX Load String
                    stringOp_LODSD();
                    goto EXEC_LOOP;
                case 0xae:    // SCAS (ES:)[rDI]  Scan String
                    stringOp_SCASB();
                    goto EXEC_LOOP;
                case 0xaf:    // SCAS ES:[DI]  Scan String
                    stringOp_SCASD();
                    goto EXEC_LOOP;
                case 0x6c:    // INS DX (ES:)[rDI] Input from Port to String
                    stringOp_INSB();

                    if (hard_irq != 0 && (eflags & 0x00000200))
                        goto OUTER_LOOP;

                    goto EXEC_LOOP;
                case 0x6d:    // INS DX ES:[DI] Input from Port to String
                    stringOp_INSD();

                    if (hard_irq != 0 && (eflags & 0x00000200))
                        goto OUTER_LOOP;

                    goto EXEC_LOOP;
                case 0x6e:    // OUTS (DS):[rSI] DX Output String to Port
                    stringOp_OUTSB();

                    if (hard_irq != 0 && (eflags & 0x00000200))
                        goto OUTER_LOOP;

                    goto EXEC_LOOP;
                case 0x6f:    // OUTS DS:[SI] DX Output String to Port
                    stringOp_OUTSD();

                    if (hard_irq != 0 && (eflags & 0x00000200))
                        goto OUTER_LOOP;

                    goto EXEC_LOOP;
                case 0xd8:    // FADD Msr ST Add
                case 0xd9:    // FLD ESsr ST Load Floating Point Value
                case 0xda:    // FIADD Mdi ST Add
                case 0xdb:    // FILD Mdi ST Load Integer
                case 0xdc:    // FADD Mdr ST Add
                case 0xdd:    // FLD Mdr ST Load Floating Point Value
                case 0xde:    // FIADD Mwi ST Add
                case 0xdf:    // FILD Mwi ST Load Integer
                    if (cr0 & ((1 << 2) | (1 << 3))) {
                        abort(7);
                    }
                    mem8            = phys_mem8[physmem8_ptr++];
                    reg_idx1        = (mem8 >> 3) & 7;
                    reg_idx0        = mem8 & 7;
                    conditional_var = ((OPbyte & 7) << 3) | ((mem8 >> 3) & 7);
                    set_lower_word_in_register(0, 0xffff);
                    if ((mem8 >> 6) == 3) {
                    } else {
                        mem8_loc = segment_translation(mem8);
                    }
                    goto EXEC_LOOP;
                case 0x9b:    // FWAIT   Check pending unmasked floating-point exceptions
                    goto EXEC_LOOP;
                case 0xe4: {    // IN Ib AL Input from Port
                    iopl = (eflags >> 12) & 3;
                    if (cpl > iopl)
                        abort(13);
                    x = phys_mem8[physmem8_ptr++];
                    set_word_in_register(0, ld8_port(x));

                    if (hard_irq != 0 && (eflags & 0x00000200))
                        goto OUTER_LOOP;

                    goto EXEC_LOOP;
                } break;
                case 0xe5:    // IN Ib eAX Input from Port
                    iopl = (eflags >> 12) & 3;
                    if (cpl > iopl)
                        abort(13);
                    x       = phys_mem8[physmem8_ptr++];
                    regs[0] = ld32_port(x);

                    if (hard_irq != 0 && (eflags & 0x00000200))
                        goto OUTER_LOOP;

                    goto EXEC_LOOP;
                case 0xe6:    // OUT AL Ib Output to Port
                    iopl = (eflags >> 12) & 3;
                    if (cpl > iopl)
                        abort(13);
                    x = phys_mem8[physmem8_ptr++];
                    st8_port(x, regs[0] & 0xff);

                    if (hard_irq != 0 && (eflags & 0x00000200))
                        goto OUTER_LOOP;

                    goto EXEC_LOOP;
                case 0xe7:    // OUT eAX Ib Output to Port
                    iopl = (eflags >> 12) & 3;
                    if (cpl > iopl)
                        abort(13);
                    x = phys_mem8[physmem8_ptr++];
                    st32_port(x, regs[0]);

                    if (hard_irq != 0 && (eflags & 0x00000200))
                        goto OUTER_LOOP;

                    goto EXEC_LOOP;
                case 0xec:    // IN DX AL Input from Port
                    iopl = (eflags >> 12) & 3;
                    if (cpl > iopl)
                        abort(13);
                    set_word_in_register(0, ld8_port(regs[2] & 0xffff));

                    if (hard_irq != 0 && (eflags & 0x00000200))
                        goto OUTER_LOOP;

                    goto EXEC_LOOP;
                case 0xed:    // IN DX eAX Input from Port
                    iopl = (eflags >> 12) & 3;
                    if (cpl > iopl)
                        abort(13);
                    regs[0] = ld32_port(regs[2] & 0xffff);

                    if (hard_irq != 0 && (eflags & 0x00000200))
                        goto OUTER_LOOP;

                    goto EXEC_LOOP;
                case 0xee:    // OUT AL DX Output to Port
                    iopl = (eflags >> 12) & 3;
                    if (cpl > iopl)
                        abort(13);
                    st8_port(regs[2] & 0xffff, regs[0] & 0xff);

                    if (hard_irq != 0 && (eflags & 0x00000200))
                        goto OUTER_LOOP;

                    goto EXEC_LOOP;
                case 0xef:    // OUT eAX DX Output to Port
                    iopl = (eflags >> 12) & 3;
                    if (cpl > iopl)
                        abort(13);
                    st32_port(regs[2] & 0xffff, regs[0]);

                    if (hard_irq != 0 && (eflags & 0x00000200))
                        goto OUTER_LOOP;

                    goto EXEC_LOOP;
                case 0x27:    // DAA  AL Decimal Adjust AL after Addition
                    op_DAA();
                    goto EXEC_LOOP;
                case 0x2f:    // DAS  AL Decimal Adjust AL after Subtraction
                    op_DAS();
                    goto EXEC_LOOP;
                case 0x37:    // AAA  AL ASCII Adjust After Addition
                    op_AAA();
                    goto EXEC_LOOP;
                case 0x3f:    // AAS  AL ASCII Adjust AL After Subtraction
                    op_AAS();
                    goto EXEC_LOOP;
                case 0xd4:    // AAM  AL ASCII Adjust AX After Multiply
                    x = phys_mem8[physmem8_ptr++];
                    op_AAM(x);
                    goto EXEC_LOOP;
                case 0xd5:    // AAD  AL ASCII Adjust AX Before Division
                    x = phys_mem8[physmem8_ptr++];
                    op_AAD(x);
                    goto EXEC_LOOP;
                case 0x63:    // ARPL Ew  Adjust RPL Field of Segment Selector
                    op_ARPL();
                    goto EXEC_LOOP;
                case 0xd6:    // SALC   Undefined and Reserved; Does not Generate #UD
                case 0xf1:    // INT1   Undefined and Reserved; Does not Generate #UD
                    abort(6);
                    break;

                case 0x0f:
                    OPbyte = phys_mem8[physmem8_ptr++];
                    switch (OPbyte) {
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
                        {
                            x = phys_mem8[physmem8_ptr] | (phys_mem8[physmem8_ptr + 1] << 8) |
                                (phys_mem8[physmem8_ptr + 2] << 16) | (phys_mem8[physmem8_ptr + 3] << 24);
                            physmem8_ptr += 4;

                            if (check_status_bits_for_jump(OPbyte & 0xf))
                                physmem8_ptr = (physmem8_ptr + x) >> 0;
                        }
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
                        case 0x9e:    // SETLE  Eb Set Byte on Condition - less or equal/not greater ((ZF=1) OR
                                      // (SF!=OF))
                        case 0x9f:    // SETNLE  Eb Set Byte on Condition - not less nor equal/greater ((ZF=0) AND
                                      // (SF=OF))
                            mem8 = phys_mem8[physmem8_ptr++];
                            x    = check_status_bits_for_jump(OPbyte & 0xf);
                            if ((mem8 >> 6) == 3) {
                                set_word_in_register(mem8 & 7, x);
                            } else {
                                mem8_loc = segment_translation(mem8);
                                st8_mem8_write(x);
                            }
                            goto EXEC_LOOP;
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
                        case 0x4e:    // CMOVLE Evqp Gvqp Conditional Move - less or equal/not greater ((ZF=1) OR
                                      // (SF!=OF))
                        case 0x4f:    // CMOVNLE Evqp Gvqp Conditional Move - not less nor equal/greater ((ZF=0) AND
                                      // (SF=OF))
                            mem8 = phys_mem8[physmem8_ptr++];
                            if ((mem8 >> 6) == 3) {
                                x = regs[mem8 & 7];
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_32bits_mem8_read();
                            }
                            if (check_status_bits_for_jump(OPbyte & 0xf))
                                regs[(mem8 >> 3) & 7] = x;
                            goto EXEC_LOOP;
                        case 0xb6:    // MOVZX Eb Gvqp Move with Zero-Extend
                            mem8     = phys_mem8[physmem8_ptr++];
                            reg_idx1 = (mem8 >> 3) & 7;
                            if ((mem8 >> 6) == 3) {
                                reg_idx0 = mem8 & 7;
                                x        = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1)) & 0xff;
                            } else {
                                mem8_loc           = segment_translation(mem8);
                                uint32_t mem8_locu = mem8_loc;
                                x                  = (((last_tlb_val = tlb_read[mem8_locu >> 12]) == -1)
                                                          ? __ld_8bits_mem8_read()
                                                          : phys_mem8[mem8_loc ^ last_tlb_val]);
                            }
                            regs[reg_idx1] = x;
                            goto EXEC_LOOP;
                        case 0xb7:    // MOVZX Ew Gvqp Move with Zero-Extend
                            mem8     = phys_mem8[physmem8_ptr++];
                            reg_idx1 = (mem8 >> 3) & 7;
                            if ((mem8 >> 6) == 3) {
                                x = regs[mem8 & 7] & 0xffff;
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_16bits_mem8_read();
                            }
                            regs[reg_idx1] = x;
                            goto EXEC_LOOP;
                        case 0xbe:    // MOVSX Eb Gvqp Move with Sign-Extension
                            mem8     = phys_mem8[physmem8_ptr++];
                            reg_idx1 = (mem8 >> 3) & 7;
                            if ((mem8 >> 6) == 3) {
                                reg_idx0 = mem8 & 7;
                                x        = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1));
                            } else {
                                mem8_loc           = segment_translation(mem8);
                                uint32_t mem8_locu = mem8_loc;
                                x                  = (((last_tlb_val = tlb_read[mem8_locu >> 12]) == -1)
                                                          ? __ld_8bits_mem8_read()
                                                          : phys_mem8[mem8_loc ^ last_tlb_val]);
                            }
                            regs[reg_idx1] = (((x) << 24) >> 24);
                            goto EXEC_LOOP;
                        case 0xbf:    // MOVSX Ew Gvqp Move with Sign-Extension
                            mem8     = phys_mem8[physmem8_ptr++];
                            reg_idx1 = (mem8 >> 3) & 7;
                            if ((mem8 >> 6) == 3) {
                                x = regs[mem8 & 7];
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_16bits_mem8_read();
                            }
                            regs[reg_idx1] = (((x) << 16) >> 16);
                            goto EXEC_LOOP;
                        case 0x00:    // SLDT
                            if (!(cr0 & (1 << 0)) || (eflags & 0x00020000))
                                abort(6);
                            mem8            = phys_mem8[physmem8_ptr++];
                            conditional_var = (mem8 >> 3) & 7;
                            switch (conditional_var) {
                                case 0:    // SLDT Store Local Descriptor Table Register
                                case 1:    // STR Store Task Register
                                    if (conditional_var == 0)
                                        x = ldt.selector;
                                    else
                                        x = tr.selector;
                                    if ((mem8 >> 6) == 3) {
                                        set_lower_word_in_register(mem8 & 7, x);
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        st16_mem8_write(x);
                                    }
                                    break;
                                case 2:    // LDTR Load Local Descriptor Table Register
                                case 3:    // LTR Load Task Register
                                    if (cpl != 0)
                                        abort(13);
                                    if ((mem8 >> 6) == 3) {
                                        x = regs[mem8 & 7] & 0xffff;
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        x        = ld_16bits_mem8_read();
                                    }
                                    if (conditional_var == 2)
                                        op_LDTR(x);
                                    else
                                        op_LTR(x);
                                    break;
                                case 4:    // VERR Verify a Segment for Reading
                                case 5:    // VERW Verify a Segment for Writing
                                    if ((mem8 >> 6) == 3) {
                                        x = regs[mem8 & 7] & 0xffff;
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        x        = ld_16bits_mem8_read();
                                    }
                                    op_VERR_VERW(x, conditional_var & 1);
                                    break;
                                default:
                                    abort(6);
                            }
                            goto EXEC_LOOP;
                        case 0x01:    // SGDT GDTR Ms Store Global Descriptor Table Register
                            mem8            = phys_mem8[physmem8_ptr++];
                            conditional_var = (mem8 >> 3) & 7;
                            switch (conditional_var) {
                                case 2:
                                case 3:
                                    if ((mem8 >> 6) == 3)
                                        abort(6);
                                    if (cpl != 0)
                                        abort(13);
                                    mem8_loc = segment_translation(mem8);
                                    x        = ld_16bits_mem8_read();
                                    mem8_loc += 2;
                                    y = ld_32bits_mem8_read();
                                    if (conditional_var == 2) {
                                        gdt.base  = y;
                                        gdt.limit = x;
                                    } else {
                                        idt.base  = y;
                                        idt.limit = x;
                                    }
                                    break;
                                case 7:
                                    if (cpl != 0)
                                        abort(13);
                                    if ((mem8 >> 6) == 3)
                                        abort(6);
                                    mem8_loc = segment_translation(mem8);
                                    tlb_flush_page(mem8_loc & -4096);
                                    break;
                                default:
                                    abort(6);
                            }
                            goto EXEC_LOOP;
                        case 0x02:    // LAR Mw Gvqp Load Access Rights Byte
                        case 0x03:    // LSL Mw Gvqp Load Segment Limit
                            op_LAR_LSL((((CS_flags >> 8) & 1) ^ 1), OPbyte & 1);
                            goto EXEC_LOOP;
                        case 0x20:    // MOV Cd Rd Move to/from Control Registers
                            if (cpl != 0)
                                abort(13);
                            mem8 = phys_mem8[physmem8_ptr++];
                            if ((mem8 >> 6) != 3)
                                abort(6);
                            reg_idx1 = (mem8 >> 3) & 7;
                            switch (reg_idx1) {
                                case 0:
                                    x = cr0;
                                    break;
                                case 2:
                                    x = cr2;
                                    break;
                                case 3:
                                    x = cr3;
                                    break;
                                case 4:
                                    x = cr4;
                                    break;
                                default:
                                    abort(6);
                            }
                            regs[mem8 & 7] = x;
                            goto EXEC_LOOP;
                        case 0x22:    // MOV Rd Cd Move to/from Control Registers
                            if (cpl != 0)
                                abort(13);
                            mem8 = phys_mem8[physmem8_ptr++];
                            if ((mem8 >> 6) != 3)
                                abort(6);
                            reg_idx1 = (mem8 >> 3) & 7;
                            x        = regs[mem8 & 7];
                            switch (reg_idx1) {
                                case 0:
                                    set_CR0(x);
                                    break;
                                case 2:
                                    cr2 = x;
                                    break;
                                case 3:
                                    set_CR3(x);
                                    break;
                                case 4:
                                    set_CR4(x);
                                    break;
                                default:
                                    abort(6);
                            }
                            goto EXEC_LOOP;
                        case 0x06:    // CLTS  CR0 Clear Task-Switched Flag in CR0
                            if (cpl != 0)
                                abort(13);
                            set_CR0(cr0 & ~(1 << 3));    // Clear Task-Switched Flag in CR0
                            goto EXEC_LOOP;
                        case 0x23:    // MOV Rd Dd Move to/from Debug Registers
                            if (cpl != 0)
                                abort(13);
                            mem8 = phys_mem8[physmem8_ptr++];
                            if ((mem8 >> 6) != 3)
                                abort(6);
                            reg_idx1 = (mem8 >> 3) & 7;
                            x        = regs[mem8 & 7];
                            if (reg_idx1 == 4 || reg_idx1 == 5)
                                abort(6);
                            goto EXEC_LOOP;
                        case 0xb2:    // LSS Mptp SS Load Far Pointer
                        case 0xb4:    // LFS Mptp FS Load Far Pointer
                        case 0xb5:    // LGS Mptp GS Load Far Pointer
                            op_16_load_far_pointer32(OPbyte & 7);
                            goto EXEC_LOOP;
                        case 0xa2:    // CPUID  IA32_BIOS_SIGN_ID CPU Identification
                            op_CPUID();
                            goto EXEC_LOOP;
                        case 0xa4:    // SHLD Gvqp Evqp Double Precision Shift Left
                            mem8 = phys_mem8[physmem8_ptr++];
                            y    = regs[(mem8 >> 3) & 7];
                            if ((mem8 >> 6) == 3) {
                                z              = phys_mem8[physmem8_ptr++];
                                reg_idx0       = mem8 & 7;
                                regs[reg_idx0] = op_SHLD(regs[reg_idx0], y, z);
                            } else {
                                mem8_loc = segment_translation(mem8);
                                z        = phys_mem8[physmem8_ptr++];
                                x        = ld_32bits_mem8_write();
                                x        = op_SHLD(x, y, z);
                                st32_mem8_write(x);
                            }
                            goto EXEC_LOOP;
                        case 0xa5:    // SHLD Gvqp Evqp Double Precision Shift Left
                            mem8 = phys_mem8[physmem8_ptr++];
                            y    = regs[(mem8 >> 3) & 7];
                            z    = regs[1];
                            if ((mem8 >> 6) == 3) {
                                reg_idx0       = mem8 & 7;
                                regs[reg_idx0] = op_SHLD(regs[reg_idx0], y, z);
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_32bits_mem8_write();
                                x        = op_SHLD(x, y, z);
                                st32_mem8_write(x);
                            }
                            goto EXEC_LOOP;
                        case 0xac:    // SHRD Gvqp Evqp Double Precision Shift Right
                            mem8 = phys_mem8[physmem8_ptr++];
                            y    = regs[(mem8 >> 3) & 7];
                            if ((mem8 >> 6) == 3) {
                                z              = phys_mem8[physmem8_ptr++];
                                reg_idx0       = mem8 & 7;
                                regs[reg_idx0] = op_SHRD(regs[reg_idx0], y, z);
                            } else {
                                mem8_loc = segment_translation(mem8);
                                z        = phys_mem8[physmem8_ptr++];
                                x        = ld_32bits_mem8_write();
                                x        = op_SHRD(x, y, z);
                                st32_mem8_write(x);
                            }
                            goto EXEC_LOOP;
                        case 0xad:    // SHRD Gvqp Evqp Double Precision Shift Right
                            mem8 = phys_mem8[physmem8_ptr++];
                            y    = regs[(mem8 >> 3) & 7];
                            z    = regs[1];
                            if ((mem8 >> 6) == 3) {
                                reg_idx0       = mem8 & 7;
                                regs[reg_idx0] = op_SHRD(regs[reg_idx0], y, z);
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_32bits_mem8_write();
                                x        = op_SHRD(x, y, z);
                                st32_mem8_write(x);
                            }
                            goto EXEC_LOOP;
                        case 0xba:    // BT Evqp  Bit Test
                            mem8            = phys_mem8[physmem8_ptr++];
                            conditional_var = (mem8 >> 3) & 7;
                            switch (conditional_var) {
                                case 4:    // BT Evqp  Bit Test
                                    if ((mem8 >> 6) == 3) {
                                        x = regs[mem8 & 7];
                                        y = phys_mem8[physmem8_ptr++];
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        y        = phys_mem8[physmem8_ptr++];
                                        x        = ld_32bits_mem8_read();
                                    }
                                    op_BT(x, y);
                                    break;
                                case 5:    // BTS  Bit Test and Set
                                case 6:    // BTR  Bit Test and Reset
                                case 7:    // BTC  Bit Test and Complement
                                    if ((mem8 >> 6) == 3) {
                                        reg_idx0       = mem8 & 7;
                                        y              = phys_mem8[physmem8_ptr++];
                                        regs[reg_idx0] = op_BTS_BTR_BTC(conditional_var & 3, regs[reg_idx0], y);
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        y        = phys_mem8[physmem8_ptr++];
                                        x        = ld_32bits_mem8_write();
                                        x        = op_BTS_BTR_BTC(conditional_var & 3, x, y);
                                        st32_mem8_write(x);
                                    }
                                    break;
                                default:
                                    abort(6);
                            }
                            goto EXEC_LOOP;
                        case 0xa3:    // BT Evqp  Bit Test
                            mem8 = phys_mem8[physmem8_ptr++];
                            y    = regs[(mem8 >> 3) & 7];
                            if ((mem8 >> 6) == 3) {
                                x = regs[mem8 & 7];
                            } else {
                                mem8_loc = segment_translation(mem8);
                                mem8_loc = (mem8_loc + ((y >> 5) << 2)) >> 0;
                                x        = ld_32bits_mem8_read();
                            }
                            op_BT(x, y);
                            goto EXEC_LOOP;
                        case 0xab:    // BTS Gvqp Evqp Bit Test and Set
                        case 0xb3:    // BTR Gvqp Evqp Bit Test and Reset
                        case 0xbb:    // BTC Gvqp Evqp Bit Test and Complement
                            mem8            = phys_mem8[physmem8_ptr++];
                            y               = regs[(mem8 >> 3) & 7];
                            conditional_var = (OPbyte >> 3) & 3;
                            if ((mem8 >> 6) == 3) {
                                reg_idx0       = mem8 & 7;
                                regs[reg_idx0] = op_BTS_BTR_BTC(conditional_var, regs[reg_idx0], y);
                            } else {
                                mem8_loc = segment_translation(mem8);
                                mem8_loc = (mem8_loc + ((y >> 5) << 2)) >> 0;
                                x        = ld_32bits_mem8_write();
                                x        = op_BTS_BTR_BTC(conditional_var, x, y);
                                st32_mem8_write(x);
                            }
                            goto EXEC_LOOP;
                        case 0xbc:    // BSF Evqp Gvqp Bit Scan Forward
                        case 0xbd:    // BSR Evqp Gvqp Bit Scan Reverse
                            mem8     = phys_mem8[physmem8_ptr++];
                            reg_idx1 = (mem8 >> 3) & 7;
                            if ((mem8 >> 6) == 3) {
                                y = regs[mem8 & 7];
                            } else {
                                mem8_loc = segment_translation(mem8);
                                y        = ld_32bits_mem8_read();
                            }
                            if (OPbyte & 1)
                                regs[reg_idx1] = op_BSR(regs[reg_idx1], y);
                            else
                                regs[reg_idx1] = op_BSF(regs[reg_idx1], y);
                            goto EXEC_LOOP;
                        case 0xaf:    // IMUL Evqp Gvqp Signed Multiply
                            mem8     = phys_mem8[physmem8_ptr++];
                            reg_idx1 = (mem8 >> 3) & 7;
                            if ((mem8 >> 6) == 3) {
                                y = regs[mem8 & 7];
                            } else {
                                mem8_loc = segment_translation(mem8);
                                y        = ld_32bits_mem8_read();
                            }
                            regs[reg_idx1] = op_IMUL32(regs[reg_idx1], y);
                            goto EXEC_LOOP;
                        case 0x31:    // RDTSC IA32_TIME_STAMP_COUNTER EAX Read Time-Stamp Counter
                            if ((cr4 & (1 << 2)) && cpl != 0)
                                abort(13);
                            x       = current_cycle_count();
                            regs[0] = x >> 0;
                            regs[2] = (x / 0x100000000) >> 0;
                            goto EXEC_LOOP;
                        case 0xc0:    // XADD  Eb Exchange and Add
                            mem8     = phys_mem8[physmem8_ptr++];
                            reg_idx1 = (mem8 >> 3) & 7;
                            if ((mem8 >> 6) == 3) {
                                reg_idx0 = mem8 & 7;
                                x        = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1));
                                y        = do_8bit_math(0, x, (regs[reg_idx1 & 3] >> ((reg_idx1 & 4) << 1)));
                                set_word_in_register(reg_idx1, x);
                                set_word_in_register(reg_idx0, y);
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_8bits_mem8_write();
                                y        = do_8bit_math(0, x, (regs[reg_idx1 & 3] >> ((reg_idx1 & 4) << 1)));
                                st8_mem8_write(y);
                                set_word_in_register(reg_idx1, x);
                            }
                            goto EXEC_LOOP;
                        case 0xc1:    // XADD  Evqp Exchange and Add
                            mem8     = phys_mem8[physmem8_ptr++];
                            reg_idx1 = (mem8 >> 3) & 7;
                            if ((mem8 >> 6) == 3) {
                                reg_idx0       = mem8 & 7;
                                x              = regs[reg_idx0];
                                y              = do_32bit_math(0, x, regs[reg_idx1]);
                                regs[reg_idx1] = x;
                                regs[reg_idx0] = y;
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_32bits_mem8_write();
                                y        = do_32bit_math(0, x, regs[reg_idx1]);
                                st32_mem8_write(y);
                                regs[reg_idx1] = x;
                            }
                            goto EXEC_LOOP;
                        case 0xb0:    // CMPXCHG Gb Eb Compare and Exchange
                            mem8     = phys_mem8[physmem8_ptr++];
                            reg_idx1 = (mem8 >> 3) & 7;
                            if ((mem8 >> 6) == 3) {
                                reg_idx0 = mem8 & 7;
                                x        = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1));
                                y        = do_8bit_math(5, regs[0], x);
                                if (y == 0) {
                                    set_word_in_register(reg_idx0, (regs[reg_idx1 & 3] >> ((reg_idx1 & 4) << 1)));
                                } else {
                                    set_word_in_register(0, x);
                                }
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_8bits_mem8_write();
                                y        = do_8bit_math(5, regs[0], x);
                                if (y == 0) {
                                    st8_mem8_write((regs[reg_idx1 & 3] >> ((reg_idx1 & 4) << 1)));
                                } else {
                                    set_word_in_register(0, x);
                                }
                            }
                            goto EXEC_LOOP;
                        case 0xb1:    // CMPXCHG Gvqp Evqp Compare and Exchange
                            mem8     = phys_mem8[physmem8_ptr++];
                            reg_idx1 = (mem8 >> 3) & 7;
                            if ((mem8 >> 6) == 3) {
                                reg_idx0 = mem8 & 7;
                                x        = regs[reg_idx0];
                                y        = do_32bit_math(5, regs[0], x);
                                if (y == 0) {
                                    regs[reg_idx0] = regs[reg_idx1];
                                } else {
                                    regs[0] = x;
                                }
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_32bits_mem8_write();
                                y        = do_32bit_math(5, regs[0], x);
                                if (y == 0) {
                                    st32_mem8_write(regs[reg_idx1]);
                                } else {
                                    regs[0] = x;
                                }
                            }
                            goto EXEC_LOOP;
                        case 0xa0:    // PUSH FS SS:[rSP] Push Word, Doubleword or Quadword Onto the Stack
                        case 0xa8:    // PUSH GS SS:[rSP] Push Word, Doubleword or Quadword Onto the Stack
                            push_dword_to_stack(segs[(OPbyte >> 3) & 7].selector);
                            goto EXEC_LOOP;
                        case 0xa1:    // POP SS:[rSP] FS Pop a Value from the Stack
                        case 0xa9:    // POP SS:[rSP] GS Pop a Value from the Stack
                            set_segment_register((OPbyte >> 3) & 7, pop_dword_from_stack_read() & 0xffff);
                            pop_dword_from_stack_incr_ptr();
                            goto EXEC_LOOP;
                        case 0xc8:    // BSWAP  Zvqp Byte Swap
                        case 0xc9:
                        case 0xca:
                        case 0xcb:
                        case 0xcc:
                        case 0xcd:
                        case 0xce:
                        case 0xcf: {
                            reg_idx1       = OPbyte & 7;
                            x              = regs[reg_idx1];
                            uint32_t xuint = x;
                            x = (xuint >> 24) | ((x >> 8) & 0x0000ff00) | ((x << 8) & 0x00ff0000) | (x << 24);
                            regs[reg_idx1] = x;
                            goto EXEC_LOOP;
                        } break;
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
                        case 0xd0:    // ADDSUBPD Wpd Vpd Packed Double-FP Add/Subtract
                        case 0xd1:    // PSRLW Qq Pq Shift Packed Data Right Logical
                        case 0xd2:    // PSRLD Qq Pq Shift Packed Data Right Logical
                        case 0xd3:    // PSRLQ Qq Pq Shift Packed Data Right Logical
                        case 0xd4:    // PADDQ Qq Pq Add Packed Quadword Integers
                        case 0xd5:    // PMULLW Qq Pq Multiply Packed Signed Integers and Store Low Result
                        case 0xd6:    // MOVQ Vq Wq Move Quadword
                        case 0xd7:    // PMOVMSKB Nq Gdqp Move Byte Mask
                        case 0xd8:    // PSUBUSB Qq Pq Subtract Packed Unsigned Integers with Unsigned Saturation
                        case 0xd9:    // PSUBUSW Qq Pq Subtract Packed Unsigned Integers with Unsigned Saturation
                        case 0xda:    // PMINUB Qq Pq Minimum of Packed Unsigned Byte Integers
                        case 0xdb:    // PAND Qd Pq Logical AND
                        case 0xdc:    // PADDUSB Qq Pq Add Packed Unsigned Integers with Unsigned Saturation
                        case 0xdd:    // PADDUSW Qq Pq Add Packed Unsigned Integers with Unsigned Saturation
                        case 0xde:    // PMAXUB Qq Pq Maximum of Packed Unsigned Byte Integers
                        case 0xdf:    // PANDN Qq Pq Logical AND NOT
                        case 0xe0:    // PAVGB Qq Pq Average Packed Integers
                        case 0xe1:    // PSRAW Qq Pq Shift Packed Data Right Arithmetic
                        case 0xe2:    // PSRAD Qq Pq Shift Packed Data Right Arithmetic
                        case 0xe3:    // PAVGW Qq Pq Average Packed Integers
                        case 0xe4:    // PMULHUW Qq Pq Multiply Packed Unsigned Integers and Store High Result
                        case 0xe5:    // PMULHW Qq Pq Multiply Packed Signed Integers and Store High Result
                        case 0xe6:    // CVTPD2DQ Wpd Vdq Convert Packed Double-FP Values to1.11 PackedDW Integers
                        case 0xe7:    // MOVNTQ Pq Mq Store of Quadword Using Non-Temporal Hint
                        case 0xe8:    // PSUBSB Qq Pq Subtract Packed Signed Integers with Signed Saturation
                        case 0xe9:    // PSUBSW Qq Pq Subtract Packed Signed Integers with Signed Saturation
                        case 0xea:    // PMINSW Qq Pq Minimum of Packed Signed Word Integers
                        case 0xeb:    // POR Qq Pq Bitwise Logical OR
                        case 0xec:    // PADDSB Qq Pq Add Packed Signed Integers with Signed Saturation
                        case 0xed:    // PADDSW Qq Pq Add Packed Signed Integers with Signed Saturation
                        case 0xee:    // PMAXSW Qq Pq Maximum of Packed Signed Word Integers
                        case 0xef:    // PXOR Qq Pq Logical Exclusive OR
                        case 0xf0:    // LDDQU Mdq Vdq Load Unaligned Integer 128 Bits
                        case 0xf1:    // PSLLW Qq Pq Shift Packed Data Left Logical
                        case 0xf2:    // PSLLD Qq Pq Shift Packed Data Left Logical
                        case 0xf3:    // PSLLQ Qq Pq Shift Packed Data Left Logical
                        case 0xf4:    // PMULUDQ Qq Pq Multiply Packed Unsigned DW Integers
                        case 0xf5:    // PMADDWD Qd Pq Multiply and Add Packed Integers
                        case 0xf6:    // PSADBW Qq Pq Compute Sum of Absolute Differences
                        case 0xf7:    // MASKMOVQ Nq (DS:)[rDI] Store Selected Bytes of Quadword
                        case 0xf8:    // PSUBB Qq Pq Subtract Packed Integers
                        case 0xf9:    // PSUBW Qq Pq Subtract Packed Integers
                        case 0xfa:    // PSUBD Qq Pq Subtract Packed Integers
                        case 0xfb:    // PSUBQ Qq Pq Subtract Packed Quadword Integers
                        case 0xfc:    // PADDB Qq Pq Add Packed Integers
                        case 0xfd:    // PADDW Qq Pq Add Packed Integers
                        case 0xfe:    // PADDD Qq Pq Add Packed Integers
                        case 0xff:
                        default:
                            abort(6);
                    }
                    break;

                default:
                    switch (OPbyte) {
                        case 0x189:    // MOV Gvqp Evqp Move
                            mem8 = phys_mem8[physmem8_ptr++];
                            x    = regs[(mem8 >> 3) & 7];
                            if ((mem8 >> 6) == 3) {
                                set_lower_word_in_register(mem8 & 7, x);
                            } else {
                                mem8_loc = segment_translation(mem8);
                                st16_mem8_write(x);
                            }
                            goto EXEC_LOOP;
                        case 0x18b:    // MOV Evqp Gvqp Move
                            mem8 = phys_mem8[physmem8_ptr++];
                            if ((mem8 >> 6) == 3) {
                                x = regs[mem8 & 7];
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_16bits_mem8_read();
                            }
                            set_lower_word_in_register((mem8 >> 3) & 7, x);
                            goto EXEC_LOOP;
                        case 0x1b8:    // MOV Ivqp Zvqp Move
                        case 0x1b9:
                        case 0x1ba:
                        case 0x1bb:
                        case 0x1bc:
                        case 0x1bd:
                        case 0x1be:
                        case 0x1bf:
                            set_lower_word_in_register(OPbyte & 7, ld16_mem8_direct());
                            goto EXEC_LOOP;
                        case 0x1a1:    // MOV Ovqp rAX Move
                            mem8_loc = segmented_mem8_loc_for_MOV();
                            x        = ld_16bits_mem8_read();
                            set_lower_word_in_register(0, x);
                            goto EXEC_LOOP;
                        case 0x1a3:    // MOV rAX Ovqp Move
                            mem8_loc = segmented_mem8_loc_for_MOV();
                            st16_mem8_write(regs[0]);
                            goto EXEC_LOOP;
                        case 0x1c7:    // MOV Ivds Evqp Move
                            mem8 = phys_mem8[physmem8_ptr++];
                            if ((mem8 >> 6) == 3) {
                                x = ld16_mem8_direct();
                                set_lower_word_in_register(mem8 & 7, x);
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld16_mem8_direct();
                                st16_mem8_write(x);
                            }
                            goto EXEC_LOOP;
                        case 0x191:
                        case 0x192:
                        case 0x193:
                        case 0x194:
                        case 0x195:
                        case 0x196:
                        case 0x197:
                            reg_idx1 = OPbyte & 7;
                            x        = regs[0];
                            set_lower_word_in_register(0, regs[reg_idx1]);
                            set_lower_word_in_register(reg_idx1, x);
                            goto EXEC_LOOP;
                        case 0x187:    // XCHG  Gvqp Exchange Register/Memory with Register
                            mem8     = phys_mem8[physmem8_ptr++];
                            reg_idx1 = (mem8 >> 3) & 7;
                            if ((mem8 >> 6) == 3) {
                                reg_idx0 = mem8 & 7;
                                x        = regs[reg_idx0];
                                set_lower_word_in_register(reg_idx0, regs[reg_idx1]);
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_16bits_mem8_write();
                                st16_mem8_write(regs[reg_idx1]);
                            }
                            set_lower_word_in_register(reg_idx1, x);
                            goto EXEC_LOOP;
                        case 0x1c4:    // LES Mp ES Load Far Pointer
                            op_16_load_far_pointer16(0);
                            goto EXEC_LOOP;
                        case 0x1c5:    // LDS Mp DS Load Far Pointer
                            op_16_load_far_pointer16(3);
                            goto EXEC_LOOP;
                        case 0x101:    // ADD Gvqp Evqp Add
                        case 0x109:    // OR Gvqp Evqp Logical Inclusive OR
                        case 0x111:    // ADC Gvqp Evqp Add with Carry
                        case 0x119:    // SBB Gvqp Evqp Integer Subtraction with Borrow
                        case 0x121:    // AND Gvqp Evqp Logical AND
                        case 0x129:    // SUB Gvqp Evqp Subtract
                        case 0x131:    // XOR Gvqp Evqp Logical Exclusive OR
                        case 0x139:    // CMP Evqp  Compare Two Operands
                            mem8            = phys_mem8[physmem8_ptr++];
                            conditional_var = (OPbyte >> 3) & 7;
                            y               = regs[(mem8 >> 3) & 7];
                            if ((mem8 >> 6) == 3) {
                                reg_idx0 = mem8 & 7;
                                set_lower_word_in_register(reg_idx0, do_16bit_math(conditional_var, regs[reg_idx0], y));
                            } else {
                                mem8_loc = segment_translation(mem8);
                                if (conditional_var != 7) {
                                    x = ld_16bits_mem8_write();
                                    x = do_16bit_math(conditional_var, x, y);
                                    st16_mem8_write(x);
                                } else {
                                    x = ld_16bits_mem8_read();
                                    do_16bit_math(7, x, y);
                                }
                            }
                            goto EXEC_LOOP;
                        case 0x103:    // ADD Evqp Gvqp Add
                        case 0x10b:    // OR Evqp Gvqp Logical Inclusive OR
                        case 0x113:    // ADC Evqp Gvqp Add with Carry
                        case 0x11b:    // SBB Evqp Gvqp Integer Subtraction with Borrow
                        case 0x123:    // AND Evqp Gvqp Logical AND
                        case 0x12b:    // SUB Evqp Gvqp Subtract
                        case 0x133:    // XOR Evqp Gvqp Logical Exclusive OR
                        case 0x13b:    // CMP Gvqp  Compare Two Operands
                            mem8            = phys_mem8[physmem8_ptr++];
                            conditional_var = (OPbyte >> 3) & 7;
                            reg_idx1        = (mem8 >> 3) & 7;
                            if ((mem8 >> 6) == 3) {
                                y = regs[mem8 & 7];
                            } else {
                                mem8_loc = segment_translation(mem8);
                                y        = ld_16bits_mem8_read();
                            }
                            set_lower_word_in_register(reg_idx1, do_16bit_math(conditional_var, regs[reg_idx1], y));
                            goto EXEC_LOOP;
                        case 0x105:    // ADD Ivds rAX Add
                        case 0x10d:    // OR Ivds rAX Logical Inclusive OR
                        case 0x115:    // ADC Ivds rAX Add with Carry
                        case 0x11d:    // SBB Ivds rAX Integer Subtraction with Borrow
                        case 0x125:    // AND Ivds rAX Logical AND
                        case 0x12d:    // SUB Ivds rAX Subtract
                        case 0x135:    // XOR Ivds rAX Logical Exclusive OR
                        case 0x13d:    // CMP rAX  Compare Two Operands
                            y               = ld16_mem8_direct();
                            conditional_var = (OPbyte >> 3) & 7;
                            set_lower_word_in_register(0, do_16bit_math(conditional_var, regs[0], y));
                            goto EXEC_LOOP;
                        case 0x181:    // ADD Ivds Evqp Add
                            mem8            = phys_mem8[physmem8_ptr++];
                            conditional_var = (mem8 >> 3) & 7;
                            if ((mem8 >> 6) == 3) {
                                reg_idx0       = mem8 & 7;
                                y              = ld16_mem8_direct();
                                regs[reg_idx0] = do_16bit_math(conditional_var, regs[reg_idx0], y);
                            } else {
                                mem8_loc = segment_translation(mem8);
                                y        = ld16_mem8_direct();
                                if (conditional_var != 7) {
                                    x = ld_16bits_mem8_write();
                                    x = do_16bit_math(conditional_var, x, y);
                                    st16_mem8_write(x);
                                } else {
                                    x = ld_16bits_mem8_read();
                                    do_16bit_math(7, x, y);
                                }
                            }
                            goto EXEC_LOOP;
                        case 0x183:    // ADD Ibs Evqp Add
                            mem8            = phys_mem8[physmem8_ptr++];
                            conditional_var = (mem8 >> 3) & 7;
                            if ((mem8 >> 6) == 3) {
                                reg_idx0 = mem8 & 7;
                                y        = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                                set_lower_word_in_register(reg_idx0, do_16bit_math(conditional_var, regs[reg_idx0], y));
                            } else {
                                mem8_loc = segment_translation(mem8);
                                y        = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                                if (conditional_var != 7) {
                                    x = ld_16bits_mem8_write();
                                    x = do_16bit_math(conditional_var, x, y);
                                    st16_mem8_write(x);
                                } else {
                                    x = ld_16bits_mem8_read();
                                    do_16bit_math(7, x, y);
                                }
                            }
                            goto EXEC_LOOP;
                        case 0x140:    // INC  Zv Increment by 1
                        case 0x141:    // REX.B   Extension of r/m field, base field, or opcode reg field
                        case 0x142:    // REX.X   Extension of SIB index field
                        case 0x143:    // REX.XB   REX.X and REX.B combination
                        case 0x144:    // REX.R   Extension of ModR/M reg field
                        case 0x145:    // REX.RB   REX.R and REX.B combination
                        case 0x146:    // REX.RX   REX.R and REX.X combination
                        case 0x147:    // REX.RXB   REX.R, REX.X and REX.B combination
                            reg_idx1 = OPbyte & 7;
                            set_lower_word_in_register(reg_idx1, increment_16bit(regs[reg_idx1]));
                            goto EXEC_LOOP;
                        case 0x148:    // DEC  Zv Decrement by 1
                        case 0x149:    // REX.WB   REX.W and REX.B combination
                        case 0x14a:    // REX.WX   REX.W and REX.X combination
                        case 0x14b:    // REX.WXB   REX.W, REX.X and REX.B combination
                        case 0x14c:    // REX.WR   REX.W and REX.R combination
                        case 0x14d:    // REX.WRB   REX.W, REX.R and REX.B combination
                        case 0x14e:    // REX.WRX   REX.W, REX.R and REX.X combination
                        case 0x14f:    // REX.WRXB   REX.W, REX.R, REX.X and REX.B combination
                            reg_idx1 = OPbyte & 7;
                            set_lower_word_in_register(reg_idx1, decrement_16bit(regs[reg_idx1]));
                            goto EXEC_LOOP;
                        case 0x16b:    // IMUL Evqp Gvqp Signed Multiply
                            mem8     = phys_mem8[physmem8_ptr++];
                            reg_idx1 = (mem8 >> 3) & 7;
                            if ((mem8 >> 6) == 3) {
                                y = regs[mem8 & 7];
                            } else {
                                mem8_loc = segment_translation(mem8);
                                y        = ld_16bits_mem8_read();
                            }
                            z = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                            set_lower_word_in_register(reg_idx1, op_16_IMUL(y, z));
                            goto EXEC_LOOP;
                        case 0x169:    // IMUL Evqp Gvqp Signed Multiply
                            mem8     = phys_mem8[physmem8_ptr++];
                            reg_idx1 = (mem8 >> 3) & 7;
                            if ((mem8 >> 6) == 3) {
                                y = regs[mem8 & 7];
                            } else {
                                mem8_loc = segment_translation(mem8);
                                y        = ld_16bits_mem8_read();
                            }
                            z = ld16_mem8_direct();
                            set_lower_word_in_register(reg_idx1, op_16_IMUL(y, z));
                            goto EXEC_LOOP;
                        case 0x185:    // TEST Evqp  Logical Compare
                            mem8 = phys_mem8[physmem8_ptr++];
                            if ((mem8 >> 6) == 3) {
                                x = regs[mem8 & 7];
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_16bits_mem8_read();
                            }
                            y = regs[(mem8 >> 3) & 7];

                            cc_dst = (((x & y) << 16) >> 16);
                            cc_op  = 13;

                            goto EXEC_LOOP;
                        case 0x1a9:    // TEST rAX  Logical Compare
                            y = ld16_mem8_direct();

                            cc_dst = (((regs[0] & y) << 16) >> 16);
                            cc_op  = 13;

                            goto EXEC_LOOP;
                        case 0x1f7:    // TEST Evqp  Logical Compare
                            mem8            = phys_mem8[physmem8_ptr++];
                            conditional_var = (mem8 >> 3) & 7;
                            switch (conditional_var) {
                                case 0:
                                    if ((mem8 >> 6) == 3) {
                                        x = regs[mem8 & 7];
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        x        = ld_16bits_mem8_read();
                                    }
                                    y = ld16_mem8_direct();

                                    cc_dst = (((x & y) << 16) >> 16);
                                    cc_op  = 13;

                                    break;
                                case 2:
                                    if ((mem8 >> 6) == 3) {
                                        reg_idx0 = mem8 & 7;
                                        set_lower_word_in_register(reg_idx0, ~regs[reg_idx0]);
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        x        = ld_16bits_mem8_write();
                                        x        = ~x;
                                        st16_mem8_write(x);
                                    }
                                    break;
                                case 3:
                                    if ((mem8 >> 6) == 3) {
                                        reg_idx0 = mem8 & 7;
                                        set_lower_word_in_register(reg_idx0, do_16bit_math(5, 0, regs[reg_idx0]));
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        x        = ld_16bits_mem8_write();
                                        x        = do_16bit_math(5, 0, x);
                                        st16_mem8_write(x);
                                    }
                                    break;
                                case 4:
                                    if ((mem8 >> 6) == 3) {
                                        x = regs[mem8 & 7];
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        x        = ld_16bits_mem8_read();
                                    }
                                    x = op_16_MUL(regs[0], x);
                                    set_lower_word_in_register(0, x);
                                    set_lower_word_in_register(2, x >> 16);
                                    break;
                                case 5:
                                    if ((mem8 >> 6) == 3) {
                                        x = regs[mem8 & 7];
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        x        = ld_16bits_mem8_read();
                                    }
                                    x = op_16_IMUL(regs[0], x);
                                    set_lower_word_in_register(0, x);
                                    set_lower_word_in_register(2, x >> 16);
                                    break;
                                case 6:
                                    if ((mem8 >> 6) == 3) {
                                        x = regs[mem8 & 7];
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        x        = ld_16bits_mem8_read();
                                    }
                                    op_16_DIV(x);
                                    break;
                                case 7:
                                    if ((mem8 >> 6) == 3) {
                                        x = regs[mem8 & 7];
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        x        = ld_16bits_mem8_read();
                                    }
                                    op_16_IDIV(x);
                                    break;
                                default:
                                    abort(6);
                            }
                            goto EXEC_LOOP;
                        case 0x1c1:    // ROL Ib Evqp Rotate
                            mem8            = phys_mem8[physmem8_ptr++];
                            conditional_var = (mem8 >> 3) & 7;
                            if ((mem8 >> 6) == 3) {
                                y        = phys_mem8[physmem8_ptr++];
                                reg_idx0 = mem8 & 7;
                                set_lower_word_in_register(reg_idx0, shift16(conditional_var, regs[reg_idx0], y));
                            } else {
                                mem8_loc = segment_translation(mem8);
                                y        = phys_mem8[physmem8_ptr++];
                                x        = ld_16bits_mem8_write();
                                x        = shift16(conditional_var, x, y);
                                st16_mem8_write(x);
                            }
                            goto EXEC_LOOP;
                        case 0x1d1:    // ROL 1 Evqp Rotate
                            mem8            = phys_mem8[physmem8_ptr++];
                            conditional_var = (mem8 >> 3) & 7;
                            if ((mem8 >> 6) == 3) {
                                reg_idx0 = mem8 & 7;
                                set_lower_word_in_register(reg_idx0, shift16(conditional_var, regs[reg_idx0], 1));
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_16bits_mem8_write();
                                x        = shift16(conditional_var, x, 1);
                                st16_mem8_write(x);
                            }
                            goto EXEC_LOOP;
                        case 0x1d3:    // ROL CL Evqp Rotate
                            mem8            = phys_mem8[physmem8_ptr++];
                            conditional_var = (mem8 >> 3) & 7;
                            y               = regs[1] & 0xff;
                            if ((mem8 >> 6) == 3) {
                                reg_idx0 = mem8 & 7;
                                set_lower_word_in_register(reg_idx0, shift16(conditional_var, regs[reg_idx0], y));
                            } else {
                                mem8_loc = segment_translation(mem8);
                                x        = ld_16bits_mem8_write();
                                x        = shift16(conditional_var, x, y);
                                st16_mem8_write(x);
                            }
                            goto EXEC_LOOP;
                        case 0x198:    // CBW AL AX Convert Byte to Word
                            set_lower_word_in_register(0, (regs[0] << 24) >> 24);
                            goto EXEC_LOOP;
                        case 0x199:    // CWD AX DX Convert Word to Doubleword
                            set_lower_word_in_register(2, (regs[0] << 16) >> 31);
                            goto EXEC_LOOP;
                        case 0x190:    // XCHG  Zvqp Exchange Register/Memory with Register
                            goto EXEC_LOOP;
                        case 0x150:    // PUSH Zv SS:[rSP] Push Word, Doubleword or Quadword Onto the Stack
                        case 0x151:
                        case 0x152:
                        case 0x153:
                        case 0x154:
                        case 0x155:
                        case 0x156:
                        case 0x157:
                            push_word_to_stack(regs[OPbyte & 7]);
                            goto EXEC_LOOP;
                        case 0x158:    // POP SS:[rSP] Zv Pop a Value from the Stack
                        case 0x159:
                        case 0x15a:
                        case 0x15b:
                        case 0x15c:
                        case 0x15d:
                        case 0x15e:
                        case 0x15f:
                            x = pop_word_from_stack_read();
                            pop_word_from_stack_incr_ptr();
                            set_lower_word_in_register(OPbyte & 7, x);
                            goto EXEC_LOOP;
                        case 0x160:    // PUSHA AX SS:[rSP] Push All General-Purpose Registers
                            op_16_PUSHA();
                            goto EXEC_LOOP;
                        case 0x161:    // POPA SS:[rSP] DI Pop All General-Purpose Registers
                            op_16_POPA();
                            goto EXEC_LOOP;
                        case 0x18f:    // POP SS:[rSP] Ev Pop a Value from the Stack
                            mem8 = phys_mem8[physmem8_ptr++];
                            if ((mem8 >> 6) == 3) {
                                x = pop_word_from_stack_read();
                                pop_word_from_stack_incr_ptr();
                                set_lower_word_in_register(mem8 & 7, x);
                            } else {
                                x = pop_word_from_stack_read();
                                y = regs[4];
                                pop_word_from_stack_incr_ptr();
                                z        = regs[4];
                                mem8_loc = segment_translation(mem8);
                                regs[4]  = y;
                                st16_mem8_write(x);
                                regs[4] = z;
                            }
                            goto EXEC_LOOP;
                        case 0x168:    // PUSH Ivs SS:[rSP] Push Word, Doubleword or Quadword Onto the Stack
                            x = ld16_mem8_direct();
                            push_word_to_stack(x);
                            goto EXEC_LOOP;
                        case 0x16a:    // PUSH Ibss SS:[rSP] Push Word, Doubleword or Quadword Onto the Stack
                            x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                            push_word_to_stack(x);
                            goto EXEC_LOOP;
                        case 0x1c8:    // ENTER Iw SS:[rSP] Make Stack Frame for Procedure Parameters
                            op_16_ENTER();
                            goto EXEC_LOOP;
                        case 0x1c9:    // LEAVE SS:[rSP] eBP High Level Procedure Exit
                            op_16_LEAVE();
                            goto EXEC_LOOP;
                        case 0x106:    // PUSH ES SS:[rSP] Push Word, Doubleword or Quadword Onto the Stack
                        case 0x10e:    // PUSH CS SS:[rSP] Push Word, Doubleword or Quadword Onto the Stack
                        case 0x116:    // PUSH SS SS:[rSP] Push Word, Doubleword or Quadword Onto the Stack
                        case 0x11e:    // PUSH DS SS:[rSP] Push Word, Doubleword or Quadword Onto the Stack
                            push_word_to_stack(segs[(OPbyte >> 3) & 3].selector);
                            goto EXEC_LOOP;
                        case 0x107:    // POP SS:[rSP] ES Pop a Value from the Stack
                        case 0x117:    // POP SS:[rSP] SS Pop a Value from the Stack
                        case 0x11f:    // POP SS:[rSP] DS Pop a Value from the Stack
                            set_segment_register((OPbyte >> 3) & 3, pop_word_from_stack_read());
                            pop_word_from_stack_incr_ptr();
                            goto EXEC_LOOP;
                        case 0x18d:    // LEA M Gvqp Load Effective Address
                            mem8 = phys_mem8[physmem8_ptr++];
                            if ((mem8 >> 6) == 3)
                                abort(6);
                            CS_flags = (CS_flags & ~0x000f) | (6 + 1);
                            set_lower_word_in_register((mem8 >> 3) & 7, segment_translation(mem8));
                            goto EXEC_LOOP;
                        case 0x1ff:    // INC  Evqp Increment by 1
                            mem8            = phys_mem8[physmem8_ptr++];
                            conditional_var = (mem8 >> 3) & 7;
                            switch (conditional_var) {
                                case 0:
                                    if ((mem8 >> 6) == 3) {
                                        reg_idx0 = mem8 & 7;
                                        set_lower_word_in_register(reg_idx0, increment_16bit(regs[reg_idx0]));
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        x        = ld_16bits_mem8_write();
                                        x        = increment_16bit(x);
                                        st16_mem8_write(x);
                                    }
                                    break;
                                case 1:
                                    if ((mem8 >> 6) == 3) {
                                        reg_idx0 = mem8 & 7;
                                        set_lower_word_in_register(reg_idx0, decrement_16bit(regs[reg_idx0]));
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        x        = ld_16bits_mem8_write();
                                        x        = decrement_16bit(x);
                                        st16_mem8_write(x);
                                    }
                                    break;
                                case 2:
                                    if ((mem8 >> 6) == 3) {
                                        x = regs[mem8 & 7] & 0xffff;
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        x        = ld_16bits_mem8_read();
                                    }
                                    push_word_to_stack((eip + physmem8_ptr - initial_mem_ptr));
                                    eip = x, physmem8_ptr = initial_mem_ptr = 0;
                                    break;
                                case 4:
                                    if ((mem8 >> 6) == 3) {
                                        x = regs[mem8 & 7] & 0xffff;
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        x        = ld_16bits_mem8_read();
                                    }
                                    eip = x, physmem8_ptr = initial_mem_ptr = 0;
                                    break;
                                case 6:
                                    if ((mem8 >> 6) == 3) {
                                        x = regs[mem8 & 7];
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        x        = ld_16bits_mem8_read();
                                    }
                                    push_word_to_stack(x);
                                    break;
                                case 3:
                                case 5:
                                    if ((mem8 >> 6) == 3)
                                        abort(6);
                                    mem8_loc = segment_translation(mem8);
                                    x        = ld_16bits_mem8_read();
                                    mem8_loc = (mem8_loc + 2) >> 0;
                                    y        = ld_16bits_mem8_read();
                                    if (conditional_var == 3)
                                        op_CALLF(0, y, x, (eip + physmem8_ptr - initial_mem_ptr));
                                    else
                                        op_JMPF(y, x);
                                    break;
                                default:
                                    abort(6);
                            }
                            goto EXEC_LOOP;
                        case 0x1eb:    // JMP Jbs  Jump
                            x            = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                            eip          = (eip + physmem8_ptr - initial_mem_ptr + x) & 0xffff,
                            physmem8_ptr = initial_mem_ptr = 0;
                            goto EXEC_LOOP;
                        case 0x1e9:    // JMP Jvds  Jump
                            x            = ld16_mem8_direct();
                            eip          = (eip + physmem8_ptr - initial_mem_ptr + x) & 0xffff,
                            physmem8_ptr = initial_mem_ptr = 0;
                            goto EXEC_LOOP;
                        case 0x170:    // JO Jbs  Jump short if overflow (OF=1)
                        case 0x171:    // JNO Jbs  Jump short if not overflow (OF=0)
                        case 0x172:    // JB Jbs  Jump short if below/not above or equal/carry (CF=1)
                        case 0x173:    // JNB Jbs  Jump short if not below/above or equal/not carry (CF=0)
                        case 0x174:    // JZ Jbs  Jump short if zero/equal (ZF=0)
                        case 0x175:    // JNZ Jbs  Jump short if not zero/not equal (ZF=1)
                        case 0x176:    // JBE Jbs  Jump short if below or equal/not above (CF=1 AND ZF=1)
                        case 0x177:    // JNBE Jbs  Jump short if not below or equal/above (CF=0 AND ZF=0)
                        case 0x178:    // JS Jbs  Jump short if sign (SF=1)
                        case 0x179:    // JNS Jbs  Jump short if not sign (SF=0)
                        case 0x17a:    // JP Jbs  Jump short if parity/parity even (PF=1)
                        case 0x17b:    // JNP Jbs  Jump short if not parity/parity odd
                        case 0x17c:    // JL Jbs  Jump short if less/not greater (SF!=OF)
                        case 0x17d:    // JNL Jbs  Jump short if not less/greater or equal (SF=OF)
                        case 0x17e:    // JLE Jbs  Jump short if less or equal/not greater ((ZF=1) OR (SF!=OF))
                        case 0x17f:    // JNLE Jbs  Jump short if not less nor equal/greater ((ZF=0) AND (SF=OF))
                            x = ((phys_mem8[physmem8_ptr++] << 24) >> 24);
                            y = check_status_bits_for_jump(OPbyte & 0xf);
                            if (y)
                                eip          = (eip + physmem8_ptr - initial_mem_ptr + x) & 0xffff,
                                physmem8_ptr = initial_mem_ptr = 0;
                            goto EXEC_LOOP;
                        case 0x1c2:    // RETN SS:[rSP]  Return from procedure
                            y       = (ld16_mem8_direct() << 16) >> 16;
                            x       = pop_word_from_stack_read();
                            regs[4] = (regs[4] & ~SS_mask) | ((regs[4] + 2 + y) & SS_mask);
                            eip = x, physmem8_ptr = initial_mem_ptr = 0;
                            goto EXEC_LOOP;
                        case 0x1c3:    // RETN SS:[rSP]  Return from procedure
                            x = pop_word_from_stack_read();
                            pop_word_from_stack_incr_ptr();
                            eip = x, physmem8_ptr = initial_mem_ptr = 0;
                            goto EXEC_LOOP;
                        case 0x1e8:    // CALL Jvds SS:[rSP] Call Procedure
                            x = ld16_mem8_direct();
                            push_word_to_stack((eip + physmem8_ptr - initial_mem_ptr));
                            eip          = (eip + physmem8_ptr - initial_mem_ptr + x) & 0xffff,
                            physmem8_ptr = initial_mem_ptr = 0;
                            goto EXEC_LOOP;
                        case 0x162:    // BOUND Gv SS:[rSP] Check Array Index Against Bounds
                            op_16_BOUND();
                            goto EXEC_LOOP;
                        case 0x1a5:    // MOVS DS:[SI] ES:[DI] Move Data from String to String
                            op_16_MOVS();
                            goto EXEC_LOOP;
                        case 0x1a7:    // CMPS ES:[DI]  Compare String Operands
                            op_16_CMPS();
                            goto EXEC_LOOP;
                        case 0x1ad:    // LODS DS:[SI] AX Load String
                            op_16_LODS();
                            goto EXEC_LOOP;
                        case 0x1af:    // SCAS ES:[DI]  Scan String
                            op_16_SCAS();
                            goto EXEC_LOOP;
                        case 0x1ab:    // STOS AX ES:[DI] Store String
                            op_16_STOS();
                            goto EXEC_LOOP;
                        case 0x16d:    // INS DX ES:[DI] Input from Port to String
                            op_16_INS();

                            if (hard_irq != 0 && (eflags & 0x00000200))
                                goto OUTER_LOOP;

                            goto EXEC_LOOP;
                        case 0x16f:    // OUTS DS:[SI] DX Output String to Port
                            op_16_OUTS();

                            if (hard_irq != 0 && (eflags & 0x00000200))
                                goto OUTER_LOOP;

                            goto EXEC_LOOP;
                        case 0x1e5:    // IN Ib eAX Input from Port
                            iopl = (eflags >> 12) & 3;
                            if (cpl > iopl)
                                abort(13);
                            x = phys_mem8[physmem8_ptr++];
                            set_lower_word_in_register(0, ld16_port(x));

                            if (hard_irq != 0 && (eflags & 0x00000200))
                                goto OUTER_LOOP;

                            goto EXEC_LOOP;
                        case 0x1e7:    // OUT eAX Ib Output to Port
                            iopl = (eflags >> 12) & 3;
                            if (cpl > iopl)
                                abort(13);
                            x = phys_mem8[physmem8_ptr++];
                            st16_port(x, regs[0] & 0xffff);

                            if (hard_irq != 0 && (eflags & 0x00000200))
                                goto OUTER_LOOP;

                            goto EXEC_LOOP;
                        case 0x1ed:    // IN DX eAX Input from Port
                            iopl = (eflags >> 12) & 3;
                            if (cpl > iopl)
                                abort(13);
                            set_lower_word_in_register(0, ld16_port(regs[2] & 0xffff));

                            if (hard_irq != 0 && (eflags & 0x00000200))
                                goto OUTER_LOOP;

                            goto EXEC_LOOP;
                        case 0x1ef:    // OUT eAX DX Output to Port
                            iopl = (eflags >> 12) & 3;
                            if (cpl > iopl)
                                abort(13);
                            st16_port(regs[2] & 0xffff, regs[0] & 0xffff);

                            if (hard_irq != 0 && (eflags & 0x00000200))
                                goto OUTER_LOOP;

                            goto EXEC_LOOP;
                        case 0x166:    //   Operand-size override prefix
                        case 0x167:    //   Address-size override prefix
                        case 0x1f0:    // LOCK   Assert LOCK# Signal Prefix
                        case 0x1f2:    // REPNZ  eCX Repeat String Operation Prefix
                        case 0x1f3:    // REPZ  eCX Repeat String Operation Prefix
                        case 0x126:    // ES ES  ES segment override prefix
                        case 0x12e:    // CS CS  CS segment override prefix
                        case 0x136:    // SS SS  SS segment override prefix
                        case 0x13e:    // DS DS  DS segment override prefix
                        case 0x164:    // FS FS  FS segment override prefix
                        case 0x165:    // GS GS  GS segment override prefix
                        case 0x100:    // ADD Gb Eb Add
                        case 0x108:    // OR Gb Eb Logical Inclusive OR
                        case 0x110:    // ADC Gb Eb Add with Carry
                        case 0x118:    // SBB Gb Eb Integer Subtraction with Borrow
                        case 0x120:    // AND Gb Eb Logical AND
                        case 0x128:    // SUB Gb Eb Subtract
                        case 0x130:    // XOR Gb Eb Logical Exclusive OR
                        case 0x138:    // CMP Eb  Compare Two Operands
                        case 0x102:    // ADD Eb Gb Add
                        case 0x10a:    // OR Eb Gb Logical Inclusive OR
                        case 0x112:    // ADC Eb Gb Add with Carry
                        case 0x11a:    // SBB Eb Gb Integer Subtraction with Borrow
                        case 0x122:    // AND Eb Gb Logical AND
                        case 0x12a:    // SUB Eb Gb Subtract
                        case 0x132:    // XOR Eb Gb Logical Exclusive OR
                        case 0x13a:    // CMP Gb  Compare Two Operands
                        case 0x104:    // ADD Ib AL Add
                        case 0x10c:    // OR Ib AL Logical Inclusive OR
                        case 0x114:    // ADC Ib AL Add with Carry
                        case 0x11c:    // SBB Ib AL Integer Subtraction with Borrow
                        case 0x124:    // AND Ib AL Logical AND
                        case 0x12c:    // SUB Ib AL Subtract
                        case 0x134:    // XOR Ib AL Logical Exclusive OR
                        case 0x13c:    // CMP AL  Compare Two Operands
                        case 0x1a0:    // MOV Ob AL Move
                        case 0x1a2:    // MOV AL Ob Move
                        case 0x1d8:    // FADD Msr ST Add
                        case 0x1d9:    // FLD ESsr ST Load Floating Point Value
                        case 0x1da:    // FIADD Mdi ST Add
                        case 0x1db:    // FILD Mdi ST Load Integer
                        case 0x1dc:    // FADD Mdr ST Add
                        case 0x1dd:    // FLD Mdr ST Load Floating Point Value
                        case 0x1de:    // FIADD Mwi ST Add
                        case 0x1df:    // FILD Mwi ST Load Integer
                        case 0x184:    // TEST Eb  Logical Compare
                        case 0x1a8:    // TEST AL  Logical Compare
                        case 0x1f6:    // TEST Eb  Logical Compare
                        case 0x1c0:    // ROL Ib Eb Rotate
                        case 0x1d0:    // ROL 1 Eb Rotate
                        case 0x1d2:    // ROL CL Eb Rotate
                        case 0x1fe:    // INC  Eb Increment by 1
                        case 0x1cd:    // INT Ib SS:[rSP] Call to Interrupt Procedure
                        case 0x1ce:    // INTO eFlags SS:[rSP] Call to Interrupt Procedure
                        case 0x1f5:    // CMC   Complement Carry Flag
                        case 0x1f8:    // CLC   Clear Carry Flag
                        case 0x1f9:    // STC   Set Carry Flag
                        case 0x1fc:    // CLD   Clear Direction Flag
                        case 0x1fd:    // STD   Set Direction Flag
                        case 0x1fa:    // CLI   Clear Interrupt Flag
                        case 0x1fb:    // STI   Set Interrupt Flag
                        case 0x19e:    // SAHF AH  Store AH into Flags
                        case 0x19f:    // LAHF  AH Load Status Flags into AH Register
                        case 0x1f4:    // HLT   Halt
                        case 0x127:    // DAA  AL Decimal Adjust AL after Addition
                        case 0x12f:    // DAS  AL Decimal Adjust AL after Subtraction
                        case 0x137:    // AAA  AL ASCII Adjust After Addition
                        case 0x13f:    // AAS  AL ASCII Adjust AL After Subtraction
                        case 0x1d4:    // AAM  AL ASCII Adjust AX After Multiply
                        case 0x1d5:    // AAD  AL ASCII Adjust AX Before Division
                        case 0x16c:    // INS DX (ES:)[rDI] Input from Port to String
                        case 0x16e:    // OUTS (DS):[rSI] DX Output String to Port
                        case 0x1a4:    // MOVS (DS:)[rSI] (ES:)[rDI] Move Data from String to String
                        case 0x1a6:    // CMPS (ES:)[rDI]  Compare String Operands
                        case 0x1aa:    // STOS AL (ES:)[rDI] Store String
                        case 0x1ac:    // LODS (DS:)[rSI] AL Load String
                        case 0x1ae:    // SCAS (ES:)[rDI]  Scan String
                        case 0x180:    // ADD Ib Eb Add
                        case 0x182:    // ADD Ib Eb Add
                        case 0x186:    // XCHG  Gb Exchange Register/Memory with Register
                        case 0x188:    // MOV Gb Eb Move
                        case 0x18a:    // MOV Eb Gb Move
                        case 0x18c:    // MOV Sw Mw Move
                        case 0x18e:    // MOV Ew Sw Move
                        case 0x19b:    // FWAIT   Check pending unmasked floating-point exceptions
                        case 0x1b0:    // MOV Ib Zb Move
                        case 0x1b1:
                        case 0x1b2:
                        case 0x1b3:
                        case 0x1b4:
                        case 0x1b5:
                        case 0x1b6:
                        case 0x1b7:
                        case 0x1c6:    // MOV Ib Eb Move
                        case 0x1cc:    // INT 3 SS:[rSP] Call to Interrupt Procedure
                        case 0x1d7:    // XLAT (DS:)[rBX+AL] AL Table Look-up Translation
                        case 0x1e4:    // IN Ib AL Input from Port
                        case 0x1e6:    // OUT AL Ib Output to Port
                        case 0x1ec:    // IN DX AL Input from Port
                        case 0x1ee:    // OUT AL DX Output to Port
                        case 0x1cf:    // IRET SS:[rSP] Flags Interrupt Return
                        case 0x1ca:    // RETF Iw  Return from procedure
                        case 0x1cb:    // RETF SS:[rSP]  Return from procedure
                        case 0x19a:    // CALLF Ap SS:[rSP] Call Procedure
                        case 0x19c:    // PUSHF Flags SS:[rSP] Push FLAGS Register onto the Stack
                        case 0x19d:    // POPF SS:[rSP] Flags Pop Stack into FLAGS Register
                        case 0x1ea:    // JMPF Ap  Jump
                        case 0x1e0:    // LOOPNZ Jbs eCX Decrement count; Jump short if count!=0 and ZF=0
                        case 0x1e1:    // LOOPZ Jbs eCX Decrement count; Jump short if count!=0 and ZF=1
                        case 0x1e2:    // LOOP Jbs eCX Decrement count; Jump short if count!=0
                        case 0x1e3:    // JCXZ Jbs  Jump short if eCX register is 0
                            OPbyte &= 0xff;
                            break;
                        case 0x163:    // ARPL Ew  Adjust RPL Field of Segment Selector
                        case 0x1d6:    // SALC   Undefined and Reserved; Does not Generate #UD
                        case 0x1f1:    // INT1   Undefined and Reserved; Does not Generate #UD
                        case 0x10f:
                            OPbyte = phys_mem8[physmem8_ptr++];
                            OPbyte |= 0x0100;
                            switch (OPbyte) {
                                case 0x180:    // JO Jvds  Jump short if overflow (OF=1)
                                case 0x181:    // JNO Jvds  Jump short if not overflow (OF=0)
                                case 0x182:    // JB Jvds  Jump short if below/not above or equal/carry (CF=1)
                                case 0x183:    // JNB Jvds  Jump short if not below/above or equal/not carry (CF=0)
                                case 0x184:    // JZ Jvds  Jump short if zero/equal (ZF=0)
                                case 0x185:    // JNZ Jvds  Jump short if not zero/not equal (ZF=1)
                                case 0x186:    // JBE Jvds  Jump short if below or equal/not above (CF=1 AND ZF=1)
                                case 0x187:    // JNBE Jvds  Jump short if not below or equal/above (CF=0 AND ZF=0)
                                case 0x188:    // JS Jvds  Jump short if sign (SF=1)
                                case 0x189:    // JNS Jvds  Jump short if not sign (SF=0)
                                case 0x18a:    // JP Jvds  Jump short if parity/parity even (PF=1)
                                case 0x18b:    // JNP Jvds  Jump short if not parity/parity odd
                                case 0x18c:    // JL Jvds  Jump short if less/not greater (SF!=OF)
                                case 0x18d:    // JNL Jvds  Jump short if not less/greater or equal (SF=OF)
                                case 0x18e:    // JLE Jvds  Jump short if less or equal/not greater ((ZF=1) OR (SF!=OF))
                                case 0x18f:    // JNLE Jvds  Jump short if not less nor equal/greater ((ZF=0) AND
                                               // (SF=OF))
                                    x = ld16_mem8_direct();
                                    if (check_status_bits_for_jump(OPbyte & 0xf))
                                        eip          = (eip + physmem8_ptr - initial_mem_ptr + x) & 0xffff,
                                        physmem8_ptr = initial_mem_ptr = 0;
                                    goto EXEC_LOOP;
                                case 0x140:    // CMOVO Evqp Gvqp Conditional Move - overflow (OF=1)
                                case 0x141:    // CMOVNO Evqp Gvqp Conditional Move - not overflow (OF=0)
                                case 0x142:    // CMOVB Evqp Gvqp Conditional Move - below/not above or equal/carry
                                               // (CF=1)
                                case 0x143:    // CMOVNB Evqp Gvqp Conditional Move - not below/above or equal/not carry
                                               // (CF=0)
                                case 0x144:    // CMOVZ Evqp Gvqp Conditional Move - zero/equal (ZF=0)
                                case 0x145:    // CMOVNZ Evqp Gvqp Conditional Move - not zero/not equal (ZF=1)
                                case 0x146:    // CMOVBE Evqp Gvqp Conditional Move - below or equal/not above (CF=1 AND
                                               // ZF=1)
                                case 0x147:    // CMOVNBE Evqp Gvqp Conditional Move - not below or equal/above (CF=0
                                               // AND ZF=0)
                                case 0x148:    // CMOVS Evqp Gvqp Conditional Move - sign (SF=1)
                                case 0x149:    // CMOVNS Evqp Gvqp Conditional Move - not sign (SF=0)
                                case 0x14a:    // CMOVP Evqp Gvqp Conditional Move - parity/parity even (PF=1)
                                case 0x14b:    // CMOVNP Evqp Gvqp Conditional Move - not parity/parity odd
                                case 0x14c:    // CMOVL Evqp Gvqp Conditional Move - less/not greater (SF!=OF)
                                case 0x14d:    // CMOVNL Evqp Gvqp Conditional Move - not less/greater or equal (SF=OF)
                                case 0x14e:    // CMOVLE Evqp Gvqp Conditional Move - less or equal/not greater ((ZF=1)
                                               // OR (SF!=OF))
                                case 0x14f:    // CMOVNLE Evqp Gvqp Conditional Move - not less nor equal/greater
                                               // ((ZF=0) AND (SF=OF))
                                    mem8 = phys_mem8[physmem8_ptr++];
                                    if ((mem8 >> 6) == 3) {
                                        x = regs[mem8 & 7];
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        x        = ld_16bits_mem8_read();
                                    }
                                    if (check_status_bits_for_jump(OPbyte & 0xf))
                                        set_lower_word_in_register((mem8 >> 3) & 7, x);
                                    goto EXEC_LOOP;
                                case 0x1b6:    // MOVZX Eb Gvqp Move with Zero-Extend
                                    mem8     = phys_mem8[physmem8_ptr++];
                                    reg_idx1 = (mem8 >> 3) & 7;
                                    if ((mem8 >> 6) == 3) {
                                        reg_idx0 = mem8 & 7;
                                        x        = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1)) & 0xff;
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        x        = ld_8bits_mem8_read();
                                    }
                                    set_lower_word_in_register(reg_idx1, x);
                                    goto EXEC_LOOP;
                                case 0x1be:    // MOVSX Eb Gvqp Move with Sign-Extension
                                    mem8     = phys_mem8[physmem8_ptr++];
                                    reg_idx1 = (mem8 >> 3) & 7;
                                    if ((mem8 >> 6) == 3) {
                                        reg_idx0 = mem8 & 7;
                                        x        = (regs[reg_idx0 & 3] >> ((reg_idx0 & 4) << 1));
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        x        = ld_8bits_mem8_read();
                                    }
                                    set_lower_word_in_register(reg_idx1, (((x) << 24) >> 24));
                                    goto EXEC_LOOP;
                                case 0x1af:    // IMUL Evqp Gvqp Signed Multiply
                                    mem8     = phys_mem8[physmem8_ptr++];
                                    reg_idx1 = (mem8 >> 3) & 7;
                                    if ((mem8 >> 6) == 3) {
                                        y = regs[mem8 & 7];
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        y        = ld_16bits_mem8_read();
                                    }
                                    set_lower_word_in_register(reg_idx1, op_16_IMUL(regs[reg_idx1], y));
                                    goto EXEC_LOOP;
                                case 0x1c1:    // XADD  Evqp Exchange and Add
                                    mem8     = phys_mem8[physmem8_ptr++];
                                    reg_idx1 = (mem8 >> 3) & 7;
                                    if ((mem8 >> 6) == 3) {
                                        reg_idx0 = mem8 & 7;
                                        x        = regs[reg_idx0];
                                        y        = do_16bit_math(0, x, regs[reg_idx1]);
                                        set_lower_word_in_register(reg_idx1, x);
                                        set_lower_word_in_register(reg_idx0, y);
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        x        = ld_16bits_mem8_write();
                                        y        = do_16bit_math(0, x, regs[reg_idx1]);
                                        st16_mem8_write(y);
                                        set_lower_word_in_register(reg_idx1, x);
                                    }
                                    goto EXEC_LOOP;
                                case 0x1a0:    // PUSH FS SS:[rSP] Push Word, Doubleword or Quadword Onto the Stack
                                case 0x1a8:    // PUSH GS SS:[rSP] Push Word, Doubleword or Quadword Onto the Stack
                                    push_word_to_stack(segs[(OPbyte >> 3) & 7].selector);
                                    goto EXEC_LOOP;
                                case 0x1a1:    // POP SS:[rSP] FS Pop a Value from the Stack
                                case 0x1a9:    // POP SS:[rSP] GS Pop a Value from the Stack
                                    set_segment_register((OPbyte >> 3) & 7, pop_word_from_stack_read());
                                    pop_word_from_stack_incr_ptr();
                                    goto EXEC_LOOP;
                                case 0x1b2:    // LSS Mptp SS Load Far Pointer
                                case 0x1b4:    // LFS Mptp FS Load Far Pointer
                                case 0x1b5:    // LGS Mptp GS Load Far Pointer
                                    op_16_load_far_pointer16(OPbyte & 7);
                                    goto EXEC_LOOP;
                                case 0x1a4:    // SHLD Gvqp Evqp Double Precision Shift Left
                                case 0x1ac:    // SHRD Gvqp Evqp Double Precision Shift Right
                                    mem8            = phys_mem8[physmem8_ptr++];
                                    y               = regs[(mem8 >> 3) & 7];
                                    conditional_var = (OPbyte >> 3) & 1;
                                    if ((mem8 >> 6) == 3) {
                                        z        = phys_mem8[physmem8_ptr++];
                                        reg_idx0 = mem8 & 7;
                                        set_lower_word_in_register(
                                            reg_idx0, op_16_SHRD_SHLD(conditional_var, regs[reg_idx0], y, z));
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        z        = phys_mem8[physmem8_ptr++];
                                        x        = ld_16bits_mem8_write();
                                        x        = op_16_SHRD_SHLD(conditional_var, x, y, z);
                                        st16_mem8_write(x);
                                    }
                                    goto EXEC_LOOP;
                                case 0x1a5:    // SHLD Gvqp Evqp Double Precision Shift Left
                                case 0x1ad:    // SHRD Gvqp Evqp Double Precision Shift Right
                                    mem8            = phys_mem8[physmem8_ptr++];
                                    y               = regs[(mem8 >> 3) & 7];
                                    z               = regs[1];
                                    conditional_var = (OPbyte >> 3) & 1;
                                    if ((mem8 >> 6) == 3) {
                                        reg_idx0 = mem8 & 7;
                                        set_lower_word_in_register(
                                            reg_idx0, op_16_SHRD_SHLD(conditional_var, regs[reg_idx0], y, z));
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        x        = ld_16bits_mem8_write();
                                        x        = op_16_SHRD_SHLD(conditional_var, x, y, z);
                                        st16_mem8_write(x);
                                    }
                                    goto EXEC_LOOP;
                                case 0x1ba:    // BT Evqp  Bit Test
                                    mem8            = phys_mem8[physmem8_ptr++];
                                    conditional_var = (mem8 >> 3) & 7;
                                    switch (conditional_var) {
                                        case 4:
                                            if ((mem8 >> 6) == 3) {
                                                x = regs[mem8 & 7];
                                                y = phys_mem8[physmem8_ptr++];
                                            } else {
                                                mem8_loc = segment_translation(mem8);
                                                y        = phys_mem8[physmem8_ptr++];
                                                x        = ld_16bits_mem8_read();
                                            }
                                            op_16_BT(x, y);
                                            break;
                                        case 5:
                                        case 6:
                                        case 7:
                                            if ((mem8 >> 6) == 3) {
                                                reg_idx0 = mem8 & 7;
                                                y        = phys_mem8[physmem8_ptr++];
                                                regs[reg_idx0] =
                                                    op_16_BTS_BTR_BTC(conditional_var & 3, regs[reg_idx0], y);
                                            } else {
                                                mem8_loc = segment_translation(mem8);
                                                y        = phys_mem8[physmem8_ptr++];
                                                x        = ld_16bits_mem8_write();
                                                x        = op_16_BTS_BTR_BTC(conditional_var & 3, x, y);
                                                st16_mem8_write(x);
                                            }
                                            break;
                                        default:
                                            abort(6);
                                    }
                                    goto EXEC_LOOP;
                                case 0x1a3:    // BT Evqp  Bit Test
                                    mem8 = phys_mem8[physmem8_ptr++];
                                    y    = regs[(mem8 >> 3) & 7];
                                    if ((mem8 >> 6) == 3) {
                                        x = regs[mem8 & 7];
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        mem8_loc = (mem8_loc + (((y & 0xffff) >> 4) << 1)) >> 0;
                                        x        = ld_16bits_mem8_read();
                                    }
                                    op_16_BT(x, y);
                                    goto EXEC_LOOP;
                                case 0x1ab:    // BTS Gvqp Evqp Bit Test and Set
                                case 0x1b3:    // BTR Gvqp Evqp Bit Test and Reset
                                case 0x1bb:    // BTC Gvqp Evqp Bit Test and Complement
                                    mem8            = phys_mem8[physmem8_ptr++];
                                    y               = regs[(mem8 >> 3) & 7];
                                    conditional_var = (OPbyte >> 3) & 3;
                                    if ((mem8 >> 6) == 3) {
                                        reg_idx0 = mem8 & 7;
                                        set_lower_word_in_register(
                                            reg_idx0, op_16_BTS_BTR_BTC(conditional_var, regs[reg_idx0], y));
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        mem8_loc = (mem8_loc + (((y & 0xffff) >> 4) << 1)) >> 0;
                                        x        = ld_16bits_mem8_write();
                                        x        = op_16_BTS_BTR_BTC(conditional_var, x, y);
                                        st16_mem8_write(x);
                                    }
                                    goto EXEC_LOOP;
                                case 0x1bc:    // BSF Evqp Gvqp Bit Scan Forward
                                case 0x1bd:    // BSR Evqp Gvqp Bit Scan Reverse
                                    mem8     = phys_mem8[physmem8_ptr++];
                                    reg_idx1 = (mem8 >> 3) & 7;
                                    if ((mem8 >> 6) == 3) {
                                        y = regs[mem8 & 7];
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        y        = ld_16bits_mem8_read();
                                    }
                                    x = regs[reg_idx1];
                                    if (OPbyte & 1)
                                        x = op_16_BSR(x, y);
                                    else
                                        x = op_16_BSF(x, y);
                                    set_lower_word_in_register(reg_idx1, x);
                                    goto EXEC_LOOP;
                                case 0x1b1:    // CMPXCHG Gvqp Evqp Compare and Exchange
                                    mem8     = phys_mem8[physmem8_ptr++];
                                    reg_idx1 = (mem8 >> 3) & 7;
                                    if ((mem8 >> 6) == 3) {
                                        reg_idx0 = mem8 & 7;
                                        x        = regs[reg_idx0];
                                        y        = do_16bit_math(5, regs[0], x);
                                        if (y == 0) {
                                            set_lower_word_in_register(reg_idx0, regs[reg_idx1]);
                                        } else {
                                            set_lower_word_in_register(0, x);
                                        }
                                    } else {
                                        mem8_loc = segment_translation(mem8);
                                        x        = ld_16bits_mem8_write();
                                        y        = do_16bit_math(5, regs[0], x);
                                        if (y == 0) {
                                            st16_mem8_write(regs[reg_idx1]);
                                        } else {
                                            set_lower_word_in_register(0, x);
                                        }
                                    }
                                    goto EXEC_LOOP;
                                case 0x100:    // SLDT LDTR Mw Store Local Descriptor Table Register
                                case 0x101:    // SGDT GDTR Ms Store Global Descriptor Table Register
                                case 0x102:    // LAR Mw Gvqp Load Access Rights Byte
                                case 0x103:    // LSL Mw Gvqp Load Segment Limit
                                case 0x120:    // MOV Cd Rd Move to/from Control Registers
                                case 0x122:    // MOV Rd Cd Move to/from Control Registers
                                case 0x106:    // CLTS  CR0 Clear Task-Switched Flag in CR0
                                case 0x123:    // MOV Rd Dd Move to/from Debug Registers
                                case 0x1a2:    // CPUID  IA32_BIOS_SIGN_ID CPU Identification
                                case 0x131:    // RDTSC IA32_TIME_STAMP_COUNTER EAX Read Time-Stamp Counter
                                case 0x190:    // SETO  Eb Set Byte on Condition - overflow (OF=1)
                                case 0x191:    // SETNO  Eb Set Byte on Condition - not overflow (OF=0)
                                case 0x192:    // SETB  Eb Set Byte on Condition - below/not above or equal/carry (CF=1)
                                case 0x193:    // SETNB  Eb Set Byte on Condition - not below/above or equal/not carry
                                               // (CF=0)
                                case 0x194:    // SETZ  Eb Set Byte on Condition - zero/equal (ZF=0)
                                case 0x195:    // SETNZ  Eb Set Byte on Condition - not zero/not equal (ZF=1)
                                case 0x196:    // SETBE  Eb Set Byte on Condition - below or equal/not above (CF=1 AND
                                               // ZF=1)
                                case 0x197:    // SETNBE  Eb Set Byte on Condition - not below or equal/above (CF=0 AND
                                               // ZF=0)
                                case 0x198:    // SETS  Eb Set Byte on Condition - sign (SF=1)
                                case 0x199:    // SETNS  Eb Set Byte on Condition - not sign (SF=0)
                                case 0x19a:    // SETP  Eb Set Byte on Condition - parity/parity even (PF=1)
                                case 0x19b:    // SETNP  Eb Set Byte on Condition - not parity/parity odd
                                case 0x19c:    // SETL  Eb Set Byte on Condition - less/not greater (SF!=OF)
                                case 0x19d:    // SETNL  Eb Set Byte on Condition - not less/greater or equal (SF=OF)
                                case 0x19e:    // SETLE  Eb Set Byte on Condition - less or equal/not greater ((ZF=1) OR
                                               // (SF!=OF))
                                case 0x19f:    // SETNLE  Eb Set Byte on Condition - not less nor equal/greater ((ZF=0)
                                               // AND (SF=OF))
                                case 0x1b0:    // CMPXCHG Gb Eb Compare and Exchange
                                    OPbyte = 0x0f;
                                    physmem8_ptr--;
                                    break;
                                case 0x104:
                                case 0x105:    // LOADALL  AX Load All of the CPU Registers
                                case 0x107:    // LOADALL  EAX Load All of the CPU Registers
                                case 0x108:    // INVD   Invalidate Internal Caches
                                case 0x109:    // WBINVD   Write Back and Invalidate Cache
                                case 0x10a:
                                case 0x10b:    // UD2   Undefined Instruction
                                case 0x10c:
                                case 0x10d:    // NOP Ev  No Operation
                                case 0x10e:
                                case 0x10f:
                                case 0x110:    // MOVUPS Wps Vps Move Unaligned Packed Single-FP Values
                                case 0x111:    // MOVUPS Vps Wps Move Unaligned Packed Single-FP Values
                                case 0x112:    // MOVHLPS Uq Vq Move Packed Single-FP Values High to Low
                                case 0x113:    // MOVLPS Vq Mq Move Low Packed Single-FP Values
                                case 0x114:    // UNPCKLPS Wq Vps Unpack and Interleave Low Packed Single-FP Values
                                case 0x115:    // UNPCKHPS Wq Vps Unpack and Interleave High Packed Single-FP Values
                                case 0x116:    // MOVLHPS Uq Vq Move Packed Single-FP Values Low to High
                                case 0x117:    // MOVHPS Vq Mq Move High Packed Single-FP Values
                                case 0x118:    // HINT_NOP Ev  Hintable NOP
                                case 0x119:    // HINT_NOP Ev  Hintable NOP
                                case 0x11a:    // HINT_NOP Ev  Hintable NOP
                                case 0x11b:    // HINT_NOP Ev  Hintable NOP
                                case 0x11c:    // HINT_NOP Ev  Hintable NOP
                                case 0x11d:    // HINT_NOP Ev  Hintable NOP
                                case 0x11e:    // HINT_NOP Ev  Hintable NOP
                                case 0x11f:    // HINT_NOP Ev  Hintable NOP
                                case 0x121:    // MOV Dd Rd Move to/from Debug Registers
                                case 0x124:    // MOV Td Rd Move to/from Test Registers
                                case 0x125:
                                case 0x126:    // MOV Rd Td Move to/from Test Registers
                                case 0x127:
                                case 0x128:    // MOVAPS Wps Vps Move Aligned Packed Single-FP Values
                                case 0x129:    // MOVAPS Vps Wps Move Aligned Packed Single-FP Values
                                case 0x12a:    // CVTPI2PS Qpi Vps Convert Packed DW Integers to1.11 PackedSingle-FP
                                               // Values
                                case 0x12b:    // MOVNTPS Vps Mps Store Packed Single-FP Values Using Non-Temporal Hint
                                case 0x12c:    // CVTTPS2PI Wpsq Ppi Convert with Trunc. Packed Single-FP Values to1.11
                                               // PackedDW Integers
                                case 0x12d:    // CVTPS2PI Wpsq Ppi Convert Packed Single-FP Values to1.11 PackedDW
                                               // Integers
                                case 0x12e:    // UCOMISS Vss  Unordered Compare Scalar Single-FP Values and Set EFLAGS
                                case 0x12f:    // COMISS Vss  Compare Scalar Ordered Single-FP Values and Set EFLAGS
                                case 0x130:    // WRMSR rCX MSR Write to Model Specific Register
                                case 0x132:    // RDMSR rCX rAX Read from Model Specific Register
                                case 0x133:    // RDPMC PMC EAX Read Performance-Monitoring Counters
                                case 0x134:    // SYSENTER IA32_SYSENTER_CS SS Fast System Call
                                case 0x135:    // SYSEXIT IA32_SYSENTER_CS SS Fast Return from Fast System Call
                                case 0x136:
                                case 0x137:    // GETSEC EAX  GETSEC Leaf Functions
                                case 0x138:    // PSHUFB Qq Pq Packed Shuffle Bytes
                                case 0x139:
                                case 0x13a:    // ROUNDPS Wps Vps Round Packed Single-FP Values
                                case 0x13b:
                                case 0x13c:
                                case 0x13d:
                                case 0x13e:
                                case 0x13f:
                                case 0x150:    // MOVMSKPS Ups Gdqp Extract Packed Single-FP Sign Mask
                                case 0x151:    // SQRTPS Wps Vps Compute Square Roots of Packed Single-FP Values
                                case 0x152:    // RSQRTPS Wps Vps Compute Recipr. of Square Roots of Packed Single-FP
                                               // Values
                                case 0x153:    // RCPPS Wps Vps Compute Reciprocals of Packed Single-FP Values
                                case 0x154:    // ANDPS Wps Vps Bitwise Logical AND of Packed Single-FP Values
                                case 0x155:    // ANDNPS Wps Vps Bitwise Logical AND NOT of Packed Single-FP Values
                                case 0x156:    // ORPS Wps Vps Bitwise Logical OR of Single-FP Values
                                case 0x157:    // XORPS Wps Vps Bitwise Logical XOR for Single-FP Values
                                case 0x158:    // ADDPS Wps Vps Add Packed Single-FP Values
                                case 0x159:    // MULPS Wps Vps Multiply Packed Single-FP Values
                                case 0x15a:    // CVTPS2PD Wps Vpd Convert Packed Single-FP Values to1.11
                                               // PackedDouble-FP Values
                                case 0x15b:    // CVTDQ2PS Wdq Vps Convert Packed DW Integers to1.11 PackedSingle-FP
                                               // Values
                                case 0x15c:    // SUBPS Wps Vps Subtract Packed Single-FP Values
                                case 0x15d:    // MINPS Wps Vps Return Minimum Packed Single-FP Values
                                case 0x15e:    // DIVPS Wps Vps Divide Packed Single-FP Values
                                case 0x15f:    // MAXPS Wps Vps Return Maximum Packed Single-FP Values
                                case 0x160:    // PUNPCKLBW Qd Pq Unpack Low Data
                                case 0x161:    // PUNPCKLWD Qd Pq Unpack Low Data
                                case 0x162:    // PUNPCKLDQ Qd Pq Unpack Low Data
                                case 0x163:    // PACKSSWB Qd Pq Pack with Signed Saturation
                                case 0x164:    // PCMPGTB Qd Pq Compare Packed Signed Integers for Greater Than
                                case 0x165:    // PCMPGTW Qd Pq Compare Packed Signed Integers for Greater Than
                                case 0x166:    // PCMPGTD Qd Pq Compare Packed Signed Integers for Greater Than
                                case 0x167:    // PACKUSWB Qq Pq Pack with Unsigned Saturation
                                case 0x168:    // PUNPCKHBW Qq Pq Unpack High Data
                                case 0x169:    // PUNPCKHWD Qq Pq Unpack High Data
                                case 0x16a:    // PUNPCKHDQ Qq Pq Unpack High Data
                                case 0x16b:    // PACKSSDW Qq Pq Pack with Signed Saturation
                                case 0x16c:    // PUNPCKLQDQ Wdq Vdq Unpack Low Data
                                case 0x16d:    // PUNPCKHQDQ Wdq Vdq Unpack High Data
                                case 0x16e:    // MOVD Ed Pq Move Doubleword
                                case 0x16f:    // MOVQ Qq Pq Move Quadword
                                case 0x170:    // PSHUFW Qq Pq Shuffle Packed Words
                                case 0x171:    // PSRLW Ib Nq Shift Packed Data Right Logical
                                case 0x172:    // PSRLD Ib Nq Shift Double Quadword Right Logical
                                case 0x173:    // PSRLQ Ib Nq Shift Packed Data Right Logical
                                case 0x174:    // PCMPEQB Qq Pq Compare Packed Data for Equal
                                case 0x175:    // PCMPEQW Qq Pq Compare Packed Data for Equal
                                case 0x176:    // PCMPEQD Qq Pq Compare Packed Data for Equal
                                case 0x177:    // EMMS   Empty MMX Technology State
                                case 0x178:    // VMREAD Gd Ed Read Field from Virtual-Machine Control Structure
                                case 0x179:    // VMWRITE Gd  Write Field to Virtual-Machine Control Structure
                                case 0x17a:
                                case 0x17b:
                                case 0x17c:    // HADDPD Wpd Vpd Packed Double-FP Horizontal Add
                                case 0x17d:    // HSUBPD Wpd Vpd Packed Double-FP Horizontal Subtract
                                case 0x17e:    // MOVD Pq Ed Move Doubleword
                                case 0x17f:    // MOVQ Pq Qq Move Quadword
                                case 0x1a6:
                                case 0x1a7:
                                case 0x1aa:    // RSM  Flags Resume from System Management Mode
                                case 0x1ae:    // FXSAVE ST Mstx Save x87 FPU, MMX, XMM, and MXCSR State
                                case 0x1b7:    // MOVZX Ew Gvqp Move with Zero-Extend
                                case 0x1b8:    // JMPE   Jump to IA-64 Instruction Set
                                case 0x1b9:    // UD G  Undefined Instruction
                                case 0x1bf:    // MOVSX Ew Gvqp Move with Sign-Extension
                                case 0x1c0:    // XADD  Eb Exchange and Add
                                default:
                                    abort(6);
                            }
                            break;
                        default:
                            abort(6);
                    }
            }
        }
    EXEC_LOOP:;

    } while (--cycles_left);
OUTER_LOOP:

    cycle_count += (N_cycles - cycles_left);
    eip = (eip + physmem8_ptr - initial_mem_ptr);
    return exit_code;
}
