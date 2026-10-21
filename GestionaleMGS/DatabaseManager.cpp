#include "DatabaseManager.hpp"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <sstream>

namespace
{
    std::string estensioneImmagine(const std::string& nomeOriginale)
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
        return estensione;
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
    struct IndiceProdotti
    {
        std::string nome;
        std::string colonne;
    };

    std::vector<IndiceProdotti> indiciUnivociProdotti(sql::Connection& connection)
    {
        std::unique_ptr<sql::Statement> statement(connection.createStatement());
        std::unique_ptr<sql::ResultSet> result(statement->executeQuery(
            "SELECT INDEX_NAME, GROUP_CONCAT(COALESCE(COLUMN_NAME, '<espressione>') "
            "ORDER BY COLUMN_NAME SEPARATOR ',') AS colonne "
            "FROM INFORMATION_SCHEMA.STATISTICS "
            "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'prodotti' "
            "AND NON_UNIQUE = 0 AND INDEX_NAME <> 'PRIMARY' "
            "GROUP BY INDEX_NAME HAVING SUM(SUB_PART IS NOT NULL) = 0"));
        std::vector<IndiceProdotti> indici;
        while (result->next())
            indici.push_back({ result->getString("INDEX_NAME"), result->getString("colonne") });
        return indici;
    }

    // Quoting is needed even for index names read from database metadata.
    std::string identificatoreSQL(const std::string& nome)
    {
        std::string risultato = "`";
        for (char c : nome)
        {
            risultato += c;
            if (c == '`') risultato += '`';
        }
        return risultato + "`";
    }

