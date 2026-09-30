#include <Windows.h>
#include <Windowsx.h>
#include <CommCtrl.h>
#include <gdiplus.h>
#include <algorithm>
#include <cstring>
#include <cwctype>
#include <filesystem>
#include <fstream>

#include "InterfacciaGrafica.hpp"
#include "FinestraRecord.hpp"

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Gdiplus.lib")

namespace
{
    constexpr int ID_PRODOTTI = 1001;
    constexpr int ID_CLIENTI = 1002;
    constexpr int ID_FILATI = 1003;
    constexpr int ID_TECNICI = 1004;
    constexpr int ID_FORNITORI = 1005;
    constexpr int ID_RICERCA = 1010;
    constexpr int ID_NUOVO = 1011;
    constexpr int ID_MODIFICA = 1012;
    constexpr int ID_ELIMINA = 1013;
    constexpr int ID_AGGIORNA = 1014;
    constexpr int ID_PANNELLO_IMMAGINE = 1015;
    constexpr int ID_CARICA_IMMAGINE = 1016;
    constexpr int ID_LOGO = 1017;

    std::wstring utf8ToWide(const std::string& testo)
    {
        if (testo.empty()) return {};
        const int dimensione = MultiByteToWideChar(CP_UTF8, 0, testo.c_str(), -1, nullptr, 0);
        std::wstring risultato(dimensione, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, testo.c_str(), -1, risultato.data(), dimensione);
        risultato.pop_back();
        return risultato;
    }

    std::string wideToUtf8(const std::wstring& testo)
    {
        if (testo.empty()) return {};
        const int dimensione = WideCharToMultiByte(CP_UTF8, 0, testo.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string risultato(dimensione, '\0');
        WideCharToMultiByte(CP_UTF8, 0, testo.c_str(), -1, risultato.data(), dimensione, nullptr, nullptr);
        risultato.pop_back();
        return risultato;
    }

    std::wstring minuscolo(std::wstring testo)
    {
        std::transform(testo.begin(), testo.end(), testo.begin(),
            [](wchar_t carattere) { return static_cast<wchar_t>(std::towlower(carattere)); });
        return testo;
    }

    void applicaFont(HWND controllo, HFONT font)
    {
        SendMessageW(controllo, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }

    bool interoPositivo(HWND genitore, const std::wstring& testo, const wchar_t* nome, int& valore)
    {
        wchar_t* fine = nullptr;
        const long numero = std::wcstol(testo.c_str(), &fine, 10);
        if (testo.empty() || fine == nullptr || *fine != L'\0' || numero <= 0 || numero > INT_MAX)
        {
            const std::wstring messaggio = std::wstring(nome) + L" deve essere un numero intero maggiore di zero.";
            MessageBoxW(genitore, messaggio.c_str(), L"Dato non valido", MB_OK | MB_ICONWARNING);
            return false;
        }
        valore = static_cast<int>(numero);
        return true;
    }

    bool obbligatorio(HWND genitore, const std::wstring& testo, const wchar_t* nome)
    {
        if (!testo.empty()) return true;
        const std::wstring messaggio = std::wstring(L"Compilare il campo ") + nome + L".";
        MessageBoxW(genitore, messaggio.c_str(), L"Dato mancante", MB_OK | MB_ICONWARNING);
        return false;
    }

    template <typename T, typename Predicato>
    int trovaIndice(const std::vector<T>& valori, Predicato predicato)
    {
        const auto posizione = std::find_if(valori.begin(), valori.end(), predicato);
        return posizione == valori.end() ? -1 : static_cast<int>(std::distance(valori.begin(), posizione));
    }

    std::string mimeTypeDaEstensione(std::wstring estensione)
    {
        estensione = minuscolo(estensione);
        if (estensione == L".png") return "image/png";
        if (estensione == L".jpg" || estensione == L".jpeg") return "image/jpeg";
        if (estensione == L".bmp") return "image/bmp";
        if (estensione == L".gif") return "image/gif";
        if (estensione == L".tif" || estensione == L".tiff") return "image/tiff";
        return "application/octet-stream";
    }
}

InterfacciaGrafica::InterfacciaGrafica(DatabaseManager& dbManager)
    : databaseManager(&dbManager), sezioneCorrente(Sezione::Prodotti), finestra(nullptr), elenco(nullptr),
    titolo(nullptr), stato(nullptr), ricerca(nullptr), pulsanteNuovo(nullptr), pulsanteModifica(nullptr),
    pulsanteElimina(nullptr), pulsanteAggiorna(nullptr), pulsanteCaricaImmagine(nullptr), logo(nullptr),
    pannelloImmagine(nullptr), fontInterfaccia(nullptr), fontTitolo(nullptr), gdiplusToken(0),
    logoAzienda(nullptr), immagineVisualizzata(nullptr),
    flussoImmagine(nullptr), messaggioImmagine(L"Immagine non presente nel database."),
    lenteAttiva(false), posizioneLente{ 0, 0 }
{
}

InterfacciaGrafica::~InterfacciaGrafica()
{
    pulisciImmagineProdotto();
    delete logoAzienda;
    if (fontInterfaccia != nullptr) DeleteObject(fontInterfaccia);
    if (fontTitolo != nullptr) DeleteObject(fontTitolo);
}

int InterfacciaGrafica::avvia(HINSTANCE istanza, int mostraComando)
{
    Gdiplus::GdiplusStartupInput gdiplusInput;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusInput, nullptr) != Gdiplus::Ok)
        gdiplusToken = 0;
    if (gdiplusToken != 0)
    {
        wchar_t percorsoEseguibile[MAX_PATH]{};
        GetModuleFileNameW(nullptr, percorsoEseguibile, MAX_PATH);
        const std::filesystem::path percorsoLogo =
            std::filesystem::path(percorsoEseguibile).parent_path() / L"Risorse" / L"logo-gran-sasso.png";
        logoAzienda = Gdiplus::Image::FromFile(percorsoLogo.c_str(), FALSE);
        if (logoAzienda != nullptr && logoAzienda->GetLastStatus() != Gdiplus::Ok)
        {
            delete logoAzienda;
            logoAzienda = nullptr;
        }
    }

    INITCOMMONCONTROLSEX controlli{ sizeof(INITCOMMONCONTROLSEX), ICC_LISTVIEW_CLASSES };
    InitCommonControlsEx(&controlli);

    WNDCLASSEXW classe{ sizeof(WNDCLASSEXW) };
    classe.lpfnWndProc = proceduraFinestra;
    classe.hInstance = istanza;
    classe.hCursor = LoadCursor(nullptr, IDC_ARROW);
    classe.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    classe.lpszClassName = L"GestionaleMGSWindow";
    classe.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    RegisterClassExW(&classe);

    finestra = CreateWindowExW(0, classe.lpszClassName, L"Gestionale MGS",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1240, 760,
        nullptr, nullptr, istanza, this);
    if (finestra == nullptr) return 1;

