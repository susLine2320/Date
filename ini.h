#include <windows.h>
#include <string>

// DLLのパスを取得して、同じフォルダの .ini パスを作る
std::wstring GetIniPath() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(GetModuleHandleW(L"Ats.dll"), path, MAX_PATH);
    std::wstring iniPath = path;
    size_t pos = iniPath.find_last_of(L"\\/");
    return iniPath.substr(0, pos + 1) + L"date.ini";
}

// 数値を読み込む
int LoadInt(const wchar_t* section, const wchar_t* key, int defaultValue) {
    return GetPrivateProfileIntW(section, key, defaultValue, GetIniPath().c_str());
}