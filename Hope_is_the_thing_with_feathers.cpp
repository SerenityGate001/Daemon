#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include <algorithm>

#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

struct Achievement { std::string name; std::string date; };
struct Event { int id; std::string title; std::string date; std::string location; std::string status; };
struct Participation { int userId; int eventId; std::string role; };
struct User { int id; std::string name; std::string email; std::string country; std::string login; std::string password; };

static std::vector<User> users;
static std::vector<Achievement> achievements;
static std::vector<Event> events;
static std::vector<Participation> participations;
static int currentUserId = 1;
static bool isLoggedIn = true;
static bool isAppRunning = true;
static HWND mainWnd = nullptr;
static HWND hLoginBtn = nullptr;
static HWND hAchievementsBtn = nullptr;
static HWND hHistoryBtn = nullptr;
static HWND hEventsBtn = nullptr;
static HWND hProfileBtn = nullptr;
static HWND hAboutBtn = nullptr;
static HWND hExitBtn = nullptr;
static HWND hGuideBtn = nullptr;

static std::string W2A(const std::wstring& w) {
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_ACP, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string result(len - 1, 0);
    WideCharToMultiByte(CP_ACP, 0, w.c_str(), -1, &result[0], len, nullptr, nullptr);
    return result;
}

static std::wstring A2W(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, nullptr, 0);
    std::wstring result(len - 1, 0);
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, &result[0], len);
    return result;
}

static void InitData() {
    users.push_back({ 1, "Даниил", "dan@daimoniy.com", "Россия", "dan", "123" });
    achievements.push_back({ "Первый шаг", "2025-01-01" });
    achievements.push_back({ "Опытный активист", "2025-03-15" });
    achievements.push_back({ "Эко-герой", "2025-05-01" });
    events.push_back({ 1, "Озеленение Петрозаводска", "2025-06-01", "Петрозаводск", "активно" });
    events.push_back({ 2, "Уборка побережья", "2025-07-10", "Сочи", "активно" });
    events.push_back({ 3, "Лекция о климате", "2025-04-15", "Москва (онлайн)", "завершено" });
    events.push_back({ 4, "Посадка леса", "2025-08-20", "Карелия", "планируется" });
    participations.push_back({ 1, 1, "участник" });
    participations.push_back({ 1, 3, "волонтёр" });
    participations.push_back({ 1, 4, "заявка подана" });
}

static void CenterWindow(HWND hWnd) {
    RECT rc;
    GetWindowRect(hWnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(hWnd, NULL, (sw - w) / 2, (sh - h) / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

static HWND hAchievementsWnd = nullptr;
static LRESULT CALLBACK AchievementsWndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_CREATE: {
        CreateWindowW(L"STATIC", L"ДОСТИЖЕНИЯ", WS_VISIBLE | WS_CHILD | SS_CENTER,
            10, 10, 460, 30, hwnd, NULL, NULL, NULL);
        HWND list = CreateWindowW(L"LISTBOX", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL,
            10, 50, 460, 200, hwnd, (HMENU)1001, NULL, NULL);
        for (auto& a : achievements) {
            std::string line = a.name + " (получено: " + a.date + ")";
            SendMessageA(list, LB_ADDSTRING, 0, (LPARAM)line.c_str());
        }
        CreateWindowW(L"BUTTON", L"ВЫЙТИ", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            180, 270, 120, 35, hwnd, (HMENU)IDOK, NULL, NULL);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(w) == IDOK) { DestroyWindow(hwnd); hAchievementsWnd = nullptr; }
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        hAchievementsWnd = nullptr;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w, l);
}

static void ShowAchievementsWindow() {
    if (hAchievementsWnd) { SetForegroundWindow(hAchievementsWnd); return; }
    hAchievementsWnd = CreateWindowExW(0, L"AchievementsClass", L"Достижения",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 350, mainWnd, NULL, GetModuleHandle(NULL), NULL);
    CenterWindow(hAchievementsWnd);
}

static HWND hHistoryWnd = nullptr;
static LRESULT CALLBACK HistoryWndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_CREATE: {
        CreateWindowW(L"STATIC", L"ИСТОРИЯ УЧАСТИЯ", WS_VISIBLE | WS_CHILD | SS_CENTER,
            10, 10, 460, 30, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"STATIC", L"Фильтры поиска:", WS_VISIBLE | WS_CHILD, 10, 55, 120, 25, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 130, 55, 200, 25, hwnd, (HMENU)2001, NULL, NULL);
        CreateWindowW(L"BUTTON", L"ПОИСК", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 340, 55, 80, 25, hwnd, (HMENU)2002, NULL, NULL);
        HWND list = CreateWindowW(L"LISTBOX", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL,
            10, 95, 460, 180, hwnd, (HMENU)2003, NULL, NULL);
        for (auto& p : participations) {
            if (p.userId == currentUserId) {
                for (auto& e : events) {
                    if (e.id == p.eventId) {
                        std::string line = e.title + " | " + e.date + " | " + p.role;
                        SendMessageA(list, LB_ADDSTRING, 0, (LPARAM)line.c_str());
                        break;
                    }
                }
            }
        }
        CreateWindowW(L"BUTTON", L"ВЫЙТИ", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 180, 290, 120, 35, hwnd, (HMENU)IDOK, NULL, NULL);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(w) == IDOK) { DestroyWindow(hwnd); hHistoryWnd = nullptr; }
        if (LOWORD(w) == 2002) {
            char filter[256] = { 0 };
            GetDlgItemTextA(hwnd, 2001, filter, 255);
            HWND list = GetDlgItem(hwnd, 2003);
            SendMessageA(list, LB_RESETCONTENT, 0, 0);
            std::string filterStr = filter;
            for (auto& p : participations) {
                if (p.userId == currentUserId) {
                    for (auto& e : events) {
                        if (e.id == p.eventId) {
                            std::string line = e.title + " | " + e.date + " | " + p.role;
                            if (filterStr.empty() || (e.title.find(filterStr) != std::string::npos) || (e.date.find(filterStr) != std::string::npos)) {
                                SendMessageA(list, LB_ADDSTRING, 0, (LPARAM)line.c_str());
                            }
                            break;
                        }
                    }
                }
            }
        }
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        hHistoryWnd = nullptr;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w, l);
}

static void ShowHistoryWindow() {
    if (hHistoryWnd) { SetForegroundWindow(hHistoryWnd); return; }
    hHistoryWnd = CreateWindowExW(0, L"HistoryClass", L"История участия",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 380, mainWnd, NULL, GetModuleHandle(NULL), NULL);
    CenterWindow(hHistoryWnd);
}

