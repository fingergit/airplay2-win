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
    CSDLPlayer player;
    player.init();

    player.loopEvents();

    /* This should never happen */
    printf("SDL_WaitEvent error: %s\n", SDL_GetError());
    exit(1);
}
