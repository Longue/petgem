 
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