static HWND hAboutWnd = nullptr;
static LRESULT CALLBACK AboutWndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_CREATE: {
        CreateWindowW(L"STATIC", L"О ПЛАТФОРМЕ", WS_VISIBLE | WS_CHILD | SS_CENTER, 10, 10, 460, 30, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"STATIC", L"ДАЙМОНИЙ", WS_VISIBLE | WS_CHILD | SS_CENTER, 10, 50, 460, 30, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"STATIC", L"Платформа для организации экологических мероприятий\nи объединения активистов по всему миру.\n\nВерсия 1.0\n2025",
            WS_VISIBLE | WS_CHILD | SS_CENTER, 10, 90, 460, 100, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"STATIC", L"КОНТАКТНЫЕ ДАННЫЕ КОМАНДЫ ПОДДЕРЖКИ:", WS_VISIBLE | WS_CHILD, 10, 210, 460, 25, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"STATIC", L"Email: support@daimoniy.org\nTelegram: @daimoniy_support",
            WS_VISIBLE | WS_CHILD, 10, 240, 460, 40, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"BUTTON", L"ВЫЙТИ", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 180, 300, 120, 35, hwnd, (HMENU)IDOK, NULL, NULL);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(w) == IDOK) { DestroyWindow(hwnd); hAboutWnd = nullptr; }
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        hAboutWnd = nullptr;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w, l);
}

static void ShowAboutWindow() {
    if (hAboutWnd) { SetForegroundWindow(hAboutWnd); return; }
    hAboutWnd = CreateWindowExW(0, L"AboutClass", L"О платформе",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 380, mainWnd, NULL, GetModuleHandle(NULL), NULL);
    CenterWindow(hAboutWnd);
}

static HWND hConfirmExitWnd = nullptr;
static LRESULT CALLBACK ConfirmExitWndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_CREATE: {
        CreateWindowW(L"STATIC", L"Выйти?", WS_VISIBLE | WS_CHILD | SS_CENTER, 10, 20, 260, 30, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"BUTTON", L"ДА", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 50, 70, 80, 35, hwnd, (HMENU)IDYES, NULL, NULL);
        CreateWindowW(L"BUTTON", L"НЕТ", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 150, 70, 80, 35, hwnd, (HMENU)IDNO, NULL, NULL);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(w) == IDYES) { isAppRunning = false; PostQuitMessage(0); }
        if (LOWORD(w) == IDNO) { DestroyWindow(hwnd); hConfirmExitWnd = nullptr; }
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        hConfirmExitWnd = nullptr;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w, l);
}

static void ShowConfirmExitWindow() {
    if (hConfirmExitWnd) { SetForegroundWindow(hConfirmExitWnd); return; }
    hConfirmExitWnd = CreateWindowExW(0, L"ConfirmExitClass", L"Подтверждение",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 150, mainWnd, NULL, GetModuleHandle(NULL), NULL);
    CenterWindow(hConfirmExitWnd);
}

static HWND hEventsWnd = nullptr;
static LRESULT CALLBACK EventsWndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_CREATE: {
        CreateWindowW(L"STATIC", L"МЕРОПРИЯТИЯ", WS_VISIBLE | WS_CHILD | SS_CENTER, 10, 10, 460, 30, hwnd, NULL, NULL, NULL);
        HWND list = CreateWindowW(L"LISTBOX", NULL, WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL,
            10, 50, 460, 200, hwnd, (HMENU)3001, NULL, NULL);
        for (auto& e : events) {
            std::string line = std::to_string(e.id) + ". " + e.title + " | " + e.date + " | " + e.location + " | " + e.status;
            SendMessageA(list, LB_ADDSTRING, 0, (LPARAM)line.c_str());
        }
        CreateWindowW(L"BUTTON", L"ВЫЙТИ", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 180, 270, 120, 35, hwnd, (HMENU)IDOK, NULL, NULL);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(w) == IDOK) { DestroyWindow(hwnd); hEventsWnd = nullptr; }
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        hEventsWnd = nullptr;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w, l);
}

static void ShowEventsWindow() {
    if (hEventsWnd) { SetForegroundWindow(hEventsWnd); return; }
    hEventsWnd = CreateWindowExW(0, L"EventsClass", L"Мероприятия",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 350, mainWnd, NULL, GetModuleHandle(NULL), NULL);
    CenterWindow(hEventsWnd);
}

static HWND hProfileWnd = nullptr;
static LRESULT CALLBACK ProfileWndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    static User* u = nullptr;
    switch (msg) {
    case WM_CREATE: {
        for (auto& user : users) if (user.id == currentUserId) { u = &user; break; }
        CreateWindowW(L"STATIC", L"ПРОФИЛЬ", WS_VISIBLE | WS_CHILD | SS_CENTER, 10, 10, 460, 30, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"STATIC", L"Имя:", WS_VISIBLE | WS_CHILD, 20, 60, 80, 25, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"EDIT", A2W(u ? u->name : "").c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER, 110, 60, 300, 25, hwnd, (HMENU)4001, NULL, NULL);
        CreateWindowW(L"STATIC", L"Email:", WS_VISIBLE | WS_CHILD, 20, 100, 80, 25, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"EDIT", A2W(u ? u->email : "").c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER, 110, 100, 300, 25, hwnd, (HMENU)4002, NULL, NULL);
        CreateWindowW(L"STATIC", L"Страна:", WS_VISIBLE | WS_CHILD, 20, 140, 80, 25, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"EDIT", A2W(u ? u->country : "").c_str(), WS_VISIBLE | WS_CHILD | WS_BORDER, 110, 140, 300, 25, hwnd, (HMENU)4003, NULL, NULL);
        CreateWindowW(L"BUTTON", L"СОХРАНИТЬ", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 80, 200, 120, 35, hwnd, (HMENU)IDOK, NULL, NULL);
        CreateWindowW(L"BUTTON", L"ОТМЕНА", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 240, 200, 120, 35, hwnd, (HMENU)IDCANCEL, NULL, NULL);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(w) == IDOK && u) {
            char name[256] = { 0 }, email[256] = { 0 }, country[256] = { 0 };
            GetDlgItemTextA(hwnd, 4001, name, 255);
            GetDlgItemTextA(hwnd, 4002, email, 255);
            GetDlgItemTextA(hwnd, 4003, country, 255);
            u->name = name; u->email = email; u->country = country;
            MessageBoxA(hwnd, "Сохранено", "Профиль", MB_OK);
            DestroyWindow(hwnd); hProfileWnd = nullptr;
        }
        if (LOWORD(w) == IDCANCEL) { DestroyWindow(hwnd); hProfileWnd = nullptr; }
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        hProfileWnd = nullptr;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w, l);
}

static void ShowProfileWindow() {
    if (hProfileWnd) { SetForegroundWindow(hProfileWnd); return; }
    hProfileWnd = CreateWindowExW(0, L"ProfileClass", L"Профиль",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 450, 300, mainWnd, NULL, GetModuleHandle(NULL), NULL);
    CenterWindow(hProfileWnd);
}