    ShowWindow(finestra, mostraComando);
    UpdateWindow(finestra);
    MSG messaggio{};
    while (GetMessageW(&messaggio, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&messaggio);
        DispatchMessageW(&messaggio);
    }
    pulisciImmagineProdotto();
    delete logoAzienda;
    logoAzienda = nullptr;
    if (gdiplusToken != 0)
    {
        Gdiplus::GdiplusShutdown(gdiplusToken);
        gdiplusToken = 0;
    }
    return static_cast<int>(messaggio.wParam);
}

LRESULT CALLBACK InterfacciaGrafica::proceduraFinestra(HWND hwnd, UINT messaggio, WPARAM wParam, LPARAM lParam)
{
    auto* interfaccia = reinterpret_cast<InterfacciaGrafica*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (messaggio == WM_NCCREATE)
    {
        const auto* creazione = reinterpret_cast<CREATESTRUCTW*>(lParam);
        interfaccia = static_cast<InterfacciaGrafica*>(creazione->lpCreateParams);
        interfaccia->finestra = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(interfaccia));
    }
    return interfaccia == nullptr ? DefWindowProcW(hwnd, messaggio, wParam, lParam)
        : interfaccia->gestisciMessaggio(messaggio, wParam, lParam);
}

LRESULT CALLBACK InterfacciaGrafica::proceduraPannelloImmagine(
    HWND hwnd, UINT messaggio, WPARAM wParam, LPARAM lParam,
    UINT_PTR idSottoclasse, DWORD_PTR datiRiferimento)
{
    auto* interfaccia = reinterpret_cast<InterfacciaGrafica*>(datiRiferimento);
    if (interfaccia == nullptr) return DefSubclassProc(hwnd, messaggio, wParam, lParam);

    switch (messaggio)
    {
    case WM_MOUSEMOVE:
    {
        interfaccia->posizioneLente = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        interfaccia->lenteAttiva = interfaccia->immagineVisualizzata != nullptr;
        TRACKMOUSEEVENT tracciamento{ sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0 };
        TrackMouseEvent(&tracciamento);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }
    case WM_MOUSELEAVE:
        interfaccia->lenteAttiva = false;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_SETCURSOR:
        if (interfaccia->immagineVisualizzata != nullptr)
        {
            SetCursor(LoadCursorW(nullptr, IDC_CROSS));
            return TRUE;
        }
        break;
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, proceduraPannelloImmagine, idSottoclasse);
        break;
    }
    return DefSubclassProc(hwnd, messaggio, wParam, lParam);
}

LRESULT InterfacciaGrafica::gestisciMessaggio(UINT messaggio, WPARAM wParam, LPARAM lParam)
{
    switch (messaggio)
    {
    case WM_CREATE:
        creaControlli();
        mostraProdotti();
        return 0;
    case WM_SIZE:
        ridimensiona(LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_NOTIFY:
        if (reinterpret_cast<LPNMHDR>(lParam)->hwndFrom == elenco)
        {
            if (reinterpret_cast<LPNMHDR>(lParam)->code == NM_DBLCLK)
                modificaRecord();
            else if (reinterpret_cast<LPNMHDR>(lParam)->code == LVN_ITEMCHANGED && sezioneCorrente == Sezione::Prodotti)
            {
                const auto* cambiamento = reinterpret_cast<LPNMLISTVIEW>(lParam);
                if ((cambiamento->uChanged & LVIF_STATE) != 0)
                    aggiornaImmagineProdotto();
            }
        }
        return 0;
    case WM_DRAWITEM:
        if (reinterpret_cast<const DRAWITEMSTRUCT*>(lParam)->CtlID == ID_LOGO)
        {
            disegnaLogo(*reinterpret_cast<const DRAWITEMSTRUCT*>(lParam));
            return TRUE;
        }
        if (reinterpret_cast<const DRAWITEMSTRUCT*>(lParam)->CtlID == ID_PANNELLO_IMMAGINE)
        {
            disegnaPannelloImmagine(*reinterpret_cast<const DRAWITEMSTRUCT*>(lParam));
            return TRUE;
        }
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_RICERCA && HIWORD(wParam) == EN_CHANGE)
        {
            aggiornaVista(false);
            return 0;
        }
        switch (LOWORD(wParam))
        {
        case ID_PRODOTTI: sezioneCorrente = Sezione::Prodotti; SetWindowTextW(ricerca, L""); mostraProdotti(); break;
        case ID_CLIENTI: sezioneCorrente = Sezione::Clienti; SetWindowTextW(ricerca, L""); mostraClienti(); break;
        case ID_FILATI: sezioneCorrente = Sezione::Filati; SetWindowTextW(ricerca, L""); mostraFilati(); break;
        case ID_TECNICI: sezioneCorrente = Sezione::Tecnici; SetWindowTextW(ricerca, L""); mostraTecnici(); break;
        case ID_FORNITORI: sezioneCorrente = Sezione::Fornitori; SetWindowTextW(ricerca, L""); mostraFornitori(); break;
        case ID_NUOVO: nuovoRecord(); break;
        case ID_MODIFICA: modificaRecord(); break;
        case ID_ELIMINA: eliminaRecord(); break;
        case ID_AGGIORNA: aggiornaVista(true); break;
        case ID_CARICA_IMMAGINE: caricaImmagineSelezionata(); break;
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(finestra, messaggio, wParam, lParam);
}

void InterfacciaGrafica::creaControlli()
{
    fontInterfaccia = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH, L"Segoe UI");
    fontTitolo = CreateFontW(26, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH, L"Segoe UI");
    logo = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
        18, 12, 170, 72, finestra, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_LOGO)), nullptr, nullptr);
    const wchar_t* etichette[] = { L"Prodotti", L"Clienti", L"Filati", L"Tecnici", L"Fornitori" };
    const int id[] = { ID_PRODOTTI, ID_CLIENTI, ID_FILATI, ID_TECNICI, ID_FORNITORI };
    for (int i = 0; i < 5; ++i)
    {
        HWND pulsante = CreateWindowExW(0, L"BUTTON", etichette[i], WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            18, 107 + i * 50, 170, 38, finestra, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id[i])), nullptr, nullptr);
        applicaFont(pulsante, fontInterfaccia);
    }

    titolo = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
        220, 14, 970, 38, finestra, nullptr, nullptr, nullptr);
    applicaFont(titolo, fontTitolo);
    ricerca = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        220, 58, 300, 34, finestra, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_RICERCA)), nullptr, nullptr);
    SendMessageW(ricerca, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"Cerca nei record..."));
    applicaFont(ricerca, fontInterfaccia);

    pulsanteNuovo = CreateWindowExW(0, L"BUTTON", L"Nuovo", WS_CHILD | WS_VISIBLE, 535, 58, 105, 34,
        finestra, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_NUOVO)), nullptr, nullptr);
    pulsanteModifica = CreateWindowExW(0, L"BUTTON", L"Modifica", WS_CHILD | WS_VISIBLE, 650, 58, 105, 34,
        finestra, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_MODIFICA)), nullptr, nullptr);
    pulsanteElimina = CreateWindowExW(0, L"BUTTON", L"Elimina", WS_CHILD | WS_VISIBLE, 765, 58, 105, 34,
        finestra, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_ELIMINA)), nullptr, nullptr);
    pulsanteAggiorna = CreateWindowExW(0, L"BUTTON", L"Aggiorna", WS_CHILD | WS_VISIBLE, 880, 58, 105, 34,
        finestra, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_AGGIORNA)), nullptr, nullptr);
    pulsanteCaricaImmagine = CreateWindowExW(0, L"BUTTON", L"Carica immagine", WS_CHILD | WS_VISIBLE, 995, 58, 170, 34,
        finestra, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_CARICA_IMMAGINE)), nullptr, nullptr);
    for (HWND controllo : { ricerca, pulsanteNuovo, pulsanteModifica, pulsanteElimina, pulsanteAggiorna, pulsanteCaricaImmagine })
        applicaFont(controllo, fontInterfaccia);

    elenco = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        220, 105, 970, 570, finestra, nullptr, nullptr, nullptr);
    ListView_SetExtendedListViewStyle(elenco, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    applicaFont(elenco, fontInterfaccia);
    pannelloImmagine = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW | SS_NOTIFY,
        900, 105, 290, 420, finestra, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_PANNELLO_IMMAGINE)), nullptr, nullptr);
    SetWindowSubclass(pannelloImmagine, proceduraPannelloImmagine, 1, reinterpret_cast<DWORD_PTR>(this));
    stato = CreateWindowExW(0, L"STATIC", L"Connesso", WS_CHILD | WS_VISIBLE,
        18, 690, 1150, 28, finestra, nullptr, nullptr, nullptr);
    applicaFont(stato, fontInterfaccia);
}

