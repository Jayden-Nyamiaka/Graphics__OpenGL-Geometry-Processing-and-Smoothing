#include <math.h>
#define _USE_MATH_DEFINES

/* Standard libraries that are just generally useful. */
#include <iostream>
#include <vector>

/* Libraries used for file parsing */
#include <fstream>
#include <sstream>

/* Map library used to store objects by name */
#include <map>

/* Eigen Library included for ArcBall */
#include <Eigen/Dense>
#include <Eigen/Sparse>
using Eigen::Vector3f;
using Eigen::Matrix4f;

/* Local libraries for half edge */
#include "structs.h"
#include "halfedge.h"

#include <stdlib.h>
srand(time(NULL));

using namespace Eigen;

int dim = 5;
int non_zero = 2;

SparseMatrix<float> op_matrix(dim, dim);
op_matrix.reserve( VectorXi::Constant(dim, 2) );

cerr << "op_matrix initialzed: \n" << op_matrix << endl << endl;


for (int i = 0; i < dim; i++) {
  for (int n = 0; n < non_zero; n++) {
    int j = rand() % dim;
    op_matrix.insert(i, j) = rand % 12 + 5;
  }
}

cerr << "op_matrix filled: \n" << op_matrix << endl << endl;


MatrixXf iden = MatrixXf::Identity(dim, dim);
cerr << "identity initialzed: \n" << op_matrix << endl << endl;


SparseMatrix<float> opF(dim, dim);
opF.reserve( Eigen::VectorXi::Constant(dim, non_zero) );

cerr << "opF initialzed: \n" << opF << endl << endl;

SparseMatrix<float> idenSparse(dim, dim);
idenSparse = iden.sparseView();
cerr << "idenSparse intialized: \n" << idenSparse << endl << endl;


opF = iden.sparseView(idenSparse) - (0.5 * op_matrix);
cerr << "opF evaluated: \n" << opF << endl << endl;




