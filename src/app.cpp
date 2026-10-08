#include "app.hpp"
#include "text_utils.hpp"
#include <filesystem>
#include <algorithm>
#include <cmath>

static std::string get_exe_dir() {
    const char* app_dir = GetApplicationDirectory();
    if (app_dir && app_dir[0] != '\0') {
        std::string path = app_dir;
        for (char& c : path) { if (c == '\\') c = '/'; }
        if (path.back() != '/') path.push_back('/');
        return path;
    }
    return "./";
}

App::App() {
    fields.resize(Glossary::NF, "");
    status_msg = "Ingrese términos (ES/EN) y/o siglas con su significado.";
    status_color = Color{180, 180, 180, 255};
}

void App::init_paths() {
    std::string base_dir = get_exe_dir();

    // Soporte para unidad RAM R:/ o archivo local idéntico al proyecto Noten
    try {
        if (std::filesystem::exists("R:/glosario.csv")) {
            base_dir = "R:/";
        }
    } catch (...) {}

    glos_path = base_dir + "glosario.csv";
    backup_dir = base_dir + "respaldos";

    glossary.load(glos_path);

    if (!glossary.msg.empty()) {
        status_msg = glossary.msg;
        status_color = Color{240, 100, 100, 255};
    } else if (glossary.needs_update) {
        status_msg = "Se agregarán las columnas de siglas al primer guardado (con respaldo).";
        status_color = Color{240, 180, 80, 255};
    }

    // Cargar configuración compartida (config.json) sin conflictos con Noten
    if (ConfigManager::load(config)) {
        font_size = config.glossary_size;
    } else {
        config.size = 64;
        config.glossary_size = 15;
        config.ahead = 1;
        config.bgt = Color{0, 0, 0, 255};
        config.bgb = Color{220, 40, 40, 255};
        config.txc = Color{245, 200, 30, 255};
        config.acr_key = 290;
        config.font = "arial.ttf";
        font_size = 15;
        ConfigManager::save(config);
    }

    // Inicializar sistema de fuentes con la fuente configurada
    font_sys.init();
    font_sys.startup_font(config.font, base_dir + "fonts");
}

void App::commit() {
    std::vector<std::string> vals = fields;
    for (auto& v : vals) {
        size_t f = v.find_first_not_of(" \t\r\n");
        if (f == std::string::npos) v.clear();
        else {
            size_t l = v.find_last_not_of(" \t\r\n");
            v = v.substr(f, l - f + 1);
        }
    }

    auto snap = glossary.rows;

    std::string ok_msg;
    if (editing_index.has_value()) {
        glossary.set_row(glossary.rows[editing_index.value()], vals);
        ok_msg = "¡Fila #" + std::to_string(editing_index.value() + 1) + " actualizada con éxito!";
    } else {
        std::vector<std::string> nueva(glossary.header.size(), "");
        glossary.set_row(nueva, vals);
        glossary.rows.push_back(nueva);
        ok_msg = "¡Guardado exitoso!";
    }

    auto [ok, err] = glossary.save(glos_path, backup_dir);
    if (ok) {
        status_msg = ok_msg;
        status_color = Color{100, 220, 120, 255};
        editing_index = std::nullopt;
        for (int i = 0; i < Glossary::NF; ++i) {
            fields[i].clear();
        }
        active_field = 0;
    } else {
        glossary.rows = snap;
        status_msg = "No se pudo guardar (¿abierto en Excel?): " + err;
        status_color = Color{240, 100, 100, 255};
    }
}

void App::change_font_size(int delta) {
    int old_sz = font_size;
    font_size = std::clamp(font_size + delta, 10, 30);
    if (font_size != old_sz) {
        config.glossary_size = font_size;
        ConfigManager::save(config);
        status_msg = "Tamaño de letra (toda la app): " + std::to_string(font_size) + "px (Ctrl +/-)";
        status_color = Color{100, 220, 255, 255};
    }
}

void App::reset_font_size() {
    font_size = 15;
    config.glossary_size = font_size;
    ConfigManager::save(config);
    status_msg = "Tamaño de letra restablecido: 15px";
    status_color = Color{100, 220, 255, 255};
}

void App::run() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(WIN_W, WIN_H, "Termen");
    SetWindowMinSize(1000, 640);
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    init_paths();

    while (!WindowShouldClose() && !should_close) {
        cursor_timer += GetFrameTime();
        if (cursor_timer >= 1.0f) cursor_timer -= 1.0f;

        handle_input();
        render();
    }

    font_sys.cleanup();
    CloseWindow();
}

