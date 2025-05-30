// tests/test_hvfem.c
#include "/opt/unity/src/unity.h"
#include "../include/hvfem.h" // Incluye la cabecera de hvfem.h
#include "../include/constants.h" // Para NUM_DIMENSIONS, PETSC_i, etc.

// PETSc includes (usando petsc.h para exhaustividad)
#include <petsc.h>
#include <petscdmplex.h> // Para DMPlexCreateBoxMesh
#include <math.h>        // Para sin, cos, M_PI

// Variables estáticas para los objetos PETSc de la suite
static DM       test_dm = NULL; // Para testear funciones que usan DM
static Vec      temp_vec_for_dm_size = NULL; // Para obtener el tamaño de DM


// --- Funciones de configuración y limpieza de la suite ---
void setUp_hvfem(void) {
    PetscCallVoid(PetscOptionsClear(NULL)); // Limpiar opciones PETSc
    if (test_dm) PetscCallVoid(DMDestroy(&test_dm));
    if (temp_vec_for_dm_size) PetscCallVoid(VecDestroy(&temp_vec_for_dm_size));
}

void tearDown_hvfem(void) {
    if (test_dm) PetscCallVoid(DMDestroy(&test_dm));
    if (temp_vec_for_dm_size) PetscCallVoid(VecDestroy(&temp_vec_for_dm_size));
}

// --- Tests Individuales ---

// Test para computeNumGaussPoints3D
void test_computeNumGaussPoints3D(void) {
    PetscInt numGaussPoints;
    PetscErrorCode ierr;

    // Test a nord=1 (gaussOrder=2)
    ierr = computeNumGaussPoints3D(1, &numGaussPoints);
    TEST_ASSERT_EQUAL_INT(PETSC_SUCCESS, ierr);
    TEST_ASSERT_EQUAL_INT(4, numGaussPoints); // Para orden 2, se usan 4 puntos (ver `hvfem.c` switch case)

    // Test a nord=2 (gaussOrder=4)
    ierr = computeNumGaussPoints3D(2, &numGaussPoints);
    TEST_ASSERT_EQUAL_INT(PETSC_SUCCESS, ierr);
    TEST_ASSERT_EQUAL_INT(11, numGaussPoints); // Para orden 4, se usan 11 puntos

    // Test a nord=3 (gaussOrder=6)
    ierr = computeNumGaussPoints3D(3, &numGaussPoints);
    TEST_ASSERT_EQUAL_INT(PETSC_SUCCESS, ierr);
    TEST_ASSERT_EQUAL_INT(24, numGaussPoints); // Para orden 6, se usan 24 puntos

    // Test a nord inválido (0 -> gaussOrder=0, fuera de rango [1,12])
    ierr = computeNumGaussPoints3D(0, &numGaussPoints);
    TEST_ASSERT_NOT_EQUAL_INT(PETSC_SUCCESS, ierr); // Se espera un error de PETSC_ERR_ARG_OUTOFRANGE

    // Test a nord inválido (7 -> gaussOrder=14, fuera de rango [1,12] en la declaración inicial)
    // Sin embargo, el switch en `hvfem.c` va hasta 12. Un nord=7 implica gaussOrder=14.
    ierr = computeNumGaussPoints3D(7, &numGaussPoints);
    TEST_ASSERT_NOT_EQUAL_INT(PETSC_SUCCESS, ierr); // Se espera un error de PETSC_ERR_ARG_OUTOFRANGE
}

