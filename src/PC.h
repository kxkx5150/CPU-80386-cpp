#ifndef _H_PC
#define _H_PC

#include "x86/x86.h"
#include <stdexcept>
#include <vector>
#ifndef NO_SDL
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#endif
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

#ifndef NO_SDL
    void paint(SDL_Renderer *render, int widht, int height);
#else
    void print();
    void input();
#endif

  private:
    x86Internal *cpu  = nullptr;
#ifndef NO_SDL
    TTF_Font    *font = nullptr;
#endif

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
