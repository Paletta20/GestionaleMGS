#include "DatabaseManager.hpp"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <sstream>

namespace
{
    std::string nomeImmagineNormalizzato(int articolo, int modello, const std::string& nomeOriginale)
    {
        std::string estensione;
        const std::size_t separatore = nomeOriginale.find_last_of("/\\");
        const std::size_t punto = nomeOriginale.find_last_of('.');
        if (punto != std::string::npos && (separatore == std::string::npos || punto > separatore))
        {
            estensione = nomeOriginale.substr(punto);
            std::transform(estensione.begin(), estensione.end(), estensione.begin(),
                [](unsigned char carattere) { return static_cast<char>(std::tolower(carattere)); });
        }
        return std::to_string(articolo) + "-" + std::to_string(modello) + estensione;
    }

    short codiceStagioneToShort(const std::string& codice)
    {
        if (codice == "PE" || codice == "pe")
        {
            return 1;
        }

        return 2;
    }

    std::string shortToCodiceStagione(short stagione)
    {
        if (stagione == 1)
        {
            return "PE";
        }

        return "AI";
    }
}

DatabaseManager::DatabaseManager()
    : driver(nullptr)
{
}

DatabaseManager::~DatabaseManager()
{
    disconnetti();
}

bool DatabaseManager::connetti(
    const std::string& host,
    const std::string& utente,
    const std::string& password,
    const std::string& database)
{
    ultimo_errore.clear();

    try
    {
        driver = get_driver_instance();
        connection.reset(driver->connect(host, utente, password));
        connection->setSchema(database);
        return true;
    }
    catch (const sql::SQLException& err)
    {
        std::ostringstream messaggio;
        messaggio << err.what()
            << " (codice: " << err.getErrorCode()
            << ", stato SQL: " << err.getSQLState() << ")";
        ultimo_errore = messaggio.str();
    }
    catch (const std::exception& err)
    {
        ultimo_errore = err.what();
    }

    connection.reset();
    return false;
}

void DatabaseManager::disconnetti()
{
    if (connection != nullptr)
    {
        connection->close();
        connection.reset();
    }
}

bool DatabaseManager::isConnesso() const
{
    return connection != nullptr && !connection->isClosed();
}

const std::string& DatabaseManager::ultimoErrore() const
{
    return ultimo_errore;
}

bool DatabaseManager::verificaSchema(std::vector<std::string>& problemi)
{
    problemi.clear();
    ultimo_errore.clear();
    if (!isConnesso())
    {
        ultimo_errore = "Connessione DataBase non attiva.";
        problemi.push_back(ultimo_errore);
        return false;
    }

    const std::vector<std::pair<std::string, std::vector<std::string>>> schemaRichiesto = {
        { "fornitori", { "id_fornitore", "nome", "telefono", "email", "indirizzo", "nazione", "attivo" } },
        { "filati", { "id_filato", "codice", "nome", "composizione", "titolo", "id_fornitore", "attivo" } },
        { "clienti", { "id_cliente", "nome", "serie", "attivo" } },
        { "tecnici", { "id_tecnico", "cognome", "nome", "attivo" } },
        { "stagioni", { "id_stagione", "codice" } },
        { "prodotti", { "id_prodotto", "articolo", "modello", "anno", "descrizione", "id_filato", "id_cliente", "id_tecnico", "id_stagione", "id_immagine" } },
        { "immagini", { "id_immagine", "id_prodotto", "percorso_file", "descrizione", "data_caricamento", "nome_file", "mime_type", "dati_immagine" } }
    };

    try
    {
        std::unique_ptr<sql::PreparedStatement> verifica(
            connection->prepareStatement(
                "SELECT COUNT(*) AS presente FROM INFORMATION_SCHEMA.COLUMNS "
                "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = ? AND COLUMN_NAME = ?"));
        for (const auto& [tabella, colonne] : schemaRichiesto)
        {
            for (const std::string& colonna : colonne)
            {
                verifica->setString(1, tabella);
                verifica->setString(2, colonna);
                std::unique_ptr<sql::ResultSet> result(verifica->executeQuery());
                if (!result->next() || result->getInt("presente") == 0)
                    problemi.push_back("Colonna mancante: " + tabella + "." + colonna);
            }
        }

        if (problemi.empty())
        {
            std::unique_ptr<sql::Statement> statement(connection->createStatement());
            std::unique_ptr<sql::ResultSet> result(statement->executeQuery(
                "SELECT COUNT(*) AS incoerenti FROM immagini i "
                "INNER JOIN prodotti p ON p.id_immagine = i.id_immagine "
                "WHERE i.dati_immagine IS NOT NULL "
                "AND i.nome_file <> CONCAT(p.articolo, '-', p.modello) "
                "AND i.nome_file NOT LIKE CONCAT(p.articolo, '-', p.modello, '.%')"));
            if (result->next() && result->getInt("incoerenti") > 0)
                problemi.push_back("Sono presenti nomi immagine non coerenti con articolo-modello.");
        }
    }
    catch (const sql::SQLException& err)
    {
        std::ostringstream messaggio;
        messaggio << err.what() << " (codice: " << err.getErrorCode()
            << ", stato SQL: " << err.getSQLState() << ")";
        ultimo_errore = messaggio.str();
        problemi.push_back(ultimo_errore);
        return false;
    }
    return problemi.empty();
}