void InterfacciaGrafica::ridimensiona(int larghezza, int altezza)
{
    if (elenco == nullptr) return;
    const bool mostraImmagine = sezioneCorrente == Sezione::Prodotti;
    const int larghezzaPannello = 450;
    const int spazioPannello = mostraImmagine ? larghezzaPannello + 14 : 0;
    const int larghezzaElenco = larghezza - 242 - spazioPannello > 300 ? larghezza - 242 - spazioPannello : 300;
    const int altezzaElenco = altezza - 160 > 180 ? altezza - 160 : 180;
    MoveWindow(elenco, 220, 105, larghezzaElenco, altezzaElenco, TRUE);
    MoveWindow(pannelloImmagine, 220 + larghezzaElenco + 14, 105, larghezzaPannello, altezzaElenco, TRUE);
    MoveWindow(titolo, 220, 12, larghezza - 238 > 300 ? larghezza - 238 : 300, 40, TRUE);
    ShowWindow(pannelloImmagine, mostraImmagine ? SW_SHOW : SW_HIDE);
    ShowWindow(pulsanteCaricaImmagine, mostraImmagine ? SW_SHOW : SW_HIDE);
    MoveWindow(stato, 18, altezza - 42, larghezza - 36, 26, TRUE);
}

std::wstring InterfacciaGrafica::testoRicerca() const
{
    const int lunghezza = GetWindowTextLengthW(ricerca);
    std::wstring testo(lunghezza + 1, L'\0');
    GetWindowTextW(ricerca, testo.data(), lunghezza + 1);
    testo.resize(lunghezza);
    return minuscolo(testo);
}

bool InterfacciaGrafica::corrispondeAllaRicerca(const std::vector<std::wstring>& valori) const
{
    const std::wstring filtro = testoRicerca();
    if (filtro.empty()) return true;
    for (const std::wstring& valore : valori)
        if (minuscolo(valore).find(filtro) != std::wstring::npos) return true;
    return false;
}

void InterfacciaGrafica::preparaElenco(const wchar_t* intestazione, const wchar_t* const colonne[], const int larghezze[], int numeroColonne)
{
    SetWindowTextW(titolo, intestazione);
    ListView_DeleteAllItems(elenco);
    while (ListView_DeleteColumn(elenco, 0)) {}
    for (int i = 0; i < numeroColonne; ++i)
    {
        LVCOLUMNW colonna{};
        colonna.mask = LVCF_TEXT | LVCF_WIDTH;
        colonna.pszText = const_cast<LPWSTR>(colonne[i]);
        colonna.cx = larghezze[i];
        ListView_InsertColumn(elenco, i, &colonna);
    }

    RECT area{};
    GetClientRect(finestra, &area);
    ridimensiona(area.right - area.left, area.bottom - area.top);
}

void InterfacciaGrafica::aggiungiRiga(const std::vector<std::wstring>& valori, int indiceRecord)
{
    if (valori.empty()) return;
    LVITEMW elemento{};
    elemento.mask = LVIF_TEXT | LVIF_PARAM;
    elemento.iItem = ListView_GetItemCount(elenco);
    elemento.pszText = const_cast<LPWSTR>(valori[0].c_str());
    elemento.lParam = indiceRecord;
    const int indice = ListView_InsertItem(elenco, &elemento);
    for (int i = 1; i < static_cast<int>(valori.size()); ++i)
        ListView_SetItemText(elenco, indice, i, const_cast<LPWSTR>(valori[i].c_str()));
}

void InterfacciaGrafica::aggiornaStato(std::size_t visibili, std::size_t totali)
{
    if (!databaseManager->ultimoErrore().empty())
        SetWindowTextW(stato, (L"Errore database: " + utf8ToWide(databaseManager->ultimoErrore())).c_str());
    else
        SetWindowTextW(stato, (L"Database online connesso | " + std::to_wstring(visibili) +
            L" visualizzati su " + std::to_wstring(totali)).c_str());
}

void InterfacciaGrafica::mostraClienti(bool ricarica)
{
    sezioneCorrente = Sezione::Clienti;
    if (ricarica) clienti = databaseManager->caricaClienti();
    const wchar_t* colonne[] = { L"Nome", L"Serie" }; const int larghezze[] = { 420, 150 };
    preparaElenco(L"Clienti", colonne, larghezze, 2);
    std::size_t visibili = 0;
    for (int i = 0; i < static_cast<int>(clienti.size()); ++i)
    {
        const auto valori = std::vector<std::wstring>{ utf8ToWide(clienti[i].getNome()), std::to_wstring(clienti[i].getSerie()) };
        if (corrispondeAllaRicerca(valori)) { aggiungiRiga(valori, i); ++visibili; }
    }
    aggiornaStato(visibili, clienti.size());
}

