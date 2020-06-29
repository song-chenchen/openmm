
/* Portions copyright (c) 2006 Stanford University and Simbios.
 * Contributors: Pande Group
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS, CONTRIBUTORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "AmoebaReferenceForce.h"
#include "AmoebaReferenceVdwForce.h"
#include "ReferenceForce.h"
#include <algorithm>
#include <cctype>
#include <cmath>

using std::vector;
using namespace OpenMM;

AmoebaReferenceVdwForce::AmoebaReferenceVdwForce() : _nonbondedMethod(NoCutoff), _cutoff(1.0e+10), _taperCutoffFactor(0.9), _n(5), _alpha(0.7), _alchemicalMethod(None) {

    setTaperCoefficients(_cutoff);
    setSigmaCombiningRule("ARITHMETIC");
    setEpsilonCombiningRule("GEOMETRIC");
}


AmoebaReferenceVdwForce::AmoebaReferenceVdwForce(const std::string& sigmaCombiningRule, const std::string& epsilonCombiningRule) : _nonbondedMethod(NoCutoff), _cutoff(1.0e+10), _taperCutoffFactor(0.9),  _n(5), _alpha(0.7), _alchemicalMethod(None) {

    setTaperCoefficients(_cutoff);
    setSigmaCombiningRule(sigmaCombiningRule);
    setEpsilonCombiningRule(epsilonCombiningRule);
}

AmoebaReferenceVdwForce::NonbondedMethod AmoebaReferenceVdwForce::getNonbondedMethod() const {
    return _nonbondedMethod;
}

void AmoebaReferenceVdwForce::setNonbondedMethod(AmoebaReferenceVdwForce::NonbondedMethod nonbondedMethod) {
    _nonbondedMethod = nonbondedMethod;
}

AmoebaReferenceVdwForce::AlchemicalMethod AmoebaReferenceVdwForce::getAlchemicalMethod() const {
    return _alchemicalMethod;
}


void AmoebaReferenceVdwForce::setAlchemicalMethod(AmoebaReferenceVdwForce::AlchemicalMethod alchemicalMethod){
    _alchemicalMethod = alchemicalMethod;
}

void AmoebaReferenceVdwForce::setSoftcorePower(int n) {
    _n = n;
}

int AmoebaReferenceVdwForce::getSoftcorePower() const {
    return _n;
}

void AmoebaReferenceVdwForce::setSoftcoreAlpha(double alpha) {
    _alpha = alpha;
}

double AmoebaReferenceVdwForce::getSoftcoreAlpha() const {
    return _alpha;
}


void AmoebaReferenceVdwForce::setTaperCoefficients(double cutoff) {
    _taperCutoff = cutoff*_taperCutoffFactor;
    if (_taperCutoff != cutoff) {
        _taperCoefficients[C3] = 10.0/pow(_taperCutoff - cutoff, 3.0);
        _taperCoefficients[C4] = 15.0/pow(_taperCutoff - cutoff, 4.0);
        _taperCoefficients[C5] =  6.0/pow(_taperCutoff - cutoff, 5.0);
    } else {
        _taperCoefficients[C3] = 0.0;
        _taperCoefficients[C4] = 0.0;
        _taperCoefficients[C5] = 0.0;
    }
}

void AmoebaReferenceVdwForce::setCutoff(double cutoff) {
    _cutoff  = cutoff;
    setTaperCoefficients(_cutoff);
}

double AmoebaReferenceVdwForce::getCutoff() const {
    return _cutoff;
}

void AmoebaReferenceVdwForce::setPeriodicBox(OpenMM::Vec3* vectors) {
    _periodicBoxVectors[0] = vectors[0];
    _periodicBoxVectors[1] = vectors[1];
    _periodicBoxVectors[2] = vectors[2];
}

void AmoebaReferenceVdwForce::setSigmaCombiningRule(const std::string& sigmaCombiningRule) {

    _sigmaCombiningRule = sigmaCombiningRule;

    // convert to upper case and set combining function

    std::transform(_sigmaCombiningRule.begin(), _sigmaCombiningRule.end(), _sigmaCombiningRule.begin(),  (int(*)(int)) std::toupper);
    if (_sigmaCombiningRule == "GEOMETRIC") {
        _combineSigmas = &AmoebaReferenceVdwForce::geometricSigmaCombiningRule;
    } else if (_sigmaCombiningRule == "CUBIC-MEAN") {
        _combineSigmas = &AmoebaReferenceVdwForce::cubicMeanSigmaCombiningRule;
    } else {
        _combineSigmas = &AmoebaReferenceVdwForce::arithmeticSigmaCombiningRule;
    }
}

std::string AmoebaReferenceVdwForce::getSigmaCombiningRule() const {
    return _sigmaCombiningRule;
}

double AmoebaReferenceVdwForce::arithmeticSigmaCombiningRule(double sigmaI, double sigmaJ) const {
    return (sigmaI + sigmaJ);
}

double AmoebaReferenceVdwForce::geometricSigmaCombiningRule(double sigmaI, double sigmaJ) const {
    return 2.0*sqrt(sigmaI*sigmaJ);
}

double AmoebaReferenceVdwForce::cubicMeanSigmaCombiningRule(double sigmaI, double sigmaJ) const {
    double sigmaI2 = sigmaI*sigmaI;
    double sigmaJ2 = sigmaJ*sigmaJ;

    return sigmaI != 0.0 && sigmaJ != 0.0 ? 2.0*(sigmaI2*sigmaI + sigmaJ2*sigmaJ)/(sigmaI2 + sigmaJ2) : 0.0;
}

void AmoebaReferenceVdwForce::setEpsilonCombiningRule(const std::string& epsilonCombiningRule) {

    _epsilonCombiningRule = epsilonCombiningRule;
    std::transform(_epsilonCombiningRule.begin(), _epsilonCombiningRule.end(), _epsilonCombiningRule.begin(),  (int(*)(int)) std::toupper);

    // convert to upper case and set combining function

    if (_epsilonCombiningRule == "ARITHMETIC") {
         _combineEpsilons = &AmoebaReferenceVdwForce::arithmeticEpsilonCombiningRule;
    } else if (_epsilonCombiningRule == "HARMONIC") {
         _combineEpsilons = &AmoebaReferenceVdwForce::harmonicEpsilonCombiningRule;
    } else if (_epsilonCombiningRule == "W-H") {
         _combineEpsilons = &AmoebaReferenceVdwForce::whEpsilonCombiningRule;
    } else if (_epsilonCombiningRule == "HHG") {
         _combineEpsilons = &AmoebaReferenceVdwForce::hhgEpsilonCombiningRule;
    } else {
         _combineEpsilons = &AmoebaReferenceVdwForce::geometricEpsilonCombiningRule;
    }
}

std::string AmoebaReferenceVdwForce::getEpsilonCombiningRule() const {
    return _epsilonCombiningRule;
}

double AmoebaReferenceVdwForce::arithmeticEpsilonCombiningRule(double epsilonI, double epsilonJ, double sigmaI, double sigmaJ) const {
    return 0.5*(epsilonI + epsilonJ);
}

double AmoebaReferenceVdwForce::geometricEpsilonCombiningRule(double epsilonI, double epsilonJ, double sigmaI, double sigmaJ) const {
    return sqrt(epsilonI*epsilonJ);
}

double AmoebaReferenceVdwForce::harmonicEpsilonCombiningRule(double epsilonI, double epsilonJ, double sigmaI, double sigmaJ) const {
    return (epsilonI != 0.0 && epsilonJ != 0.0) ? 2.0*(epsilonI*epsilonJ)/(epsilonI + epsilonJ) : 0.0;
}
double AmoebaReferenceVdwForce::whEpsilonCombiningRule(double epsilonI, double epsilonJ, double sigmaI, double sigmaJ) const {
    double  sigmaI3 = sigmaI * sigmaI * sigmaI;
    double  sigmaJ3 = sigmaJ * sigmaJ * sigmaJ;
    double  sigmaI6 = sigmaI3 * sigmaI3;
    double  sigmaJ6 = sigmaJ3 * sigmaJ3;
    double  eps_s = sqrt(epsilonI*epsilonJ);
    return (epsilonI != 0.0 && epsilonJ != 0.0) ? 2.0*eps_s*sigmaI3*sigmaJ3/(sigmaI6+sigmaJ6) : 0.0;
}

double AmoebaReferenceVdwForce::hhgEpsilonCombiningRule(double epsilonI, double epsilonJ, double sigmaI, double sigmaJ) const {
    double denominator = sqrt(epsilonI) + sqrt(epsilonJ);
    return (epsilonI != 0.0 && epsilonJ != 0.0) ? 4.0*(epsilonI*epsilonJ)/(denominator*denominator) : 0.0;
}

void AmoebaReferenceVdwForce::addReducedForce(unsigned int particleI, unsigned int particleIV,
                                              double reduction, double sign,
                                              Vec3& force, vector<Vec3>& forces) const {

    forces[particleI][0]  += sign*force[0]*reduction;
    forces[particleI][1]  += sign*force[1]*reduction;
    forces[particleI][2]  += sign*force[2]*reduction;

    forces[particleIV][0] += sign*force[0]*(1.0 - reduction);
    forces[particleIV][1] += sign*force[1]*(1.0 - reduction);
    forces[particleIV][2] += sign*force[2]*(1.0 - reduction);
}

double AmoebaReferenceVdwForce::calculatePairIxn(double combinedSigma, double combinedEpsilon, double softcore,
                                                 const Vec3& particleIPosition,
                                                 const Vec3& particleJPosition,
                                                 Vec3& force) const {
    
    static const double dhal = 0.07;
    static const double ghal = 0.12;
    static const double dhal1 = 1.07;
    static const double ghal1 = 1.12;

    // get deltaR, R2, and R between 2 atoms

    double deltaR[ReferenceForce::LastDeltaRIndex];
    if (_nonbondedMethod == CutoffPeriodic)
        ReferenceForce::getDeltaRPeriodic(particleJPosition, particleIPosition, _periodicBoxVectors, deltaR);
    else
        ReferenceForce::getDeltaR(particleJPosition, particleIPosition, deltaR);

    double r_ij_2       = deltaR[ReferenceForce::R2Index];
    double r_ij         = deltaR[ReferenceForce::RIndex];

    double rho = r_ij / combinedSigma;
    double rho2 = rho * rho;
    double rho6 = rho2 * rho2 * rho2;
    double rhoplus = rho + dhal;
    double rhodec2 = rhoplus * rhoplus;
    double rhodec = rhodec2 * rhodec2 * rhodec2;
    double s1 = 1.0 / (softcore + rhodec * rhoplus);
    double s2 = 1.0 / (softcore + rho6 * rho + 0.12);
    double point72 = dhal1 * dhal1;
    double t1 = dhal1 * point72 * point72 * point72 * s1;
    double t2 = ghal1 * s2;
    double t2min = t2 - 2;
    double dt1 = -7.0 * rhodec * t1 * s1;
    double dt2 = -7.0 * rho6 * t2 * s2;
    double energy = combinedEpsilon * t1 * t2min;
    double dEdR = combinedEpsilon * (dt1 * t2min + t1 * dt2) / combinedSigma;

    // tapering
    if ((_nonbondedMethod == CutoffNonPeriodic || _nonbondedMethod == CutoffPeriodic) && r_ij > _taperCutoff) {
        double delta    = r_ij - _taperCutoff;
        double taper    = 1.0 + delta*delta*delta*(_taperCoefficients[C3] + delta*(_taperCoefficients[C4] + delta*_taperCoefficients[C5]));
        double dtaper   = delta*delta*(3.0*_taperCoefficients[C3] + delta*(4.0*_taperCoefficients[C4] + delta*5.0*_taperCoefficients[C5]));
        dEdR            = energy*dtaper + dEdR*taper;
        energy         *= taper;
    }

    dEdR                   /= r_ij;

    force[0]                = dEdR*deltaR[0];
    force[1]                = dEdR*deltaR[1];
    force[2]                = dEdR*deltaR[2];

    return energy;

}

void AmoebaReferenceVdwForce::setReducedPositions(int numParticles,
                                                  const vector<Vec3>& particlePositions,
                                                  const std::vector<int>& indexIVs, 
                                                  const std::vector<double>& reductions,
                                                  std::vector<Vec3>& reducedPositions) const {

    reducedPositions.resize(numParticles);
    for (unsigned int ii = 0; ii <  static_cast<unsigned int>(numParticles); ii++) {
        if (reductions[ii] != 0.0) {
            int reductionIndex     = indexIVs[ii];
            reducedPositions[ii]   = Vec3(reductions[ii]*(particlePositions[ii][0] - particlePositions[reductionIndex][0]) + particlePositions[reductionIndex][0], 
                                          reductions[ii]*(particlePositions[ii][1] - particlePositions[reductionIndex][1]) + particlePositions[reductionIndex][1], 
                                          reductions[ii]*(particlePositions[ii][2] - particlePositions[reductionIndex][2]) + particlePositions[reductionIndex][2]); 
        } else {
            reducedPositions[ii]   = Vec3(particlePositions[ii][0], particlePositions[ii][1], particlePositions[ii][2]); 
        }
    }
}

double AmoebaReferenceVdwForce::calculateForceAndEnergy(int numParticles, double lambda,
                                                        const vector<Vec3>& particlePositions,
                                                        const std::vector<int>& indexIVs, 
                                                        const std::vector<double>& sigmas,
                                                        const std::vector<double>& epsilons,
                                                        const std::vector<double>& reductions,
                                                        const std::vector<bool>& isAlchemical,
                                                        const std::vector< std::set<int> >& allExclusions,
                                                        vector<Vec3>& forces) const {

    // set reduced coordinates

    std::vector<Vec3> reducedPositions;
    setReducedPositions(numParticles, particlePositions, indexIVs, reductions, reducedPositions);

    // loop over all particle pairs

    //    (1) initialize exclusion vector
    //    (2) calculate pair ixn, if not excluded
    //    (3) accumulate forces: if particle is a site where interaction position != particle position,
    //        then call addReducedForce() to apportion force to particle and its covalent partner
    //        based on reduction factor
    //    (4) reset exclusion vector

    double energy = 0.0;
    std::vector<unsigned int> exclusions(numParticles, 0);
    for (unsigned int ii = 0; ii < static_cast<unsigned int>(numParticles); ii++) {
 
        double sigmaI      = sigmas[ii];
        double epsilonI    = epsilons[ii];
        bool isAlchemicalI = isAlchemical[ii];
        for (int jj : allExclusions[ii])
            exclusions[jj] = 1;

        for (unsigned int jj = ii+1; jj < static_cast<unsigned int>(numParticles); jj++) {
            if (exclusions[jj] == 0) {

                double combinedSigma   = (this->*_combineSigmas)(sigmaI, sigmas[jj]);

                double combinedEpsilon = (this->*_combineEpsilons)(epsilonI, epsilons[jj], sigmaI, sigmas[jj]);

              
                double softcore = 0.0;

                if (this->_alchemicalMethod == Decouple && (isAlchemicalI != isAlchemical[jj])) {
                   combinedEpsilon *= pow(lambda, this->_n);
                   softcore = this->_alpha * pow(1.0 - lambda, 2);
                } else if (this->_alchemicalMethod == Annihilate && (isAlchemicalI || isAlchemical[jj])) {
                   combinedEpsilon *= pow(lambda, this->_n);
                   softcore = this->_alpha * pow(1.0 - lambda, 2);
                }


                Vec3 force;
                energy += calculatePairIxn(combinedSigma, combinedEpsilon, softcore,
                                           reducedPositions[ii], reducedPositions[jj], force);

                if (indexIVs[ii] == ii) {
                    forces[ii][0] -= force[0];
                    forces[ii][1] -= force[1];
                    forces[ii][2] -= force[2];
                } else {
                    addReducedForce(ii, indexIVs[ii], reductions[ii], -1.0, force, forces);
                }
                if (indexIVs[jj] == jj) {
                    forces[jj][0] += force[0];
                    forces[jj][1] += force[1];
                    forces[jj][2] += force[2];
                } else {
                    addReducedForce(jj, indexIVs[jj], reductions[jj], 1.0, force, forces);
                }

            }
        }

        for (int jj : allExclusions[ii])
            exclusions[jj] = 0;
    }

    return energy;
}

double AmoebaReferenceVdwForce::calculateForceAndEnergy(int numParticles, double lambda,
                                                        const vector<Vec3>& particlePositions,
                                                        const std::vector<int>& indexIVs, 
                                                        const std::vector<double>& sigmas,
                                                        const std::vector<double>& epsilons,
                                                        const std::vector<double>& reductions,
                                                        const std::vector<bool>& isAlchemical,
                                                        const NeighborList& neighborList,
                                                        vector<Vec3>& forces) const {

    // set reduced coordinates

    std::vector<Vec3> reducedPositions;
    setReducedPositions(numParticles, particlePositions, indexIVs, reductions, reducedPositions);

    // loop over neighbor list
    //    (1) calculate pair vdw ixn
    //    (2) accumulate forces: if particle is a site where interaction position != particle position,
    //        then call addReducedForce() to apportion force to particle and its covalent partner
    //        based on reduction factor

    double energy = 0.0;
    for (unsigned int ii = 0; ii < neighborList.size(); ii++) {

        OpenMM::AtomPair pair       = neighborList[ii];
        int siteI                   = pair.first;
        int siteJ                   = pair.second;

        double combinedSigma   = (this->*_combineSigmas)(sigmas[siteI], sigmas[siteJ]);

        double combinedEpsilon = (this->*_combineEpsilons)(epsilons[siteI], epsilons[siteJ], sigmas[siteI], sigmas[siteJ]);

        double softcore        = 0.0;
        int isAlchemicalI      = isAlchemical[siteI];
        int isAlchemicalJ      = isAlchemical[siteJ];

        if (this->_alchemicalMethod == Decouple && (isAlchemicalI != isAlchemicalJ)) {
           combinedEpsilon *= pow(lambda, this->_n);
           softcore = this->_alpha * pow(1.0 - lambda, 2);
        } else if (this->_alchemicalMethod == Annihilate && (isAlchemicalI || isAlchemicalJ)) {
           combinedEpsilon *= pow(lambda, this->_n);
           softcore = this->_alpha * pow(1.0 - lambda, 2);
        }


        Vec3 force;
        energy += calculatePairIxn(combinedSigma, combinedEpsilon, softcore,
                                   reducedPositions[siteI], reducedPositions[siteJ], force);
                
        if (indexIVs[siteI] == siteI) {
            forces[siteI][0] -= force[0];
            forces[siteI][1] -= force[1];
            forces[siteI][2] -= force[2];
        } else {
            addReducedForce(siteI, indexIVs[siteI], reductions[siteI], -1.0, force, forces);
        }
        if (indexIVs[siteJ] == siteJ) {
            forces[siteJ][0] += force[0];
            forces[siteJ][1] += force[1];
            forces[siteJ][2] += force[2];
        } else {
            addReducedForce(siteJ, indexIVs[siteJ], reductions[siteJ], 1.0, force, forces);
        }

    }

    return energy;
}
