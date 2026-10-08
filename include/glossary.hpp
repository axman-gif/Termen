#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <utility>

struct SimilarTerm {
    std::vector<std::string> values; // 6 campos
    double score;
};

class Glossary {
public:
    static inline const std::vector<std::string> FIELDS = {
        "es", "en", "acresp", "significadoesp", "acreng", "meaningeng"
    };
    static inline const std::vector<std::string> HEADER_NAMES = {
        "es", "en", "acr. esp", "significado esp", "acr. eng", "meaning eng"
    };
    static inline const std::vector<std::string> LABELS = {
        "Término en Español (ES):", "Término en Inglés (EN):",
        "Sigla ES (acr. esp):", "Significado ES:",
        "Sigla EN (acr. eng):", "Meaning EN:"
    };
    static inline const std::vector<std::string> LABELS_SEARCH = {
        "Filtrar Español (ES):", "Filtrar Inglés (EN):",
        "Filtrar Sigla ES:", "Filtrar Significado ES:",
        "Filtrar Sigla EN:", "Filtrar Meaning EN:"
    };
    static inline const std::vector<std::string> COL_TITLES = {
        "ESPAÑOL", "INGLÉS", "ACR. ESP", "SIGNIFICADO ESP", "ACR. ENG", "MEANING ENG", "ACCIONES"
    };
    static constexpr int NF = 6;

    std::vector<std::string> header;
    std::vector<std::vector<std::string>> rows;
    std::unordered_map<std::string, int> idx;

    std::string enc = "utf-8";
    bool bom = true;
    char sep = ',';
    std::string eol = "\r\n";
    bool exists = false;
    bool readonly = false;
    bool backup_done = false;
    std::string msg;
    bool needs_update = false;

    Glossary();

    void load(const std::string& filepath);
    std::pair<bool, std::string> save(const std::string& filepath, const std::string& backup_dir);

    std::string cell(const std::vector<std::string>& row, const std::string& campo) const;
    std::vector<std::string> row_values(const std::vector<std::string>& row) const;
    void set_row(std::vector<std::string>& row, const std::vector<std::string>& vals);

    static std::optional<std::string> validate(const std::vector<std::string>& vals);

    std::vector<SimilarTerm> find_similar_terms(const std::vector<std::string>& vals,
                                                std::optional<size_t> ignore_index = std::nullopt) const;

    std::vector<std::pair<size_t, std::vector<std::string>>> get_processed_rows(
        const std::vector<std::string>& filter_fields,
        bool search_mode,
        std::optional<int> sort_col,
        bool sort_asc) const;
};