void InterfacciaGrafica::mostraFilati(bool ricarica)
{
    sezioneCorrente = Sezione::Filati;
    if (ricarica) filati = databaseManager->caricaFilati();
    const wchar_t* colonne[] = { L"Codice", L"Nome", L"Composizione", L"Titolo", L"Fornitore" };
    const int larghezze[] = { 100, 220, 230, 120, 220 };
    preparaElenco(L"Filati", colonne, larghezze, 5);
    std::size_t visibili = 0;
    for (int i = 0; i < static_cast<int>(filati.size()); ++i)
    {
        const auto& d = filati[i];
        const auto valori = std::vector<std::wstring>{ std::to_wstring(d.getCodice()), utf8ToWide(d.getNome()), utf8ToWide(d.getComposizione()), utf8ToWide(d.getTitolo()), utf8ToWide(d.getFornitore()) };
        if (corrispondeAllaRicerca(valori)) { aggiungiRiga(valori, i); ++visibili; }
    }
    aggiornaStato(visibili, filati.size());
}

void InterfacciaGrafica::mostraFornitori(bool ricarica)
{
    sezioneCorrente = Sezione::Fornitori;
    if (ricarica) fornitori = databaseManager->caricaFornitori();
    const wchar_t* colonne[] = { L"Nome", L"Telefono", L"Email", L"Indirizzo", L"Nazione" };
    const int larghezze[] = { 190, 150, 230, 260, 130 };
    preparaElenco(L"Fornitori", colonne, larghezze, 5);
    std::size_t visibili = 0;
    for (int i = 0; i < static_cast<int>(fornitori.size()); ++i)
    {
        const auto& d = fornitori[i];
        const auto valori = std::vector<std::wstring>{ utf8ToWide(d.getNome()), utf8ToWide(d.getTelefono()), utf8ToWide(d.getEmail()), utf8ToWide(d.getIndirizzo()), utf8ToWide(d.getNazione()) };
        if (corrispondeAllaRicerca(valori)) { aggiungiRiga(valori, i); ++visibili; }
    }
    aggiornaStato(visibili, fornitori.size());
}

void InterfacciaGrafica::mostraTecnici(bool ricarica)
{
    sezioneCorrente = Sezione::Tecnici;
    if (ricarica) tecnici = databaseManager->caricaTecnici();
    const wchar_t* colonne[] = { L"Cognome", L"Nome" }; const int larghezze[] = { 320, 320 };
    preparaElenco(L"Tecnici", colonne, larghezze, 2);
    std::size_t visibili = 0;
    for (int i = 0; i < static_cast<int>(tecnici.size()); ++i)
    {
        const auto valori = std::vector<std::wstring>{ utf8ToWide(tecnici[i].getCognome()), utf8ToWide(tecnici[i].getNome()) };
        if (corrispondeAllaRicerca(valori)) { aggiungiRiga(valori, i); ++visibili; }
    }
    aggiornaStato(visibili, tecnici.size());
}

void InterfacciaGrafica::mostraProdotti(bool ricarica)
{
    sezioneCorrente = Sezione::Prodotti;
    if (ricarica) prodotti = databaseManager->caricaProdotti();
    const wchar_t* colonne[] = { L"Articolo", L"Modello", L"Anno", L"Stagione", L"Descrizione", L"Cliente", L"Filato", L"Tecnico" };
    const int larghezze[] = { 90, 90, 80, 90, 250, 160, 160, 180 };
    preparaElenco(L"Prodotti", colonne, larghezze, 8);
    std::size_t visibili = 0;
    for (int i = 0; i < static_cast<int>(prodotti.size()); ++i)
    {
        const auto& d = prodotti[i];
        const auto valori = std::vector<std::wstring>{ std::to_wstring(d.getArticolo()), std::to_wstring(d.getModello()),
            std::to_wstring(d.getAnno()), d.getStagione() == 1 ? L"PE" : L"AI", utf8ToWide(d.getDescrizione()),
            utf8ToWide(d.getCliente().getNome()), utf8ToWide(d.getFilato().getNome()),
            utf8ToWide(d.getTecnico().getCognome() + " " + d.getTecnico().getNome()) };
        if (corrispondeAllaRicerca(valori)) { aggiungiRiga(valori, i); ++visibili; }
    }
    aggiornaStato(visibili, prodotti.size());
    aggiornaImmagineProdotto();
}

void InterfacciaGrafica::pulisciImmagineProdotto()
{
    lenteAttiva = false;
    delete immagineVisualizzata;
    immagineVisualizzata = nullptr;
    if (flussoImmagine != nullptr)
    {
        flussoImmagine->Release();
        flussoImmagine = nullptr;
    }
}

void InterfacciaGrafica::aggiornaImmagineProdotto()
{
    pulisciImmagineProdotto();
    messaggioImmagine = L"Immagine non presente nel database.";

    const int indice = indiceRecordSelezionato();
    if (indice >= 0 && indice < static_cast<int>(prodotti.size()) && gdiplusToken != 0)
    {
        std::string nomeFile;
        std::string mimeType;
        std::vector<unsigned char> dati;
        if (databaseManager->caricaImmagineProdotto(
            prodotti[indice].getArticolo(), prodotti[indice].getModello(), nomeFile, mimeType, dati) && !dati.empty())
        {
            HGLOBAL memoria = GlobalAlloc(GMEM_MOVEABLE, dati.size());
            if (memoria != nullptr)
            {
                void* destinazione = GlobalLock(memoria);
                if (destinazione != nullptr)
                {
                    std::memcpy(destinazione, dati.data(), dati.size());
                    GlobalUnlock(memoria);
                    if (CreateStreamOnHGlobal(memoria, TRUE, &flussoImmagine) == S_OK)
                    {
                        Gdiplus::Image* immagine = Gdiplus::Image::FromStream(flussoImmagine, FALSE);
                        if (immagine != nullptr && immagine->GetLastStatus() == Gdiplus::Ok &&
                            immagine->GetWidth() > 0 && immagine->GetHeight() > 0)
                            immagineVisualizzata = immagine;
                        else
                            delete immagine;
                    }
                    else
                    {
                        GlobalFree(memoria);
                    }
                }
                else
                {
                    GlobalFree(memoria);
                }
            }
        }
    }

    if (pannelloImmagine != nullptr)
        InvalidateRect(pannelloImmagine, nullptr, TRUE);
}

