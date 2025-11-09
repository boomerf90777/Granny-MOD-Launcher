#include <windows.h>
#include <shellapi.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

// Структура для хранения информации о моде
struct ModInfo {
    std::wstring name;
    std::wstring developer;
    std::wstring year;
    std::wstring link;
    std::wstring path; // путь к папке мода
    std::wstring exePath; // путь к exe, если есть
};

// Глобальные переменные
std::vector<ModInfo> mods;
int selectedModIndex = -1;

// Путь к папке mods
std::wstring modsFolder = L"mods";

// Глобальный HWND для надписи
HWND hwndFooter = NULL;

// Объявление функций
void LoadMods();
void ShowModsList(HWND hwndList);
void ShowModInfo(HWND hwndText);
void LaunchMod(const ModInfo& mod);
void LaunchOriginalGame();
void ResizeFooter(HWND hwnd, HWND hwndFooter);
std::wstring GetExecutableFolder();

// Обработчик сообщений окна
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hwndList, hwndInfo, hwndLaunchBtn, hwndRefreshBtn, hwndLaunchOrigBtn;
    switch (uMsg) {
    case WM_CREATE:
        hwndList = CreateWindow(L"LISTBOX", NULL,
            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL,
            10, 10, 200, 150,
            hwnd, (HMENU)1, NULL, NULL);

        hwndInfo = CreateWindow(L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY | ES_MULTILINE,
            220, 10, 300, 150,
            hwnd, (HMENU)2, NULL, NULL);

        hwndLaunchBtn = CreateWindow(L"BUTTON", L"Launch mod",
            WS_CHILD | WS_VISIBLE,
            10, 170, 120, 30,
            hwnd, (HMENU)3, NULL, NULL);

        hwndRefreshBtn = CreateWindow(L"BUTTON", L"Refresh",
            WS_CHILD | WS_VISIBLE,
            140, 170, 120, 30,
            hwnd, (HMENU)4, NULL, NULL);

        // добавляем кнопку "Запустить оригинальную игру"
        hwndLaunchOrigBtn = CreateWindow(L"BUTTON", L"Launch original game",
            WS_CHILD | WS_VISIBLE,
            10, 210, 200, 30,
            hwnd, (HMENU)5, NULL, NULL);

        // создаем надпись "made by _boomerf90777_" в правом нижнем углу
        hwndFooter = CreateWindow(
            L"STATIC", L"made by _boomerf90777_",
            WS_CHILD | WS_VISIBLE | SS_RIGHT,
            0, 0, 200, 20,
            hwnd, NULL, NULL, NULL);
        ResizeFooter(hwnd, hwndFooter);

        // Вызов функции загрузки модов
        LoadMods();
        ShowModsList(hwndList);
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == 1) { // выбор в списке
            int sel = (int)SendMessage(hwndList, LB_GETCURSEL, 0, 0);
            selectedModIndex = sel;
            HWND hwndInfo = GetDlgItem(GetParent(hwndList), 2);
            ShowModInfo(hwndInfo);
        }
        else if (LOWORD(wParam) == 3) { // запуск мода
            if (selectedModIndex >= 0 && selectedModIndex < (int)mods.size()) {
                LaunchMod(mods[selectedModIndex]);
            }
        }
        else if (LOWORD(wParam) == 4) { // обновить список
            LoadMods();
            ShowModsList(hwndList);
            if (!mods.empty()) {
                SendMessage(hwndList, LB_SETCURSEL, 0, 0);
                selectedModIndex = 0;
                HWND hwndInfo = GetDlgItem(GetParent(hwndList), 2);
                ShowModInfo(hwndInfo);
            }
            else {
                SetWindowText(GetDlgItem(GetParent(hwndList), 2), L"");
            }
        }
        else if (LOWORD(wParam) == 5) { // запуск оригинальной игры
            LaunchOriginalGame();
        }
        return 0;

    case WM_SIZE:
        if (hwndFooter != NULL) {
            ResizeFooter(hwnd, hwndFooter);
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void LoadMods() {
    mods.clear();
    WIN32_FIND_DATA findFileData;
    std::wstring searchPath = modsFolder + L"\\*";

    HANDLE hFind = FindFirstFile(searchPath.c_str(), &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            std::wstring folderName = findFileData.cFileName;
            if (folderName != L"." && folderName != L"..") {
                ModInfo mod;
                mod.path = modsFolder + L"\\" + folderName;

                // ищем ini по исправленной логике
                std::wstring iniPath = mod.path + L"\\mod.ini";
                if (GetFileAttributes(iniPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
                    std::wifstream iniFile(iniPath);
                    std::wstring line;
                    while (std::getline(iniFile, line)) {
                        size_t pos = line.find(L"=");
                        if (pos != std::wstring::npos) {
                            std::wstring key = line.substr(0, pos);
                            std::wstring value = line.substr(pos + 1);
                            // trim
                            key.erase(0, key.find_first_not_of(L" \t"));
                            key.erase(key.find_last_not_of(L" \t") + 1);
                            value.erase(0, value.find_first_not_of(L" \t"));
                            value.erase(value.find_last_not_of(L" \t") + 1);

                            if (key == L"Title") {
                                mod.name = value;
                            }
                            else if (key == L"Developer") {
                                mod.developer = value;
                            }
                            else if (key == L"Year") {
                                mod.year = value;
                            }
                            else if (key == L"Link") {
                                mod.link = value;
                            }
                        }
                    }
                }
                if (mod.name.empty()) {
                    // Название по папке, если не указано в ini
                    size_t pos = folderName.find_last_of(L"\\/");
                    mod.name = folderName;
                }

                // ищем exe
                WIN32_FIND_DATA exeFind;
                std::wstring exeSearch = mod.path + L"\\*.exe";
                HANDLE hExe = FindFirstFile(exeSearch.c_str(), &exeFind);
                if (hExe != INVALID_HANDLE_VALUE) {
                    mod.exePath = mod.path + L"\\" + exeFind.cFileName;
                    FindClose(hExe);
                }
                else {
                    mod.exePath = L"";
                }

                mods.push_back(mod);
            }
        }
    } while (FindNextFile(hFind, &findFileData) != 0);
    FindClose(hFind);
}

void ShowModsList(HWND hwndList) {
    SendMessage(hwndList, LB_RESETCONTENT, 0, 0);
    for (size_t i = 0; i < mods.size(); ++i) {
        SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)mods[i].name.c_str());
    }
    if (!mods.empty()) {
        SendMessage(hwndList, LB_SETCURSEL, 0, 0);
        selectedModIndex = 0;
        HWND hwndInfo = GetDlgItem(GetParent(hwndList), 2);
        ShowModInfo(hwndInfo);
    }
}

