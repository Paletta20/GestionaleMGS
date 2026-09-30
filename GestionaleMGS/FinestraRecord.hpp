#ifndef FinestraRecord_hpp
#define FinestraRecord_hpp

#include <Windows.h>
#include <string>
#include <vector>

struct CampoRecord
{
    std::wstring etichetta;
    std::wstring valore;
    std::vector<std::wstring> opzioni;
    bool solaLettura = false;
    int indiceSelezionato = -1;
};

class FinestraRecord
{
private:
    HWND genitore;
    HWND finestra;
    HFONT font;
    std::vector<CampoRecord>* campi;
    std::vector<HWND> controlli;
    bool salvato;

    static LRESULT CALLBACK procedura(HWND hwnd, UINT messaggio, WPARAM wParam, LPARAM lParam);
    LRESULT gestisci(UINT messaggio, WPARAM wParam, LPARAM lParam);
    void creaControlli();
    void raccogliValori();

public:
    explicit FinestraRecord(HWND finestraGenitore);
    ~FinestraRecord();
    bool mostra(const std::wstring& titolo, std::vector<CampoRecord>& campiRecord);
};

#endif
