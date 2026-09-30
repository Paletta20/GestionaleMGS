#include <Windows.h>

#include <clocale>
#include <future>
#include <iostream>
#include <string>
#include <vector>

#include "ConfigurazioneDatabase.hpp"
#include "DatabaseManager.hpp"
#include "InterfacciaGrafica.hpp"
#include "Menu.hpp"
#include "SchermataAvvio.hpp"

int main(int argc, char* argv[])
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    std::setlocale(LC_ALL, ".UTF-8");

    const bool modalitaConsole = argc > 1 && std::string(argv[1]) == "--console";
    const bool modalitaVerifica = argc > 1 && std::string(argv[1]) == "--verify";
    const bool modalitaGrafica = !modalitaConsole && !modalitaVerifica;
    if (modalitaGrafica) ShowWindow(GetConsoleWindow(), SW_HIDE);

    ConfigurazioneDatabase configurazione;
    std::string erroreConfigurazione;
    if (!ConfigurazioneDatabase::carica(configurazione, erroreConfigurazione))
    {
        std::cerr << erroreConfigurazione << std::endl;
        MessageBoxA(nullptr, erroreConfigurazione.c_str(), "Gestionale MGS", MB_OK | MB_ICONERROR);
        return 1;
    }

    DatabaseManager mysql;
    bool connessioneRiuscita = false;
    if (modalitaGrafica)
    {
        auto connessione = std::async(std::launch::async, [&]() {
            return mysql.connetti(configurazione.host, configurazione.utente,
                configurazione.password, configurazione.database);
            });
        SchermataAvvio::mostra(GetModuleHandleW(nullptr));
        connessioneRiuscita = connessione.get();
    }
    else
    {
        connessioneRiuscita = mysql.connetti(configurazione.host, configurazione.utente,
            configurazione.password, configurazione.database);
    }

    if (!connessioneRiuscita)
    {
        const std::string errore = "Connessione al database online non riuscita: " + mysql.ultimoErrore();
        std::cerr << errore << std::endl;
        MessageBoxA(nullptr, errore.c_str(), "Gestionale MGS", MB_OK | MB_ICONERROR);
        return 1;
    }

    if (!mysql.preparaArchivioImmagini())
    {
        const std::string errore = "Preparazione archivio immagini non riuscita: " + mysql.ultimoErrore();
        std::cerr << errore << std::endl;
        MessageBoxA(nullptr, errore.c_str(), "Gestionale MGS", MB_OK | MB_ICONERROR);
        return 1;
    }

    if (modalitaVerifica)
    {
        std::vector<std::string> problemi;
        const bool schemaValido = mysql.verificaSchema(problemi);
        mysql.caricaClienti(); const std::string erroreClienti = mysql.ultimoErrore();
        mysql.caricaFilati(); const std::string erroreFilati = mysql.ultimoErrore();
        mysql.caricaFornitori(); const std::string erroreFornitori = mysql.ultimoErrore();
        mysql.caricaTecnici(); const std::string erroreTecnici = mysql.ultimoErrore();
        const std::vector<Prodotto> prodotti = mysql.caricaProdotti();
        const std::string erroreProdotti = mysql.ultimoErrore();

        if (!erroreClienti.empty()) problemi.push_back("Caricamento clienti: " + erroreClienti);
        if (!erroreFilati.empty()) problemi.push_back("Caricamento filati: " + erroreFilati);
        if (!erroreFornitori.empty()) problemi.push_back("Caricamento fornitori: " + erroreFornitori);
        if (!erroreTecnici.empty()) problemi.push_back("Caricamento tecnici: " + erroreTecnici);
        if (!erroreProdotti.empty()) problemi.push_back("Caricamento prodotti: " + erroreProdotti);
        for (const Prodotto& prodotto : prodotti)
        {
            if (!prodotto.haCodiciRelazionaliCoerenti())
            {
                problemi.push_back(
                    "Prodotto incoerente gia' presente: articolo " + std::to_string(prodotto.getArticolo()) +
                    ", modello " + std::to_string(prodotto.getModello()));
            }
        }

        if (schemaValido && problemi.empty())
        {
            std::cout << "Verifica completata: schema e letture dal database sono corretti." << std::endl;
            return 0;
        }
        for (const std::string& problema : problemi) std::cerr << problema << std::endl;
        return 2;
    }

    if (modalitaConsole)
    {
        std::cout << "Connessione al database online riuscita." << std::endl;
        Menu menu(mysql);
        menu.avvia();
        return 0;
    }

    InterfacciaGrafica interfaccia(mysql);
    return interfaccia.avvia(GetModuleHandleW(nullptr), SW_SHOWDEFAULT);
}