static HWND hRegisterWnd = nullptr;
static LRESULT CALLBACK RegisterWndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_CREATE: {
        CreateWindowW(L"STATIC", L"РЕГИСТРАЦИЯ", WS_VISIBLE | WS_CHILD | SS_CENTER, 10, 10, 460, 30, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"STATIC", L"ФИО:", WS_VISIBLE | WS_CHILD, 20, 60, 100, 25, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 130, 60, 320, 25, hwnd, (HMENU)5101, NULL, NULL);
        CreateWindowW(L"STATIC", L"Почта:", WS_VISIBLE | WS_CHILD, 20, 100, 100, 25, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 130, 100, 320, 25, hwnd, (HMENU)5102, NULL, NULL);
        CreateWindowW(L"STATIC", L"Страна:", WS_VISIBLE | WS_CHILD, 20, 140, 100, 25, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 130, 140, 320, 25, hwnd, (HMENU)5103, NULL, NULL);
        CreateWindowW(L"STATIC", L"Логин:", WS_VISIBLE | WS_CHILD, 20, 180, 100, 25, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 130, 180, 320, 25, hwnd, (HMENU)5104, NULL, NULL);
        CreateWindowW(L"STATIC", L"Пароль:", WS_VISIBLE | WS_CHILD, 20, 220, 100, 25, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 130, 220, 320, 25, hwnd, (HMENU)5105, NULL, NULL);

        CreateWindowW(L"BUTTON", L"Зарегистрироваться", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 140, 260, 200, 35, hwnd, (HMENU)5106, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Войти через Google", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 140, 310, 200, 35, hwnd, (HMENU)5107, NULL, NULL);

        CreateWindowW(L"BUTTON", L"Пользовательское соглашение", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 20, 360, 200, 30, hwnd, (HMENU)5108, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Принять", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 240, 360, 100, 30, hwnd, (HMENU)5109, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Отклонить", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 350, 360, 100, 30, hwnd, (HMENU)5110, NULL, NULL);

        CreateWindowW(L"BUTTON", L"ВЫЙТИ", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 380, 410, 80, 30, hwnd, (HMENU)IDOK, NULL, NULL);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(w) == IDOK) { DestroyWindow(hwnd); hRegisterWnd = nullptr; }
        if (LOWORD(w) == 5106) {
            MessageBoxA(hwnd, "Регистрация успешно завершена!", "Успех", MB_OK);
            DestroyWindow(hwnd);
            hRegisterWnd = nullptr;
        }
        if (LOWORD(w) == 5107) MessageBoxA(hwnd, "Вход через Google", "Инфо", MB_OK);
        if (LOWORD(w) == 5108) MessageBoxA(hwnd, "Пользовательское соглашение...", "Соглашение", MB_OK);
        if (LOWORD(w) == 5109) MessageBoxA(hwnd, "Вы приняли соглашение", "Соглашение", MB_OK);
        if (LOWORD(w) == 5110) MessageBoxA(hwnd, "Вы отклонили соглашение", "Соглашение", MB_OK);
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        hRegisterWnd = nullptr;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w, l);
}

static void ShowRegisterWindow() {
    if (hRegisterWnd) { SetForegroundWindow(hRegisterWnd); return; }
    hRegisterWnd = CreateWindowExW(0, L"RegisterClass", L"Регистрация",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 490, mainWnd, NULL, GetModuleHandle(NULL), NULL);
    CenterWindow(hRegisterWnd);
}

static HWND hSuccessRegWnd = nullptr;
static LRESULT CALLBACK SuccessRegWndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_CREATE: {
        CreateWindowW(L"STATIC", L"Успешная регистрация", WS_VISIBLE | WS_CHILD | SS_CENTER, 10, 20, 260, 30, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"STATIC", L"Вы успешно зарегистрировались!", WS_VISIBLE | WS_CHILD | SS_CENTER, 10, 60, 260, 30, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"BUTTON", L"ОК", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 100, 110, 80, 35, hwnd, (HMENU)IDOK, NULL, NULL);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(w) == IDOK) { DestroyWindow(hwnd); hSuccessRegWnd = nullptr; }
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        hSuccessRegWnd = nullptr;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w, l);
}

static void ShowSuccessRegWindow() {
    if (hSuccessRegWnd) { SetForegroundWindow(hSuccessRegWnd); return; }
    hSuccessRegWnd = CreateWindowExW(0, L"SuccessRegClass", L"Успех",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 200, mainWnd, NULL, GetModuleHandle(NULL), NULL);
    CenterWindow(hSuccessRegWnd);
}

static HWND hLoginRegisteredWnd = nullptr;
static LRESULT CALLBACK LoginRegisteredWndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_CREATE: {
        CreateWindowW(L"STATIC", L"Вход на платформу", WS_VISIBLE | WS_CHILD | SS_CENTER, 10, 10, 360, 30, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"STATIC", L"Блок иллюстрации", WS_VISIBLE | WS_CHILD | SS_CENTER, 10, 50, 360, 80, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"STATIC", L"Аккаунт:", WS_VISIBLE | WS_CHILD, 20, 150, 80, 25, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 110, 150, 250, 25, hwnd, (HMENU)7101, NULL, NULL);
        CreateWindowW(L"STATIC", L"Логин:", WS_VISIBLE | WS_CHILD, 20, 190, 80, 25, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 110, 190, 250, 25, hwnd, (HMENU)7102, NULL, NULL);
        CreateWindowW(L"STATIC", L"Пароль:", WS_VISIBLE | WS_CHILD, 20, 230, 80, 25, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 110, 230, 250, 25, hwnd, (HMENU)7103, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Вход", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 140, 280, 100, 35, hwnd, (HMENU)7104, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Выйти", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 300, 280, 80, 35, hwnd, (HMENU)IDOK, NULL, NULL);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(w) == IDOK) { DestroyWindow(hwnd); hLoginRegisteredWnd = nullptr; }
        if (LOWORD(w) == 7104) {
            isLoggedIn = true;
            currentUserId = 1;
            DestroyWindow(hwnd);
            hLoginRegisteredWnd = nullptr;
        }
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        hLoginRegisteredWnd = nullptr;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w, l);
}

static void ShowLoginRegisteredWindow() {
    if (hLoginRegisteredWnd) { SetForegroundWindow(hLoginRegisteredWnd); return; }
    hLoginRegisteredWnd = CreateWindowExW(0, L"LoginRegisteredClass", L"Вход",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 370, mainWnd, NULL, GetModuleHandle(NULL), NULL);
    CenterWindow(hLoginRegisteredWnd);
}

static HWND hLoginUnregisteredWnd = nullptr;
static LRESULT CALLBACK LoginUnregisteredWndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_CREATE: {
        CreateWindowW(L"STATIC", L"Вход на платформу", WS_VISIBLE | WS_CHILD | SS_CENTER, 10, 10, 360, 30, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"STATIC", L"Блок иллюстрации", WS_VISIBLE | WS_CHILD | SS_CENTER, 10, 50, 360, 80, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"STATIC", L"Сообщение об ограниченном функционале\nдля незарегистрированных пользователей",
            WS_VISIBLE | WS_CHILD | SS_CENTER, 10, 150, 360, 50, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Войти без регистрации", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 100, 220, 180, 35, hwnd, (HMENU)7201, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Выйти", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 300, 280, 80, 35, hwnd, (HMENU)IDOK, NULL, NULL);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(w) == IDOK) { DestroyWindow(hwnd); hLoginUnregisteredWnd = nullptr; }
        if (LOWORD(w) == 7201) {
            DestroyWindow(hwnd);
            hLoginUnregisteredWnd = nullptr;
        }
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        hLoginUnregisteredWnd = nullptr;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w, l);
}

