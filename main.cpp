#include "crow.h"
#include <nlohmann/json.hpp>
#include <vector>
#include <mutex>
#include <string>
#include <chrono>
#include <ctime>

using json = nlohmann::json;

struct Data {
    int device_id;
    std::string name;
    std::string timestamp;
    bool status;
};

std::vector<Data> data;
std::mutex data_mutex;

// Automatische JSON-Konvertierung für nlohmann::json
void to_json(json& j, const Data& a) {
    j = json{
        {"device_id", a.device_id},
        {"name", a.name},
        {"timestamp", a.timestamp},
        {"status", a.status}
    };
}

std::string get_timestamp() {
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    std::tm timeinfo{};
#if defined(_WIN32)
    localtime_s(&timeinfo, &now);
#else
    localtime_r(&now, &timeinfo);
#endif

    char buffer[20];
    std::strftime(buffer, sizeof(buffer), "%d.%m.%Y_%H:%M", &timeinfo);
    return std::string(buffer);
}

int main() {
    crow::SimpleApp app;

    // RFID-Log Eintrag erstellen
    CROW_ROUTE(app, "/api/rfid/log").methods("PUT"_method)(
        [](const crow::request& req) {

            json body;
            try {
                body = json::parse(req.body);
            } catch (...) {
                return crow::response(400, "Invalid JSON");
            }

            // JSON Validierung
            if (
                !body.contains("device_id") || !body["device_id"].is_number_integer() ||
                !body.contains("name") || !body["name"].is_string() ||
                !body.contains("status") || !body["status"].is_boolean()
            ) {
                return crow::response(
                    400,
                    "JSON must contain: device_id(int), name(string), status(bool)."
                );
            }

            Data newData{
                body["device_id"].get<int>(),
                body["name"].get<std::string>(),
                get_timestamp(),
                body["status"].get<bool>()
            };

            {
                std::lock_guard<std::mutex> lock(data_mutex);
                data.push_back(newData);
            }

            json response{
                {"message", "RFID Log entry stored."},
                {"timestamp", newData.timestamp}
            };

            return crow::response(201, response.dump());
        }
    );

    // Alle Logs abrufen
    CROW_ROUTE(app, "/api/rfid/log").methods("GET"_method)(
        []() {
            std::lock_guard<std::mutex> lock(data_mutex);
            json j = data;
            return crow::response(200, j.dump());
        }
    );

    app.port(18080).multithreaded().run();
}