    bool identitaProdottiNonNull(sql::Connection& connection)
    {
        std::unique_ptr<sql::Statement> statement(connection.createStatement());
        std::unique_ptr<sql::ResultSet> colonne(statement->executeQuery(
            "SELECT COUNT(*) AS presenti FROM INFORMATION_SCHEMA.COLUMNS "
            "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'prodotti' "
            "AND COLUMN_NAME IN ('articolo', 'modello', 'anno', 'id_stagione') AND IS_NULLABLE = 'NO'"));
        if (colonne->next() && colonne->getInt("presenti") == 4) return true;
        colonne.reset();
        std::unique_ptr<sql::ResultSet> vincolo(statement->executeQuery(
            "SELECT cc.CHECK_CLAUSE FROM INFORMATION_SCHEMA.TABLE_CONSTRAINTS tc "
            "INNER JOIN INFORMATION_SCHEMA.CHECK_CONSTRAINTS cc "
            "ON cc.CONSTRAINT_SCHEMA = tc.CONSTRAINT_SCHEMA AND cc.CONSTRAINT_NAME = tc.CONSTRAINT_NAME "
            "WHERE tc.CONSTRAINT_SCHEMA = DATABASE() AND tc.TABLE_NAME = 'prodotti' "
            "AND tc.CONSTRAINT_NAME = 'chk_prodotti_identita_completa' AND tc.ENFORCED = 'YES'"));
        if (!vincolo->next()) return false;
        std::string clausola = vincolo->getString("CHECK_CLAUSE");
        clausola.erase(std::remove_if(clausola.begin(), clausola.end(), [](unsigned char c) {
            return std::isspace(c) || c == '`' || c == '(' || c == ')';
        }), clausola.end());
        std::transform(clausola.begin(), clausola.end(), clausola.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return clausola == "articoloisnotnullandmodelloisnotnullandannoisnotnullandid_stagioneisnotnull";
    }

    std::string erroreProdotto(const sql::SQLException& err)
    {
        if (err.getErrorCode() == 1062)
            return "Esiste gia' un prodotto con la stessa combinazione articolo, modello, anno e stagione. "
                "Scegliere una combinazione diversa.";
        std::ostringstream messaggio;
        messaggio << err.what() << " (codice: " << err.getErrorCode()
            << ", stato SQL: " << err.getSQLState() << ")";
        return messaggio.str();
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
        { "trattamenti", { "id_trattamento", "trattamento" } },
        { "stagioni", { "id_stagione", "codice" } },
        { "prodotti", { "id_prodotto", "articolo", "modello", "anno", "descrizione", "id_filato", "id_cliente", "id_tecnico", "id_stagione", "id_trattamento", "id_immagine" } },
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

        if (problemi.empty() && !verificaUnicitaProdotti())
            problemi.push_back(ultimo_errore);

        if (problemi.empty())
        {
            std::unique_ptr<sql::Statement> statement(connection->createStatement());
            std::unique_ptr<sql::ResultSet> result(statement->executeQuery(
                "SELECT COUNT(*) AS incoerenti FROM immagini i "
                "INNER JOIN prodotti p ON p.id_immagine = i.id_immagine "
                "INNER JOIN stagioni s ON s.id_stagione = p.id_stagione "
                "WHERE i.dati_immagine IS NOT NULL "
                "AND i.nome_file <> CONCAT(p.articolo, '-', p.modello, '-', p.anno, '-', s.codice) "
                "AND i.nome_file NOT LIKE CONCAT(p.articolo, '-', p.modello, '-', p.anno, '-', s.codice, '.%')"));
            if (result->next() && result->getInt("incoerenti") > 0)
                problemi.push_back("Sono presenti nomi immagine non coerenti con articolo-modello-anno-stagione.");
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

bool DatabaseManager::verificaUnicitaProdotti()
{
    ultimo_errore.clear();
    if (!isConnesso())
    {
        ultimo_errore = "Connessione DataBase non attiva.";
        return false;
    }
    try
    {
        bool presente = false;
        for (const auto& indice : indiciUnivociProdotti(*connection))
        {
            if (indice.colonne == "articolo,modello")
            {
                ultimo_errore = "E' ancora presente un vincolo univoco sulla sola coppia articolo-modello.";
                return false;
            }
            if (indice.colonne == "anno,articolo,id_stagione,modello") presente = true;
        }
        if (!presente || !identitaProdottiNonNull(*connection))
        {
            ultimo_errore = "Manca il vincolo UNIQUE completo su articolo, modello, anno e stagione "
                "oppure e' consentita un'identita' incompleta.";
            return false;
        }
        return true;
    }
    catch (const sql::SQLException& err)
    {
        ultimo_errore = err.what();
        return false;
    }
}

bool DatabaseManager::preparaArchivioProdotti()
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
        // No automatic merging or deletion: inconsistent archives need explicit correction.
        std::unique_ptr<sql::ResultSet> incompleti(statement->executeQuery(
            "SELECT COUNT(*) AS presenti FROM prodotti WHERE articolo IS NULL OR modello IS NULL "
            "OR anno IS NULL OR id_stagione IS NULL"));
        if (incompleti->next() && incompleti->getInt("presenti") != 0)
        {
            ultimo_errore = "Migrazione interrotta: esistono prodotti con articolo, modello, anno o stagione NULL. "
                "Correggere i dati prima di riavviare. Nessun prodotto e' stato eliminato.";
            return false;
        }
        incompleti.reset();
        std::unique_ptr<sql::ResultSet> duplicati(statement->executeQuery(
            "SELECT articolo, modello, anno, id_stagione, COUNT(*) AS quanti FROM prodotti "
            "GROUP BY articolo, modello, anno, id_stagione HAVING COUNT(*) > 1 LIMIT 1"));
        if (duplicati->next())
        {
            std::ostringstream errore;
            errore << "Migrazione interrotta: " << duplicati->getInt("quanti")
                << " prodotti duplicati per articolo " << duplicati->getInt("articolo")
                << ", modello " << duplicati->getInt("modello") << ", anno " << duplicati->getInt("anno")
                << ", id stagione " << duplicati->getInt("id_stagione")
                << ". Correggere i duplicati prima di riavviare. Nessun prodotto e' stato eliminato.";
            ultimo_errore = errore.str();
            return false;
        }
        duplicati.reset();

        const auto indici = indiciUnivociProdotti(*connection);
        std::vector<std::string> modifiche;
        bool presente = false;
        for (const auto& indice : indici)
        {
            if (indice.colonne == "anno,articolo,id_stagione,modello") presente = true;
            if (indice.colonne == "articolo,modello")
                modifiche.push_back("DROP INDEX " + identificatoreSQL(indice.nome));
        }
        if (!presente)
            modifiche.push_back("ADD CONSTRAINT uq_prodotti_collezione UNIQUE (articolo, modello, anno, id_stagione)");
        if (!identitaProdottiNonNull(*connection))
            modifiche.push_back("ADD CONSTRAINT chk_prodotti_identita_completa CHECK "
                "(articolo IS NOT NULL AND modello IS NOT NULL AND anno IS NOT NULL AND id_stagione IS NOT NULL)");
        if (!modifiche.empty())
        {
            // One atomic ALTER: never remove the old index before creating the new one.
            std::string query = "ALTER TABLE prodotti ";
            for (std::size_t i = 0; i < modifiche.size(); ++i)
            {
                if (i != 0) query += ", ";
                query += modifiche[i];
            }
            statement->execute(query);
        }
        return verificaUnicitaProdotti();
    }
    catch (const sql::SQLException& err)
    {
        ultimo_errore = "Impossibile applicare l'unicita' dei prodotti: " + std::string(err.what()) +
            ". Verificare lo schema e i privilegi ALTER dell'utente database.";
        return false;
    }
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
            "INNER JOIN stagioni s ON s.id_stagione = p.id_stagione "
            "SET i.nome_file = CONCAT(p.articolo, '-', p.modello, '-', p.anno, '-', s.codice, "
            "CASE WHEN i.nome_file IS NOT NULL AND INSTR(i.nome_file, '.') > 0 "
            "THEN CONCAT('.', LOWER(SUBSTRING_INDEX(i.nome_file, '.', -1))) ELSE '' END), "
            "i.percorso_file = CONCAT('database://', p.articolo, '-', p.modello, '-', p.anno, '-', s.codice, "
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

bool DatabaseManager::preparaArchivioTrattamenti()
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
        statement->execute(
            "CREATE TABLE IF NOT EXISTS trattamenti ("
            "id_trattamento INT NOT NULL AUTO_INCREMENT, "
            "trattamento VARCHAR(255) NOT NULL, "
            "PRIMARY KEY (id_trattamento), "
            "UNIQUE KEY uq_trattamenti_trattamento (trattamento)"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");

        std::unique_ptr<sql::ResultSet> colonna(statement->executeQuery(
            "SELECT COUNT(*) AS presente FROM INFORMATION_SCHEMA.COLUMNS "
            "WHERE TABLE_SCHEMA = DATABASE() AND TABLE_NAME = 'prodotti' "
            "AND COLUMN_NAME = 'id_trattamento'"));
        if (!colonna->next() || colonna->getInt("presente") == 0)
            statement->execute("ALTER TABLE prodotti ADD COLUMN id_trattamento INT NULL AFTER id_stagione");

        std::unique_ptr<sql::ResultSet> vincolo(statement->executeQuery(
            "SELECT COUNT(*) AS presente FROM INFORMATION_SCHEMA.TABLE_CONSTRAINTS "
            "WHERE CONSTRAINT_SCHEMA = DATABASE() AND TABLE_NAME = 'prodotti' "
            "AND CONSTRAINT_NAME = 'fk_prodotti_trattamenti' AND CONSTRAINT_TYPE = 'FOREIGN KEY'"));
        if (!vincolo->next() || vincolo->getInt("presente") == 0)
        {
            statement->execute(
                "ALTER TABLE prodotti ADD CONSTRAINT fk_prodotti_trattamenti "
                "FOREIGN KEY (id_trattamento) REFERENCES trattamenti(id_trattamento) "
                "ON DELETE SET NULL ON UPDATE CASCADE");
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

std::vector<Trattamento> DatabaseManager::caricaTrattamenti()
{
    std::vector<Trattamento> trattamenti;
    std::unique_ptr<sql::ResultSet> result = eseguiSelect(
        "SELECT id_trattamento, trattamento FROM trattamenti ORDER BY id_trattamento");
    if (result == nullptr) return trattamenti;

    while (result->next())
    {
        trattamenti.emplace_back(
            result->getInt("id_trattamento"),
            result->getString("trattamento"));
    }
    return trattamenti;
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
            ", tr.id_trattamento, tr.trattamento "
            "FROM prodotti p "
            "INNER JOIN stagioni s ON s.id_stagione = p.id_stagione "
            "INNER JOIN filati fi ON fi.id_filato = p.id_filato "
            "LEFT JOIN fornitori fo ON fo.id_fornitore = fi.id_fornitore "
            "INNER JOIN clienti c ON c.id_cliente = p.id_cliente "
            "INNER JOIN tecnici t ON t.id_tecnico = p.id_tecnico "
            "LEFT JOIN trattamenti tr ON tr.id_trattamento = p.id_trattamento "
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
        if (!result->isNull("id_trattamento"))
        {
            prodotti.back().setTrattamento(Trattamento(
                result->getInt("id_trattamento"),
                result->getString("trattamento")));
        }
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

bool DatabaseManager::inserisciTrattamento(const Trattamento& trattamento)
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
                "INSERT INTO trattamenti (trattamento) VALUES (?)"));
        statement->setString(1, trattamento.getTrattamento());
        statement->executeUpdate();
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

bool DatabaseManager::aggiornaTrattamento(int idTrattamento, const Trattamento& trattamento)
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
                "UPDATE trattamenti SET trattamento = ? WHERE id_trattamento = ?"));
        statement->setString(1, trattamento.getTrattamento());
        statement->setInt(2, idTrattamento);
        return statement->executeUpdate() > 0;
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

bool DatabaseManager::eliminaTrattamento(int idTrattamento)
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
                "DELETE FROM trattamenti WHERE id_trattamento = ?"));
        statement->setInt(1, idTrattamento);
        return statement->executeUpdate() > 0;
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

