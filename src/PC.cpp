#include <cstdint>
#include <cstdio>
#include <thread>
#include <chrono>
#include "PC.h"

PC::PC()
{
    cpu = new x86Internal(mem_size);
}
PC::~PC()
{
    delete[] bin0;
    delete[] bin1;
    delete[] bin2;
    delete cpu;
}
void PC::init()
{
    printf("load file\n");
    load(0, "bin/vmlinux-2.6.20.bin");
    load(1, "bin/root.bin");
    load(2, "bin/linuxstart.bin");
}
void PC::load(int binno, std::string path)
{
    FILE *f = fopen(path.c_str(), "rb");
    fseek(f, 0, SEEK_END);
    const int size = ftell(f);
    fseek(f, 0, SEEK_SET);
    auto buffer = new uint8_t[size];
    fread(buffer, size, 1, f);

    int offset = 0;
    if (binno == 0) {
        bin0   = buffer;
        offset = 0x00100000;
    } else if (binno == 1) {
        bin1        = buffer;
        offset      = 0x00400000;
        initrd_size = size;
    } else if (binno == 2) {
        bin2   = buffer;
        offset = 0x10000;
    }

    cpu->load(buffer, offset, size);
    fclose(f);
}
void PC::start()
{
    cpu->write_string(cmdline_addr, "console=ttyS0 root=/dev/ram0 rw init=/sbin/init notsc=1");
    cpu->start(start_addr, initrd_size, cmdline_addr);
}
void PC::run_cpu()
{
    std::size_t Ncycles     = cpu->cycle_count + 100000;
    bool        do_reset    = false;
    bool        err_on_exit = false;

    while (cpu->cycle_count < Ncycles) {
        cpu->pit->update_irq();

        int exit_status = cpu->exec(Ncycles - cpu->cycle_count);
        if (exit_status == 256) {
            if (reset_request) {
                do_reset = true;
                break;
            }
        } else if (exit_status == 257) {
            err_on_exit = true;
            break;
        } else {
            do_reset = true;
            break;
        }
    }
    if (!do_reset) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        run_cpu();
    }
}
