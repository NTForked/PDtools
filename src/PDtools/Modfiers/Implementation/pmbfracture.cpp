#include "pmbfracture.h"

#include <limits>
#include "PDtools/Particles/pd_particles.h"

namespace PDtools
{
//------------------------------------------------------------------------------
PmbFracture::PmbFracture(double alpha):
    m_alpha(alpha)
{
}
//------------------------------------------------------------------------------
PmbFracture::~PmbFracture()
{

}
//------------------------------------------------------------------------------
void PmbFracture::registerParticleParameters()
{
    m_indexS0 = m_particles->registerParameter("s0");
    m_indexS_tmp = m_particles->registerParameter("s_tmp");
    m_indexUnbreakable =  m_particles->registerParameter("unbreakable");
    m_indexStretch = m_particles->getPdParamId("stretch");
    m_indexS00 = m_particles->registerPdParameter("s00");
    m_indexConnected = m_particles->registerPdParameter("connected");
    m_data = &m_particles->data();
    m_initialGhostParameters = {"s0"};
    m_ghostParameters = {"s0"};
    m_idToCol = &m_particles->idToCol();
}
//------------------------------------------------------------------------------
void PmbFracture::initialize()
{
    const ivec & colToId = m_particles->colToId();

    // Setting the initial max stretch between two particles
    for(unsigned int i=0; i<m_particles->nParticles(); i++)
    {
        const int id_i = colToId(i);
        const double s0_i = (*m_data)(i, m_indexS0);

        vector<pair<int, vector<double>>> & PDconnections = m_particles->pdConnections(id_i);
        for(auto &con:PDconnections)
        {
            const int id_j = con.first;
            const int col_j = (*m_idToCol)[id_j];
            const double s0_j = (*m_data)(col_j, m_indexS0);
            con.second[m_indexS00] = 0.5*(s0_i + s0_j);
            m_s00 = s0_i; // TMP FIX
        }
    }
}
//------------------------------------------------------------------------------
void PmbFracture::evaluateStepOne(const int id_i, const int i)
{
    if((*m_data)(i, m_indexUnbreakable) >= 1)
        return;

    const double s0_i = (*m_data)(i, m_indexS0);
    vector<pair<int, vector<double>>> & PDconnections = m_particles->pdConnections(id_i);

    double s0_new = numeric_limits<double>::min();

    for(auto &con:PDconnections)
    {
        const int id_j = con.first;
        const int j = (*m_idToCol)[id_j];

        if((*m_data)(j, m_indexUnbreakable) >= 1)
            continue;

        if(con.second[m_indexConnected] <= 0.5)
            continue;

        const double stretch = con.second[m_indexStretch];
        const double s00 = con.second[m_indexS00];
        const double s0_j = (*m_data)(j, m_indexS0);
        const double s0 = std::min(s0_i, s0_j);
//        if(id_i == 1916)
//            cout << stretch << " ";

        if(stretch > s0)
        {
             con.second[m_indexConnected] = 0;
        }

        double s0_tmp = s00 - m_alpha*stretch;
        s0_new = std::max(s0_new, s0_tmp);
//        cout << stretch << " " << s0_i << " " << s0_j << endl;
    }

    (*m_data)(i, m_indexS_tmp) = s0_new;
}
//------------------------------------------------------------------------------
void PmbFracture::updateStepOne(const int id_i, const int i)
{
    (void) id_i;
    (*m_data)(i, m_indexS0) = (*m_data)(i, m_indexS_tmp);
}
//------------------------------------------------------------------------------
}