bool DatabaseManager::preparaArchivioImmagini()
{
    ultimo_errore.clear();
    if (!isConnesso())
    {
        ultimo_errore = "Connessione DataBase non attiva.";
        return false;
    }

    const std::vector<std::pair<std::string, std::string>> colonne = {
        { "nome_file", "ALTER TABLE immagini ADD COLUMN nome_file VARCHAR(255) NULL AFTER percorso_file" },
        { "mime_type", "ALTER TABLE immagini ADD COLUMN mime_type VARCHAR(100) NULL AFTER nome_file" },
        { "dati_immagine", "ALTER TABLE immagini ADD COLUMN dati_immagine LONGBLOB NULL AFTER mime_type" }
    };

    try
    {
        std::unique_ptr<sql::PreparedStatement> verifica(
            connection->prepareStatement(
                "SELECT COUNT(*) AS presente FROM INFORMATION_SCHEMA.COLUMNS "
                "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'immagini' AND COLUMN_NAME = ?"));
        for (const auto& [nomeColonna, queryCreazione] : colonne)
        {
            verifica->setString(1, nomeColonna);
            std::unique_ptr<sql::ResultSet> result(verifica->executeQuery());
            if (!result->next() || result->getInt("presente") == 0)
            {
                std::unique_ptr<sql::Statement> alter(connection->createStatement());
                alter->execute(queryCreazione);
            }
        }

        std::unique_ptr<sql::Statement> normalizza(connection->createStatement());
        normalizza->executeUpdate(
            "UPDATE immagini i "
            "INNER JOIN prodotti p ON p.id_immagine = i.id_immagine "
            "SET i.nome_file = CONCAT(p.articolo, '-', p.modello, "
            "CASE WHEN i.nome_file IS NOT NULL AND INSTR(i.nome_file, '.') > 0 "
            "THEN CONCAT('.', LOWER(SUBSTRING_INDEX(i.nome_file, '.', -1))) ELSE '' END), "
            "i.percorso_file = CONCAT('database://', p.articolo, '-', p.modello, "
            "CASE WHEN i.nome_file IS NOT NULL AND INSTR(i.nome_file, '.') > 0 "
            "THEN CONCAT('.', LOWER(SUBSTRING_INDEX(i.nome_file, '.', -1))) ELSE '' END) "
            "WHERE i.dati_immagine IS NOT NULL");
        return true;
    }
    catch (const sql::SQLException& err)
    {
        std::ostringstream messaggio;
        messaggio << err.what() << " (codice: " << err.getErrorCode()
            << ", stato SQL: " << err.getSQLState() << ")";
        ultimo_errore = messaggio.str();
        return false;
    }
}

int DatabaseManager::trovaFornitoreEsistente(const std::string& nome)
{
    ultimo_errore.clear();

    try
    {
        std::unique_ptr<sql::PreparedStatement> cerca(
            connection->prepareStatement(
                "SELECT id_fornitore FROM fornitori WHERE nome = ? AND attivo = 1"));
        cerca->setString(1, nome);

        std::unique_ptr<sql::ResultSet> result(cerca->executeQuery());
        if (result->next())
        {
            return result->getInt("id_fornitore");
        }

        ultimo_errore = "Fornitore non esistente o non attivo. Scegliere un fornitore gia' presente nel database.";
        return 0;
    }
    catch (const sql::SQLException& err)
    {
        std::ostringstream messaggio;
        messaggio << err.what()
            << " (codice: " << err.getErrorCode()
            << ", stato SQL: " << err.getSQLState() << ")";
        ultimo_errore = messaggio.str();
        return 0;
    }
}

bool DatabaseManager::eseguiQuery(const std::string& query)
{
    ultimo_errore.clear();

    if (!isConnesso())
    {
        ultimo_errore = "Connessione DataBase non attiva.";
        return false;
    }

    try
    {
        std::unique_ptr<sql::Statement> statement(connection->createStatement());
        statement->execute(query);
        return true;
    }
    catch (const sql::SQLException& err)
    {
        std::ostringstream messaggio;
        messaggio << err.what()
            << " (codice: " << err.getErrorCode()
            << ", stato SQL: " << err.getSQLState() << ")";
        ultimo_errore = messaggio.str();
        return false;
    }
}

std::unique_ptr<sql::ResultSet> DatabaseManager::eseguiSelect(const std::string& query)
{
    ultimo_errore.clear();

    if (!isConnesso())
    {
        ultimo_errore = "Connessione DataBase non attiva.";
        return nullptr;
    }

    try
    {
        std::unique_ptr<sql::Statement> statement(connection->createStatement());
        return std::unique_ptr<sql::ResultSet>(statement->executeQuery(query));
    }
    catch (const sql::SQLException& err)
    {
        std::ostringstream messaggio;
        messaggio << err.what()
            << " (codice: " << err.getErrorCode()
            << ", stato SQL: " << err.getSQLState() << ")";
        ultimo_errore = messaggio.str();
        return nullptr;
    }
}

