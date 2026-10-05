#ifndef DatabaseManager_hpp
#define DatabaseManager_hpp

#include <memory>
#include <string>
#include <vector>

#include <mysql/jdbc.h>

#include "Cliente.hpp"
#include "Filato.hpp"
#include "Fornitore.hpp"
#include "Immagine.hpp"
#include "Prodotto.hpp"
#include "Tecnico.hpp"
#include "Trattamento.hpp"

class DatabaseManager
{
private:
    sql::Driver* driver;
    std::unique_ptr<sql::Connection> connection;
    std::string ultimo_errore;

    int trovaFornitoreEsistente(const std::string& nome);

public:
    DatabaseManager();
    ~DatabaseManager();

    bool connetti(
        const std::string& host,
        const std::string& utente,
        const std::string& password,
        const std::string& database);

    void disconnetti();

    bool isConnesso() const;
    const std::string& ultimoErrore() const;
    bool verificaSchema(std::vector<std::string>& problemi);
    bool preparaArchivioImmagini();
    bool preparaArchivioTrattamenti();

    bool eseguiQuery(const std::string& query);
    std::unique_ptr<sql::ResultSet> eseguiSelect(const std::string& query);

    std::vector<Cliente> caricaClienti();
    std::vector<Filato> caricaFilati();
    std::vector<Fornitore> caricaFornitori();
    std::vector<Tecnico> caricaTecnici();
    std::vector<Trattamento> caricaTrattamenti();
    std::vector<Prodotto> caricaProdotti();

    bool inserisciCliente(const Cliente& cliente);
    bool aggiornaCliente(int vecchiaSerie, const Cliente& cliente);
    bool eliminaCliente(int serie);

    bool inserisciFilato(const Filato& filato);
    bool aggiornaFilato(int codice, const Filato& filato);
    bool eliminaFilato(int codice);

    bool inserisciFornitore(const Fornitore& fornitore);
    bool aggiornaFornitore(int idFornitore, const Fornitore& fornitore);
    bool eliminaFornitore(int idFornitore);

    bool inserisciTecnico(const Tecnico& tecnico);
    bool aggiornaTecnico(
        const std::string& vecchioCognome,
        const std::string& vecchioNome,
        const Tecnico& tecnico);
    bool eliminaTecnico(const std::string& cognome, const std::string& nome);

    bool inserisciTrattamento(const Trattamento& trattamento);
    bool aggiornaTrattamento(int idTrattamento, const Trattamento& trattamento);
    bool eliminaTrattamento(int idTrattamento);

    bool inserisciProdotto(const Prodotto& prodotto);
    bool aggiornaProdotto(int vecchioArticolo, int vecchioModello, const Prodotto& prodotto);
    bool eliminaProdotto(int articolo, int modello);
    bool salvaImmagineProdotto(
        int articolo,
        int modello,
        const std::string& nomeFile,
        const std::string& mimeType,
        const std::vector<unsigned char>& dati);
    bool caricaImmagineProdotto(
        int articolo,
        int modello,
        std::string& nomeFile,
        std::string& mimeType,
        std::vector<unsigned char>& dati);
};

#endif


