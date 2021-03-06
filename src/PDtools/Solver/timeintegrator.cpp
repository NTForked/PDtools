#include "timeintegrator.h"

#include "PDtools/Force/force.h"
#include "PDtools/Modfiers/modifier.h"
#include "PDtools/Particles/pd_particles.h"
#include "PDtools/SavePdData/savepddata.h"

#include "Utilities/epd_functions.h"

namespace PDtools {
//------------------------------------------------------------------------------
void TimeIntegrator::solve() {
  initialize();
  checkInitialization();
  calculateForces(0);
  updateProperties(0);
  save(0);

  // Looping over all time, particles and components.
  for (int i = 0; i < m_steps; i++) {
    stepForward(i);
  }
}
//------------------------------------------------------------------------------
void TimeIntegrator::stepForward(int timeStep) {
  applyBoundaryConditions();
  integrateStepOne();

  updateGridAndCommunication();
  updateProperties(timeStep + 1);
  updateGhosts();
  //    updateElementQuadrature(*m_particles);

  modifiersStepOne();
  save(timeStep + 1);
  //----------------------------------------------------------------------
  zeroForces();
  calculateForces(timeStep + 1);
  //----------------------------------------------------------------------
  updateGhosts();

  modifiersStepTwo();
  integrateStepTwo();

  m_t += m_dt;
}
//------------------------------------------------------------------------------
void TimeIntegrator::initialize() {
  if (m_particles->hasParameter("rho")) {
    m_indexRho = m_particles->getParamId("rho");
  } else if (m_particles->hasParameter("mass")) {
    m_indexRho = m_particles->getParamId("mass");
  } else {
    cerr << "ERROR: Particles data does not contain either"
         << " 'rho' or 'mass'. This is needed for time integration." << endl;
    throw ParticlesMissingRhoOrMass;
  }

  Solver::initialize();
}
//------------------------------------------------------------------------------
void TimeIntegrator::checkInitialization() {
  if (m_dt == 0) {
    cerr << "Error: 'dt' is not set in the timeintegrator" << endl;
    throw TimeStepNotSet;
  }

  // Check for forces and order of modifiers
  Solver::checkInitialization();
}
//------------------------------------------------------------------------------
}
