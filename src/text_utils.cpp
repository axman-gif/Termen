#include "text_utils.hpp"
#include <unordered_map>
#include <algorithm>
#include <cctype>

namespace TextUtils {

static std::unordered_map<std::string, std::string> s_norm_cache;
static std::unordered_map<std::string, std::string> s_strip_marks_cache;

char32_t decode_utf8(const char* ptr, int* out_len) {
    if (!ptr || !*ptr) {
        if (out_len) *out_len = 0;
        return 0;
    }
    const unsigned char c = static_cast<unsigned char>(*ptr);
    if (c < 0x80) {
        if (out_len) *out_len = 1;
        return c;
    } else if ((c & 0xE0) == 0xC0) {
        if (out_len) *out_len = 2;
        return ((c & 0x1F) << 6) | (static_cast<unsigned char>(ptr[1]) & 0x3F);
    } else if ((c & 0xF0) == 0xE0) {
        if (out_len) *out_len = 3;
        return ((c & 0x0F) << 12) |
               ((static_cast<unsigned char>(ptr[1]) & 0x3F) << 6) |
               (static_cast<unsigned char>(ptr[2]) & 0x3F);
    } else if ((c & 0xF8) == 0xF0) {
        if (out_len) *out_len = 4;
        return ((c & 0x07) << 18) |
               ((static_cast<unsigned char>(ptr[1]) & 0x3F) << 12) |
               ((static_cast<unsigned char>(ptr[2]) & 0x3F) << 6) |
               (static_cast<unsigned char>(ptr[3]) & 0x3F);
    }
    if (out_len) *out_len = 1;
    return c;
}

std::string encode_utf8(char32_t cp) {
    std::string res;
    if (cp < 0x80) {
        res.push_back(static_cast<char>(cp));
    } else if (cp < 0x800) {
        res.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
        res.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        res.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
        res.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        res.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        res.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
        res.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        res.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        res.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    return res;
}

bool is_letter(char32_t cp) {
    if ((cp >= U'a' && cp <= U'z') || (cp >= U'A' && cp <= U'Z')) return true;
    switch (cp) {
        case 0x00E1: case 0x00C1:
        case 0x00E0: case 0x00C0:
        case 0x00E2: case 0x00C2:
        case 0x00E4: case 0x00C4:
        case 0x00E9: case 0x00C9:
        case 0x00E8: case 0x00C8:
        case 0x00EA: case 0x00CA:
        case 0x00EB: case 0x00CB:
        case 0x00ED: case 0x00CD:
        case 0x00EC: case 0x00CC:
        case 0x00EE: case 0x00CE:
        case 0x00EF: case 0x00CF:
        case 0x00F3: case 0x00D3:
        case 0x00F2: case 0x00D2:
        case 0x00F4: case 0x00D4:
        case 0x00F6: case 0x00D6:
        case 0x00FA: case 0x00DA:
        case 0x00F9: case 0x00D9:
        case 0x00FB: case 0x00DB:
        case 0x00FC: case 0x00DC:
        case 0x00F1: case 0x00D1:
            return true;
        default:
            return false;
    }
}

char32_t to_lower_cp(char32_t cp) {
    if (cp >= U'A' && cp <= U'Z') return cp + (U'a' - U'A');
    switch (cp) {
        case 0x00C1: return 0x00E1; // Á -> á
        case 0x00C0: return 0x00E0; // À -> à
        case 0x00C2: return 0x00E2; // Â -> â
        case 0x00C4: return 0x00E4; // Ä -> ä
        case 0x00C9: return 0x00E9; // É -> é
        case 0x00C8: return 0x00E8; // È -> è
        case 0x00CA: return 0x00EA; // Ê -> ê
        case 0x00CB: return 0x00EB; // Ë -> ë
        case 0x00CD: return 0x00ED; // Í -> í
        case 0x00CC: return 0x00EC; // Ì -> ì
        case 0x00CE: return 0x00EE; // Î -> î
        case 0x00CF: return 0x00EF; // Ï -> ï
        case 0x00D3: return 0x00F3; // Ó -> ó
        case 0x00D2: return 0x00F2; // Ò -> ò
        case 0x00D4: return 0x00F4; // Ô -> ô
        case 0x00D6: return 0x00F6; // Ö -> ö
        case 0x00DA: return 0x00FA; // Ú -> ú
        case 0x00D9: return 0x00F9; // Ù -> ù
        case 0x00DB: return 0x00FB; // Û -> û
        case 0x00DC: return 0x00FC; // Ü -> ü
        case 0x00D1: return 0x00F1; // Ñ -> ñ
        default: return cp;
    }
}

std::string to_lower_utf8(const std::string& str) {
    std::string res;
    res.reserve(str.size());
    const char* p = str.c_str();
    while (*p) {
        int len = 0;
        char32_t cp = decode_utf8(p, &len);
        res += encode_utf8(to_lower_cp(cp));
        p += len;
    }
    return res;
}

size_t utf8_length(const std::string& str) {
    size_t count = 0;
    const char* p = str.c_str();
    while (*p) {
        int len = 0;
        decode_utf8(p, &len);
        if (len <= 0) break;
        p += len;
        ++count;
    }
    return count;
}

void pop_back_utf8(std::string& str) {
    if (str.empty()) return;
    while (!str.empty()) {
        unsigned char c = static_cast<unsigned char>(str.back());
        str.pop_back();
        if ((c & 0xC0) != 0x80) {
            break;
        }
    }
}

std::string norm(const std::string& str) {
    auto it = s_norm_cache.find(str);
    if (it != s_norm_cache.end()) {
        return it->second;
    }

    std::string lower = to_lower_utf8(str);
    std::string res;
    res.reserve(lower.size());

    const char* p = lower.c_str();
    while (*p) {
        int len = 0;
        char32_t cp = decode_utf8(p, &len);
        p += len;

        // Descomposición NFD y eliminación de marcas diacríticas (Mn)
        switch (cp) {
            case 0x00E1: case 0x00E0: case 0x00E2: case 0x00E4: res.push_back('a'); break;
            case 0x00E9: case 0x00E8: case 0x00EA: case 0x00EB: res.push_back('e'); break;
            case 0x00ED: case 0x00EC: case 0x00EE: case 0x00EF: res.push_back('i'); break;
            case 0x00F3: case 0x00F2: case 0x00F4: case 0x00F6: res.push_back('o'); break;
            case 0x00FA: case 0x00F9: case 0x00FB: case 0x00FC: res.push_back('u'); break;
            case 0x00F1: res.push_back('n'); break;
            case 0x0300: case 0x0301: case 0x0302: case 0x0303:
            case 0x0308: case 0x030A: case 0x0327:
                break;
            default:
                if (cp < 0x80) {
                    res.push_back(static_cast<char>(cp));
                } else {
                    res += encode_utf8(cp);
                }
                break;
        }
    }

    // Trim de espacios
    size_t first = res.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        res.clear();
    } else {
        size_t last = res.find_last_not_of(" \t\r\n");
        res = res.substr(first, last - first + 1);
    }

    if (s_norm_cache.size() >= 65536) {
        s_norm_cache.clear();
    }
    s_norm_cache[str] = res;
    return res;
}

std::string strip_marks(const std::string& str) {
    auto it = s_strip_marks_cache.find(str);
    if (it != s_strip_marks_cache.end()) {
        return it->second;
    }

    std::string res;
    res.reserve(str.size());
    const char* p = str.c_str();
    while (*p) {
        int len = 0;
        char32_t cp = decode_utf8(p, &len);
        p += len;

        switch (cp) {
            case 0x00E1: case 0x00E0: case 0x00E2: case 0x00E4: res.push_back('a'); break;
            case 0x00E9: case 0x00E8: case 0x00EA: case 0x00EB: res.push_back('e'); break;
            case 0x00ED: case 0x00EC: case 0x00EE: case 0x00EF: res.push_back('i'); break;
            case 0x00F3: case 0x00F2: case 0x00F4: case 0x00F6: res.push_back('o'); break;
            case 0x00FA: case 0x00F9: case 0x00FB: case 0x00FC: res.push_back('u'); break;
            case 0x00F1: res.push_back('n'); break;
            case 0x00C1: case 0x00C0: case 0x00C2: case 0x00C4: res.push_back('A'); break;
            case 0x00C9: case 0x00C8: case 0x00CA: case 0x00CB: res.push_back('E'); break;
            case 0x00CD: case 0x00CC: case 0x00CE: case 0x00CF: res.push_back('I'); break;
            case 0x00D3: case 0x00D2: case 0x00D4: case 0x00D6: res.push_back('O'); break;
            case 0x00DA: case 0x00D9: case 0x00DB: case 0x00DC: res.push_back('U'); break;
            case 0x00D1: res.push_back('N'); break;
            case 0x0300: case 0x0301: case 0x0302: case 0x0303:
            case 0x0308: case 0x030A: case 0x0327:
                break;
            default:
                if (cp < 0x80) {
                    res.push_back(static_cast<char>(cp));
                } else {
                    res += encode_utf8(cp);
                }
                break;
        }
    }

    size_t first = res.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        res.clear();
    } else {
        size_t last = res.find_last_not_of(" \t\r\n");
        res = res.substr(first, last - first + 1);
    }

