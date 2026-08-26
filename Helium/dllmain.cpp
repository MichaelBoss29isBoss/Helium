// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "redirection.h"



namespace Helium {

    DWORD WINAPI MainThread(LPVOID lpReserved) {

        if (HeliumConfig::bConsole)
        {
            AllocConsole();

            FILE* fptr;
            freopen_s(&fptr, "CONOUT$", "w+", stdout);

            SetConsoleTitleA("Helium - Made by: Ducki67 (@ducki67 on discord) | Dllt buil on: " __DATE__ " " __TIME__);

            cout << "[Helium] Console initalized!! \n\n";
            cout <<"Helium - Made by : Ducki67(@ducki67 on discord) | Dll built on : " __DATE__ " " __TIME__;
            cout << "\n\n";

        }
        Sleep(2400); // so your game doesnt crash ig
        hookCurl();
        return 0;
    }
  
};



BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        
        DisableThreadLibraryCalls(hModule);

        {
            HANDLE hThread = CreateThread(nullptr, 0, Helium::MainThread, hModule, 0, nullptr);
            if (hThread) {
                CloseHandle(hThread); 
            }
        }
        break;

    case DLL_PROCESS_DETACH:
        
        break;
    }
    return TRUE;
}

