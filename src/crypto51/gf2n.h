#ifndef CRYPTOPP_GF2N_H
#define CRYPTOPP_GF2N_H

/*! \file */

#include "cryptlib.h"
#include "secblock.h"
#include "misc.h"
#include "algebra.h"

#include <iosfwd>

NAMESPACE_BEGIN(CryptoPP)

//! Polynomial with Coefficients in GF(2)
/*!	\nosubgrouping */
class PolynomialMod2
{
public:
	//! \name ENUMS, EXCEPTIONS, and TYPEDEFS
	//@{
		//! divide by zero exception
		class DivideByZero : public Exception
		{
		public:
			DivideByZero() : Exception(OTHER_ERROR, "PolynomialMod2: division by zero") {}
		};

		typedef unsigned int RandomizationParameter;
	//@}

	//! \name CREATORS
	//@{
		//! creates the zero polynomial
		PolynomialMod2();
		//! copy constructor
		PolynomialMod2(const PolynomialMod2& t);

		//! convert from word
		/*! value should be encoded with the least significant bit as coefficient to x^0
			and most significant bit as coefficient to x^(WORD_BITS-1)
			bitLength denotes how much memory to allocate initially
		*/
		PolynomialMod2(word value, unsigned int bitLength=WORD_BITS);

		//! convert from big-endian byte array
		PolynomialMod2(const byte *encodedPoly, unsigned int byteCount)
			{Decode(encodedPoly, byteCount);}

		//! convert from big-endian form stored in a BufferedTransformation
		PolynomialMod2(BufferedTransformation &encodedPoly, unsigned int byteCount)
			{Decode(encodedPoly, byteCount);}

		//! create a random polynomial uniformly distributed over all polynomials with degree less than bitcount
		PolynomialMod2(RandomNumberGenerator &rng, unsigned int bitcount)
			{Randomize(rng, bitcount);}

		//! return x^i
		static PolynomialMod2 Monomial(unsigned i);
		//! return x^t0 + x^t1 + x^t2
		static PolynomialMod2 Trinomial(unsigned t0, unsigned t1, unsigned t2);
		//! return x^t0 + x^t1 + x^t2 + x^t3 + x^t4
		static PolynomialMod2 Pentanomial(unsigned t0, unsigned t1, unsigned t2, unsigned int t3, unsigned int t4);
		//! return x^(n-1) + ... + x + 1
		static PolynomialMod2 AllOnes(unsigned n);

		//!
		static const PolynomialMod2 &Zero();
		//!
		static const PolynomialMod2 &One();
	//@}

	//! \name ENCODE/DECODE
	//@{
		//! minimum number of bytes to encode this polynomial
		/*! MinEncodedSize of 0 is 1 */
		unsigned int MinEncodedSize() const {return STDMAX(1U, ByteCount());}

		//! encode in big-endian format
		/*! if outputLen < MinEncodedSize, the most significant bytes will be dropped
			if outputLen > MinEncodedSize, the most significant bytes will be padded
		*/
		unsigned int Encode(byte *output, unsigned int outputLen) const;
		//!
		unsigned int Encode(BufferedTransformation &bt, unsigned int outputLen) const;

		//!
		void Decode(const byte *input, unsigned int inputLen);
		//! 
		//* Precondition: bt.MaxRetrievable() >= inputLen
		void Decode(BufferedTransformation &bt, unsigned int inputLen);

		//! encode value as big-endian octet string
		void DEREncodeAsOctetString(BufferedTransformation &bt, unsigned int length) const;
		//! decode value as big-endian octet string
		void BERDecodeAsOctetString(BufferedTransformation &bt, unsigned int length);
	//@}

	//! \name ACCESSORS
	//@{
		//! number of significant bits = Degree() + 1
		unsigned int BitCount() const;
		//! number of significant bytes = ceiling(BitCount()/8)
		unsigned int ByteCount() const;
		//! number of significant words = ceiling(ByteCount()/sizeof(word))
		unsigned int WordCount() const;

		//! return the n-th bit, n=0 being the least significant bit
		bool GetBit(unsigned int n) const {return GetCoefficient(n)!=0;}
		//! return the n-th byte
		byte GetByte(unsigned int n) const;

		//! the zero polynomial will return a degree of -1
		signed int Degree() const {return BitCount()-1;}
		//! degree + 1
		unsigned int CoefficientCount() const {return BitCount();}
		//! return coefficient for x^i
		int GetCoefficient(unsigned int i) const
			{return (i/WORD_BITS < reg.size()) ? int(reg[i/WORD_BITS] >> (i % WORD_BITS)) & 1 : 0;}
		//! return coefficient for x^i
		int operator[](unsigned int i) const {return GetCoefficient(i);}

