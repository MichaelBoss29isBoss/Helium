#pragma once
#include "pch.h"


//std::regex ohost("https:\\/\\/(.*)\\.ol\\.epicgames.com");
std::regex ohost("https:\\/\\/(.*)\\.epicgames\\.com");
std::string nhost(HeliumConfig::sBackendUrl);

bool shouldRedirect(const std::string& url) {
    static const char* domains[] = {
        ".ol.epicgames.com",
        ".ol.epicgames.net",
        ".on.epicgames.com",
        ".game-social.epicgames.com",
        ".ak.epicgames.com",
        ".epicgames.dev"
    };
    for (const auto& d : domains) {
        if (url.find(d) != std::string::npos) return true;
    }
    return false;
}

using t_curl_easy_setopt = int (*)(void* handle, int option, ...);
t_curl_easy_setopt oCurlEasySetopt = nullptr;

int hkCurlEasySetopt(void* handle, int option, void* value, void* value2) {
    if (!handle) return HeliumConfig::iCURLE_BAD_FUNCTION_ARGUMENT;

    if (option == HeliumConfig::iCURLOPT_SSL_VERIFYPEER) {
        return oCurlEasySetopt(handle, option, (void*)0);
    }

    if (option == HeliumConfig::iCURLOPT_URL) {
        const char* url = (const char*)value;
        std::string ourl(url ? url : "");

        if (shouldRedirect(ourl)) {
            ourl = std::regex_replace(ourl, ohost, nhost);
        }

        return oCurlEasySetopt(handle, option, (void*)ourl.c_str());
    }

    return oCurlEasySetopt(handle, option, value, value2);
}

uintptr_t sigscan(const wchar_t* pattern) {
    MODULEINFO modInfo;
    HMODULE hModule = GetModuleHandle(NULL);
    GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(MODULEINFO));

    uintptr_t base = (uintptr_t)modInfo.lpBaseOfDll;
    uintptr_t size = (uintptr_t)modInfo.SizeOfImage;

    size_t pattern_len = wcslen(pattern);
    for (uintptr_t i = base; i < base + size - pattern_len; i++) {
        bool match = true;
        for (size_t k = 0; k < pattern_len; k++) {
            if (((const unsigned char*)i)[k] != (pattern[k] & 0xFF)) {
                match = false;
                break;
            }
        }
        if (match) return i;
    }

    return 0;
}


static int modrmLen(const unsigned char* c, int modrmOff) {
    int len = 1;
    unsigned char m = c[modrmOff];
    int mod = (m >> 6) & 3;
    int rm = m & 7;
    if (mod != 3 && rm == 4) {
        len++;
        unsigned char sib = c[modrmOff + 1];
        int base = sib & 7;
        if (base == 5 && mod == 0) len += 4;
    }
    if (mod == 0 && rm == 5) len += 4;
    else if (mod == 1) len += 1;
    else if (mod == 2) len += 4;
    return len;
}