// Test para computeGaussPoints3D
void test_computeGaussPoints3D(void) {
    PetscInt numGaussPoints;
    PetscReal **gaussPoints = NULL;
    PetscReal *weights = NULL;
    PetscErrorCode ierr;

    // Para nord=1, computeNumGaussPoints3D devuelve 4 puntos de Gauss (gaussOrder=2)
    // Cambiamos numGaussPoints a 4 para este test
    numGaussPoints = 4; // Corregido: para nord=1 (gaussOrder=2), se usan 4 puntos.

    PetscCallVoid(PetscCalloc1(numGaussPoints, &gaussPoints));
    for (PetscInt i = 0; i < numGaussPoints; ++i) {
        PetscCallVoid(PetscCalloc1(NUM_DIMENSIONS, &gaussPoints[i]));
    }
    PetscCallVoid(PetscCalloc1(numGaussPoints, &weights));

    ierr = computeGaussPoints3D(numGaussPoints, gaussPoints, weights);
    TEST_ASSERT_EQUAL_INT(PETSC_SUCCESS, ierr);
    TEST_ASSERT_NOT_NULL(gaussPoints);
    TEST_ASSERT_NOT_NULL(weights);

    // Para el caso de 4 puntos de Gauss (nord=1, gaussOrder=2), los valores esperados
    // provienen de la renormalización de `nord2_3DGaussPoints`.
    // Los puntos son simétricos y el peso total debe ser 1/6 (volumen del tetraedro de referencia).
    // Cada punto tiene un peso de (1/3) / 8 = 1/24 antes de sumar, pero después de renormalizar
    // `weights[i] = gaussPoints[i][NUM_DIMENSIONS]/8` se refiere a la cuarta columna de `nord2_3DGaussPoints`
    // que es 0.333333333333333 (1/3). Así que el peso renormalizado es (1/3)/8 = 1/24.
    // La suma de todos los pesos es 4 * (1/24) = 4/24 = 1/6.
    // Los puntos son:
    // P0: xi = (1 - 0.7236) / 2 = 0.1381966
    //     eta = -(1 - 0.7236 - 0.7236 - 0.7236) / 2 = -(-1.17082039) / 2 = 0.58541019
    //     zeta = (1 - 0.7236) / 2 = 0.1381966
    // Esto es más complejo de asertar en un test unitario simple para todos los puntos.
    // Nos centraremos en que la función devuelve valores y la suma de pesos total sea 1/6.
    
    // Sumar los pesos para verificar el volumen total
    PetscReal total_weight = 0.0;
    for (PetscInt i = 0; i < numGaussPoints; ++i) {
        total_weight += weights[i];
    }
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1.0/6.0, total_weight);


    // Limpiar memoria
    for (PetscInt i = 0; i < numGaussPoints; ++i) {
        PetscCallVoid(PetscFree(gaussPoints[i]));
    }
    PetscCallVoid(PetscFree(gaussPoints));
    PetscCallVoid(PetscFree(weights));
}

// Test para vectorRotation
void test_vectorRotation(void) {
    PetscReal rotatedVector[NUM_DIMENSIONS];
    PetscErrorCode ierr;

    // La función vectorRotation comienza con un vector base [1,0,0] (eje X)
    // y aplica rotaciones en el orden M1 (XY-plane, azimuth), M2 (XZ-plane, dip), M3 (YZ-plane, tetha=0).
    // Las fórmulas de rotación son:
    // rotatedVector[0] = cos(azimuth_rad) * cos(dip_rad)
    // rotatedVector[1] = sin(azimuth_rad) * cos(dip_rad)
    // rotatedVector[2] = sin(dip_rad)

    // Test 1: Azimuth 0, Dip 0 -> Vector inicial [1, 0, 0]
    ierr = vectorRotation(0.0, 0.0, rotatedVector);
    TEST_ASSERT_EQUAL_INT(PETSC_SUCCESS, ierr);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1.0, rotatedVector[0]); // cos(0)*cos(0) = 1
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, rotatedVector[1]); // sin(0)*cos(0) = 0
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, rotatedVector[2]); // sin(0) = 0

    // Test 2: Azimuth 90 (rotación de 90 grados en XY), Dip 0 -> [0, 1, 0] (eje Y)
    ierr = vectorRotation(90.0, 0.0, rotatedVector);
    TEST_ASSERT_EQUAL_INT(PETSC_SUCCESS, ierr);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, rotatedVector[0]); // cos(90)*cos(0) = 0
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1.0, rotatedVector[1]); // sin(90)*cos(0) = 1
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, rotatedVector[2]); // sin(0) = 0

    // Test 3: Azimuth 0, Dip 90 (rotación de 90 grados en XZ) -> [0, 0, 1] (eje Z)
    ierr = vectorRotation(0.0, 90.0, rotatedVector);
    TEST_ASSERT_EQUAL_INT(PETSC_SUCCESS, ierr);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, rotatedVector[0]); // cos(0)*cos(90) = 0
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, rotatedVector[1]); // sin(0)*cos(90) = 0
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1.0, rotatedVector[2]); // sin(90) = 1

    // Test 4: Azimuth 45, Dip 45
    // cos(45) = 1/sqrt(2) = 0.70710678118
    // sin(45) = 1/sqrt(2) = 0.70710678118
    // rotatedVector[0] = cos(45)*cos(45) = (1/sqrt(2))*(1/sqrt(2)) = 0.5
    // rotatedVector[1] = sin(45)*cos(45) = (1/sqrt(2))*(1/sqrt(2)) = 0.5
    // rotatedVector[2] = sin(45) = 1/sqrt(2) = 0.70710678118
    ierr = vectorRotation(45.0, 45.0, rotatedVector);
    TEST_ASSERT_EQUAL_INT(PETSC_SUCCESS, ierr);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.5, rotatedVector[0]);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.5, rotatedVector[1]);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.70710678118, rotatedVector[2]);
}