		//!
		bool IsZero() const {return !*this;}
		//!
		bool Equals(const PolynomialMod2 &rhs) const;
	//@}

	//! \name MANIPULATORS
	//@{
		//!
		PolynomialMod2&  operator=(const PolynomialMod2& t);
		//!
		PolynomialMod2&  operator&=(const PolynomialMod2& t);
		//!
		PolynomialMod2&  operator^=(const PolynomialMod2& t);
		//!
		PolynomialMod2&  operator+=(const PolynomialMod2& t) {return *this ^= t;}
		//!
		PolynomialMod2&  operator-=(const PolynomialMod2& t) {return *this ^= t;}
		//!
		PolynomialMod2&  operator*=(const PolynomialMod2& t);
		//!
		PolynomialMod2&  operator/=(const PolynomialMod2& t);
		//!
		PolynomialMod2&  operator%=(const PolynomialMod2& t);
		//!
		PolynomialMod2&  operator<<=(unsigned int);
		//!
		PolynomialMod2&  operator>>=(unsigned int);

		//!
		void Randomize(RandomNumberGenerator &rng, unsigned int bitcount);

		//!
		void SetBit(unsigned int i, int value = 1);
		//! set the n-th byte to value
		void SetByte(unsigned int n, byte value);

		//!
		void SetCoefficient(unsigned int i, int value) {SetBit(i, value);}

		//!
		void swap(PolynomialMod2 &a) {reg.swap(a.reg);}
	//@}

	//! \name UNARY OPERATORS
	//@{
		//!
		bool			operator!() const;
		//!
		PolynomialMod2	operator+() const {return *this;}
		//!
		PolynomialMod2	operator-() const {return *this;}
	//@}

	//! \name BINARY OPERATORS
	//@{
		//!
		PolynomialMod2 And(const PolynomialMod2 &b) const;
		//!
		PolynomialMod2 Xor(const PolynomialMod2 &b) const;
		//!
		PolynomialMod2 Plus(const PolynomialMod2 &b) const {return Xor(b);}
		//!
		PolynomialMod2 Minus(const PolynomialMod2 &b) const {return Xor(b);}
		//!
		PolynomialMod2 Times(const PolynomialMod2 &b) const;
		//!
		PolynomialMod2 DividedBy(const PolynomialMod2 &b) const;
		//!
		PolynomialMod2 Modulo(const PolynomialMod2 &b) const;

		//!
		PolynomialMod2 operator>>(unsigned int n) const;
		//!
		PolynomialMod2 operator<<(unsigned int n) const;
	//@}

	//! \name OTHER ARITHMETIC FUNCTIONS
	//@{
		//! sum modulo 2 of all coefficients
		unsigned int Parity() const;

		//! check for irreducibility
		bool IsIrreducible() const;

		//! is always zero since we're working modulo 2
		PolynomialMod2 Doubled() const {return Zero();}
		//!
		PolynomialMod2 Squared() const;

		//! only 1 is a unit
		bool IsUnit() const {return Equals(One());}
		//! return inverse if *this is a unit, otherwise return 0
		PolynomialMod2 MultiplicativeInverse() const {return IsUnit() ? One() : Zero();}

		//! greatest common divisor
		static PolynomialMod2 Gcd(const PolynomialMod2 &a, const PolynomialMod2 &n);
		//! calculate multiplicative inverse of *this mod n
		PolynomialMod2 InverseMod(const PolynomialMod2 &) const;

		//! calculate r and q such that (a == d*q + r) && (deg(r) < deg(d))
		static void Divide(PolynomialMod2 &r, PolynomialMod2 &q, const PolynomialMod2 &a, const PolynomialMod2 &d);
	//@}

	//! \name INPUT/OUTPUT
	//@{
		//!
		friend std::ostream& operator<<(std::ostream& out, const PolynomialMod2 &a);
	//@}

private:
	friend class GF2NT;

	SecWordBlock reg;
};

//! GF(2^n) with Polynomial Basis
class GF2NP : public QuotientRing<EuclideanDomainOf<PolynomialMod2> >
{
public:
	GF2NP(const PolynomialMod2 &modulus);

	virtual GF2NP * Clone() const {return new GF2NP(*this);}
	virtual void DEREncode(BufferedTransformation &bt) const
		{assert(false);}	// no ASN.1 syntax yet for general polynomial basis

	void DEREncodeElement(BufferedTransformation &out, const Element &a) const;
	void BERDecodeElement(BufferedTransformation &in, Element &a) const;

	bool Equal(const Element &a, const Element &b) const
		{assert(a.Degree() < m_modulus.Degree() && b.Degree() < m_modulus.Degree()); return a.Equals(b);}

	bool IsUnit(const Element &a) const
		{assert(a.Degree() < m_modulus.Degree()); return !!a;}

