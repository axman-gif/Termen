#include "font_system.hpp"
#include "text_utils.hpp"
#include <filesystem>
#include <algorithm>
#include <cstdlib>

FontSystem::FontSystem() {
    // 1. ASCII básico (32 a 126)
    for (int i = 32; i <= 126; ++i) {
        codepoints.push_back(i);
    }

    // 2. Rango Suplemento Latin-1 (160 a 255) que incluye vocales con tildes, ñ, diéresis, ¿, ¡, etc.
    for (int i = 0x00A0; i <= 0x00FF; ++i) {
        codepoints.push_back(i);
    }

    // 3. Puntuación y símbolos especiales (comillas tipográficas, guiones, viñeta, flechas, triángulo dropdown)
    std::vector<int> extra_cps = {
        0x2014, // Raya —
        0x2018, 0x2019, // Comillas simples ‘ ’
        0x201C, 0x201D, // Comillas dobles “ ”
        0x2026, // Puntos suspensivos …
        0x2022, // Viñeta •
        0x2190, // Flecha izquierda ←
        0x2192, // Flecha derecha →
        0x25BC  // Triángulo menú desplegable ▼
    };
    for (int cp : extra_cps) {
        codepoints.push_back(cp);
    }
}

FontSystem::~FontSystem() {
    cleanup();
}

void FontSystem::init() {
    ui_font = GetFontDefault();
}

void FontSystem::cleanup() {
    for (auto& pair : font_cache) {
        if (pair.second.texture.id != 0 && pair.second.texture.id != GetFontDefault().texture.id) {
            UnloadFont(pair.second);
        }
    }
    font_cache.clear();
}

void FontSystem::scan_fonts(const std::string& fonts_dir) {
    if (!font_list.empty()) return;

    std::vector<std::pair<std::string, bool>> dirs;
    dirs.push_back({fonts_dir, true});

#ifdef _WIN32
    const char* windir = getenv("WINDIR");
    std::string win = windir ? windir : "C:/Windows";
    dirs.push_back({win + "/Fonts", false});
    const char* loc = getenv("LOCALAPPDATA");
    if (loc) {
        dirs.push_back({std::string(loc) + "/Microsoft/Windows/Fonts", false});
    }
#else
    const char* home = getenv("HOME");
    std::string h = home ? home : "";
    dirs.push_back({"/usr/share/fonts", false});
    dirs.push_back({"/usr/local/share/fonts", false});
    if (!h.empty()) {
        dirs.push_back({h + "/.local/share/fonts", false});
        dirs.push_back({h + "/.fonts", false});
        dirs.push_back({h + "/Library/Fonts", false});
    }
    dirs.push_back({"/System/Library/Fonts", false});
    dirs.push_back({"/Library/Fonts", false});
#endif

    std::vector<FontEntry> local_fonts;
    std::vector<FontEntry> system_fonts;

    for (const auto& d_pair : dirs) {
        const std::string& d = d_pair.first;
        bool is_local = d_pair.second;
        try {
            if (!std::filesystem::exists(d) || !std::filesystem::is_directory(d)) continue;
            for (const auto& entry : std::filesystem::recursive_directory_iterator(
                     d, std::filesystem::directory_options::skip_permission_denied)) {
                if (entry.is_regular_file()) {
                    std::string ext = entry.path().extension().string();
                    for (char& c : ext) c = static_cast<char>(tolower(c));
                    if (ext == ".ttf" || ext == ".otf") {
                        std::string fname = entry.path().filename().string();
                        std::string fstem = entry.path().stem().string();
                        std::string fpath = entry.path().string();
                        for (char& c : fpath) { if (c == '\\') c = '/'; }

                        std::string low_fname = fname;
                        for (char& c : low_fname) c = static_cast<char>(tolower(c));

                        if (font_files_map.find(low_fname) == font_files_map.end()) {
                            font_files_map[low_fname] = fpath;
                            if (is_local) {
                                local_fonts.push_back({fstem + " (fonts/)", fpath, fname});
                            } else {
                                system_fonts.push_back({fstem, fpath, fname});
                            }
                        }
                    }
                }
            }
        } catch (...) {
            continue;
        }
    }

    auto comp = [](const FontEntry& a, const FontEntry& b) {
        std::string la = a.label, lb = b.label;
        for (char& c : la) c = static_cast<char>(tolower(c));
        for (char& c : lb) c = static_cast<char>(tolower(c));
        return la < lb;
    };
    std::sort(local_fonts.begin(), local_fonts.end(), comp);
    std::sort(system_fonts.begin(), system_fonts.end(), comp);

    font_list.clear();
    font_list.push_back({"Predeterminada (raylib)", "", "__default__"});
    for (const auto& f : local_fonts) font_list.push_back(f);
    for (const auto& f : system_fonts) font_list.push_back(f);
}

