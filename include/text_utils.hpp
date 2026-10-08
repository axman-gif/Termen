#pragma once

#include <string>
#include <vector>
#include <optional>
#include "raylib.h"

namespace TextUtils {

// Decodificación y manipulación UTF-8
char32_t decode_utf8(const char* ptr, int* out_len);
std::string encode_utf8(char32_t cp);
bool is_letter(char32_t cp);
char32_t to_lower_cp(char32_t cp);
std::string to_lower_utf8(const std::string& str);
size_t utf8_length(const std::string& str);
void pop_back_utf8(std::string& str);

// Normalización lingüística (idéntica a la versión Python y Noten)
std::string norm(const std::string& str);
std::string strip_marks(const std::string& str);
std::string hnorm(const std::string& str);

// Algoritmo de similitud difflib.SequenceMatcher (Ratcliff-Obershelp)
double sequence_matcher_ratio(const std::string& a, const std::string& b);
double sim(const std::string& a, const std::string& b);

// Ajuste visual de texto con fuentes Raylib
std::string fit(const std::string& s, Font font, int size, float max_w);
std::string tail(const std::string& s, Font font, int size, float max_w);

} // namespace TextUtils
