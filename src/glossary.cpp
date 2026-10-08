#include "glossary.hpp"
#include "text_utils.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <algorithm>

Glossary::Glossary() {
    header = HEADER_NAMES;
    for (size_t i = 0; i < FIELDS.size(); ++i) {
        idx[FIELDS[i]] = static_cast<int>(i);
    }
}

static std::vector<std::vector<std::string>> parse_csv(const std::string& content, char sep) {
    std::vector<std::vector<std::string>> rows;
    std::vector<std::string> current_row;
    std::string current_field;
    bool in_quotes = false;

    for (size_t i = 0; i < content.size(); ++i) {
        char c = content[i];
        if (in_quotes) {
            if (c == '"') {
                if (i + 1 < content.size() && content[i + 1] == '"') {
                    current_field.push_back('"');
                    ++i;
                } else {
                    in_quotes = false;
                }
            } else {
                current_field.push_back(c);
            }
        } else {
            if (c == '"') {
                in_quotes = true;
            } else if (c == sep) {
                current_row.push_back(current_field);
                current_field.clear();
            } else if (c == '\r') {
                if (i + 1 < content.size() && content[i + 1] == '\n') {
                    ++i;
                }
                current_row.push_back(current_field);
                current_field.clear();
                rows.push_back(current_row);
                current_row.clear();
            } else if (c == '\n') {
                current_row.push_back(current_field);
                current_field.clear();
                rows.push_back(current_row);
                current_row.clear();
            } else {
                current_field.push_back(c);
            }
        }
    }

    if (!current_field.empty() || !current_row.empty()) {
        current_row.push_back(current_field);
        rows.push_back(current_row);
    }

    return rows;
}

static std::string escape_csv_field(const std::string& field, char sep) {
    bool needs_quotes = false;
    if (field.find(sep) != std::string::npos ||
        field.find('"') != std::string::npos ||
        field.find('\n') != std::string::npos ||
        field.find('\r') != std::string::npos) {
        needs_quotes = true;
    }

    if (!needs_quotes) {
        return field;
    }

    std::string res = "\"";
    for (char c : field) {
        if (c == '"') {
            res += "\"\"";
        } else {
            res.push_back(c);
        }
    }
    res += "\"";
    return res;
}