void InterfacciaGrafica::caricaImmagineSelezionata()
{
    const int indice = indiceRecordSelezionato();
    if (indice < 0 || indice >= static_cast<int>(prodotti.size()))
    {
        MessageBoxW(finestra, L"Selezionare prima un prodotto.", L"Nessuna selezione", MB_OK | MB_ICONINFORMATION);
        return;
    }

    wchar_t percorsoFile[32768]{};
    OPENFILENAMEW selezione{};
    selezione.lStructSize = sizeof(OPENFILENAMEW);
    selezione.hwndOwner = finestra;
    selezione.lpstrFilter = L"Immagini supportate\0*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.tif;*.tiff\0PNG\0*.png\0JPEG\0*.jpg;*.jpeg\0Tutti i file\0*.*\0";
    selezione.lpstrFile = percorsoFile;
    selezione.nMaxFile = static_cast<DWORD>(std::size(percorsoFile));
    selezione.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&selezione)) return;

    const std::filesystem::path percorso(percorsoFile);
    std::ifstream file(percorso, std::ios::binary | std::ios::ate);
    if (!file)
    {
        MessageBoxW(finestra, L"Impossibile leggere il file selezionato.", L"Errore immagine", MB_OK | MB_ICONERROR);
        return;
    }
    const std::streamsize dimensione = file.tellg();
    constexpr std::streamsize dimensioneMassima = 10 * 1024 * 1024;
    if (dimensione <= 0 || dimensione > dimensioneMassima)
    {
        MessageBoxW(finestra, L"L'immagine deve avere una dimensione compresa tra 1 byte e 10 MB.", L"Dimensione non valida", MB_OK | MB_ICONWARNING);
        return;
    }
    file.seekg(0, std::ios::beg);
    std::vector<unsigned char> dati(static_cast<std::size_t>(dimensione));
    if (!file.read(reinterpret_cast<char*>(dati.data()), dimensione))
    {
        MessageBoxW(finestra, L"Lettura del file immagine non riuscita.", L"Errore immagine", MB_OK | MB_ICONERROR);
        return;
    }

    const Prodotto& prodotto = prodotti[indice];
    if (!databaseManager->salvaImmagineProdotto(
        prodotto.getArticolo(), prodotto.getModello(), wideToUtf8(percorso.filename().wstring()),
        mimeTypeDaEstensione(percorso.extension().wstring()), dati))
    {
        mostraErroreDatabase(L"Caricamento immagine");
        return;
    }

    aggiornaImmagineProdotto();
    MessageBoxW(finestra, L"Immagine caricata nel database online.", L"Caricamento completato", MB_OK | MB_ICONINFORMATION);
}

void InterfacciaGrafica::disegnaLogo(const DRAWITEMSTRUCT& disegno)
{
    const int larghezza = disegno.rcItem.right - disegno.rcItem.left;
    const int altezza = disegno.rcItem.bottom - disegno.rcItem.top;
    Gdiplus::Graphics grafica(disegno.hDC);
    grafica.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    grafica.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);

    Gdiplus::SolidBrush sfondo(Gdiplus::Color(255, 255, 255, 255));
    grafica.FillRectangle(&sfondo, 0, 0, larghezza, altezza);
    if (logoAzienda == nullptr) return;

    const UINT larghezzaOriginale = logoAzienda->GetWidth();
    const UINT altezzaOriginale = logoAzienda->GetHeight();
    const int sorgenteX = static_cast<int>(larghezzaOriginale * 0.03);
    const int sorgenteY = static_cast<int>(altezzaOriginale * 0.285);
    const int sorgenteLarghezza = static_cast<int>(larghezzaOriginale * 0.94);
    const int sorgenteAltezza = static_cast<int>(altezzaOriginale * 0.46);
    const double scalaX = static_cast<double>(larghezza - 8) / sorgenteLarghezza;
    const double scalaY = static_cast<double>(altezza - 8) / sorgenteAltezza;
    const double scala = scalaX < scalaY ? scalaX : scalaY;
    const int larghezzaLogo = static_cast<int>(sorgenteLarghezza * scala);
    const int altezzaLogo = static_cast<int>(sorgenteAltezza * scala);
    const int x = (larghezza - larghezzaLogo) / 2;
    const int y = (altezza - altezzaLogo) / 2;

    grafica.DrawImage(
        logoAzienda,
        Gdiplus::Rect(x, y, larghezzaLogo, altezzaLogo),
        sorgenteX, sorgenteY, sorgenteLarghezza, sorgenteAltezza,
        Gdiplus::UnitPixel);
}

