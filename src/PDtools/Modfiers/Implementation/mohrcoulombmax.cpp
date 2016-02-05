#include "mohrcoulombmax.h"

//#include "PDtools/Force/force.h"
#include "PDtools/Particles/pd_particles.h"

namespace PDtools
{
//------------------------------------------------------------------------------
MohrCoulombMax::MohrCoulombMax(double mu, double C, double T, int dim):
    m_C(C), m_T(T), m_dim(dim)
{
    m_phi = mu*M_PI/180.;
    m_d = tan(m_phi);
    m_neededProperties= {pair<string, int>("stress", 1)};
    m_C = 0.5*C*(1./(sqrt(m_d*m_d + 1.) + m_d));
}
//------------------------------------------------------------------------------
void MohrCoulombMax::registerParticleParameters()
{
    m_data = &m_particles->data();
    m_indexUnbreakable =  m_particles->registerParameter("unbreakable");
    m_indexConnected = m_particles->registerPdParameter("connected");
    m_indexCompute = m_particles->registerPdParameter("compute");
    m_indexStressCenter = m_particles->registerPdParameter("stressIndex");
    m_indexBroken = m_particles->registerParameter("broken", 0);
    m_idToCol = &m_particles->idToCol();

    switch(m_dim)
    {
    case 1:
        m_ghostParameters = {"s_xx"};
        m_indexStress[0] = m_particles->registerParameter("s_xx");
        break;
    case 2:
        m_ghostParameters = {"s_xx", "s_yy", "s_xy"};
        m_indexStress[0] = m_particles->registerParameter("s_xx");
        m_indexStress[1] = m_particles->registerParameter("s_yy");
        m_indexStress[2] = m_particles->registerParameter("s_xy");
        break;
    case 3:
        m_ghostParameters = {"s_xx", "s_yy", "s_zz", "s_xy", "s_xz", "s_yz"};
        m_indexStress[0] = m_particles->registerParameter("s_xx");
        m_indexStress[1] = m_particles->registerParameter("s_yy");
        m_indexStress[2] = m_particles->registerParameter("s_xy");
        m_indexStress[3] = m_particles->registerParameter("s_zz");
        m_indexStress[4] = m_particles->registerParameter("s_xz");
        m_indexStress[5] = m_particles->registerParameter("s_yz");
        break;
    }
    m_ghostParameters.push_back("broken");
}
//------------------------------------------------------------------------------
void MohrCoulombMax::initialize()
{
    const int indexRadius=  m_particles->getParamId("radius");
    const std::unordered_map<int, int> &idToCol = m_particles->idToCol();
    const ivec &colToId = m_particles->colToId();
    const int indexDr0 = m_particles->getPdParamId("dr0");
    const int nParticles = m_particles->nParticles();
    const mat & R0 = m_particles->r0();
    const mat & data = *m_data;
    m_broken = false;

    const double sf = 1.15;

#ifdef USE_OPENMP
# pragma omp parallel for
#endif
    for(unsigned int i=0; i<nParticles; i++)
    {
        const int id_a = colToId.at(i);
        vector<pair<int, vector<double>>> & PDconnections = m_particles->pdConnections(id_a);
        const vec & r_a = R0.row(i).t();
        const double radius_i = sf*data(i, indexRadius);

        for(auto &con:PDconnections)
        {
            const int id_c = con.first;
            const int c = idToCol.at(id_c);
            const double dr0_ac = con.second[indexDr0];
            const double radius_c = sf*data(c, indexRadius);
            const double radius_ac = radius_i + radius_c;

            int m = max(id_a, id_c);
            int mi = min(id_a, id_c);
//            cout << m << " " << mi << endl;
            con.second[m_indexStressCenter] = -1;
            if(mi%m == 0)
            {
                con.second[m_indexStressCenter] = m;
            }else
            {
                con.second[m_indexStressCenter] = mi;
            }

            /*
            if(radius_ac >= dr0_ac)
            {
                con.second[m_indexStressCenter] = -1;
                continue;
            }
            const vec & r_c = R0.row(c).t();
            const vec & center = 0.5*(r_c + r_a);
            int closestIndex = -1;
            double closestDistance = numeric_limits<double>::max();

            for(auto &con_b:PDconnections)
            {
                const int id_b = con_b.first;
                const int b = idToCol.at(id_b);
                if(c == b)
                    continue;

                const vec & r_b = R0.row(b).t();

                // Distance to center
                 const arma::vec& diff = center - r_b;

                 double distanceToCenterSq = 0;
                 for(int d=0;d<m_dim; d++)
                 {
                     distanceToCenterSq += diff(d)*diff(d);
                 }

                 if(distanceToCenterSq <= closestDistance)
                 {
                     closestIndex = id_b;
                     closestDistance = distanceToCenterSq;
                 }
            }

            if(closestIndex != -1)
            {
                con.second[m_indexStressCenter] = closestIndex;
//                cout << id_a << " " << id_c << " " << closestIndex << endl;
            }
            else
            {
                cerr << "ERROR in calculating the closest stress point." << endl;
                exit(EXIT_FAILURE);
            }
            */
        }
    }
}
//------------------------------------------------------------------------------
void MohrCoulombMax::evaluateStepOne(const int id_i, const int i)
{
    const mat & data = *m_data;

    if(data(i, m_indexUnbreakable) >= 1)
        return;

    const int broken_i = data(i, m_indexBroken);

    vector<pair<int, vector<double>>> & PDconnections = m_particles->pdConnections(id_i);

    for(auto &con:PDconnections)
    {
        const int id_j = con.first;
        const int j = (*m_idToCol)[id_j];

        if((*m_data)(j, m_indexUnbreakable) >= 1)
            continue;

        if(con.second[m_indexConnected] <= 0.5)
            continue;

        if(broken_i)
        {
            con.second[m_indexConnected] = 0;
            m_broken = true;
        }

        const int broken_j = data(j, m_indexBroken);
        if(broken_j)
        {
            con.second[m_indexConnected] = 0;
            m_broken = true;
        }
    }
}
//------------------------------------------------------------------------------
void MohrCoulombMax::evaluateStepTwo(const int id_i, const int i)
{
    // First calculating the total stress on an material point. The stress is averaged
    // all its bonds, and the stress state on a bond is the mean of the
    // stress at each material point.
    mat & data = *m_data;

    double cos_theta = cos(M_PI/2. + m_phi);
    double sin_theta = sin(M_PI/2. + m_phi);

    if(m_dim == 2)
    {
        double sx, sy, sxy;
        sx = data(i, m_indexStress[0]);
        sy = data(i, m_indexStress[1]);
        sxy = data(i, m_indexStress[2]);

        double first = 0.5*(sx + sy);
        double second = sqrt(0.25*(sx - sy)*(sx - sy) + sxy*sxy);

        double s1 = first + second;
        double s2 = first - second;
        double p_1 = min(s1, s2);
        double p_2 = max(s1, s2);

        double shear = fabs(0.5*(p_1 - p_2)*sin_theta);
        double normal = 0.5*(p_1 + p_2) + 0.5*(p_1 - p_2)*cos_theta;

        if(shear >= fabs(m_C - m_d*normal) && normal < 0)
        {
            data(i, m_indexBroken) = 1;
        }
        else if(p_2 >= m_T)
        {
            data(i, m_indexBroken) = 1;
        }
    }
}
//------------------------------------------------------------------------------
void MohrCoulombMax::evaluateStepTwo()
{
    if(m_broken)
    {
        m_state = true;
    }
    else
    {
        m_state = false;
    }

    m_broken = false;
}
//------------------------------------------------------------------------------
}
/*
double sx, sy, sxy;
if(con.second[m_indexStressCenter] <= -1)
{
    sx = 0.5*(data(i, m_indexStress[0]) + data(j, m_indexStress[0]));
    sy = 0.5*(data(i, m_indexStress[1]) + data(j, m_indexStress[1]));
    sxy = 0.5*(data(i, m_indexStress[2]) + data(j, m_indexStress[2]));
}
else
{
    const int id_b = con.second[m_indexStressCenter];
    const int b = (*m_idToCol)[id_b];
    sx =  data(b, m_indexStress[0]);
    sy =  data(b, m_indexStress[1]);
    sxy = data(b, m_indexStress[2]);
}

sx = data(i, m_indexStress[0]);
sy = data(i, m_indexStress[1]);
sxy = data(i, m_indexStress[2]);

double first = 0.5*(sx + sy);
double second = sqrt(0.25*(sx - sy)*(sx - sy) + sxy*sxy);

double s1 = first + second;
double s2 = first - second;
double p_1 = min(s1, s2);
double p_2 = max(s1, s2);

double shear = fabs(0.5*(p_1 - p_2)*sin_theta);
double normal = 0.5*(p_1 + p_2) + 0.5*(p_1 - p_2)*cos_theta;

if(shear >= fabs(m_C - m_d*normal) && normal < 0)
{
    con.second[m_indexConnected] = 0;
}
else if(p_2 >= m_T)
{
    con.second[m_indexConnected] = 0;
}

sx = data(j, m_indexStress[0]);
sy = data(j, m_indexStress[1]);
sxy = data(j, m_indexStress[2]);

first = 0.5*(sx + sy);
second = sqrt(0.25*(sx - sy)*(sx - sy) + sxy*sxy);

s1 = first + second;
s2 = first - second;
p_1 = min(s1, s2);
p_2 = max(s1, s2);

shear = fabs(0.5*(p_1 - p_2)*sin_theta);
normal = 0.5*(p_1 + p_2) + 0.5*(p_1 - p_2)*cos_theta;

if(shear >= fabs(m_C - m_d*normal) && normal < 0)
{
    con.second[m_indexConnected] = 0;
}
else if(p_2 >= m_T)
{
    con.second[m_indexConnected] = 0;
}
*/