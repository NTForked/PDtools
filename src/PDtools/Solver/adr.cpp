#include "adr.h"

#include "PDtools/Particles/pd_particles.h"
#include "PDtools/Force/force.h"
#include "PDtools/Modfiers/modifier.h"
#include "PDtools/SavePdData/savepddata.h"

using namespace arma;

namespace PDtools
//------------------------------------------------------------------------------
{
ADR::ADR()
{
    m_dt = 1.0;
    m_c = 0;
}
//------------------------------------------------------------------------------
ADR::~ADR()
{

}
//------------------------------------------------------------------------------
void ADR::solve()
{
    initialize();
    checkInitialization();

    // Looping over all time, particles and components.
    for (int i = 0; i < m_steps; i++)
    {
        stepForward(i);
    }
}
//------------------------------------------------------------------------------
void ADR::stepForward(int i)
{
    modifiersStepOne();
    save(i);

    m_globalError = 2*m_errorThreshold;
    iterate();

    cout << "i = " << i << " " << m_globalError << endl;

    modifiersStepTwo();
    m_t += m_dt;
}
//------------------------------------------------------------------------------
void ADR::iterate()
{
    int counter = 0;
    int nParticles = m_particles->nParticles();

    do
    {
        integrateStepOne();

        calculateForces();
        staticModifiers();

        integrateStepTwo();

        counter++;
        if(counter >200)
            break;
    }
    while(m_globalError > m_errorThreshold);

    cout << "counter = " << counter << " error =" << m_globalError << endl;
    calculateForces();


    //--------------------------------------------------
    if(!m_qsModifiers.empty())
    {
#ifdef USE_OPENMP
# pragma omp parallel for
#endif
        for(int i=0; i<nParticles; i++)
        {
            pair<int, int> id(i, i);
            for(Modifier *modifier:m_qsModifiers)
            {
                modifier->evaluateStepOne(id);
            }

        }
#ifdef USE_OPENMP
# pragma omp parallel for
#endif
        for(int i=0; i<nParticles; i++)
        {
            pair<int, int> id(i, i);
            for(Modifier *modifier:m_qsModifiers)
            {
                modifier->evaluateStepTwo(id);
            }

        }
        for(Modifier *modifier:m_qsModifiers)
        {
            modifier->evaluateStepTwo();
        }

        bool state = false;

        for(Modifier *modifier:m_qsModifiers)
        {
            if(modifier->state())
                state = true;
        }

        if(state)
        {
            m_globalError = 2*m_errorThreshold;
            iterate();
        }
    }
}
//------------------------------------------------------------------------------
void ADR::addQsModifiers(Modifier *mod)
{
    m_qsModifiers.push_back(mod);
}
//------------------------------------------------------------------------------
void ADR::checkInitialization()
{
}
//------------------------------------------------------------------------------
void ADR::initialize()
{
    //-----------------------------
    mat & F = m_particles->F();
    mat & v = m_particles->v();
    mat & Fold = m_particles->Fold();
    int nParticles = m_particles->nParticles();
#ifdef USE_OPENMP
# pragma omp parallel for
#endif
    for(int i=0; i<nParticles; i++)
    {
        for(int d=0; d<m_dim; d++){
            F(d, i) = 0;
            Fold(d, i) = 0;
            v(d, i) = 0;
        }
    }

    //-----------------------------
    calculateForces();
    calculateStableMass();
    Solver::initialize();
}
//------------------------------------------------------------------------------
void ADR::calculateStableMass()
{
    arma::vec &stableMass = m_particles->stableMass();
    int nParticles = m_particles->nParticles();
#ifdef USE_OPENMP
# pragma omp parallel for
#endif
    for(int i=0; i<nParticles; i++)
    {
        pair<int, int> id(i, i);
        double stableMassMax = 0;
        for(Force *oneBodyForce:m_oneBodyForces)
        {
            double sm = oneBodyForce->calculateStableMass(id, m_dt);
            stableMassMax = max(stableMassMax, sm);
        }
        stableMass(id.second) = stableMassMax;
    }
}
//------------------------------------------------------------------------------
void ADR::integrateStepOne()
{
    using namespace arma;

    double alpha = (2.*m_dt)/(2. + m_c*m_dt);
    double beta = (2. - m_c*m_dt)/(2. + m_c*m_dt);
    double rho = sqrt(beta);
    double errorScaling = rho/(1.0001 - rho);

    mat & r = m_particles->r();
    mat & v = m_particles->v();
    mat & F = m_particles->F();
    mat & Fold = m_particles->Fold();
    arma::imat & isStatic = m_particles->isStatic();
    const vec & stableMass = m_particles->stableMass();
    double error = 0.0;
    double Fn = 0;
    double deltaFn = 0;
    int nParticles = m_particles->nParticles();
//#ifdef USE_OPENMP
//# pragma omp parallel for reduction(max: error)
//#endif
    for(int i=0; i<nParticles; i++)
    {
        int col_i = i;
        if(isStatic(col_i))
            continue;

        double drSquared = 0;

        for(int d=0; d<m_dim; d++){
            double rPrev = r(d, col_i);

            v(d, col_i) = beta*v(d, col_i) + alpha*F(d, col_i)/stableMass(col_i);
            r(d, col_i) += v(d, col_i)*m_dt;

            deltaFn += pow(Fold(d, col_i) - F(d, col_i), 2);
            Fn += pow(F(d, col_i), 2);

            Fold(d, col_i) = F(d, col_i);
            F(d, col_i) = 0;

            drSquared += (r(d, col_i) - rPrev)*(r(d, col_i) - rPrev);
        }

        double error_i = errorScaling*sqrt(drSquared);
        error = std::max(error, error_i);
    }

    m_globalError = error;
    m_globalError = sqrt(deltaFn/Fn);

    if(std::isnan(m_globalError))
        m_globalError = 2*m_errorThreshold;
}
//------------------------------------------------------------------------------
void ADR::integrateStepTwo()
{

    const mat & v = m_particles->v();
    const mat & F = m_particles->F();
    const mat & Fold = m_particles->Fold();
    const vec & stableMass = m_particles->stableMass();
    arma::imat & isStatic = m_particles->isStatic();

    // Calculating the damping coefficient
    double numerator = 0;
    double denominator = 0;
    int nParticles = m_particles->nParticles();

#ifdef USE_OPENMP
# pragma omp parallel for
#endif
    for(int i=0; i<nParticles; i++)
    {
        int col_i = i;
        if(isStatic(col_i))
            continue;
        for(int d=0; d<m_dim; d++)
        {
            numerator += -v(d, col_i)*(F(d, col_i) - Fold(d, col_i))
                    /(stableMass(col_i)*m_dt);
            denominator += v(d, col_i)*v(d, col_i);
        }
    }

    m_c = 0;
    if(denominator > 0)
    {
        if(numerator/denominator > 0)
        {
            m_c = 2*sqrt(numerator/denominator);
        }
    }

//    if(m_c >= 2.0)
//    {
////        cout << "c = " << m_c << endl;
//        m_c = 1.999999999999999999999999999;
//    }
//    m_c = 1.999999999999999999999999999;
}
//------------------------------------------------------------------------------
void ADR::staticModifiers()
{
    for(Modifier *modifier:m_modifiers)
    {
        modifier->staticEvaluation();
    }
}
//------------------------------------------------------------------------------
}