    if (s_strip_marks_cache.size() >= 65536) {
        s_strip_marks_cache.clear();
    }
    s_strip_marks_cache[str] = res;
    return res;
}

std::string hnorm(const std::string& str) {
    std::string s = str;
    if (s.size() >= 3 && static_cast<unsigned char>(s[0]) == 0xEF &&
        static_cast<unsigned char>(s[1]) == 0xBB && static_cast<unsigned char>(s[2]) == 0xBF) {
        s = s.substr(3);
    }
    std::string n = norm(s);
    std::string res;
    res.reserve(n.size());
    for (char c : n) {
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n' && c != '.' && c != '_') {
            res.push_back(c);
        }
    }
    return res;
}

// Implementación de difflib.SequenceMatcher (Ratcliff-Obershelp)
struct MatchBlock {
    int a_start;
    int b_start;
    int length;
};

static MatchBlock find_longest_match(const std::string& a, int alo, int ahi,
                                     const std::string& b, int blo, int bhi) {
    MatchBlock best{alo, blo, 0};
    for (int i = alo; i < ahi; ++i) {
        for (int j = blo; j < bhi; ++j) {
            int k = 0;
            while (i + k < ahi && j + k < bhi && a[i + k] == b[j + k]) {
                ++k;
            }
            if (k > best.length) {
                best = {i, j, k};
            }
        }
    }
    return best;
}