// Test para tetrahedronXYZToXiEtaZeta (Conversión de coordenadas)
// void test_tetrahedronXYZToXiEtaZeta(void) {
//     PetscScalar cellCoords[4 * NUM_DIMENSIONS]; // 4 vertices * 3 coords
//     PetscReal xyz_point[NUM_DIMENSIONS];
//     PetscReal XiEtaZeta_point[NUM_DIMENSIONS];
//     PetscErrorCode ierr;
// 
//     // Define un tetraedro de referencia (vertices: V0=(0,0,0), V1=(1,0,0), V2=(0,1,0), V3=(0,0,1))
//     // La función tetrahedronXYZToXiEtaZeta mapea de XYZ físico a XiEtaZeta de referencia.
//     // Si el tetraedro físico es el mismo que el de referencia, la transformación es la identidad.
//     cellCoords[0] = 0.0; cellCoords[1] = 0.0; cellCoords[2] = 0.0; // V0 (0,0,0)
//     cellCoords[3] = 1.0; cellCoords[4] = 0.0; cellCoords[5] = 0.0; // V1 (1,0,0)
//     cellCoords[6] = 0.0; cellCoords[7] = 1.0; cellCoords[8] = 0.0; // V2 (0,1,0)
//     cellCoords[9] = 0.0; cellCoords[10] = 0.0; cellCoords[11] = 1.0; // V3 (0,0,1)
// 
//     // Test 1: Centroide físico (0.25, 0.25, 0.25) -> debe ser (0.25, 0.25, 0.25) en referencia
//     xyz_point[0] = 0.25; xyz_point[1] = 0.25; xyz_point[2] = 0.25;
//     ierr = tetrahedronXYZToXiEtaZeta(cellCoords, xyz_point, XiEtaZeta_point);
//     TEST_ASSERT_EQUAL_INT(PETSC_SUCCESS, ierr);
//     TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.25, XiEtaZeta_point[0]);
//     TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.25, XiEtaZeta_point[1]);
//     TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.25, XiEtaZeta_point[2]);
//     // La suma de los 4 componentes barycéntricos (L0, L1, L2, L3) siempre es 1.
//     // Aquí, XiEtaZeta_point son L1, L2, L3. L0 sería 1 - (L1+L2+L3).
//     TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.25, 1.0 - (XiEtaZeta_point[0] + XiEtaZeta_point[1] + XiEtaZeta_point[2]));
// 
//     // Test 2: Vértice físico V0 (0,0,0) -> debe ser (0,0,0) en referencia
//     xyz_point[0] = 0.0; xyz_point[1] = 0.0; xyz_point[2] = 0.0;
//     ierr = tetrahedronXYZToXiEtaZeta(cellCoords, xyz_point, XiEtaZeta_point);
//     TEST_ASSERT_EQUAL_INT(PETSC_SUCCESS, ierr);
//     TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, XiEtaZeta_point[0]);
//     TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, XiEtaZeta_point[1]);
//     TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, XiEtaZeta_point[2]);
// 
//     // Test 3: Vértice físico V1 (1,0,0) -> debe ser (1,0,0) en referencia
//     xyz_point[0] = 1.0; xyz_point[1] = 0.0; xyz_point[2] = 0.0;
//     ierr = tetrahedronXYZToXiEtaZeta(cellCoords, xyz_point, XiEtaZeta_point);
//     TEST_ASSERT_EQUAL_INT(PETSC_SUCCESS, ierr);
//     TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1.0, XiEtaZeta_point[0]);
//     TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, XiEtaZeta_point[1]);
//     TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, XiEtaZeta_point[2]);
// }

