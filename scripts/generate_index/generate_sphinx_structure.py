#!/usr/bin/env python3

import os
from pathlib import Path
import argparse

# --- Configuration ---
# List of source files (relative to src_dir defined later)
# Order matters for the API index generation
SOURCE_FILES_ORDERED = [
    "common.c",
    "inputs.c",
    "source.c",
    "grid.c",
    "hvfem.c",
    "assembly.c",
    "solver.c",
    "postprocessing.c",
    "kernel.c",
]

# Basic pages to create
CONTENT_PAGES = [
    "installation",
    "usage",
    "theory",
    "contributing",
]
# --- End Configuration ---

def create_file(filepath: Path, content: str):
    """Creates a file with the given content, creating parent directories if needed."""
    try:
        filepath.parent.mkdir(parents=True, exist_ok=True)
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"  Created: {filepath}")
    except IOError as e:
        print(f"  Error creating {filepath}: {e}")
    except Exception as e:
        print(f"  Unexpected error creating {filepath}: {e}")


def generate_api_page(api_dir: Path, source_filename: str, project_name: str):
    """Generates the content for an individual API page (.rst)."""
    base_name = os.path.splitext(source_filename)[0]
    title = f"{base_name.capitalize()} Module (`{source_filename}`)"
    underline = "=" * len(title)

    content = f""".. _api-{base_name}:

{title}
{underline}

.. Integrates all documentation from the specified C file.
.. Adjust directives below if you prefer fine-grained control (e.g., doxygenfunction).

.. doxygenfile:: {source_filename}
   :project: {project_name}

.. Uncomment and list specific functions/structs if needed:
   Functions
   ---------
   .. doxygenfunction:: function_name_1
      :project: {project_name}
   .. doxygenfunction:: function_name_2
      :project: {project_name}

   Structures
   ----------
   .. doxygenstruct:: struct_name_1
      :project: {project_name}
      :members:

"""
    create_file(api_dir / f"{base_name}.rst", content)

def generate_api_index(api_dir: Path, ordered_source_files: list):
    """Generates the index.rst file for the API directory."""
    title = "API Reference"
    underline = "=" * len(title)
    toctree_entries = [f"   {os.path.splitext(fname)[0]}" for fname in ordered_source_files]

    content = f""".. _api-index:

{title}
{underline}

This section provides the detailed API documentation extracted directly from the PETGEM source code comments.

.. toctree::
   :maxdepth: 1
   :caption: Modules:

{os.linesep.join(toctree_entries)}

"""
    create_file(api_dir / "index.rst", content)


def generate_main_index(source_dir: Path, content_pages: list, api_exists: bool):
    """Generates the main index.rst file."""
    title = "PETGEM Documentation"
    underline = "=" * len(title)

    toctree_entries = content_pages[:] # Copy the list
    if api_exists:
        toctree_entries.append("api/index") # Add link to API index

    toctree_content = [f"   {page}" for page in toctree_entries]

    content = f""".. PETGEM documentation master file, created by generate_sphinx_structure.py

Welcome to PETGEM's documentation!
==================================

**PETGEM: Parallel Edge-based Tool for Electromagnetic Modelling**

This documentation provides guides on installation, usage, theory, and the detailed API reference for PETGEM.

.. toctree::
   :maxdepth: 2
   :caption: Contents:

{os.linesep.join(toctree_content)}

Indices and tables
==================

* :ref:`genindex`
* :ref:`search`

"""
    create_file(source_dir / "index.rst", content)

def generate_content_page(source_dir: Path, page_name: str):
    """Generates a basic placeholder for a content page."""
    title = page_name.capitalize().replace('_', ' ')
    underline = "=" * len(title)
    content = f""".. _{page_name}:

{title}
{underline}

.. todo::
    Write content for the {title} section.

"""
    create_file(source_dir / f"{page_name}.rst", content)


# --- Main Execution ---
if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Generate basic Sphinx documentation structure for PETGEM.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )
    parser.add_argument(
        "--docs-dir",
        type=Path,
        default=Path("docs"), # Default relative to where script is run
        help="Main directory for documentation (e.g., 'docs')."
    )
    parser.add_argument(
        "--project-name",
        type=str,
        default="PETGEM",
        help="Project name used in Breathe directives (must match breathe_projects key in conf.py)."
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="Overwrite existing .rst files if they exist."
    )

    args = parser.parse_args()

    docs_dir = args.docs_dir.resolve()
    source_dir = docs_dir / "source"
    api_dir = source_dir / "api"

    print(f"--- Generating Sphinx Structure in: {docs_dir} ---")
    print(f"Project Name: {args.project_name}")

    # --- Create API Pages ---
    print("\nCreating API .rst files...")
    if api_dir.exists() and not args.force:
        print(f"  Directory '{api_dir}' already exists. Use --force to potentially overwrite files within.")
    api_dir.mkdir(parents=True, exist_ok=True) # Ensure API dir exists

    # Generate individual API files
    for fname in SOURCE_FILES_ORDERED:
        target_rst = api_dir / f"{os.path.splitext(fname)[0]}.rst"
        if not target_rst.exists() or args.force:
             generate_api_page(api_dir, fname, args.project_name)
        else:
             print(f"  Skipped (exists): {target_rst}")

    # Generate API index file
    api_index_rst = api_dir / "index.rst"
    if not api_index_rst.exists() or args.force:
        generate_api_index(api_dir, SOURCE_FILES_ORDERED)
    else:
        print(f"  Skipped (exists): {api_index_rst}")

    # --- Create Content Pages ---
    print("\nCreating Content .rst files...")
    if source_dir.exists() and not args.force:
         print(f"  Directory '{source_dir}' already exists. Use --force to potentially overwrite files within.")
    source_dir.mkdir(parents=True, exist_ok=True) # Ensure source dir exists

    for page in CONTENT_PAGES:
        target_rst = source_dir / f"{page}.rst"
        if not target_rst.exists() or args.force:
            generate_content_page(source_dir, page)
        else:
            print(f"  Skipped (exists): {target_rst}")

    # --- Create Main Index Page ---
    print("\nCreating main index.rst...")
    main_index_rst = source_dir / "index.rst"
    if not main_index_rst.exists() or args.force:
        generate_main_index(source_dir, CONTENT_PAGES, api_dir.exists()) # Pass api_exists=True
    else:
        print(f"  Skipped (exists): {main_index_rst}")

    print("\n--- Structure Generation Complete ---")
    print(f"Basic Sphinx source structure generated in '{source_dir}'.")
    print("Next steps:")
    print("1. Create/Configure 'conf.py' inside '{source_dir}'.")
    print("2. Create/Configure 'Doxyfile' in '{docs_dir}'.")
    print("3. Create/Configure 'Makefile' in '{docs_dir}'.")
    print("4. Populate the content pages (.rst files).")
    print("5. Run 'make html' from '{docs_dir}' to build.")