#include "session_manager.hpp"
#include "static_files.hpp"

#include <crow.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace {
fs::path detectProjectRoot() {
    auto cursor = fs::current_path();
    while (!cursor.empty()) {
        if (fs::exists(cursor / "index.html") && fs::exists(cursor / "assets")) {
            return cursor;
        }
        cursor = cursor.parent_path();
    }
    return fs::current_path();
}

crow::response jsonResponse(const Json& payload, int status = 200) {
    crow::response response;
    response.code = status;
    response.set_header("Content-Type", "application/json; charset=UTF-8");
    response.write(payload.dump());
    return response;
}

Json parseBody(const crow::request& req) {
    const auto payload = nlohmann::json::parse(req.body, nullptr, false);
    return payload.is_discarded() ? Json::object() : payload;
}
}

int main() {
    crow::SimpleApp app;
    SessionManager session;

    const fs::path root = detectProjectRoot();

    CROW_ROUTE(app, "/")([&root]() {
        crow::response response;
        response.code = 200;
        response.set_header("Content-Type", "text/html; charset=UTF-8");
        response.write(readTextFile((root / "index.html").string()));
        return response;
    });

    CROW_ROUTE(app, "/assets/<path>")([&root](const std::string& assetPath) {
        const fs::path filePath = root / "assets" / fs::path(assetPath);
        if (!fs::exists(filePath) || !fs::is_regular_file(filePath)) {
            return crow::response(404, "Not Found");
        }
        crow::response response;
        response.code = 200;
        response.set_header("Content-Type", detectMimeType(filePath.string()));
        response.write(readTextFile(filePath.string()));
        return response;
    });

    CROW_ROUTE(app, "/state").methods(crow::HTTPMethod::GET)([&session]() {
        return jsonResponse(session.getState());
    });

    CROW_ROUTE(app, "/settings").methods(crow::HTTPMethod::POST)([&session](const crow::request& req) {
        return jsonResponse(session.updateSettings(parseBody(req)));
    });

    CROW_ROUTE(app, "/startGame").methods(crow::HTTPMethod::POST)([&session](const crow::request& req) {
        return jsonResponse(session.startGame(parseBody(req)));
    });

    CROW_ROUTE(app, "/restart").methods(crow::HTTPMethod::POST)([&session]() {
        return jsonResponse(session.restartGame());
    });

    CROW_ROUTE(app, "/endGame").methods(crow::HTTPMethod::POST)([&session]() {
        return jsonResponse(session.endGame());
    });

    CROW_ROUTE(app, "/move").methods(crow::HTTPMethod::POST)([&session](const crow::request& req) {
        return jsonResponse(session.makeMove(parseBody(req)));
    });

    std::cout << "Server running at http://127.0.0.1:18080" << std::endl;
    app.port(18080).multithreaded().run();
    return 0;
}
