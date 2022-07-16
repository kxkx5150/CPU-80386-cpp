#include <cstdint>
#include <cstdio>
#include <string>
#include <thread>
#include <chrono>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "PC.h"

PC::PC()
{
    cpu = new x86Internal(mem_size);

    TTF_Init();
    font = TTF_OpenFont("bin/cp437.ttf", 14);
    if (font == NULL) {
        printf("error: font not found\n");
        exit(EXIT_FAILURE);
    }
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
    auto __     = fread(buffer, size, 1, f);

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
    printf("\n\n************************\n");
    printf("************************\n");
    printf("****** Boot Linux ******\n");
    printf("************************\n");
    printf("************************\n\n\n");
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
    // if (!do_reset) {
    //     // std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // }
}
void PC::paint(SDL_Renderer *renderer, int widht, int height)
{
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 22, 22, 22, SDL_ALPHA_OPAQUE);
    SDL_Rect rect;
    rect.x = 0;
    rect.y = 0;
    rect.w = 560;
    rect.h = 300;

    SDL_RenderFillRect(renderer, &rect);
    int       stridx = cpu->serial->strbufs_idx;
    SDL_Color color  = {255, 255, 255};

    for (int y = 0; y < 25; ++y) {
        string str = "";

        if (24 < stridx) {
            str = cpu->serial->strbufs[stridx - 24 + y];
        } else {
            str = cpu->serial->strbufs[y];
        }
        int strlen = str.length();
        if (strlen == 0)
            continue;

        SDL_Rect r1;
        r1.x = 0;
        r1.y = y * 14;
        r1.w = strlen * 9;
        r1.h = 14;

        SDL_Surface *text_surface = TTF_RenderUTF8_Solid(font, str.c_str(), color);
        SDL_Texture *Message      = SDL_CreateTextureFromSurface(renderer, text_surface);

        SDL_RenderCopy(renderer, Message, NULL, &r1);
        SDL_FreeSurface(text_surface);
        SDL_DestroyTexture(Message);
    }
    SDL_RenderPresent(renderer);
    SDL_Delay(10);
}
