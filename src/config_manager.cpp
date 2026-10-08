#include "config_manager.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

static int parse_int_property(const std::string& json, const std::string& key, int default_val) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return default_val;
    pos = json.find(':', pos + search.size());
    if (pos == std::string::npos) return default_val;
    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) ++pos;
    size_t start = pos;
    if (pos < json.size() && json[pos] == '-') ++pos;
    while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9') ++pos;
    if (pos == start) return default_val;
    try { return std::stoi(json.substr(start, pos - start)); }
    catch (...) { return default_val; }
}

static std::string parse_string_property(const std::string& json, const std::string& key, const std::string& default_val) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return default_val;
    pos = json.find(':', pos + search.size());
    if (pos == std::string::npos) return default_val;
    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\r' || json[pos] == '\n')) ++pos;
    if (pos >= json.size() || json[pos] != '"') return default_val;
    ++pos;
    size_t start = pos;
    while (pos < json.size() && json[pos] != '"') {
        if (json[pos] == '\\' && pos + 1 < json.size()) pos += 2;
        else ++pos;
    }
    return json.substr(start, pos - start);
}

static Color parse_color_property(const std::string& json, const std::string& key, Color default_val) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return default_val;
    pos = json.find('[', pos + search.size());
    if (pos == std::string::npos) return default_val;
    size_t end_bracket = json.find(']', pos);
    if (end_bracket == std::string::npos) return default_val;

    std::string arr_str = json.substr(pos + 1, end_bracket - pos - 1);
    std::stringstream ss(arr_str);
    std::string item;
    int comps[4] = {default_val.r, default_val.g, default_val.b, default_val.a};
    int count = 0;
    while (std::getline(ss, item, ',') && count < 4) {
        size_t s1 = item.find_first_not_of(" \t\r\n");
        size_t s2 = item.find_last_not_of(" \t\r\n");
        if (s1 != std::string::npos && s2 != std::string::npos) {
            try { comps[count] = std::clamp(std::stoi(item.substr(s1, s2 - s1 + 1)), 0, 255); }
            catch (...) {}
            ++count;
        }
    }
    return Color{
        static_cast<unsigned char>(comps[0]),
        static_cast<unsigned char>(comps[1]),
        static_cast<unsigned char>(comps[2]),
        static_cast<unsigned char>(comps[3])
    };
}

std::string ConfigManager::get_config_path() {
    try {
        if (std::filesystem::exists("R:/config.json")) {
            return "R:/config.json";
        }
    } catch (...) {}

    const char* app_dir = GetApplicationDirectory();
    if (app_dir && app_dir[0] != '\0') {
        std::string p = app_dir;
        for (char& c : p) { if (c == '\\') c = '/'; }
        if (p.back() != '/') p.push_back('/');
        if (std::filesystem::exists(p + "config.json")) {
            return p + "config.json";
        }
    }

    try {
        if (std::filesystem::exists("../Noten/build/config.json")) {
            return "../Noten/build/config.json";
        }
        if (std::filesystem::exists("../config.json")) {
            return "../config.json";
        }
    } catch (...) {}

    if (app_dir && app_dir[0] != '\0') {
        std::string p = app_dir;
        for (char& c : p) { if (c == '\\') c = '/'; }
        if (p.back() != '/') p.push_back('/');
        return p + "config.json";
    }
    return "./config.json";
}

bool ConfigManager::load(AppConfig& cfg) {
    std::string path = get_config_path();
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::string json((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    cfg.size = std::clamp(parse_int_property(json, "size", 64), 12, 72);
    
    // Primero intentar leer glossary_size; si no existe, revisar glosario_size
    int gl_sz = parse_int_property(json, "glossary_size", 0);
    if (gl_sz <= 0) {
        gl_sz = parse_int_property(json, "glosario_size", 0);
    }
    if (gl_sz > 0) {
        cfg.glossary_size = std::clamp(gl_sz, 10, 32);
    } else {
        cfg.glossary_size = 15;
    }

    cfg.ahead = std::clamp(parse_int_property(json, "ahead", 1), 1, 3);
    cfg.bgt = parse_color_property(json, "bgt", Color{0, 0, 0, 255});
    cfg.bgb = parse_color_property(json, "bgb", Color{220, 40, 40, 255});
    cfg.txc = parse_color_property(json, "txc", Color{245, 200, 30, 255});
    cfg.font = parse_string_property(json, "font", "arial.ttf");
    cfg.acr_key = parse_int_property(json, "acr_key", 290);
    if (cfg.acr_key == 96) cfg.acr_key = 290;

    return true;
}

bool ConfigManager::save(const AppConfig& cfg) {
    std::string path = get_config_path();
    std::string tmp_path = path + ".tmp";

    std::string out = "{\n";
    out += "  \"size\": " + std::to_string(cfg.size) + ",\n";
    out += "  \"glossary_size\": " + std::to_string(cfg.glossary_size) + ",\n";
    out += "  \"ahead\": " + std::to_string(cfg.ahead) + ",\n";
    out += "  \"bgt\": [" + std::to_string(cfg.bgt.r) + ", " + std::to_string(cfg.bgt.g) + ", " + std::to_string(cfg.bgt.b) + "],\n";
    out += "  \"bgb\": [" + std::to_string(cfg.bgb.r) + ", " + std::to_string(cfg.bgb.g) + ", " + std::to_string(cfg.bgb.b) + "],\n";
    out += "  \"txc\": [" + std::to_string(cfg.txc.r) + ", " + std::to_string(cfg.txc.g) + ", " + std::to_string(cfg.txc.b) + "],\n";
    out += "  \"acr_key\": " + std::to_string(cfg.acr_key);
    if (!cfg.font.empty()) {
        out += ",\n  \"font\": \"" + cfg.font + "\"";
    }
    out += "\n}\n";

    try {
        std::ofstream file(tmp_path, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) return false;
        file.write(out.data(), out.size());
        file.close();

        std::error_code ec;
        std::filesystem::rename(tmp_path, path, ec);
        if (ec) {
            std::filesystem::remove(path, ec);
            std::filesystem::rename(tmp_path, path, ec);
            if (ec) return false;
        }
        return true;
    } catch (...) {
        return false;
    }
}
