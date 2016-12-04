/*
// @HEADER
// ************************************************************************
//
//        Piro: Strategy package for embedded analysis capabilitites
//                  Copyright (2010) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Andy Salinger (agsalin@sandia.gov), Sandia
// National Laboratories.
//
// ************************************************************************
// @HEADER
*/

#include "Piro_ConfigDefs.hpp"

#ifdef HAVE_PIRO_TEMPUS
#include "Piro_TempusSolver.hpp"
#include "Tempus_StepperBackwardEuler.hpp"
#include "Piro_ObserverToTempusIntegrationObserverAdapter.hpp"

#ifdef HAVE_PIRO_NOX
#include "Piro_NOXSolver.hpp"
#endif /* HAVE_PIRO_NOX */

#include "Piro_Test_ThyraSupport.hpp"
#include "Piro_Test_WeakenedModelEvaluator.hpp"
#include "Piro_Test_MockObserver.hpp"

#include "MockModelEval_A.hpp"

#include "Rythmos_BackwardEulerStepper.hpp"
#include "Rythmos_SimpleIntegrationControlStrategy.hpp"

#include "Thyra_EpetraModelEvaluator.hpp"
#include "Thyra_ModelEvaluatorHelpers.hpp"

#include "Thyra_DefaultNominalBoundsOverrideModelEvaluator.hpp"

#include "Thyra_AmesosLinearOpWithSolveFactory.hpp"

#include "Teuchos_UnitTestHarness.hpp"

#include "Teuchos_Ptr.hpp"
#include "Teuchos_Array.hpp"
#include "Teuchos_Tuple.hpp"

#include <stdexcept>

using namespace Teuchos;
using namespace Piro;
using namespace Piro::Test;

namespace Thyra {
  typedef ModelEvaluatorBase MEB;
} // namespace Thyra

// Setup support

const RCP<EpetraExt::ModelEvaluator> epetraModelNew()
{
#ifdef HAVE_MPI
  const MPI_Comm comm = MPI_COMM_WORLD;
#else /*HAVE_MPI*/
  const int comm = 0;
#endif /*HAVE_MPI*/
  return rcp(new MockModelEval_A(comm));
}

const RCP<Thyra::ModelEvaluatorDefaultBase<double> > thyraModelNew(const RCP<EpetraExt::ModelEvaluator> &epetraModel)
{
  const RCP<Thyra::LinearOpWithSolveFactoryBase<double> > lowsFactory(new Thyra::AmesosLinearOpWithSolveFactory);
  return epetraModelEvaluator(epetraModel, lowsFactory);
}

RCP<Thyra::ModelEvaluatorDefaultBase<double> > defaultModelNew()
{
  return thyraModelNew(epetraModelNew());
}
/*
RCP<Rythmos::IntegrationControlStrategyBase<double> > stepStrategyNew(double fixedTimeStep)
{
  const RCP<Teuchos::ParameterList> integrationControlPL =
    rcp(new Teuchos::ParameterList("Rythmos Integration Control"));
  integrationControlPL->set("Take Variable Steps", false);
  integrationControlPL->set("Fixed dt", fixedTimeStep);
  return Rythmos::simpleIntegrationControlStrategy<double>(integrationControlPL);
}
*/