bool DatabaseManager::inserisciProdotto(const Prodotto& prodotto)
{
    ultimo_errore.clear();

    if (!isConnesso())
    {
        ultimo_errore = "Connessione DataBase non attiva.";
        return false;
    }

    if (prodotto.getAnno() < 1900 || prodotto.getAnno() > 2100 ||
        (prodotto.getStagione() != 1 && prodotto.getStagione() != 2))
    {
        ultimo_errore = "Anno o stagione non validi: anno tra 1900 e 2100, stagione PE o AI.";
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
                "(articolo, modello, anno, descrizione, id_filato, id_cliente, id_tecnico, id_stagione, id_trattamento, id_immagine) "
                "SELECT ?, ?, ?, ?, fi.id_filato, c.id_cliente, t.id_tecnico, s.id_stagione, NULLIF(?, 0), ? "
                "FROM filati fi "
                "INNER JOIN clienti c ON c.serie = ? AND c.attivo = 1 "
                "INNER JOIN tecnici t ON t.cognome = ? AND t.nome = ? AND t.attivo = 1 "
                "INNER JOIN stagioni s ON s.codice = ? "
                "WHERE fi.codice = ? AND fi.attivo = 1"));
        statement->setInt(1, prodotto.getArticolo());
        statement->setInt(2, prodotto.getModello());
        statement->setInt(3, prodotto.getAnno());
        statement->setString(4, prodotto.getDescrizione());
        statement->setInt(5, prodotto.getTrattamento().getId_Trattamento());
        statement->setInt(6, idImmagine);
        statement->setInt(7, prodotto.getCliente().getSerie());
        statement->setString(8, prodotto.getTecnico().getCognome());
        statement->setString(9, prodotto.getTecnico().getNome());
        statement->setString(10, shortToCodiceStagione(prodotto.getStagione()));
        statement->setInt(11, prodotto.getFilato().getCodice());

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

        ultimo_errore = erroreProdotto(err);
        return false;
    }
}

