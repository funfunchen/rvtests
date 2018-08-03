#ifndef _GRAMMARGAMMA_H_
#define _GRAMMARGAMMA_H_

class EigenMatrix;
class Matrix;

class GrammarGamma {
 public:
  enum AFMethod {
    AF_KINSHIP = 0,  // calculate BLUE AF adjusted by kinship
    AF_MEAN          // calculate AF as genotype mean
  };

 public:  // Make this Impl public to make optimization function easy to write
  class Impl;
  Impl* impl;

 public:
  GrammarGamma(AFMethod);
  ~GrammarGamma();

  // @return 0 when success
  int FitNullModel(Matrix& Xnull, Matrix& y, const EigenMatrix& kinshipU,
                   const EigenMatrix& kinshipS);
  int TestCovariate(Matrix& Xnull, Matrix& y, Matrix& Xcol,
                    const EigenMatrix& kinshipU, const EigenMatrix& kinshipS);
  double GetAF(const EigenMatrix& kinshipU, const EigenMatrix& kinshipS);
  double GetAFFromMean();
  double GetPvalue();
  double GetBeta();
  double GetBetaVar();

  double GetSigmaG2() const;  // sigma_g^2
  double GetSigmaE2() const;  // sigma_e^2
  double GetDelta() const;    // delta = sigma2_e / sigma2_g
};

#endif /* _GRAMMARGAMMA_H_ */