static int get_matching_chars(const std::string& a, int alo, int ahi,
                              const std::string& b, int blo, int bhi) {
    if (alo >= ahi || blo >= bhi) return 0;
    MatchBlock match = find_longest_match(a, alo, ahi, b, blo, bhi);
    if (match.length == 0) return 0;
    int total = match.length;
    total += get_matching_chars(a, alo, match.a_start, b, blo, match.b_start);
    total += get_matching_chars(a, match.a_start + match.length, ahi,
                                b, match.b_start + match.length, bhi);
    return total;
}

double sequence_matcher_ratio(const std::string& a, const std::string& b) {
    if (a.empty() || b.empty()) return 0.0;
    if (a == b) return 1.0;
    int matches = get_matching_chars(a, 0, static_cast<int>(a.size()),
                                     b, 0, static_cast<int>(b.size()));
    return (2.0 * matches) / static_cast<double>(a.size() + b.size());
}

double sim(const std::string& a, const std::string& b) {
    if (a.empty() || b.empty()) return 0.0;
    double r = sequence_matcher_ratio(a, b);
    if (r >= 0.68 || (a.size() >= 3 && b.find(a) != std::string::npos)) {
        return r;
    }
    return 0.0;
}

std::string fit(const std::string& s, Font font, int size, float max_w) {
    if (MeasureTextEx(font, s.c_str(), static_cast<float>(size), 1.0f).x <= max_w) {
        return s;
    }
    std::string cur = s;
    while (!cur.empty()) {
        std::string cand = cur + "...";
        if (MeasureTextEx(font, cand.c_str(), static_cast<float>(size), 1.0f).x <= max_w) {
            return cand;
        }
        pop_back_utf8(cur);
    }
    return "...";
}

std::string tail(const std::string& s, Font font, int size, float max_w) {
    std::string cur = s;
    while (!cur.empty() && MeasureTextEx(font, cur.c_str(), static_cast<float>(size), 1.0f).x > max_w) {
        int len = 0;
        decode_utf8(cur.c_str(), &len);
        if (len <= 0) break;
        cur.erase(0, len);
    }
    return cur;
}

} // namespace TextUtils