std::vector<Cliente> DatabaseManager::caricaClienti()
{
    std::vector<Cliente> clienti;
    std::unique_ptr<sql::ResultSet> result =
        eseguiSelect(
            "SELECT id_cliente, nome, serie "
            "FROM clienti "
            "WHERE attivo = 1 "
            "ORDER BY id_cliente");

    if (result == nullptr)
    {
        return clienti;
    }

    while (result->next())
    {
        clienti.emplace_back(
            result->getInt("id_cliente"),
            result->getString("nome"),
            result->getInt("serie"));
    }

    return clienti;
}

std::vector<Filato> DatabaseManager::caricaFilati()
{
    std::vector<Filato> filati;
    std::unique_ptr<sql::ResultSet> result =
        eseguiSelect(
            "SELECT fi.codice, fi.nome, fi.composizione, fi.titolo, fo.nome AS fornitore "
            "FROM filati fi "
            "LEFT JOIN fornitori fo ON fo.id_fornitore = fi.id_fornitore "
            "WHERE fi.attivo = 1 "
            "ORDER BY fi.id_filato");

    if (result == nullptr)
    {
        return filati;
    }

    while (result->next())
    {
        filati.emplace_back(
            result->getInt("codice"),
            result->getString("nome"),
            result->getString("composizione"),
            result->getString("titolo"),
            result->getString("fornitore"));
    }

    return filati;
}

std::vector<Fornitore> DatabaseManager::caricaFornitori()
{
    std::vector<Fornitore> fornitori;
    std::unique_ptr<sql::ResultSet> result =
        eseguiSelect(
            "SELECT id_fornitore, nome, telefono, email, indirizzo, nazione, attivo "
            "FROM fornitori "
            "WHERE attivo = 1 "
            "ORDER BY id_fornitore");

    if (result == nullptr)
    {
        return fornitori;
    }

    while (result->next())
    {
        fornitori.emplace_back(
            result->getInt("id_fornitore"),
            result->getString("nome"),
            result->getString("telefono"),
            result->getString("email"),
            result->getString("indirizzo"),
            result->getString("nazione"),
            result->getInt("attivo"));
    }

    return fornitori;
}

std::vector<Tecnico> DatabaseManager::caricaTecnici()
{
    std::vector<Tecnico> tecnici;
    std::unique_ptr<sql::ResultSet> result =
        eseguiSelect(
            "SELECT id_tecnico, cognome, nome "
            "FROM tecnici "
            "WHERE attivo = 1 "
            "ORDER BY id_tecnico");

    if (result == nullptr)
    {
        return tecnici;
    }

    while (result->next())
    {
        tecnici.emplace_back(
            result->getInt("id_tecnico"),
            result->getString("cognome"),
            result->getString("nome"));
    }

    return tecnici;
}

std::vector<Prodotto> DatabaseManager::caricaProdotti()
{
    std::vector<Prodotto> prodotti;
    std::unique_ptr<sql::ResultSet> result =
        eseguiSelect(
            "SELECT "
            "p.id_prodotto, p.articolo, p.modello, p.anno, p.descrizione AS descrizione_prodotto, "
            "i.id_immagine, i.id_prodotto AS id_prodotto_immagine, "
            "i.percorso_file AS path_immagine, i.descrizione AS descrizione_immagine, "
            "i.data_caricamento, "
            "s.codice AS stagione, "
            "fi.codice AS codice_filato, fi.nome AS nome_filato, fi.composizione AS composizione_filato, "
            "fi.titolo AS titolo_filato, fo.nome AS nome_fornitore, "
            "c.id_cliente, c.nome AS nome_cliente, c.serie AS serie_cliente, "
            "t.id_tecnico, t.cognome AS tecnico_cognome, t.nome AS tecnico_nome "
            "FROM prodotti p "
            "INNER JOIN stagioni s ON s.id_stagione = p.id_stagione "
            "INNER JOIN filati fi ON fi.id_filato = p.id_filato "
            "LEFT JOIN fornitori fo ON fo.id_fornitore = fi.id_fornitore "
            "INNER JOIN clienti c ON c.id_cliente = p.id_cliente "
            "INNER JOIN tecnici t ON t.id_tecnico = p.id_tecnico "
            "LEFT JOIN immagini i ON i.id_immagine = p.id_immagine "
            "ORDER BY p.id_prodotto");

    if (result == nullptr)
    {
        return prodotti;
    }

    while (result->next())
    {
        Filato filato(
            result->getInt("codice_filato"),
            result->getString("nome_filato"),
            result->getString("composizione_filato"),
            result->getString("titolo_filato"),
            result->getString("nome_fornitore"));

        Cliente cliente(
            result->getInt("id_cliente"),
            result->getString("nome_cliente"),
            result->getInt("serie_cliente"));

        Tecnico tecnico(
            result->getInt("id_tecnico"),
            result->getString("tecnico_cognome"),
            result->getString("tecnico_nome"));

        Immagine immagine(
            result->getInt("id_immagine"),
            result->getInt("id_prodotto_immagine"),
            result->getString("path_immagine"),
            result->getString("descrizione_immagine"),
            result->getString("data_caricamento"));

        prodotti.emplace_back(
            result->getInt("id_prodotto"),
            codiceStagioneToShort(result->getString("stagione")),
            result->getInt("anno"),
            result->getInt("articolo"),
            result->getInt("modello"),
            result->getString("descrizione_prodotto"),
            immagine,
            filato,
            cliente,
            tecnico);
    }

    return prodotti;
}

