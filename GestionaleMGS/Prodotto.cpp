//
//  Prodotto.cpp
//  GestionaleMGS
//

#include <string>
#include <iostream>
#include <sstream>

#include "Prodotto.hpp"
#include "Filato.hpp"
#include "Cliente.hpp"
#include "Tecnico.hpp"

using namespace std;

namespace
{
    const string PERCORSO_IMMAGINI_PRODOTTI_TEMPORANEO = "C:/GestionaleMGS/ImmaginiProdotti/";
}

string Prodotto::generaPathImmagine(int articolo, int modello)
{
    ostringstream path;
    path << PERCORSO_IMMAGINI_PRODOTTI_TEMPORANEO << articolo << "-" << modello;
    return path.str();
}

int Prodotto::estraiCodiceFilatoDaArticolo(int articolo)
{
    return articolo / 100;
}

int Prodotto::estraiSerieClienteDaModello(int modello)
{
    return (modello / 100) % 10;
}

bool Prodotto::haCodiciRelazionaliCoerenti() const
{
    return articolo >= 10000 && articolo <= 99999 &&
        modello >= 10000 && modello <= 99999 &&
        estraiCodiceFilatoDaArticolo(articolo) == filato.getCodice() &&
        estraiSerieClienteDaModello(modello) == cliente.getSerie();
}

void Prodotto::aggiornaPathImmagineAutomatico()
{
    immagine.setPercorsoFile(generaPathImmagine(articolo, modello));
}

Prodotto::Prodotto(int id_prodotto,
    short stagione,
    int anno,
    int articolo,
    int modello,
    string descrizione,
    string pathImmagine,
    Filato filato,
    Cliente cliente,
    Tecnico tecnico)
    : id_prodotto(id_prodotto),
    stagione(stagione),
    anno(anno),
    articolo(articolo),
    modello(modello),
    descrizione(descrizione),
    immagine(pathImmagine.empty() ? generaPathImmagine(articolo, modello) : pathImmagine, descrizione),
    filato(filato),
    cliente(cliente),
    tecnico(tecnico)
{
}

Prodotto::Prodotto(int id_prodotto,
    short stagione,
    int anno,
    int articolo,
    int modello,
    string descrizione,
    Immagine immagine,
    Filato filato,
    Cliente cliente,
    Tecnico tecnico)
    : id_prodotto(id_prodotto),
    stagione(stagione),
    anno(anno),
    articolo(articolo),
    modello(modello),
    descrizione(descrizione),
    immagine(immagine),
    filato(filato),
    cliente(cliente),
    tecnico(tecnico)
{
    if (this->immagine.getPercorsoFile().empty())
    {
        aggiornaPathImmagineAutomatico();
    }

    this->immagine.setDescrizione(descrizione);
}

Prodotto::Prodotto(short stagione,
    int anno,
    int articolo,
    int modello,
    string descrizione,
    string pathImmagine,
    Filato filato,
    Cliente cliente,
    Tecnico tecnico)
    : stagione(stagione),
    anno(anno),
    articolo(articolo),
    modello(modello),
    descrizione(descrizione),
    immagine(pathImmagine.empty() ? generaPathImmagine(articolo, modello) : pathImmagine, descrizione),
    filato(filato),
    cliente(cliente),
    tecnico(tecnico)
{
}

Prodotto::Prodotto(short stagione,
    int anno,
    int articolo,
    int modello,
    string descrizione,
    Immagine immagine,
    Filato filato,
    Cliente cliente,
    Tecnico tecnico)
    : Prodotto(
        0,
        stagione,
        anno,
        articolo,
        modello,
        descrizione,
        immagine,
        filato,
        cliente,
        tecnico)
{
}

Prodotto::Prodotto(int id_prodotto,
    short stagione,
    int anno,
    int articolo,
    int modello,
    string descrizione,
    Filato filato,
    Cliente cliente,
    Tecnico tecnico)
    : Prodotto(
        id_prodotto,
        stagione,
        anno,
        articolo,
        modello,
        descrizione,
        generaPathImmagine(articolo, modello),
        filato,
        cliente,
        tecnico)
{
}

