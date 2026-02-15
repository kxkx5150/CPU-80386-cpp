#include <cstddef>
#ifndef NO_SDL
#define SDL_MAIN_HANDLED
#endif
#include "PC.h"
#ifndef NO_SDL
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#endif
#include <cstdio>
#include <time.h>
#include <thread>

int Running = 1;

#ifndef NO_SDL
void render_loop(PC *pc, SDL_Renderer *render, int width, int height)
{
    while (Running) {
        pc->paint(render, width, height);
    }
}
#else
void print_loop(PC *pc)
{
    while (Running) {
        pc->print();
    }
}
void input_loop(PC *pc)
{
    while (Running) {
        pc->input();
    }
}
#endif // NO_SDL
int main(int ArgCount, char **Args)
{
    static const int width = 840, height = 350;
    PC              *pc = new PC();
    pc->init();
    pc->start();

    int speed = 20;

#ifndef NO_SDL
    SDL_Window *window =
        SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL);
    SDL_Renderer *render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderSetScale(render, 1, 1);

    std::thread th(render_loop, pc, render, width, height);
#else
    std::thread print(print_loop, pc);
    std::thread input(input_loop, pc);
#endif

    while (Running) {
        pc->run_cpu();
#ifndef NO_SDL
        SDL_Event Event;
        while (SDL_PollEvent(&Event)) {
            if (Event.type == SDL_QUIT)
                Running = 0;
        }
#endif
        if (speed != 100)
#ifndef NO_SDL
            SDL_Delay(1000 / speed);
#else
            std::this_thread::sleep_for(std::chrono::milliseconds(1000 / speed));
#endif
    }

#ifndef NO_SDL
    th.join();
#else
    print.join();
    input.join();
#endif
    return 0;
}
