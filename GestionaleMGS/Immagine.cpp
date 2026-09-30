//
//  Immagine.cpp
//  GestionaleMGS
//

#include "Immagine.hpp"

Immagine::Immagine()
    : id_immagine(0),
    id_prodotto(0),
    percorso_file(""),
    descrizione(""),
    data_caricamento("")
{
}

Immagine::Immagine(
    string percorso_file,
    string descrizione)
    : id_immagine(0),
    id_prodotto(0),
    percorso_file(percorso_file),
    descrizione(descrizione),
    data_caricamento("")
{
}

Immagine::Immagine(
    int id_immagine,
    int id_prodotto,
    string percorso_file,
    string descrizione,
    string data_caricamento)
    : id_immagine(id_immagine),
    id_prodotto(id_prodotto),
    percorso_file(percorso_file),
    descrizione(descrizione),
    data_caricamento(data_caricamento)
{
}

int Immagine::getId_Immagine() const {
    return id_immagine;
}

int Immagine::getId_Prodotto() const {
    return id_prodotto;
}

string Immagine::getPercorsoFile() const {
    return percorso_file;
}

string Immagine::getDescrizione() const {
    return descrizione;
}

string Immagine::getDataCaricamento() const {
    return data_caricamento;
}

void Immagine::setId_Immagine(int nuovoIdImmagine) {
    id_immagine = nuovoIdImmagine;
}

void Immagine::setId_Prodotto(int nuovoIdProdotto) {
    id_prodotto = nuovoIdProdotto;
}

void Immagine::setPercorsoFile(string nuovoPercorsoFile) {
    percorso_file = nuovoPercorsoFile;
}

void Immagine::setDescrizione(string nuovaDescrizione) {
    descrizione = nuovaDescrizione;
}

void Immagine::setDataCaricamento(string nuovaDataCaricamento) {
    data_caricamento = nuovaDataCaricamento;
}