bool DatabaseManager::inserisciCliente(const Cliente& cliente)
{
    ultimo_errore.clear();

    if (!isConnesso())
    {
        ultimo_errore = "Connessione DataBase non attiva.";
        return false;
    }

    try
    {
        std::unique_ptr<sql::PreparedStatement> statement(
            connection->prepareStatement(
                "INSERT INTO clienti (nome, serie, attivo) VALUES (?, ?, 1)"));
        statement->setString(1, cliente.getNome());
        statement->setInt(2, cliente.getSerie());
        statement->executeUpdate();
        return true;
    }
    catch (const sql::SQLException& err)
    {
        std::ostringstream messaggio;
        messaggio << err.what()
            << " (codice: " << err.getErrorCode()
            << ", stato SQL: " << err.getSQLState() << ")";
        ultimo_errore = messaggio.str();
        return false;
    }
}

bool DatabaseManager::eliminaCliente(int serie)
{
    ultimo_errore.clear();

    if (!isConnesso())
    {
        ultimo_errore = "Connessione DataBase non attiva.";
        return false;
    }

    try
    {
        std::unique_ptr<sql::PreparedStatement> statement(
            connection->prepareStatement("UPDATE clienti SET attivo = 0 WHERE serie = ?"));
        statement->setInt(1, serie);
        return statement->executeUpdate() > 0;
    }
    catch (const sql::SQLException& err)
    {
        std::ostringstream messaggio;
        messaggio << err.what()
            << " (codice: " << err.getErrorCode()
            << ", stato SQL: " << err.getSQLState() << ")";
        ultimo_errore = messaggio.str();
        return false;
    }
}

bool DatabaseManager::aggiornaCliente(int vecchiaSerie, const Cliente& cliente)
{
    ultimo_errore.clear();

    if (!isConnesso())
    {
        ultimo_errore = "Connessione DataBase non attiva.";
        return false;
    }

    try
    {
        std::unique_ptr<sql::PreparedStatement> statement(
            connection->prepareStatement(
                "UPDATE clienti SET nome = ?, serie = ? WHERE serie = ? AND attivo = 1"));
        statement->setString(1, cliente.getNome());
        statement->setInt(2, cliente.getSerie());
        statement->setInt(3, vecchiaSerie);
        return statement->executeUpdate() > 0;
    }
    catch (const sql::SQLException& err)
    {
        std::ostringstream messaggio;
        messaggio << err.what()
            << " (codice: " << err.getErrorCode()
            << ", stato SQL: " << err.getSQLState() << ")";
        ultimo_errore = messaggio.str();
        return false;
    }
}

bool DatabaseManager::inserisciFilato(const Filato& filato)
{
    ultimo_errore.clear();

    if (!isConnesso())
    {
        ultimo_errore = "Connessione DataBase non attiva.";
        return false;
    }

    int idFornitore = trovaFornitoreEsistente(filato.getFornitore());
    if (idFornitore == 0)
    {
        return false;
    }

    try
    {
        std::unique_ptr<sql::PreparedStatement> statement(
            connection->prepareStatement(
                "INSERT INTO filati (codice, nome, composizione, titolo, id_fornitore, attivo) "
                "VALUES (?, ?, ?, ?, ?, 1)"));
        statement->setInt(1, filato.getCodice());
        statement->setString(2, filato.getNome());
        statement->setString(3, filato.getComposizione());
        statement->setString(4, filato.getTitolo());
        statement->setInt(5, idFornitore);
        statement->executeUpdate();
        return true;
    }
    catch (const sql::SQLException& err)
    {
        std::ostringstream messaggio;
        messaggio << err.what()
            << " (codice: " << err.getErrorCode()
            << ", stato SQL: " << err.getSQLState() << ")";
        ultimo_errore = messaggio.str();
        return false;
    }
}

bool DatabaseManager::aggiornaFilato(int codice, const Filato& filato)
{
    ultimo_errore.clear();

    if (!isConnesso())
    {
        ultimo_errore = "Connessione DataBase non attiva.";
        return false;
    }

    int idFornitore = trovaFornitoreEsistente(filato.getFornitore());
    if (idFornitore == 0)
    {
        return false;
    }

    try
    {
        std::unique_ptr<sql::PreparedStatement> statement(
            connection->prepareStatement(
                "UPDATE filati "
                "SET nome = ?, composizione = ?, titolo = ?, id_fornitore = ? "
                "WHERE codice = ? AND attivo = 1"));
        statement->setString(1, filato.getNome());
        statement->setString(2, filato.getComposizione());
        statement->setString(3, filato.getTitolo());
        statement->setInt(4, idFornitore);
        statement->setInt(5, codice);
        return statement->executeUpdate() > 0;
    }
    catch (const sql::SQLException& err)
    {
        std::ostringstream messaggio;
        messaggio << err.what()
            << " (codice: " << err.getErrorCode()
            << ", stato SQL: " << err.getSQLState() << ")";
        ultimo_errore = messaggio.str();
        return false;
    }
}