void InterfacciaGrafica::disegnaPannelloImmagine(const DRAWITEMSTRUCT& disegno)
{
    const int larghezza = disegno.rcItem.right - disegno.rcItem.left;
    const int altezza = disegno.rcItem.bottom - disegno.rcItem.top;
    HDC dcMemoria = CreateCompatibleDC(disegno.hDC);
    HBITMAP bitmapMemoria = dcMemoria != nullptr
        ? CreateCompatibleBitmap(disegno.hDC, larghezza, altezza)
        : nullptr;
    HGDIOBJ bitmapPrecedente = bitmapMemoria != nullptr
        ? SelectObject(dcMemoria, bitmapMemoria)
        : nullptr;
    HDC dcDisegno = bitmapPrecedente != nullptr ? dcMemoria : disegno.hDC;

    {
    Gdiplus::Graphics grafica(dcDisegno);
    grafica.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    grafica.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);

    Gdiplus::SolidBrush sfondo(Gdiplus::Color(255, 255, 255, 255));
    grafica.FillRectangle(&sfondo, 0, 0, larghezza, altezza);
    Gdiplus::Pen bordo(Gdiplus::Color(255, 190, 196, 202), 1.0f);
    grafica.DrawRectangle(&bordo, 0, 0, larghezza - 1, altezza - 1);

    if (immagineVisualizzata != nullptr)
    {
        const double scalaX = static_cast<double>(larghezza - 20) / immagineVisualizzata->GetWidth();
        const double scalaY = static_cast<double>(altezza - 20) / immagineVisualizzata->GetHeight();
        const double scala = scalaX < scalaY ? scalaX : scalaY;
        const int larghezzaImmagine = static_cast<int>(immagineVisualizzata->GetWidth() * scala);
        const int altezzaImmagine = static_cast<int>(immagineVisualizzata->GetHeight() * scala);
        const int x = (larghezza - larghezzaImmagine) / 2;
        const int y = (altezza - altezzaImmagine) / 2;
        grafica.DrawImage(immagineVisualizzata, x, y, larghezzaImmagine, altezzaImmagine);

        const bool puntatoreSullImmagine =
            posizioneLente.x >= x && posizioneLente.x < x + larghezzaImmagine &&
            posizioneLente.y >= y && posizioneLente.y < y + altezzaImmagine;
        if (lenteAttiva && puntatoreSullImmagine)
        {
            constexpr int diametroLente = 180;
            constexpr int raggioLente = diametroLente / 2;
            constexpr double ingrandimento = 2.5;

            int centroX = posizioneLente.x;
            int centroY = posizioneLente.y;
            if (centroX < raggioLente + 2) centroX = raggioLente + 2;
            if (centroX > larghezza - raggioLente - 2) centroX = larghezza - raggioLente - 2;
            if (centroY < raggioLente + 2) centroY = raggioLente + 2;
            if (centroY > altezza - raggioLente - 2) centroY = altezza - raggioLente - 2;

            const double origineX =
                static_cast<double>(posizioneLente.x - x) * immagineVisualizzata->GetWidth() / larghezzaImmagine;
            const double origineY =
                static_cast<double>(posizioneLente.y - y) * immagineVisualizzata->GetHeight() / altezzaImmagine;
            double sorgenteLarghezza =
                diametroLente * static_cast<double>(immagineVisualizzata->GetWidth()) /
                (larghezzaImmagine * ingrandimento);
            double sorgenteAltezza =
                diametroLente * static_cast<double>(immagineVisualizzata->GetHeight()) /
                (altezzaImmagine * ingrandimento);
            if (sorgenteLarghezza < 1.0) sorgenteLarghezza = 1.0;
            if (sorgenteAltezza < 1.0) sorgenteAltezza = 1.0;
            if (sorgenteLarghezza > immagineVisualizzata->GetWidth())
                sorgenteLarghezza = immagineVisualizzata->GetWidth();
            if (sorgenteAltezza > immagineVisualizzata->GetHeight())
                sorgenteAltezza = immagineVisualizzata->GetHeight();

            double sorgenteX = origineX - sorgenteLarghezza / 2.0;
            double sorgenteY = origineY - sorgenteAltezza / 2.0;
            const double massimoX = immagineVisualizzata->GetWidth() - sorgenteLarghezza;
            const double massimoY = immagineVisualizzata->GetHeight() - sorgenteAltezza;
            if (sorgenteX < 0.0) sorgenteX = 0.0;
            if (sorgenteY < 0.0) sorgenteY = 0.0;
            if (sorgenteX > massimoX) sorgenteX = massimoX;
            if (sorgenteY > massimoY) sorgenteY = massimoY;

            const int lenteX = centroX - raggioLente;
            const int lenteY = centroY - raggioLente;
            Gdiplus::GraphicsPath areaLente;
            areaLente.AddEllipse(lenteX, lenteY, diametroLente, diametroLente);
            grafica.SetClip(&areaLente);
            Gdiplus::SolidBrush sfondoLente(Gdiplus::Color(255, 255, 255, 255));
            grafica.FillEllipse(&sfondoLente, lenteX, lenteY, diametroLente, diametroLente);
            grafica.DrawImage(
                immagineVisualizzata,
                Gdiplus::Rect(lenteX, lenteY, diametroLente, diametroLente),
                static_cast<INT>(sorgenteX), static_cast<INT>(sorgenteY),
                static_cast<INT>(sorgenteLarghezza), static_cast<INT>(sorgenteAltezza),
                Gdiplus::UnitPixel);
            grafica.ResetClip();

            Gdiplus::Pen ombraLente(Gdiplus::Color(90, 0, 0, 0), 5.0f);
            grafica.DrawEllipse(&ombraLente, lenteX + 2, lenteY + 2, diametroLente - 4, diametroLente - 4);
            Gdiplus::Pen bordoLente(Gdiplus::Color(255, 245, 245, 245), 2.0f);
            grafica.DrawEllipse(&bordoLente, lenteX + 2, lenteY + 2, diametroLente - 4, diametroLente - 4);
        }
    }
    else
    {
        SetBkMode(dcDisegno, TRANSPARENT);
        SetTextColor(dcDisegno, RGB(95, 103, 112));
        HFONT precedente = static_cast<HFONT>(SelectObject(dcDisegno, fontInterfaccia));
        RECT testo{ 24, 0, larghezza - 24, altezza };
        DrawTextW(dcDisegno, messaggioImmagine.c_str(), -1, &testo,
            DT_CENTER | DT_WORDBREAK | DT_CALCRECT);
        const int altezzaTesto = testo.bottom - testo.top;
        testo.top = (altezza - altezzaTesto) / 2;
        testo.bottom = testo.top + altezzaTesto;
        DrawTextW(dcDisegno, messaggioImmagine.c_str(), -1, &testo, DT_CENTER | DT_WORDBREAK);
        SelectObject(dcDisegno, precedente);
    }
    }

    if (bitmapPrecedente != nullptr)
    {
        BitBlt(disegno.hDC, 0, 0, larghezza, altezza, dcMemoria, 0, 0, SRCCOPY);
        SelectObject(dcMemoria, bitmapPrecedente);
    }
    if (bitmapMemoria != nullptr) DeleteObject(bitmapMemoria);
    if (dcMemoria != nullptr) DeleteDC(dcMemoria);
}

void InterfacciaGrafica::aggiornaVista(bool ricarica)
{
    switch (sezioneCorrente)
    {
    case Sezione::Prodotti: mostraProdotti(ricarica); break;
    case Sezione::Clienti: mostraClienti(ricarica); break;
    case Sezione::Filati: mostraFilati(ricarica); break;
    case Sezione::Tecnici: mostraTecnici(ricarica); break;
    case Sezione::Fornitori: mostraFornitori(ricarica); break;
    }
}

int InterfacciaGrafica::indiceRecordSelezionato() const
{
    const int riga = ListView_GetNextItem(elenco, -1, LVNI_SELECTED);
    if (riga < 0) return -1;
    LVITEMW elemento{};
    elemento.mask = LVIF_PARAM;
    elemento.iItem = riga;
    return ListView_GetItem(elenco, &elemento) ? static_cast<int>(elemento.lParam) : -1;
}

void InterfacciaGrafica::mostraErroreDatabase(const wchar_t* operazione)
{
    const std::wstring messaggio = std::wstring(operazione) + L" non riuscita.\n\n" + utf8ToWide(databaseManager->ultimoErrore());
    MessageBoxW(finestra, messaggio.c_str(), L"Errore database", MB_OK | MB_ICONERROR);
}

