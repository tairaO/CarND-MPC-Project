#ifndef MPC_H
#define MPC_H

#include <vector>
#include <cppad/cppad.hpp>
#include <cppad/ipopt/solve.hpp>
#include "Eigen-3.3/Eigen/Core"


using namespace std;

class MPC {
 public:
  MPC();

  virtual ~MPC();

  // Solve the model given an initial state and polynomial coefficients.
  // Return the first actuatotions.
  vector<vector<double>> Solve(Eigen::VectorXd state, Eigen::VectorXd coeffs, Eigen::VectorXd weights, CppAD::AD<double> v_target);
};

#endif /* MPC_H */