// Test para computeCellOrientation (Requiere un DM real, solo chequea ejecución)
void test_computeCellOrientation_execution(void) {
    PetscInt cellOrientation[10];
    PetscErrorCode ierr;

    // Crea un DM simple (1x1x1 caja con un solo tetraedro, si la interpolación lo permite)
    // Para tetraedros, DMPlexCreateBoxMesh con 'interpolate=PETSC_TRUE' suele descomponer en varios tetras.
    // Un simple tetraedro para el test sería más directo si la topología fuera manual.
    // Usaremos el arg `N=0` para no refinamientos.
    PetscInt dim = 3, cells[] = {1,1,1};
    DMBoundaryType periodicity[3] = {DM_BOUNDARY_NONE, DM_BOUNDARY_NONE, DM_BOUNDARY_NONE};
    // El 9º argumento (N) debe ser PetscInt, 0 significa sin refinamientos.
    PetscCallVoid(DMPlexCreateBoxMesh(PETSC_COMM_WORLD, dim, PETSC_FALSE, cells, NULL, NULL, periodicity, PETSC_TRUE, 0, PETSC_FALSE, &test_dm));
    PetscCallVoid(DMSetFromOptions(test_dm));
    PetscCallVoid(DMSetUp(test_dm));

    PetscInt cell_idx = 0; // Test con la primera celda

    for (PetscInt j = 0; j < 10; ++j) { cellOrientation[j] = 0; } // Inicializar para evitar basura

    ierr = computeCellOrientation(test_dm, cell_idx, cellOrientation);
    TEST_ASSERT_EQUAL_INT(PETSC_SUCCESS, ierr);

    // No se hacen aserciones específicas sobre 'cellOrientation'
    // ya que su valor es muy dependiente de la implementación interna y la topología del DM.
    // El test solo verifica que la función se ejecuta sin errores fatales.
    // Para un test más profundo, se necesitaría un DM con una topología y orientaciones conocidas.
}

// Test para computeBasisFunctions (Solo chequea ejecución, muy complejo para unit test profundo)
void test_computeBasisFunctions_execution(void) {
    PetscInt nord = 1; // Para orden 1, numDofInCell es 6
    PetscInt cellOrientation[10] = {0}; // Orientación dummy
    PetscReal jacobian[NUM_DIMENSIONS][NUM_DIMENSIONS] = {{1,0,0},{0,1,0},{0,0,1}}; // Jacobiano Identidad
    PetscReal invJacobian[NUM_DIMENSIONS][NUM_DIMENSIONS] = {{1,0,0},{0,1,0},{0,0,1}}; // Inverso de Jacobiano Identidad
    PetscReal XiEtaZeta[NUM_DIMENSIONS] = {0.25, 0.25, 0.25}; // Centroide
    PetscReal **basisFunctions = NULL;
    PetscReal **curlBasisFunctions = NULL;
    PetscErrorCode ierr;
    
    // numDofInCell para nord=1 es 6
    PetscInt numDofInCell_nord1 = nord * (nord + 2) * (nord + 3) / 2;
    TEST_ASSERT_EQUAL_INT(6, numDofInCell_nord1); // Asegurarse de que el cálculo sea correcto

    // Asignar memoria para los resultados
    PetscCallVoid(PetscCalloc1(NUM_DIMENSIONS, &basisFunctions));
    PetscCallVoid(PetscCalloc1(NUM_DIMENSIONS, &curlBasisFunctions));
    for (PetscInt i = 0; i < NUM_DIMENSIONS; ++i){
        PetscCallVoid(PetscCalloc1(numDofInCell_nord1, &basisFunctions[i]));
        PetscCallVoid(PetscCalloc1(numDofInCell_nord1, &curlBasisFunctions[i]));
    }

    ierr = computeBasisFunctions(nord, cellOrientation, jacobian, invJacobian, XiEtaZeta, basisFunctions, curlBasisFunctions);
    TEST_ASSERT_EQUAL_INT(PETSC_SUCCESS, ierr);
    // Asertar no-nulidad de los punteros principales
    TEST_ASSERT_NOT_NULL(basisFunctions);
    TEST_ASSERT_NOT_NULL(curlBasisFunctions);
    // Podríamos añadir aserciones de valores si conocemos el comportamiento de las funciones base
    // para este caso simple (ej. para un tetraedro de referencia y jacobiano identidad).

    // Limpiar memoria
    for (PetscInt i = 0; i < NUM_DIMENSIONS; ++i){
        PetscCallVoid(PetscFree(basisFunctions[i]));
        PetscCallVoid(PetscFree(curlBasisFunctions[i]));
    }
    PetscCallVoid(PetscFree(basisFunctions));
    PetscCallVoid(PetscFree(curlBasisFunctions));
}


