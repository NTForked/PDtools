#include "calculatestressstrain.h"

#include "Particles/pd_particles.h"
#include "PDtools/Force/force.h"

namespace PDtools
{
//------------------------------------------------------------------------------
CalculateStressStrain::CalculateStressStrain(vector<Force *> &forces, double E,
                                             double nu, double delta,
                                             bool planeStress,
                                             bool greenLagrange):
    CalculateProperty("stress"),
    m_E(E),
    m_nu(nu),
    m_delta(delta),
    m_planeStress(planeStress)
{
    for(Force* force: forces)
    {
        m_forces.push_back(force);
    }

    m_greenStrain = greenLagrange;
    if(m_greenStrain)
    {
        m_smallStrain = false;
    }
    else
    {
        m_smallStrain = true;
    }
}
//------------------------------------------------------------------------------
void CalculateStressStrain::initialize()
{
    m_particles->setNeedGhostR0(true);
    m_indexConnected = m_particles->getPdParamId("connected");
    m_indexVolume = m_particles->getParamId("volume");
    m_indexVolumeScaling = m_particles->getPdParamId("volumeScaling");
    m_indexDr0 = m_particles->getPdParamId("dr0");
    m_indexBrokenNow = m_particles->registerParameter("brokenNow", 0);

    switch(m_dim)
    {
    case 1:
        m_nStressStrainElements = 1;
        m_indexStress[0] = m_particles->registerParameter("s_xx");
        m_indexStrain[0] = m_particles->registerParameter("e_xx");
        m_indexK[0] = m_particles->registerParameter("K_x");
        break;
    case 2:
        m_nStressStrainElements = 3;
        m_indexK[0] = m_particles->registerParameter("K_x");
        m_indexK[1] = m_particles->registerParameter("K_y");
        m_indexK[2] = m_particles->registerParameter("K_xy");
        m_indexStress[0] = m_particles->registerParameter("s_xx");
        m_indexStress[1] = m_particles->registerParameter("s_yy");
        m_indexStress[2] = m_particles->registerParameter("s_xy");
        m_indexStrain[0] = m_particles->registerParameter("e_xx");
        m_indexStrain[1] = m_particles->registerParameter("e_yy");
        m_indexStrain[2] = m_particles->registerParameter("e_xy");
        break;
    case 3:
        m_nStressStrainElements = 6;
        m_indexK[0] = m_particles->registerParameter("K_x");
        m_indexK[1] = m_particles->registerParameter("K_y");
        m_indexK[2] = m_particles->registerParameter("K_xy");
        m_indexK[3] = m_particles->registerParameter("K_zz");
        m_indexK[4] = m_particles->registerParameter("K_xz");
        m_indexK[5] = m_particles->registerParameter("K_yz");
        m_indexStress[0] = m_particles->registerParameter("s_xx");
        m_indexStress[1] = m_particles->registerParameter("s_yy");
        m_indexStress[2] = m_particles->registerParameter("s_xy");
        m_indexStress[3] = m_particles->registerParameter("s_zz");
        m_indexStress[4] = m_particles->registerParameter("s_xz");
        m_indexStress[5] = m_particles->registerParameter("s_yz");
        m_indexStrain[0] = m_particles->registerParameter("e_xx");
        m_indexStrain[1] = m_particles->registerParameter("e_yy");
        m_indexStrain[2] = m_particles->registerParameter("e_xy");
        m_indexStrain[3] = m_particles->registerParameter("e_zz");
        m_indexStrain[4] = m_particles->registerParameter("e_xz");
        m_indexStrain[5] = m_particles->registerParameter("e_yz");
        break;
    }

    // Computing the shape tensor
    const ivec &colToId = m_particles->colToId();
    const int nParticles = m_particles->nParticles();

    for(int i=0; i<nParticles; i++)
    {
        const int id_i = colToId(i);
        computeK(id_i, i);
    }
}
//------------------------------------------------------------------------------
void CalculateStressStrain::clean()
{
    /*
    const int nParticles = m_particles->nParticles();
    arma::mat & data = m_particles->data();

    // Zeroing the stress
#ifdef USE_OPENMP
#pragma omp parallel for
#endif
    for(int i=0; i<nParticles; i++)
    {
        for(int s=0; s<m_nStressStrainElements; s++)
        {
            data(i, m_indexStress[s]) = 0;
            data(i, m_indexStrain[s]) = 0;
        }
    }
    */
}
//------------------------------------------------------------------------------
void CalculateStressStrain::update()
{
    const ivec &colToId = m_particles->colToId();
    const auto &idToCol = m_particles->idToCol();
    const int nParticles = m_particles->nParticles();
    const mat &r = m_particles->r();
    const mat &r0 = m_particles->r0();
    mat &data = m_particles->data();

    mat F = zeros(m_dim, m_dim);
    mat E = zeros(m_dim, m_dim);
    mat K = zeros(m_dim, m_dim);
    mat P = zeros(m_dim, m_dim);
    mat strain = zeros(m_dim, m_dim);

    double dr_ij[m_dim];
    double dr0_ij[m_dim];

    for(int i=0; i<nParticles; i++)
    {
        const int id_i = colToId(i);
        if(data(i, m_indexBrokenNow))
        {
            computeK(id_i, i);
            data(i, m_indexBrokenNow) = 0;
        }

        if(m_dim >= 2)
        {
            K(0, 0) = data(i, m_indexK[0]);
            K(1, 1) = data(i, m_indexK[1]);
            K(0, 1) = data(i, m_indexK[2]);
            K(1, 0) = K(0, 1);
        }

        F.zeros();
        vector<pair<int, vector<double>>> & PDconnections_i = m_particles->pdConnections(id_i);
        const int nConnections = PDconnections_i.size();
        int nConnected = 0;

        for(int l_j=0; l_j<nConnections; l_j++)
        {
            const auto &con_i = PDconnections_i[l_j];
            if(con_i.second[m_indexConnected] <= 0.5)
                continue;

            const int id_j = con_i.first;
            const int j = idToCol.at(id_j);

            const double vol_j = data(j, m_indexVolume);
            const double dr0 = con_i.second[m_indexDr0];
            const double volumeScaling_ij = con_i.second[m_indexVolumeScaling];
            const double vol = vol_j*volumeScaling_ij;
            const double w = m_delta/dr0;

            for(int d=0; d<m_dim; d++)
            {
                dr_ij[d] = r(j, d) - r(i, d);
                dr0_ij[d] = r0(j, d) - r0(i, d);
            }

            for(int d=0; d<m_dim; d++)
            {
                for(int d2=0; d2<m_dim; d2++)
                {
                    F(d, d2) += w*dr_ij[d]*dr0_ij[d2]*vol;
                }
            }

            nConnected++;
        }

        if(nConnected <= 3)
        {
            data(i, m_indexStrain[0]) = 0;
            data(i, m_indexStrain[1]) = 0;
            data(i, m_indexStrain[2]) = 0;
            data(i, m_indexStress[0]) = 0;
            data(i, m_indexStress[1]) = 0;
            data(i, m_indexStress[2]) = 0;
            continue;
        }
        else
        {
            F = F*K; // K = inv(K);
            if(m_smallStrain)
            {
                strain = 0.5*(F.t() + F);
                strain(0,0) -= 1.;
                strain(1,1) -= 1.;

            }
            else
            {
                strain = 0.5*F.t()*F;
                strain(0,0) -= 0.5;
                strain(1,1) -= 0.5;
            }
        }

        data(i, m_indexStrain[0]) = strain(0,0);
        data(i, m_indexStrain[1]) = strain(1,1);
        data(i, m_indexStrain[2]) = strain(0,1);

        // Assuming linear elasticity
        if(m_dim == 2)
        {
            // Constituent model, linear elastic
            // Computing the second PK stress
            if(m_planeStress)
            {
                const double a = m_E/(1. - m_nu*m_nu);
                P(0,0) = a*(strain(0,0) + m_nu*strain(1,1));
                P(1,1) = a*(strain(1,1) + m_nu*strain(0,0));
                P(0,1) = a*(1 - m_nu)*strain(0,1);
                P(1,0) = P(0,1);
            }
            else // Plane strain
            {
                const double a = m_E/((1. + m_nu)*(1. - 2*m_nu));
                P(0,0) = a*((1.-m_nu)*strain(0,0) + m_nu*strain(1,1));
                P(1,1) = a*((1.-m_nu)*strain(1,1) + m_nu*strain(0,0));
                P(0,1) = a*0.5*(1. - 2.*m_nu)*strain(0,1);
                P(1,0) = P(0,1);
            }

            // Converting PK stress to Cauchy stress
            if(m_greenStrain)
            {
                const double detF = 1./det(F);
                P = detF*F*P*F.t();
            }
            data(i, m_indexStress[0]) = P(0,0);
            data(i, m_indexStress[1]) = P(1,1);
            data(i, m_indexStress[2]) = P(0,1);
        }

        if(m_dim == 3)
        {
//            F(2,2) -= 0.5;
//            data(i, m_indexStrain[3]) = F(2,2);
//            data(i, m_indexStrain[4]) = F(0,2);
//            data(i, m_indexStrain[5]) = F(1,2);
        }
    }

    //--------------------------------------------------------------------------
    // Adding stress from non-pd forces
    //--------------------------------------------------------------------------
//    for(Force *force: m_forces)
//    {
//        if(!force->getCalulateStress())
//            continue;

//        for(int i=0; i<nParticles; i++)
//        {
//            const int id_i = colToId(i);
//            force->calculateStress(id_i, i, m_indexStress);
//        }
    //    }
}
//------------------------------------------------------------------------------
void CalculateStressStrain::computeK(int id, int i)
{
    const auto &idToCol = m_particles->idToCol();
    const mat &r0 = m_particles->r0();
    mat &data = m_particles->data();
    mat K = zeros(m_dim, m_dim);
    vector<pair<int, vector<double>>> & PDconnections_i = m_particles->pdConnections(id);
    const int nConnections = PDconnections_i.size();
    double dr0_ij[m_dim];

    if(id == 26161)
    {
        double b = 2;
    }
    int nConnected = 0;
    for(int l_j=0; l_j<nConnections; l_j++)
    {
        const auto &con_i = PDconnections_i[l_j];
        if(con_i.second[m_indexConnected] <= 0.5)
            continue;

        const int id_j = con_i.first;
        const int j = idToCol.at(id_j);

        const double vol_j = data(j, m_indexVolume);
        const double dr0 = con_i.second[m_indexDr0];
        const double volumeScaling_ij = con_i.second[m_indexVolumeScaling];
        const double vol = vol_j*volumeScaling_ij;
        const double w = m_delta/dr0;

        for(int d=0; d<m_dim; d++)
        {
            dr0_ij[d] = r0(j, d) - r0(i, d);
        }

        for(int d=0; d<m_dim; d++)
        {
            for(int d2=0; d2<m_dim; d2++)
            {
                K(d, d2) += w*dr0_ij[d]*dr0_ij[d2]*vol;
            }
        }

        nConnected++;
    }

    if(nConnected <= 3)
    {
        if(m_dim >= 2)
        {
            data(i, m_indexK[0]) = 0;
            data(i, m_indexK[1]) = 0;
            data(i, m_indexK[2]) = 0;
        }
        if(m_dim == 3)
        {
            data(i, m_indexK[3]) = 0;
            data(i, m_indexK[4]) = 0;
            data(i, m_indexK[5]) = 0;
        }
    }
    else
    {
//        K = inv_sympd(K);
        K = inv(K);
    }

    if(m_dim >= 2)
    {
        data(i, m_indexK[0]) = K(0,0);
        data(i, m_indexK[1]) = K(1,1);
        data(i, m_indexK[2]) = K(0,1);
    }
    if(m_dim == 3)
    {
        data(i, m_indexK[3]) = K(2,2);
        data(i, m_indexK[4]) = K(0,2);
        data(i, m_indexK[5]) = K(1,2);
    }
}
//------------------------------------------------------------------------------
}
