TEMPLATE = lib
TARGET = PDtools
CONFIG += staticlib
CONFIG   -= app_bundle
CONFIG   -= qt

include(../../defaults.pri)

HEADERS += \
    PDtools.h \
    Particles/particles.h \
    Particles/pd_particles.h \
    Grid/grid.h \
    Domain/domain.h \
    Solver/solver.h \
    data/pdshareddata.h \
    Particles/saveparticles.h \
    Particles/loadparticles.h \
    Particles/loadpdparticles.h \
    PdFunctions/pdfunctions.h \
    Solver/timeintegrator.h \
    Solver/TimeIntegrators/velocityverletintegrator.h \
    Solver/solvers.h \
    Force/force.h \
    Force/forces.h \
    Force/PdForces/pd_bondforce.h \
    Modfiers/modifier.h \
    Modfiers/Implementation/BoundaryConditions/velocityboundary.h \
    Modfiers/Implementation/FractureCriterion/pmbfracture.h \
    SavePdData/savepddata.h \
    SavePdData/Implementations/computedamage.h \
    SavePdData/Implementations/computekineticenergy.h \
    SavePdData/Implementations/computepotentialenergy.h \
    Force/PdForces/contactforce.h \
    Solver/adr.h \
    Modfiers/Implementation/BoundaryConditions/moveparticles.h \
    Modfiers/modifiers.h \
    Modfiers/Implementation/FractureCriterion/adrfracture.h \
    SavePdData/Implementations/computemaxstretch.h \
    Modfiers/Implementation/FractureCriterion/mohrcoulombfracture.h \
    SavePdData/Implementations/computeaveragestretch.h \
    Modfiers/Implementation/FractureCriterion/adrfractureaverage.h \
    Modfiers/Implementation/FractureCriterion/adrmohrcoulombfracture.h \
    Force/PdForces/pd_bondforcegaussian.h \
    Force/PdForces/pd_pmb.h \
    Force/PdForces/pd_osp.h \
    Modfiers/Implementation/BoundaryConditions/boundaryforce.h \
    Modfiers/Implementation/FractureCriterion/bondenergyfracture.h \
    Modfiers/Implementation/FractureCriterion/simplefracture.h \
    Force/PdForces/LPS/pd_lps.h \
    Solver/ADRsolvers/dynamicadr.h \
    Solver/staticsolver.h \
    Solver/TimeIntegrators/eulercromerintegrator.h \
    PdFunctions/pdfunctionsmpi.h \
    SavePdData/Implementations/computegridid.h \
    Force/PdForces/viscousdamper.h \
    CalculateProperties/calculateproperty.h \
    CalculateProperties/Implementation/calculatepdangles.h \
    CalculateProperties/calculateproperties.h \
    Force/DemForces/demforce.h \
    CalculateProperties/Implementation/calculatestress.h \
    Modfiers/Implementation/FractureCriterion/mohrcoulombbondfracture.h \
    Modfiers/Implementation/rigidwall.h \
    Modfiers/Implementation/FractureCriterion/adrmohrcoulombbondfracture.h \
    Modfiers/Implementation/FractureCriterion/mohrcoulombmax.h \
    Modfiers/Implementation/FractureCriterion/mohrcoulombmaxfracture.h \
    Modfiers/Implementation/FractureCriterion/mohrcoulombweightedaverage.h \
    Force/PdForces/pd_dampenedbondforce.h \
    Modfiers/Implementation/dumpstate.h \
    Modfiers/Implementation/FractureCriterion/mohrcoulombnodesplit.h \
    Modfiers/Implementation/FractureCriterion/mohrcoulommaxconnected.h \
    CalculateProperties/Implementation/calculatestrain.h \
    CalculateProperties/Implementation/calculatedamage.h \
    Modfiers/Implementation/FractureCriterion/mohrcoulombmaxfractureweighted.h \
    Modfiers/Implementation/FractureCriterion/strainfracture.h \
    Modfiers/Implementation/FractureCriterion/vonmisesfracture.h \
    CalculateProperties/Implementation/calculatestressstrain.h \
    Force/PdForces/LPS/pd_lpsdampenedcontact.h \
    Modfiers/Implementation/BoundaryConditions/strainboundary.h \
    Force/PdForces/LPS/lps_mc.h \
    Force/PdForces/LPS/pd_lps_adrmc.h \
    Force/PdForces/pd_nopd.h \
    Modfiers/Implementation/BoundaryConditions/moveparticleszone.h \
    Modfiers/Implementation/savestate.h \
    Modfiers/Implementation/BoundaryConditions/moveparticletype.h \
    Modfiers/Implementation/FractureCriterion/mohrcoulombmaxfractureweightedadr.h \
    Force/PdForces/pd_pmb_linear_integrator.h \
    Mesh/pdmesh.h \
    Mesh/loadmesh.h \
    Mesh/meshtopdpartices.h \
    Elements/pd_element.h \
    Utilities/gaussianquadrature.h \
    Utilities/geometryfunctions.h \
    PdFunctions/pdelementfunctions.h \
    Utilities/boostgeometry_settings.h \
    Force/EPD/epd_lps.h \
    Force/EPD/epd_bondforce.h \
    Utilities/epd_functions.h \
    CalculateProperties/ImplementationEPD/calculatestressstrainepd.h \
    Force/PdForces/pd_bondforce_hourglass.h \
    Modfiers/Implementation/BoundaryConditions/boundarystress.h \
    Force/PdForces/LPS/pd_lps_crit_strain.h \
    Force/PdForces/LPS/pd_lps_adr_strain.h \
    Force/PdForces/LPS_porosity/lps_p_mc.h \
    Force/PdForces/LPS_porosity/pd_lps_p.h \
    Force/PdForces/LPS_porosity/pd_lps_p_adrmc.h \
    Force/PdForces/LPS_porosity/pd_lpsdampenedcontact_p.h \
    Force/PdForces/LPS/pd_lps_k.h \
    Force/PdForces/LPSS/pd_lpss.h \
    Force/PdForces/LPSS/pd_lpss_g.h \
    Force/PdForces/LPSS/pd_lpss_opt.h \
    Force/PdForces/LPS/pd_lps2.h