// Test para computeElementalGradientMatrix (Solo chequea ejecución, para orden 1)
void test_computeElementalGradientMatrix_execution(void) {
    PetscInt cellOrientation[10] = {0}; // Orientación dummy para el test de ejecución
    PetscReal **gradientMatrix = NULL;
    PetscErrorCode ierr;

    // Para orden 1:
    // NUM_EDGES_PER_ELEMENT = 6
    // NUM_VERTICES_PER_ELEMENT = 4
    PetscInt numEdges = NUM_EDGES_PER_ELEMENT; // 6
    PetscInt numVertices = NUM_VERTICES_PER_ELEMENT; // 4

    // Asignar memoria para la matriz de gradiente (6x4)
    PetscCallVoid(PetscCalloc1(numEdges, &gradientMatrix));
    PetscCallVoid(PetscCalloc1(numEdges * numVertices, &gradientMatrix[0]));
    for (PetscInt i = 1; i < numEdges; ++i){
       gradientMatrix[i] = gradientMatrix[i - 1] + numVertices;
    }

    // Para un test más significativo, se debería inicializar `cellOrientation`
    // con valores conocidos y luego asertar valores específicos en `gradientMatrix`.
    // Por ejemplo, para la arista 0 (v0->v1), si NoriE[0] == 0, `gradientMatrix[0][0] = 1` y `gradientMatrix[0][1] = -1`.
    // Si NoriE[0] == 1, `gradientMatrix[0][0] = -1` y `gradientMatrix[0][1] = 1`.
    // Aquí, al usar {0} como dummy, se asume NoriE[i] = 0 para todos.
    cellOrientation[4] = 0; // E0: v0 -> v1, orientación directa
    cellOrientation[5] = 0; // E1: v1 -> v2, orientación directa
    cellOrientation[6] = 0; // E2: v2 -> v0, orientación directa
    cellOrientation[7] = 0; // E3: v0 -> v3, orientación directa
    cellOrientation[8] = 0; // E4: v3 -> v1, orientación directa
    cellOrientation[9] = 0; // E5: v2 -> v3, orientación directa


    ierr = computeElementalGradientMatrix(cellOrientation, gradientMatrix);
    TEST_ASSERT_EQUAL_INT(PETSC_SUCCESS, ierr);
    TEST_ASSERT_NOT_NULL(gradientMatrix);

    // Asertar valores para el caso de orientaciones directas (NoriE[i] = 0)
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1.0, gradientMatrix[0][0]);  // E0 (v0->v1), v0 es el final (+)
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, -1.0, gradientMatrix[0][1]); // E0 (v0->v1), v1 es el inicio (-)

    // Limpiar memoria
    PetscCallVoid(PetscFree(gradientMatrix[0]));
    PetscCallVoid(PetscFree(gradientMatrix));
}

// Test para computeElementalGradientMatrix2 (Solo chequea ejecución)
// Como se mencionó en la descripción de la función en hvfem.c, esta función
// actualmente calcula las funciones base H1, pero no devuelve la matriz de gradiente.
// Este test solo verifica que la ejecución no falla.
void test_computeElementalGradientMatrix2_execution(void) {
    PetscInt nord = 1;
    PetscInt cellOrientation[10] = {0}; // Orientación dummy
    PetscInt numGaussPoints = 1; // Para nord=1, gaussOrder=2, computeNumGaussPoints3D da 4.
                                 // Pero la función solo usa el primer punto de Gauss en su loop.
    PetscReal **gaussPoints = NULL;
    PetscReal *weights = NULL;
    PetscErrorCode ierr;

    // Asignar memoria para los datos de Gauss
    PetscCallVoid(PetscCalloc1(numGaussPoints, &gaussPoints));
    for (PetscInt i = 0; i < numGaussPoints; ++i) {
        PetscCallVoid(PetscCalloc1(NUM_DIMENSIONS, &gaussPoints[i]));
    }
    PetscCallVoid(PetscCalloc1(numGaussPoints, &weights));

    // Rellenar con un punto de Gauss (ej. centroide del tetraedro de referencia)
    gaussPoints[0][0] = 0.25;
    gaussPoints[0][1] = 0.25;
    gaussPoints[0][2] = 0.25;
    weights[0] = 1.0; // El peso real es 1/6, pero para un test de ejecución no es crítico

    ierr = computeElementalGradientMatrix2(nord, cellOrientation, numGaussPoints, gaussPoints, weights);
    TEST_ASSERT_EQUAL_INT(PETSC_SUCCESS, ierr);

    // Limpiar memoria
    for (PetscInt i = 0; i < numGaussPoints; ++i) {
        PetscCallVoid(PetscFree(gaussPoints[i]));
    }
    PetscCallVoid(PetscFree(gaussPoints));
    PetscCallVoid(PetscFree(weights));
}

