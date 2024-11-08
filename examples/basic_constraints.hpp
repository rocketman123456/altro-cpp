// Copyright [2021] Optimus Ride Inc.

#pragma once

#include <limits>
#include <vector>

#include "altro/constraints/constraint.hpp"
#include "altro/eigentypes.hpp"
#include "altro/utils/utils.hpp"

namespace altro
{
    namespace examples
    {

        class GoalConstraint : public constraints::Constraint<constraints::Equality>
        {
        public:
            explicit GoalConstraint(const VectorXd& xf)
                : xf_(xf)
            {}

            static constraints::ConstraintPtr<constraints::Equality> Create(const VectorXd& xf) { return std::make_shared<GoalConstraint>(xf); }

            std::string GetLabel() const override { return "Goal Constraint"; }
            int         StateDimension() const override { return xf_.size(); }
            int         OutputDimension() const override { return xf_.size(); }

            void Evaluate(const VectorXdRef& x, const VectorXdRef& u, Eigen::Ref<VectorXd> c) override
            {
                ALTRO_UNUSED(u);
                c = x - xf_;
            }
            void Jacobian(const VectorXdRef& x, const VectorXdRef& u, Eigen::Ref<MatrixXd> jac) override
            {
                ALTRO_UNUSED(x);
                ALTRO_UNUSED(u);
                jac.setIdentity();
            }

        private:
            VectorXd xf_;
        };

        class ControlBound : public constraints::Constraint<constraints::NegativeOrthant>
        {
        public:
            explicit ControlBound(const int m)
                : m_(m)
                , lower_bound_(m, -std::numeric_limits<double>::infinity())
                , upper_bound_(m, +std::numeric_limits<double>::infinity())
            {}

            ControlBound(const std::vector<double>& lb, const std::vector<double>& ub)
                : m_(lb.size())
                , lower_bound_(lb)
                , upper_bound_(ub)
            {
                ALTRO_ASSERT(lb.size() == ub.size(), "Upper and lower bounds must have the same length.");
                ALTRO_ASSERT(lb.size() > 0, "Cannot pass in empty bounds.");
                GetFiniteIndices(upper_bound_, &index_upper_bound_);
                GetFiniteIndices(lower_bound_, &index_lower_bound_);
                ValidateBounds();
            }

            void SetUpperBound(const std::vector<double>& ub)
            {
                ALTRO_ASSERT(ub.size() == static_cast<size_t>(m_), "Inconsistent control dimension when setting upper bound.");
                upper_bound_ = ub;
                GetFiniteIndices(upper_bound_, &index_upper_bound_);
                ValidateBounds();
            }

            void SetUpperBound(std::vector<double>&& ub)
            {
                ALTRO_ASSERT(ub.size() == static_cast<size_t>(m_), "Inconsistent control dimension when setting upper bound.");
                upper_bound_ = std::move(ub);
                GetFiniteIndices(upper_bound_, &index_upper_bound_);
                ValidateBounds();
            }

            void SetLowerBound(const std::vector<double>& lb)
            {
                ALTRO_ASSERT(lb.size() == static_cast<size_t>(m_), "Inconsistent control dimension when setting lower bound.");
                lower_bound_ = lb;
                GetFiniteIndices(lower_bound_, &index_lower_bound_);
                ValidateBounds();
            }

            void SetLowerBound(std::vector<double>&& lb)
            {
                ALTRO_ASSERT(lb.size() == static_cast<size_t>(m_), "Inconsistent control dimension when setting lower bound.");
                lower_bound_ = std::move(lb);
                GetFiniteIndices(lower_bound_, &index_lower_bound_);
                ValidateBounds();
            }

            std::string GetLabel() const override { return "Control Bound"; }

            int ControlDimension() const override { return m_; }

            int OutputDimension() const override { return index_lower_bound_.size() + index_upper_bound_.size(); }

            void Evaluate(const VectorXdRef& /*x*/, const VectorXdRef& u, Eigen::Ref<VectorXd> c) override
            {
                ALTRO_ASSERT(u.size() == m_, "Inconsistent control dimension when evaluating control bound.");

                for (size_t i = 0; i < index_lower_bound_.size(); ++i)
                {
                    size_t j = index_lower_bound_[i];
                    c(i)     = lower_bound_.at(j) - u(j);
                }
                int offset = index_lower_bound_.size();
                for (size_t i = 0; i < index_upper_bound_.size(); ++i)
                {
                    size_t j      = index_upper_bound_[i];
                    c(i + offset) = u(j) - upper_bound_.at(j);
                }
            }

            void Jacobian(const VectorXdRef& x, const VectorXdRef& u, Eigen::Ref<MatrixXd> jac) override
            {
                (void)u; // surpress erroneous unused variable error
                ALTRO_ASSERT(u.size() == m_, "Inconsistent control dimension when evaluating control bound.");
                jac.setZero();

                int n = x.size(); // state dimension
                for (size_t i = 0; i < index_lower_bound_.size(); ++i)
                {
                    size_t j      = index_lower_bound_[i];
                    jac(i, n + j) = -1;
                }
                int offset = index_lower_bound_.size();
                for (size_t i = 0; i < index_upper_bound_.size(); ++i)
                {
                    size_t j               = index_upper_bound_[i];
                    jac(i + offset, n + j) = 1;
                }
            }

        private:
            void ValidateBounds()
            {
                for (int i = 0; i < m_; ++i)
                {
                    ALTRO_ASSERT(lower_bound_[i] <= upper_bound_[i], "Lower bound isn't less than the upper bound.");
                }
            }
            static void GetFiniteIndices(const std::vector<double>& bound, std::vector<size_t>* index)
            {
                index->clear();
                for (size_t i = 0; i < bound.size(); ++i)
                {
                    if (std::abs(bound[i]) < std::numeric_limits<double>::max())
                    {
                        index->emplace_back(i);
                    }
                }
            }
            int                 m_;
            std::vector<double> lower_bound_;
            std::vector<double> upper_bound_;
            std::vector<size_t> index_lower_bound_;
            std::vector<size_t> index_upper_bound_;
        };

    } // namespace examples
} // namespace altro