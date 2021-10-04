///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (C) 2021, University of Edinburgh, University of Trento
// Copyright note valid unless otherwise stated in individual files.
// All rights reserved.
///////////////////////////////////////////////////////////////////////////////

namespace crocoddyl {

template <typename Scalar>
ControlParametrizationModelPolyTwoRK4Tpl<Scalar>::ControlParametrizationModelPolyTwoRK4Tpl(const std::size_t nw)
    : Base(nw, 3 * nw) {}

template <typename Scalar>
ControlParametrizationModelPolyTwoRK4Tpl<Scalar>::~ControlParametrizationModelPolyTwoRK4Tpl() {}

template <typename Scalar>
void ControlParametrizationModelPolyTwoRK4Tpl<Scalar>::calc(
    const boost::shared_ptr<ControlParametrizationDataAbstract>& data, const Scalar t,
    const Eigen::Ref<const VectorXs>& u) const {
  if (static_cast<std::size_t>(u.size()) != nu_) {
    throw_pretty("Invalid argument: "
                 << "u has wrong dimension (it should be " + std::to_string(nu_) + ")");
  }
  Data* d = static_cast<Data*>(data.get());
  const Eigen::VectorBlock<const Eigen::Ref<const VectorXs> >& p0 = u.head(nw_);
  const Eigen::VectorBlock<const Eigen::Ref<const VectorXs> >& p1 = u.segment(nw_, nw_);
  const Eigen::VectorBlock<const Eigen::Ref<const VectorXs> >& p2 = u.tail(nw_);
  d->tmp_t2 = t * t;
  d->c[2] = Scalar(2.) * d->tmp_t2 - t;
  d->c[1] = -Scalar(2.) * d->c[2] + Scalar(2.) * t;
  d->c[0] = d->c[2] - Scalar(2.) * t + Scalar(1.);
  d->w = d->c[2] * p2 + d->c[1] * p1 + d->c[0] * p0;
}

template <typename Scalar>
void ControlParametrizationModelPolyTwoRK4Tpl<Scalar>::calcDiff(
    const boost::shared_ptr<ControlParametrizationDataAbstract>& data, const Scalar,
    const Eigen::Ref<const VectorXs>&) const {
  Data* d = static_cast<Data*>(data.get());
  d->dw_du.leftCols(nw_).diagonal().array() = d->c[0];
  d->dw_du.middleCols(nw_, nw_).diagonal().array() = d->c[1];
  d->dw_du.rightCols(nw_).diagonal().array() = d->c[2];
}

template <typename Scalar>
boost::shared_ptr<ControlParametrizationDataAbstractTpl<Scalar> >
ControlParametrizationModelPolyTwoRK4Tpl<Scalar>::createData() {
  return boost::allocate_shared<Data>(Eigen::aligned_allocator<Data>(), this);
}

template <typename Scalar>
void ControlParametrizationModelPolyTwoRK4Tpl<Scalar>::params(
    const boost::shared_ptr<ControlParametrizationDataAbstract>& data, const Scalar,
    const Eigen::Ref<const VectorXs>& w) const {
  if (static_cast<std::size_t>(w.size()) != nw_) {
    throw_pretty("Invalid argument: "
                 << "w has wrong dimension (it should be " + std::to_string(nw_) + ")");
  }
  data->u.head(nw_) = w;
  data->u.segment(nw_, nw_) = w;
  data->u.tail(nw_) = w;
}

template <typename Scalar>
void ControlParametrizationModelPolyTwoRK4Tpl<Scalar>::convertBounds(const Eigen::Ref<const VectorXs>& w_lb,
                                                                     const Eigen::Ref<const VectorXs>& w_ub,
                                                                     Eigen::Ref<VectorXs> u_lb,
                                                                     Eigen::Ref<VectorXs> u_ub) const {
  if (static_cast<std::size_t>(u_lb.size()) != nu_) {
    throw_pretty("Invalid argument: "
                 << "u_lb has wrong dimension (it should be " + std::to_string(nu_) + ")");
  }
  if (static_cast<std::size_t>(u_ub.size()) != nu_) {
    throw_pretty("Invalid argument: "
                 << "u_ub has wrong dimension (it should be " + std::to_string(nu_) + ")");
  }
  if (static_cast<std::size_t>(w_lb.size()) != nw_) {
    throw_pretty("Invalid argument: "
                 << "w_lb has wrong dimension (it should be " + std::to_string(nw_) + ")");
  }
  if (static_cast<std::size_t>(w_ub.size()) != nw_) {
    throw_pretty("Invalid argument: "
                 << "w_ub has wrong dimension (it should be " + std::to_string(nw_) + ")");
  }
  u_lb.head(nw_) = w_lb;
  u_lb.segment(nw_, nw_) = w_lb;
  u_lb.tail(nw_) = w_lb;
  u_ub.head(nw_) = w_ub;
  u_ub.segment(nw_, nw_) = w_ub;
  u_ub.tail(nw_) = w_ub;
}

template <typename Scalar>
void ControlParametrizationModelPolyTwoRK4Tpl<Scalar>::multiplyByJacobian(
    const boost::shared_ptr<ControlParametrizationDataAbstract>& data, const Eigen::Ref<const MatrixXs>& A,
    Eigen::Ref<MatrixXs> out, const AssignmentOp op) const {
  assert_pretty(is_a_AssignmentOp(op), ("op must be one of the AssignmentOp {settop, addto, rmfrom}"));
  if (A.rows() != out.rows() || static_cast<std::size_t>(A.cols()) != nw_ ||
      static_cast<std::size_t>(out.cols()) != nu_) {
    throw_pretty("Invalid argument: "
                 << "A and out have wrong dimensions (" + std::to_string(A.rows()) + "," + std::to_string(A.cols()) +
                        " and " + std::to_string(out.rows()) + "," + std::to_string(out.cols()) + +")");
  }
  Data* d = static_cast<Data*>(data.get());
  switch (op) {
    case setto:
      out.leftCols(nw_) = d->c[0] * A;
      out.middleCols(nw_, nw_) = d->c[1] * A;
      out.rightCols(nw_) = d->c[2] * A;
      break;
    case addto:
      out.leftCols(nw_) += d->c[0] * A;
      out.middleCols(nw_, nw_) += d->c[1] * A;
      out.rightCols(nw_) += d->c[2] * A;
      break;
    case rmfrom:
      out.leftCols(nw_) -= d->c[0] * A;
      out.middleCols(nw_, nw_) -= d->c[1] * A;
      out.rightCols(nw_) -= d->c[2] * A;
      break;
    default:
      throw_pretty("Invalid argument: allowed operators: setto, addto, rmfrom");
      break;
  }
}

template <typename Scalar>
void ControlParametrizationModelPolyTwoRK4Tpl<Scalar>::multiplyJacobianTransposeBy(
    const boost::shared_ptr<ControlParametrizationDataAbstract>& data, const Eigen::Ref<const MatrixXs>& A,
    Eigen::Ref<MatrixXs> out, const AssignmentOp op) const {
  assert_pretty(is_a_AssignmentOp(op), ("op must be one of the AssignmentOp {settop, addto, rmfrom}"));
  if (A.cols() != out.cols() || static_cast<std::size_t>(A.rows()) != nw_ ||
      static_cast<std::size_t>(out.rows()) != nu_) {
    throw_pretty("Invalid argument: "
                 << "A and out have wrong dimensions (" + std::to_string(A.rows()) + "," + std::to_string(A.cols()) +
                        " and " + std::to_string(out.rows()) + "," + std::to_string(out.cols()) + ")");
  }
  Data* d = static_cast<Data*>(data.get());
  switch (op) {
    case setto:
      out.topRows(nw_) = d->c[0] * A;
      out.middleRows(nw_, nw_) = d->c[1] * A;
      out.bottomRows(nw_) = d->c[2] * A;
      break;
    case addto:
      out.topRows(nw_) += d->c[0] * A;
      out.middleRows(nw_, nw_) += d->c[1] * A;
      out.bottomRows(nw_) += d->c[2] * A;
      break;
    case rmfrom:
      out.topRows(nw_) -= d->c[0] * A;
      out.middleRows(nw_, nw_) -= d->c[1] * A;
      out.bottomRows(nw_) -= d->c[2] * A;
      break;
    default:
      throw_pretty("Invalid argument: allowed operators: setto, addto, rmfrom");
      break;
  }
}

}  // namespace crocoddyl
