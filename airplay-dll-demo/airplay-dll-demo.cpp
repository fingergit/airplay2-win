// airplay-dll-demo.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <windows.h>
#include <iostream>
#include "Airplay2Head.h"
#include "CAirServerCallback.h"
#include "SDL.h"
#include "CSDLPlayer.h"

int main()
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    printf("Usage: \n [s] to start server\n [q] to stop\n [-] and [=] to scale video size.\n\n");

    CSDLPlayer player;
    player.init();

    player.loopEvents();

    /* This should never happen */
    printf("SDL_WaitEvent error: %s\n", SDL_GetError());

    return 0;
}