SOURCES += \
    Grid/grid.cpp \
    Domain/domain.cpp \
    Solver/solver.cpp \
    Solver/adr.cpp \
    Solver/ADRsolvers/dynamicadr.cpp \
    Solver/staticsolver.cpp \
    Solver/timeintegrator.cpp \
    Solver/TimeIntegrators/velocityverletintegrator.cpp \
    data/pdshareddata.cpp \
    Particles/saveparticles.cpp \
    Particles/loadparticles.cpp \
    Particles/loadpdparticles.cpp \
    PdFunctions/pdfunctions.cpp \
    PdFunctions/pdfunctionsmpi.cpp \
    Force/force.cpp \
    Force/PdForces/pd_bondforce.cpp \
    Modfiers/modifier.cpp \
    SavePdData/savepddata.cpp \
    SavePdData/Implementations/computedamage.cpp \
    SavePdData/Implementations/computekineticenergy.cpp \
    SavePdData/Implementations/computepotentialenergy.cpp \
    SavePdData/Implementations/computemaxstretch.cpp \
    SavePdData/Implementations/computeaveragestretch.cpp \
    Force/PdForces/contactforce.cpp \
    Particles/particles.cpp \
    Particles/pd_particles.cpp \
    Solver/TimeIntegrators/eulercromerintegrator.cpp \
    Force/PdForces/pd_bondforcegaussian.cpp \
    Force/PdForces/pd_pmb.cpp \
    Force/PdForces/pd_osp.cpp \
    Force/PdForces/LPS/pd_lps.cpp \
    Force/PdForces/LPS/pd_lpsdampenedcontact.cpp \
    Force/PdForces/viscousdamper.cpp \
    Force/PdForces/pd_dampenedbondforce.cpp \
    Force/DemForces/demforce.cpp \
    SavePdData/Implementations/computegridid.cpp \
    CalculateProperties/calculateproperty.cpp \
    CalculateProperties/Implementation/calculatepdangles.cpp \
    CalculateProperties/Implementation/calculatestress.cpp \
    CalculateProperties/Implementation/calculatestrain.cpp \
    CalculateProperties/Implementation/calculatedamage.cpp \
    CalculateProperties/Implementation/calculatestressstrain.cpp \
    Modfiers/Implementation/dumpstate.cpp \
    Modfiers/Implementation/rigidwall.cpp \
    Modfiers/Implementation/BoundaryConditions/boundaryforce.cpp \
    Modfiers/Implementation/BoundaryConditions/velocityboundary.cpp \
    Modfiers/Implementation/BoundaryConditions/moveparticles.cpp \
    Modfiers/Implementation/FractureCriterion/mohrcoulombfracture.cpp \
    Modfiers/Implementation/FractureCriterion/adrfracture.cpp \
    Modfiers/Implementation/FractureCriterion/pmbfracture.cpp \
    Modfiers/Implementation/FractureCriterion/bondenergyfracture.cpp \
    Modfiers/Implementation/FractureCriterion/simplefracture.cpp \
    Modfiers/Implementation/FractureCriterion/mohrcoulombbondfracture.cpp \
    Modfiers/Implementation/FractureCriterion/adrmohrcoulombbondfracture.cpp \
    Modfiers/Implementation/FractureCriterion/mohrcoulombmax.cpp \
    Modfiers/Implementation/FractureCriterion/mohrcoulombmaxfracture.cpp \
    Modfiers/Implementation/FractureCriterion/mohrcoulombweightedaverage.cpp \
    Modfiers/Implementation/FractureCriterion/mohrcoulombnodesplit.cpp \
    Modfiers/Implementation/FractureCriterion/mohrcoulommaxconnected.cpp \
    Modfiers/Implementation/FractureCriterion/mohrcoulombmaxfractureweighted.cpp \
    Modfiers/Implementation/FractureCriterion/strainfracture.cpp \
    Modfiers/Implementation/FractureCriterion/vonmisesfracture.cpp \
    Modfiers/Implementation/FractureCriterion/adrfractureaverage.cpp \
    Modfiers/Implementation/FractureCriterion/adrmohrcoulombfracture.cpp \
    Modfiers/Implementation/BoundaryConditions/strainboundary.cpp \
    Force/PdForces/LPS/lps_mc.cpp \
    Force/PdForces/LPS/pd_lps_adrmc.cpp \
    Force/PdForces/pd_nopd.cpp \
    Modfiers/Implementation/BoundaryConditions/moveparticleszone.cpp \
    Modfiers/Implementation/savestate.cpp \
    Modfiers/Implementation/BoundaryConditions/moveparticletype.cpp \
    Modfiers/Implementation/FractureCriterion/mohrcoulombmaxfractureweightedadr.cpp \
    Force/PdForces/pd_pmb_linear_integrator.cpp \
    Mesh/pdmesh.cpp \
    Mesh/loadmesh.cpp \
    Mesh/meshtopdpartices.cpp \
    Elements/pd_element.cpp \
    Utilities/gaussianquadrature.cpp \
    Utilities/geometryfunctions.cpp \
    PdFunctions/pdelementfunctions.cpp \
    Utilities/boostgeometry_settings.cpp \
    Force/EPD/epd_lps.cpp \
    Force/EPD/epd_bondforce.cpp \
    Utilities/epd_functions.cpp \
    CalculateProperties/ImplementationEPD/calculatestressstrainepd.cpp \
    Force/PdForces/pd_bondforce_hourglass.cpp \
    Modfiers/Implementation/BoundaryConditions/boundarystress.cpp \
    Force/PdForces/LPS/pd_lps_crit_strain.cpp \
    Force/PdForces/LPS/pd_lps_adr_strain.cpp \
    Force/PdForces/LPS_porosity/lps_p_mc.cpp \
    Force/PdForces/LPS_porosity/pd_lps_p_adrmc.cpp \
    Force/PdForces/LPS_porosity/pd_lps_p.cpp \
    Force/PdForces/LPS_porosity/pd_lpsdampenedcontact_p.cpp \
    Force/PdForces/LPS/pd_lps_k.cpp \
    Force/PdForces/LPSS/pd_lpss.cpp \
    Force/PdForces/LPSS/pd_lpss_g.cpp \
    Force/PdForces/LPSS/pd_lpss_opt.cpp \
    Force/PdForces/LPS/pd_lps2.cpp

include(deployment.pri)
