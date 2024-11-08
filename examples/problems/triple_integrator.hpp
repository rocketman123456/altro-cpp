// Copyright [2021] Optimus Ride Inc.

#pragma once

#include <fmt/format.h>
#include <fmt/ostream.h>

#include "altro/common/trajectory.hpp"
#include "altro/eigentypes.hpp"
#include "altro/problem/discretized_model.hpp"
#include "altro/problem/problem.hpp"
#include "examples/basic_constraints.hpp"
#include "examples/quadratic_cost.hpp"
#include "examples/triple_integrator.hpp"

namespace altro
{
    namespace problems
    {
        using CostFunType = altro::examples::QuadraticCost;
        using ModelType   = altro::examples::TripleIntegrator;

        template<int dof = 2>
        class TripleIntegratorProblem
        {
        public:
            static constexpr int NStates   = 3 * dof;
            static constexpr int NControls = dof;

            int                 N    = 10;
            float               h    = 0.1;
            Eigen::MatrixXd     Q    = Eigen::VectorXd::Constant(NStates, 1.0).asDiagonal();
            Eigen::MatrixXd     R    = Eigen::VectorXd::Constant(NControls, 0.001).asDiagonal();
            Eigen::MatrixXd     Qf   = Eigen::VectorXd::Constant(NStates, 1e5).asDiagonal();
            Eigen::VectorXd     xf   = Eigen::VectorXd::Zero(NStates);
            Eigen::VectorXd     x0   = Eigen::VectorXd::Zero(NStates);
            std::vector<double> ubnd = std::vector<double>(dof);

            TripleIntegratorProblem()
            {
                for (int i = 0; i < dof; ++i)
                {
                    xf(i)   = i + 1;
                    x0(i)   = -(i + 1);
                    ubnd[i] = 100 * (i + 1);
                }
            }

            template<class Integrator = altro::problem::RungeKutta4<NStates, NControls>>
            altro::problem::Problem MakeProblem(const bool add_constraints = false)
            {
                altro::problem::Problem prob(N);

                // Cost Function
                Eigen::VectorXd              xref = xf;
                Eigen::VectorXd              uref(m);
                const bool                   is_term = true;
                std::shared_ptr<CostFunType> qterm   = std::make_shared<CostFunType>(CostFunType::LQRCost(Qf, R * 0, xref, uref, is_term));
                for (int k = 0; k < N; ++k)
                {
                    std::shared_ptr<CostFunType> qcost = std::make_shared<CostFunType>(CostFunType::LQRCost(Q, R, xref, uref));
                    prob.SetCostFunction(qcost, k);
                }
                prob.SetCostFunction(qterm, N);

                // Dynamics
                using DModelType = altro::problem::DiscretizedModel<ModelType, Integrator>;
                ModelType  model_continuous(dof);
                DModelType model = DModelType(model_continuous);
                for (int k = 0; k < N; ++k)
                {
                    prob.SetDynamics(std::make_shared<DModelType>(model), k);
                }

                // Initial State
                prob.SetInitialState(x0);

                // Constraints
                if (add_constraints)
                {
                    std::vector<double> lb;
                    std::vector<double> ub;
                    for (int i = 0; i < dof; ++i)
                    {
                        lb.emplace_back(-ubnd[i]);
                        ub.emplace_back(+ubnd[i]);
                    }
                    for (int k = 0; k < N; ++k)
                    {
                        altro::constraints::ConstraintPtr<altro::constraints::Inequality> bnd = std::make_shared<altro::examples::ControlBound>(lb, ub);
                        prob.SetConstraint(bnd, k);
                    }

                    altro::constraints::ConstraintPtr<altro::constraints::Equality> goal = std::make_shared<altro::examples::GoalConstraint>(xf);
                    prob.SetConstraint(goal, N);
                }

                return prob;
            }

            template<int n_size = NStates, int m_size = NControls>
            altro::Trajectory<n_size, m_size> InitialTrajectory()
            {
                altro::Trajectory<n_size, m_size> Z(n, m, N);
                Z.SetUniformStep(h);
                return Z;
            }

        private:
            int n = NStates;
            int m = NControls;
        };

    } // namespace problems
} // namespace altro