static void ShowLoginUnregisteredWindow() {
    if (hLoginUnregisteredWnd) { SetForegroundWindow(hLoginUnregisteredWnd); return; }
    hLoginUnregisteredWnd = CreateWindowExW(0, L"LoginUnregisteredClass", L"Вход (гость)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 360, mainWnd, NULL, GetModuleHandle(NULL), NULL);
    CenterWindow(hLoginUnregisteredWnd);
}

static HWND hGuidesWnd = nullptr;
static LRESULT CALLBACK GuidesWndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_CREATE: {
        CreateWindowW(L"STATIC", L"РУКОВОДСТВО", WS_VISIBLE | WS_CHILD | SS_CENTER, 10, 10, 460, 30, hwnd, NULL, NULL, NULL);

        CreateWindowW(L"BUTTON", L"Сокращенное руководство", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 100, 60, 280, 40, hwnd, (HMENU)8101, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Полное руководство", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 100, 120, 280, 40, hwnd, (HMENU)8102, NULL, NULL);

        CreateWindowW(L"STATIC", L"фильтры поиска:", WS_VISIBLE | WS_CHILD, 10, 180, 120, 25, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 130, 180, 200, 25, hwnd, (HMENU)8103, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Поиск", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 340, 180, 80, 25, hwnd, (HMENU)8104, NULL, NULL);

        CreateWindowW(L"BUTTON", L"Выйти", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 180, 230, 120, 35, hwnd, (HMENU)IDOK, NULL, NULL);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(w) == IDOK) { DestroyWindow(hwnd); hGuidesWnd = nullptr; }
        if (LOWORD(w) == 8101) MessageBoxA(hwnd, "Краткое руководство пользователя", "Руководство", MB_OK);
        if (LOWORD(w) == 8102) MessageBoxA(hwnd, "Полное руководство пользователя", "Руководство", MB_OK);
        if (LOWORD(w) == 8104) MessageBoxA(hwnd, "Поиск по руководству", "Поиск", MB_OK);
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        hGuidesWnd = nullptr;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w, l);
}

static void ShowGuidesWindow() {
    if (hGuidesWnd) { SetForegroundWindow(hGuidesWnd); return; }
    hGuidesWnd = CreateWindowExW(0, L"GuidesClass", L"Руководство",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 320, mainWnd, NULL, GetModuleHandle(NULL), NULL);
    CenterWindow(hGuidesWnd);
}

static HWND hLoginRegisterWnd = nullptr;
static LRESULT CALLBACK LoginRegisterWndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_CREATE: {
        CreateWindowW(L"BUTTON", L"РЕГИСТРАЦИЯ", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 100, 30, 200, 50, hwnd, (HMENU)100, NULL, NULL);
        CreateWindowW(L"BUTTON", L"ВХОД", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 100, 100, 200, 50, hwnd, (HMENU)200, NULL, NULL);
        CreateWindowW(L"BUTTON", L"РУКОВОДСТВО ПОЛЬЗОВАТЕЛЯ", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 20, 180, 180, 40, hwnd, (HMENU)300, NULL, NULL);
        CreateWindowW(L"BUTTON", L"ВЫЙТИ", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 220, 180, 100, 40, hwnd, (HMENU)400, NULL, NULL);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(w) == 100) { ShowRegisterWindow(); }
        if (LOWORD(w) == 200) { ShowLoginRegisteredWindow(); }
        if (LOWORD(w) == 300) { ShowGuidesWindow(); }
        if (LOWORD(w) == 400) { DestroyWindow(hwnd); hLoginRegisterWnd = nullptr; }
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        hLoginRegisterWnd = nullptr;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w, l);
}

static void ShowLoginRegisterWindow() {
    if (hLoginRegisterWnd) { SetForegroundWindow(hLoginRegisterWnd); return; }
    hLoginRegisterWnd = CreateWindowExW(0, L"LoginRegisterClass", L"Вход / Регистрация",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 420, 280, mainWnd, NULL, GetModuleHandle(NULL), NULL);
    CenterWindow(hLoginRegisterWnd);
}

static void CreateButtons(HWND hwnd) {
    int centerX = 400;

    hLoginBtn = CreateWindowW(L"BUTTON", L"ВОЙТИ В АККАУНТ", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        centerX - 110, 220, 220, 45, hwnd, (HMENU)700, NULL, NULL);

    hAchievementsBtn = CreateWindowW(L"BUTTON", L"ДОСТИЖЕНИЯ", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        centerX - 110, 290, 220, 40, hwnd, (HMENU)100, NULL, NULL);
    hHistoryBtn = CreateWindowW(L"BUTTON", L"ИСТОРИЯ", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        centerX - 110, 340, 220, 40, hwnd, (HMENU)200, NULL, NULL);
    hAboutBtn = CreateWindowW(L"BUTTON", L"О ПЛАТФОРМЕ", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        centerX - 110, 390, 220, 40, hwnd, (HMENU)500, NULL, NULL);
    hGuideBtn = CreateWindowW(L"BUTTON", L"РУКОВОДСТВО", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        centerX - 110, 440, 220, 40, hwnd, (HMENU)800, NULL, NULL);

    hEventsBtn = CreateWindowW(L"BUTTON", L"МЕРОПРИЯТИЯ", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        centerX - 110, 500, 220, 40, hwnd, (HMENU)300, NULL, NULL);
    hProfileBtn = CreateWindowW(L"BUTTON", L"ПРОФИЛЬ", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        centerX - 110, 550, 220, 40, hwnd, (HMENU)400, NULL, NULL);

    hExitBtn = CreateWindowW(L"BUTTON", L"ВЫЙТИ", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        centerX - 55, 610, 110, 35, hwnd, (HMENU)600, NULL, NULL);
}

