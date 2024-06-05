///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (C) 2019-2024, LAAS-CNRS, University of Edinburgh,
//                          Heriot-Watt University
// Copyright note valid unless otherwise stated in individual files.
// All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "crocoddyl/core/utils/exception.hpp"

namespace crocoddyl {

template <typename Scalar>
ActionModelLQRTpl<Scalar>::ActionModelLQRTpl(const MatrixXs& A,
                                             const MatrixXs& B,
                                             const MatrixXs& Q,
                                             const MatrixXs& R,
                                             const MatrixXs& N)
    : Base(boost::make_shared<StateVector>(A.cols()), B.cols(), 0),
      drift_free_(true) {
  const std::size_t nx = state_->get_nx();
  VectorXs f = VectorXs::Zero(nx);
  VectorXs q = VectorXs::Zero(nx);
  VectorXs r = VectorXs::Zero(nu_);
  set_LQR(A, B, Q, R, N, f, q, r);
}

template <typename Scalar>
ActionModelLQRTpl<Scalar>::ActionModelLQRTpl(
    const MatrixXs& A, const MatrixXs& B, const MatrixXs& Q, const MatrixXs& R,
    const MatrixXs& N, const VectorXs& f, const VectorXs& q, const VectorXs& r)
    : Base(boost::make_shared<StateVector>(A.cols()), B.cols(), 0),
      drift_free_(false) {
  set_LQR(A, B, Q, R, N, f, q, r);
}

template <typename Scalar>
ActionModelLQRTpl<Scalar>::ActionModelLQRTpl(const std::size_t nx,
                                             const std::size_t nu,
                                             const bool drift_free)
    : Base(boost::make_shared<StateVector>(nx), nu, 0),
      A_(MatrixXs::Identity(nx, nx)),
      B_(MatrixXs::Identity(nx, nu)),
      Q_(MatrixXs::Identity(nx, nx)),
      R_(MatrixXs::Identity(nu, nu)),
      N_(MatrixXs::Zero(nx, nu)),
      f_(drift_free ? VectorXs::Zero(nx) : VectorXs::Ones(nx)),
      q_(VectorXs::Ones(nx)),
      r_(VectorXs::Ones(nu)),
      drift_free_(drift_free) {}

template <typename Scalar>
ActionModelLQRTpl<Scalar>::~ActionModelLQRTpl() {}

template <typename Scalar>
void ActionModelLQRTpl<Scalar>::calc(
    const boost::shared_ptr<ActionDataAbstract>& data,
    const Eigen::Ref<const VectorXs>& x, const Eigen::Ref<const VectorXs>& u) {
  if (static_cast<std::size_t>(x.size()) != state_->get_nx()) {
    throw_pretty("Invalid argument: "
                 << "x has wrong dimension (it should be " +
                        std::to_string(state_->get_nx()) + ")");
  }
  if (static_cast<std::size_t>(u.size()) != nu_) {
    throw_pretty("Invalid argument: "
                 << "u has wrong dimension (it should be " +
                        std::to_string(nu_) + ")");
  }
  Data* d = static_cast<Data*>(data.get());

  data->xnext.noalias() = A_ * x;
  data->xnext.noalias() += B_ * u;
  data->xnext += f_;

  // cost = 0.5 * x^T * Q * x + 0.5 * u^T * R * u + x^T * N * u + q^T * x + r^T
  // * u
  d->Q_x_tmp.noalias() = Q_ * x;
  data->cost = Scalar(0.5) * x.dot(d->Q_x_tmp);
  d->R_u_tmp.noalias() = R_ * u;
  data->cost += Scalar(0.5) * u.dot(d->R_u_tmp);
  d->Q_x_tmp.noalias() = N_ * u;
  data->cost += x.dot(d->Q_x_tmp);
  data->cost += q_.dot(x);
  data->cost += r_.dot(u);
}

template <typename Scalar>
void ActionModelLQRTpl<Scalar>::calc(
    const boost::shared_ptr<ActionDataAbstract>& data,
    const Eigen::Ref<const VectorXs>& x) {
  if (static_cast<std::size_t>(x.size()) != state_->get_nx()) {
    throw_pretty("Invalid argument: "
                 << "x has wrong dimension (it should be " +
                        std::to_string(state_->get_nx()) + ")");
  }
  Data* d = static_cast<Data*>(data.get());

  d->xnext = x;
  // cost = 0.5 * x^T * Q * x + q^T * x
  d->Q_x_tmp.noalias() = Q_ * x;
  data->cost = Scalar(0.5) * x.dot(d->Q_x_tmp);
  data->cost += q_.dot(x);
}

template <typename Scalar>
void ActionModelLQRTpl<Scalar>::calcDiff(
    const boost::shared_ptr<ActionDataAbstract>& data,
    const Eigen::Ref<const VectorXs>& x, const Eigen::Ref<const VectorXs>& u) {
  if (static_cast<std::size_t>(x.size()) != state_->get_nx()) {
    throw_pretty("Invalid argument: "
                 << "x has wrong dimension (it should be " +
                        std::to_string(state_->get_nx()) + ")");
  }
  if (static_cast<std::size_t>(u.size()) != nu_) {
    throw_pretty("Invalid argument: "
                 << "u has wrong dimension (it should be " +
                        std::to_string(nu_) + ")");
  }

  data->Fx = A_;
  data->Fu = B_;
  data->Lxx = Q_;
  data->Luu = R_;
  data->Lxu = N_;
  data->Lx = q_;
  data->Lx.noalias() += Q_ * x;
  data->Lx.noalias() += N_ * u;
  data->Lu = r_;
  data->Lu.noalias() += N_.transpose() * x;
  data->Lu.noalias() += R_ * u;
}

template <typename Scalar>
void ActionModelLQRTpl<Scalar>::calcDiff(
    const boost::shared_ptr<ActionDataAbstract>& data,
    const Eigen::Ref<const VectorXs>& x) {
  if (static_cast<std::size_t>(x.size()) != state_->get_nx()) {
    throw_pretty("Invalid argument: "
                 << "x has wrong dimension (it should be " +
                        std::to_string(state_->get_nx()) + ")");
  }

  data->Lxx = Q_;
  data->Lx = q_;
  data->Lx.noalias() += Q_ * x;
}

template <typename Scalar>
boost::shared_ptr<ActionDataAbstractTpl<Scalar>>
ActionModelLQRTpl<Scalar>::createData() {
  return boost::allocate_shared<Data>(Eigen::aligned_allocator<Data>(), this);
}

template <typename Scalar>
bool ActionModelLQRTpl<Scalar>::checkData(
    const boost::shared_ptr<ActionDataAbstract>& data) {
  boost::shared_ptr<Data> d = boost::dynamic_pointer_cast<Data>(data);
  if (d != NULL) {
    return true;
  } else {
    return false;
  }
}

template <typename Scalar>
ActionModelLQRTpl<Scalar> ActionModelLQRTpl<Scalar>::Random(
    const std::size_t nx, const std::size_t nu) {
  MatrixXs A = MatrixXs::Random(nx, nx);
  MatrixXs B = MatrixXs::Random(nx, nu);
  MatrixXs H_tmp = MatrixXs::Random(nx + nu, nx + nu);
  MatrixXs H = H_tmp.transpose() * H_tmp;
  const Eigen::Block<MatrixXs> Q = H.topLeftCorner(nx, nx);
  const Eigen::Block<MatrixXs> R = H.bottomRightCorner(nu, nu);
  const Eigen::Block<MatrixXs> N = H.topRightCorner(nx, nu);
  VectorXs f = VectorXs::Random(nx);
  VectorXs q = VectorXs::Random(nx);
  VectorXs r = VectorXs::Random(nu);
  return ActionModelLQRTpl<Scalar>(A, B, Q, R, N, f, q, r);
}

template <typename Scalar>
void ActionModelLQRTpl<Scalar>::print(std::ostream& os) const {
  os << "ActionModelLQR {nx=" << state_->get_nx() << ", nu=" << nu_
     << ", drift_free=" << drift_free_ << "}";
}

template <typename Scalar>
const typename MathBaseTpl<Scalar>::MatrixXs& ActionModelLQRTpl<Scalar>::get_A()
    const {
  return A_;
}

template <typename Scalar>
const typename MathBaseTpl<Scalar>::MatrixXs& ActionModelLQRTpl<Scalar>::get_B()
    const {
  return B_;
}

template <typename Scalar>
const typename MathBaseTpl<Scalar>::VectorXs& ActionModelLQRTpl<Scalar>::get_f()
    const {
  return f_;
}

template <typename Scalar>
const typename MathBaseTpl<Scalar>::MatrixXs& ActionModelLQRTpl<Scalar>::get_Q()
    const {
  return Q_;
}

template <typename Scalar>
const typename MathBaseTpl<Scalar>::MatrixXs& ActionModelLQRTpl<Scalar>::get_R()
    const {
  return R_;
}

template <typename Scalar>
const typename MathBaseTpl<Scalar>::MatrixXs& ActionModelLQRTpl<Scalar>::get_N()
    const {
  return N_;
}

template <typename Scalar>
const typename MathBaseTpl<Scalar>::VectorXs& ActionModelLQRTpl<Scalar>::get_q()
    const {
  return q_;
}

template <typename Scalar>
const typename MathBaseTpl<Scalar>::VectorXs& ActionModelLQRTpl<Scalar>::get_r()
    const {
  return r_;
}

template <typename Scalar>
void ActionModelLQRTpl<Scalar>::set_LQR(const MatrixXs& A, const MatrixXs& B,
                                        const MatrixXs& Q, const MatrixXs& R,
                                        const MatrixXs& N, const VectorXs& f,
                                        const VectorXs& q, const VectorXs& r) {
  const std::size_t nx = state_->get_nx();
  if (static_cast<std::size_t>(A.rows()) != nx) {
    throw_pretty("Invalid argument: "
                 << "A should be a squared matrix with size " +
                        std::to_string(nx));
  }
  if (static_cast<std::size_t>(B.rows()) != nx) {
    throw_pretty("Invalid argument: "
                 << "B has wrong dimension (it should have " +
                        std::to_string(nx) + " rows)");
  }
  if (static_cast<std::size_t>(Q.rows()) != nx ||
      static_cast<std::size_t>(Q.cols()) != nx) {
    throw_pretty("Invalid argument: "
                 << "Q has wrong dimension (it should be " +
                        std::to_string(nx) + "x " + std::to_string(nx) + ")");
  }
  if (static_cast<std::size_t>(R.rows()) != nu_ ||
      static_cast<std::size_t>(R.cols()) != nu_) {
    throw_pretty("Invalid argument: "
                 << "R has wrong dimension (it should be " +
                        std::to_string(nu_) + "x " + std::to_string(nu_) + ")");
  }
  if (static_cast<std::size_t>(N.rows()) != nx ||
      static_cast<std::size_t>(N.cols()) != nu_) {
    throw_pretty("Invalid argument: "
                 << "N has wrong dimension (it should be " +
                        std::to_string(nx) + "x " + std::to_string(nu_) + ")");
  }
  if (static_cast<std::size_t>(f.size()) != nx) {
    throw_pretty("Invalid argument: "
                 << "f has wrong dimension (it should be " +
                        std::to_string(nx) + ")");
  }
  if (static_cast<std::size_t>(q.size()) != nx) {
    throw_pretty("Invalid argument: "
                 << "q has wrong dimension (it should be " +
                        std::to_string(nx) + ")");
  }
  if (static_cast<std::size_t>(r.size()) != nu_) {
    throw_pretty("Invalid argument: "
                 << "r has wrong dimension (it should be " +
                        std::to_string(nu_) + ")");
  }
  H_ = MatrixXs::Zero(nx + nu_, nx + nu_);
  H_ << Q, N, N.transpose(), R;
  Eigen::LLT<MatrixXs> H_llt(H_);
  if (!H_.isApprox(H_.transpose()) || H_llt.info() == Eigen::NumericalIssue) {
    throw_pretty("Invalid argument "
                 << "[Q, N; N.T, R] is not semi-positive definite");
  }

  A_ = A;
  B_ = B;
  f_ = f;
  Q_ = Q;
  R_ = R;
  N_ = N;
  q_ = q;
  r_ = r;
}

}  // namespace crocoddyl
