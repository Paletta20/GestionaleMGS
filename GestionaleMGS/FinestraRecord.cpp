#include "FinestraRecord.hpp"

#include <algorithm>

namespace
{
    constexpr int ID_SALVA = 1;
    constexpr int ID_ANNULLA = 2;
    constexpr int ID_CAMPO_BASE = 2000;

    void applicaFont(HWND controllo, HFONT font)
    {
        SendMessageW(controllo, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }
}

FinestraRecord::FinestraRecord(HWND finestraGenitore)
    : genitore(finestraGenitore), finestra(nullptr), font(nullptr), campi(nullptr), salvato(false)
{
}

FinestraRecord::~FinestraRecord()
{
    if (font != nullptr) DeleteObject(font);
}

bool FinestraRecord::mostra(const std::wstring& titolo, std::vector<CampoRecord>& campiRecord)
{
    static bool classeRegistrata = false;
    if (!classeRegistrata)
    {
        WNDCLASSEXW classe{ sizeof(WNDCLASSEXW) };
        classe.lpfnWndProc = procedura;
        classe.hInstance = GetModuleHandleW(nullptr);
        classe.hCursor = LoadCursor(nullptr, IDC_ARROW);
        classe.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        classe.lpszClassName = L"GestionaleMGSRecordWindow";
        classe.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        classeRegistrata = RegisterClassExW(&classe) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
    }

    campi = &campiRecord;
    controlli.clear();
    salvato = false;
    const int altezzaCalcolata = 145 + static_cast<int>(campiRecord.size()) * 54;
    const int altezza = altezzaCalcolata < 760 ? altezzaCalcolata : 760;

    RECT areaGenitore{};
    GetWindowRect(genitore, &areaGenitore);
    const int x = areaGenitore.left + ((areaGenitore.right - areaGenitore.left) - 600) / 2;
    const int y = areaGenitore.top + ((areaGenitore.bottom - areaGenitore.top) - altezza) / 2;

    finestra = CreateWindowExW(WS_EX_DLGMODALFRAME, L"GestionaleMGSRecordWindow", titolo.c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU, x, y, 600, altezza,
        genitore, nullptr, GetModuleHandleW(nullptr), this);
    if (finestra == nullptr) return false;

    EnableWindow(genitore, FALSE);
    ShowWindow(finestra, SW_SHOW);
    UpdateWindow(finestra);

    MSG messaggio{};
    while (IsWindow(finestra) && GetMessageW(&messaggio, nullptr, 0, 0) > 0)
    {
        if (!IsDialogMessageW(finestra, &messaggio))
        {
            TranslateMessage(&messaggio);
            DispatchMessageW(&messaggio);
        }
    }

    EnableWindow(genitore, TRUE);
    SetForegroundWindow(genitore);
    return salvato;
}

LRESULT CALLBACK FinestraRecord::procedura(HWND hwnd, UINT messaggio, WPARAM wParam, LPARAM lParam)
{
    auto* modulo = reinterpret_cast<FinestraRecord*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (messaggio == WM_NCCREATE)
    {
        const auto* creazione = reinterpret_cast<CREATESTRUCTW*>(lParam);
        modulo = static_cast<FinestraRecord*>(creazione->lpCreateParams);
        modulo->finestra = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(modulo));
    }
    return modulo == nullptr ? DefWindowProcW(hwnd, messaggio, wParam, lParam)
        : modulo->gestisci(messaggio, wParam, lParam);
}

LRESULT FinestraRecord::gestisci(UINT messaggio, WPARAM wParam, LPARAM lParam)
{
    switch (messaggio)
    {
    case WM_CREATE:
        creaControlli();
        return 0;
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_SALVA)
        {
            raccogliValori();
            salvato = true;
            DestroyWindow(finestra);
        }
        else if (LOWORD(wParam) == ID_ANNULLA)
        {
            DestroyWindow(finestra);
        }
        return 0;
    case WM_CLOSE:
        DestroyWindow(finestra);
        return 0;
    }
    return DefWindowProcW(finestra, messaggio, wParam, lParam);
}

void FinestraRecord::creaControlli()
{
    font = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH, L"Segoe UI");

    for (int i = 0; i < static_cast<int>(campi->size()); ++i)
    {
        CampoRecord& campo = (*campi)[i];
        const int y = 22 + i * 54;
        HWND etichetta = CreateWindowExW(0, L"STATIC", campo.etichetta.c_str(), WS_CHILD | WS_VISIBLE,
            22, y + 6, 155, 26, finestra, nullptr, nullptr, nullptr);
        applicaFont(etichetta, font);

        HWND controllo = nullptr;
        if (!campo.opzioni.empty())
        {
            controllo = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
                180, y, 380, 260, finestra,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_CAMPO_BASE + i)), nullptr, nullptr);
            for (const std::wstring& opzione : campo.opzioni)
                SendMessageW(controllo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(opzione.c_str()));

            int selezione = campo.indiceSelezionato;
            if (selezione < 0 && !campo.valore.empty())
                selezione = static_cast<int>(SendMessageW(controllo, CB_FINDSTRINGEXACT, -1, reinterpret_cast<LPARAM>(campo.valore.c_str())));
            if (selezione < 0 && !campo.opzioni.empty()) selezione = 0;
            SendMessageW(controllo, CB_SETCURSEL, selezione, 0);
        }
        else
        {
            DWORD stile = WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL;
            if (campo.solaLettura) stile |= ES_READONLY;
            controllo = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", campo.valore.c_str(), stile,
                180, y, 380, 32, finestra,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_CAMPO_BASE + i)), nullptr, nullptr);
        }
        applicaFont(controllo, font);
        controlli.push_back(controllo);
    }

    const int yPulsanti = 35 + static_cast<int>(campi->size()) * 54;
    HWND salva = CreateWindowExW(0, L"BUTTON", L"Salva", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        350, yPulsanti, 100, 36, finestra, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_SALVA)), nullptr, nullptr);
    HWND annulla = CreateWindowExW(0, L"BUTTON", L"Annulla", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        460, yPulsanti, 100, 36, finestra, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_ANNULLA)), nullptr, nullptr);
    applicaFont(salva, font);
    applicaFont(annulla, font);
}

void FinestraRecord::raccogliValori()
{
    for (int i = 0; i < static_cast<int>(campi->size()); ++i)
    {
        CampoRecord& campo = (*campi)[i];
        if (!campo.opzioni.empty())
        {
            campo.indiceSelezionato = static_cast<int>(SendMessageW(controlli[i], CB_GETCURSEL, 0, 0));
            if (campo.indiceSelezionato >= 0)
                campo.valore = campo.opzioni[campo.indiceSelezionato];
        }
        else
        {
            const int lunghezza = GetWindowTextLengthW(controlli[i]);
            std::wstring valore(lunghezza + 1, L'\0');
            GetWindowTextW(controlli[i], valore.data(), lunghezza + 1);
            valore.resize(lunghezza);
            campo.valore = valore;
        }
    }
}