void InterfacciaGrafica::nuovoRecord()
{
    FinestraRecord modulo(finestra);
    bool riuscito = false;

    if (sezioneCorrente == Sezione::Clienti)
    {
        std::vector<CampoRecord> campi = { {L"Nome"}, {L"Serie"} };
        if (!modulo.mostra(L"Nuovo cliente", campi)) return;
        int serie = 0;
        if (!obbligatorio(finestra, campi[0].valore, L"Nome") || !interoPositivo(finestra, campi[1].valore, L"Serie", serie)) return;
        riuscito = databaseManager->inserisciCliente(Cliente(wideToUtf8(campi[0].valore), serie));
    }
    else if (sezioneCorrente == Sezione::Tecnici)
    {
        std::vector<CampoRecord> campi = { {L"Cognome"}, {L"Nome"} };
        if (!modulo.mostra(L"Nuovo tecnico", campi)) return;
        if (!obbligatorio(finestra, campi[0].valore, L"Cognome") || !obbligatorio(finestra, campi[1].valore, L"Nome")) return;
        riuscito = databaseManager->inserisciTecnico(Tecnico(wideToUtf8(campi[0].valore), wideToUtf8(campi[1].valore)));
    }
    else if (sezioneCorrente == Sezione::Fornitori)
    {
        std::vector<CampoRecord> campi = { {L"Nome"}, {L"Telefono"}, {L"Email"}, {L"Indirizzo"}, {L"Nazione"} };
        if (!modulo.mostra(L"Nuovo fornitore", campi)) return;
        if (!obbligatorio(finestra, campi[0].valore, L"Nome")) return;
        riuscito = databaseManager->inserisciFornitore(Fornitore(wideToUtf8(campi[0].valore), wideToUtf8(campi[1].valore),
            wideToUtf8(campi[2].valore), wideToUtf8(campi[3].valore), wideToUtf8(campi[4].valore)));
    }
    else if (sezioneCorrente == Sezione::Filati)
    {
        const auto scelteFornitori = databaseManager->caricaFornitori();
        if (scelteFornitori.empty()) { MessageBoxW(finestra, L"Inserire prima almeno un fornitore attivo.", L"Fornitore necessario", MB_OK | MB_ICONWARNING); return; }
        std::vector<std::wstring> opzioni;
        for (const auto& f : scelteFornitori) opzioni.push_back(utf8ToWide(f.getNome()));
        std::vector<CampoRecord> campi = { {L"Codice"}, {L"Nome"}, {L"Composizione"}, {L"Titolo"}, {L"Fornitore", L"", opzioni} };
        if (!modulo.mostra(L"Nuovo filato", campi)) return;
        int codice = 0;
        if (!interoPositivo(finestra, campi[0].valore, L"Codice", codice) || !obbligatorio(finestra, campi[1].valore, L"Nome")) return;
        riuscito = databaseManager->inserisciFilato(Filato(codice, wideToUtf8(campi[1].valore), wideToUtf8(campi[2].valore),
            wideToUtf8(campi[3].valore), scelteFornitori[campi[4].indiceSelezionato].getNome()));
    }
    else
    {
        const auto scelteFilati = databaseManager->caricaFilati();
        const auto scelteClienti = databaseManager->caricaClienti();
        const auto scelteTecnici = databaseManager->caricaTecnici();
        if (scelteFilati.empty() || scelteClienti.empty() || scelteTecnici.empty())
        {
            MessageBoxW(finestra, L"Per creare un prodotto servono filato, cliente e tecnico attivi.", L"Dati collegati mancanti", MB_OK | MB_ICONWARNING);
            return;
        }
        std::vector<std::wstring> opzioniTecnici;
        for (const auto& t : scelteTecnici) opzioniTecnici.push_back(utf8ToWide(t.getCognome() + " " + t.getNome()));
        std::vector<CampoRecord> campi = { {L"Stagione", L"", {L"PE", L"AI"}}, {L"Anno"}, {L"Articolo"}, {L"Modello"},
            {L"Descrizione"}, {L"Tecnico", L"", opzioniTecnici} };
        if (!modulo.mostra(L"Nuovo prodotto", campi)) return;
        int anno = 0, articolo = 0, modello = 0;
        if (!interoPositivo(finestra, campi[1].valore, L"Anno", anno) || !interoPositivo(finestra, campi[2].valore, L"Articolo", articolo) ||
            !interoPositivo(finestra, campi[3].valore, L"Modello", modello) || !obbligatorio(finestra, campi[4].valore, L"Descrizione")) return;

        if (anno < 1900 || anno > 2100 || articolo < 10000 || articolo > 99999 || modello < 10000 || modello > 99999)
        {
            MessageBoxW(finestra, L"Anno deve essere compreso tra 1900 e 2100; articolo e modello devono essere numeri di cinque cifre.",
                L"Codici prodotto non validi", MB_OK | MB_ICONWARNING);
            return;
        }
        const int codiceFilato = Prodotto::estraiCodiceFilatoDaArticolo(articolo);
        const int serieCliente = Prodotto::estraiSerieClienteDaModello(modello);
        const int indiceFilato = trovaIndice(scelteFilati, [codiceFilato](const Filato& f) { return f.getCodice() == codiceFilato; });
        const int indiceCliente = trovaIndice(scelteClienti, [serieCliente](const Cliente& c) { return c.getSerie() == serieCliente; });
        if (indiceFilato < 0)
        {
            const std::wstring messaggio = L"Codice filato inserito: " + std::to_wstring(codiceFilato) +
                L", che non esiste o non e' attivo.";
            MessageBoxW(finestra, messaggio.c_str(), L"Filato non valido", MB_OK | MB_ICONWARNING);
            return;
        }
        if (indiceCliente < 0)
        {
            const std::wstring messaggio = L"Serie cliente inserita: " + std::to_wstring(serieCliente) +
                L", che non esiste o non e' attiva.";
            MessageBoxW(finestra, messaggio.c_str(), L"Cliente non valido", MB_OK | MB_ICONWARNING);
            return;
        }
        Prodotto prodotto(static_cast<short>(campi[0].indiceSelezionato + 1), anno, articolo, modello, wideToUtf8(campi[4].valore),
            scelteFilati[indiceFilato], scelteClienti[indiceCliente], scelteTecnici[campi[5].indiceSelezionato]);
        riuscito = databaseManager->inserisciProdotto(prodotto);
    }

    if (!riuscito) { mostraErroreDatabase(L"Inserimento"); return; }
    aggiornaVista(true);
}

