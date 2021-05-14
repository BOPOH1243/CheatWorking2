#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <thread>
#include <comdef.h>
struct offsets {
    const DWORD dwLocalPlayer = 0xD8E2CC;
    const DWORD dwEntityList = 0x4DA720C;
    const DWORD m_iTeamNum = 0xF4;
    const DWORD m_iGlowIndex = 0xA438;
    const DWORD dwGlowObjectManager = 0x52EF6B0;
    const DWORD m_bSpotted = 0x93D;
}offset;
struct states {
    bool wh = true;
    bool rh = true;
}state;


HANDLE process; // непосредственно сам процесс CSGO
DWORD clientBase; // клиентская часть
DWORD engineBase; // игровая часть (движок)
template <typename T>
T readMem(DWORD address)
{
    T buffer;
    ReadProcessMemory(process, (LPVOID)address, &buffer, sizeof(buffer), 0);
    return buffer;
}

template <typename T>
void writeMem(DWORD address, T value)
{
    WriteProcessMemory(process, (LPVOID)address, &value, sizeof(value), 0);
}


void wallhack()
{
    std::cout << "WH started\n";
    while (true) // создаем бесконечный цикл
    {
        Sleep(10); // таймаут 10 мс, чтобы не грузить процессор под 100
        if (!state.wh && !readMem<DWORD>(readMem<DWORD>(clientBase + offset.dwLocalPlayer) + 0xED)) // если вх выключено или не удается прочитать память - выходим из цикла
            continue;

        DWORD glowObj = readMem<DWORD>(clientBase + offset.dwGlowObjectManager); // создаем объект glowObj из модельки игрока
        DWORD myTeam = readMem<DWORD>(readMem<DWORD>(clientBase + offset.dwLocalPlayer) + offset.m_iTeamNum); // создаем объект тиммейтов

        for (int x = 0; x < 32; x++) // сам вх
        {
            DWORD player = readMem<DWORD>(clientBase + offset.dwEntityList + x * 0x10); // обычный игрок
            if (player == 0)
                continue;

            bool dormant = readMem<bool>(player + 0xED); // спектатор
            if (dormant)
                continue;

            DWORD team = readMem<DWORD>(player + offset.m_iTeamNum); // тиммейт
            if (team != 2 && team != 3)
                continue;

            DWORD currentGlowIndex = readMem<DWORD>(player + offset.m_iGlowIndex); // текущий индекс игрока

            if (team != myTeam) // если игрок не тиммейт
            {
                // делаем его обводку красным
                writeMem<float>(glowObj + currentGlowIndex * 0x38 + 0x4, 255); // red
                writeMem<float>(glowObj + currentGlowIndex * 0x38 + 0x8, 0); // green
                writeMem<float>(glowObj + currentGlowIndex * 0x38 + 0xC, 0); // blue
                writeMem<float>(glowObj + currentGlowIndex * 0x38 + 0x10, 255);
                writeMem<bool>(glowObj + currentGlowIndex * 0x38 + 0x24, true);
                writeMem<bool>(glowObj + currentGlowIndex * 0x38 + 0x25, false);
            }
            else // если игрок тиммейт
            {
                // делаем его обводку синим
                writeMem<float>(glowObj + currentGlowIndex * 0x38 + 0x4, 0); // red
                writeMem<float>(glowObj + currentGlowIndex * 0x38 + 0x8, 0); // green
                writeMem<float>(glowObj + currentGlowIndex * 0x38 + 0xC, 255); // blue
                writeMem<float>(glowObj + currentGlowIndex * 0x38 + 0x10, 255);
                writeMem<bool>(glowObj + currentGlowIndex * 0x38 + 0x24, true);
                writeMem<bool>(glowObj + currentGlowIndex * 0x38 + 0x25, false);
            }
        }

    }
}
uintptr_t GetModuleBaseAddress(DWORD dwProcID, const char* szModuleName)
{
    uintptr_t ModuleBaseAddress = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, dwProcID);
    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 ModuleEntry32;
        ModuleEntry32.dwSize = sizeof(MODULEENTRY32);
        if (Module32First(hSnapshot, &ModuleEntry32))
        {
            do
            {
                if (strcmp(ModuleEntry32.szModule, szModuleName) == 0)
                {
                    ModuleBaseAddress = (uintptr_t)ModuleEntry32.modBaseAddr;
                    break;
                }
            } while (Module32Next(hSnapshot, &ModuleEntry32));
        }
        CloseHandle(hSnapshot);
    }
    return ModuleBaseAddress;
}

DWORD getModuleBaseAddress(DWORD pid, const char* name)
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    MODULEENTRY32 mEntry;
    mEntry.dwSize = sizeof(MODULEENTRY32);
    do
    {

        if (!strcmp(mEntry.szModule, name))
        {
            CloseHandle(snapshot);
            return (DWORD)mEntry.modBaseAddr;
        }
    } while (Module32Next(snapshot, &mEntry));
}
int main() {

    SetConsoleTitle("Top non-pasted shit 1337"); // устанавливаем заголовок нашей консоли

    std::cout << "Open CS:GO\n"; // выводим в консоль сообщение о том, что надо открыть ксго

    HWND hwnd;

    do {
        hwnd = FindWindowA(0, "Counter-Strike: Global Offensive"); // ищем ксго, если находим - выходим из цикла
        Sleep(50); // таймаут (чтобы не грузить процессор)
    } while (!hwnd);

    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid); // получаем id приложения
    process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid); // заходим в кс го его id

    std::cout << "Csgo started, pid " << pid << ".\n"; // выводим сообщение о том, что ксго запущена

    do {
        clientBase = GetModuleBaseAddress(pid, "client.dll");// ищем клиент кс го panoramauiclient.dll
        Sleep(50);
    } while (!clientBase);
    std::cout << "panorama gefunden\n";

    do {
        engineBase = GetModuleBaseAddress(pid, "engine.dll"); // ищем движок кс го
        Sleep(50);
    } while (!engineBase);
    std::cout << "engine gefunden\n";
    std::thread whThread(wallhack);
    while (true)
    {

        if (GetAsyncKeyState(VK_F9)) // если нажали f9
        {
            bool wh = state.wh;
            wh = !wh; // заменяем значение переменной на противоположное
            if (wh)
                std::cout<<("wh: on\n"); // если wallhack - true, то пишем, что вх включен
            else
                std::cout<<("wh: off\n"); // иначе пишем, что вх выключен

            Sleep(100); // таймаут, чтобы сбросить нагрузку

        }

    }
}