Layout App::compute_layout(size_t displayed_rows_count) {
    Layout lay;
    int cur_w = GetScreenWidth();
    int cur_h = GetScreenHeight();
    if (cur_w <= 0) cur_w = WIN_W;
    if (cur_h <= 0) cur_h = WIN_H;
    lay.win_w = cur_w;
    lay.win_h = cur_h;

    lay.margin_x = std::max(28.0f, static_cast<float>(cur_w) * 0.032f);
    float content_w = static_cast<float>(cur_w) - 2.0f * lay.margin_x;

    // 1. Barra superior
    lay.top_bar_h = std::max(50.0f, static_cast<float>(font_size) + 24.0f);
    float btn_h = std::max(30.0f, static_cast<float>(font_size) + 10.0f);
    float btn_y = (lay.top_bar_h - btn_h) / 2.0f;

    // Botones superiores de derecha a izquierda
    float rx = static_cast<float>(cur_w) - 20.0f;

    if (sort_col.has_value()) {
        float w_sort = std::max(115.0f, font_sys.measure_text("Orden Orig.", font_size) + 18.0f);
        rx -= w_sort;
        lay.btn_reset_sort = Rectangle{rx, btn_y, w_sort, btn_h};
        rx -= 8.0f;
    }

    if (search_mode) {
        float w_clear = std::max(80.0f, font_sys.measure_text("Limpiar", font_size) + 18.0f);
        rx -= w_clear;
        lay.btn_clear_rec = Rectangle{rx, btn_y, w_clear, btn_h};
        rx -= 8.0f;
    }

    float w_search = std::max(120.0f, font_sys.measure_text("BUSCAR (F3)", font_size) + 20.0f);
    rx -= w_search;
    lay.btn_search_rec = Rectangle{rx, btn_y, w_search, btn_h};
    rx -= 8.0f;

    std::string font_pref_lbl = "Aa: " + font_sys.current_font_label + " v";
    float w_font = std::clamp(font_sys.measure_text(font_pref_lbl, font_size) + 24.0f, 150.0f, 260.0f);
    rx -= w_font;
    lay.btn_font_rec = Rectangle{rx, btn_y, w_font, btn_h};
    rx -= 6.0f;

    float w_plus = std::max(34.0f, font_sys.measure_text("A+", font_size) + 14.0f);
    rx -= w_plus;
    lay.btn_font_plus = Rectangle{rx, btn_y, w_plus, btn_h};
    rx -= 4.0f;

    float w_minus = std::max(34.0f, font_sys.measure_text("A-", font_size) + 14.0f);
    rx -= w_minus;
    lay.btn_font_minus = Rectangle{rx, btn_y, w_minus, btn_h};

    // Textos izquierdos en barra superior
    lay.title_x = 20.0f;
    lay.title_y = (lay.top_bar_h - static_cast<float>(font_size)) / 2.0f;
    float title_w = font_sys.measure_text("TERMEN", font_size);

    if (search_mode) {
        lay.mode_text = "MODO: FILTRADO ACTIVO";
        lay.mode_color = Color{80, 200, 240, 255};
        lay.count_text = "Vistas: " + std::to_string(displayed_rows_count) + " / " + std::to_string(glossary.rows.size());
        lay.count_color = Color{80, 200, 240, 255};
    } else if (editing_index.has_value()) {
        lay.mode_text = "MODO: EDITANDO FILA #" + std::to_string(editing_index.value() + 1);
        lay.mode_color = Color{240, 180, 50, 255};
        lay.count_text = "Total: " + std::to_string(glossary.rows.size());
        lay.count_color = Color{160, 160, 160, 255};
    } else {
        lay.mode_text = "MODO: REGISTRO";
        lay.mode_color = Color{140, 140, 140, 255};
        lay.count_text = "Total: " + std::to_string(glossary.rows.size());
        lay.count_color = Color{160, 160, 160, 255};
    }

    lay.mode_x = lay.title_x + title_w + 18.0f;
    lay.mode_y = lay.title_y;
    float mode_w = font_sys.measure_text(lay.mode_text, font_size);

    lay.count_x = lay.mode_x + mode_w + 16.0f;
    lay.count_y = lay.title_y;

    // 2. Cuadros de entrada (6 campos)
    float box_h = std::max(36.0f, static_cast<float>(font_size) + 18.0f);
    float label_gap = 5.0f;
    float top_gap = std::max(10.0f, static_cast<float>(font_size) * 0.5f);
    float row_gap = std::max(8.0f, static_cast<float>(font_size) * 0.4f);

    // Fila 1 (2 campos: Término Español, Término Inglés)
    float gap1 = 16.0f;
    float w1 = (content_w - gap1) / 2.0f;
    lay.label_y[0] = lay.top_bar_h + top_gap;
    lay.label_y[1] = lay.label_y[0];
    float r1_box_y = lay.label_y[0] + static_cast<float>(font_size) + label_gap;
    lay.field_rects[0] = Rectangle{lay.margin_x, r1_box_y, w1, box_h};
    lay.field_rects[1] = Rectangle{lay.margin_x + w1 + gap1, r1_box_y, w1, box_h};

    // Fila 2 (4 campos: Sigla ESP, Significado ESP, Sigla ENG, Meaning ENG)
    float gap2 = 14.0f;
    float w2 = (content_w - 3.0f * gap2) / 4.0f;
    lay.label_y[2] = r1_box_y + box_h + row_gap;
    lay.label_y[3] = lay.label_y[2];
    lay.label_y[4] = lay.label_y[2];
    lay.label_y[5] = lay.label_y[2];
    float r2_box_y = lay.label_y[2] + static_cast<float>(font_size) + label_gap;
    for (int i = 0; i < 4; ++i) {
        lay.field_rects[2 + i] = Rectangle{lay.margin_x + static_cast<float>(i) * (w2 + gap2), r2_box_y, w2, box_h};
    }

    // 3. Mensaje de estado
    float status_gap = std::max(8.0f, static_cast<float>(font_size) * 0.4f);
    lay.status_y = r2_box_y + box_h + status_gap;

    // 4. Tabla
    float table_gap = std::max(8.0f, static_cast<float>(font_size) * 0.4f);
    lay.table_x = lay.margin_x;
    lay.table_y = lay.status_y + static_cast<float>(font_size) + table_gap;
    lay.table_w = content_w;
    lay.head_h = std::max(32.0f, static_cast<float>(font_size) + 14.0f);
    lay.sb_w = 14.0f;

    // Pie
    lay.footer_h = std::max(32.0f, static_cast<float>(font_size) + 12.0f);
    lay.footer_y = static_cast<float>(cur_h) - lay.footer_h;

    lay.table_h = std::max(160.0f, (lay.footer_y - 10.0f) - lay.table_y);
    lay.list_y = lay.table_y + lay.head_h;
    lay.list_h = lay.table_h - lay.head_h;
    lay.list_w = lay.table_w - lay.sb_w;
    lay.row_h = std::max(32, font_size + 14);

    // 5. Columnas
    lay.act_btn_w = std::max(60.0f, font_sys.measure_text("Editar", font_size) + 16.0f);
    lay.act_btn_h = std::max(24.0f, static_cast<float>(font_size) + 8.0f);
    lay.act_w = lay.act_btn_w * 2.0f + 24.0f;

    float data_w = lay.list_w - lay.act_w;
    float acr_hdr_w = font_sys.measure_text("ACR. ESP [A-Z]", font_size) + 20.0f;
    float acr_w = std::max(std::max(95.0f, static_cast<float>(font_size) * 5.2f), acr_hdr_w);
    float rem_w = std::max(200.0f, data_w - 2.0f * acr_w);
    float w_text = rem_w / 4.0f;

    lay.col_w[0] = w_text;
    lay.col_w[1] = w_text;
    lay.col_w[2] = acr_w;
    lay.col_w[3] = w_text;
    lay.col_w[4] = acr_w;
    lay.col_w[5] = w_text;

    lay.col_off[0] = 0.0f;
    for (int k = 1; k < 6; ++k) {
        lay.col_off[k] = lay.col_off[k - 1] + lay.col_w[k - 1];
    }
    lay.act_off = lay.col_off[5] + lay.col_w[5];

    return lay;
}

