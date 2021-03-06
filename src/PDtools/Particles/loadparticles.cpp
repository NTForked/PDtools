#include "loadparticles.h"

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <fstream>
#include <regex>
#include <stdexcept>

#include "particles.h"
#include "pd_particles.h"

namespace PDtools {
//------------------------------------------------------------------------------
LoadParticles::LoadParticles() {}
//------------------------------------------------------------------------------
LoadParticles::~LoadParticles() {}
//------------------------------------------------------------------------------
Particles LoadParticles::load(string loadPath, string format, bool bin,
                              unordered_map<string, int> loadParameters) {
  m_format = format;
  m_binary = bin;
  Particles particles;

  if (m_binary) {
    FILE *binaryData = fopen(loadPath.c_str(), "rb");

    if (!binaryData) {
      cerr << "ERROR: Could not open " << loadPath << endl;
    }

    if (m_format == "xyz") {
      cerr << "Binary xyz not implemented" << endl;
      throw 10;
    } else if (m_format == "ply") {
      loadParameters = read_plyBinaryHeader(binaryData, loadParameters);
    } else if (m_format == "lmp") {
      read_lmpBinaryHeader(binaryData, loadParameters);
    } else {
      cerr << "Format: '" << m_format << "' not supported" << endl;
      throw 10;
    }

    loadBinaryBody(particles, binaryData, loadParameters);
    fclose(binaryData);
  } else {
    std::fstream data(loadPath, std::ios::in);
    unordered_map<std::string, int> parameters;

    if (m_format == "xyz") {
      parameters = read_xyzHeader(data);
    } else if (m_format == "ply") {
      cerr << "ply not implemented" << endl;
      throw 10;
    } else if (m_format == "lmp") {
      parameters = read_lmpHeader(data);
    } else {
      cerr << "Format: '" << m_format << "' not supported" << endl;
      throw 10;
    }

    loadBody(particles, data, parameters);
    data.close();
  }

  return particles;
}
//------------------------------------------------------------------------------

unordered_map<std::string, int> LoadParticles::read_xyzHeader(std::fstream &data) {
  string line;

  // Reading the number of particles
  std::getline(data, line);

  m_nParticles = stoi(line);

  // Reading the comments
  // This program follows a convention that the comments
  // in a xyz-file must name the variables.
  std::getline(data, line);
  boost::regex rr("([A-Za-z_]+)");
  boost::sregex_iterator next(line.begin(), line.end(), rr);
  boost::sregex_iterator end;

  unordered_map<std::string, int> parameters;

  int position = 0;
  while (next != end) {
    boost::smatch match = *next;
    parameters[match.str()] = position;
    position++;
    next++;
  }

  return parameters;
}
//------------------------------------------------------------------------------

unordered_map<std::string, int> LoadParticles::read_lmpHeader(std::fstream &data) {
  string line;

  // Skipping the timestep
  std::getline(data, line);
  std::getline(data, line);

  // Reading the number of particles
  std::getline(data, line);
  std::getline(data, line);
  m_nParticles = stoi(line);

  // Ignoring all other data exepct data parameters
  const int maxHeadersteps = 100;
  unordered_map<std::string, int> parameters;
  bool notFound = true;

  for (int n = 0; n < maxHeadersteps; n++) {
    std::getline(data, line);
    string start = line.substr(0, 12);
    if (boost::iequals(start, "ITEM: ATOMS ")) {
      line = line.substr(12, -1); // Removing 'ITEM: ATOMS '

      // Reading the variables
      boost::regex rr("([A-Za-z_]+)");
      boost::sregex_iterator next(line.begin(), line.end(), rr);
      boost::sregex_iterator end;

      int position = 0;
      while (next != end) {
        boost::smatch match = *next;
        parameters[match.str()] = position;
        position++;
        next++;
      }
      notFound = false;
      break;
    }
  }

  if (notFound) {
    cerr << "Error reading LAMMPS header file" << endl;
    exit(EXIT_FAILURE);
  }

  return parameters;
}
//------------------------------------------------------------------------------

unordered_map<string, int>
LoadParticles::read_lmpBinaryHeader(FILE *data,
                                    unordered_map<string, int> parameters) {
  (void)parameters;
  int timeStep;
  int nParticles;
  int triclinic = false;
  int nChunks;
  int nColumns;
  int chunkLength;
  int boundary[3][2];
  double boundaryCoordinates[3][2];
  double shear[3];

  if (m_useLegacyFormat) {
    fread(&timeStep, sizeof(int), 1, data);
    fread(&nParticles, sizeof(int), 1, data);
  } else {
    long long int tmp_currentTimeStep, tmp_nParticles;
    fread(&tmp_currentTimeStep, sizeof(long long int), 1, data);
    fread(&tmp_nParticles, sizeof(long long int), 1, data);
    timeStep = tmp_currentTimeStep;
    nParticles = tmp_nParticles;
    fread(&triclinic, sizeof(int), 1, data);
    fread(&boundary[0][0], 6 * sizeof(int), 1, data);
  }
  fread(&boundaryCoordinates[0][0], 6 * sizeof(double), 1, data);

  if (m_useLegacyFormat) {
    fread(&shear[0], 3 * sizeof(double), 1, data);
  }

  if (triclinic) {
    cerr << "ERROR: triclinic not supported in LAMMPS binary file" << endl;
    int dump[3];
    fread(&dump[0], 3 * sizeof(int), 1, data);
  }

  fread(&nColumns, sizeof(int), 1, data);
  fread(&nChunks, sizeof(int), 1, data);
  fread(&chunkLength, sizeof(int), 1, data);

  m_timeStep = timeStep;
  m_nParticles = nParticles;
  m_nColumns = nColumns;
  m_chunkLength = chunkLength;

  unordered_map<string, int> empty;

  return empty;
}
//------------------------------------------------------------------------------

unordered_map<string, int>
LoadParticles::read_plyBinaryHeader(FILE *data,
                                    unordered_map<string, int> parameters) {
  parameters = {};
  string line = readBinaryLine(data);

  if (line != "ply") {
    cerr << "Error: loading corrupt ply-file:";
    throw 20;
  }

  line = readBinaryLine(data);
  if (line != "format binary_little_endian 1.0") {
    cerr << "Error: error loading binary ply-file:";
    throw 20;
  }

  int position = 0;
  int nParticles = 0;
  while (line != "end_header") {
    line = readBinaryLine(data);
    vector<string> lineSplit;
    boost::trim_if(line, boost::is_any_of("\t "));
    boost::split(lineSplit, line, boost::is_any_of("\t "),
                 boost::token_compress_on);

    if (lineSplit[0] == "comment")
      continue;

    if (lineSplit[0] == "element") {
      if (lineSplit[1] == "vertex") {
        nParticles = stoi(lineSplit[2]);
      } else {
        cerr << "Error: only vertex element supported in ply file" << endl;
        cerr << "found: " << lineSplit[1] << endl;
        throw 20;
      }
    }

    if (lineSplit[0] == "property") {
      if (lineSplit[1] == "double") {
        parameters[lineSplit[2]] = position;
        position++;
      } else {
        cerr << "Error: in ply 'property'' only 'double' is supported" << endl;
        cerr << "found: " << lineSplit[1] << endl;
        throw 20;
      }
    }
  }

  m_timeStep = 0;
  m_nColumns = parameters.size();
  m_nParticles = nParticles;
  m_chunkLength = nParticles * parameters.size();

  return parameters;
}
//------------------------------------------------------------------------------

void LoadParticles::loadBody(Particles &particles, std::fstream &rawData,
                             unordered_map<string, int> parameters) {
  particles.maxParticles(m_nParticles);
  particles.nParticles(m_nParticles);
  particles.totParticles(m_nParticles);
  string line;

  //--------------------------------------------------------------------------
  // Storing only non-basic parameters in the parameters
  int counter = 0;
  vector<pair<int, int>> data_config_mapping;
  for (auto param : parameters) {
    bool found = false;

    for (string basic_parameter : basicParameters) {
      if (param.first == basic_parameter) {
        found = true;
      }
    }

    if (!found) {
      particles.parameters()[param.first] = counter;
      data_config_mapping.push_back(pair<int, int>(counter, param.second));
      counter++;
    }
  }

  //--------------------------------------------------------------------------
  bool idIsset = false;
  int idPos = 0;
  if (parameters.count("id") > 0) {
    idIsset = true;
    idPos = parameters["id"];
  }

  vector<pair<int, int>> position_config;
  int dim = 0;
  if (parameters.count("x") > 0) {
    dim++;
    position_config.push_back(pair<int, int>(0, parameters["x"]));
  }
  if (parameters.count("y") > 0) {
    dim++;
    position_config.push_back(pair<int, int>(1, parameters["y"]));
  }
  if (parameters.count("z") > 0) {
    dim++;
    position_config.push_back(pair<int, int>(2, parameters["z"]));
  }
  particles.dim(dim);

  //--------------------------------------------------------------------------
  // Creating the data matrix
  particles.initializeMatrices();
  ivec &idToCol = particles.getIdToCol_v();
  arma::ivec &colToId = particles.colToId();
  arma::mat &r = particles.r();
  arma::mat &data = particles.data();

  // Reading all the data from file
  for (unsigned int i = 0; i < particles.nParticles(); i++) {
    vector<string> lineSplit;
    getline(rawData, line);
    boost::trim_if(line, boost::is_any_of("\t "));
    boost::split(lineSplit, line, boost::is_any_of("\t "),
                 boost::token_compress_on);

    // Collecting the data
    if (idIsset) {
      idToCol[stoi(lineSplit[idPos])] = i;
      colToId[i] = stoi(lineSplit[idPos]);
    } else {
      idToCol[i] = i;
      colToId[i] = i;
    }

    for (pair<int, int> pc : position_config) {
      r(i, pc.first) = stod(lineSplit[pc.second]);
    }
    for (pair<int, int> dfc : data_config_mapping) {
      data(i, dfc.first) = stod(lineSplit[dfc.second]);
    }
  }
}
//------------------------------------------------------------------------------

void LoadParticles::loadBinaryBody(Particles &particles, FILE *rawData,
                                   unordered_map<string, int> parameters) {
  particles.maxParticles(m_nParticles);
  particles.nParticles(m_nParticles);
  particles.totParticles(m_nParticles);
  // Storing only non-basic parameters in the parameters
  int counter = 0;
  vector<pair<int, int>> data_config_mapping;
  for (auto param : parameters) {
    bool found = false;
    for (string basic_parameter : basicParameters) {
      if (param.first == basic_parameter) {
        found = true;
      }
    }

    if (!found) {
      particles.parameters()[param.first] = counter;
      data_config_mapping.push_back(pair<int, int>(counter, param.second));
      counter++;
    }
  }

  //--------------------------------------------------------------------------
  bool idIsset = false;
  int idPos = 0;
  if (parameters.count("id") > 0) {
    idIsset = true;
    idPos = parameters["id"];
  }

  vector<pair<int, int>> position_config;
  int dim = 0;
  if (parameters.count("x") > 0) {
    dim++;
    position_config.push_back(pair<int, int>(0, parameters["x"]));
  }
  if (parameters.count("y") > 0) {
    dim++;
    position_config.push_back(pair<int, int>(1, parameters["y"]));
  }
  if (parameters.count("z") > 0) {
    dim++;
    position_config.push_back(pair<int, int>(2, parameters["z"]));
  }
  particles.dim(dim);

  //--------------------------------------------------------------------------
  // Creating the data matrix
  particles.initializeMatrices();

  int nColumns = m_nColumns;
  ivec &idToCol = particles.getIdToCol_v();
  arma::ivec &colToId = particles.colToId();
  arma::mat &r = particles.r();
  arma::mat &data = particles.data();

  // Reading all the data from file
  for (unsigned int i = 0; i < particles.nParticles(); i++) {
    double line[nColumns];
    fread(&line[0], nColumns * sizeof(double), 1, rawData);

    // Collecting the data
    if (idIsset) {
      idToCol[int(line[idPos])] = i;
      colToId[i] = int(line[idPos]);
    } else {
      idToCol[i] = i;
      colToId[i] = i;
    }

    for (pair<int, int> pc : position_config) {
      r(i, pc.first) = line[pc.second];
    }
    for (pair<int, int> dfc : data_config_mapping) {
      data(i, dfc.first) = line[dfc.second];
    }
  }
}
//------------------------------------------------------------------------------
}
//------------------------------------------------------------------------------
