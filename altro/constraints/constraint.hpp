// Copyright [2021] Optimus Ride Inc.

#pragma once

#include <iostream>
#include <memory>
#include <string>

#include "altro/common/functionbase.hpp"
#include "altro/eigentypes.hpp"
#include "altro/utils/utils.hpp"

namespace altro
{
    namespace constraints
    {

        // Forward-declare for use in ZeroCone
        class IdentityCone;

        /**
         * @brief An Equality constraint (alias for ZeroCone)
         *
         * Generic equality constraint of the form
         * \f[ g(x,u) = 0 \f]
         *
         * The projection operation for equality constraints of this form projects the
         * value(s) to zero. The dual cone is the identity map.
         */
        class ZeroCone
        {
        public:
            ZeroCone()     = delete;
            using DualCone = IdentityCone;

            static void Projection(const VectorXdRef& x, Eigen::Ref<VectorXd> x_proj)
            {
                ALTRO_ASSERT(x.size() == x_proj.size(), "x and x_proj must be the same size");
                ALTRO_UNUSED(x);
                x_proj.setZero();
            }
            static void Jacobian(const VectorXdRef& x, Eigen::Ref<MatrixXd> jac)
            {
                ALTRO_UNUSED(x);
                jac.setZero();
            }
            static void Hessian(const VectorXdRef& x, const VectorXdRef& b, Eigen::Ref<MatrixXd> hess)
            {
                ALTRO_ASSERT(hess.rows() == hess.cols(), "Hessian must be square.");
                ALTRO_ASSERT(x.size() == b.size(), "x and b must be the same size.");
                ALTRO_UNUSED(x);
                ALTRO_UNUSED(b);
                hess.setZero();
            }
        };

        /**
         * @brief An alias for the `ZeroCone` cone.
         *
         */
        using Equality = ZeroCone;

        /**
         * @brief The Identity projection
         *
         * The identity projection projects a point onto itself. It is the dual cone
         * for equality constraints, and is used in conic augmented Lagrangian to
         * handle the equality constraints.
         *
         */
        class IdentityCone
        {
        public:
            IdentityCone() = delete;
            using DualCone = ZeroCone;

            static void Projection(const VectorXdRef& x, Eigen::Ref<VectorXd> x_proj)
            {
                ALTRO_ASSERT(x.size() == x_proj.size(), "x and x_proj must be the same size");
                x_proj = x;
            }
            static void Jacobian(const VectorXdRef& x, Eigen::Ref<MatrixXd> jac)
            {
                ALTRO_ASSERT(jac.rows() == jac.cols(), "Jacobian must be square.");
                ALTRO_UNUSED(x);
                jac.setIdentity();
            }
            static void Hessian(const VectorXdRef& x, const VectorXdRef& b, Eigen::Ref<MatrixXd> hess)
            {
                ALTRO_ASSERT(hess.rows() == hess.cols(), "Hessian must be square.");
                ALTRO_ASSERT(x.size() == b.size(), "x and b must be the same size.");
                ALTRO_UNUSED(x);
                ALTRO_UNUSED(b);
                hess.setZero();
            }
        };

        /**
         * @brief The space of all negative numbers, an alias for inequality constraints.
         *
         * Used to represent inequality constraints of the form:
         * \f[ h(x) \leq 0 \f]
         *
         * The negative orthant is a self-dual cone, and it's projection operator is
         * an element-wise `min(0, x)`.
         *
         */
        class NegativeOrthant
        {
        public:
            NegativeOrthant() = delete;
            using DualCone    = NegativeOrthant;

            static void Projection(const VectorXdRef& x, Eigen::Ref<VectorXd> x_proj)
            {
                ALTRO_ASSERT(x.size() == x_proj.size(), "x and x_proj must be the same size");
                for (int i = 0; i < x.size(); ++i)
                {
                    x_proj(i) = std::min(0.0, x(i));
                }
            }
            static void Jacobian(const VectorXdRef& x, Eigen::Ref<MatrixXd> jac)
            {
                ALTRO_ASSERT(jac.rows() == jac.cols(), "Jacobian must be square.");
                for (int i = 0; i < x.size(); ++i)
                {
                    jac(i, i) = x(i) > 0 ? 0 : 1;
                }
            }
            static void Hessian(const VectorXdRef& x, const VectorXdRef& b, Eigen::Ref<MatrixXd> hess)
            {
                ALTRO_ASSERT(hess.rows() == hess.cols(), "Hessian must be square.");
                ALTRO_ASSERT(x.size() == b.size(), "x and b must be the same size.");
                ALTRO_UNUSED(x);
                ALTRO_UNUSED(b);
                hess.setZero();
            }
        };

        /**
         * @brief A alias for the `NegativeOrthant` cone.
         *
         */
        using Inequality = NegativeOrthant;

        /**
         * @brief Contains basic information about a single constraint
         *
         */
        struct ConstraintInfo
        {
            std::string label;
            int         index;
            VectorXd    violation;
            std::string type;

            std::string ToString(int precision = 4) const;
        };

        std::ostream& operator<<(std::ostream& os, const ConstraintInfo& coninfo);

        // clang-format off
/**
 * @brief An abstract constraint of the form:
 * \f[ g(x, u) \in K \f]
 *
 * where \f$ K \f$ is an arbitrary convex cone, specified by the `ConType` type parameteter.
 * This formulation supports generic equality and inequality constraints.
 *
 * # Interface
 * The user is expected to implement the folowing interface when defining a constraint:
 * - `int OutputDimension() const` - size of output (length of constraint).
 * - `void Evaluate(const VectorXdRef& x, const VectorXdRef& u, Eigen::Ref<Eigen::VectorXd> out)`
 * - `void Jacobian(const VectorXdRef& x, const VectorXdRef& u, Eigen::Ref<MatrixXd> out)`
 * - `std::string GetLabel() const` - A brief description of the constraint for printing.
 *
 * Where we use the following Eigen type alias:
 * 
 *      using VectorXdRef = Eigen::Ref<const Eigen::VectorXd>
 *
 * The constraint is required to at least have continuous 1st order derivatives,
 * and these derivatives must be implemented by the user. No automatic or
 * approximation differentiation methods are provided, although the Jacobian
 * can be verified using a finite difference method using `CheckJacobian`. See
 * documentation for `FunctionBase` for more information.
 *
 * @tparam ConType The type of constraint (equality, inequality, conic, etc.)
 */
        // clang-format on
        template<class ConType>
        class Constraint : public FunctionBase
        {
        public:
            using ConstraintType = ConType;

            // These aren't used right now, but they need to be defined.
            int StateDimension() const override
            {
                ALTRO_ASSERT(false, "StateDimension hasn't been defined for this constraint.");
                return -1;
            }
            int ControlDimension() const override
            {
                ALTRO_ASSERT(false, "ControlDimension hasn't been defined for this constraint.");
                return -1;
            }

            // TODO(bjackson) [SW-14476] add 2nd order terms when implementing DDP
            bool HasHessian() const override { return false; }

            virtual std::string GetLabel() const { return GetConstraintType(); }

            std::string GetConstraintType() const
            {
                if (std::is_same<ConType, Equality>::value)
                {
                    return "Equality Constraint";
                }
                else if (std::is_same<ConType, Inequality>::value)
                {
                    return "Inequality Constraint";
                }
                else
                {
                    return "Undefined Constraint Type";
                }
            }
        };

        template<class ConType>
        using ConstraintPtr = std::shared_ptr<Constraint<ConType>>;

    } // namespace constraints
} // namespace altro