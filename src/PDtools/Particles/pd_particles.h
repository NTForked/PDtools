#ifndef PD_PARTICLES_H
#define PD_PARTICLES_H

#include <map>
#include "particles.h"

// For element based PD
#include "Mesh/pdmesh.h"
#include "Elements/pd_element.h"
#include "Utilities/gaussianquadrature.h"

namespace PDtools
{
//------------------------------------------------------------------------------
// Extending the particle struct for PD-data
//------------------------------------------------------------------------------

class PD_Particles: public Particles
{
protected:
    // For all particles
    mat m_r0;
    mat m_r_prev;
    mat m_F;
    mat m_b;
    mat m_u;

    // For ADR
    vec m_stableMass;
    mat m_Fold;

    unordered_map<int, vector<pair<int, vector<double>>>> m_PdConnections;
    unordered_map<string, int> m_PdParameters;

    map<int, vector<int>> m_sendtParticles;
    map<int, vector<int>> m_receivedParticles;
    vector<vector<int>>  m_sendtParticles2;
    vector<vector<int>>  m_receivedParticles2;

    // For element based PD
    unordered_map<int, int> m_idToElement;
    unordered_map<int, int> m_elementToId;
    vector<PD_triElement> m_triElements;
    vector<PD_quadElement> m_quadElements;

    // For gaussian integration
    mat m_gaussianPoints;
    mat m_shapeFunction;
    vec m_gaussianWeights;
public:
    PD_Particles();

    virtual void initializeElements(const size_t nTriangles,
                                    const size_t nQuads, const size_t degree);

    virtual void initializeMatrices();

    void initializeBodyForces();

    void setPdConnections(int id, vector<pair<int, vector<double>>> connections);

    vector<pair<int, vector<double>>> & pdConnections(int id);

    virtual void deleteParticleById(const int deleteId);

    mat & r0();
    mat & r_prev();
    mat & F();
    vec & stableMass();
    mat & Fold();
    mat & b();
    mat & u();

    const unordered_map<string, int> & PdParameters() const;

    int getPdParamId(string paramId) const;

    int registerPdParameter(string paramId, double value=0);

    void dimensionalScaling(const double E0, const double L0, const double v0,
                       const double t0, const double rho0);

    void sendtParticles(map<int, vector<int> > sp);
    const map<int, vector<int> > & sendtParticles() const;
    void receivedParticles(map<int, vector<int>> sp);
    const map<int, vector<int> > & receivedParticles() const;
    void clearGhostParameters();
    vector<vector<int> > getSendtParticles2() const;
    void setSendtParticles2(const vector<vector<int> > &sendtParticles2);
    vector<vector<int> > getReceivedParticles2() const;
    void setReceivedParticles2(const vector<vector<int> > &receivedParticles2);

    void addTri(const PD_triElement &tElement);
    void addQuad(const PD_quadElement &qElement);

    vector<PD_quadElement> &getQuadElements();
    vector<PD_triElement> &getTriElements();
    const mat & getShapeFunction() const;
    void setShapeFunction(const mat &shapeFunction);
    unordered_map<int, int> &getIdToElement();
    size_t nIntegrationPoints() const;

    void uppdateR_prev();
};
//------------------------------------------------------------------------------
// Inline functions

inline void PD_Particles::setPdConnections(int id, vector<pair<int, vector<double>>> connections) {
    m_PdConnections[id] = connections;
}

inline vector<pair<int, vector<double>>> & PD_Particles::pdConnections(int id) {
    return m_PdConnections.at(id);
}

inline mat & PD_Particles::u() {
    return m_u;
}

inline mat & PD_Particles::r0() {
    return m_r0;
}

inline mat & PD_Particles::r_prev() {
    return m_r_prev;
}

inline mat & PD_Particles::F() {
    return m_F;
}

inline vec & PD_Particles::stableMass() {
    return m_stableMass;
}

inline mat & PD_Particles::Fold() {
    return m_Fold;
}

inline mat & PD_Particles::b() {
    return m_b;
}

inline void PD_Particles::sendtParticles(map<int, vector<int> > sp) {
    m_sendtParticles = sp;
}

inline const map<int, vector<int>> & PD_Particles::sendtParticles() const {
    return m_sendtParticles;
}

inline void PD_Particles::receivedParticles(map<int, vector<int> > sp) {
    m_receivedParticles = sp;
}

inline const map<int, vector<int>> & PD_Particles::receivedParticles() const {
    return m_receivedParticles;
}
//------------------------------------------------------------------------------
}
#endif // PD_PARTICLES_H
