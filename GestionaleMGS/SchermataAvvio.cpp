#include <Windows.h>
#include <gdiplus.h>

#include <algorithm>
#include <filesystem>
#include <memory>

#include "SchermataAvvio.hpp"

#pragma comment(lib, "Gdiplus.lib")

namespace
{
    constexpr UINT_PTR ID_TIMER_GIF = 1;

    std::filesystem::path percorsoGif()
    {
        std::vector<wchar_t> buffer(32768);
        const DWORD lunghezza = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (lunghezza == 0 || lunghezza >= buffer.size()) return {};
        return std::filesystem::path(buffer.data()).parent_path() /
            L"Risorse" / L"gran-sasso-intro-logo-preview.gif";
    }
}

SchermataAvvio::SchermataAvvio()
    : finestra(nullptr), gif(nullptr), gdiplusToken(0), dimensioneTemporale(GUID_NULL),
    fotogrammaCorrente(0), numeroFotogrammi(0), larghezza(960), altezza(540)
{
}

SchermataAvvio::~SchermataAvvio()
{
    delete gif;
    if (gdiplusToken != 0) Gdiplus::GdiplusShutdown(gdiplusToken);
}

void SchermataAvvio::mostra(HINSTANCE istanza)
{
    SchermataAvvio schermata;
    schermata.esegui(istanza);
}

bool SchermataAvvio::caricaGif()
{
    Gdiplus::GdiplusStartupInput input;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &input, nullptr) != Gdiplus::Ok) return false;

    const std::filesystem::path percorso = percorsoGif();
    std::error_code errore;
    if (!std::filesystem::is_regular_file(percorso, errore)) return false;

    gif = Gdiplus::Image::FromFile(percorso.c_str(), FALSE);
    if (gif == nullptr || gif->GetLastStatus() != Gdiplus::Ok) return false;
    larghezza = static_cast<int>(gif->GetWidth());
    altezza = static_cast<int>(gif->GetHeight());

    UINT numeroDimensioni = gif->GetFrameDimensionsCount();
    if (numeroDimensioni == 0) return true;
    std::vector<GUID> dimensioni(numeroDimensioni);
    if (gif->GetFrameDimensionsList(dimensioni.data(), numeroDimensioni) != Gdiplus::Ok) return true;
    dimensioneTemporale = dimensioni[0];
    numeroFotogrammi = gif->GetFrameCount(&dimensioneTemporale);
    if (numeroFotogrammi == 0) numeroFotogrammi = 1;

    ritardi.assign(numeroFotogrammi, 70);
    const UINT dimensioneProprieta = gif->GetPropertyItemSize(PropertyTagFrameDelay);
    if (dimensioneProprieta > 0)
    {
        std::vector<BYTE> buffer(dimensioneProprieta);
        auto* proprieta = reinterpret_cast<Gdiplus::PropertyItem*>(buffer.data());
        if (gif->GetPropertyItem(PropertyTagFrameDelay, dimensioneProprieta, proprieta) == Gdiplus::Ok)
        {
            const auto* valori = static_cast<const UINT*>(proprieta->value);
            const UINT disponibili = proprieta->length / sizeof(UINT);
            for (UINT i = 0; i < numeroFotogrammi && i < disponibili; ++i)
                ritardi[i] = valori[i] * 10 < 20 ? 20 : valori[i] * 10;
        }
    }
    return true;
}

