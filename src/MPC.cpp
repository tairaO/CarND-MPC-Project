#include "MPC.h"
#include <cppad/cppad.hpp>
#include <cppad/ipopt/solve.hpp>
#include "Eigen-3.3/Eigen/Core"

using CppAD::AD;

// TODO: Set the timestep length and duration
size_t N = 14; // how long you should weight the future. previous: 10, 12, 15, 20
double dt = 0.1; //the time step controller input 

// This value assumes the model presented in the classroom is used.
//
// It was obtained by measuring the radius formed by running the vehicle in the
// simulator around in a circle with a constant steering angle and velocity on a
// flat terrain.
//
// Lf was tuned until the the radius formed by the simulating the model
// presented in the classroom matched the previous radius.
//
// This is the length from front to CoG that has a similar radius.
const double Lf = 2.67;
size_t x_start = 0;
size_t y_start = x_start + N;
size_t psi_start = y_start + N;
size_t v_start = psi_start + N;
size_t cte_start = v_start + N;
size_t epsi_start = cte_start + N;
size_t delta_start = epsi_start + N;
size_t a_start = delta_start + N - 1;


class FG_eval {
 public:
  // Fitted polynomial coefficients
  Eigen::VectorXd coeffs;
  Eigen::VectorXd weights;
  CppAD::AD<double> v_target;
  FG_eval(Eigen::VectorXd coeffs, Eigen::VectorXd weights, CppAD::AD<double> v_target) { 
    this->coeffs   = coeffs; 
    this->weights  = weights;
    this->v_target = v_target;
  }

  typedef CPPAD_TESTVECTOR(AD<double>) ADvector;
  void operator()(ADvector& fg, const ADvector& vars) {
    // TODO: implement MPC
    // `fg` a vector of the cost constraints, `vars` is a vector of variable values (state & actuators)
    // NOTE: You'll probably go back and forth between this function and
    // the Solver function below.

    // Cost function
    fg[0] = 0;
    
    // Minimize the cross-track-error, direction error and the speed error
    for (unsigned int t = 0; t < N; t++) {
      fg[0] += weights(0) * CppAD::pow(vars[cte_start + t], 2);
      fg[0] += weights(1) * CppAD::pow(vars[epsi_start + t], 2);
      fg[0] += weights(2) * CppAD::pow(vars[v_start + t] - v_target, 2);
    }
    // Minimize the use of actuators.
    for (unsigned int t = 0; t < N - 1; t++) {
      fg[0] += weights(3) * CppAD::pow(vars[delta_start + t], 2);
      fg[0] += weights(4) * CppAD::pow(vars[a_start + t], 2);
    }
    // Minimize the value gap between sequential actuations.
    for (unsigned int t = 0; t < N - 2; t++) {
      fg[0] += weights(5) * CppAD::pow(vars[delta_start + t + 1] - vars[delta_start + t], 2);
      fg[0] += weights(6) * CppAD::pow(vars[a_start + t + 1] - vars[a_start + t], 2);
    }
    

    // Constraints
    fg[1 + x_start]    = vars[x_start];
    fg[1 + y_start]    = vars[y_start];
    fg[1 + psi_start]  = vars[psi_start];
    fg[1 + v_start]    = vars[v_start];
    fg[1 + cte_start]  = vars[cte_start];
    fg[1 + epsi_start] = vars[epsi_start];

    // The rest of the constraints
    for (unsigned int t = 1; t < N; t++) {
      AD<double> x1     = vars[x_start    + t];
      AD<double> y1     = vars[y_start    + t];
      AD<double> psi1   = vars[psi_start  + t];
      AD<double> v1     = vars[v_start    + t];
      AD<double> cte1   = vars[cte_start  + t];
      AD<double> epsi1  = vars[epsi_start + t];
      
      AD<double> x0     = vars[x_start     + t - 1];
      AD<double> y0     = vars[y_start     + t - 1];
      AD<double> psi0   = vars[psi_start   + t - 1];
      AD<double> v0     = vars[v_start     + t - 1];
      AD<double> cte0   = vars[cte_start   + t - 1];
      AD<double> epsi0  = vars[epsi_start  + t - 1];
      AD<double> delta0 = vars[delta_start + t - 1];
      AD<double> a0     = vars[a_start     + t - 1];
      
      AD<double> f0 = coeffs[0] + coeffs[1]*x0 + coeffs[2]*CppAD::pow(x0, 2) + coeffs[3]*CppAD::pow(x0, 3); // the reference trajectory
      AD<double> psides0 = CppAD::atan(coeffs[1] + 2.0*coeffs[2]*x0 + 3.0*coeffs[3]*CppAD::pow(x0, 2)); // the slope of the reference trajectory

      fg[1 + x_start    + t] = x1    - (x0             + v0*CppAD::cos(psi0)*dt);
      fg[1 + y_start    + t] = y1    - (y0             + v0*CppAD::sin(psi0)*dt);
      fg[1 + psi_start  + t] = psi1  - (psi0           + v0/Lf*delta0*dt);
      fg[1 + v_start    + t] = v1    - (v0             + a0*dt);
      fg[1 + cte_start  + t] = cte1  - (f0 - y0        + v0*CppAD::sin(epsi0)*dt);
      fg[1 + epsi_start + t] = epsi1 - (psi0 - psides0 + v0/Lf*delta0*dt);
    }
  }
};

