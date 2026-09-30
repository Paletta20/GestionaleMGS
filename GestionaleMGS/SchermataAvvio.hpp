#ifndef SchermataAvvio_hpp
#define SchermataAvvio_hpp

#include <Windows.h>

#include <string>
#include <vector>

namespace Gdiplus
{
    class Image;
}

class SchermataAvvio
{
private:
    HWND finestra;
    Gdiplus::Image* gif;
    ULONG_PTR gdiplusToken;
    GUID dimensioneTemporale;
    UINT fotogrammaCorrente;
    UINT numeroFotogrammi;
    std::vector<UINT> ritardi;
    int larghezza;
    int altezza;

    SchermataAvvio();
    ~SchermataAvvio();
    bool caricaGif();
    void avanzaFotogramma();
    void disegna();
    int esegui(HINSTANCE istanza);
    static LRESULT CALLBACK procedura(HWND hwnd, UINT messaggio, WPARAM wParam, LPARAM lParam);
    LRESULT gestisci(UINT messaggio, WPARAM wParam, LPARAM lParam);

public:
    static void mostra(HINSTANCE istanza);
};

#endif