const RCP<TempusSolver<double> > solverNew(
    const RCP<Thyra::ModelEvaluatorDefaultBase<double> > &thyraModel,
    double finalTime)
{
  const RCP<ParameterList> tempusPL(new ParameterList("Tempus"));
  tempusPL->set("Integrator Name", "Demo Integrator");
  tempusPL->sublist("Demo Integrator").set("Integrator Type", "Integrator Basic"); 
  tempusPL->sublist("Demo Integrator").set("Stepper Name", "Demo Stepper"); 
  tempusPL->sublist("Demo Integrator").sublist("Solution History").set("Storage Type", "Unlimited"); 
  tempusPL->sublist("Demo Integrator").sublist("Solution History").set("Storage Limit", 20); 
  tempusPL->sublist("Demo Integrator").sublist("Time Step Control").set("Minimum Simulation Time", 0.0); 
  tempusPL->sublist("Demo Stepper").set("Stepper Type", "Backward Euler"); 
  tempusPL->sublist("Demo Stepper").set("Solver Name", "Demo Solver"); 
  tempusPL->sublist("Demo Stepper").sublist("Demo Solver").sublist("NOX").sublist("Direction").set("Method","Newton"); 
  Teuchos::RCP<Tempus::IntegratorBasic<double> > integrator = Tempus::integratorBasic<double>(tempusPL, thyraModel);
  const RCP<Thyra::NonlinearSolverBase<double> > stepSolver = Teuchos::null;
  
  RCP<ParameterList> stepperPL = Teuchos::rcp(&(tempusPL->sublist("Demo Stepper")), false);
  const RCP<Tempus::Stepper<double> > stepper = rcp(new Tempus::StepperBackwardEuler<double>(stepperPL, thyraModel));
  return rcp(new TempusSolver<double>(integrator, stepper, stepSolver, thyraModel, finalTime));
}
/*
const RCP<RythmosSolver<double> > solverNew(
    const RCP<Thyra::ModelEvaluatorDefaultBase<double> > &thyraModel,
    double initialTime,
    double finalTime,
    const RCP<Piro::ObserverBase<double> > &observer)
{
  const RCP<Rythmos::IntegrationObserverBase<double> > rythmosObserver =
    rcp(new ObserverToRythmosIntegrationObserverAdapter<double>(observer));
  const RCP<Rythmos::DefaultIntegrator<double> > integrator =
    Rythmos::observedDefaultIntegrator(rythmosObserver);
  const RCP<Thyra::NonlinearSolverBase<double> > stepSolver =
    Rythmos::timeStepNonlinearSolver<double>();
  const RCP<Rythmos::SolverAcceptingStepperBase<double> > stepper =
    Rythmos::backwardEulerStepper<double>(thyraModel, stepSolver);

  return rcp(new RythmosSolver<double>(integrator, stepper, stepSolver, thyraModel, initialTime, finalTime));
}

const RCP<RythmosSolver<double> > solverNew(
    const RCP<Thyra::ModelEvaluatorDefaultBase<double> > &thyraModel,
    double initialTime,
    double finalTime,
    double fixedTimeStep,
    const RCP<Piro::ObserverBase<double> > &observer)
{
  const RCP<Rythmos::IntegrationControlStrategyBase<double> > stepStrategy =
    stepStrategyNew(fixedTimeStep);
  const RCP<Rythmos::IntegrationObserverBase<double> > rythmosObserver =
    rcp(new ObserverToRythmosIntegrationObserverAdapter<double>(observer));
  const RCP<Rythmos::DefaultIntegrator<double> > integrator =
    Rythmos::defaultIntegrator(stepStrategy, rythmosObserver);
  const RCP<Thyra::NonlinearSolverBase<double> > stepSolver =
    Rythmos::timeStepNonlinearSolver<double>();
  const RCP<Rythmos::SolverAcceptingStepperBase<double> > stepper =
    Rythmos::backwardEulerStepper<double>(thyraModel, stepSolver);

  return rcp(new RythmosSolver<double>(integrator, stepper, stepSolver, thyraModel, initialTime, finalTime));
}

const RCP<RythmosSolver<double> > solverNew(
    const RCP<Thyra::ModelEvaluatorDefaultBase<double> > &thyraModel,
    double finalTime,
    const RCP<Thyra::ModelEvaluatorDefaultBase<double> > &steadyStateModel)
{
  const RCP<Rythmos::DefaultIntegrator<double> > integrator =
    Rythmos::defaultIntegrator<double>();
  const RCP<Thyra::NonlinearSolverBase<double> > stepSolver =
    Rythmos::timeStepNonlinearSolver<double>();
  const RCP<Rythmos::SolverAcceptingStepperBase<double> > stepper =
    Rythmos::backwardEulerStepper<double>(thyraModel, stepSolver);

  return rcp(new RythmosSolver<double>(integrator, stepper, stepSolver, thyraModel, finalTime, steadyStateModel));
}

const RCP<RythmosSolver<double> > solverNew(
    const RCP<EpetraExt::ModelEvaluator> &epetraModel,
    double finalTime)
{
  return solverNew(thyraModelNew(epetraModel), finalTime);
}
*/
Thyra::ModelEvaluatorBase::InArgs<double> getStaticNominalValues(const Thyra::ModelEvaluator<double> &model)
{
  Thyra::ModelEvaluatorBase::InArgs<double> result = model.getNominalValues();
  if (result.supports(Thyra::ModelEvaluatorBase::IN_ARG_x_dot)) {
    result.set_x_dot(Teuchos::null);
  }
  return result;
}
/*
void vectorAssign(Thyra::VectorBase<double> &target, const ArrayView<const double> &values) {
  for (int i = 0; i < values.size(); ++i) {
    Thyra::set_ele(i, values[i], ptrFromRef(target));
  }
}
*/