Prodotto::Prodotto(short stagione,
    int anno,
    int articolo,
    int modello,
    string descrizione,
    Filato filato,
    Cliente cliente,
    Tecnico tecnico)
    : Prodotto(
        stagione,
        anno,
        articolo,
        modello,
        descrizione,
        generaPathImmagine(articolo, modello),
        filato,
        cliente,
        tecnico)
{
}

//Metodi Get

int Prodotto::getId_Prodotto() const {
    return id_prodotto;
}

short Prodotto::getStagione() const {
    return stagione;
}

int Prodotto::getAnno() const {
    return anno;
}

int Prodotto::getArticolo() const {
    return articolo;
}

int Prodotto::getModello() const {
    return modello;
}

string Prodotto::getDescrizione() const {
    return descrizione;
}

string Prodotto::getPathImmagine() const {
    return immagine.getPercorsoFile();
}

const Immagine& Prodotto::getImmagine() const
{
    return immagine;
}

const Filato& Prodotto::getFilato() const
{
    return filato;
}

const Cliente& Prodotto::getCliente() const
{
    return cliente;
}

const Tecnico& Prodotto::getTecnico() const
{
    return tecnico;
}

const Trattamento& Prodotto::getTrattamento() const
{
    return trattamento;
}

//Metodi Set

void Prodotto::setStagione(short newStagione) {
    stagione = newStagione;
}

void Prodotto::setAnno(int newAnno) {
    anno = newAnno;
}

void Prodotto::setArticolo(int newArticolo) {
    articolo = newArticolo;
    aggiornaPathImmagineAutomatico();
}

void Prodotto::setModello(int newModello) {
    modello = newModello;
    aggiornaPathImmagineAutomatico();
}

void Prodotto::setDescrizione(string newDescrizione) {
    descrizione = newDescrizione;
    immagine.setDescrizione(newDescrizione);
}

void Prodotto::setPathImmagine(string newPathImmagine) {
    immagine.setPercorsoFile(newPathImmagine);
}

void Prodotto::setImmagine(const Immagine& nuovaImmagine)
{
    immagine = nuovaImmagine;
    immagine.setDescrizione(descrizione);
}

void Prodotto::setFilato(const Filato& nuovoFilato)
{
    filato = nuovoFilato;
}

void Prodotto::setCliente(const Cliente& nuovoCliente)
{
    cliente = nuovoCliente;
}

void Prodotto::setTecnico(const Tecnico& nuovoTecnico)
{
    tecnico = nuovoTecnico;
}

void Prodotto::setTrattamento(const Trattamento& nuovoTrattamento)
{
    trattamento = nuovoTrattamento;
}

//Metodi Stampa

void Prodotto::stampaProdotto() {
    cout << "Articolo: " << getArticolo() << " Modello: " << getModello() << endl;
}

void Prodotto::stampaRecordProdotto() {

    cout << "===============================" << endl;
    cout << "PRODOTTO: " << modello << "/" << articolo << endl;
    cout << "===============================" << endl;

    cout << "Stagione: ";
	if (stagione == 1)
		cout << "PE" << endl;
	else
		cout << "AI" << endl;

    cout << "Anno: " << anno << endl;
    cout << "Articolo: " << articolo << endl;
    cout << "Modello: " << modello << endl;
    cout << "Descrizione: " << descrizione << endl;

    cout << "FILATO => ";
    filato.stampaFilato();

    cout << "CLIENTE => ";
    cliente.stampaCliente();

    cout << "TECNICO => ";
    tecnico.stampaTecnico();

    cout << "TRATTAMENTO => ";
    trattamento.stampaTrattamento();
    cout << endl;

    cout << "===============================" << endl;

    cout << endl;
}

