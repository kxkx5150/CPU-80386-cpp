#include <cstddef>
#define SDL_MAIN_HANDLED
#include "PC.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <cstdio>
#include <time.h>
#include <thread>

int Running = 1;

void render_loop(PC *pc, SDL_Renderer *render, int width, int height)
{
    while (Running) {
        pc->paint(render, width, height);
    }
}
int main(int ArgCount, char **Args)
{
    static const int width = 560, height = 300;
    PC              *pc = new PC();
    pc->init();
    pc->start();

    SDL_Window *window =
        SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL);
    SDL_Renderer *render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderSetScale(render, 1, 1);

    std::thread th(render_loop, pc, render, width, height);

    while (Running) {
        pc->run_cpu();
        SDL_Event Event;
        while (SDL_PollEvent(&Event)) {
            if (Event.type == SDL_QUIT)
                Running = 0;
        }
    }

    th.join();
    return 0;
}
