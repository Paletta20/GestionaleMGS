#ifndef InterfacciaGrafica_hpp
#define InterfacciaGrafica_hpp

#include <Windows.h>
#include <string>
#include <vector>

#include "DatabaseManager.hpp"

namespace Gdiplus
{
    class Image;
}

class InterfacciaGrafica
{
private:
    enum class Sezione { Prodotti, Clienti, Filati, Tecnici, Fornitori };

    DatabaseManager* databaseManager;
    Sezione sezioneCorrente;
    HWND finestra;
    HWND elenco;
    HWND titolo;
    HWND stato;
    HWND ricerca;
    HWND pulsanteNuovo;
    HWND pulsanteModifica;
    HWND pulsanteElimina;
    HWND pulsanteAggiorna;
    HWND pulsanteCaricaImmagine;
    HWND logo;
    HWND pannelloImmagine;
    HFONT fontInterfaccia;
    HFONT fontTitolo;
    ULONG_PTR gdiplusToken;
    Gdiplus::Image* logoAzienda;
    Gdiplus::Image* immagineVisualizzata;
    IStream* flussoImmagine;
    std::wstring messaggioImmagine;
    bool lenteAttiva;
    POINT posizioneLente;

    std::vector<Cliente> clienti;
    std::vector<Filato> filati;
    std::vector<Fornitore> fornitori;
    std::vector<Tecnico> tecnici;
    std::vector<Prodotto> prodotti;

    static LRESULT CALLBACK proceduraFinestra(HWND hwnd, UINT messaggio, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK proceduraPannelloImmagine(
        HWND hwnd, UINT messaggio, WPARAM wParam, LPARAM lParam,
        UINT_PTR idSottoclasse, DWORD_PTR datiRiferimento);
    LRESULT gestisciMessaggio(UINT messaggio, WPARAM wParam, LPARAM lParam);
    void creaControlli();
    void ridimensiona(int larghezza, int altezza);
    void mostraClienti(bool ricarica = true);
    void mostraFilati(bool ricarica = true);
    void mostraFornitori(bool ricarica = true);
    void mostraTecnici(bool ricarica = true);
    void mostraProdotti(bool ricarica = true);
    void aggiornaImmagineProdotto();
    void caricaImmagineSelezionata();
    void pulisciImmagineProdotto();
    void disegnaLogo(const DRAWITEMSTRUCT& disegno);
    void disegnaPannelloImmagine(const DRAWITEMSTRUCT& disegno);
    void aggiornaVista(bool ricarica);
    void nuovoRecord();
    void modificaRecord();
    void eliminaRecord();
    int indiceRecordSelezionato() const;
    std::wstring testoRicerca() const;
    bool corrispondeAllaRicerca(const std::vector<std::wstring>& valori) const;
    void preparaElenco(const wchar_t* intestazione, const wchar_t* const colonne[], const int larghezze[], int numeroColonne);
    void aggiungiRiga(const std::vector<std::wstring>& valori, int indiceRecord);
    void aggiornaStato(std::size_t recordVisibili, std::size_t recordTotali);
    void mostraErroreDatabase(const wchar_t* operazione);

public:
    explicit InterfacciaGrafica(DatabaseManager& dbManager);
    ~InterfacciaGrafica();
    int avvia(HINSTANCE istanza, int mostraComando);
};

#endif
