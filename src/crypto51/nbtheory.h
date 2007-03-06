// nbtheory.h - written and placed in the public domain by Wei Dai

#ifndef CRYPTOPP_NBTHEORY_H
#define CRYPTOPP_NBTHEORY_H

#include "integer.h"
#include "algparam.h"

namespace CryptoPP {

// export a table of small primes
extern const unsigned int maxPrimeTableSize;
extern const word lastSmallPrime;
extern unsigned int primeTableSize;
extern word primeTable[];

// build up the table to maxPrimeTableSize
void BuildPrimeTable();

// ************ primality testing ****************

bool IsSmallPrime(const Integer &p);

// returns true if p is divisible by some prime less than bound
// bound not be greater than the largest entry in the prime table
bool TrialDivision(const Integer &p, unsigned bound);

// returns true if p is NOT divisible by small primes
bool SmallDivisorsTest(const Integer &p);

bool IsStrongProbablePrime(const Integer &n, const Integer &b);
bool IsStrongLucasProbablePrime(const Integer &n);

// Rabin-Miller primality test, i.e. repeating the strong probable prime test 
// for several rounds with random bases
bool RabinMillerTest(RandomNumberGenerator &rng, const Integer &w, unsigned int rounds);

// primality test, used to generate primes
bool IsPrime(const Integer &p);

// more reliable than IsPrime(), used to verify primes generated by others
bool VerifyPrime(RandomNumberGenerator &rng, const Integer &p, unsigned int level = 1);

class PrimeSelector
{
public:
	virtual ~PrimeSelector() {}
	const PrimeSelector *GetSelectorPointer() const {return this;}
	virtual bool IsAcceptable(const Integer &candidate) const =0;
};

// use a fast sieve to find the first probable prime in {x | p<=x<=max and x%mod==equiv}
// returns true iff successful, value of p is undefined if no such prime exists
bool FirstPrime(Integer &p, const Integer &max, const Integer &equiv, const Integer &mod, const PrimeSelector *pSelector);

unsigned int PrimeSearchInterval(const Integer &max);

AlgorithmParameters<AlgorithmParameters<AlgorithmParameters<NullNameValuePairs, Integer::RandomNumberType>, Integer>, Integer>
	MakeParametersForTwoPrimesOfEqualSize(unsigned int productBitLength);

// ********** other number theoretic functions ************

inline Integer GCD(const Integer &a, const Integer &b)
	{return Integer::Gcd(a,b);}
inline bool RelativelyPrime(const Integer &a, const Integer &b)
	{return Integer::Gcd(a,b) == Integer::One();}
inline Integer LCM(const Integer &a, const Integer &b)
	{return a/Integer::Gcd(a,b)*b;}
inline Integer EuclideanMultiplicativeInverse(const Integer &a, const Integer &b)
	{return a.InverseMod(b);}

// use Chinese Remainder Theorem to calculate x given x mod p and x mod q
Integer CRT(const Integer &xp, const Integer &p, const Integer &xq, const Integer &q);
// use this one if u = inverse of p mod q has been precalculated
Integer CRT(const Integer &xp, const Integer &p, const Integer &xq, const Integer &q, const Integer &u);

// if b is prime, then Jacobi(a, b) returns 0 if a%b==0, 1 if a is quadratic residue mod b, -1 otherwise
// check a number theory book for what Jacobi symbol means when b is not prime
int Jacobi(const Integer &a, const Integer &b);

// calculates the Lucas function V_e(p, 1) mod n
Integer Lucas(const Integer &e, const Integer &p, const Integer &n);
// calculates x such that m==Lucas(e, x, p*q), p q primes
Integer InverseLucas(const Integer &e, const Integer &m, const Integer &p, const Integer &q);
// use this one if u=inverse of p mod q has been precalculated
Integer InverseLucas(const Integer &e, const Integer &m, const Integer &p, const Integer &q, const Integer &u);

inline Integer ModularExponentiation(const Integer &a, const Integer &e, const Integer &m)
	{return a_exp_b_mod_c(a, e, m);}
// returns x such that x*x%p == a, p prime
Integer ModularSquareRoot(const Integer &a, const Integer &p);
// returns x such that a==ModularExponentiation(x, e, p*q), p q primes,
// and e relatively prime to (p-1)*(q-1)
Integer ModularRoot(const Integer &a, const Integer &e, const Integer &p, const Integer &q);
// use this one if dp=d%(p-1), dq=d%(q-1), (d is inverse of e mod (p-1)*(q-1))
// and u=inverse of p mod q have been precalculated
Integer ModularRoot(const Integer &a, const Integer &dp, const Integer &dq, const Integer &p, const Integer &q, const Integer &u);

// find r1 and r2 such that ax^2 + bx + c == 0 (mod p) for x in {r1, r2}, p prime
// returns true if solutions exist
bool SolveModularQuadraticEquation(Integer &r1, Integer &r2, const Integer &a, const Integer &b, const Integer &c, const Integer &p);

// returns log base 2 of estimated number of operations to calculate discrete log or factor a number
unsigned int DiscreteLogWorkFactor(unsigned int bitlength);
unsigned int FactoringWorkFactor(unsigned int bitlength);

}

#endif