bool DatabaseManager::aggiornaProdotto(int idProdotto, const Prodotto& prodotto)
{
    ultimo_errore.clear();

    if (!isConnesso())
    {
        ultimo_errore = "Connessione DataBase non attiva.";
        return false;
    }

    if (prodotto.getAnno() < 1900 || prodotto.getAnno() > 2100 ||
        (prodotto.getStagione() != 1 && prodotto.getStagione() != 2))
    {
        ultimo_errore = "Anno o stagione non validi: anno tra 1900 e 2100, stagione PE o AI.";
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
        // Lock the target and its new references before updating. A successful
        // no-op save must not be mistaken for a missing record or invalid links.
        std::unique_ptr<sql::PreparedStatement> verifica(connection->prepareStatement(
            "SELECT p.id_prodotto FROM prodotti p "
            "INNER JOIN filati fi ON fi.codice = ? AND fi.attivo = 1 "
            "INNER JOIN clienti c ON c.serie = ? AND c.attivo = 1 "
            "INNER JOIN tecnici t ON t.cognome = ? AND t.nome = ? AND t.attivo = 1 "
            "INNER JOIN stagioni s ON s.codice = ? "
            "WHERE p.id_prodotto = ? FOR UPDATE"));
        verifica->setInt(1, prodotto.getFilato().getCodice());
        verifica->setInt(2, prodotto.getCliente().getSerie());
        verifica->setString(3, prodotto.getTecnico().getCognome());
        verifica->setString(4, prodotto.getTecnico().getNome());
        verifica->setString(5, shortToCodiceStagione(prodotto.getStagione()));
        verifica->setInt(6, idProdotto);
        std::unique_ptr<sql::ResultSet> esistente(verifica->executeQuery());
        if (!esistente->next())
        {
            ultimo_errore = "Prodotto non trovato oppure filato, cliente, tecnico o stagione non disponibili.";
            connection->rollback();
            connection->setAutoCommit(true);
            return false;
        }
        esistente.reset();

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
                "p.id_trattamento = NULLIF(?, 0), "
                "i.percorso_file = CONCAT('database://', ?, "
                "CASE WHEN INSTR(i.nome_file, '.') > 0 THEN CONCAT('.', LOWER(SUBSTRING_INDEX(i.nome_file, '.', -1))) ELSE '' END), "
                "i.nome_file = CASE WHEN i.nome_file IS NULL THEN NULL ELSE "
                "CONCAT(?, "
                "CASE WHEN INSTR(i.nome_file, '.') > 0 THEN CONCAT('.', LOWER(SUBSTRING_INDEX(i.nome_file, '.', -1))) ELSE '' END) END, "
                "i.descrizione = ? "
                "WHERE p.id_prodotto = ?"));
        statement->setInt(1, prodotto.getFilato().getCodice());
        statement->setInt(2, prodotto.getCliente().getSerie());
        statement->setString(3, prodotto.getTecnico().getCognome());
        statement->setString(4, prodotto.getTecnico().getNome());
        statement->setString(5, shortToCodiceStagione(prodotto.getStagione()));
        statement->setInt(6, prodotto.getArticolo());
        statement->setInt(7, prodotto.getModello());
        statement->setInt(8, prodotto.getAnno());
        statement->setString(9, prodotto.getDescrizione());
        statement->setInt(10, prodotto.getTrattamento().getId_Trattamento());
        const std::string nomeBase = std::to_string(prodotto.getArticolo()) + "-" +
            std::to_string(prodotto.getModello()) + "-" + std::to_string(prodotto.getAnno()) + "-" +
            shortToCodiceStagione(prodotto.getStagione());
        statement->setString(11, nomeBase);
        statement->setString(12, nomeBase);
        statement->setString(13, prodotto.getImmagine().getDescrizione());
        statement->setInt(14, idProdotto);
        statement->executeUpdate();
        connection->commit();
        connection->setAutoCommit(true);
        return true;
    }
    catch (const sql::SQLException& err)
    {
        try
        {
            connection->rollback();
            connection->setAutoCommit(true);
        }
        catch (...)
        {
        }
        ultimo_errore = erroreProdotto(err);
        return false;
    }
}
bool DatabaseManager::eliminaProdotto(int idProdotto)
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
                "SELECT id_immagine FROM prodotti WHERE id_prodotto = ? FOR UPDATE"));
        cercaImmagine->setInt(1, idProdotto);

        std::unique_ptr<sql::ResultSet> immagineResult(cercaImmagine->executeQuery());
        if (!immagineResult->next())
        {
            connection->rollback();
            connection->setAutoCommit(true);
            return false;
        }

        int idImmagine = immagineResult->getInt("id_immagine");

        std::unique_ptr<sql::PreparedStatement> eliminaProdotto(
            connection->prepareStatement("DELETE FROM prodotti WHERE id_prodotto = ?"));
        eliminaProdotto->setInt(1, idProdotto);

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
    int idProdotto,
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
        const std::string estensione = estensioneImmagine(nomeFile);
        const std::string contenuto(reinterpret_cast<const char*>(dati.data()), dati.size());
        std::istringstream flusso(contenuto);
        std::unique_ptr<sql::PreparedStatement> statement(
            connection->prepareStatement(
                "UPDATE immagini i "
                "INNER JOIN prodotti p ON p.id_immagine = i.id_immagine "
                "INNER JOIN stagioni s ON s.id_stagione = p.id_stagione "
                "SET i.nome_file = CONCAT(p.articolo, '-', p.modello, '-', p.anno, '-', s.codice, ?), "
                "i.mime_type = ?, i.dati_immagine = ?, "
                "i.percorso_file = CONCAT('database://', p.articolo, '-', p.modello, '-', p.anno, '-', s.codice, ?), "
                "i.data_caricamento = NOW(6) "
                "WHERE p.id_prodotto = ?"));
        statement->setString(1, estensione);
        statement->setString(2, mimeType);
        statement->setBlob(3, &flusso);
        statement->setString(4, estensione);
        statement->setInt(5, idProdotto);
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
    int idProdotto,
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
                "WHERE p.id_prodotto = ?"));
        statement->setInt(1, idProdotto);
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




