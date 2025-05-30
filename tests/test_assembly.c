// tests/test_assembly.c
#include "/opt/unity/src/unity.h"
#include "../include/assembly.h"
#include "../include/inputs.h"
#include "../include/grid.h"
#include "../include/source.h"
#include "../include/constants.h" // For NUM_DIMENSIONS, PETSC_PI, MU

// PETSc includes (ahora con petsc.h)
#include <petsc.h>


// Dummy PETSc objects for testing
static DM       test_dm = NULL;
static Vec      test_resistivity = NULL;
static Mat      test_A = NULL;
static Mat      test_B = NULL;
static Mat      test_G = NULL;
static Grid     test_grid;
static setSource test_sources;
static Params   test_params;

void setUp_assembly(void) {
    PetscCallVoid(PetscOptionsClear(NULL));

    if (test_A) PetscCallVoid(MatDestroy(&test_A));
    if (test_B) PetscCallVoid(MatDestroy(&test_B));
    if (test_G) PetscCallVoid(MatDestroy(&test_G));
    if (test_dm) PetscCallVoid(DMDestroy(&test_dm));
    if (test_resistivity) PetscCallVoid(VecDestroy(&test_resistivity));

    memset(&test_params, 0, sizeof(Params));
    test_params.nord = 1;
    strcpy(test_params.mode, "CSEM");
    test_params.numMPITasks = 1;

    memset(&test_grid, 0, sizeof(Grid));
    test_grid.dim = 3; test_grid.numDofInVertex = 1; test_grid.numDofInEdge = 1;
    test_grid.numDofInFace = 0; test_grid.numDofInVolume = 0;
    test_grid.numDofInCell = 6; test_grid.numH1DofInCell = 4;
    test_grid.cellStart = 0; test_grid.cellEnd = 1;

    memset(&test_sources, 0, sizeof(setSource));
    test_sources.numSources = 1; test_sources.freq = 1.0;
    test_sources.sourceArray = (Source*)malloc(sizeof(Source) * test_sources.numSources);
    memset(test_sources.sourceArray, 0, sizeof(Source) * test_sources.numSources);
    test_sources.sourceArray[0].position[0] = 0.5;
    test_sources.sourceArray[0].position[1] = 0.5;
    test_sources.sourceArray[0].position[2] = 0.5;
    test_sources.sourceArray[0].current = 1.0;
    test_sources.sourceArray[0].length = 1.0;
    test_sources.sourceArray[0].dip = 0.0;
    test_sources.sourceArray[0].azimuth = 0.0;

    PetscInt dim = 3, cells[] = {1,1,1};
    DMBoundaryType periodicity[3] = {DM_BOUNDARY_NONE, DM_BOUNDARY_NONE, DM_BOUNDARY_NONE};
    PetscCallVoid(DMPlexCreateBoxMesh(PETSC_COMM_WORLD, dim, PETSC_FALSE, cells, NULL, NULL, periodicity, PETSC_TRUE, 0, PETSC_FALSE, &test_dm));
    PetscCallVoid(DMSetFromOptions(test_dm));
    PetscCallVoid(DMSetUp(test_dm));

    PetscCallVoid(VecCreate(PETSC_COMM_WORLD, &test_resistivity));
    PetscCallVoid(VecSetSizes(test_resistivity, PETSC_DECIDE, 3));
    PetscCallVoid(VecSetFromOptions(test_resistivity));
    PetscCallVoid(VecSet(test_resistivity, 1.0));
}

void tearDown_assembly(void) {
    if (test_A) PetscCallVoid(MatDestroy(&test_A));
    if (test_B) PetscCallVoid(MatDestroy(&test_B));
    if (test_G) PetscCallVoid(MatDestroy(&test_G));
    if (test_dm) PetscCallVoid(DMDestroy(&test_dm));
    if (test_resistivity) PetscCallVoid(VecDestroy(&test_resistivity));
    if (test_sources.sourceArray) free(test_sources.sourceArray);
}

void test_assembleSystem_CSEM_nord1(void) {
    PetscCallVoid(DMPlexCreateBoxMesh(PETSC_COMM_WORLD, test_grid.dim, PETSC_FALSE, (PetscInt[]){1,1,1}, NULL, NULL, (DMBoundaryType[]){DM_BOUNDARY_NONE, DM_BOUNDARY_NONE, DM_BOUNDARY_NONE}, PETSC_TRUE, 0, PETSC_FALSE, &test_grid.H1dm));
    PetscCallVoid(DMSetFromOptions(test_grid.H1dm));
    PetscCallVoid(DMSetUp(test_grid.H1dm));

    PetscErrorCode ierr = assembleSystem(test_dm, test_resistivity, test_grid, test_sources, test_params, &test_A, &test_B, &test_G);
    TEST_ASSERT_EQUAL_INT(PETSC_SUCCESS, ierr);

    TEST_ASSERT_NOT_NULL(test_A);
    TEST_ASSERT_NOT_NULL(test_B);
    TEST_ASSERT_NOT_NULL(test_G);

    PetscInt M_A, N_A, M_B, N_B, M_G, N_G;
    PetscCallVoid(MatGetSize(test_A, &M_A, &N_A));
    PetscCallVoid(MatGetSize(test_B, &M_B, &N_B));
    PetscCallVoid(MatGetSize(test_G, &M_G, &N_G));

    TEST_ASSERT_TRUE(M_A > 0); TEST_ASSERT_TRUE(N_A > 0); TEST_ASSERT_EQUAL_INT(M_A, N_A);
    TEST_ASSERT_TRUE(M_B > 0); TEST_ASSERT_EQUAL_INT(test_sources.numSources, N_B);
    TEST_ASSERT_TRUE(M_G > 0); TEST_ASSERT_TRUE(N_G > 0); TEST_ASSERT_EQUAL_INT(M_G, M_A);
}

void suite_assembly(void) {
    setUp_assembly();
    RUN_TEST(test_assembleSystem_CSEM_nord1);
    tearDown_assembly();
}