void App::handle_input() {
    Vector2 mouse_pos = GetMousePosition();
    bool click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    float wheel = GetMouseWheelMove();

    // Filas procesadas (filtradas y/o ordenadas)
    auto display_rows = glossary.get_processed_rows(fields, search_mode, sort_col, sort_asc);
    Layout lay = compute_layout(display_rows.size());

    // --------------------------------------------------------------------------
    // MODAL DE CONFIRMACIÓN PARA SALIR (ESC)
    // --------------------------------------------------------------------------
    if (quit_confirm_open) {
        float mw = std::clamp(static_cast<float>(font_size) * 28.0f, 560.0f, static_cast<float>(lay.win_w) - 40.0f);
        float mh = std::clamp(static_cast<float>(font_size) * 7.5f, 155.0f, static_cast<float>(lay.win_h) - 40.0f);
        float mx = (lay.win_w - mw) / 2.0f;
        float my = (lay.win_h - mh) / 2.0f;
        float btn_w = std::max(110.0f, static_cast<float>(font_size) * 6.0f);
        float btn_h = std::max(28.0f, static_cast<float>(font_size) + 10.0f);
        Rectangle btn_quit_ok{mx + mw - 2.0f * btn_w - 24.0f, my + mh - btn_h - 16.0f, btn_w, btn_h};
        Rectangle btn_quit_cancel{mx + mw - btn_w - 16.0f, my + mh - btn_h - 16.0f, btn_w, btn_h};

        // Presionar de nuevo ESC cierra la aplicación
        if (IsKeyPressed(KEY_ESCAPE)) {
            should_close = true;
            return;
        }

        // Cualquier otra tecla cancela y continúa
        if (GetKeyPressed() > 0) {
            quit_confirm_open = false;
            return;
        }

        if (click) {
            if (CheckCollisionPointRec(mouse_pos, btn_quit_ok)) {
                should_close = true;
                return;
            }
            if (CheckCollisionPointRec(mouse_pos, btn_quit_cancel) ||
                !CheckCollisionPointRec(mouse_pos, Rectangle{mx, my, mw, mh})) {
                quit_confirm_open = false;
                return;
            }
        }
        return;
    }

    float content_h = static_cast<float>(display_rows.size() * lay.row_h);
    float max_scroll = std::max(0.0f, content_h - lay.list_h);
    scroll = std::clamp(scroll, 0.0f, max_scroll);

    // Dimensiones y posición de barra de desplazamiento
    bool has_sb = content_h > lay.list_h;
    float thumb_h = has_sb ? std::max(30.0f, (lay.list_h * lay.list_h) / content_h) : lay.list_h;
    float travel = std::max(1.0f, lay.list_h - thumb_h);
    float thumb_y = lay.list_y + (max_scroll > 0.0f ? (scroll / max_scroll * travel) : 0.0f);
    float sb_x = lay.table_x + lay.table_w - lay.sb_w;

    // Manejo del menú de fuentes si está abierto
    if (font_sys.menu_open) {
        float menu_w = std::max(260.0f, lay.btn_font_rec.width);
        float menu_x = lay.btn_font_rec.x;
        if (menu_x + menu_w > lay.win_w - 10.0f) menu_x = lay.win_w - menu_w - 10.0f;
        float menu_y = lay.btn_font_rec.y + lay.btn_font_rec.height + 2.0f;

        if (wheel != 0.0f) {
            font_sys.handle_scroll(wheel, menu_x, menu_y, menu_w, mouse_pos.x, mouse_pos.y);
        }

        if (click) {
            if (font_sys.handle_click(menu_x, menu_y, menu_w, mouse_pos.x, mouse_pos.y)) {
                config.font = font_sys.current_font_pref;
                ConfigManager::save(config);
                return;
            }
            if (!CheckCollisionPointRec(mouse_pos, lay.btn_font_rec)) {
                font_sys.menu_open = false;
            }
        }
    }

    if (state == STATE_EDITING) {
        bool ctrl_down = (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && !IsKeyDown(KEY_RIGHT_ALT);
        bool shift_down = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

        // Cambio de tamaño de fuente unificado para toda la aplicación con Ctrl + "+/-"
        if (ctrl_down) {
            bool zoom_in = IsKeyPressed(KEY_KP_ADD) || IsKeyPressedRepeat(KEY_KP_ADD) ||
                           IsKeyPressed(KEY_EQUAL) || IsKeyPressedRepeat(KEY_EQUAL) ||
                           IsKeyPressed(KEY_RIGHT_BRACKET) || IsKeyPressedRepeat(KEY_RIGHT_BRACKET);
            bool zoom_out = IsKeyPressed(KEY_KP_SUBTRACT) || IsKeyPressedRepeat(KEY_KP_SUBTRACT) ||
                            IsKeyPressed(KEY_MINUS) || IsKeyPressedRepeat(KEY_MINUS) ||
                            IsKeyPressed(KEY_SLASH) || IsKeyPressedRepeat(KEY_SLASH);

            if (wheel > 0.0f) zoom_in = true;
            else if (wheel < 0.0f) zoom_out = true;

            if (zoom_in) {
                change_font_size(+1);
            } else if (zoom_out) {
                change_font_size(-1);
            } else if (IsKeyPressed(KEY_ZERO) || IsKeyPressed(KEY_KP_0)) {
                reset_font_size();
            }
        }

        // Clics en botones A- y A+
        bool hov_font_minus = CheckCollisionPointRec(mouse_pos, lay.btn_font_minus);
        bool hov_font_plus = CheckCollisionPointRec(mouse_pos, lay.btn_font_plus);
        if (hov_font_minus && click) {
            change_font_size(-1);
        } else if (hov_font_plus && click) {
            change_font_size(+1);
        }

        bool hov_font_btn = CheckCollisionPointRec(mouse_pos, lay.btn_font_rec);
        bool hov_search_btn = CheckCollisionPointRec(mouse_pos, lay.btn_search_rec);
        bool hov_clear_btn = CheckCollisionPointRec(mouse_pos, lay.btn_clear_rec) && search_mode;
        bool hov_reset_sort = CheckCollisionPointRec(mouse_pos, lay.btn_reset_sort) && sort_col.has_value();

        // Botón selector de fuente
        if (hov_font_btn && click) {
            font_sys.menu_open = !font_sys.menu_open;
            if (font_sys.menu_open) {
                font_sys.scan_fonts();
            }
        }

        // Alternar modo búsqueda
        bool toggle_search = (hov_search_btn && click) || IsKeyPressed(KEY_F3) || (ctrl_down && IsKeyPressed(KEY_F));
        if (toggle_search) {
            search_mode = !search_mode;
            scroll = 0.0f;
            if (search_mode) {
                editing_index = std::nullopt;
                for (int i = 0; i < Glossary::NF; ++i) fields[i].clear();
                active_field = 0;
                status_msg = "Modo Búsqueda ACTIVADO: escriba en los campos para filtrar.";
                status_color = Color{80, 200, 240, 255};
            } else {
                for (int i = 0; i < Glossary::NF; ++i) fields[i].clear();
                active_field = 0;
                status_msg = "Modo Registro ACTIVADO.";
                status_color = Color{180, 180, 180, 255};
            }
        }

        // Botón limpiar filtros en modo búsqueda
        if (hov_clear_btn && click && search_mode) {
            for (int i = 0; i < Glossary::NF; ++i) fields[i].clear();
            scroll = 0.0f;
            status_msg = "Filtros de búsqueda limpiados.";
            status_color = Color{80, 200, 240, 255};
        }

        // Botón restablecer orden alfabético
        if (hov_reset_sort && click) {
            sort_col = std::nullopt;
            sort_asc = true;
            scroll = 0.0f;
            status_msg = "Orden restablecido al original del archivo CSV.";
            status_color = Color{180, 180, 180, 255};
        }

        // Clic en los encabezados de la tabla para ordenar
        if (click && !font_sys.menu_open &&
            mouse_pos.x >= lay.table_x && mouse_pos.x < lay.table_x + lay.list_w &&
            mouse_pos.y >= lay.table_y && mouse_pos.y < lay.table_y + lay.head_h) {
            for (int k = 0; k < 6; ++k) {
                float hx = lay.table_x + lay.col_off[k];
                float hw = lay.col_w[k];
                if (mouse_pos.x >= hx && mouse_pos.x < hx + hw) {
                    if (sort_col.has_value() && sort_col.value() == k) {
                        if (sort_asc) {
                            sort_asc = false;
                            status_msg = "Ordenando '" + Glossary::COL_TITLES[k] + "' de Z a A.";
                        } else {
                            sort_col = std::nullopt;
                            sort_asc = true;
                            status_msg = "Orden restablecido al original del archivo CSV.";
                        }
                    } else {
                        sort_col = k;
                        sort_asc = true;
                        status_msg = "Ordenando '" + Glossary::COL_TITLES[k] + "' de A a Z.";
                    }
                    scroll = 0.0f;
                    status_color = search_mode ? Color{80, 200, 240, 255} : Color{230, 140, 30, 255};
                    break;
                }
            }
        }

        // Clic sobre cuadro de texto para activarlo
        if (click && !font_sys.menu_open) {
            for (int i = 0; i < Glossary::NF; ++i) {
                if (CheckCollisionPointRec(mouse_pos, lay.field_rects[i])) {
                    active_field = i;
                }
            }
        }

        // Atajos de edición en cuadro activo
        if (ctrl_down && IsKeyPressed(KEY_X)) {
            fields[active_field].clear();
            status_msg = "Texto del cuadro borrado.";
            status_color = Color{240, 180, 80, 255};
        } else if (ctrl_down && IsKeyPressed(KEY_V)) {
            const char* clip = GetClipboardText();
            if (clip) {
                std::string t = clip;
                for (char& c : t) {
                    if (c == '\r' || c == '\n') c = ' ';
                }
                while (TextUtils::utf8_length(fields[active_field] + t) > MAX_LEN && !t.empty()) {
                    TextUtils::pop_back_utf8(t);
                }
                fields[active_field] += t;
            }
        } else if (IsKeyPressed(KEY_TAB)) {
            active_field = (active_field + (shift_down ? -1 : 1) + Glossary::NF) % Glossary::NF;
        } else if (IsKeyPressed(KEY_UP)) {
            active_field = (active_field - 1 + Glossary::NF) % Glossary::NF;
        } else if (IsKeyPressed(KEY_DOWN)) {
            active_field = (active_field + 1) % Glossary::NF;
        } else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            if (search_mode) {
                active_field = (active_field + 1) % Glossary::NF;
            } else {
                std::vector<std::string> vals = fields;
                for (auto& v : vals) {
                    size_t f = v.find_first_not_of(" \t\r\n");
                    if (f == std::string::npos) v.clear();
                    else {
                        size_t l = v.find_last_not_of(" \t\r\n");
                        v = v.substr(f, l - f + 1);
                    }
                }
                auto err = Glossary::validate(vals);
                if (!err.has_value()) {
                    similar_terms = glossary.find_similar_terms(vals, editing_index);
                    if (!similar_terms.empty()) {
                        state = STATE_CONFIRM;
                    } else {
                        commit();
                    }
                } else {
                    status_msg = err.value();
                    status_color = Color{240, 180, 80, 255};
                    if (!vals[active_field].empty() && active_field < Glossary::NF - 1) {
                        active_field++;
                    }
                }
            }
        } else if (IsKeyPressed(KEY_ESCAPE)) {
            if (font_sys.menu_open) {
                font_sys.menu_open = false;
            } else if (search_mode) {
                bool has_any = false;
                for (const auto& f : fields) {
                    if (!f.empty()) { has_any = true; break; }
                }
                if (has_any) {
                    for (int i = 0; i < Glossary::NF; ++i) fields[i].clear();
                    scroll = 0.0f;
                    status_msg = "Filtro limpiado.";
                    status_color = Color{80, 200, 240, 255};
                } else {
                    search_mode = false;
                    status_msg = "Modo Búsqueda cerrado.";
                    status_color = Color{180, 180, 180, 255};
                }
            } else if (editing_index.has_value()) {
                editing_index = std::nullopt;
                for (int i = 0; i < Glossary::NF; ++i) fields[i].clear();
                status_msg = "Edición cancelada.";
                status_color = Color{240, 180, 80, 255};
            } else {
                // Confirmación para salir
                quit_confirm_open = true;
            }
        }

        // Entrada de caracteres de texto UTF-8
        int ch = GetCharPressed();
        while (ch > 0) {
            if (!ctrl_down && ((ch >= 32 && ch <= 126) || (ch >= 160 && ch <= 255) || ch == 0x00BF || ch == 0x00A1)) {
                if (TextUtils::utf8_length(fields[active_field]) < MAX_LEN) {
                    fields[active_field] += TextUtils::encode_utf8(ch);
                }
            }
            ch = GetCharPressed();
        }

        // Tecla Backspace (con repetición de pulsación)
        if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && !ctrl_down) {
            if (!fields[active_field].empty()) {
                TextUtils::pop_back_utf8(fields[active_field]);
            }
        }

        // Rueda del ratón sobre la tabla
        bool mouse_in_table = (mouse_pos.x >= lay.table_x && mouse_pos.x < lay.table_x + lay.table_w &&
                               mouse_pos.y >= lay.list_y && mouse_pos.y < lay.list_y + lay.list_h);
        if (wheel != 0.0f && mouse_in_table && !font_sys.menu_open) {
            scroll = std::clamp(scroll - wheel * 36.0f, 0.0f, max_scroll);
        }

        // Interacción con la barra de desplazamiento
        if (has_sb && !font_sys.menu_open) {
            if (click && mouse_pos.x >= sb_x && mouse_pos.x < sb_x + lay.sb_w &&
                mouse_pos.y >= lay.list_y && mouse_pos.y < lay.list_y + lay.list_h) {
                if (mouse_pos.y >= thumb_y && mouse_pos.y <= thumb_y + thumb_h) {
                    sb_drag = true;
                    sb_grab = mouse_pos.y - thumb_y;
                } else if (mouse_pos.y < thumb_y) {
                    scroll = std::max(0.0f, scroll - lay.list_h);
                } else {
                    scroll = std::min(max_scroll, scroll + lay.list_h);
                }
            }
            if (sb_drag) {
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                    float pos = (mouse_pos.y - sb_grab - lay.list_y) / travel;
                    scroll = std::clamp(pos * max_scroll, 0.0f, max_scroll);
                } else {
                    sb_drag = false;
                }
            }
        } else {
            sb_drag = false;
        }

        // Interacción con los botones de acción en filas ("Editar" y "Borrar")
        bool in_list = (mouse_pos.x >= lay.table_x && mouse_pos.x < lay.table_x + lay.list_w &&
                        mouse_pos.y >= lay.list_y && mouse_pos.y < lay.list_y + lay.list_h) && !font_sys.menu_open;
        if (click && in_list && !font_sys.menu_open) {
            for (size_t disp_idx = 0; disp_idx < display_rows.size(); ++disp_idx) {
                float row_y = lay.list_y + static_cast<float>(disp_idx * lay.row_h) - scroll;
                if (row_y + lay.row_h < lay.list_y || row_y > lay.list_y + lay.list_h) {
                    continue;
                }

                size_t orig_idx = display_rows[disp_idx].first;
                float btn_y = row_y + (static_cast<float>(lay.row_h) - lay.act_btn_h) / 2.0f;
                Rectangle be{lay.table_x + lay.act_off + 8.0f, btn_y, lay.act_btn_w, lay.act_btn_h};
                Rectangle bd{lay.table_x + lay.act_off + 14.0f + lay.act_btn_w, btn_y, lay.act_btn_w, lay.act_btn_h};

                if (CheckCollisionPointRec(mouse_pos, be)) {
                    search_mode = false;
                    editing_index = orig_idx;
                    auto vals = glossary.row_values(glossary.rows[orig_idx]);
                    for (int k = 0; k < Glossary::NF; ++k) {
                        fields[k] = vals[k];
                    }
                    active_field = 0;
                    status_msg = "Editando fila #" + std::to_string(orig_idx + 1) + ". Modifique los campos y presione Enter.";
                    status_color = Color{240, 180, 50, 255};
                    break;
                }

                if (CheckCollisionPointRec(mouse_pos, bd)) {
                    auto snap = glossary.rows;
                    glossary.rows.erase(glossary.rows.begin() + orig_idx);
                    auto [ok, err] = glossary.save(glos_path, backup_dir);
                    if (ok) {
                        if (editing_index.has_value()) {
                            if (editing_index.value() == orig_idx) {
                                editing_index = std::nullopt;
                                for (int k = 0; k < Glossary::NF; ++k) fields[k].clear();
                            } else if (editing_index.value() > orig_idx) {
                                editing_index = editing_index.value() - 1;
                            }
                        }
                        status_msg = "Fila eliminada correctamente.";
                        status_color = Color{240, 100, 100, 255};
                    } else {
                        glossary.rows = snap;
                        status_msg = "No se pudo guardar (¿abierto en Excel?): " + err;
                        status_color = Color{240, 100, 100, 255};
                    }
                    break;
                }
            }
        }
    } else if (state == STATE_CONFIRM) {
        sb_drag = false;
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_S)) {
            commit();
            state = STATE_EDITING;
        } else if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_N)) {
            status_msg = "Guardado cancelado. Puede modificar los términos.";
            status_color = Color{240, 180, 80, 255};
            state = STATE_EDITING;
        }
    }
}

