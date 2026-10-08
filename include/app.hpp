#pragma once

#include <string>
#include <vector>
#include <optional>
#include <algorithm>
#include "raylib.h"
#include "font_system.hpp"
#include "glossary.hpp"
#include "config_manager.hpp"

enum AppState {
    STATE_EDITING = 0,
    STATE_CONFIRM = 1
};

struct Layout {
    int win_w = 1240;
    int win_h = 760;
    float margin_x = 40.0f;

    // Barra superior
    float top_bar_h = 52.0f;
    float title_x = 20.0f;
    float title_y = 14.0f;
    float mode_x = 0.0f;
    float mode_y = 14.0f;
    std::string mode_text;
    Color mode_color{140, 140, 140, 255};

    float count_x = 0.0f;
    float count_y = 14.0f;
    std::string count_text;
    Color count_color{160, 160, 160, 255};

    // Botones superiores
    Rectangle btn_font_minus{0, 0, 0, 0};
    Rectangle btn_font_plus{0, 0, 0, 0};
    Rectangle btn_font_rec{0, 0, 0, 0};
    Rectangle btn_search_rec{0, 0, 0, 0};
    Rectangle btn_clear_rec{0, 0, 0, 0};
    Rectangle btn_reset_sort{0, 0, 0, 0};

    // Cuadros de entrada (6 campos)
    Rectangle field_rects[Glossary::NF];
    float label_y[Glossary::NF];

    // Mensaje de estado
    float status_y = 0.0f;

    // Tabla
    float table_x = 0.0f;
    float table_y = 0.0f;
    float table_w = 0.0f;
    float table_h = 0.0f;
    float head_h = 32.0f;
    float sb_w = 14.0f;
    float list_y = 0.0f;
    float list_h = 0.0f;
    float list_w = 0.0f;
    int row_h = 32;

    // Columnas
    float col_off[6];
    float col_w[6];
    float act_off = 0.0f;
    float act_w = 0.0f;
    float act_btn_w = 60.0f;
    float act_btn_h = 24.0f;

    // Pie
    float footer_y = 0.0f;
    float footer_h = 32.0f;
};

class App {
public:
    static constexpr int WIN_W = 1240;
    static constexpr int WIN_H = 760;
    static constexpr int MAX_LEN = 120;
    static constexpr int SB_W = 14;

    // Componentes principales
    FontSystem font_sys;
    Glossary glossary;
    std::string glos_path;
    std::string backup_dir;
    AppConfig config;

    // Estado de la UI
    AppState state = STATE_EDITING;
    std::vector<std::string> fields;
    int active_field = 0;
    std::optional<size_t> editing_index = std::nullopt;
    bool search_mode = false;
    std::vector<SimilarTerm> similar_terms;

    // Ordenamiento
    std::optional<int> sort_col = std::nullopt;
    bool sort_asc = true;

    // Mensaje de estado
    std::string status_msg;
    Color status_color = Color{180, 180, 180, 255};

    // Desplazamiento de tabla
    float scroll = 0.0f;
    bool sb_drag = false;
    float sb_grab = 0.0f;

    // Tamaño de fuente único y unificado para toda la aplicación
    int font_size = 15;
    void change_font_size(int delta);
    void reset_font_size();
    int get_row_height() const { return std::max(32, font_size + 14); }

    // Cuadro de confirmación para salir al presionar ESC
    bool quit_confirm_open = false;
    bool should_close = false;

    // Temporizador de cursor parpadeante
    float cursor_timer = 0.0f;

    App();
    void run();
    Layout compute_layout(size_t displayed_rows_count = 0);

private:
    void init_paths();
    void commit();
    void handle_input();
    void render();
};