//
// MPC class definition implementation.
//
MPC::MPC() {}
MPC::~MPC() {}

vector<vector<double>> MPC::Solve(Eigen::VectorXd state, Eigen::VectorXd coeffs, Eigen::VectorXd weights, AD<double> v_target) {
  bool ok = true;
  typedef CPPAD_TESTVECTOR(double) Dvector;

  double x    = state[0];
  double y    = state[1];
  double psi  = state[2];
  double v    = state[3];
  double cte  = state[4];
  double epsi = state[5];

  // TODO: Set the number of model variables (includes both states and inputs).
  // For example: If the state is a 4 element vector, the actuators is a 2
  // element vector and there are 10 timesteps. The number of variables is:
  //
  // 4 * 10 + 2 * 9
  size_t n_vars = 6*N + 2*(N-1); // 6 state variables and 2 controls
  // TODO: Set the number of constraints
  size_t n_constraints = 6*N;

  // Initial value of the independent variables.
  // SHOULD BE 0 besides initial state
  Dvector vars(n_vars);
  for (unsigned int i = 0; i < n_vars; i++) {
    vars[i] = 0;
  }
  
  // Set the initial variable values
  vars[x_start]    = x;
  vars[y_start]    = y;
  vars[psi_start]  = psi;
  vars[v_start]    = v;
  vars[cte_start]  = cte;
  vars[epsi_start] = epsi;

  Dvector vars_lowerbound(n_vars);
  Dvector vars_upperbound(n_vars);
  // TODO: Set lower and upper limits for variables.
  // bounds of state
  for (unsigned int i = 0; i < delta_start; i++) {
    vars_lowerbound[i] = -1.0e19;
    vars_upperbound[i] =  1.0e19;
  }

  // bounds of steering
  for (unsigned int i = delta_start; i < a_start; i++) {
    vars_lowerbound[i] = -0.436332;
    vars_upperbound[i] =  0.436332;
  }

  // bounds of acceleration
  for (unsigned int i = a_start; i < n_vars; i++) {
    vars_lowerbound[i] = -1.0;
    vars_upperbound[i] =  1.0;
  }


  // Lower and upper limits for the constraints
  // Should be 0 besides initial state.
  Dvector constraints_lowerbound(n_constraints);
  Dvector constraints_upperbound(n_constraints);
  for (unsigned int i = 0; i < n_constraints; i++) {
    constraints_lowerbound[i] = 0;
    constraints_upperbound[i] = 0;
  }
  
  constraints_lowerbound[x_start]    = x;
  constraints_lowerbound[y_start]    = y;
  constraints_lowerbound[psi_start]  = psi;
  constraints_lowerbound[v_start]    = v;
  constraints_lowerbound[cte_start]  = cte;
  constraints_lowerbound[epsi_start] = epsi;

  constraints_upperbound[x_start]    = x;
  constraints_upperbound[y_start]    = y;
  constraints_upperbound[psi_start]  = psi;
  constraints_upperbound[v_start]    = v;
  constraints_upperbound[cte_start]  = cte;
  constraints_upperbound[epsi_start] = epsi;
  
  
  // object that computes objective and constraints
  FG_eval fg_eval(coeffs, weights, v_target);

  //
  // NOTE: You don't have to worry about these options
  //
  // options for IPOPT solver
  std::string options;
  // Uncomment this if you'd like more print information
  options += "Integer print_level  0\n";
  // NOTE: Setting sparse to true allows the solver to take advantage
  // of sparse routines, this makes the computation MUCH FASTER. If you
  // can uncomment 1 of these and see if it makes a difference or not but
  // if you uncomment both the computation time should go up in orders of
  // magnitude.
  options += "Sparse  true        forward\n";
  options += "Sparse  true        reverse\n";
  // NOTE: Currently the solver has a maximum time limit of 0.5 seconds.
  // Change this as you see fit.
  options += "Numeric max_cpu_time          0.5\n";

  // place to return solution
  CppAD::ipopt::solve_result<Dvector> solution;

  // solve the problem
  CppAD::ipopt::solve<Dvector, FG_eval>(
      options, vars, vars_lowerbound, vars_upperbound, constraints_lowerbound,
      constraints_upperbound, fg_eval, solution);

  // Check some of the solution values
  ok &= solution.status == CppAD::ipopt::solve_result<Dvector>::success;

  // Cost
  auto cost = solution.obj_value;

  // Print the detail of the cost
  AD<double> cost_cte    = 0;
  AD<double> cost_epsi   = 0;
  AD<double> cost_v      = 0;	
  AD<double> cost_delta  = 0;
  AD<double> cost_a      = 0;
  AD<double> cost_ddelta = 0;
  AD<double> cost_da     = 0;

  for (unsigned int t = 0; t < N; t++) {
      cost_cte    += weights(0) * CppAD::pow(solution.x[cte_start + t], 2);
      cost_epsi   += weights(1) * CppAD::pow(solution.x[epsi_start + t], 2);
      cost_v      += weights(2) * CppAD::pow(solution.x[v_start + t] - v_target, 2);
    }
  for (unsigned int t = 0; t < N - 1; t++) {
      cost_delta  += weights(3) * CppAD::pow(solution.x[delta_start + t], 2);
      cost_a      += weights(4) * CppAD::pow(solution.x[a_start + t], 2);
    }
  for (unsigned int t = 0; t < N - 2; t++) {
      cost_ddelta += weights(5) * CppAD::pow(solution.x[delta_start + t + 1] - solution.x[delta_start + t], 2);
      cost_da     += weights(6) * CppAD::pow(solution.x[a_start + t + 1]     - solution.x[a_start + t], 2);
    }
  std::cout << "Total cost             : " << cost        << std::endl;
  std::cout << "CTE cost               : " << cost_cte    << std::endl;
  std::cout << "epsi cost              : " << cost_epsi   << std::endl;
  std::cout << "Speed cost             : " << cost_v      << std::endl;
  std::cout << "Steering cost          : " << cost_delta  << std::endl;
  std::cout << "Throttle cost          : " << cost_a      << std::endl;
  std::cout << "Change of steering cost: " << cost_ddelta << std::endl;
  std::cout << "Change of throttle cost: " << cost_da     << std::endl;  

  // TODO: Return the first actuator values. The variables can be accessed with
  // `solution.x[i]`.
  //
  // {...} is shorthand for creating a vector, so auto x1 = {1.0,2.0}
  // creates a 2 element double vector.
  std::vector<double> mpc_x;
  std::vector<double> mpc_y;
  for(unsigned int i=0; i<N;i++){
    mpc_x.push_back(solution.x[x_start+i]);
    mpc_y.push_back(solution.x[y_start+i]);
  }
  return {{solution.x[delta_start], solution.x[a_start]}, mpc_x, mpc_y};
}