bool DatabaseManager::eliminaFilato(int codice)
{
    ultimo_errore.clear();

    if (!isConnesso())
    {
        ultimo_errore = "Connessione DataBase non attiva.";
        return false;
    }

    try
    {
        std::unique_ptr<sql::PreparedStatement> statement(
            connection->prepareStatement("UPDATE filati SET attivo = 0 WHERE codice = ?"));
        statement->setInt(1, codice);
        return statement->executeUpdate() > 0;
    }
    catch (const sql::SQLException& err)
    {
        std::ostringstream messaggio;
        messaggio << err.what()
            << " (codice: " << err.getErrorCode()
            << ", stato SQL: " << err.getSQLState() << ")";
        ultimo_errore = messaggio.str();
        return false;
    }
}

bool DatabaseManager::inserisciFornitore(const Fornitore& fornitore)
{
    ultimo_errore.clear();

    if (!isConnesso())
    {
        ultimo_errore = "Connessione MySQL non attiva.";
        return false;
    }

    try
    {
        std::unique_ptr<sql::PreparedStatement> statement(
            connection->prepareStatement(
                "INSERT INTO fornitori "
                "(nome, telefono, email, indirizzo, nazione, attivo) "
                "VALUES (?, ?, ?, ?, ?, ?)"));
        statement->setString(1, fornitore.getNome());
        statement->setString(2, fornitore.getTelefono());
        statement->setString(3, fornitore.getEmail());
        statement->setString(4, fornitore.getIndirizzo());
        statement->setString(5, fornitore.getNazione());
        statement->setInt(6, fornitore.getAttivo());
        statement->executeUpdate();
        return true;
    }
    catch (const sql::SQLException& err)
    {
        std::ostringstream messaggio;
        messaggio << err.what()
            << " (codice: " << err.getErrorCode()
            << ", stato SQL: " << err.getSQLState() << ")";
        ultimo_errore = messaggio.str();
        return false;
    }
}

bool DatabaseManager::aggiornaFornitore(int idFornitore, const Fornitore& fornitore)
{
    ultimo_errore.clear();

    if (!isConnesso())
    {
        ultimo_errore = "Connessione MySQL non attiva.";
        return false;
    }

    try
    {
        std::unique_ptr<sql::PreparedStatement> statement(
            connection->prepareStatement(
                "UPDATE fornitori "
                "SET nome = ?, telefono = ?, email = ?, indirizzo = ?, nazione = ? "
                "WHERE id_fornitore = ? AND attivo = 1"));
        statement->setString(1, fornitore.getNome());
        statement->setString(2, fornitore.getTelefono());
        statement->setString(3, fornitore.getEmail());
        statement->setString(4, fornitore.getIndirizzo());
        statement->setString(5, fornitore.getNazione());
        statement->setInt(6, idFornitore);
        return statement->executeUpdate() > 0;
    }
    catch (const sql::SQLException& err)
    {
        std::ostringstream messaggio;
        messaggio << err.what()
            << " (codice: " << err.getErrorCode()
            << ", stato SQL: " << err.getSQLState() << ")";
        ultimo_errore = messaggio.str();
        return false;
    }
}

bool DatabaseManager::eliminaFornitore(int idFornitore)
{
    ultimo_errore.clear();

    if (!isConnesso())
    {
        ultimo_errore = "Connessione MySQL non attiva.";
        return false;
    }

    try
    {
        std::unique_ptr<sql::PreparedStatement> statement(
            connection->prepareStatement("UPDATE fornitori SET attivo = 0 WHERE id_fornitore = ?"));
        statement->setInt(1, idFornitore);
        return statement->executeUpdate() > 0;
    }
    catch (const sql::SQLException& err)
    {
        std::ostringstream messaggio;
        messaggio << err.what()
            << " (codice: " << err.getErrorCode()
            << ", stato SQL: " << err.getSQLState() << ")";
        ultimo_errore = messaggio.str();
        return false;
    }
}

bool DatabaseManager::aggiornaTecnico(
    const std::string& vecchioCognome,
    const std::string& vecchioNome,
    const Tecnico& tecnico)
{
    ultimo_errore.clear();

    if (!isConnesso())
    {
        ultimo_errore = "Connessione DataBase non attiva.";
        return false;
    }

    try
    {
        std::unique_ptr<sql::PreparedStatement> statement(
            connection->prepareStatement(
                "UPDATE tecnici "
                "SET cognome = ?, nome = ? "
                "WHERE cognome = ? AND nome = ? AND attivo = 1"));
        statement->setString(1, tecnico.getCognome());
        statement->setString(2, tecnico.getNome());
        statement->setString(3, vecchioCognome);
        statement->setString(4, vecchioNome);
        return statement->executeUpdate() > 0;
    }
    catch (const sql::SQLException& err)
    {
        std::ostringstream messaggio;
        messaggio << err.what()
            << " (codice: " << err.getErrorCode()
            << ", stato SQL: " << err.getSQLState() << ")";
        ultimo_errore = messaggio.str();
        return false;
    }
}

