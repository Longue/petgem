 
/* C libraries */ 


/* PETSc libraries */
#include <petscsys.h>
#include <petscksp.h> 
#include <petscdmplex.h>
#include "inputs.h"


/* PETGEM functions */ 


// =============================================================================
// Function: readUserParams
// =============================================================================

/**
 * @brief Solves the linear system AX=B using KSP.
 * @param[in] dm The DMPlex object (used for communicator).
 * @param[in] A The system matrix (assembled by assembleSystem).
 * @param[in] B The right-hand side matrix (assembled by assembleSystem, one column per source).
 * @param[in] G The discrete gradient matrix (used for PCBDDC setup if A is MATIS).
 * @param[in] params A Params struct containing simulation parameters (used for PCBDDC setup).
 * @param[out] X Pointer to the solution matrix (Mat) to be created and populated.
 * @return PetscErrorCode PETSC_SUCCESS on successful solve.
 * @details Creates a KSP context. Sets the operators (A for matrix, A for preconditioner matrix by default).
 *          If the matrix A is of type MATIS and G is provided, it configures the preconditioner
 *          to PCBDDC and sets the discrete gradient using `PCBDDCSetDiscreteGradient`.
 *          Sets KSP options from the command line/options file.
 *          Creates the solution matrix X with the appropriate size and type.
 *          Calls `KSPMatSolve` to solve for multiple right-hand sides.
 *          Prints status messages. Destroys the KSP context.
 */

PetscErrorCode solveSystem(DM dm, Mat A, Mat B, Mat G, Params params, Mat *X){
    
    PetscFunctionBeginUser;
    
    /* Create KSP object */
    MPI_Comm comm = PetscObjectComm((PetscObject)dm);
    KSP ksp;

    /* Setup solver and run it */
    PetscCall(KSPCreate(comm, &ksp));
    PetscCall(KSPSetOperators(ksp, A, A));

    PetscBool ismatis = PETSC_FALSE;
    PetscCall(PetscObjectTypeCompare((PetscObject)A, MATIS, &ismatis));
    if (ismatis && G) {
      PC pc;

      PetscCall(KSPGetPC(ksp, &pc));
      PetscCall(PCSetType(pc, PCBDDC));
      PetscCall(PCBDDCSetDiscreteGradient(pc, G, params.nord, 0, PETSC_TRUE, PETSC_TRUE));
    }
    PetscCall(KSPSetFromOptions(ksp));

    PetscInt M, N, m, n;
    VecType vtype;
    PetscCall(MatGetSize(B, &M, &N));
    PetscCall(MatGetLocalSize(B, &m, &n));
    PetscCall(MatGetVecType(A, &vtype));
    PetscCall(MatCreateDenseFromVecType(comm, vtype, m, n, M, N, m, NULL, X));
    PetscCall(PetscPrintf(comm, "\n Solution of %" PetscInt_FMT " linear systems:\n", N));
    PetscCall(PetscPrintf(comm, "   Solver process    = Initiated\n"));
    PetscCall(KSPMatSolve(ksp, B, *X));    
    PetscCall(PetscPrintf(comm, "   Solver process    = Finished\n"));
    PetscCall(KSPDestroy(&ksp));  
    
    PetscFunctionReturn(PETSC_SUCCESS);
}