bool FontSystem::apply_font(const std::string& path, const std::string& pref, const std::string& label) {
    cleanup();

    if (path.empty()) {
        current_font_path.clear();
        current_font_label = "Predeterminada";
        current_font_pref = pref;
        return true;
    }

    // Cargar con tamaño base 20 para verificar
    Font f = LoadFontEx(path.c_str(), 20, codepoints.data(), static_cast<int>(codepoints.size()));
    if (f.texture.id == 0 || f.texture.id == GetFontDefault().texture.id || f.glyphCount <= 0) {
        current_font_path.clear();
        current_font_label = "Predeterminada";
        current_font_pref = pref;
        return false;
    }

    SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
    font_cache[20] = f;

    current_font_path = path;
    current_font_label = label;
    current_font_pref = pref;
    return true;
}

void FontSystem::startup_font(const std::string& pref, const std::string& fonts_dir) {
    if (pref == "__default__") {
        apply_font("", "__default__", "Predeterminada");
        return;
    }

    if (pref.empty()) {
        std::vector<std::string> init_fonts = {
            "C:/Windows/Fonts/segoeui.ttf",
            "C:/Windows/Fonts/arial.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/System/Library/Fonts/Supplemental/Arial.ttf"
        };
        for (const auto& p : init_fonts) {
            if (std::filesystem::exists(p)) {
                std::string stem = std::filesystem::path(p).stem().string();
                std::string fname = std::filesystem::path(p).filename().string();
                if (apply_font(p, fname, stem)) {
                    return;
                }
            }
        }
        apply_font("", "", "Predeterminada");
        return;
    }

    scan_fonts(fonts_dir);
    std::string low_pref = pref;
    for (char& c : low_pref) c = static_cast<char>(tolower(c));

    auto it = font_files_map.find(low_pref);
    if (it != font_files_map.end()) {
        std::string stem = std::filesystem::path(it->second).stem().string();
        apply_font(it->second, pref, stem);
    } else {
        apply_font("", pref, "Predeterminada");
    }
}

Font FontSystem::get_font(int size) {
    int clamped_size = std::clamp(size, 8, 80);
    auto it = font_cache.find(clamped_size);
    if (it != font_cache.end()) {
        return it->second;
    }

    Font f;
    if (!current_font_path.empty()) {
        f = LoadFontEx(current_font_path.c_str(), clamped_size, codepoints.data(), static_cast<int>(codepoints.size()));
        if (f.texture.id != 0 && f.texture.id != GetFontDefault().texture.id) {
            SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
        } else {
            f = GetFontDefault();
        }
    } else {
        f = GetFontDefault();
    }
    font_cache[clamped_size] = f;
    return f;
}

void FontSystem::draw_text(const std::string& text, float x, float y, int size, Color color) {
    Font f = get_font(size);
    DrawTextEx(f, text.c_str(), Vector2{x, y}, static_cast<float>(size), 1.0f, color);
}

float FontSystem::measure_text(const std::string& text, int size) {
    Font f = get_font(size);
    return MeasureTextEx(f, text.c_str(), static_cast<float>(size), 1.0f).x;
}

std::string FontSystem::fit_text(const std::string& text, int size, float max_w) {
    Font f = get_font(size);
    return TextUtils::fit(text, f, size, max_w);
}

