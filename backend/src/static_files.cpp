#include "static_files.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

std::string detectMimeType(const std::string& path) {
    const auto dot = path.find_last_of('.');
    const auto ext = dot == std::string::npos ? "" : path.substr(dot + 1);
    if (ext == "html") return "text/html; charset=UTF-8";
    if (ext == "css") return "text/css; charset=UTF-8";
    if (ext == "js") return "application/javascript; charset=UTF-8";
    if (ext == "json") return "application/json; charset=UTF-8";
    if (ext == "svg") return "image/svg+xml";
    if (ext == "png") return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    return "text/plain; charset=UTF-8";
}

std::string readTextFile(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Cannot open file: " + path);
    }
    std::ostringstream content;
    content << input.rdbuf();
    return content.str();
}
