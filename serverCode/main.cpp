#include "crow.h"
#include <nlohmann/json.hpp>
#include <vector>
#include <mutex>
#include <string>
#include <chrono>
#include <ctime>
#include <map>
#include <sstream>
#include <fstream> // für CSV-Datei

using json = nlohmann::json;

// Zuordnung von user_id zu Name
const std::map<int, std::string> user_id_to_name = {
    {101, "Joel Frei"},
    {102, "Kai Knezevic"},
    {103, "Armin Kasumovic"}
};

const std::string CSV_Filename =
        "C:/Users/a9Frei/Documents/BBZ/Softwareentwicklung/projekt zeiterfassung/RFID_DATA/Data_Log.csv";

struct Data {
    int user_id; // USER Identifikation
    std::string name; // Name der Person
    std::string timestamp; // ”01.01.2000_18:00”
    bool status; // 1 -> gekommen, 0-> gegangen
};

std::vector<Data> data;
std::mutex data_mutex;

// Map für aktuellen Status (eingeloggt/ausgeloggt)
std::map<int, bool> current_status;

void to_json(json &j, const Data &a) {
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
        return it->second;
    }
    return "";
}

// Ausgabe aller aktuell eingeloggten User auf der Konsole
void print_logged_in_users() {
    std::cout << "=== Aktuell eingeloggte Karten ===" << std::endl;
    for (const auto &[user_id, status]: current_status) {
        if (status) {
            std::cout << "User ID: " << user_id
                    << " | Name: " << find_name_by_id(user_id)
                    << std::endl;
        }
    }
    std::cout << "=================================" << std::endl;
}

int main() {
    crow::SimpleApp app;

    // PUT-Route: Fügt einen neuen RFID-Logeintrag hinzu
    CROW_ROUTE(app, "/api/rfid/log").methods("PUT"_method)([](const crow::request &req) {
        json body;
        try {
            std::cout << "hier wird es nun versucht" << std::endl;
            body = json::parse(req.body);
        } catch (const std::exception &e) {
            return crow::response(400, "Invalid JSON format in request body.");
        }

        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm *ltm = std::localtime(&now);
        char buffer[20];
        std::strftime(buffer, sizeof(buffer), "%d.%m.%Y_%H:%M", ltm);
        std::string generated_timestamp(buffer);

        if (!body.contains("user_id") || !body["user_id"].is_number_integer()) {
            return crow::response(400, "Invalid JSON: missing or incorrect data type for user_id (int).");
        }

        int user_id = body["user_id"].get<int>();

        bool newStatus; {
            std::lock_guard<std::mutex> lock(data_mutex);
            bool lastStatus = current_status[user_id];
            newStatus = !lastStatus;
            current_status[user_id] = newStatus;
        }

        std::string name = find_name_by_id(user_id);
        if (name.empty()) {
            return crow::response(404, "User ID " + std::to_string(user_id) + " not found in the user list.");
        }

        Data newData{user_id, name, generated_timestamp, newStatus}; {
            std::lock_guard<std::mutex> lock(data_mutex);
            data.push_back(newData);
        }

        std::ofstream file(CSV_Filename, std::ios::app);
        if (file.is_open()) {
            file << newData.user_id << ","
                    << newData.name << ","
                    << newData.timestamp << ","
                    << (newData.status ? "gekommen" : "gegangen")
                    << "\n";
            file.close();
        } else {
            return crow::response(400, "File could not be opened.");
        }

        print_logged_in_users();
        // Rückmeldung für Buzzer
        json responseJson{
            {"user_id", user_id},
            {"name", name},
            {"timestamp", generated_timestamp},
            {"status", newStatus ? "gekommen" : "gegangen"}
        };

        return crow::response(201, responseJson.dump());
    });

    // GET-Route: Gibt alle gespeicherten RFID-Log-Einträge zurück
    CROW_ROUTE(app, "/api/rfid/log").methods("GET"_method)([]() {
        std::lock_guard<std::mutex> lock(data_mutex);
        json j = data;
        return crow::response(200, j.dump());
    });

    app.bindaddr("0.0.0.0").port(18080).multithreaded().run();
}