// Floating point tolerance
const double tol = 1.0e-8;

TEUCHOS_UNIT_TEST(Piro_TempusSolver, TimeZero_Solution)
{
  const RCP<Thyra::ModelEvaluatorDefaultBase<double> > model = defaultModelNew();
  const double finalTime = 0.0;

  const RCP<TempusSolver<double> > solver = solverNew(model, finalTime);

  const Thyra::MEB::InArgs<double> inArgs = solver->getNominalValues();

  Thyra::MEB::OutArgs<double> outArgs = solver->createOutArgs();
  const int solutionResponseIndex = solver->Ng() - 1;
  const RCP<Thyra::VectorBase<double> > solution =
    Thyra::createMember(solver->get_g_space(solutionResponseIndex));
  outArgs.set_g(solutionResponseIndex, solution);

  solver->evalModel(inArgs, outArgs);

  const RCP<const Thyra::VectorBase<double> > initialCondition =
    model->getNominalValues().get_x();

  TEST_COMPARE_FLOATING_ARRAYS(
      arrayFromVector(*solution),
      arrayFromVector(*initialCondition),
      tol);
}


TEUCHOS_UNIT_TEST(Piro_TempusSolver, TimeZero_Response)
{
  const RCP<Thyra::ModelEvaluatorDefaultBase<double> > model = defaultModelNew();

  const int responseIndex = 0;

  const RCP<Thyra::VectorBase<double> > expectedResponse =
    Thyra::createMember(model->get_g_space(responseIndex));
  {
    const Thyra::MEB::InArgs<double> modelInArgs = getStaticNominalValues(*model);
    Thyra::MEB::OutArgs<double> modelOutArgs = model->createOutArgs();
    modelOutArgs.set_g(responseIndex, expectedResponse);
    model->evalModel(modelInArgs, modelOutArgs);
  }

  const double finalTime = 0.0;
  const RCP<TempusSolver<double> > solver = solverNew(model, finalTime);

  const Thyra::MEB::InArgs<double> inArgs = solver->getNominalValues();

  Thyra::MEB::OutArgs<double> outArgs = solver->createOutArgs();
  const RCP<Thyra::VectorBase<double> > response =
    Thyra::createMember(solver->get_g_space(responseIndex));
  outArgs.set_g(responseIndex, response);

  solver->evalModel(inArgs, outArgs);

  const Array<double> expected = arrayFromVector(*expectedResponse);
  const Array<double> actual = arrayFromVector(*response);
  TEST_COMPARE_FLOATING_ARRAYS(actual, expected, tol);
}

