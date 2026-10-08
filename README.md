# 📋 Termen — Gestor y Editor Avanzado de Glosario Médico Bilingüe ✨

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg?style=for-the-badge&logo=c%2B%2B)](https://en.cppreference.com/w/cpp/23)
[![Raylib](https://img.shields.io/badge/GUI-Raylib_5.5-red.svg?style=for-the-badge&logo=raylib)](https://www.raylib.com/)
[![CMake](https://img.shields.io/badge/Build-CMake-orange.svg?style=for-the-badge&logo=cmake)](https://cmake.org/)
[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg?style=for-the-badge)](LICENSE)

**Termen** es una aplicación nativa de alto rendimiento en **C++23** con **Raylib** diseñada para la gestión, curación y edición especializada del Glosario Médico bilingüe (Español 🇪🇸 / Inglés 🇬🇧). Actúa como la herramienta complementaria y compañera oficial del proyecto [Noten](https://github.com/wikiaxman/Noten), ofreciendo detección avanzada de duplicados por similitud fonética/textual, escaneo automático de fuentes del sistema operativo y edición fluida en tiempo real con cero dependencias externas complejas.

---

## 🌟 Tabla de Contenidos

- [🎯 Para Usuarios](#-para-usuarios)
  - [✨ Características Principales](#-características-principales)
  - [⌨️ Atajos de Teclado y Navegación](#️-atajos-de-teclado-y-navegación)
  - [📊 Estructura del Glosario Médico y Manejo de CSV](#-estructura-del-glosario-médico-y-manejo-de-csv)
- [💻 Para Desarrolladores](#-para-desarrolladores)
  - [🏗️ Arquitectura Modular](#️-arquitectura-modular)
  - [🔍 Algoritmo de Similitud y Prevención de Duplicados](#-algoritmo-de-similitud-y-prevención-de-duplicados)
  - [🔤 Sistema de Detección Dinámica de Fuentes](#-sistema-de-detección-dinámica-de-fuentes)
  - [⚙️ Requisitos del Sistema](#️-requisitos-del-sistema)
  - [🚀 Compilación e Instalación](#-compilación-e-instalación)
  - [📄 Configuración Compartida (`config.json`) y Respaldos Automáticos](#-configuración-compartida-configjson-y-respaldos-automáticos)
- [🤝 Contribuciones](#-contribuciones)
- [📜 Licencia](#-licencia)

---

## 🎯 Para Usuarios

### ✨ Características Principales

- **📋 Estructura Médica Completa de 6 Columnas**:
  - `es`: Término clínico en Español (ES).
  - `en`: Término clínico en Inglés (EN).
  - `acr. esp`: Sigla o acrónimo en Español (ej. `ERC`).
  - `significado esp`: Expansión o significado clínico en Español.
  - `acr. eng`: Sigla o acrónimo en Inglés (ej. `CKD`).
  - `meaning eng`: Expansión o significado clínico en Inglés.
  - Columna dinámica de acciones: botones contextuales **Editar** y **Borrar** por fila.
- **🔍 Modo Búsqueda y Filtrado Instantáneo**:
  - Activación rápida con `F3` o `Ctrl + F`.
  - Filtrado en tiempo real en los 6 campos simultáneamente con normalización de caracteres (ignora mayúsculas y acentos diacríticos).
- **📶 Ordenamiento Alfabético Multicriterio**:
  - Clic en los encabezados de columna:
    - 1ᵉʳ clic: Ascendente `[A-Z]`.
    - 2ᵈᵒ clic: Descendente `[Z-A]`.
    - 3ᵉʳ clic: Restablecer al orden original del archivo CSV.
  - Botón visible `Orden Original` para restaurar la vista original con un solo clic.
- **🛡️ Prevención Inteligente de Duplicados por Similitud**:
  - Detección difusa mediante el algoritmo Ratcliff-Obershelp (`difflib.SequenceMatcher`).
  - Ventana modal interactiva de confirmación que calcula y expone el porcentaje de coincidencia antes de insertar registros redundantes.
- **🔤 Detección Automática de Fuentes Tipográficas**:
  - Escaneo al vuelo de las fuentes instaladas en Windows (`C:/Windows/Fonts`, `%LOCALAPPDATA%/Microsoft/Windows/Fonts`) y en la carpeta local `fonts/`.
  - Selector tipográfico visual interactivo desplegable en la barra superior.
- **🛡️ Manejo Seguro de Archivos CSV y Respaldos**:
  - Detección automática del delimitador (`,`, `;`, `\t`) y codificación (UTF-8 con y sin BOM).
  - Generación de copias de seguridad fechadas automáticamente en `respaldos/glosario_YYYYMMDD_HHMMSS.csv` antes de guardar cualquier cambio.
  - Compatible con discos RAM (`R:/`), optimizado para el flujo de trabajo conjunto con [Noten](https://github.com/wikiaxman/Noten).
- **🎚️ Ajuste Dinámico de Tamaño de Letra**:
  - Tamaño de letra unificado para toda la interfaz (10–30 px), ajustable con la rueda del ratón o atajos de teclado.

---

### ⌨️ Atajos de Teclado y Navegación

| Atajo | Acción | Descripción |
| :--- | :--- | :--- |
| `F3` o `Ctrl + F` | **Alternar Modo Búsqueda** | Alterna entre el modo de inserción/edición y el filtro de búsqueda. |
| `Tab` / `Shift + Tab` | **Navegar campos** | Salta al siguiente o anterior cuadro de entrada de texto. |
| `Flecha Arriba` / `Abajo` | **Cambiar campo activo** | Cambia verticalmente entre los campos de edición. |
| `Enter` | **Guardar / Siguiente** | En modo registro guarda el término; en modo búsqueda salta al siguiente campo. |
| `Ctrl + +` / `Ctrl + -` | **Zoom de Fuente** | Incrementa o disminuye el tamaño de fuente unificado de la app. |
| `Ctrl + Rueda Ratón` | **Zoom Dinámico** | Modifica el tamaño de letra en tiempo real con el ratón. |
| `Ctrl + 0` | **Restablecer Fuente** | Devuelve el tamaño de letra al valor predeterminado (15 px). |
| `Ctrl + X` | **Limpiar Campo** | Borra todo el contenido del cuadro de texto activo. |
| `Ctrl + V` | **Pegar Texto** | Pega el contenido del portapapeles en el campo actual. |
| `Escape` | **Limpiar / Cancelar / Salir** | Limpia el filtro, cancela la edición en curso o despliega confirmación de salida. |
| `Escape` *(en diálogo de salida)* | **Confirmar Salida** | Cierra la aplicación con seguridad. |
| `Cualquier otra tecla` *(en diálogo)* | **Permanecer** | Descarta la confirmación de salida y vuelve al editor. |
| `Enter` o `S` *(en modal similitud)* | **Aceptar Duplicado** | Confirma la inserción del término a pesar de la advertencia de similitud. |
| `Escape` o `N` *(en modal similitud)* | **Revisar Término** | Cancela la inserción para permitir corregir el término introducido. |

> 💡 **Tip:** Puedes hacer clic directamente en cualquier encabezado de columna para ordenar alfabéticamente la tabla completa, o arrastrar la barra de scroll lateral para desplazarte rápidamente por miles de términos médicos.

---

### 📊 Estructura del Glosario Médico y Manejo de CSV

El archivo maestro `glosario.csv` se ubica en el directorio de la aplicación (o en el disco RAM `R:/` si está activo) con codificación **UTF-8 con BOM**:

```csv
es,en,acr. esp,significado esp,acr. eng,meaning eng
fiebre,fever,,,
enfermedad renal crónica,chronic kidney disease,ERC,enfermedad renal crónica,CKD,chronic kidney disease
infarto agudo de miocardio,acute myocardial infarction,IAM,infarto agudo de miocardio,AMI,acute myocardial infarction
```

- **Preservación estructural:** Si el archivo contiene columnas adicionales personalizadas, Termen las preserva intactas durante las operaciones de guardado.
- **Respaldos históricos:** Toda operación de guardado genera previamente un archivo de seguridad fechado dentro de `respaldos/`.

---

## 💻 Para Desarrolladores

### 🏗️ Arquitectura Modular

El proyecto organiza el código separando cabeceras (`include/`) e implementaciones (`src/`), garantizando modularidad, mantenibilidad y desacoplamiento limpio:

```
Termen/
├── CMakeLists.txt                          # 🛠️ Configuración de compilación CMake con FetchContent
├── README.md                               # 📖 Documentación general y técnica del proyecto
├── LICENSE                                 # ⚖️ Licencia de código abierto Apache 2.0
├── .gitignore                              # 🚫 Reglas de exclusión para Git
├── config.json                             # ⚙️ Archivo de configuración visual compartido con Noten
├── fonts/                                  # 🔤 Carpeta local para colocar tipografías (.ttf / .otf)
├── .vscode/
│   └── tasks.json                          # ⚡ Tareas de compilación para Visual Studio Code
│
├── include/                                # 📂 Archivos de cabecera (.hpp)
│   ├── app.hpp                             # Coordinación del estado global, ciclo de interfaz y layout
│   ├── config_manager.hpp                  # Gestión de persistencia y compatibilidad de config.json
│   ├── font_system.hpp                     # Escaneo de fuentes del SO, caché y renderizado tipográfico
│   ├── glossary.hpp                        # Gestión de CSV, ordenamiento, filtrado y respaldos fechados
│   └── text_utils.hpp                      # Normalización UTF-8 y algoritmo de coincidencia difusa
│
└── src/                                    # 📂 Código fuente (.cpp)
    ├── main.cpp                            # Punto de entrada de la aplicación
    ├── app.cpp                             # Implementación del ciclo principal, interfaz y modales
    ├── config_manager.cpp                  # Lectura/escritura atómica de configuraciones JSON
    ├── font_system.cpp                     # Detección de fuentes del sistema y atlas con filtrado bilineal
    ├── glossary.cpp                        # Motor de datos CSV, respaldos y ordenamiento
    └── text_utils.cpp                      # Lógica de similitud SequenceMatcher y codepoints UTF-8
```

---

### 🔍 Algoritmo de Similitud y Prevención de Duplicados

Para prevenir duplicados accidentales o términos redundantes, Termen incorpora en C++ una implementación fiel del algoritmo **Ratcliff-Obershelp** (`difflib.SequenceMatcher`):

$$\text{Similitud} = \frac{2 \cdot M}{T}$$

Donde:
- $M$ es la suma de caracteres coincidentes en los bloques comunes contiguos más largos entre ambas cadenas.
- $T$ es el total combinado de caracteres de las dos cadenas evaluadas.

```cpp
// Normalización antes de comparación:
// Convierte a minúsculas y elimina tildes/marcas diacríticas (á -> a, é -> e, etc.)
// mientras preserva 'ñ' para precisión médica en español.
std::string key1 = TextUtils::normalize_key(term1);
std::string key2 = TextUtils::normalize_key(term2);
double ratio = TextUtils::similarity_ratio(key1, key2);
```

Si el ratio supera el umbral de similitud ($\ge 75\%$), se despliega automáticamente la ventana modal informativa indicando el término preexistente y el grado de coincidencia detectado.

---

### 🔤 Sistema de Detección Dinámica de Fuentes

Heredado y sincronizado con el estándar tipográfico de [Noten](https://github.com/wikiaxman/Noten):

1. **Rutas escaneadas automáticamente**:
   - `C:/Windows/Fonts` (Fuentes instaladas del sistema en Windows).
   - `%LOCALAPPDATA%/Microsoft/Windows/Fonts` (Fuentes de usuario en Windows).
   - Carpeta local `fonts/` dentro del directorio de Termen.
   - Rutas equivalentes en distribuciones Linux y macOS.
2. **Soporte de caracteres enriquecido**:
   - Rango ASCII completo (32–126).
   - Suplemento Latin-1: vocales con tildes (`á, é, í, ó, ú`), diéresis (`ü`), `ñ, Ñ`, signos de apertura `¿, ¡`, comillas tipográficas y guiones.
3. **Calidad de renderizado**:
   - Texturas con filtrado bilineal activado (`TEXTURE_FILTER_BILINEAR`) para máxima nitidez en cualquier resolución de pantalla y tamaño de escala.

---

### ⚙️ Requisitos del Sistema

- **Compilador C++**: Compatible con **C++23** (GCC 13+, Clang 16+, o MSVC v143+).
- **Sistema de Compilación**: **CMake 3.20** o superior y generador compatible (Ninja o Make).
- **Biblioteca Gráfica**: [Raylib 5.5](https://github.com/raysan5/raylib) (gestionada automáticamente mediante `FetchContent`).
- **Plataformas Soportadas**: Windows 10/11, Linux (X11/Wayland) y macOS.

---

### 🚀 Compilación e Instalación

#### Opción A: Compilación estándar con CMake y Ninja (Recomendada)

1. **Clonar el repositorio**:
   ```bash
   git clone https://github.com/wikiaxman/Termen.git
   cd Termen
   ```

2. **Configurar el proyecto**:
   ```bash
   cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   ```

3. **Compilar el ejecutable**:
   ```bash
   cmake --build build --config Release
   ```

4. **Ejecutar Termen**:
   - En **Windows**:
     ```powershell
     .\build\Termen.exe
     ```
   - En **Linux / macOS**:
     ```bash
     ./build/Termen
     ```

#### Opción B: Compilación rápida con `g++` (Usando Raylib de Noten)

Si ya tienes compilado el proyecto hermano `Noten` en un directorio contiguo:

```powershell
g++ -std=c++23 -O2 src/main.cpp src/app.cpp src/font_system.cpp src/glossary.cpp src/text_utils.cpp src/config_manager.cpp -Iinclude -I../Noten/build/_deps/raylib-src/src -L../Noten/build/_deps/raylib-build/raylib -lraylib -lopengl32 -lgdi32 -lwinmm -lshell32 -mwindows -o Termen.exe
```

---

### 📄 Configuración Compartida (`config.json`) y Respaldos Automáticos

El archivo `config.json` se comparte directamente con [Noten](https://github.com/wikiaxman/Noten) para mantener una experiencia visual homogénea y unificada entre ambas herramientas:

```json
{
  "size": 64,
  "glossary_size": 15,
  "ahead": 1,
  "bgt": [0, 0, 0],
  "bgb": [220, 40, 40],
  "txc": [245, 200, 30],
  "acr_key": 290,
  "font": "arial.ttf"
}
```

- **`glossary_size`**: Define y almacena el tamaño de fuente unificado para toda la interfaz de Termen (10–30 px).
- **`font`**: Tipografía activa sincronizada entre Noten y Termen.
- **`size`**, **`ahead`**, **`bgt`**, **`bgb`**, **`txc`**, **`acr_key`**: Configuraciones de Noten preservadas de manera no destructiva.

---

## 🤝 Contribuciones

¡Las contribuciones son bienvenidas! Si deseas proponer mejoras, reportar problemas o añadir nuevas herramientas:
1. Haz un Fork del repositorio.
2. Crea una rama para tu característica (`git checkout -b feature/nueva-funcionalidad`).
3. Confirma tus cambios (`git commit -m "feat: agregar soporte para nueva funcionalidad"`).
4. Sube la rama (`git push origin feature/nueva-funcionalidad`).
5. Abre un **Pull Request**.

---

## 📜 Licencia

Este proyecto está distribuido bajo la Licencia **Apache 2.0**. Para más información sobre los términos de uso, reproducción, distribución y concesión de derechos de autor y patentes, consulta el archivo [LICENSE](LICENSE).

---

*Desarrollado con dedicación para optimizar la labor crítica de los intérpretes médicos.* 🩺💙