bool DatabaseManager::inserisciTecnico(const Tecnico& tecnico)
{
    ultimo_errore.clear();

    if (!isConnesso())
    {
        ultimo_errore = "Connessione DataBase non attiva.";
        return false;
    }

    try
    {
        std::unique_ptr<sql::PreparedStatement> statement(
            connection->prepareStatement(
                "INSERT INTO tecnici (cognome, nome, attivo) VALUES (?, ?, 1)"));
        statement->setString(1, tecnico.getCognome());
        statement->setString(2, tecnico.getNome());
        statement->executeUpdate();
        return true;
    }
    catch (const sql::SQLException& err)
    {
        std::ostringstream messaggio;
        messaggio << err.what()
            << " (codice: " << err.getErrorCode()
            << ", stato SQL: " << err.getSQLState() << ")";
        ultimo_errore = messaggio.str();
        return false;
    }
}

bool DatabaseManager::eliminaTecnico(const std::string& cognome, const std::string& nome)
{
    ultimo_errore.clear();

    if (!isConnesso())
    {
        ultimo_errore = "Connessione DataBase non attiva.";
        return false;
    }

    try
    {
        std::unique_ptr<sql::PreparedStatement> statement(
            connection->prepareStatement(
                "UPDATE tecnici SET attivo = 0 WHERE cognome = ? AND nome = ?"));
        statement->setString(1, cognome);
        statement->setString(2, nome);
        return statement->executeUpdate() > 0;
    }
    catch (const sql::SQLException& err)
    {
        std::ostringstream messaggio;
        messaggio << err.what()
            << " (codice: " << err.getErrorCode()
            << ", stato SQL: " << err.getSQLState() << ")";
        ultimo_errore = messaggio.str();
        return false;
    }
}

bool DatabaseManager::inserisciProdotto(const Prodotto& prodotto)
{
    ultimo_errore.clear();

    if (!isConnesso())
    {
        ultimo_errore = "Connessione DataBase non attiva.";
        return false;
    }

    if (!prodotto.haCodiciRelazionaliCoerenti())
    {
        ultimo_errore = "Codici prodotto non coerenti: le prime tre cifre dell'articolo devono "
            "corrispondere al codice filato e la cifra delle centinaia del modello alla serie cliente.";
        return false;
    }

    try
    {
        connection->setAutoCommit(false);
        const Immagine& immagine = prodotto.getImmagine();

        std::unique_ptr<sql::PreparedStatement> immagineStatement(
            connection->prepareStatement(
                "INSERT INTO immagini "
                "(id_prodotto, percorso_file, descrizione, data_caricamento) "
                "VALUES (0, ?, ?, NOW(6))"));
        immagineStatement->setString(1, immagine.getPercorsoFile());
        immagineStatement->setString(2, immagine.getDescrizione());
        immagineStatement->executeUpdate();

        std::unique_ptr<sql::Statement> idStatement(connection->createStatement());
        std::unique_ptr<sql::ResultSet> idImmagineResult(
            idStatement->executeQuery("SELECT LAST_INSERT_ID() AS id_immagine"));
        if (!idImmagineResult->next())
        {
            ultimo_errore = "Impossibile leggere id_immagine appena inserito.";
            connection->rollback();
            connection->setAutoCommit(true);
            return false;
        }

        int idImmagine = idImmagineResult->getInt("id_immagine");

        std::unique_ptr<sql::PreparedStatement> statement(
            connection->prepareStatement(
                "INSERT INTO prodotti "
                "(articolo, modello, anno, descrizione, id_filato, id_cliente, id_tecnico, id_stagione, id_immagine) "
                "SELECT ?, ?, ?, ?, fi.id_filato, c.id_cliente, t.id_tecnico, s.id_stagione, ? "
                "FROM filati fi "
                "INNER JOIN clienti c ON c.serie = ? AND c.attivo = 1 "
                "INNER JOIN tecnici t ON t.cognome = ? AND t.nome = ? AND t.attivo = 1 "
                "INNER JOIN stagioni s ON s.codice = ? "
                "WHERE fi.codice = ? AND fi.attivo = 1"));
        statement->setInt(1, prodotto.getArticolo());
        statement->setInt(2, prodotto.getModello());
        statement->setInt(3, prodotto.getAnno());
        statement->setString(4, prodotto.getDescrizione());
        statement->setInt(5, idImmagine);
        statement->setInt(6, prodotto.getCliente().getSerie());
        statement->setString(7, prodotto.getTecnico().getCognome());
        statement->setString(8, prodotto.getTecnico().getNome());
        statement->setString(9, shortToCodiceStagione(prodotto.getStagione()));
        statement->setInt(10, prodotto.getFilato().getCodice());

        if (statement->executeUpdate() == 0)
        {
            ultimo_errore = "Inserimento prodotto non riuscito: filato, cliente, tecnico o stagione non trovati.";
            connection->rollback();
            connection->setAutoCommit(true);
            return false;
        }

        std::unique_ptr<sql::ResultSet> idProdottoResult(
            idStatement->executeQuery("SELECT LAST_INSERT_ID() AS id_prodotto"));
        if (!idProdottoResult->next())
        {
            ultimo_errore = "Impossibile leggere id_prodotto appena inserito.";
            connection->rollback();
            connection->setAutoCommit(true);
            return false;
        }

        int idProdotto = idProdottoResult->getInt("id_prodotto");

        std::unique_ptr<sql::PreparedStatement> aggiornaImmagine(
            connection->prepareStatement(
                "UPDATE immagini SET id_prodotto = ? WHERE id_immagine = ?"));
        aggiornaImmagine->setInt(1, idProdotto);
        aggiornaImmagine->setInt(2, idImmagine);
        aggiornaImmagine->executeUpdate();

        connection->commit();
        connection->setAutoCommit(true);
        return true;
    }
    catch (const sql::SQLException& err)
    {
        try
        {
            if (connection != nullptr)
            {
                connection->rollback();
                connection->setAutoCommit(true);
            }
        }
        catch (...)
        {
        }

        std::ostringstream messaggio;
        messaggio << err.what()
            << " (codice: " << err.getErrorCode()
            << ", stato SQL: " << err.getSQLState() << ")";
        ultimo_errore = messaggio.str();
        return false;
    }
}