void InterfacciaGrafica::modificaRecord()
{
    const int indice = indiceRecordSelezionato();
    if (indice < 0) { MessageBoxW(finestra, L"Selezionare prima un record.", L"Nessuna selezione", MB_OK | MB_ICONINFORMATION); return; }
    FinestraRecord modulo(finestra);
    bool riuscito = false;

    if (sezioneCorrente == Sezione::Clienti)
    {
        const Cliente originale = clienti[indice];
        std::vector<CampoRecord> campi = { {L"Nome", utf8ToWide(originale.getNome())}, {L"Serie", std::to_wstring(originale.getSerie())} };
        if (!modulo.mostra(L"Modifica cliente", campi)) return;
        int serie = 0;
        if (!obbligatorio(finestra, campi[0].valore, L"Nome") || !interoPositivo(finestra, campi[1].valore, L"Serie", serie)) return;
        riuscito = databaseManager->aggiornaCliente(originale.getSerie(), Cliente(wideToUtf8(campi[0].valore), serie));
    }
    else if (sezioneCorrente == Sezione::Tecnici)
    {
        const Tecnico originale = tecnici[indice];
        std::vector<CampoRecord> campi = { {L"Cognome", utf8ToWide(originale.getCognome())}, {L"Nome", utf8ToWide(originale.getNome())} };
        if (!modulo.mostra(L"Modifica tecnico", campi)) return;
        if (!obbligatorio(finestra, campi[0].valore, L"Cognome") || !obbligatorio(finestra, campi[1].valore, L"Nome")) return;
        riuscito = databaseManager->aggiornaTecnico(originale.getCognome(), originale.getNome(), Tecnico(wideToUtf8(campi[0].valore), wideToUtf8(campi[1].valore)));
    }
    else if (sezioneCorrente == Sezione::Fornitori)
    {
        const Fornitore originale = fornitori[indice];
        std::vector<CampoRecord> campi = { {L"Nome", utf8ToWide(originale.getNome())}, {L"Telefono", utf8ToWide(originale.getTelefono())},
            {L"Email", utf8ToWide(originale.getEmail())}, {L"Indirizzo", utf8ToWide(originale.getIndirizzo())}, {L"Nazione", utf8ToWide(originale.getNazione())} };
        if (!modulo.mostra(L"Modifica fornitore", campi)) return;
        if (!obbligatorio(finestra, campi[0].valore, L"Nome")) return;
        Fornitore aggiornato(originale.getId_Fornitore(), wideToUtf8(campi[0].valore), wideToUtf8(campi[1].valore), wideToUtf8(campi[2].valore),
            wideToUtf8(campi[3].valore), wideToUtf8(campi[4].valore));
        riuscito = databaseManager->aggiornaFornitore(originale.getId_Fornitore(), aggiornato);
    }
    else if (sezioneCorrente == Sezione::Filati)
    {
        const Filato originale = filati[indice];
        const auto scelteFornitori = databaseManager->caricaFornitori();
        std::vector<std::wstring> opzioni;
        for (const auto& f : scelteFornitori) opzioni.push_back(utf8ToWide(f.getNome()));
        const int selezione = trovaIndice(scelteFornitori, [&](const Fornitore& f) { return f.getNome() == originale.getFornitore(); });
        std::vector<CampoRecord> campi = { {L"Codice", std::to_wstring(originale.getCodice()), {}, true}, {L"Nome", utf8ToWide(originale.getNome())},
            {L"Composizione", utf8ToWide(originale.getComposizione())}, {L"Titolo", utf8ToWide(originale.getTitolo())}, {L"Fornitore", L"", opzioni, false, selezione} };
        if (!modulo.mostra(L"Modifica filato", campi)) return;
        if (!obbligatorio(finestra, campi[1].valore, L"Nome")) return;
        Filato aggiornato(originale.getCodice(), wideToUtf8(campi[1].valore), wideToUtf8(campi[2].valore), wideToUtf8(campi[3].valore),
            scelteFornitori[campi[4].indiceSelezionato].getNome());
        riuscito = databaseManager->aggiornaFilato(originale.getCodice(), aggiornato);
    }
    else
    {
        const Prodotto originale = prodotti[indice];
        const auto scelteFilati = databaseManager->caricaFilati();
        const auto scelteClienti = databaseManager->caricaClienti();
        const auto scelteTecnici = databaseManager->caricaTecnici();
        std::vector<std::wstring> opzioniTecnici;
        for (const auto& t : scelteTecnici) opzioniTecnici.push_back(utf8ToWide(t.getCognome() + " " + t.getNome()));
        const int iTecnico = trovaIndice(scelteTecnici, [&](const Tecnico& t) { return t.getCognome() == originale.getTecnico().getCognome() && t.getNome() == originale.getTecnico().getNome(); });
        std::vector<CampoRecord> campi = { {L"Stagione", L"", {L"PE", L"AI"}, false, originale.getStagione() - 1},
            {L"Anno", std::to_wstring(originale.getAnno())}, {L"Articolo", std::to_wstring(originale.getArticolo())},
            {L"Modello", std::to_wstring(originale.getModello())}, {L"Descrizione", utf8ToWide(originale.getDescrizione())},
            {L"Tecnico", L"", opzioniTecnici, false, iTecnico} };
        if (!modulo.mostra(L"Modifica prodotto", campi)) return;
        int anno = 0, articolo = 0, modello = 0;
        if (!interoPositivo(finestra, campi[1].valore, L"Anno", anno) || !interoPositivo(finestra, campi[2].valore, L"Articolo", articolo) ||
            !interoPositivo(finestra, campi[3].valore, L"Modello", modello) || !obbligatorio(finestra, campi[4].valore, L"Descrizione")) return;

        if (anno < 1900 || anno > 2100 || articolo < 10000 || articolo > 99999 || modello < 10000 || modello > 99999)
        {
            MessageBoxW(finestra, L"Anno deve essere compreso tra 1900 e 2100; articolo e modello devono essere numeri di cinque cifre.",
                L"Codici prodotto non validi", MB_OK | MB_ICONWARNING);
            return;
        }
        const int codiceFilato = Prodotto::estraiCodiceFilatoDaArticolo(articolo);
        const int serieCliente = Prodotto::estraiSerieClienteDaModello(modello);
        const int indiceFilato = trovaIndice(scelteFilati, [codiceFilato](const Filato& f) { return f.getCodice() == codiceFilato; });
        const int indiceCliente = trovaIndice(scelteClienti, [serieCliente](const Cliente& c) { return c.getSerie() == serieCliente; });
        if (indiceFilato < 0 || indiceCliente < 0)
        {
            MessageBoxW(finestra, L"Articolo o modello non identificano un filato e un cliente attivi. Controllare le cifre inserite.",
                L"Relazioni prodotto non valide", MB_OK | MB_ICONWARNING);
            return;
        }
        Prodotto aggiornato = originale;
        aggiornato.setStagione(static_cast<short>(campi[0].indiceSelezionato + 1));
        aggiornato.setAnno(anno); aggiornato.setArticolo(articolo); aggiornato.setModello(modello); aggiornato.setDescrizione(wideToUtf8(campi[4].valore));
        aggiornato.setFilato(scelteFilati[indiceFilato]); aggiornato.setCliente(scelteClienti[indiceCliente]);
        aggiornato.setTecnico(scelteTecnici[campi[5].indiceSelezionato]);
        riuscito = databaseManager->aggiornaProdotto(originale.getArticolo(), originale.getModello(), aggiornato);
    }

    if (!riuscito) { mostraErroreDatabase(L"Modifica"); return; }
    aggiornaVista(true);
}

void InterfacciaGrafica::eliminaRecord()
{
    const int indice = indiceRecordSelezionato();
    if (indice < 0) { MessageBoxW(finestra, L"Selezionare prima un record.", L"Nessuna selezione", MB_OK | MB_ICONINFORMATION); return; }
    if (MessageBoxW(finestra, L"Confermi l'eliminazione del record selezionato?", L"Conferma eliminazione", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES) return;

    bool riuscito = false;
    switch (sezioneCorrente)
    {
    case Sezione::Clienti: riuscito = databaseManager->eliminaCliente(clienti[indice].getSerie()); break;
    case Sezione::Filati: riuscito = databaseManager->eliminaFilato(filati[indice].getCodice()); break;
    case Sezione::Fornitori: riuscito = databaseManager->eliminaFornitore(fornitori[indice].getId_Fornitore()); break;
    case Sezione::Tecnici: riuscito = databaseManager->eliminaTecnico(tecnici[indice].getCognome(), tecnici[indice].getNome()); break;
    case Sezione::Prodotti: riuscito = databaseManager->eliminaProdotto(prodotti[indice].getArticolo(), prodotti[indice].getModello()); break;
    }
    if (!riuscito) { mostraErroreDatabase(L"Eliminazione"); return; }
    aggiornaVista(true);
}