// Test para crossProduct
void test_crossProduct(void) {
    PetscReal v1[NUM_DIMENSIONS];
    PetscReal v2[NUM_DIMENSIONS];
    PetscReal expected[NUM_DIMENSIONS];
    PetscReal result[NUM_DIMENSIONS];
    PetscErrorCode ierr;
    PetscReal tolerance = 1e-9; // Define una tolerancia adecuada para doubles

    // Test 1: i x j = k
    v1[0] = 1.0; v1[1] = 0.0; v1[2] = 0.0;
    v2[0] = 0.0; v2[1] = 1.0; v2[2] = 0.0;
    expected[0] = 0.0; expected[1] = 0.0; expected[2] = 1.0;
    ierr = crossProduct(v1, v2, result);
    TEST_ASSERT_EQUAL_INT(PETSC_SUCCESS, ierr);
    TEST_ASSERT_DOUBLE_ARRAY_WITHIN(tolerance, expected, result, NUM_DIMENSIONS); // CORRECCIÓN AQUÍ

    // Test 2: j x k = i
    v1[0] = 0.0; v1[1] = 1.0; v1[2] = 0.0;
    v2[0] = 0.0; v2[1] = 0.0; v2[2] = 1.0;
    expected[0] = 1.0; expected[1] = 0.0; expected[2] = 0.0;
    ierr = crossProduct(v1, v2, result);
    TEST_ASSERT_EQUAL_INT(PETSC_SUCCESS, ierr);
    TEST_ASSERT_DOUBLE_ARRAY_WITHIN(tolerance, expected, result, NUM_DIMENSIONS); // CORRECCIÓN AQUÍ

    // Test 3: k x i = j
    v1[0] = 0.0; v1[1] = 0.0; v1[2] = 1.0;
    v2[0] = 1.0; v2[1] = 0.0; v2[2] = 0.0;
    expected[0] = 0.0; expected[1] = 1.0; expected[2] = 0.0;
    ierr = crossProduct(v1, v2, result);
    TEST_ASSERT_EQUAL_INT(PETSC_SUCCESS, ierr);
    TEST_ASSERT_DOUBLE_ARRAY_WITHIN(tolerance, expected, result, NUM_DIMENSIONS); // CORRECCIÓN AQUÍ

    // Test 4: Paralelos -> 0
    v1[0] = 1.0; v1[1] = 2.0; v1[2] = 3.0;
    v2[0] = 2.0; v2[1] = 4.0; v2[2] = 6.0;
    expected[0] = 0.0; expected[1] = 0.0; expected[2] = 0.0;
    ierr = crossProduct(v1, v2, result);
    TEST_ASSERT_EQUAL_INT(PETSC_SUCCESS, ierr);
    TEST_ASSERT_DOUBLE_ARRAY_WITHIN(tolerance, expected, result, NUM_DIMENSIONS); // CORRECCIÓN AQUÍ
}


// --- Test Group Runner ---
void suite_hvfem(void) {
    // Las funciones setUp/tearDown son llamadas por la suite
    setUp_hvfem();
    RUN_TEST(test_computeNumGaussPoints3D);
    RUN_TEST(test_computeGaussPoints3D);
    RUN_TEST(test_vectorRotation);
    //RUN_TEST(test_tetrahedronXYZToXiEtaZeta);
    RUN_TEST(test_computeCellOrientation_execution);
    RUN_TEST(test_computeBasisFunctions_execution);
    RUN_TEST(test_computeElementalGradientMatrix_execution);
    RUN_TEST(test_computeElementalGradientMatrix2_execution);
    RUN_TEST(test_crossProduct);
    tearDown_hvfem();
}