void Glossary::load(const std::string& filepath) {
    header = HEADER_NAMES;
    rows.clear();
    for (size_t i = 0; i < FIELDS.size(); ++i) {
        idx[FIELDS[i]] = static_cast<int>(i);
    }
    enc = "utf-8";
    bom = true;
    sep = ',';
    eol = "\r\n";
    exists = false;
    readonly = false;
    backup_done = false;
    msg.clear();
    needs_update = false;

    if (!std::filesystem::exists(filepath)) {
        return;
    }
    exists = true;

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        readonly = true;
        msg = "No se pudo leer glosario.csv; modo solo lectura.";
        return;
    }

    std::string raw((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    bom = (raw.size() >= 3 &&
           static_cast<unsigned char>(raw[0]) == 0xEF &&
           static_cast<unsigned char>(raw[1]) == 0xBB &&
           static_cast<unsigned char>(raw[2]) == 0xBF);

    std::string txt = bom ? raw.substr(3) : raw;

    if (txt.find("\r\n") != std::string::npos) {
        eol = "\r\n";
    } else {
        eol = "\n";
    }

    // Detectar separador observando la primera línea
    std::string first_line;
    size_t first_newline = txt.find('\n');
    if (first_newline != std::string::npos) {
        first_line = txt.substr(0, first_newline);
    } else {
        first_line = txt;
    }

    size_t c_comma = std::count(first_line.begin(), first_line.end(), ',');
    size_t c_semi = std::count(first_line.begin(), first_line.end(), ';');
    size_t c_tab = std::count(first_line.begin(), first_line.end(), '\t');

    if (c_semi > c_comma && c_semi >= c_tab) {
        sep = ';';
    } else if (c_tab > c_comma && c_tab > c_semi) {
        sep = '\t';
    } else {
        sep = ',';
    }

    auto parsed_rows = parse_csv(txt, sep);
    if (parsed_rows.empty()) {
        return;
    }

    std::vector<std::string> raw_header = parsed_rows[0];
    std::unordered_map<std::string, int> found;
    for (size_t i = 0; i < raw_header.size(); ++i) {
        std::string k = TextUtils::hnorm(raw_header[i]);
        if (!k.empty() && found.find(k) == found.end()) {
            found[k] = static_cast<int>(i);
        }
    }

    if (found.find("es") == found.end() && found.find("en") == found.end()) {
        readonly = true;
        msg = "La cabecera de glosario.csv no tiene columnas es/en; modo solo lectura.";
        header = raw_header;
        return;
    }

    header = raw_header;
    for (size_t f = 0; f < FIELDS.size(); ++f) {
        const std::string& field_name = FIELDS[f];
        if (found.find(field_name) == found.end()) {
            found[field_name] = static_cast<int>(header.size());
            header.push_back(HEADER_NAMES[f]);
            needs_update = true;
        }
    }

    idx.clear();
    for (const auto& f : FIELDS) {
        idx[f] = found[f];
    }

    for (size_t r = 1; r < parsed_rows.size(); ++r) {
        auto& row = parsed_rows[r];
        bool has_content = false;
        for (const auto& c : row) {
            std::string trimmed = c;
            size_t s1 = trimmed.find_first_not_of(" \t\r\n");
            if (s1 != std::string::npos) {
                has_content = true;
                break;
            }
        }
        if (!has_content) continue;

        while (row.size() < header.size()) {
            row.push_back("");
        }
        rows.push_back(row);
    }
}

std::pair<bool, std::string> Glossary::save(const std::string& filepath, const std::string& backup_dir) {
    if (readonly) {
        return {false, "el glosario está en modo solo lectura"};
    }

    std::string tmp_path = filepath + ".tmp";

    try {
        if (exists && !backup_done) {
            std::filesystem::create_directories(backup_dir);

            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S");
            std::string stamp = ss.str();

            std::string dest = backup_dir + "/glosario_" + stamp + ".csv";
            int n = 1;
            while (std::filesystem::exists(dest)) {
                dest = backup_dir + "/glosario_" + stamp + "_" + std::to_string(n) + ".csv";
                ++n;
            }

            std::filesystem::copy_file(filepath, dest, std::filesystem::copy_options::overwrite_existing);
            backup_done = true;
        }

        std::ofstream out(tmp_path, std::ios::binary);
        if (!out.is_open()) {
            return {false, "No se pudo crear archivo temporal para escritura."};
        }

        if (bom) {
            const unsigned char bom_bytes[] = {0xEF, 0xBB, 0xBF};
            out.write(reinterpret_cast<const char*>(bom_bytes), 3);
        }

        // Escribir cabecera
        for (size_t i = 0; i < header.size(); ++i) {
            out << escape_csv_field(header[i], sep);
            if (i + 1 < header.size()) out << sep;
        }
        out << eol;

        // Escribir filas
        for (const auto& row : rows) {
            for (size_t i = 0; i < header.size(); ++i) {
                std::string val = (i < row.size()) ? row[i] : "";
                out << escape_csv_field(val, sep);
                if (i + 1 < header.size()) out << sep;
            }
            out << eol;
        }
        out.close();

        std::error_code ec;
        std::filesystem::rename(tmp_path, filepath, ec);
        if (ec) {
            // En Windows a veces rename sobreescribe fallando si existe, usar replace o remove+rename
            std::filesystem::remove(filepath, ec);
            std::filesystem::rename(tmp_path, filepath, ec);
            if (ec) {
                return {false, ec.message()};
            }
        }

        exists = true;
        return {true, ""};
    } catch (const std::exception& e) {
        std::error_code ec;
        std::filesystem::remove(tmp_path, ec);
        return {false, e.what()};
    }
}

std::string Glossary::cell(const std::vector<std::string>& row, const std::string& campo) const {
    auto it = idx.find(campo);
    if (it == idx.end()) return "";
    int i = it->second;
    if (i < static_cast<int>(row.size())) {
        std::string s = row[i];
        size_t f = s.find_first_not_of(" \t\r\n");
        if (f == std::string::npos) return "";
        size_t l = s.find_last_not_of(" \t\r\n");
        return s.substr(f, l - f + 1);
    }
    return "";
}

std::vector<std::string> Glossary::row_values(const std::vector<std::string>& row) const {
    std::vector<std::string> res;
    res.reserve(FIELDS.size());
    for (const auto& f : FIELDS) {
        res.push_back(cell(row, f));
    }
    return res;
}

void Glossary::set_row(std::vector<std::string>& row, const std::vector<std::string>& vals) {
    for (size_t f = 0; f < FIELDS.size() && f < vals.size(); ++f) {
        int i = idx[FIELDS[f]];
        while (static_cast<int>(row.size()) <= i) {
            row.push_back("");
        }
        row[i] = vals[f];
    }
}

std::optional<std::string> Glossary::validate(const std::vector<std::string>& vals) {
    if (vals.size() < 6) return "Error interno en campos.";

    const std::string& es = vals[0];
    const std::string& en = vals[1];
    const std::string& ae = vals[2];
    const std::string& se = vals[3];
    const std::string& ai = vals[4];
    const std::string& mi = vals[5];

    bool any_val = false;
    for (const auto& v : vals) {
        if (!v.empty()) { any_val = true; break; }
    }
    if (!any_val) {
        return "No hay nada que guardar.";
    }

    if ((!es.empty()) != (!en.empty())) {
        return "Completa ES y EN (o deja ambos vacíos).";
    }

    if (!es.empty() && (TextUtils::utf8_length(es) < 3 || TextUtils::utf8_length(en) < 3)) {
        return "ES y EN deben tener al menos 3 caracteres.";
    }

    if ((!ae.empty()) != (!se.empty())) {
        return "La sigla ES y su significado van juntos.";
    }

    if ((!ai.empty()) != (!mi.empty())) {
        return "La sigla EN y su meaning van juntos.";
    }

    return std::nullopt;
}

std::vector<SimilarTerm> Glossary::find_similar_terms(const std::vector<std::string>& vals,
                                                     std::optional<size_t> ignore_index) const {
    if (vals.size() < 6) return {};

    const std::string& es = vals[0];
    const std::string& en = vals[1];
    const std::string& ae = vals[2];
    const std::string& ai = vals[4];

    std::string nes = TextUtils::norm(es);
    std::string nen = TextUtils::norm(en);

    std::vector<SimilarTerm> res;

    for (size_t idx_row = 0; idx_row < rows.size(); ++idx_row) {
        if (ignore_index.has_value() && idx_row == ignore_index.value()) {
            continue;
        }

        auto c = row_values(rows[idx_row]);
        double score = std::max(TextUtils::sim(nes, TextUtils::norm(c[0])),
                                TextUtils::sim(nen, TextUtils::norm(c[1])));

        if (!ae.empty() && !c[2].empty() && TextUtils::strip_marks(ae) == TextUtils::strip_marks(c[2])) {
            score = std::max(score, 1.0);
        }
        if (!ai.empty() && !c[4].empty() && TextUtils::strip_marks(ai) == TextUtils::strip_marks(c[4])) {
            score = std::max(score, 1.0);
        }

        if (score > 0.0) {
            res.push_back({c, score});
        }
    }

    std::sort(res.begin(), res.end(), [](const SimilarTerm& a, const SimilarTerm& b) {
        return a.score > b.score;
    });

    return res;
}

std::vector<std::pair<size_t, std::vector<std::string>>> Glossary::get_processed_rows(
    const std::vector<std::string>& filter_fields,
    bool search_mode,
    std::optional<int> sort_col,
    bool sort_asc) const {

    std::vector<std::pair<size_t, std::vector<std::string>>> res;

    // 1. Filtrado
    if (!search_mode) {
        res.reserve(rows.size());
        for (size_t i = 0; i < rows.size(); ++i) {
            res.push_back({i, rows[i]});
        }
    } else {
        std::vector<std::pair<int, std::string>> active_filters;
        for (int k = 0; k < NF && k < static_cast<int>(filter_fields.size()); ++k) {
            std::string q = TextUtils::norm(filter_fields[k]);
            if (!q.empty()) {
                active_filters.push_back({k, q});
            }
        }

        if (active_filters.empty()) {
            res.reserve(rows.size());
            for (size_t i = 0; i < rows.size(); ++i) {
                res.push_back({i, rows[i]});
            }
        } else {
            for (size_t orig_idx = 0; orig_idx < rows.size(); ++orig_idx) {
                bool match = true;
                for (const auto& filt : active_filters) {
                    std::string val = TextUtils::norm(cell(rows[orig_idx], FIELDS[filt.first]));
                    if (val.find(filt.second) == std::string::npos) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    res.push_back({orig_idx, rows[orig_idx]});
                }
            }
        }
    }

    // 2. Ordenamiento alfabético
    if (sort_col.has_value() && sort_col.value() >= 0 && sort_col.value() < NF) {
        const std::string& campo = FIELDS[sort_col.value()];

        struct SortItem {
            std::string norm_val;
            size_t orig_idx;
            std::vector<std::string> row;
        };

        std::vector<SortItem> con_datos;
        std::vector<std::pair<size_t, std::vector<std::string>>> sin_datos;

        for (const auto& item : res) {
            std::string val = TextUtils::norm(cell(item.second, campo));
            if (!val.empty()) {
                con_datos.push_back({val, item.first, item.second});
            } else {
                sin_datos.push_back(item);
            }
        }

        if (sort_asc) {
            std::sort(con_datos.begin(), con_datos.end(), [](const SortItem& a, const SortItem& b) {
                return a.norm_val < b.norm_val;
            });
        } else {
            std::sort(con_datos.begin(), con_datos.end(), [](const SortItem& a, const SortItem& b) {
                return a.norm_val > b.norm_val;
            });
        }

        res.clear();
        res.reserve(con_datos.size() + sin_datos.size());
        for (const auto& item : con_datos) {
            res.push_back({item.orig_idx, item.row});
        }
        for (const auto& item : sin_datos) {
            res.push_back(item);
        }
    }

    return res;
}