int SchermataAvvio::esegui(HINSTANCE istanza)
{
    if (!caricaGif()) return 0;

    static bool classeRegistrata = false;
    if (!classeRegistrata)
    {
        WNDCLASSEXW classe{ sizeof(WNDCLASSEXW) };
        classe.lpfnWndProc = procedura;
        classe.hInstance = istanza;
        classe.hCursor = LoadCursor(nullptr, IDC_ARROW);
        classe.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        classe.lpszClassName = L"GestionaleMGSSplashWindow";
        classeRegistrata = RegisterClassExW(&classe) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
    }
    if (!classeRegistrata) return 0;

    RECT areaLavoro{};
    if (!SystemParametersInfoW(SPI_GETWORKAREA, 0, &areaLavoro, 0))
    {
        areaLavoro.right = GetSystemMetrics(SM_CXSCREEN);
        areaLavoro.bottom = GetSystemMetrics(SM_CYSCREEN);
    }
    const int areaLavoroLarghezza = areaLavoro.right - areaLavoro.left;
    const int areaLavoroAltezza = areaLavoro.bottom - areaLavoro.top;
    const double scalaX = static_cast<double>(areaLavoroLarghezza) * 0.9 / larghezza;
    const double scalaY = static_cast<double>(areaLavoroAltezza) * 0.9 / altezza;
    const double scalaMinima = scalaX < scalaY ? scalaX : scalaY;
    const double scala = scalaMinima < 1.0 ? scalaMinima : 1.0;
    larghezza = static_cast<int>(larghezza * scala);
    altezza = static_cast<int>(altezza * scala);
    const int x = (GetSystemMetrics(SM_CXSCREEN) - larghezza) / 2;
    const int y = (GetSystemMetrics(SM_CYSCREEN) - altezza) / 2;

    finestra = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, L"GestionaleMGSSplashWindow", L"",
        WS_POPUP, x, y, larghezza, altezza, nullptr, nullptr, istanza, this);
    if (finestra == nullptr) return 0;

    ShowWindow(finestra, SW_SHOW);
    UpdateWindow(finestra);
    SetTimer(finestra, ID_TIMER_GIF, ritardi.empty() ? 2000 : ritardi[0], nullptr);

    MSG messaggio{};
    while (IsWindow(finestra) && GetMessageW(&messaggio, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&messaggio);
        DispatchMessageW(&messaggio);
    }
    return 0;
}

LRESULT CALLBACK SchermataAvvio::procedura(HWND hwnd, UINT messaggio, WPARAM wParam, LPARAM lParam)
{
    auto* schermata = reinterpret_cast<SchermataAvvio*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (messaggio == WM_NCCREATE)
    {
        const auto* creazione = reinterpret_cast<CREATESTRUCTW*>(lParam);
        schermata = static_cast<SchermataAvvio*>(creazione->lpCreateParams);
        schermata->finestra = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(schermata));
    }
    return schermata == nullptr ? DefWindowProcW(hwnd, messaggio, wParam, lParam)
        : schermata->gestisci(messaggio, wParam, lParam);
}

LRESULT SchermataAvvio::gestisci(UINT messaggio, WPARAM wParam, LPARAM lParam)
{
    switch (messaggio)
    {
    case WM_PAINT:
        disegna();
        return 0;
    case WM_TIMER:
        if (wParam == ID_TIMER_GIF) avanzaFotogramma();
        return 0;
    case WM_LBUTTONDOWN:
    case WM_KEYDOWN:
    case WM_CLOSE:
        DestroyWindow(finestra);
        return 0;
    case WM_DESTROY:
        KillTimer(finestra, ID_TIMER_GIF);
        return 0;
    }
    return DefWindowProcW(finestra, messaggio, wParam, lParam);
}

void SchermataAvvio::avanzaFotogramma()
{
    KillTimer(finestra, ID_TIMER_GIF);
    if (numeroFotogrammi <= 1 || fotogrammaCorrente + 1 >= numeroFotogrammi)
    {
        DestroyWindow(finestra);
        return;
    }

    ++fotogrammaCorrente;
    gif->SelectActiveFrame(&dimensioneTemporale, fotogrammaCorrente);
    InvalidateRect(finestra, nullptr, FALSE);
    SetTimer(finestra, ID_TIMER_GIF, ritardi[fotogrammaCorrente], nullptr);
}

void SchermataAvvio::disegna()
{
    PAINTSTRUCT paint{};
    HDC dc = BeginPaint(finestra, &paint);
    Gdiplus::Graphics grafica(dc);
    grafica.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    grafica.DrawImage(gif, 0, 0, larghezza, altezza);
    EndPaint(finestra, &paint);
}