std::string FontSystem::tail_text(const std::string& text, int size, float max_w) {
    Font f = get_font(size);
    return TextUtils::tail(text, f, size, max_w);
}

void FontSystem::draw_menu(float x, float y, float w, float mx, float my) {
    if (!menu_open) return;

    int total_fonts = static_cast<int>(font_list.size());
    int visible_rows = std::min(MENU_VISIBLE_ROWS, total_fonts);
    float h = static_cast<float>(visible_rows * MENU_ROW_HEIGHT);

    // Fondo y borde del menú
    DrawRectangle(static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), static_cast<int>(h), Color{20, 20, 20, 245});
    DrawRectangleLines(static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), static_cast<int>(h), Color{120, 120, 120, 255});

    for (int r = 0; r < visible_rows; ++r) {
        int idx = scroll_offset + r;
        if (idx >= total_fonts) break;

        const auto& entry = font_list[idx];
        float ry = y + r * MENU_ROW_HEIGHT;

        bool is_selected = (entry.path == current_font_path);
        bool is_hover = (mx >= x && mx < x + w && my >= ry && my < ry + MENU_ROW_HEIGHT);

        if (is_selected) {
            DrawRectangle(static_cast<int>(x + 1), static_cast<int>(ry), static_cast<int>(w - 2), MENU_ROW_HEIGHT, Color{200, 110, 20, 255});
        } else if (is_hover) {
            DrawRectangle(static_cast<int>(x + 1), static_cast<int>(ry), static_cast<int>(w - 2), MENU_ROW_HEIGHT, Color{55, 55, 55, 255});
        }

        std::string display_label = fit_text(entry.label, 14, w - 24.0f);
        draw_text(display_label, x + 10.0f, ry + 6.0f, 14, is_selected ? WHITE : Color{230, 230, 230, 255});
    }

    // Barra de desplazamiento si hay más elementos
    if (total_fonts > MENU_VISIBLE_ROWS) {
        float sb_w = 6.0f;
        float sb_x = x + w - sb_w - 2.0f;
        float thumb_h = std::max(20.0f, (static_cast<float>(MENU_VISIBLE_ROWS) / total_fonts) * h);
        float max_scroll = static_cast<float>(total_fonts - MENU_VISIBLE_ROWS);
        float thumb_y = y + (static_cast<float>(scroll_offset) / max_scroll) * (h - thumb_h);
        DrawRectangle(static_cast<int>(sb_x), static_cast<int>(thumb_y), static_cast<int>(sb_w), static_cast<int>(thumb_h), Color{140, 140, 140, 255});
    }
}

bool FontSystem::handle_click(float x, float y, float w, float mx, float my) {
    if (!menu_open) return false;

    int total_fonts = static_cast<int>(font_list.size());
    int visible_rows = std::min(MENU_VISIBLE_ROWS, total_fonts);
    float h = static_cast<float>(visible_rows * MENU_ROW_HEIGHT);

    if (mx >= x && mx < x + w && my >= y && my < y + h) {
        int row_idx = static_cast<int>((my - y) / MENU_ROW_HEIGHT);
        int target_idx = scroll_offset + row_idx;
        if (target_idx >= 0 && target_idx < total_fonts) {
            const auto& entry = font_list[target_idx];
            apply_font(entry.path, entry.filename, entry.label);
            menu_open = false;
            return true;
        }
    }
    return false;
}

void FontSystem::handle_scroll(float wheel, float x, float y, float w, float mx, float my) {
    if (!menu_open || wheel == 0.0f) return;

    int total_fonts = static_cast<int>(font_list.size());
    int visible_rows = std::min(MENU_VISIBLE_ROWS, total_fonts);
    float h = static_cast<float>(visible_rows * MENU_ROW_HEIGHT);

    if (mx >= x && mx < x + w && my >= y && my < y + h) {
        int max_scroll = std::max(0, total_fonts - MENU_VISIBLE_ROWS);
        scroll_offset = std::clamp(scroll_offset - static_cast<int>(wheel * 2.0f), 0, max_scroll);
    }
}