bool DatabaseManager::aggiornaProdotto(int vecchioArticolo, int vecchioModello, const Prodotto& prodotto)
{
    ultimo_errore.clear();

    if (!isConnesso())
    {
        ultimo_errore = "Connessione DataBase non attiva.";
        return false;
    }

    if (!prodotto.haCodiciRelazionaliCoerenti())
    {
        ultimo_errore = "Codici prodotto non coerenti: le prime tre cifre dell'articolo devono "
            "corrispondere al codice filato e la cifra delle centinaia del modello alla serie cliente.";
        return false;
    }

    try
    {
        std::unique_ptr<sql::PreparedStatement> statement(
            connection->prepareStatement(
                "UPDATE prodotti p "
                "INNER JOIN filati fi ON fi.codice = ? AND fi.attivo = 1 "
                "INNER JOIN clienti c ON c.serie = ? AND c.attivo = 1 "
                "INNER JOIN tecnici t ON t.cognome = ? AND t.nome = ? AND t.attivo = 1 "
                "INNER JOIN stagioni s ON s.codice = ? "
                "LEFT JOIN immagini i ON i.id_immagine = p.id_immagine "
                "SET p.articolo = ?, p.modello = ?, p.anno = ?, p.descrizione = ?, "
                "p.id_filato = fi.id_filato, p.id_cliente = c.id_cliente, "
                "p.id_tecnico = t.id_tecnico, p.id_stagione = s.id_stagione, "
                "i.percorso_file = ?, i.descrizione = ? "
                "WHERE p.articolo = ? AND p.modello = ?"));
        statement->setInt(1, prodotto.getFilato().getCodice());
        statement->setInt(2, prodotto.getCliente().getSerie());
        statement->setString(3, prodotto.getTecnico().getCognome());
        statement->setString(4, prodotto.getTecnico().getNome());
        statement->setString(5, shortToCodiceStagione(prodotto.getStagione()));
        statement->setInt(6, prodotto.getArticolo());
        statement->setInt(7, prodotto.getModello());
        statement->setInt(8, prodotto.getAnno());
        statement->setString(9, prodotto.getDescrizione());
        statement->setString(10, prodotto.getImmagine().getPercorsoFile());
        statement->setString(11, prodotto.getImmagine().getDescrizione());
        statement->setInt(12, vecchioArticolo);
        statement->setInt(13, vecchioModello);
        return statement->executeUpdate() > 0;
    }
    catch (const sql::SQLException& err)
    {
        std::ostringstream messaggio;
        messaggio << err.what()
            << " (codice: " << err.getErrorCode()
            << ", stato SQL: " << err.getSQLState() << ")";
        ultimo_errore = messaggio.str();
        return false;
    }
}
bool DatabaseManager::eliminaProdotto(int articolo, int modello)
{
    ultimo_errore.clear();

    if (!isConnesso())
    {
        ultimo_errore = "Connessione DataBase non attiva.";
        return false;
    }

    try
    {
        connection->setAutoCommit(false);

        std::unique_ptr<sql::PreparedStatement> cercaImmagine(
            connection->prepareStatement(
                "SELECT id_immagine FROM prodotti WHERE articolo = ? AND modello = ?"));
        cercaImmagine->setInt(1, articolo);
        cercaImmagine->setInt(2, modello);

        std::unique_ptr<sql::ResultSet> immagineResult(cercaImmagine->executeQuery());
        if (!immagineResult->next())
        {
            connection->rollback();
            connection->setAutoCommit(true);
            return false;
        }

        int idImmagine = immagineResult->getInt("id_immagine");

        std::unique_ptr<sql::PreparedStatement> eliminaProdotto(
            connection->prepareStatement("DELETE FROM prodotti WHERE articolo = ? AND modello = ?"));
        eliminaProdotto->setInt(1, articolo);
        eliminaProdotto->setInt(2, modello);

        if (eliminaProdotto->executeUpdate() == 0)
        {
            connection->rollback();
            connection->setAutoCommit(true);
            return false;
        }

        std::unique_ptr<sql::PreparedStatement> eliminaImmagine(
            connection->prepareStatement("DELETE FROM immagini WHERE id_immagine = ?"));
        eliminaImmagine->setInt(1, idImmagine);
        eliminaImmagine->executeUpdate();

        connection->commit();
        connection->setAutoCommit(true);
        return true;
    }
    catch (const sql::SQLException& err)
    {
        try
        {
            if (connection != nullptr)
            {
                connection->rollback();
                connection->setAutoCommit(true);
            }
        }
        catch (...)
        {
        }

        std::ostringstream messaggio;
        messaggio << err.what()
            << " (codice: " << err.getErrorCode()
            << ", stato SQL: " << err.getSQLState() << ")";
        ultimo_errore = messaggio.str();
        return false;
    }
}

