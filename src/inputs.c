/*
 * Filename: inputs.h
 * Author: Octavio Castillo Reyes (UPC/BSC)
 * Date: 2024-10-04
 *
 * Description:
 * This file contains functions for handling input data and user parameters in a PETGEM simulation. 
 * It includes functions for parsing input data, and processing user-provided parameters. 
 * The functions in this file facilitate the setup and configuration of the PETGEM code.
 *
 * List of Functions:
 * - void readUserParams(): Read user parameters for PETGEM simulation. 
*/

/* C libraries */ 
#include <time.h>
#include <stdio.h>
#include <sys/stat.h>

/* PETSc libraries */
#include <petscsys.h>

/* PETGEM functions */ 
#include "common.h"
#include "inputs.h"  

// =============================================================================
// Function: readUserParams
// =============================================================================
PetscErrorCode readUserParams(Params *params, PetscMPIInt size) {

    PetscFunctionBeginUser;
    
    /* Variables declaration */
    char  meshFilename[PETSC_MAX_PATH_LEN];
    char  receiversFilename[PETSC_MAX_PATH_LEN];
    char  outputDir[PETSC_MAX_PATH_LEN];
    char  outputFilename[PETSC_MAX_PATH_LEN];
    char  sourceFilename[PETSC_MAX_PATH_LEN];
    char  mode[PETSC_MAX_PATH_LEN];
    PetscBool   meshFilenameIsPresent, receiversFilenameIsPresent, nordIsPresent, modeIsPresent, sourceFilenameIsPresent;
    PetscBool   outputDirIsPresent, outputFilenameIsPresent, isCSEM, isMT;
    PetscInt    nord; /* Basis order = 1, 2, 3, 4, 5, 6 */    
    
    /* Read mesh filename (hdf5 format) */
    PetscCall(PetscOptionsGetString(NULL, NULL, "-mesh_filename", meshFilename, sizeof(meshFilename), &meshFilenameIsPresent));  
    PetscCheck(meshFilenameIsPresent, PETSC_COMM_WORLD, PETSC_ERR_ARG_NULL, "Exiting: Mesh file missing. Mandatory parameter required for simulation.\n");
    PetscCall(PetscStrncpy(params->meshFile, meshFilename, sizeof(params->meshFile)));

    /* Read receivers filename (hdf5 format) */
    PetscCall(PetscOptionsGetString(NULL, NULL, "-receivers_filename", receiversFilename, sizeof(receiversFilename), &receiversFilenameIsPresent));  
    PetscCheck(receiversFilenameIsPresent, PETSC_COMM_WORLD, PETSC_ERR_ARG_NULL, "Exiting: Receivers file missing. Mandatory parameter required for simulation.\n");
    PetscCall(PetscStrncpy(params->receiversFile, receiversFilename, sizeof(params->receiversFile)));

    /* Read output directory */
    PetscCall(PetscOptionsGetString(NULL, NULL, "-output_dir", outputDir, sizeof(outputDir), &outputDirIsPresent));  
    PetscCheck(outputDirIsPresent, PETSC_COMM_WORLD, PETSC_ERR_ARG_NULL, "Exiting: Output directory missing. Mandatory parameter required for simulation.\n");
    PetscCall(PetscStrncpy(params->outputDirectory, outputDir, sizeof(params->outputDirectory)));

    /* Read output filename */
    PetscCall(PetscOptionsGetString(NULL, NULL, "-output_filename", outputFilename, sizeof(outputFilename), &outputFilenameIsPresent));  
    PetscCheck(outputFilenameIsPresent, PETSC_COMM_WORLD, PETSC_ERR_ARG_NULL, "Exiting: Output filename missing. Mandatory parameter required for simulation.\n");
    PetscCall(PetscStrncpy(params->outputFilename, outputFilename, sizeof(params->outputFilename)));
    
    /* Read basis order (nord = 1, 2, 3, 4, 5, 6) */
    PetscCall(PetscOptionsGetInt(NULL, NULL, "-nord", &nord, &nordIsPresent));
    PetscCheck(nordIsPresent, PETSC_COMM_WORLD, PETSC_ERR_ARG_NULL, "Exiting: Nord parameter missing. Mandatory parameter required for simulation.\n");
    /* Check is a valid basis order */
    nordIsPresent = (nord >= 1 && nord <= 6) ? PETSC_TRUE : PETSC_FALSE;
    PetscCheck(nordIsPresent, PETSC_COMM_WORLD, PETSC_ERR_ARG_NULL, "Exiting: Nord parameter out of valid range (nord = 1, 2, 3, 4, 5, 6).\n");
    params->nord = nord;
    
    /* Read modeling mode (csem or mt) */
    PetscCall(PetscOptionsGetString(NULL, NULL, "-mode", mode, sizeof(mode), &modeIsPresent));  
    PetscCheck(modeIsPresent, PETSC_COMM_WORLD, PETSC_ERR_ARG_NULL, "Exiting: Mode parameter missing. Mandatory parameter required for simulation.\n");
    /* Check if is a valid mode type */
    PetscCall(PetscStrcasecmp(mode, "CSEM", &isCSEM));
    PetscCall(PetscStrcasecmp(mode, "MT", &isMT));
    PetscCheck(isCSEM || isMT, PETSC_COMM_WORLD, PETSC_ERR_ARG_WRONG,  "Exiting: Expected mode type 'CSEM' or 'MT'.\n");
    if (isCSEM) {
        PetscCall(PetscStrncpy(params->mode, "CSEM", sizeof(params->mode)));
    } else {
        PetscCall(PetscStrncpy(params->mode, "MT", sizeof(params->mode)));
    }

    /* Read source filename */
    PetscCall(PetscOptionsGetString(NULL, NULL, "-source_filename", sourceFilename, sizeof(sourceFilename), &sourceFilenameIsPresent));  
    PetscCheck(sourceFilenameIsPresent, PETSC_COMM_WORLD, PETSC_ERR_ARG_NULL, "Exiting: Source filename missing. Mandatory parameter required for simulation.\n");
    PetscCall(PetscStrncpy(params->sourceFilename, sourceFilename, sizeof(params->sourceFilename)));
    
    /* Number of MPI tasks */ 
    params->numMPITasks = size;
   
    /* Create output directory */
    createDirectory(outputDir);
    
    PetscFunctionReturn(PETSC_SUCCESS);
}
