#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include "raylib.h"

struct FontEntry {
    std::string label;
    std::string path;
    std::string filename;
};

class FontSystem {
public:
    std::string current_font_path;
    std::string current_font_label = "Predeterminada";
    std::string current_font_pref;

    std::map<int, Font> font_cache;
    Font ui_font;
    std::vector<int> codepoints;
    std::unordered_map<std::string, std::string> font_files_map;
    std::vector<FontEntry> font_list;

    // Estado del menú desplegable de fuentes
    bool menu_open = false;
    int scroll_offset = 0;
    static constexpr int MENU_VISIBLE_ROWS = 12;
    static constexpr int MENU_ROW_HEIGHT = 28;

    FontSystem();
    ~FontSystem();

    void init();
    void cleanup();

    // Detección y escaneo de fuentes locales y del sistema operativo
    void scan_fonts(const std::string& fonts_dir = "fonts");
    bool apply_font(const std::string& path, const std::string& pref, const std::string& label);
    void startup_font(const std::string& pref = "", const std::string& fonts_dir = "fonts");

    // Acceso a la fuente activa cacheada
    Font get_font(int size);

    // Métodos de dibujo y medida con la fuente activa
    void draw_text(const std::string& text, float x, float y, int size, Color color);
    float measure_text(const std::string& text, int size);
    std::string fit_text(const std::string& text, int size, float max_w);
    std::string tail_text(const std::string& text, int size, float max_w);

    // Control y renderizado del menú selector de fuentes
    void draw_menu(float x, float y, float w, float mx, float my);
    bool handle_click(float x, float y, float w, float mx, float my);
    void handle_scroll(float wheel, float x, float y, float w, float mx, float my);
};