TEUCHOS_UNIT_TEST(Piro_TempusSolver, TimeZero_NoDfDpMv_NoSensitivity)
{
  const RCP<Thyra::ModelEvaluatorDefaultBase<double> > model(
      new WeakenedModelEvaluator_NoDfDpMv(defaultModelNew()));

  const double finalTime = 0.0;
  const RCP<TempusSolver<double> > solver = solverNew(model, finalTime);

  const Thyra::MEB::InArgs<double> inArgs = solver->getNominalValues();
  Thyra::MEB::OutArgs<double> outArgs = solver->createOutArgs();

  const int responseIndex = 0;
  const int solutionResponseIndex = solver->Ng() - 1;
  const int parameterIndex = 0;

  TEST_ASSERT(outArgs.supports(Thyra::MEB::OUT_ARG_DgDp, responseIndex, parameterIndex).none());
  TEST_ASSERT(outArgs.supports(Thyra::MEB::OUT_ARG_DgDp, solutionResponseIndex, parameterIndex).none());

  TEST_NOTHROW(solver->evalModel(inArgs, outArgs));
}


TEUCHOS_UNIT_TEST(Piro_TempusSolver, TimeZero_NoDgDp_NoResponseSensitivity)
{
  const RCP<Thyra::ModelEvaluatorDefaultBase<double> > model(
      new WeakenedModelEvaluator_NoDgDp(defaultModelNew()));

  const double finalTime = 0.0;
  const RCP<TempusSolver<double> > solver = solverNew(model, finalTime);

  const Thyra::MEB::InArgs<double> inArgs = solver->getNominalValues();
  Thyra::MEB::OutArgs<double> outArgs = solver->createOutArgs();

  const int responseIndex = 0;
  const int solutionResponseIndex = solver->Ng() - 1;
  const int parameterIndex = 0;

  TEST_ASSERT(outArgs.supports(Thyra::MEB::OUT_ARG_DgDp, responseIndex, parameterIndex).none());
  TEST_ASSERT(!outArgs.supports(Thyra::MEB::OUT_ARG_DgDp, solutionResponseIndex, parameterIndex).none());

  TEST_NOTHROW(solver->evalModel(inArgs, outArgs));
}

