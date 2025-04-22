# PETGEM Makefile

# Target executable
TARGET := build/kernel
all: $(TARGET)

include ${PETSC_DIR}/lib/petsc/conf/variables
include ${PETSC_DIR}/lib/petsc/conf/rules

# Conditional flag for Extrae support ( set 1 to include Extrae, 0 to exclude)
USE_EXTRAE ?= 0

# Add Extrae includes and flags if USE_EXTRAE is set to 1
ifeq ($(USE_EXTRAE), 1)
    E_CFLAGS := -I$(EXTRAE_HOME)/include -DUSE_EXTRAE
    E_LDFLAGS := -L$(EXTRAE_HOME)/lib -lmpitrace
endif

# Our include folder
I_CFLAGS := -Iinclude

# List of source files
SRCS := src/kernel.c src/common.c src/inputs.c src/source.c src/grid.c src/assembly.c src/hvfem.c src/solver.c src/postprocessing.c

# List of object files
OBJS := $(SRCS:.c=.o)

# Compile all object files and generate the final executable
$(TARGET): $(OBJS)
	$(CLINKER) $^ -o $@ $(CFLAGS) $(E_LDFLAGS) $(PETSC_LIB)

# Rule to compile each source file (uses PETSc's makefile variable)
%.o: %.c
	${PETSC_COMPILE_SINGLE} $(CFLAGS) $(I_CFLAGS) $(E_FLAGS) $< -o $@

# --- Targets de Documentación ---

DOXYFILE = Doxyfile
SPHINX_PYTHON = python3 # O simplemente python si está en el PATH
SPHINX_SCRIPT_DIR = scripts/generate_index
SPHINX_GENERATOR_SCRIPT = $(SPHINX_SCRIPT_DIR)/generate_sphinx_structure.py
SPHINX_SOURCE_DIR = docs/source
SPHINX_BUILD_DIR = docs/build
SPHINX_BUILD = $(SPHINX_PYTHON) -m sphinx # Forma recomendada de llamar a Sphinx

# Target para generar la documentación completa
# Se quitan las dependencias de archivos fuente específicos para ejecutar siempre
# que se llame, confiando en 'make clean' para forzar reconstrucción.
docs: run_doxygen run_script_generator run_sphinx

run_doxygen:
	@echo ">>> [DOCS] Generando Doxygen XML..."
	doxygen $(DOXYFILE)

run_script_generator:
	@echo ">>> [DOCS] Ejecutando script generador de estructura Sphinx..."
	$(SPHINX_PYTHON) $(SPHINX_GENERATOR_SCRIPT)

run_sphinx:
	@echo ">>> [DOCS] Construyendo documentación Sphinx HTML..."
	$(SPHINX_BUILD) -b html $(SPHINX_SOURCE_DIR) $(SPHINX_BUILD_DIR)/html
	@echo ">>> [DOCS] Documentación HTML generada en $(SPHINX_BUILD_DIR)/html"


# --- Targets de Limpieza ---

# Limpia solo los artefactos del kernel
clean_kernel:
	@echo ">>> [CLEAN] Limpiando archivos de compilación PETSc/kernel..."
	rm -f $(OBJS) $(TARGET)
	@echo ">>> [CLEAN] Artefactos del kernel eliminados."

# Limpia solo los artefactos de documentación
clean_doc:
	@echo ">>> [CLEAN] Limpiando directorios de documentación (build y doxygen)..."
	rm -rf $(SPHINX_BUILD_DIR)/* docs/doxygen/*
	@echo ">>> [CLEAN] Artefactos de documentación eliminados."

clean_all: clean_kernel clean_doc
	@echo ">>> [CLEAN_ALL] Ejecutando limpieza de PETSc (si existe)..."
	$(MAKE) clean # Llama al 'clean' (probablemente de PETSc)
	@echo ">>> [CLEAN_ALL] Limpieza completa finalizada."