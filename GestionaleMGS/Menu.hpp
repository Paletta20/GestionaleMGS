#ifndef Menu_hpp
#define Menu_hpp

#include "DatabaseManager.hpp"

class Menu
{
private:

    DatabaseManager* databaseManager;

    void menuIniziale();

    void menuProdotti();

    void menuClienti();

    void menuFilati();

    void menuFornitori();

    void menuTecnici();

    int leggiScelta();

    void sceltoProdotti();

    void sceltoClienti();

    void sceltoFilati();

    void sceltoFornitori();

    void sceltoTecnici();

    void inserisciNewProdotto();

    void cercaProdotto();

    void modificaProdotto();

    void eliminaProdotto();

    void stampaProdottiDaDatabase();

    void inserisciNewCliente();

    void cercaCliente();

    void modificaCliente();

    void eliminaCliente();

    void stampaClientiDaDatabase();

    void inserisciNewFilato();

    void cercaFilato();

    void modificaFilato();

    void eliminaFilato();

    void stampaFilatiDaDatabase();

    void inserisciNewFornitore();

    void cercaFornitore();

    void modificaFornitore();

    void eliminaFornitore();

    void stampaFornitoriDaDatabase();

    void inserisciNewTecnico();

    void cercaTecnico();

    void modificaTecnico();

    void eliminaTecnico();

    void stampaTecniciDaDatabase();

public:

    Menu(DatabaseManager& dbManager);

    void avvia();
};

#endif