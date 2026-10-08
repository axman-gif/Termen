# Termen (C++ / Raylib)

**Termen** es la aplicación nativa en **C++23** con **Raylib** para la gestión y edición avanzada del Glosario Médico, diseñada como herramienta complementaria y compañera oficial del proyecto [Noten](https://github.com/wikiaxman/Noten).

---

## 🌟 Características Principales

1. **Estructura Médica de 6 Columnas**:
   - `es`: Término en Español (ES)
   - `en`: Término en Inglés (EN)
   - `acr. esp`: Sigla o acrónimo en Español
   - `significado esp`: Significado o expansión en Español
   - `acr. eng`: Sigla o acrónimo en Inglés
   - `meaning eng`: Significado o expansión en Inglés
   - Columna dinámica de acciones: **Editar** y **Borrar** por fila.

2. **Sistema de Detección de Fuentes (Heredado de Noten)**:
   - **Escaneo automático**: Detecta fuentes instaladas en el sistema operativo (`C:/Windows/Fonts`, `%LOCALAPPDATA%/Microsoft/Windows/Fonts` en Windows; y rutas equivalentes en Linux/macOS) además de la carpeta local `fonts/`.
   - **Soporte tipográfico completo para español**: Codepoints ASCII (32–126), Suplemento Latin-1 (tildes, diéresis, `ñ`, `¿`, `¡`), comillas tipográficas, guiones y caracteres especiales.
   - **Filtrado bilineal de texturas**: Renderizado nítido a cualquier escala.
   - **Menú selector visual**: Botón desplegable interactivo en la barra superior con soporte de scroll y selección en caliente.

3. **Compatibilidad Total y Manejo de CSV**:
   - Detección automática de delimitador (`,`, `;`, `\t`) y codificación (UTF-8 con/sin BOM).
   - Preservación de columnas adicionales existentes en el archivo `glosario.csv`.
   - **Respaldos automáticos**: Crea copias de seguridad fechadas en `respaldos/glosario_YYYYMMDD_HHMMSS.csv` antes de modificar el archivo.
   - Soporte para disco RAM `R:/` si está presente, compatible con el flujo de trabajo de Noten.

4. **Modo Búsqueda / Filtrado en Tiempo Real**:
   - Tecla rápida `F3` o botón `MODO BÚSQUEDA`.
   - Filtrado simultáneo sobre cualquiera de los 6 campos con normalización de caracteres (ignora acentos y mayúsculas).

5. **Ordenamiento Alfabético**:
   - Clic en cualquiera de los encabezados de columnas:
     - 1er clic: Ascendente `[A-Z]`
     - 2do clic: Descendente `[Z-A]`
     - 3er clic: Restablecer al orden original del CSV
   - Botón `Orden Original` visible cuando hay un orden activo.

6. **Algoritmo de Similitud y Prevención de Duplicados**:
   - Implementación C++ del algoritmo de coincidencia de secuencias (Ratcliff-Obershelp / `difflib.SequenceMatcher`).
   - Ventana modal de confirmación si se detectan términos similares existentes (mostrando porcentaje de coincidencia y detalles).

---

## 🛠️ Estructura del Código

```
Termen/
├── CMakeLists.txt              # Configuración CMake con FetchContent para Raylib
├── LICENSE                     # Licencia Apache 2.0
├── .gitignore
├── .vscode/
│   └── tasks.json              # Tareas de compilación para VS Code
├── fonts/                      # Carpeta local para colocar archivos .ttf o .otf
├── include/
│   ├── app.hpp                 # Controlador principal y renderizado de interfaz
│   ├── font_system.hpp         # Sistema de detección y caché de fuentes
│   ├── glossary.hpp            # Carga, guardado, respaldos y validación de CSV
│   └── text_utils.hpp          # Normalización, UTF-8 y cálculo de similitud
└── src/
    ├── app.cpp
    ├── font_system.cpp
    ├── glossary.cpp
    ├── text_utils.cpp
    └── main.cpp
```

---

## 🚀 Compilación y Ejecución

### Opción A: Compilación rápida con `g++` (usando Raylib de Noten)

Si ya tienes compilado el proyecto hermano `Noten`:

```powershell
g++ -std=c++23 -O2 src/main.cpp src/app.cpp src/font_system.cpp src/glossary.cpp src/text_utils.cpp src/config_manager.cpp -Iinclude -I../Noten/build/_deps/raylib-src/src -L../Noten/build/_deps/raylib-build/raylib -lraylib -lopengl32 -lgdi32 -lwinmm -lshell32 -mwindows -o Termen.exe
```

### Opción B: Con CMake y Ninja

```powershell
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

---

## ⚙️ Configuración Compartida (`config.json`)

El archivo `config.json` es compartido directamente con el proyecto [Noten](https://github.com/wikiaxman/Noten) sin conflictos:

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

- `"size"`: Mantiene el tamaño de letra del editor de texto de Noten (p. ej. 64).
- `"glossary_size"`: Almacena el tamaño de letra unificado para toda la aplicación Termen (10–30 px).
- `"font"`: Fuente compartida entre ambas aplicaciones.

---

## ⌨️ Atajos de Teclado

| Atajo | Función |
| :--- | :--- |
| `Ctrl + +` / `Ctrl + -` | Aumentar / Disminuir tamaño de letra unificado en toda la app (o botones `A-` / `A+`) |
| `Ctrl + Rueda Ratón` | Ajustar tamaño de letra con el ratón |
| `Ctrl + 0` | Restablecer tamaño de letra predeterminado (15 px) |
| `Escape` | Limpiar filtro / Cancelar edición / Abrir diálogo de confirmación de salida |
| `Escape` (en diálogo) | Salir y cerrar la aplicación |
| `Cualquier otra tecla` (en diálogo) | Cancelar y permanecer en el editor |
| `F3` o `Ctrl + F` | Alternar Modo Búsqueda / Modo Registro |
| `Tab` / `Shift + Tab` | Navegar hacia el siguiente / anterior cuadro de texto |
| `Flecha Arriba` / `Flecha Abajo` | Cambiar cuadro de texto activo |
| `Enter` | Guardar registro (Modo Registro) / Siguiente campo (Modo Búsqueda) |
| `Ctrl + X` | Borrar texto del campo activo |
| `Ctrl + V` | Pegar texto del portapapeles |
| `Enter` o `S` | Confirmar guardado en la ventana modal de similitud |
| `Escape` o `N` | Cancelar y volver a editar en la ventana modal de similitud |

---

## 📜 Licencia

Este proyecto está distribuido bajo la Licencia **Apache 2.0**. Para más información sobre los términos de uso, reproducción, distribución y concesión de derechos de autor y patentes, consulta el archivo [LICENSE](LICENSE).