static int InstrLen(const unsigned char* c) {
    int i = 0;
    bool rexW = false;
    while (true) {
        unsigned char b = c[i];
        if (b == 0x66 || b == 0x67 || b == 0xF0 || b == 0xF2 || b == 0xF3 ||
            b == 0x2E || b == 0x36 || b == 0x3E || b == 0x26 || b == 0x64 || b == 0x65) { i++; continue; }
        if (b >= 0x40 && b <= 0x4F) { rexW = (b & 0x08) != 0; i++; continue; }
        break;
    }
    unsigned char op = c[i]; i++;
    bool two = false;
    if (op == 0x0F) { two = true; op = c[i]; i++; }

    if (two) {
        if ((op >= 0x10 && op <= 0x17) || (op >= 0x28 && op <= 0x2F) || (op >= 0x54 && op <= 0x5F) ||
            (op >= 0x6E && op <= 0x6F) || (op >= 0x7E && op <= 0x7F) || (op >= 0x80 && op <= 0x8F) ||
            (op >= 0x90 && op <= 0x97) || (op >= 0x40 && op <= 0x4F) || op == 0xAF || op == 0xB6 || op == 0xB7 ||
            op == 0xBE || op == 0xBF) { i += modrmLen(c, i); return i; }
        if (op == 0x05 || op == 0x0D) { i += 4; return i; }
        return -1;
    }

    if (op >= 0x50 && op <= 0x5F) return i;
    if (op == 0x68 || op == 0x6A) { i += (op == 0x68 ? 4 : 1); return i; }
    if (op >= 0x70 && op <= 0x7F) { i += 1; return i; }
    if (op == 0xEB) { i += 1; return i; }
    if (op == 0xE8 || op == 0xE9) { i += 4; return i; }
    if (op == 0xC3) return i;
    if (op == 0xC2) { i += 2; return i; }
    if (op == 0x90) return i;
    if (op == 0x98 || op == 0x99 || op == 0x9C || op == 0x9D || op == 0xC9) return i;
    if (op >= 0xB0 && op <= 0xB7) { i += 1; return i; }
    if (op >= 0xB8 && op <= 0xBF) { i += (rexW ? 8 : 4); return i; }
    if (op == 0xC6 || op == 0xC7) { i += modrmLen(c, i); i += (op == 0xC6 ? 1 : (rexW ? 8 : 4)); return i; }
    if (op == 0x80 || op == 0x81 || op == 0x83) { i += modrmLen(c, i); i += (op == 0x81 ? (rexW ? 8 : 4) : 1); return i; }
    if (op == 0x88 || op == 0x89 || op == 0x8A || op == 0x8B || op == 0x8C || op == 0x8D || op == 0x8E) { i += modrmLen(c, i); return i; }
    if (op == 0x84 || op == 0x85) { i += modrmLen(c, i); return i; }
    if (op >= 0x00 && op <= 0x3D) {
        if (op == 0x04 || op == 0x05 || op == 0x0C || op == 0x0D || op == 0x14 || op == 0x15 || op == 0x24 || op == 0x25 ||
            op == 0x2C || op == 0x2D || op == 0x34 || op == 0x35 || op == 0x3C || op == 0x3D) { i += (rexW ? 8 : 4); return i; }
        if ((op & 0xC0) == 0x00 || (op & 0xC0) == 0x40 || (op & 0xC0) == 0x80) { i += modrmLen(c, i); return i; }
        return i;
    }
    if (op == 0x63) { i += modrmLen(c, i); return i; }
    if (op == 0xF6 || op == 0xF7) { unsigned char m = c[i]; i += modrmLen(c, i); if ((m & 0x38) == 0x00) i += (op == 0xF6 ? 1 : (rexW ? 8 : 4)); return i; }
    if (op == 0xFF) { i += modrmLen(c, i); return i; }
    return -1;
}

static void* MakeTrampoline(uintptr_t target, size_t& outN) {
    int n = 0;
    while (n < 12) {
        int l = InstrLen((const unsigned char*)target + n);
        if (l <= 0) { if (n < 12) n = 12; break; }
        n += l;
    }
    outN = (size_t)n;
    uintptr_t tramp = (uintptr_t)VirtualAlloc(NULL, n + 12, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (!tramp) return nullptr;
    memcpy((void*)tramp, (void*)target, n);
    BYTE* p = (BYTE*)(tramp + n);
    p[0] = 0x48; p[1] = 0xB8;
    *(unsigned __int64*)(p + 2) = (unsigned __int64)(target + n);
    p[10] = 0xFF; p[11] = 0xE0;
    return (void*)tramp;
}

void hookCurl() {
    const wchar_t* pattern = HeliumConfig::sCurl_Easy_Setopt_SIG;
    uintptr_t aCurlEasySetopt = sigscan(pattern);

    if (aCurlEasySetopt == 0) {
        MessageBox(NULL, L"Failed to find curl_easy_setopt! (Pattern may be outdated for current season)", L"Error", MB_OK);
        return;
    }

    size_t patch;
    void* tramp = MakeTrampoline(aCurlEasySetopt, patch);
    if (!tramp) {
        MessageBox(NULL, L"Failed to allocate trampoline!", L"Error", MB_OK);
        return;
    }
    oCurlEasySetopt = (t_curl_easy_setopt)tramp;

    DWORD oldProtect;
    if (!VirtualProtect((LPVOID)aCurlEasySetopt, patch, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        std::cerr << "Failed to change memory protection!" << std::endl;
        return;
    }

    BYTE buf[32];
    memset(buf, 0x90, sizeof(buf));
    buf[0] = 0x48; buf[1] = 0xB8;
    *(unsigned __int64*)(buf + 2) = (unsigned __int64)(UINT_PTR)hkCurlEasySetopt;
    buf[10] = 0xFF; buf[11] = 0xE0;
    memcpy((void*)aCurlEasySetopt, buf, patch);

    VirtualProtect((LPVOID)aCurlEasySetopt, patch, oldProtect, &oldProtect);

    std::wcout << L"curl was hooked at address: 0x" << std::hex << aCurlEasySetopt << std::endl;
}

