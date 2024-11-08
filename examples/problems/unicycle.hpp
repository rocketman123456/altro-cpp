// Copyright [2021] Optimus Ride Inc.

#pragma once

#include <fmt/format.h>
#include <fmt/ostream.h>

#include "altro/augmented_lagrangian/al_problem.hpp"
#include "altro/augmented_lagrangian/al_solver.hpp"
#include "altro/common/trajectory.hpp"
#include "altro/eigentypes.hpp"
#include "altro/ilqr/ilqr.hpp"
#include "altro/problem/discretized_model.hpp"
#include "altro/problem/problem.hpp"
#include "examples/basic_constraints.hpp"
#include "examples/obstacle_constraints.hpp"
#include "examples/quadratic_cost.hpp"
#include "examples/unicycle.hpp"

namespace altro
{
    namespace problems
    {
        class UnicycleProblem
        {
        public:
            static constexpr int NStates   = 3;
            static constexpr int NControls = 2;

            UnicycleProblem();

            enum Scenario
            {
                kTurn90,
                kThreeObstacles
            };

            using ModelType   = altro::problem::DiscretizedModel<altro::examples::Unicycle>;
            using CostFunType = altro::examples::QuadraticCost;

            // Problem Data
            static constexpr int HEAP = Eigen::Dynamic;
            const int            n    = NStates;
            const int            m    = NControls;

            int       N     = 100;
            ModelType model = ModelType(altro::examples::Unicycle());

            Eigen::Matrix3d Q    = Eigen::Vector3d::Constant(NStates, 1e-2).asDiagonal();
            Eigen::Matrix2d R    = Eigen::Vector2d::Constant(NControls, 1e-2).asDiagonal();
            Eigen::Matrix3d Qf   = Eigen::Vector3d::Constant(NStates, 100).asDiagonal();
            Eigen::Vector3d xf   = Eigen::Vector3d(1.5, 1.5, M_PI / 2);
            Eigen::Vector3d x0   = Eigen::Vector3d(0, 0, 0);
            Eigen::Vector2d u0   = Eigen::Vector2d::Constant(NControls, 0.1);
            Eigen::Vector2d uref = Eigen::Vector2d::Zero();

            std::shared_ptr<examples::QuadraticCost> qcost;
            std::shared_ptr<examples::QuadraticCost> qterm;

            double                            v_bnd = 1.5; // linear velocity bound
            double                            w_bnd = 1.5; // angular velocity bound
            Eigen::VectorXd                   cx;          // x-coordinates of obstacles
            Eigen::VectorXd                   cy;          // y-coordinates of obstacles
            Eigen::VectorXd                   cr;          // radii of obstacles
            std::vector<double>               lb;
            std::vector<double>               ub;
            altro::examples::CircleConstraint obstacles;

            altro::problem::Problem MakeProblem(const bool add_constraints = true);

            template<int n_size = NStates, int m_size = NControls>
            altro::Trajectory<n_size, m_size> InitialTrajectory();

            template<int n_size = NStates, int m_size = NControls>
            altro::ilqr::iLQR<n_size, m_size> MakeSolver(const bool alcost = false);

            template<int n_size = NStates, int m_size = NControls>
            altro::augmented_lagrangian::AugmentedLagrangianiLQR<n_size, m_size> MakeALSolver();

            void SetScenario(Scenario scenario) { scenario_ = scenario; }

            float GetTimeStep() const { return tf / N; }

        private:
            bool  scenario_ = kTurn90;
            float tf        = 3.0;
        };

        template<int n_size, int m_size>
        altro::Trajectory<n_size, m_size> UnicycleProblem::InitialTrajectory()
        {
            altro::Trajectory<n_size, m_size> Z(n, m, N);
            for (int k = 0; k < N; ++k)
            {
                Z.Control(k) = u0;
            }
            float h = GetTimeStep();
            Z.SetUniformStep(h);
            return Z;
        }

        template<int n_size, int m_size>
        altro::ilqr::iLQR<n_size, m_size> UnicycleProblem::MakeSolver(const bool alcost)
        {
            altro::problem::Problem prob = MakeProblem();
            if (alcost)
            {
                prob = altro::augmented_lagrangian::BuildAugLagProblem<n_size, m_size>(prob);
            }
            altro::ilqr::iLQR<n_size, m_size> solver(prob);

            std::shared_ptr<altro::Trajectory<n_size, m_size>> traj_ptr =
                std::make_shared<altro::Trajectory<n_size, m_size>>(InitialTrajectory<n_size, m_size>());

            solver.SetTrajectory(traj_ptr);
            solver.Rollout();
            return solver;
        }

        template<int n_size, int m_size>
        altro::augmented_lagrangian::AugmentedLagrangianiLQR<n_size, m_size> UnicycleProblem::MakeALSolver()
        {
            altro::problem::Problem                                              prob = MakeProblem(true);
            altro::augmented_lagrangian::AugmentedLagrangianiLQR<n_size, m_size> solver_al(prob);
            solver_al.SetTrajectory(std::make_shared<altro::Trajectory<NStates, NControls>>(InitialTrajectory()));
            solver_al.GetiLQRSolver().Rollout();

            return solver_al;
        }

    } // namespace problems
} // namespace altro