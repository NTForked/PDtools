#include "pdfunctionsmpi.h"

#ifdef USE_MPI
#include <mpi.h>
#ifdef USE_BOOST_MPI
#endif
#include <boost/mpi.hpp>
#endif

#include "config.h"
#include "Grid/grid.h"
#include "Modfiers/modifier.h"
#include "Particles/pd_particles.h"

#define DEBUG_MPI_PRINT 0

namespace PDtools {

//------------------------------------------------------------------------------
void exchangeGhostParticles_boundary(Grid &grid, PD_Particles &particles) {
#ifdef USE_MPI
  const int myRank = MPI::COMM_WORLD.Get_rank();

  // Collecting the boundary particles
  map<int, vector<pair<int, int>>> toNeighbours;
  const vector<int> boundaryGridPoints = grid.boundaryGridPoints();
  unordered_map<int, GridPoint> &gridpoints = grid.gridpoints();

  for (int gId : boundaryGridPoints) {
    const GridPoint &gridPoint = gridpoints.at(gId);
    const vector<int> &neighbourRanks = gridPoint.neighbourRanks();
    const vector<pair<int, int>> l_particles = gridPoint.particles();

    for (const int nRank : neighbourRanks) {
      vector<pair<int, int>> &l_p = toNeighbours[nRank];
      l_p.insert(l_p.end(), l_particles.begin(), l_particles.end());
    }
  }
  const vector<int> &ghostParameters = particles.ghostParameters();
  const int needVelocity = particles.needGhostVelocity();
  const int needR0 = particles.getNeedGhostR0();
  ivec &idToCol = particles.getIdToCol_v();
  ivec &colToId = particles.colToId();
  mat &r = particles.r();
  mat &r0 = particles.r0();
  mat &v = particles.v();
  mat &data = particles.data();

  // Sending the ghost particles to the other cpus
  const int nParticles = particles.nParticles();
  int nGhostparams = 1 + M_DIM + ghostParameters.size();
  if (needVelocity)
    nGhostparams += M_DIM;
  if (needR0)
    nGhostparams += M_DIM;

  int nGhostParticles = particles.nGhostParticles();

  for (const auto &id_toNeighbours : toNeighbours) {
    const int toNode = id_toNeighbours.first;
    const vector<pair<int, int>> &l_p = id_toNeighbours.second;

    vector<double> ghostSend;

    for (const auto &id_col : l_p) {
      const int id_i = id_col.first;
      const int col = id_col.second;
      ghostSend.push_back(id_i);
      for (int d = 0; d < M_DIM; d++) {
        ghostSend.push_back(r(col, d));
      }
      if (needVelocity) {
        for (int d = 0; d < M_DIM; d++) {
          ghostSend.push_back(v(col, d));
        }
      }
      if (needR0) {
        for (int d = 0; d < M_DIM; d++) {
          ghostSend.push_back(r0(col, d));
        }
      }
      for (const int j : ghostParameters) {
        ghostSend.push_back(data(col, j));
      }
    }

    int nSendElements = ghostSend.size();
    int nRecieveElements;
    MPI_Status status;
    MPI_Sendrecv(&nSendElements, 1, MPI_INT, toNode, myRank * 10000,
                 &nRecieveElements, 1, MPI_INT, toNode, 10000 * toNode,
                 MPI_COMM_WORLD, &status);

    double ghostRecieve[nRecieveElements];

    MPI_Sendrecv(&ghostSend[0], nSendElements, MPI_DOUBLE, toNode,
                 myRank * 12000, &ghostRecieve, nRecieveElements, MPI_DOUBLE,
                 toNode, 12000 * toNode, MPI_COMM_WORLD, &status);

    double l_r[3] = {0, 0, 0};
    size_t j = 0;
    while (j < nRecieveElements) {
      //            int j = i*nGhostparams;
      const int col = nParticles + nGhostParticles;
      const int id = ghostRecieve[j++];
      idToCol[id] = col;
      colToId[col] = id;

      for (int d = 0; d < M_DIM; d++) {
        r(col, d) = ghostRecieve[j++];
      }
      if (needVelocity) {
        for (int d = 0; d < M_DIM; d++) {
          v(col, d) = ghostRecieve[j++];
        }
      }
      if (needR0) {
        for (int d = 0; d < M_DIM; d++) {
          r0(col, d) = ghostRecieve[j++];
        }
      }
      for (const int g : ghostParameters) {
        data(col, g) = ghostRecieve[j++];
      }

      nGhostParticles++;

      // Adding to the ghost particle to the correct grid point
      l_r[0] = r(col, 0);
      l_r[1] = r(col, 1);
      l_r[2] = r(col, 2);
      const int gId = grid.gridId(l_r);
      const pair<int, int> id_pos(id, col);
      GridPoint &gp = gridpoints[gId];
      gp.addParticle(id_pos);
    }
  }
  particles.nGhostParticles(nGhostParticles);
#endif
}
//------------------------------------------------------------------------------
void exchangeInitialGhostParticles_boundary(Grid &grid,
                                            PD_Particles &particles) {
#ifdef USE_MPI
  // Collecting the boundary particles
  map<int, vector<pair<int, int>>> toNeighbours;
  const vector<int> boundaryGridPoints = grid.boundaryGridPoints();
  unordered_map<int, GridPoint> &gridpoints = grid.gridpoints();

  for (int gId : boundaryGridPoints) {
    const GridPoint &gridPoint = gridpoints.at(gId);
    const vector<int> &neighbourRanks = gridPoint.neighbourRanks();
    const vector<pair<int, int>> l_particles = gridPoint.particles();

    for (const int nRank : neighbourRanks) {
      vector<pair<int, int>> &l_p = toNeighbours[nRank];
      l_p.insert(l_p.end(), l_particles.begin(), l_particles.end());
    }
  }

  const vector<int> &ghostParameters = particles.ghostParameters();
  const int nPdParameters = particles.PdParameters().size();
  ivec &idToCol = particles.getIdToCol_v();
  ivec &colToId = particles.colToId();
  mat &r = particles.r();
  mat &r0 = particles.r0();
  mat &data = particles.data();

  // Sending the ghost particles to the other CPU's
  const size_t nParticles = particles.nParticles();
  size_t nGhostParticles = particles.nGhostParticles();

  for (const auto &id_toNeighbours : toNeighbours) {
    const int toNode = id_toNeighbours.first;
    const vector<pair<int, int>> &l_p = id_toNeighbours.second;

    vector<double> sendData;

    for (const auto &id_col : l_p) {
      const int id = id_col.first;
      const int col = id_col.second;
      const auto &pd_connections = particles.pdConnections(id);

      sendData.push_back(id);
      for (int d = 0; d < M_DIM; d++) {
        sendData.push_back(r(col, d));
      }
      for (int d = 0; d < M_DIM; d++) {
        sendData.push_back(r0(col, d));
      }
      for (const int j : ghostParameters) {
        sendData.push_back(data(col, j));
      }

      sendData.push_back(pd_connections.size());
      for (const auto &con : pd_connections) {
        sendData.push_back(con.first);

        for (const double &param : con.second) {
          sendData.push_back(param);
        }
      }
    }

    // Sending and receiving data
    int nSendElements = sendData.size();
    int nRecieveElements;
    MPI_Status status;

    MPI_Sendrecv(&nSendElements, 1, MPI_INT, toNode, 0, &nRecieveElements, 1,
                 MPI_INT, toNode, 0, MPI_COMM_WORLD, &status);

    vector<double> recieveData;
    recieveData.resize(nRecieveElements);

    MPI_Sendrecv(&sendData[0], nSendElements, MPI_DOUBLE, toNode, 1,
                 &recieveData[0], nRecieveElements, MPI_DOUBLE, toNode, 1,
                 MPI_COMM_WORLD, &status);

    // Storing the received ghost data
    unsigned int j = 0;
    while (j < nRecieveElements) {
      const int col = nParticles + nGhostParticles;
      const int id = (int)recieveData[j++];

      idToCol[id] = col;
      colToId[col] = id;
      for (int d = 0; d < M_DIM; d++) {
        r(col, d) = recieveData[j++];
      }
      for (int d = 0; d < M_DIM; d++) {
        r0(col, d) = recieveData[j++];
      }
      for (const int g : ghostParameters) {
        data(col, g) = recieveData[j++];
      }

      const int nPdConnections = recieveData[j++];
      vector<pair<int, vector<double>>> connectionsVector;

      for (int i = 0; i < nPdConnections; i++) {
        const int con_id = (int)recieveData[j++];
        vector<double> connectionData;

        for (int k = 0; k < nPdParameters; k++) {
          connectionData.push_back(recieveData[j++]);
        }

        connectionsVector.push_back(
            pair<int, vector<double>>(con_id, connectionData));
      }
      particles.setPdConnections(id, connectionsVector);
      nGhostParticles++;

      // Adding to the ghost particles to the correct grid point
      const vec3 &l_r = r.row(col).t();
      const int gId = grid.gridId(l_r);
      const pair<int, int> id_pos(id, col);
      GridPoint &gp = gridpoints[gId];
      gp.addParticle(id_pos);
    }
  }

  particles.nGhostParticles(nGhostParticles);
#endif
}
//------------------------------------------------------------------------------
void updateGrid(Grid &grid, PD_Particles &particles, const bool ADR) {
#ifdef USE_MPI
  const int me = MPI::COMM_WORLD.Get_rank();

  const vector<int> &neighbouringCores = grid.neighbouringCores();

  std::map<int, vector<int>> particlesTo;
  std::map<int, vector<int>> particlesFrom;

  for (int core : neighbouringCores) {
    particlesTo[core] = vector<int>();
    particlesFrom[core] = vector<int>();
  }
#if DEBUG_MPI_PRINT
  string test = to_string(me) + "->";
  for (int core : neighbouringCores) {
    test += " " + to_string(core);
  }
#endif
#endif
  mat &r = particles.r();
  mat &r0 = particles.r0();
  ivec &colToId = particles.colToId();
  ivec &idToCol = particles.getIdToCol_v();
  unordered_map<int, GridPoint> &gridpoints = grid.gridpoints();

  double r_i[3] = {0, 0, 0};

  size_t nParticles = particles.nParticles();
  for (unsigned int i = 0; i < nParticles; i++) {
    const int id_i = colToId.at(i);
    r_i[0] = r(i, 0);
    r_i[1] = r(i, 1);
    r_i[2] = r(i, 2);
    const int gId = grid.gridId(r_i);
#ifdef USE_MPI
    const int belongsTo = grid.belongsTo(gId);
    if (belongsTo != me) {
      particlesTo.at(belongsTo).push_back(id_i);
    }

    // Adding to periodic boundary points for later removal
    GridPoint &gridPoint = gridpoints.at(gId);
    if (gridPoint.isGhost() && belongsTo == me) {
      gridPoint.addParticle(pair<int, int>(id_i, i));
    }
#else
    const pair<int, int> id_pos(id_i, i);
    gridpoints[gId]->addParticle(id_pos);
#endif
  }
#ifdef USE_MPI
  // Checking periodic boundaries
  const int dim = grid.dim();
  const vector<int> boundaryGridPoints = grid.periodicReceiveGridIds();

  for (int gId : boundaryGridPoints) {
    const GridPoint &gridPoint = gridpoints.at(gId);
    const int belongsTo = gridPoint.periodicNeighbourRank();
    const vector<pair<int, int>> l_particles = gridPoint.particles();
    const vector<double> &shift = gridPoint.periodicShift();

    for (const auto &idCol : l_particles) {
      const int id_i = idCol.first;
      const int i = idCol.second;
      for (int d = 0; d < dim; d++) {
        r(i, d) += shift[d];
        r0(i, d) += shift[d];
      }
      particlesTo.at(belongsTo).push_back(id_i);
    }
  }

  // Sending and receiving data
  vector<int> parameterIds;
  const int nPdParameters = particles.PdParameters().size();
  for (const auto &string_id : particles.parameters()) {
    parameterIds.push_back(string_id.second);
  }
  nParticles = particles.nParticles();
  const int nVerletLists = particles.getVerletSize();
  mat &v = particles.v();
  mat &F = particles.F();
  mat &Fold = particles.Fold();
  mat &r_prev = particles.r_prev();
  vec &stableMass = particles.stableMass();
  ivec &isStatic = particles.isStatic();
  mat &data = particles.data();

  vector<int> gotParticles;

  for (const pair<int, vector<int>> &coreParticles : particlesTo) {
    const int toCore = coreParticles.first;
    const vector<int> sParticles = coreParticles.second;

    vector<double> sendData;

    // Sending data
    for (const int id : sParticles) {
      const int i = idToCol[id];
      const auto &pd_connections = particles.pdConnections(id);

      sendData.push_back(id);
      for (int d = 0; d < M_DIM; d++) {
        sendData.push_back(r(i, d));
      }
      for (int d = 0; d < M_DIM; d++) {
        sendData.push_back(r0(i, d));
      }
      for (int d = 0; d < M_DIM; d++) {
        sendData.push_back(v(i, d));
      }
      for (int d = 0; d < M_DIM; d++) {
        sendData.push_back(F(i, d));
      }
      for (const int p : parameterIds) {
        sendData.push_back(data(i, p));
      }
      if (ADR) {
        for (int d = 0; d < M_DIM; d++) {
          sendData.push_back(Fold(i, d));
        }
        for (int d = 0; d < M_DIM; d++) {
          sendData.push_back(r_prev(i, d));
        }
        sendData.push_back(stableMass(i));
      }

      sendData.push_back(isStatic(i));
      sendData.push_back(pd_connections.size());

      for (const auto &con : pd_connections) {
        sendData.push_back(con.first);

        for (const double &param : con.second) {
          sendData.push_back(param);
        }
      }

      // Verlet lists
      for (int verletId = 0; verletId < nVerletLists; verletId++) {
        const vector<int> &verletlist = particles.verletList(id, verletId);
        sendData.push_back(verletlist.size());
        for (int i : verletlist) {
          sendData.push_back(i);
        }
      }
    }

    // Sending and receiving data
    int nSendElements = sendData.size();
    int nRecieveElements;
    MPI_Status status;

#if DEBUG_MPI_PRINT
    cout << me << "->" << toCore << " sending " << nSendElements << endl;
#endif
    MPI_Sendrecv(&nSendElements, 1, MPI_INT, toCore, 0, &nRecieveElements, 1,
                 MPI_INT, toCore, 0, MPI_COMM_WORLD, &status);
#if DEBUG_MPI_PRINT
    cout << me << "<-" << toCore << " recv " << nRecieveElements << endl;
#endif
    double recieveData[nRecieveElements];

    MPI_Sendrecv(&sendData[0], nSendElements, MPI_DOUBLE, toCore, 1,
                 &recieveData, nRecieveElements, MPI_DOUBLE, toCore, 1,
                 MPI_COMM_WORLD, &status);

    // Storing the received particle data
    int j = 0;
    while (j < nRecieveElements) {
      const int i = nParticles;
      const int id = recieveData[j++];
      idToCol[id] = i;
      colToId[i] = id;

      for (int d = 0; d < M_DIM; d++) {
        r(i, d) = recieveData[j++];
      }
      for (int d = 0; d < M_DIM; d++) {
        r0(i, d) = recieveData[j++];
      }
      for (int d = 0; d < M_DIM; d++) {
        v(i, d) = recieveData[j++];
      }
      for (int d = 0; d < M_DIM; d++) {
        F(i, d) = recieveData[j++];
      }
      for (const int p : parameterIds) {
        data(i, p) = recieveData[j++];
      }
      if (ADR) {
        for (int d = 0; d < M_DIM; d++) {
          Fold(i, d) = recieveData[j++];
        }
        for (int d = 0; d < M_DIM; d++) {
          r_prev(i, d) = recieveData[j++];
        }
        stableMass(i) = recieveData[j++];
      }
      isStatic(i) = recieveData[j++];
      const int nPdConnections = recieveData[j++];
      vector<pair<int, vector<double>>> connectionsVector;

      for (int i = 0; i < nPdConnections; i++) {
        const int con_id = (int)recieveData[j++];
        vector<double> connectionData;

        for (int k = 0; k < nPdParameters; k++) {
          connectionData.push_back(recieveData[j++]);
        }

        connectionsVector.push_back(
            pair<int, vector<double>>(con_id, connectionData));
      }
      // Verlet lists
      for (int verletId = 0; verletId < nVerletLists; verletId++) {
        vector<int> verletlist;
        int nVerletElements = recieveData[j++];
        for (int k = 0; k < nVerletElements; k++) {
          verletlist.push_back(recieveData[j++]);
        }
        particles.setVerletList(id, verletlist, verletId);
      }

      particles.setPdConnections(id, connectionsVector);
      nParticles++;
      particlesFrom[toCore].push_back(id);
      gotParticles.push_back(id);
    }
  }

  particles.nParticles(nParticles);

  // Deleting sent particles
  for (const pair<int, vector<int>> &coreParticles : particlesTo) {
    const vector<int> sParticles = coreParticles.second;
    for (const int id : sParticles) {
      particles.deleteParticleById(id);
    }
  }

  particles.sendtParticles(particlesTo);
  particles.receivedParticles(particlesFrom);

  for (unsigned int i = 0; i < nParticles; i++) {
    const int id = colToId.at(i);
    r_i[0] = r(i, 0);
    r_i[1] = r(i, 1);
    r_i[2] = r(i, 2);
    const int gId = grid.gridId(r_i);
    const pair<int, int> id_pos(id, i);
    const int belongsTo = grid.belongsTo(gId);

    if (belongsTo == me) {
      GridPoint &gp = gridpoints[gId];
      gp.addParticle(id_pos);
    } else {
      cerr << me << " DOES NOT BELONG TO ME: " << id << endl;
      cerr << r_i << endl;
      exit(1);
    }
  }
#endif
}
//------------------------------------------------------------------------------
void updateModifierLists(Modifier &modifier, PD_Particles &particles,
                         int counter) {
#ifdef USE_MPI
  const int me = MPI::COMM_WORLD.Get_rank();
  const map<int, vector<int>> &sendtParticles = particles.sendtParticles();

  for (const pair<int, vector<int>> &coreParticles : sendtParticles) {
    const int core = coreParticles.first;
    const vector<int> sParticles = coreParticles.second;

    vector<int> sendData;

    for (const int id : sParticles) {
      const bool removed = modifier.removeFromList(id);
      if (removed) {
        sendData.push_back(id);
      }
    }

    int nSendElements = sendData.size();
    int nRecieveElements;
    MPI_Status status;
    MPI_Sendrecv(&nSendElements, 1, MPI_INT, core, me * 1000 + counter,
                 &nRecieveElements, 1, MPI_INT, core, 1000 * core + counter,
                 MPI_COMM_WORLD, &status);

    int toBeAdded[nRecieveElements];

    MPI_Sendrecv(&sendData[0], nSendElements, MPI_INT, core,
                 me * 1200 + counter, &toBeAdded, nRecieveElements, MPI_INT,
                 core, 1200 * core + counter, MPI_COMM_WORLD, &status);

    if (nRecieveElements > 0) {
      for (const int id : toBeAdded) {
        modifier.addToList(id);
      }
    }
  }
#endif
}
//------------------------------------------------------------------------------
void exchangeInitialPeriodicBoundaryParticles(Grid &grid,
                                              PD_Particles &particles) {
#ifdef USE_MPI
  const int myRank = MPI::COMM_WORLD.Get_rank();

  // Collecting the boundary particles
  map<int, vector<pair<int, int>>> toRanks;
  const vector<int> boundaryGridPoints = grid.periodicSendGridIds();
  unordered_map<int, GridPoint> &gridpoints = grid.gridpoints();

  for (int gId : boundaryGridPoints) {
    const GridPoint &gridPoint = gridpoints.at(gId);
    const int nRank = gridPoint.periodicNeighbourRank();
    const vector<pair<int, int>> l_particles = gridPoint.particles();
    vector<pair<int, int>> &l_p = toRanks[nRank];
    l_p.insert(l_p.end(), l_particles.begin(), l_particles.end());
  }

  const vector<int> &ghostParameters = particles.ghostParameters();
  const int nPdParameters = particles.PdParameters().size();
  ivec &idToCol_v = particles.getIdToCol_v();
  ivec &colToId = particles.colToId();
  mat &r = particles.r();
  mat &r0 = particles.r0();
  mat &data = particles.data();

  // Sending the ghost particles to the other cpus
  const int nParticles = particles.nParticles();
  int nGhostParticles = particles.nGhostParticles();

  for (const auto &id_toRank : toRanks) {
    const int toRank = id_toRank.first;
    const vector<pair<int, int>> &l_p = id_toRank.second;
    vector<double> sendData;

    double r_i[3] = {0, 0, 0};
    for (const auto &id_col : l_p) {
      const int id_i = id_col.first;
      const int i = id_col.second;

      // Getting the shift
      r_i[0] = r(i, 0);
      r_i[1] = r(i, 1);
      r_i[2] = r(i, 2);
      const int gId = grid.gridId(r_i);
      const GridPoint &gridPoint = gridpoints.at(gId);
      const vector<double> &shift = gridPoint.periodicShift();
      const auto &pd_connections = particles.pdConnections(id_i);

      // Collecting send data
      sendData.push_back(id_i);
      for (int d = 0; d < M_DIM; d++) {
        sendData.push_back(r_i[d] + shift[d]);
      }
      for (int d = 0; d < M_DIM; d++) {
        sendData.push_back(r0(i, d) + shift[d]);
      }
      for (const int j : ghostParameters) {
        sendData.push_back(data(i, j));
      }

      sendData.push_back(pd_connections.size());
      for (const auto &con : pd_connections) {
        sendData.push_back(con.first);

        for (const double &param : con.second) {
          sendData.push_back(param);
        }
      }
    }

    // Sending and receiving data
    int nSendElements = sendData.size();
    int nRecieveElements;
    MPI_Status status;

    MPI_Sendrecv(&nSendElements, 1, MPI_INT, toRank, myRank * 100,
                 &nRecieveElements, 1, MPI_INT, toRank, toRank * 100,
                 MPI_COMM_WORLD, &status);

    double recieveData[nRecieveElements];

    MPI_Sendrecv(&sendData[0], nSendElements, MPI_DOUBLE, toRank, myRank,
                 &recieveData, nRecieveElements, MPI_DOUBLE, toRank, toRank,
                 MPI_COMM_WORLD, &status);

    // Storing the received ghost data
    int j = 0;
    double l_r[3] = {0, 0, 0};
    while (j < nRecieveElements) {
      const int col = nParticles + nGhostParticles;
      const int id = recieveData[j++];
      idToCol_v[id] = col;
      colToId[col] = id;

      for (int d = 0; d < M_DIM; d++) {
        r(col, d) = recieveData[j++];
      }
      for (int d = 0; d < M_DIM; d++) {
        r0(col, d) = recieveData[j++];
      }
      for (const int g : ghostParameters) {
        data(col, g) = recieveData[j++];
      }

      const int nPdConnections = recieveData[j++];
      vector<pair<int, vector<double>>> connectionsVector;

      for (int i = 0; i < nPdConnections; i++) {
        const int con_id = (int)recieveData[j++];
        vector<double> connectionData;

        for (int k = 0; k < nPdParameters; k++) {
          connectionData.push_back(recieveData[j++]);
        }

        connectionsVector.push_back(
            pair<int, vector<double>>(con_id, connectionData));
      }
      particles.setPdConnections(id, connectionsVector);
      nGhostParticles++;

      // Adding to the ghost particles to the correct grid point
      l_r[0] = r(col, 0);
      l_r[1] = r(col, 1);
      l_r[2] = r(col, 2);
      const int gId = grid.gridId(l_r);
      const pair<int, int> id_pos(id, col);

      GridPoint &gp = gridpoints[gId];
      gp.addParticle(id_pos);
    }
  }
  particles.nGhostParticles(nGhostParticles);
#endif
}
//------------------------------------------------------------------------------
void exchangePeriodicBoundaryParticles(Grid &grid, PD_Particles &particles) {
#ifdef USE_MPI
  const int myRank = MPI::COMM_WORLD.Get_rank();

  // Collecting the boundary particles
  map<int, vector<pair<int, int>>> toRanks;
  const vector<int> boundaryGridPoints = grid.periodicSendGridIds();
  unordered_map<int, GridPoint> &gridpoints = grid.gridpoints();

  for (int gId : boundaryGridPoints) {
    const GridPoint &gridPoint = gridpoints.at(gId);
    const int nRank = gridPoint.periodicNeighbourRank();
    const vector<pair<int, int>> l_particles = gridPoint.particles();
    vector<pair<int, int>> &l_p = toRanks[nRank];
    l_p.insert(l_p.end(), l_particles.begin(), l_particles.end());
  }

  const vector<int> &ghostParameters = particles.ghostParameters();
  const int needVelocity = particles.needGhostVelocity();
  const int needR0 = particles.getNeedGhostR0();
//  const int nPdParameters = particles.PdParameters().size();
  ivec & idToCol = particles.getIdToCol_v();
  ivec &colToId = particles.colToId();
  mat &r = particles.r();
  mat &r0 = particles.r0();
  mat &v = particles.v();
  mat &data = particles.data();

  // Sending the ghost particles to the other cpus
  const int nParticles = particles.nParticles();
  int nGhostParticles = particles.nGhostParticles();
  //    MPI_Barrier(MPI_COMM_WORLD);

  for (const auto &id_toRank : toRanks) {
    const int toRank = id_toRank.first;
    const vector<pair<int, int>> &l_p = id_toRank.second;
    vector<double> sendData;

    for (const auto &id_col : l_p) {
      const int id_i = id_col.first;
      const int i = id_col.second;

      // Getting the shift
      const vec3 &r_i = r.row(i).t();
      const int gId = grid.gridId(r_i);
      const GridPoint &gridPoint = gridpoints.at(gId);
      const vector<double> &shift = gridPoint.periodicShift();

      const vec3 &r0_i = r0.row(i).t();
      const int gId0 = grid.gridId(r0_i);
      const GridPoint &gridPoint0 = gridpoints.at(gId0);
      const vector<double> &shift0 = gridPoint0.periodicShift();

      // Collecting send data
      sendData.push_back(id_i);
      for (int d = 0; d < M_DIM; d++) {
        sendData.push_back(r(i, d) + shift[d]);
      }
      if (needVelocity) {
        for (int d = 0; d < M_DIM; d++) {
          sendData.push_back(v(i, d));
        }
      }
      if (needR0) {
        for (int d = 0; d < M_DIM; d++) {
          sendData.push_back(r0(i, d) + shift0[d]);
        }
      }
      for (const int j : ghostParameters) {
        sendData.push_back(data(i, j));
      }
    }

    // Sending and receiving data
    int nSendElements = sendData.size();
    int nRecieveElements;
    MPI_Status status;

    MPI_Sendrecv(&nSendElements, 1, MPI_INT, toRank, myRank * 100,
                 &nRecieveElements, 1, MPI_INT, toRank, toRank * 100,
                 MPI_COMM_WORLD, &status);

    double recieveData[nRecieveElements];

    MPI_Sendrecv(&sendData[0], nSendElements, MPI_DOUBLE, toRank, myRank,
                 &recieveData, nRecieveElements, MPI_DOUBLE, toRank, toRank,
                 MPI_COMM_WORLD, &status);

    // Storing the received ghost data
    int j = 0;
    while (j < nRecieveElements) {
      const int col = nParticles + nGhostParticles;
      const int id = recieveData[j++];

      idToCol[id] = col;
      colToId[col] = id;
      for (int d = 0; d < M_DIM; d++) {
        r(col, d) = recieveData[j++];
      }
      if (needVelocity) {
        for (int d = 0; d < M_DIM; d++) {
          v(col, d) = recieveData[j++];
        }
      }
      if (needR0) {
        for (int d = 0; d < M_DIM; d++) {
          r0(col, d) = recieveData[j++];
        }
      }
      for (const int g : ghostParameters) {
        data(col, g) = recieveData[j++];
      }

      nGhostParticles++;

      // Adding to the ghost particles to the correct grid point
      const vec3 &l_r = r.row(col).t();
      const int gId = grid.gridId(l_r);
      const pair<int, int> id_pos(id, col);

      GridPoint &gp = gridpoints[gId];
      gp.addParticle(id_pos);
    }
  }
  particles.nGhostParticles(nGhostParticles);
#endif
}
//------------------------------------------------------------------------------
void exchangeInitialGhostParticles(Grid &grid, PD_Particles &particles) {
  particles.nGhostParticles(0);
  exchangeInitialPeriodicBoundaryParticles(grid, particles);
  exchangeInitialGhostParticles_boundary(grid, particles);
}
//------------------------------------------------------------------------------
void exchangeGhostParticles(Grid &grid, PD_Particles &particles) {
  particles.nGhostParticles(0);
  exchangePeriodicBoundaryParticles(grid, particles);
  exchangeGhostParticles_boundary(grid, particles);
}
//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------