/*
TEUCHOS_UNIT_TEST(Piro_RythmosSolver, ObserveInitialCondition)
{
  const RCP<Thyra::ModelEvaluatorDefaultBase<double> > model = defaultModelNew();
  const RCP<MockObserver<double> > observer(new MockObserver<double>);
  const double timeStamp = 2.0;

  const RCP<RythmosSolver<double> > solver = solverNew(model, timeStamp, timeStamp, observer);

  const Thyra::MEB::InArgs<double> inArgs = solver->getNominalValues();
  const Thyra::MEB::OutArgs<double> outArgs = solver->createOutArgs();
  solver->evalModel(inArgs, outArgs);

  {
    const RCP<const Thyra::VectorBase<double> > solution =
      observer->lastSolution();

    const RCP<const Thyra::VectorBase<double> > initialCondition =
      model->getNominalValues().get_x();

    TEST_COMPARE_FLOATING_ARRAYS(
        arrayFromVector(*solution),
        arrayFromVector(*initialCondition),
        tol);
  }

  TEST_FLOATING_EQUALITY(observer->lastStamp(), timeStamp, tol);
}


TEUCHOS_UNIT_TEST(Piro_RythmosSolver, ObserveFinalSolution)
{
  const RCP<Thyra::ModelEvaluatorDefaultBase<double> > model = defaultModelNew();
  const RCP<MockObserver<double> > observer(new MockObserver<double>);
  const double initialTime = 0.0;
  const double finalTime = 0.1;
  const double timeStepSize = 0.05;

  const RCP<RythmosSolver<double> > solver =
    solverNew(model, initialTime, finalTime, timeStepSize, observer);

  const Thyra::MEB::InArgs<double> inArgs = solver->getNominalValues();

  Thyra::MEB::OutArgs<double> outArgs = solver->createOutArgs();
  const int solutionResponseIndex = solver->Ng() - 1;
  const RCP<Thyra::VectorBase<double> > solution =
    Thyra::createMember(solver->get_g_space(solutionResponseIndex));
  outArgs.set_g(solutionResponseIndex, solution);

  solver->evalModel(inArgs, outArgs);

  TEST_COMPARE_FLOATING_ARRAYS(
      arrayFromVector(*observer->lastSolution()),
      arrayFromVector(*solution),
      tol);

  TEST_FLOATING_EQUALITY(observer->lastStamp(), finalTime, tol);
}

// builds a simple backward euler stepper factory
template <typename Scalar>
class TestStepperFactory : public Piro::RythmosStepperFactory<Scalar> {
public:
  Teuchos::RCP<Rythmos::StepperBase<Scalar> > buildStepper(
                        const Teuchos::RCP<Thyra::ModelEvaluator<Scalar> > & model,
                        const Teuchos::RCP<Thyra::NonlinearSolverBase<Scalar> > & solver,
                        const Teuchos::RCP<Teuchos::ParameterList> & paramList)
  { return Rythmos::backwardEulerStepper<double>(model, solver); }
};

TEUCHOS_UNIT_TEST(Piro_RythmosSolver, ExternalStepper_Interface)
{
  // a simple parameter list to get things started
  Teuchos::RCP<Teuchos::ParameterList> pl =
    Teuchos::getParametersFromXmlString("\
   <ParameterList>\
     <ParameterList name=\"Rythmos\">\
       <Parameter name=\"Nonlinear Solver Type\" type=\"string\" value='Rythmos'/>\
       <Parameter name=\"Final Time\" type=\"double\" value=\"1\"/>\
       <ParameterList name=\"Stratimikos\">\
       </ParameterList>\
       <Parameter name=\"Stepper Type\" type=\"string\" value=\"Test Stepper\"/>\
       <ParameterList name=\"Rythmos Stepper\">\
       </ParameterList>\
       <ParameterList name=\"Rythmos Integrator\">\
       </ParameterList>\
       <ParameterList name=\"Rythmos Integration Control\">\
         <Parameter name=\"Take Variable Steps\" type=\"bool\" value=\"false\"/>\
         <Parameter name=\"Number of Time Steps\" type=\"int\" value=\"1\"/>\
       </ParameterList>\
     </ParameterList>\
   </ParameterList>");

  const RCP<Thyra::ModelEvaluatorDefaultBase<double> > model = defaultModelNew();

  {
    // this is simply to excercise externally added stepper is easy to use
    Teuchos::RCP<RythmosStepperFactory<double> > testStepperFactory = Teuchos::rcp(new TestStepperFactory<double>);

    const RCP<RythmosSolver<double> > solver = Teuchos::rcp(new RythmosSolver<double>);
    solver->addStepperFactory("Test Stepper",testStepperFactory); // now "Stepper Type" can be used

    solver->initialize(pl,model);
    // should find the "Test Stepper", so this method call should succeed
    TEST_NOTHROW(solver->initialize(pl,model));
  }

  {
    // this is simply to excercise externally added stepper is easy to use
    Teuchos::RCP<RythmosStepperFactory<double> > testStepperFactory = Teuchos::rcp(new TestStepperFactory<double>);

    const RCP<RythmosSolver<double> > solver = Teuchos::rcp(new RythmosSolver<double>);
    solver->addStepperFactory("Test Stepper New",testStepperFactory); // now "Stepper Type" can be used

    // There is no "Test Stepper" so this method call should throw
    TEST_THROW(solver->initialize(pl,model),Teuchos::Exceptions::InvalidParameter);
  }
}

*/
#endif /* HAVE_PIRO_TEMPUS */