bool DatabaseManager::salvaImmagineProdotto(
    int articolo,
    int modello,
    const std::string& nomeFile,
    const std::string& mimeType,
    const std::vector<unsigned char>& dati)
{
    ultimo_errore.clear();
    if (!isConnesso())
    {
        ultimo_errore = "Connessione DataBase non attiva.";
        return false;
    }
    if (dati.empty())
    {
        ultimo_errore = "Il file immagine selezionato e' vuoto.";
        return false;
    }

    try
    {
        const std::string nomeArchivio = nomeImmagineNormalizzato(articolo, modello, nomeFile);
        const std::string contenuto(reinterpret_cast<const char*>(dati.data()), dati.size());
        std::istringstream flusso(contenuto);
        std::unique_ptr<sql::PreparedStatement> statement(
            connection->prepareStatement(
                "UPDATE immagini i "
                "INNER JOIN prodotti p ON p.id_immagine = i.id_immagine "
                "SET i.nome_file = ?, i.mime_type = ?, i.dati_immagine = ?, "
                "i.percorso_file = ?, i.data_caricamento = NOW(6) "
                "WHERE p.articolo = ? AND p.modello = ?"));
        statement->setString(1, nomeArchivio);
        statement->setString(2, mimeType);
        statement->setBlob(3, &flusso);
        statement->setString(4, "database://" + nomeArchivio);
        statement->setInt(5, articolo);
        statement->setInt(6, modello);
        if (statement->executeUpdate() == 0)
        {
            ultimo_errore = "Prodotto o record immagine associato non trovato.";
            return false;
        }
        return true;
    }
    catch (const sql::SQLException& err)
    {
        std::ostringstream messaggio;
        messaggio << err.what() << " (codice: " << err.getErrorCode()
            << ", stato SQL: " << err.getSQLState() << ")";
        ultimo_errore = messaggio.str();
        return false;
    }
}

bool DatabaseManager::caricaImmagineProdotto(
    int articolo,
    int modello,
    std::string& nomeFile,
    std::string& mimeType,
    std::vector<unsigned char>& dati)
{
    ultimo_errore.clear();
    nomeFile.clear();
    mimeType.clear();
    dati.clear();
    if (!isConnesso())
    {
        ultimo_errore = "Connessione DataBase non attiva.";
        return false;
    }

    try
    {
        std::unique_ptr<sql::PreparedStatement> statement(
            connection->prepareStatement(
                "SELECT i.nome_file, i.mime_type, i.dati_immagine "
                "FROM prodotti p INNER JOIN immagini i ON i.id_immagine = p.id_immagine "
                "WHERE p.articolo = ? AND p.modello = ?"));
        statement->setInt(1, articolo);
        statement->setInt(2, modello);
        std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
        if (!result->next())
        {
            ultimo_errore = "Record immagine associato al prodotto non trovato.";
            return false;
        }
        if (result->isNull("dati_immagine")) return true;

        nomeFile = result->isNull("nome_file") ? "" : result->getString("nome_file");
        mimeType = result->isNull("mime_type") ? "" : result->getString("mime_type");
        std::unique_ptr<std::istream> flusso(result->getBlob("dati_immagine"));
        char carattere = 0;
        while (flusso != nullptr && flusso->get(carattere))
            dati.push_back(static_cast<unsigned char>(carattere));
        return true;
    }
    catch (const sql::SQLException& err)
    {
        std::ostringstream messaggio;
        messaggio << err.what() << " (codice: " << err.getErrorCode()
            << ", stato SQL: " << err.getSQLState() << ")";
        ultimo_errore = messaggio.str();
        return false;
    }
}