	unsigned int MaxElementBitLength() const
		{return m;}

	unsigned int MaxElementByteLength() const
		{return BitsToBytes(MaxElementBitLength());}

	Element SquareRoot(const Element &a) const;

	Element HalfTrace(const Element &a) const;

	// returns z such that z^2 + z == a
	Element SolveQuadraticEquation(const Element &a) const;

protected:
	unsigned int m;
};

//! GF(2^n) with Trinomial Basis
class GF2NT : public GF2NP
{
public:
	// polynomial modulus = x^t0 + x^t1 + x^t2, t0 > t1 > t2
	GF2NT(unsigned int t0, unsigned int t1, unsigned int t2);

	GF2NP * Clone() const {return new GF2NT(*this);}
	void DEREncode(BufferedTransformation &bt) const;

	const Element& Multiply(const Element &a, const Element &b) const;

	const Element& Square(const Element &a) const
		{return Reduced(a.Squared());}

	const Element& MultiplicativeInverse(const Element &a) const;

private:
	const Element& Reduced(const Element &a) const;

	unsigned int t0, t1;
	mutable PolynomialMod2 result;
};

//! GF(2^n) with Pentanomial Basis
class GF2NPP : public GF2NP
{
public:
	// polynomial modulus = x^t0 + x^t1 + x^t2 + x^t3 + x^t4, t0 > t1 > t2 > t3 > t4
	GF2NPP(unsigned int t0, unsigned int t1, unsigned int t2, unsigned int t3, unsigned int t4)
		: GF2NP(PolynomialMod2::Pentanomial(t0, t1, t2, t3, t4)), t0(t0), t1(t1), t2(t2), t3(t3) {}

	GF2NP * Clone() const {return new GF2NPP(*this);}
	void DEREncode(BufferedTransformation &bt) const;

private:
	unsigned int t0, t1, t2, t3;
};

// construct new GF2NP from the ASN.1 sequence Characteristic-two
GF2NP * BERDecodeGF2NP(BufferedTransformation &bt);

//!
inline bool operator==(const CryptoPP::PolynomialMod2 &a, const CryptoPP::PolynomialMod2 &b)
	{return a.Equals(b);}
//!
inline bool operator!=(const CryptoPP::PolynomialMod2 &a, const CryptoPP::PolynomialMod2 &b)
	{return !(a==b);}
//! compares degree
inline bool operator> (const CryptoPP::PolynomialMod2 &a, const CryptoPP::PolynomialMod2 &b)
	{return a.Degree() > b.Degree();}
//! compares degree
inline bool operator>=(const CryptoPP::PolynomialMod2 &a, const CryptoPP::PolynomialMod2 &b)
	{return a.Degree() >= b.Degree();}
//! compares degree
inline bool operator< (const CryptoPP::PolynomialMod2 &a, const CryptoPP::PolynomialMod2 &b)
	{return a.Degree() < b.Degree();}
//! compares degree
inline bool operator<=(const CryptoPP::PolynomialMod2 &a, const CryptoPP::PolynomialMod2 &b)
	{return a.Degree() <= b.Degree();}
//!
inline CryptoPP::PolynomialMod2 operator&(const CryptoPP::PolynomialMod2 &a, const CryptoPP::PolynomialMod2 &b) {return a.And(b);}
//!
inline CryptoPP::PolynomialMod2 operator^(const CryptoPP::PolynomialMod2 &a, const CryptoPP::PolynomialMod2 &b) {return a.Xor(b);}
//!
inline CryptoPP::PolynomialMod2 operator+(const CryptoPP::PolynomialMod2 &a, const CryptoPP::PolynomialMod2 &b) {return a.Plus(b);}
//!
inline CryptoPP::PolynomialMod2 operator-(const CryptoPP::PolynomialMod2 &a, const CryptoPP::PolynomialMod2 &b) {return a.Minus(b);}
//!
inline CryptoPP::PolynomialMod2 operator*(const CryptoPP::PolynomialMod2 &a, const CryptoPP::PolynomialMod2 &b) {return a.Times(b);}
//!
inline CryptoPP::PolynomialMod2 operator/(const CryptoPP::PolynomialMod2 &a, const CryptoPP::PolynomialMod2 &b) {return a.DividedBy(b);}
//!
inline CryptoPP::PolynomialMod2 operator%(const CryptoPP::PolynomialMod2 &a, const CryptoPP::PolynomialMod2 &b) {return a.Modulo(b);}

NAMESPACE_END

NAMESPACE_BEGIN(std)
template<> inline void swap(CryptoPP::PolynomialMod2 &a, CryptoPP::PolynomialMod2 &b)
{
	a.swap(b);
}
NAMESPACE_END

#endif
