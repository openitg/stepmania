#ifndef CRYPTOPP_RC5_H
#define CRYPTOPP_RC5_H

/** \file
*/

#include "seckey.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

struct RC5_Info : public FixedBlockSize<8>, public VariableKeyLength<16, 0, 255>, public VariableRounds<16>
{
	static const char *StaticAlgorithmName() {return "RC5";}
	typedef word32 RC5_WORD;
};

/// <a href="http://www.weidai.com/scan-mirror/cs.html#RC5">RC5</a>
class RC5 : public RC5_Info, public BlockCipherDocumentation
{
	class Base : public BlockCipherBaseTemplate<RC5_Info>
	{
	public:
		void UncheckedSetKey(CipherDir direction, const byte *userKey, unsigned int length, unsigned int rounds);

	protected:
		unsigned int r;       // number of rounds
		SecBlock<RC5_WORD> sTable;  // expanded key table
	};

	class Enc : public Base
	{
	public:
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;
	};

	class Dec : public Base
	{
	public:
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;
	};

public:
	typedef BlockCipherTemplate<ENCRYPTION, Enc> Encryption;
	typedef BlockCipherTemplate<DECRYPTION, Dec> Decryption;
};

typedef RC5::Encryption RC5Encryption;
typedef RC5::Decryption RC5Decryption;

NAMESPACE_END

#endif
