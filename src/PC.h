#ifndef _H_PC
#define _H_PC

#include "x86/x86.h"
#include <stdexcept>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <cstddef>

class PC {
  private:
  public:
    PC();
    ~PC();

    void init();
    void load(int binno, std::string path);
    void start();
    void run_cpu();

  private:
    x86Internal *cpu = nullptr;

    uint8_t *bin0 = nullptr;
    uint8_t *bin1 = nullptr;
    uint8_t *bin2 = nullptr;

    int mem_size     = 16 * 1024 * 1024;
    int start_addr   = 0x10000;
    int initrd_size  = 0;
    int cmdline_addr = 0xf800;
    int steps        = -1;

    int reset_request = 0;
};
#endif
