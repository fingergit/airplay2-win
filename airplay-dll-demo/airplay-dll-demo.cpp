// airplay-dll-demo.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <windows.h>
#include <iostream>
#include "Airplay2Head.h"
#include "CAirServerCallback.h"

int main()
{
    std::cout << "Hello World!\n";
    CAirServerCallback* callback = new CAirServerCallback();

    void* server = fgServerStart("FgAirplay", callback);

    while (true)
    {
        Sleep(1000);
    }

    if (server != NULL) {
        fgServerStop(server);
    }
    delete callback;
}