static LRESULT CALLBACK MainProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    if (msg == WM_CREATE) {
        mainWnd = hwnd;
        CreateButtons(hwnd);
        return 0;
    }
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        HFONT big = CreateFont(68, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Segoe UI");
        HFONT old = (HFONT)SelectObject(hdc, big);
        SetTextColor(hdc, RGB(34, 139, 34));
        SetBkMode(hdc, TRANSPARENT);
        std::wstring title = L"ДАЙМОНИЙ";
        SIZE sz;
        GetTextExtentPoint32W(hdc, title.c_str(), (int)title.size(), &sz);
        int titleX = (rc.right - sz.cx) / 2;
        TextOutW(hdc, titleX, 60, title.c_str(), (int)title.size());
        SelectObject(hdc, old);
        DeleteObject(big);

        HFONT subFont = CreateFont(20, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, L"Segoe UI");
        SelectObject(hdc, subFont);
        std::wstring subtitle = L"Вместе мы можем изменить мир";
        GetTextExtentPoint32W(hdc, subtitle.c_str(), (int)subtitle.size(), &sz);
        int subX = (rc.right - sz.cx) / 2;
        TextOutW(hdc, subX, 140, subtitle.c_str(), (int)subtitle.size());
        SelectObject(hdc, old);
        DeleteObject(subFont);

        EndPaint(hwnd, &ps);
        return 0;
    }
    if (msg == WM_SIZE) {
        RECT rc;
        GetClientRect(hwnd, &rc);
        int centerX = (rc.right - rc.left) / 2;
        if (hLoginBtn) SetWindowPos(hLoginBtn, NULL, centerX - 110, 220, 220, 45, SWP_NOZORDER);
        if (hAchievementsBtn) SetWindowPos(hAchievementsBtn, NULL, centerX - 110, 290, 220, 40, SWP_NOZORDER);
        if (hHistoryBtn) SetWindowPos(hHistoryBtn, NULL, centerX - 110, 340, 220, 40, SWP_NOZORDER);
        if (hAboutBtn) SetWindowPos(hAboutBtn, NULL, centerX - 110, 390, 220, 40, SWP_NOZORDER);
        if (hGuideBtn) SetWindowPos(hGuideBtn, NULL, centerX - 110, 440, 220, 40, SWP_NOZORDER);
        if (hEventsBtn) SetWindowPos(hEventsBtn, NULL, centerX - 110, 500, 220, 40, SWP_NOZORDER);
        if (hProfileBtn) SetWindowPos(hProfileBtn, NULL, centerX - 110, 550, 220, 40, SWP_NOZORDER);
        if (hExitBtn) SetWindowPos(hExitBtn, NULL, centerX - 55, 610, 110, 35, SWP_NOZORDER);
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    }
    if (msg == WM_COMMAND) {
        if (LOWORD(w) == 100) ShowAchievementsWindow();
        if (LOWORD(w) == 200) ShowHistoryWindow();
        if (LOWORD(w) == 300) ShowEventsWindow();
        if (LOWORD(w) == 400) ShowProfileWindow();
        if (LOWORD(w) == 500) ShowAboutWindow();
        if (LOWORD(w) == 800) ShowGuidesWindow();
        if (LOWORD(w) == 600) ShowConfirmExitWindow();
        if (LOWORD(w) == 700) ShowLoginRegisterWindow();
        return 0;
    }
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w, l);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    InitData();

    WNDCLASSW wc = {};
    wc.lpfnWndProc = MainProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"DaimoniyClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    WNDCLASSW achievementsClass = { 0 };
    achievementsClass.lpfnWndProc = AchievementsWndProc;
    achievementsClass.hInstance = hInst;
    achievementsClass.lpszClassName = L"AchievementsClass";
    achievementsClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&achievementsClass);

    WNDCLASSW historyClass = { 0 };
    historyClass.lpfnWndProc = HistoryWndProc;
    historyClass.hInstance = hInst;
    historyClass.lpszClassName = L"HistoryClass";
    historyClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&historyClass);

    WNDCLASSW aboutClass = { 0 };
    aboutClass.lpfnWndProc = AboutWndProc;
    aboutClass.hInstance = hInst;
    aboutClass.lpszClassName = L"AboutClass";
    aboutClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&aboutClass);

    WNDCLASSW eventsClass = { 0 };
    eventsClass.lpfnWndProc = EventsWndProc;
    eventsClass.hInstance = hInst;
    eventsClass.lpszClassName = L"EventsClass";
    eventsClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&eventsClass);

    WNDCLASSW profileClass = { 0 };
    profileClass.lpfnWndProc = ProfileWndProc;
    profileClass.hInstance = hInst;
    profileClass.lpszClassName = L"ProfileClass";
    profileClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&profileClass);

    WNDCLASSW loginRegClass = { 0 };
    loginRegClass.lpfnWndProc = LoginRegisterWndProc;
    loginRegClass.hInstance = hInst;
    loginRegClass.lpszClassName = L"LoginRegisterClass";
    loginRegClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&loginRegClass);

    WNDCLASSW confirmExitClass = { 0 };
    confirmExitClass.lpfnWndProc = ConfirmExitWndProc;
    confirmExitClass.hInstance = hInst;
    confirmExitClass.lpszClassName = L"ConfirmExitClass";
    confirmExitClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&confirmExitClass);

    WNDCLASSW registerClass = { 0 };
    registerClass.lpfnWndProc = RegisterWndProc;
    registerClass.hInstance = hInst;
    registerClass.lpszClassName = L"RegisterClass";
    registerClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&registerClass);

    WNDCLASSW successRegClass = { 0 };
    successRegClass.lpfnWndProc = SuccessRegWndProc;
    successRegClass.hInstance = hInst;
    successRegClass.lpszClassName = L"SuccessRegClass";
    successRegClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&successRegClass);

    WNDCLASSW loginRegisteredClass = { 0 };
    loginRegisteredClass.lpfnWndProc = LoginRegisteredWndProc;
    loginRegisteredClass.hInstance = hInst;
    loginRegisteredClass.lpszClassName = L"LoginRegisteredClass";
    loginRegisteredClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&loginRegisteredClass);

    WNDCLASSW loginUnregisteredClass = { 0 };
    loginUnregisteredClass.lpfnWndProc = LoginUnregisteredWndProc;
    loginUnregisteredClass.hInstance = hInst;
    loginUnregisteredClass.lpszClassName = L"LoginUnregisteredClass";
    loginUnregisteredClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&loginUnregisteredClass);

    WNDCLASSW guidesClass = { 0 };
    guidesClass.lpfnWndProc = GuidesWndProc;
    guidesClass.hInstance = hInst;
    guidesClass.lpszClassName = L"GuidesClass";
    guidesClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassW(&guidesClass);

    HWND hwnd = CreateWindowExW(0, L"DaimoniyClass", L"Даймоний", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 850, 750, NULL, NULL, hInst, NULL);
    if (!hwnd) return 1;
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) && isAppRunning) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
//#pragma comment(linker, "/SUBSYSTEM:WINDOWS")
//#define PQXX_DISABLE_DEPRECATED_HEADERS
//
//#include <windows.h>
//#include <string>
//#include <vector>
//#include <unordered_map>
//#include <pqxx/pqxx>
//
//
//class ClientManager {
//public:
//    pqxx::connection* conn;
//
//    ClientManager(pqxx::connection& connection) : conn(&connection) {}
//
//    void createTables() {
//        pqxx::work txn(*conn);
//        txn.exec("CREATE TABLE IF NOT EXISTS clients (id SERIAL PRIMARY KEY, first_name VARCHAR(100) NOT NULL, last_name VARCHAR(100) NOT NULL, email VARCHAR(200) UNIQUE NOT NULL, created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)");
//        txn.exec("CREATE TABLE IF NOT EXISTS phones (id SERIAL PRIMARY KEY, client_id INTEGER REFERENCES clients(id) ON DELETE CASCADE, phone_number VARCHAR(20) NOT NULL, UNIQUE(client_id, phone_number))");
//        txn.commit();
//    }
//
//    int addClient(const std::string& firstName, const std::string& lastName, const std::string& email, const std::vector<std::string>& phones = {}) {
//        pqxx::work txn(*conn);
//        std::string query = "INSERT INTO clients (first_name, last_name, email) VALUES (" + txn.quote(firstName) + ", " + txn.quote(lastName) + ", " + txn.quote(email) + ") RETURNING id";
//        pqxx::result res = txn.exec(query);
//        int clientId = res[0][0].as<int>();
//        for (const auto& phone : phones) {
//            if (!phone.empty()) {
//                txn.exec("INSERT INTO phones (client_id, phone_number) VALUES (" + txn.quote(clientId) + ", " + txn.quote(phone) + ")");
//            }
//        }
//        txn.commit();
//        return clientId;
//    }
//
//    void updateClient(int clientId, const std::string& firstName, const std::string& lastName, const std::string& email) {
//        pqxx::work txn(*conn);
//        txn.exec("UPDATE clients SET first_name = " + txn.quote(firstName) + ", last_name = " + txn.quote(lastName) + ", email = " + txn.quote(email) + " WHERE id = " + txn.quote(clientId));
//        txn.commit();
//    }
//
//    void removeClient(int clientId) {
//        {
//            pqxx::work txn(*conn);
//            txn.exec("DELETE FROM clients WHERE id = " + txn.quote(clientId));
//            txn.commit();
//        }
//        renumberIds();
//    }
//
//    void renumberIds() {
//        pqxx::work txn(*conn);
//        pqxx::result res = txn.exec("SELECT id FROM clients ORDER BY id");
//        int newId = 1;
//        for (const auto& row : res) {
//            int oldId = row[0].as<int>();
//            if (oldId != newId) {
//                txn.exec("UPDATE clients SET id = " + txn.quote(newId) + " WHERE id = " + txn.quote(oldId));
//                txn.exec("UPDATE phones SET client_id = " + txn.quote(newId) + " WHERE client_id = " + txn.quote(oldId));
//            }
//            newId++;
//        }
//        newId--;
//        if (newId > 0) {
//            txn.exec("SELECT setval('clients_id_seq', " + txn.quote(newId) + ")");
//        }
//        txn.commit();
//    }
//
//    std::vector<std::string> getClientPhones(int clientId) {
//        pqxx::work txn(*conn);
//        pqxx::result res = txn.exec("SELECT phone_number FROM phones WHERE client_id = " + txn.quote(clientId));
//        std::vector<std::string> phones;
//        for (const auto& row : res) {
//            phones.push_back(row[0].as<std::string>());
//        }
//        return phones;
//    }
//
//    bool clientExists(int clientId) {
//        pqxx::work txn(*conn);
//        pqxx::result res = txn.exec("SELECT COUNT(*) FROM clients WHERE id = " + txn.quote(clientId));
//        return res[0][0].as<int>() > 0;
//    }
//
//    std::string getFullClientInfo(int clientId) {
//        std::string result;
//
//        {
//            pqxx::work txn(*conn);
//            pqxx::result res = txn.exec("SELECT first_name, last_name, email FROM clients WHERE id = " + txn.quote(clientId));
//            if (res.empty()) return "";
//            result = "=== КЛИЕНТ ID: " + std::to_string(clientId) + " ===\n";
//            result += "Имя: " + res[0][0].as<std::string>() + "\n";
//            result += "Фамилия: " + res[0][1].as<std::string>() + "\n";
//            result += "Email: " + res[0][2].as<std::string>() + "\n";
//        }
//
//        std::vector<std::string> phones = getClientPhones(clientId);
//        result += "Телефоны: ";
//        if (phones.empty()) result += "нет";
//        else for (const auto& p : phones) result += p + " ";
//        result += "\n";
//
//        return result;
//    }
//};
//
//
//#define IDC_NAME_INPUT    101
//#define IDC_SURNAME_INPUT 102
//#define IDC_EMAIL_INPUT   103
//#define IDC_PHONES_INPUT  104
//#define IDC_ADD_BUTTON    105
//#define IDC_CLEAR_BUTTON  106
//#define IDC_UPDATE_BUTTON 107
//#define IDC_DELETE_BUTTON 108
//#define IDC_SEARCH_BUTTON 109
//#define IDC_CLIENT_LIST   111
//#define IDC_STATUS_BAR    112
//#define IDC_VIEW_ALL_BTN  113
//#define IDC_SEARCH_ID_BTN 114
//
//struct ControlInfo {
//    HWND hwnd;
//    double leftRatio;
//    double topRatio;
//    double widthRatio;
//    double heightRatio;
//};
//
//std::unordered_map<int, ControlInfo> controls;
//ClientManager* g_manager = nullptr;
//
//std::string WStringToString(const std::wstring& wstr) {
//    if (wstr.empty()) return std::string();
//    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
//    std::string strTo(size_needed, 0);
//    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
//    return strTo;
//}
//
//std::wstring StringToWString(const std::string& str) {
//    if (str.empty()) return std::wstring();
//    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
//    std::wstring wstrTo(size_needed, 0);
//    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
//    return wstrTo;
//}
//
//void UpdateStatus(HWND hwnd, const std::string& message) {
//    SetWindowTextW(GetDlgItem(hwnd, IDC_STATUS_BAR), StringToWString(message).c_str());
//}
//
//void RefreshClientList(HWND hwnd) {
//    HWND hList = GetDlgItem(hwnd, IDC_CLIENT_LIST);
//    SendMessageW(hList, LB_RESETCONTENT, 0, 0);
//    try {
//        pqxx::work txn(*g_manager->conn);
//        pqxx::result res = txn.exec("SELECT id, first_name, last_name, email FROM clients ORDER BY id");
//        for (const auto& row : res) {
//            int id = row[0].as<int>();
//            std::string display = std::to_string(id) + ": " + row[1].as<std::string>() + " " + row[2].as<std::string>() + " (" + row[3].as<std::string>() + ")";
//            SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)StringToWString(display).c_str());
//        }
//        UpdateStatus(hwnd, "List is updated, total: " + std::to_string(res.size()));
//    }
//    catch (const std::exception& e) {
//        UpdateStatus(hwnd, "Ошибка: " + std::string(e.what()));
//    }
//}
//
//void LoadClientToForm(HWND hwnd) {
//    HWND hList = GetDlgItem(hwnd, IDC_CLIENT_LIST);
//    int selectedIndex = (int)SendMessageW(hList, LB_GETCURSEL, 0, 0);
//    if (selectedIndex == LB_ERR) return;
//    wchar_t buffer[512] = { 0 };
//    SendMessageW(hList, LB_GETTEXT, selectedIndex, (LPARAM)buffer);
//    int clientId = std::stoi(std::wstring(buffer).substr(0, std::wstring(buffer).find(L':')));
//    try {
//        pqxx::work txn(*g_manager->conn);
//        pqxx::result res = txn.exec("SELECT first_name, last_name, email FROM clients WHERE id = " + txn.quote(clientId));
//        if (!res.empty()) {
//            SetWindowTextW(GetDlgItem(hwnd, IDC_NAME_INPUT), StringToWString(res[0][0].as<std::string>()).c_str());
//            SetWindowTextW(GetDlgItem(hwnd, IDC_SURNAME_INPUT), StringToWString(res[0][1].as<std::string>()).c_str());
//            SetWindowTextW(GetDlgItem(hwnd, IDC_EMAIL_INPUT), StringToWString(res[0][2].as<std::string>()).c_str());
//            txn.commit();
//
//            std::string phonesStr;
//            std::vector<std::string> phones = g_manager->getClientPhones(clientId);
//            for (const auto& p : phones) {
//                if (!phonesStr.empty()) phonesStr += ", ";
//                phonesStr += p;
//            }
//            SetWindowTextW(GetDlgItem(hwnd, IDC_PHONES_INPUT), StringToWString(phonesStr).c_str());
//            UpdateStatus(hwnd, "Загружен клиент ID " + std::to_string(clientId));
//        }
//    }
//    catch (const std::exception& e) {
//        UpdateStatus(hwnd, "Ошибка: " + std::string(e.what()));
//    }
//}
//
//void ResizeControls(HWND hwnd, int clientWidth, int clientHeight) {
//    for (auto& pair : controls) {
//        ControlInfo& info = pair.second;
//        SetWindowPos(info.hwnd, NULL,
//            (int)(info.leftRatio * clientWidth),
//            (int)(info.topRatio * clientHeight),
//            (int)(info.widthRatio * clientWidth),
//            (int)(info.heightRatio * clientHeight),
//            SWP_NOZORDER);
//    }
//}
//
//void ShowInfoDialog(HWND hwndParent, const std::string& info) {
//    MessageBoxA(hwndParent, info.c_str(), "Информация о клиенте", MB_OK | MB_ICONINFORMATION);
//}
//
//INT_PTR CALLBACK SearchIdDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
//    static int* resultId = nullptr;
//
//    switch (uMsg) {
//    case WM_INITDIALOG:
//        resultId = reinterpret_cast<int*>(lParam);
//        SetDlgItemTextA(hDlg, 1000, "");
//        return TRUE;
//    case WM_COMMAND:
//        if (LOWORD(wParam) == IDOK) {
//            char buffer[32] = { 0 };
//            GetDlgItemTextA(hDlg, 1000, buffer, 32);
//            if (resultId) *resultId = atoi(buffer);
//            EndDialog(hDlg, IDOK);
//            return TRUE;
//        }
//        if (LOWORD(wParam) == IDCANCEL) {
//            if (resultId) *resultId = -1;
//            EndDialog(hDlg, IDCANCEL);
//            return TRUE;
//        }
//        break;
//    }
//    return FALSE;
//}
//
//LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
//    switch (uMsg) {
//    case WM_CREATE: {
//        int bw = 800, bh = 620;
//
//        controls[IDC_CLIENT_LIST] = {
//            CreateWindowW(L"LISTBOX", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
//                450, 10, 330, 400, hwnd, (HMENU)IDC_CLIENT_LIST, GetModuleHandleW(NULL), NULL),
//            450.0 / bw, 10.0 / bh, 330.0 / bw, 400.0 / bh
//        };
//
//        CreateWindowW(L"BUTTON", L"Обновить", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
//            450, 420, 100, 30, hwnd, (HMENU)IDC_SEARCH_BUTTON, GetModuleHandleW(NULL), NULL);
//        CreateWindowW(L"BUTTON", L"Все клиенты", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
//            560, 420, 110, 30, hwnd, (HMENU)IDC_VIEW_ALL_BTN, GetModuleHandleW(NULL), NULL);
//
//        CreateWindowW(L"STATIC", L"Имя:", WS_VISIBLE | WS_CHILD, 10, 10, 80, 25, hwnd, (HMENU)NULL, GetModuleHandleW(NULL), NULL);
//        CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 100, 10, 320, 25, hwnd, (HMENU)IDC_NAME_INPUT, GetModuleHandleW(NULL), NULL);
//        CreateWindowW(L"STATIC", L"Фамилия:", WS_VISIBLE | WS_CHILD, 10, 45, 80, 25, hwnd, (HMENU)NULL, GetModuleHandleW(NULL), NULL);
//        CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 100, 45, 320, 25, hwnd, (HMENU)IDC_SURNAME_INPUT, GetModuleHandleW(NULL), NULL);
//        CreateWindowW(L"STATIC", L"Email:", WS_VISIBLE | WS_CHILD, 10, 80, 80, 25, hwnd, (HMENU)NULL, GetModuleHandleW(NULL), NULL);
//        CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 100, 80, 320, 25, hwnd, (HMENU)IDC_EMAIL_INPUT, GetModuleHandleW(NULL), NULL);
//        CreateWindowW(L"STATIC", L"Телефоны:", WS_VISIBLE | WS_CHILD, 10, 115, 80, 25, hwnd, (HMENU)NULL, GetModuleHandleW(NULL), NULL);
//        CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE, 100, 115, 320, 50, hwnd, (HMENU)IDC_PHONES_INPUT, GetModuleHandleW(NULL), NULL);
//
//        CreateWindowW(L"BUTTON", L"ДОБАВИТЬ", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
//            10, 180, 120, 40, hwnd, (HMENU)IDC_ADD_BUTTON, GetModuleHandleW(NULL), NULL);
//        CreateWindowW(L"BUTTON", L"ИЗМЕНИТЬ", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
//            140, 180, 120, 40, hwnd, (HMENU)IDC_UPDATE_BUTTON, GetModuleHandleW(NULL), NULL);
//        CreateWindowW(L"BUTTON", L"УДАЛИТЬ", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
//            270, 180, 120, 40, hwnd, (HMENU)IDC_DELETE_BUTTON, GetModuleHandleW(NULL), NULL);
//        CreateWindowW(L"BUTTON", L"ОЧИСТИТЬ", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
//            10, 235, 380, 35, hwnd, (HMENU)IDC_CLEAR_BUTTON, GetModuleHandleW(NULL), NULL);
//
//        controls[IDC_STATUS_BAR] = {
//            CreateWindowW(L"STATIC", L"Готов", WS_VISIBLE | WS_CHILD | WS_BORDER | SS_LEFT,
//                10, 580, 770, 25, hwnd, (HMENU)IDC_STATUS_BAR, GetModuleHandleW(NULL), NULL),
//            10.0 / bw, 580.0 / bh, 770.0 / bw, 25.0 / bh
//        };
//
//        RefreshClientList(hwnd);
//        break;
//    }
//    case WM_SIZE: {
//        ResizeControls(hwnd, LOWORD(lParam), HIWORD(lParam));
//        break;
//    }
//    case WM_COMMAND: {
//        if (LOWORD(wParam) == IDC_ADD_BUTTON) {
//            wchar_t name[256] = { 0 }, surname[256] = { 0 }, email[256] = { 0 }, phones[1000] = { 0 };
//            GetWindowTextW(GetDlgItem(hwnd, IDC_NAME_INPUT), name, 256);
//            GetWindowTextW(GetDlgItem(hwnd, IDC_SURNAME_INPUT), surname, 256);
//            GetWindowTextW(GetDlgItem(hwnd, IDC_EMAIL_INPUT), email, 256);
//            GetWindowTextW(GetDlgItem(hwnd, IDC_PHONES_INPUT), phones, 1000);
//            if (wcslen(name) == 0 || wcslen(email) == 0) {
//                MessageBoxW(hwnd, L"Имя и Email обязательны!", L"Ошибка", MB_OK);
//                break;
//            }
//            try {
//                std::vector<std::string> phoneList;
//                std::wstring pw(phones);
//                size_t s = 0, e = 0;
//                while ((e = pw.find(L',', s)) != std::wstring::npos) {
//                    if (e > s) phoneList.push_back(WStringToString(pw.substr(s, e - s)));
//                    s = e + 1;
//                }
//                if (s < pw.length()) phoneList.push_back(WStringToString(pw.substr(s)));
//                g_manager->addClient(WStringToString(name), WStringToString(surname), WStringToString(email), phoneList);
//                UpdateStatus(hwnd, "Клиент добавлен");
//                RefreshClientList(hwnd);
//                SetWindowTextW(GetDlgItem(hwnd, IDC_NAME_INPUT), L"");
//                SetWindowTextW(GetDlgItem(hwnd, IDC_SURNAME_INPUT), L"");
//                SetWindowTextW(GetDlgItem(hwnd, IDC_EMAIL_INPUT), L"");
//                SetWindowTextW(GetDlgItem(hwnd, IDC_PHONES_INPUT), L"");
//            }
//            catch (const std::exception& e) {
//                UpdateStatus(hwnd, "Ошибка: " + std::string(e.what()));
//            }
//            break;
//        }
//        if (LOWORD(wParam) == IDC_UPDATE_BUTTON) {
//            HWND hList = GetDlgItem(hwnd, IDC_CLIENT_LIST);
//            int idx = (int)SendMessageW(hList, LB_GETCURSEL, 0, 0);
//            if (idx == LB_ERR) {
//                MessageBoxW(hwnd, L"Выберите клиента", L"Ошибка", MB_OK);
//                break;
//            }
//            wchar_t buf[512] = { 0 };
//            SendMessageW(hList, LB_GETTEXT, idx, (LPARAM)buf);
//            int cid = std::stoi(std::wstring(buf).substr(0, std::wstring(buf).find(L':')));
//            wchar_t name[256] = { 0 }, surname[256] = { 0 }, email[256] = { 0 };
//            GetWindowTextW(GetDlgItem(hwnd, IDC_NAME_INPUT), name, 256);
//            GetWindowTextW(GetDlgItem(hwnd, IDC_SURNAME_INPUT), surname, 256);
//            GetWindowTextW(GetDlgItem(hwnd, IDC_EMAIL_INPUT), email, 256);
//            g_manager->updateClient(cid, WStringToString(name), WStringToString(surname), WStringToString(email));
//            UpdateStatus(hwnd, "Клиент обновлен");
//            RefreshClientList(hwnd);
//            break;
//        }
//        if (LOWORD(wParam) == IDC_DELETE_BUTTON) {
//            HWND hList = GetDlgItem(hwnd, IDC_CLIENT_LIST);
//            int idx = (int)SendMessageW(hList, LB_GETCURSEL, 0, 0);
//            if (idx == LB_ERR) break;
//            wchar_t buf[512] = { 0 };
//            SendMessageW(hList, LB_GETTEXT, idx, (LPARAM)buf);
//            int cid = std::stoi(std::wstring(buf).substr(0, std::wstring(buf).find(L':')));
//            if (MessageBoxW(hwnd, L"Удалить?", L"Подтверждение", MB_YESNO) == IDYES) {
//                g_manager->removeClient(cid);
//                UpdateStatus(hwnd, "Клиент удален");
//                RefreshClientList(hwnd);
//            }
//            break;
//        }
//        if (LOWORD(wParam) == IDC_CLEAR_BUTTON) {
//            SetWindowTextW(GetDlgItem(hwnd, IDC_NAME_INPUT), L"");
//            SetWindowTextW(GetDlgItem(hwnd, IDC_SURNAME_INPUT), L"");
//            SetWindowTextW(GetDlgItem(hwnd, IDC_EMAIL_INPUT), L"");
//            SetWindowTextW(GetDlgItem(hwnd, IDC_PHONES_INPUT), L"");
//            UpdateStatus(hwnd, "Поля очищены");
//            break;
//        }
//        if (LOWORD(wParam) == IDC_VIEW_ALL_BTN) {
//            std::string all = "=== ВСЕ КЛИЕНТЫ ===\n\n";
//            std::vector<int> ids;
//            {
//                pqxx::work txn(*g_manager->conn);
//                pqxx::result res = txn.exec("SELECT id FROM clients ORDER BY id");
//                for (const auto& row : res) {
//                    ids.push_back(row[0].as<int>());
//                }
//            }
//            for (int id : ids) {
//                all += g_manager->getFullClientInfo(id) + "\n";
//            }
//            ShowInfoDialog(hwnd, all);
//            break;
//        }
//        if (LOWORD(wParam) == IDC_SEARCH_ID_BTN) {
//            int id = -1;
//            INT_PTR result = DialogBoxParamA(GetModuleHandle(NULL), MAKEINTRESOURCEA(NULL), hwnd, SearchIdDialogProc, (LPARAM)&id);
//            if (result == IDOK && id > 0) {
//                if (g_manager->clientExists(id)) {
//                    ShowInfoDialog(hwnd, g_manager->getFullClientInfo(id));
//                }
//                else {
//                    MessageBoxW(hwnd, L"Клиент не найден", L"Результат", MB_OK);
//                }
//            }
//            break;
//        }
//        if (LOWORD(wParam) == IDC_SEARCH_BUTTON) RefreshClientList(hwnd);
//        if (HIWORD(wParam) == LBN_SELCHANGE && LOWORD(wParam) == IDC_CLIENT_LIST) LoadClientToForm(hwnd);
//        break;
//    }
//    case WM_DESTROY:
//        PostQuitMessage(0);
//        break;
//    default:
//        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
//    }
//    return 0;
//}
//
//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
//    try {
//        pqxx::connection conn("host=localhost port=5432 user=postgres client_encoding=UTF8");
//        ClientManager manager(conn);
//        g_manager = &manager;
//        manager.createTables();
//
//        WNDCLASSW wc = { 0 };
//        wc.lpfnWndProc = WindowProc;
//        wc.hInstance = hInstance;
//        wc.lpszClassName = L"ClientManagerClass";
//        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
//        wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
//        RegisterClassW(&wc);
//
//        HWND hwnd = CreateWindowExW(0, L"ClientManagerClass", L"Управление клиентами",
//            WS_OVERLAPPEDWINDOW | WS_THICKFRAME | WS_MAXIMIZEBOX,
//            CW_USEDEFAULT, CW_USEDEFAULT, 850, 650,
//            NULL, NULL, hInstance, NULL);
//        if (!hwnd) return 0;
//        ShowWindow(hwnd, nCmdShow);
//
//        MSG msg;
//        while (GetMessageW(&msg, NULL, 0, 0)) {
//            TranslateMessage(&msg);
//            DispatchMessageW(&msg);
//        }
//        return 0;
//    }
//    catch (const std::exception& e) {
//        MessageBoxA(NULL, e.what(), "Ошибка", MB_OK);
//        return 1;
//    }
//}