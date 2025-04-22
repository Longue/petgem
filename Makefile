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

# Clean rule
clean::
	@echo ">>> Limpiando archivos de compilación PETSc/kernel..."
	rm -f $(OBJS) $(TARGET) # Asumiendo que OBJS y TARGET están definidos arriba
	# O lo que sea que limpie el ejecutable 'kernel' y sus objetos

	@echo ">>> Limpiando directorios de documentación (build y doxygen)..."
	rm -rf docs/build/* docs/doxygen/*
	@echo ">>> Limpieza completada."

## CI RULES
# AutoDoc
DOXYFILE = Doxyfile
SPHINX_SOURCE_DIR = docs/source
SPHINX_BUILD_DIR = docs/build

docs: $(SRCS) include/*.h $(DOXYFILE) $(SPHINX_SOURCE_DIR)/*.rst $(SPHINX_SOURCE_DIR)/conf.py
	@echo "Generating Doxygen XML..."
	doxygen $(DOXYFILE)
	@echo "Building Sphinx HTML documentation..."
	sphinx-build -b html $(SPHINX_SOURCE_DIR) $(SPHINX_BUILD_DIR)/html