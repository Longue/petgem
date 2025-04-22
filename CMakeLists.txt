# CMakeLists.txt para PETGEM (Versión C - TFG)

# Versión mínima requerida de CMake
cmake_minimum_required(VERSION 3.10)

# Definición del Proyecto
project(petgem_c VERSION 0.1 LANGUAGES C)

# --- Configuración del Compilador (Opcional pero recomendado) ---
# Establecer el estándar C (ej. C11)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
# Podrías añadir flags de compilación generales aquí si es necesario
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
# Para Debug (si compilaste PETSc con debugging):
# set(CMAKE_BUILD_TYPE Debug)

# --- Encontrar PETSc ---
# Busca una instalación de PETSc usando las variables de entorno PETSC_DIR y PETSC_ARCH
# Es crucial que estas variables estén definidas al ejecutar cmake
find_package(PETSc REQUIRED)
# Si se encuentra, PETSc define variables como:
# PETSC_FOUND, PETSC_INCLUDE_DIRS, PETSC_LIBRARIES, PETSC_DEFINITIONS

# --- Opción para Extrae (Replica el USE_EXTRAE del Makefile) ---
option(USE_EXTRAE "Enable Extrae support (requires EXTRAE_HOME env var)" OFF)

# --- Definir Archivos Fuente y Directorios de Inclusión ---

# Añadir el directorio local 'include' a la ruta de búsqueda de headers
# Es importante hacerlo ANTES de add_executable si los sources incluyen headers locales
include_directories(include)
# Alternativa moderna (mejor asociar al target):
# target_include_directories(kernel PRIVATE include) # Se añade después de add_executable

# Listar los archivos fuente .c en el directorio src/
# Usar file(GLOB ...) es simple pero CMake advierte si añades/quitas archivos
# (necesitarías re-ejecutar cmake). Para este proyecto está bien.
file(GLOB SOURCE_FILES "src/*.c")

# --- Definir el Ejecutable Principal ---
add_executable(kernel ${SOURCE_FILES})

# --- Enlazar con PETSc y configurar includes/definiciones ---
# Añadir directorios de inclusión de PETSc al target 'kernel'
target_include_directories(kernel PRIVATE ${PETSC_INCLUDE_DIRS})

# Añadir definiciones de preprocesador de PETSc al target 'kernel'
target_compile_definitions(kernel PRIVATE ${PETSC_DEFINITIONS})

# Enlazar el target 'kernel' con las bibliotecas de PETSc
target_link_libraries(kernel PRIVATE ${PETSC_LIBRARIES})

# Asociar nuestro directorio local 'include' específicamente a este target (mejor práctica)
target_include_directories(kernel PRIVATE include)

# --- Configuración Condicional de Extrae ---
if(USE_EXTRAE)
    # Verificar si la variable de entorno EXTRAE_HOME está definida
    if(NOT DEFINED ENV{EXTRAE_HOME})
        message(FATAL_ERROR "USE_EXTRAE is ON but EXTRAE_HOME environment variable is not set.")
    else()
        message(STATUS "Extrae support enabled. Using EXTRAE_HOME=$ENV{EXTRAE_HOME}")
        # Añadir directorio de include de Extrae
        target_include_directories(kernel PRIVATE $ENV{EXTRAE_HOME}/include)
        # Añadir definición para activar código condicional en C
        target_compile_definitions(kernel PRIVATE USE_EXTRAE)
        # Añadir bibliotecas de Extrae (usando el formato del Makefile)
        # Nota: Usar find_library sería más robusto, pero esto replica el Makefile.
        target_link_libraries(kernel PRIVATE "-L$ENV{EXTRAE_HOME}/lib" -lmpitrace)
    endif()
endif()

# --- Configuración de la Salida (Opcional) ---
# Por defecto, CMake pondrá 'kernel' en el directorio raíz del build.
# Si quieres replicar exactamente 'build/kernel' desde la raíz del proyecto:
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
# Sin embargo, es más estándar dejar que CMake maneje el directorio de build.
# El ejecutable estará en: /ruta/a/tu/directorio_build/kernel

# --- Fin del CMakeLists.txt ---

message(STATUS "PETSc Include Dirs: ${PETSC_INCLUDE_DIRS}")
message(STATUS "PETSc Libraries: ${PETSC_LIBRARIES}")