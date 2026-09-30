//
//  Immagine.hpp
//  GestionaleMGS
//

#ifndef Immagine_hpp
#define Immagine_hpp

#include <string>

using namespace std;

class Immagine {

private:
    int id_immagine;
    int id_prodotto;
    string percorso_file;
    string descrizione;
    string data_caricamento;

public:
    Immagine();

    Immagine(
        string percorso_file,
        string descrizione);

    Immagine(
        int id_immagine,
        int id_prodotto,
        string percorso_file,
        string descrizione,
        string data_caricamento);

    int getId_Immagine() const;

    int getId_Prodotto() const;

    string getPercorsoFile() const;

    string getDescrizione() const;

    string getDataCaricamento() const;

    void setId_Immagine(int nuovoIdImmagine);

    void setId_Prodotto(int nuovoIdProdotto);

    void setPercorsoFile(string nuovoPercorsoFile);

    void setDescrizione(string nuovaDescrizione);

    void setDataCaricamento(string nuovaDataCaricamento);
};

#endif /* Immagine_hpp */