void ShowModInfo(HWND hwndText) {
    if (selectedModIndex >= 0 && selectedModIndex < (int)mods.size()) {
        const ModInfo& m = mods[selectedModIndex];
        std::wstringstream ss;
        ss << L"Title: " << m.name << L"\r\n";
        ss << L"Developer: " << m.developer << L"\r\n";
        ss << L"Year: " << m.year << L"\r\n";
        ss << L"Link: " << m.link;
        SetWindowText(hwndText, ss.str().c_str());
    }
}

void LaunchMod(const ModInfo& mod) {
    if (!mod.exePath.empty()) {
        ShellExecuteW(NULL, L"open", mod.exePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
    else {
        ShellExecuteW(NULL, L"explore", mod.path.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
}

void LaunchOriginalGame() {
    // Запуск через Steam с ID игры
    const wchar_t* steamCommand = L"steam://rungameid/962400";
    ShellExecuteW(NULL, L"open", steamCommand, NULL, NULL, SW_SHOWNORMAL);
}

void ResizeFooter(HWND hwnd, HWND hwndFooter) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    int footerWidth = 200;
    int footerHeight = 20;
    int x = rc.right - footerWidth - 10; // отступ слева
    int y = rc.bottom - footerHeight - 10; // отступ сверху
    SetWindowPos(hwndFooter, NULL, x, y, footerWidth, footerHeight, SWP_NOZORDER);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ModLauncherClass";

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        wc.lpszClassName,
        L"Granny MOD Manager",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 540, 290,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
