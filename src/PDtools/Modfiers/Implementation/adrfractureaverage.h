#ifndef ADRFRACTUREAVERAGE_H
#define ADRFRACTUREAVERAGE_H

#include <unordered_map>
#include "PDtools/Modfiers/modifier.h"

namespace PDtools
{
class Force;
//------------------------------------------------------------------------------
class ADRfractureAverage: public Modifier
{
public:
    ADRfractureAverage(double alpha);
    ~ADRfractureAverage();

    virtual void initialize();
    virtual void evaluateStepOne(const pair<int, int> &pIdcol);
    virtual void evaluateStepTwo(const pair<int, int> &pIdcol);

    virtual void evaluateStepTwo();

private:
    double m_alpha;
    pair<int, pair<int, vector<double>> *> m_maxPId;
    double m_maxStretch;
    int m_indexS0;
    int m_indexStretch;
    int m_indexUnbreakable;
    int m_indexS00;
    int m_indexS_avg;
    arma::mat * m_data;
    std::unordered_map<int, int> * m_pIds;
};
//------------------------------------------------------------------------------
}
#endif // ADRFRACTUREAVERAGE_H