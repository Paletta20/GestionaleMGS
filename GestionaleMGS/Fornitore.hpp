#ifndef Fornitore_hpp
#define Fornitore_hpp

#include <string>

using namespace std;

class Fornitore
{
private:
    int id_fornitore;
    string nome;
    string telefono;
    string email;
    string indirizzo;
    string nazione;
    int attivo;

public:
    Fornitore(
        int id_fornitore,
        string nome,
        string telefono,
        string email,
        string indirizzo,
        string nazione,
        int attivo = 1);

    Fornitore(
        string nome,
        string telefono,
        string email,
        string indirizzo,
        string nazione,
        int attivo = 1);

    int getId_Fornitore() const;
    string getNome() const;
    string getTelefono() const;
    string getEmail() const;
    string getIndirizzo() const;
    string getNazione() const;
    int getAttivo() const;

    void setNome(string newNome);
    void setTelefono(string newTelefono);
    void setEmail(string newEmail);
    void setIndirizzo(string newIndirizzo);
    void setNazione(string newNazione);
    void setAttivo(int newAttivo);

    void stampaFornitore();
    void stampaRecordFornitore();
};

#endif
