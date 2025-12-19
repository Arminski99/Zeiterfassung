#include "crow.h"
#include <nlohmann/json.hpp>
#include <vector>
#include <mutex>
#include <string>
#include <chrono>
#include <ctime>
#include <map>
#include <sstream>

using json = nlohmann::json;

// Zuordnung von user_id zu Name
const std::map<int, std::string> user_id_to_name = {
    {101, "Joel Frei"},
    {102, "Kai Knezevic"},
    {103, "Armin Kasumovic"}
};


struct Data {
    int user_id; // USER Identifikation
    std::string name; // Name der Person
    std::string timestamp; // ”01.01.2000_18:00”
    bool status; // 1 -> gekommen, 0-> gegangen
};

std::vector<Data> data;
std::mutex data_mutex;

//************************************MAP
// Map für aktuellen Status (eingeloggt/ausgeloggt)
std::map<int, bool> current_status;


void to_json(json& j, const Data& a) {
    j = json{
            {"user_id", a.user_id},
            {"name", a.name},
            {"timestamp", a.timestamp},
            {"status", a.status}
    };
}

// Hilfsfunktion, um den Namen anhand der user_id zu finden
std::string find_name_by_id(int user_id) {
    auto it = user_id_to_name.find(user_id);
    if (it != user_id_to_name.end()) {
        return it->second; // Name gefunden
    }
    return ""; // Name nicht gefunden
}

//************************************* user logged in
// Ausgabe aller aktuell eingeloggten User auf der Konsole
void print_logged_in_users() {
    std::cout << "=== Aktuell eingeloggte Karten ===" << std::endl;
    for (const auto& [user_id, status] : current_status) {
        if (status) {
            std::cout << "User ID: " << user_id
                      << " | Name: " << find_name_by_id(user_id)
                      << std::endl;
        }
    }
    std::cout << "=================================" << std::endl;
}
/* ================= */

int main() {
    crow::SimpleApp app;

    // PUT-Route: Fügt einen neuen RFID-Logeintrag hinzu
    // Endpunkt: /api/rfid/log
    CROW_ROUTE(app, "/api/rfid/log").methods("PUT"_method)([](const crow::request& req) {
        json body;
        try {
            // Wandelt den Request-Body in ein JSON-Objekt um
            body = json::parse(req.body);
        } catch (const std::exception& e) {
            // Fehler bei der JSON-Analyse
            return crow::response(400, "Invalid JSON format in request body.");
        }
        // Serverseitige Timestamp-Generierung im Format "DD.MM.YYYY_HH:MM"
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm* ltm = std::localtime(&now);

        char buffer[20];
        std::strftime(buffer, sizeof(buffer), "%d.%m.%Y_%H:%M", ltm);
        std::string generated_timestamp(buffer);


        // Prüft auf user_id (int)
        if (!body.contains("user_id") || !body["user_id"].is_number_integer()) {
             return crow::response(400, "Invalid JSON: missing or incorrect data type for user_id (int).");
        }

        // Prüft auf status (bool)
        if (!body.contains("status") || !body["status"].is_boolean()) {
            return crow::response(400, "Invalid JSON: missing or incorrect data type for status (bool).");
        }

        // Holt die user_id aus dem Request
        int user_id = body["user_id"].get<int>();

        // Sucht den Namen anhand der user_id in der hartkodierten Map
        std::string name = find_name_by_id(user_id);

        if (name.empty()) {
            // user_id wurde nicht in der Map gefunden
            return crow::response(404, "User ID " + std::to_string(user_id) + " not found in the user list.");
        }

        Data newData{
                user_id,             // user_id aus dem Request
                name,                // Name aus der Map
                generated_timestamp, // Zuweisung des generierten Timestamps
                body["status"].get<bool>()
        };

        // Daten unter Schutz des Mutex zur globalen Liste hinzufügen
        {
            std::lock_guard<std::mutex> lock(data_mutex);
            data.push_back(newData);
            current_status[newData.user_id] = newData.status; // aktualisiert den aktuellen Status
        }

        print_logged_in_users(); // Ausgabe aller aktuell eingeloggten User

        // Erfolgreiche Antwort (201 Created) zurückgeben
        return crow::response(201, "RFID Log entry successfully recorded for " + name + " at " + generated_timestamp + ".");
    });

    // GET-Route: Gibt alle gespeicherten RFID-Log-Einträge zurück
    // Endpunkt: /api/rfid/log
    CROW_ROUTE(app, "/api/rfid/log").methods("GET"_method)([]() {
        // Schützt den Vektor vor gleichzeitigen Zugriffen
        std::lock_guard<std::mutex> lock(data_mutex);

        // Konvertiere den Vektor der Data-Strukturen in ein JSON-Array
        json j = data;

        // Gebe die Daten als JSON zurück
        return crow::response(200, j.dump());
    });

    //********************************** CSV
    CROW_ROUTE(app, "/api/rfid/log/csv").methods("GET"_method)([](){
        std::lock_guard<std::mutex> lock(data_mutex);

        std::stringstream csv;
        csv << "user_id,name,timestamp,status\n";

        for (const auto& entry : data) {
            csv << entry.user_id << ","
                << entry.name << ","
                << entry.timestamp << ","
                << (entry.status ? "gekommen" : "gegangen")
                << "\n";
        }

        crow::response res;
        res.code = 200;
        res.set_header("Content-Type", "text/csv");
        res.set_header("Content-Disposition", "attachment; filename=\"rfid_logs.csv\"");
        res.body = csv.str();
        return res;
    });
    /* ================= */

    app.port(18080).multithreaded().run();
}
