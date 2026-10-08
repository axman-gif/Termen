#pragma once

#include <string>
#include "raylib.h"

struct AppConfig {
    int size = 64;                      // Tamaño de fuente del editor principal de Noten
    int glossary_size = 15;             // Tamaño de fuente unificado para Termen
    int ahead = 1;                      // Parámetro ahead de Noten
    Color bgt = {0, 0, 0, 255};         // Fondo degradado superior
    Color bgb = {220, 40, 40, 255};     // Fondo degradado inferior
    Color txc = {245, 200, 30, 255};    // Color de texto en Noten
    int acr_key = 290;                  // Tecla de acrónimos en Noten (F1)
    std::string font = "arial.ttf";     // Nombre de archivo de fuente compartida
};

class ConfigManager {
public:
    static std::string get_config_path();
    static bool load(AppConfig& cfg);
    static bool save(const AppConfig& cfg);
};