void App::render() {
    Vector2 mouse_pos = GetMousePosition();
    auto display_rows = glossary.get_processed_rows(fields, search_mode, sort_col, sort_asc);
    Layout lay = compute_layout(display_rows.size());

    float content_h = static_cast<float>(display_rows.size() * lay.row_h);
    float max_scroll = std::max(0.0f, content_h - lay.list_h);
    bool has_sb = content_h > lay.list_h;
    float thumb_h = has_sb ? std::max(30.0f, (lay.list_h * lay.list_h) / content_h) : lay.list_h;
    float travel = std::max(1.0f, lay.list_h - thumb_h);
    float thumb_y = lay.list_y + (max_scroll > 0.0f ? (scroll / max_scroll * travel) : 0.0f);
    float sb_x = lay.table_x + lay.table_w - lay.sb_w;

    BeginDrawing();
    ClearBackground(Color{25, 25, 25, 255});

    // --------------------------------------------------------------------------
    // ENCABEZADO SUPERIOR
    // --------------------------------------------------------------------------
    DrawRectangle(0, 0, lay.win_w, static_cast<int>(lay.top_bar_h), Color{15, 15, 15, 255});
    font_sys.draw_text("TERMEN", lay.title_x, lay.title_y, font_size, WHITE);
    font_sys.draw_text(lay.mode_text, lay.mode_x, lay.mode_y, font_size, lay.mode_color);
    font_sys.draw_text(lay.count_text, lay.count_x, lay.count_y, font_size, lay.count_color);

    // Botón "A-"
    bool hov_f_minus = CheckCollisionPointRec(mouse_pos, lay.btn_font_minus);
    DrawRectangleRec(lay.btn_font_minus, hov_f_minus ? Color{55, 55, 55, 255} : Color{35, 35, 35, 255});
    DrawRectangleLinesEx(lay.btn_font_minus, 1.0f, Color{90, 90, 90, 255});
    float t_a_minus_w = font_sys.measure_text("A-", font_size);
    font_sys.draw_text("A-", lay.btn_font_minus.x + (lay.btn_font_minus.width - t_a_minus_w) / 2.0f,
                       lay.btn_font_minus.y + (lay.btn_font_minus.height - font_size) / 2.0f, font_size, WHITE);

    // Botón "A+"
    bool hov_f_plus = CheckCollisionPointRec(mouse_pos, lay.btn_font_plus);
    DrawRectangleRec(lay.btn_font_plus, hov_f_plus ? Color{55, 55, 55, 255} : Color{35, 35, 35, 255});
    DrawRectangleLinesEx(lay.btn_font_plus, 1.0f, Color{90, 90, 90, 255});
    float t_a_plus_w = font_sys.measure_text("A+", font_size);
    font_sys.draw_text("A+", lay.btn_font_plus.x + (lay.btn_font_plus.width - t_a_plus_w) / 2.0f,
                       lay.btn_font_plus.y + (lay.btn_font_plus.height - font_size) / 2.0f, font_size, WHITE);

    // Botón Selector de Fuente
    bool hov_font_btn = CheckCollisionPointRec(mouse_pos, lay.btn_font_rec);
    Color font_btn_bg = font_sys.menu_open ? Color{200, 110, 20, 255} : (hov_font_btn ? Color{55, 55, 55, 255} : Color{35, 35, 35, 255});
    DrawRectangleRec(lay.btn_font_rec, font_btn_bg);
    DrawRectangleLinesEx(lay.btn_font_rec, 1.0f, font_sys.menu_open ? Color{250, 160, 50, 255} : Color{90, 90, 90, 255});
    std::string font_btn_lbl = font_sys.fit_text("Aa: " + font_sys.current_font_label, font_size, lay.btn_font_rec.width - 24.0f) + " v";
    float t_font_lbl_w = font_sys.measure_text(font_btn_lbl, font_size);
    font_sys.draw_text(font_btn_lbl, lay.btn_font_rec.x + (lay.btn_font_rec.width - t_font_lbl_w) / 2.0f,
                       lay.btn_font_rec.y + (lay.btn_font_rec.height - font_size) / 2.0f, font_size, WHITE);

    // Botón "Modo Búsqueda"
    bool hov_search_btn = CheckCollisionPointRec(mouse_pos, lay.btn_search_rec);
    if (search_mode) {
        Color b_bg = hov_search_btn ? Color{35, 130, 190, 255} : Color{20, 95, 150, 255};
        DrawRectangleRec(lay.btn_search_rec, b_bg);
        DrawRectangleLinesEx(lay.btn_search_rec, 1.0f, Color{80, 200, 255, 255});
        float t_search_w = font_sys.measure_text("BUSCAR (F3)", font_size);
        font_sys.draw_text("BUSCAR (F3)", lay.btn_search_rec.x + (lay.btn_search_rec.width - t_search_w) / 2.0f,
                           lay.btn_search_rec.y + (lay.btn_search_rec.height - font_size) / 2.0f, font_size, WHITE);
    } else {
        Color b_bg = hov_search_btn ? Color{50, 50, 50, 255} : Color{35, 35, 35, 255};
        DrawRectangleRec(lay.btn_search_rec, b_bg);
        DrawRectangleLinesEx(lay.btn_search_rec, 1.0f, Color{80, 80, 80, 255});
        float t_search_w = font_sys.measure_text("BUSCAR (F3)", font_size);
        font_sys.draw_text("BUSCAR (F3)", lay.btn_search_rec.x + (lay.btn_search_rec.width - t_search_w) / 2.0f,
                           lay.btn_search_rec.y + (lay.btn_search_rec.height - font_size) / 2.0f, font_size, Color{200, 200, 200, 255});
    }

    // Botón "Limpiar" (modo búsqueda)
    if (search_mode) {
        bool hov_clear_btn = CheckCollisionPointRec(mouse_pos, lay.btn_clear_rec);
        Color c_bg = hov_clear_btn ? Color{180, 60, 60, 255} : Color{120, 40, 40, 255};
        DrawRectangleRec(lay.btn_clear_rec, c_bg);
        DrawRectangleLinesEx(lay.btn_clear_rec, 1.0f, Color{220, 80, 80, 255});
        float t_clear_w = font_sys.measure_text("Limpiar", font_size);
        font_sys.draw_text("Limpiar", lay.btn_clear_rec.x + (lay.btn_clear_rec.width - t_clear_w) / 2.0f,
                           lay.btn_clear_rec.y + (lay.btn_clear_rec.height - font_size) / 2.0f, font_size, WHITE);
    }

    // Botón "Orden Original" (si hay orden aplicado)
    if (sort_col.has_value()) {
        bool hov_reset_sort = CheckCollisionPointRec(mouse_pos, lay.btn_reset_sort);
        Color r_bg = hov_reset_sort ? Color{70, 130, 70, 255} : Color{45, 90, 45, 255};
        DrawRectangleRec(lay.btn_reset_sort, r_bg);
        DrawRectangleLinesEx(lay.btn_reset_sort, 1.0f, Color{100, 220, 120, 255});
        float t_sort_w = font_sys.measure_text("Orden Orig.", font_size);
        font_sys.draw_text("Orden Orig.", lay.btn_reset_sort.x + (lay.btn_reset_sort.width - t_sort_w) / 2.0f,
                           lay.btn_reset_sort.y + (lay.btn_reset_sort.height - font_size) / 2.0f, font_size, WHITE);
    }

    // --------------------------------------------------------------------------
    // CUADROS DE TEXTO Y ETIQUETAS
    // --------------------------------------------------------------------------
    const auto& current_labels = search_mode ? Glossary::LABELS_SEARCH : Glossary::LABELS;
    for (int i = 0; i < Glossary::NF; ++i) {
        bool on = (active_field == i);
        Rectangle r = lay.field_rects[i];

        Color lbl_c = search_mode ? Color{80, 200, 240, 255} : Color{200, 200, 200, 255};
        font_sys.draw_text(current_labels[i], r.x, lay.label_y[i], font_size, lbl_c);

        Color box_bg = on ? Color{40, 40, 40, 255} : Color{30, 30, 30, 255};
        DrawRectangleRec(r, box_bg);

        Color border_c;
        if (search_mode) {
            border_c = on ? Color{80, 200, 240, 255} : Color{40, 80, 100, 255};
        } else {
            border_c = on ? Color{230, 140, 30, 255} : Color{70, 70, 70, 255};
        }
        DrawRectangleLinesEx(r, on ? 2.0f : 1.0f, border_c);

        std::string txt_to_render = fields[i];
        bool show_cursor = on && (cursor_timer < 0.5f);
        if (show_cursor) {
            txt_to_render += "|";
        }
        std::string disp_txt = font_sys.tail_text(txt_to_render, font_size, r.width - 24.0f);
        font_sys.draw_text(disp_txt, r.x + 10.0f, r.y + (r.height - static_cast<float>(font_size)) / 2.0f, font_size, WHITE);
    }

    // Mensaje de estado
    std::string fit_status = font_sys.fit_text(status_msg, font_size, lay.table_w);
    font_sys.draw_text(fit_status, lay.table_x, lay.status_y, font_size, status_color);

    // --------------------------------------------------------------------------
    // TABLA CON BOTONES DE ENCABEZADO Y FILAS
    // --------------------------------------------------------------------------
    DrawRectangleLinesEx(Rectangle{lay.table_x, lay.table_y, lay.table_w, lay.table_h}, 1.0f, Color{70, 70, 70, 255});

    // Barra de encabezados interactivos
    DrawRectangle(static_cast<int>(lay.table_x), static_cast<int>(lay.table_y), static_cast<int>(lay.table_w), static_cast<int>(lay.head_h), Color{35, 35, 35, 255});
    DrawLine(static_cast<int>(lay.table_x), static_cast<int>(lay.table_y + lay.head_h),
             static_cast<int>(lay.table_x + lay.table_w), static_cast<int>(lay.table_y + lay.head_h), Color{80, 80, 80, 255});

    for (int k = 0; k < 6; ++k) {
        Rectangle h_rec{lay.table_x + lay.col_off[k], lay.table_y, lay.col_w[k], lay.head_h};
        bool hov_h = CheckCollisionPointRec(mouse_pos, h_rec) && (state == STATE_EDITING) && !font_sys.menu_open && !quit_confirm_open;
        bool is_sorted = sort_col.has_value() && (sort_col.value() == k);

        Color h_bg;
        if (is_sorted) {
            h_bg = search_mode ? Color{40, 70, 95, 255} : Color{80, 50, 20, 255};
        } else if (hov_h) {
            h_bg = Color{55, 55, 55, 255};
        } else {
            h_bg = Color{35, 35, 35, 255};
        }
        DrawRectangleRec(h_rec, h_bg);

        std::string txt_title = Glossary::COL_TITLES[k];
        Color t_color;
        if (is_sorted) {
            txt_title += sort_asc ? " [A-Z]" : " [Z-A]";
            t_color = search_mode ? Color{100, 220, 255, 255} : Color{255, 190, 50, 255};
        } else if (hov_h) {
            t_color = WHITE;
        } else {
            t_color = search_mode ? Color{80, 200, 240, 255} : Color{230, 140, 30, 255};
        }

        std::string fit_hdr = font_sys.fit_text(txt_title, font_size, h_rec.width - 16.0f);
        font_sys.draw_text(fit_hdr, h_rec.x + 8.0f, lay.table_y + (lay.head_h - static_cast<float>(font_size)) / 2.0f, font_size, t_color);
    }

    // Columna "ACCIONES"
    font_sys.draw_text("ACCIONES", lay.table_x + lay.act_off + 12.0f, lay.table_y + (lay.head_h - static_cast<float>(font_size)) / 2.0f, font_size,
                       search_mode ? Color{80, 200, 240, 255} : Color{230, 140, 30, 255});

    // Separadores verticales de columnas
    for (int k = 1; k < 6; ++k) {
        float off = lay.col_off[k];
        DrawLine(static_cast<int>(lay.table_x + off), static_cast<int>(lay.table_y),
                 static_cast<int>(lay.table_x + off), static_cast<int>(lay.table_y + lay.table_h), Color{60, 60, 60, 255});
    }
    DrawLine(static_cast<int>(lay.table_x + lay.act_off), static_cast<int>(lay.table_y),
             static_cast<int>(lay.table_x + lay.act_off), static_cast<int>(lay.table_y + lay.table_h), Color{60, 60, 60, 255});

    // Filas renderizadas dentro del área con tijera (Scissor)
    bool in_list = (mouse_pos.x >= lay.table_x && mouse_pos.x < lay.table_x + lay.list_w &&
                    mouse_pos.y >= lay.list_y && mouse_pos.y < lay.list_y + lay.list_h) && !font_sys.menu_open && !quit_confirm_open;

    BeginScissorMode(static_cast<int>(lay.table_x), static_cast<int>(lay.list_y), static_cast<int>(lay.list_w), static_cast<int>(lay.list_h));
    if (display_rows.empty()) {
        font_sys.draw_text("No se encontraron registros con los criterios ingresados.",
                           lay.table_x + 30.0f, lay.list_y + 30.0f, font_size, Color{140, 140, 140, 255});
    } else {
        for (size_t disp_idx = 0; disp_idx < display_rows.size(); ++disp_idx) {
            float row_y = lay.list_y + static_cast<float>(disp_idx * lay.row_h) - scroll;
            if (row_y + lay.row_h < lay.list_y || row_y > lay.list_y + lay.list_h) {
                continue;
            }

            size_t orig_idx = display_rows[disp_idx].first;
            const auto& row_data = display_rows[disp_idx].second;

            Color bg_row = (disp_idx % 2 == 0) ? Color{30, 30, 30, 255} : Color{22, 22, 22, 255};
            if (editing_index.has_value() && editing_index.value() == orig_idx) {
                bg_row = Color{60, 50, 20, 255};
            }
            DrawRectangle(static_cast<int>(lay.table_x + 1), static_cast<int>(row_y), static_cast<int>(lay.list_w - 1), lay.row_h - 1, bg_row);

            auto vals = glossary.row_values(row_data);
            float text_y = row_y + (static_cast<float>(lay.row_h) - static_cast<float>(font_size)) / 2.0f;
            for (int k = 0; k < 6; ++k) {
                std::string cell_txt = font_sys.fit_text(vals[k], font_size, lay.col_w[k] - 20.0f);
                font_sys.draw_text(cell_txt, lay.table_x + lay.col_off[k] + 10.0f, text_y, font_size, WHITE);
            }

            // Botón "Editar"
            float btn_y = row_y + (static_cast<float>(lay.row_h) - lay.act_btn_h) / 2.0f;
            Rectangle be{lay.table_x + lay.act_off + 8.0f, btn_y, lay.act_btn_w, lay.act_btn_h};
            bool hov_e = in_list && CheckCollisionPointRec(mouse_pos, be) && (state == STATE_EDITING);
            DrawRectangleRec(be, hov_e ? Color{70, 150, 220, 255} : Color{40, 90, 140, 255});
            float t_edit_w = font_sys.measure_text("Editar", font_size);
            font_sys.draw_text("Editar", be.x + (be.width - t_edit_w) / 2.0f, be.y + (be.height - static_cast<float>(font_size)) / 2.0f, font_size, WHITE);

            // Botón "Borrar"
            Rectangle bd{lay.table_x + lay.act_off + 14.0f + lay.act_btn_w, btn_y, lay.act_btn_w, lay.act_btn_h};
            bool hov_d = in_list && CheckCollisionPointRec(mouse_pos, bd) && (state == STATE_EDITING);
            DrawRectangleRec(bd, hov_d ? Color{220, 70, 70, 255} : Color{140, 40, 40, 255});
            float t_del_w = font_sys.measure_text("Borrar", font_size);
            font_sys.draw_text("Borrar", bd.x + (bd.width - t_del_w) / 2.0f, bd.y + (bd.height - static_cast<float>(font_size)) / 2.0f, font_size, WHITE);
        }
    }
    EndScissorMode();

    // Barra de desplazamiento lateral
    DrawRectangle(static_cast<int>(sb_x), static_cast<int>(lay.list_y), static_cast<int>(lay.sb_w), static_cast<int>(lay.list_h), Color{18, 18, 18, 255});
    DrawLine(static_cast<int>(sb_x), static_cast<int>(lay.list_y), static_cast<int>(sb_x), static_cast<int>(lay.list_y + lay.list_h), Color{60, 60, 60, 255});

    if (has_sb) {
        bool hov_t = sb_drag || (mouse_pos.x >= sb_x && mouse_pos.x < sb_x + lay.sb_w &&
                                 mouse_pos.y >= thumb_y && mouse_pos.y <= thumb_y + thumb_h);
        Color thumb_color = search_mode ? Color{80, 200, 240, 255} : Color{230, 140, 30, 255};
        DrawRectangle(static_cast<int>(sb_x + 2), static_cast<int>(thumb_y + 1), static_cast<int>(lay.sb_w - 4), static_cast<int>(thumb_h - 2),
                      hov_t ? thumb_color : Color{110, 110, 110, 255});
    }

    // Pie con atajos e instrucciones
    DrawRectangle(0, static_cast<int>(lay.footer_y), lay.win_w, static_cast<int>(lay.footer_h), Color{15, 15, 15, 255});
    std::string hints = "[Ctrl +/-] Tamaño letra | [Esc] Salir | [F3] Buscar | [Clic columna] Ordenar | [TAB] Campo | [Enter] Guardar";
    if (font_sys.measure_text(hints, font_size) > static_cast<float>(lay.win_w) - 40.0f) {
        hints = "[Ctrl +/-] Zoom | [Esc] Salir | [F3] Buscar | [Clic col] Ordenar | [TAB] Campo | [Enter] Guardar";
    }
    if (font_sys.measure_text(hints, font_size) > static_cast<float>(lay.win_w) - 40.0f) {
        hints = "[Ctrl +/-] Zoom | [Esc] Salir | [F3] Buscar | [Enter] Guardar";
    }
    font_sys.draw_text(hints, lay.margin_x, lay.footer_y + (lay.footer_h - static_cast<float>(font_size)) / 2.0f, font_size, Color{150, 150, 150, 255});

    // Menú desplegable de fuentes (dibujado encima de la tabla si está activo)
    if (font_sys.menu_open) {
        float menu_w = std::max(260.0f, lay.btn_font_rec.width);
        float menu_x = lay.btn_font_rec.x;
        if (menu_x + menu_w > lay.win_w - 10.0f) menu_x = lay.win_w - menu_w - 10.0f;
        float menu_y = lay.btn_font_rec.y + lay.btn_font_rec.height + 2.0f;
        font_sys.draw_menu(menu_x, menu_y, menu_w, mouse_pos.x, mouse_pos.y);
    }

    // --------------------------------------------------------------------------
    // VENTANA MODAL DE CONFIRMACIÓN (TÉRMINOS SIMILARES)
    // --------------------------------------------------------------------------
    if (state == STATE_CONFIRM) {
        DrawRectangle(0, 0, lay.win_w, lay.win_h, Color{0, 0, 0, 180});

        float mw = std::clamp(static_cast<float>(font_size) * 42.0f, 850.0f, static_cast<float>(lay.win_w) - 40.0f);
        float mh = std::clamp(static_cast<float>(font_size) * 17.0f, 360.0f, static_cast<float>(lay.win_h) - 40.0f);
        float mx = (lay.win_w - mw) / 2.0f;
        float my = (lay.win_h - mh) / 2.0f;

        DrawRectangle(static_cast<int>(mx), static_cast<int>(my), static_cast<int>(mw), static_cast<int>(mh), Color{25, 25, 25, 255});
        DrawRectangleLinesEx(Rectangle{mx, my, mw, mh}, 2.0f, Color{230, 140, 30, 255});

        float line_h = static_cast<float>(font_size) + 8.0f;
        font_sys.draw_text("¡ATENCIÓN! TÉRMINOS SIMILARES DETECTADOS", mx + 20.0f, my + 18.0f, font_size, Color{240, 180, 50, 255});
        font_sys.draw_text("Lo que intenta ingresar se parece a los siguientes registros:", mx + 20.0f, my + 18.0f + line_h, font_size, LIGHTGRAY);

        size_t show_count = std::min(similar_terms.size(), size_t(5));
        for (size_t k = 0; k < show_count; ++k) {
            const auto& c = similar_terms[k].values;
            int pct = static_cast<int>(similar_terms[k].score * 100.0);

            std::string es_val = c[0].empty() ? "-" : c[0];
            std::string en_val = c[1].empty() ? "-" : c[1];
            std::string linea = "• ES: " + es_val + "  <--->  EN: " + en_val;

            if (!c[2].empty() || !c[4].empty()) {
                std::string esp_sig = c[2].empty() ? "-" : c[2];
                std::string eng_sig = c[4].empty() ? "-" : c[4];
                linea += "   [ESP: " + esp_sig + " | ENG: " + eng_sig + "]";
            }
            linea += " (" + std::to_string(pct) + "%)";

            std::string fit_line = font_sys.fit_text(linea, font_size, mw - 60.0f);
            font_sys.draw_text(fit_line, mx + 30.0f, my + 18.0f + 2.0f * line_h + static_cast<float>(k) * line_h, font_size, Color{220, 220, 220, 255});
        }

        font_sys.draw_text("¿Desea guardar de todos modos?", mx + 20.0f, my + mh - 2.0f * (static_cast<float>(font_size) + 10.0f) - 16.0f, font_size, WHITE);
        font_sys.draw_text("[ENTER / S] Confirmar e ingresar  |  [ESC / N] Cancelar y corregir",
                           mx + 20.0f, my + mh - (static_cast<float>(font_size) + 10.0f) - 16.0f, font_size, Color{100, 220, 120, 255});
    }

    // --------------------------------------------------------------------------
    // CUADRO DE CONFIRMACIÓN PARA SALIR DE LA APLICACIÓN (ESC)
    // --------------------------------------------------------------------------
    if (quit_confirm_open) {
        DrawRectangle(0, 0, lay.win_w, lay.win_h, Color{0, 0, 0, 180});

        float mw = std::clamp(static_cast<float>(font_size) * 28.0f, 560.0f, static_cast<float>(lay.win_w) - 40.0f);
        float mh = std::clamp(static_cast<float>(font_size) * 7.5f, 155.0f, static_cast<float>(lay.win_h) - 40.0f);
        float mx = (lay.win_w - mw) / 2.0f;
        float my = (lay.win_h - mh) / 2.0f;

        DrawRectangle(static_cast<int>(mx), static_cast<int>(my), static_cast<int>(mw), static_cast<int>(mh), Color{25, 25, 25, 255});
        DrawRectangleLinesEx(Rectangle{mx, my, mw, mh}, 2.0f, Color{230, 140, 30, 255});

        float line_h = static_cast<float>(font_size) + 8.0f;
        font_sys.draw_text("¿Cerrar Termen?", mx + 20.0f, my + 18.0f, font_size, Color{240, 180, 50, 255});
        font_sys.draw_text("Presione [ESC] de nuevo para salir de la aplicación.", mx + 20.0f, my + 18.0f + line_h, font_size, WHITE);
        font_sys.draw_text("Presione cualquier otra tecla o haga clic en Cancelar para volver.", mx + 20.0f, my + 18.0f + 2.0f * line_h, font_size, LIGHTGRAY);

        float btn_w = std::max(110.0f, static_cast<float>(font_size) * 6.0f);
        float btn_h = std::max(28.0f, static_cast<float>(font_size) + 10.0f);
        Rectangle btn_quit_ok{mx + mw - 2.0f * btn_w - 24.0f, my + mh - btn_h - 16.0f, btn_w, btn_h};
        Rectangle btn_quit_cancel{mx + mw - btn_w - 16.0f, my + mh - btn_h - 16.0f, btn_w, btn_h};

        bool hov_q_ok = CheckCollisionPointRec(mouse_pos, btn_quit_ok);
        bool hov_q_canc = CheckCollisionPointRec(mouse_pos, btn_quit_cancel);

        DrawRectangleRec(btn_quit_ok, hov_q_ok ? Color{220, 70, 70, 255} : Color{160, 40, 40, 255});
        float t_q_ok_w = font_sys.measure_text("Salir (ESC)", font_size);
        font_sys.draw_text("Salir (ESC)", btn_quit_ok.x + (btn_quit_ok.width - t_q_ok_w) / 2.0f,
                           btn_quit_ok.y + (btn_quit_ok.height - static_cast<float>(font_size)) / 2.0f, font_size, WHITE);

        DrawRectangleRec(btn_quit_cancel, hov_q_canc ? Color{65, 65, 65, 255} : Color{45, 45, 45, 255});
        float t_q_canc_w = font_sys.measure_text("Cancelar", font_size);
        font_sys.draw_text("Cancelar", btn_quit_cancel.x + (btn_quit_cancel.width - t_q_canc_w) / 2.0f,
                           btn_quit_cancel.y + (btn_quit_cancel.height - static_cast<float>(font_size)) / 2.0f, font_size, WHITE);
    }

    EndDrawing();
}
