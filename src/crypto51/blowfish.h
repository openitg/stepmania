#ifndef CRYPTOPP_BLOWFISH_H
#define CRYPTOPP_BLOWFISH_H

/** \file */

#include "seckey.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

struct Blowfish_Info : public FixedBlockSize<8>, public VariableKeyLength<16, 1, 56>, public FixedRounds<16>
{
	static const char *StaticAlgorithmName() {return "Blowfish";}
};

//! <a href="http://www.weidai.com/scan-mirror/cs.html#Blowfish">Blowfish</a>
class Blowfish : public Blowfish_Info, public BlockCipherDocumentation
{
	class Base : public BlockCipherBaseTemplate<Blowfish_Info>
	{
	public:
		void ProcessAndXorBlock(const byte *inBlock, const byte *xorBlock, byte *outBlock) const;
		void UncheckedSetKey(CipherDir direction, const byte *key_string, unsigned int keylength);

	private:
		void crypt_block(const word32 in[2], word32 out[2]) const;

		static const word32 p_init[ROUNDS+2];
		static const word32 s_init[4*256];

		FixedSizeSecBlock<word32, ROUNDS+2> pbox;
		FixedSizeSecBlock<word32, 4*256> sbox;
	};

public:
	typedef BlockCipherTemplate<ENCRYPTION, Base> Encryption;
	typedef BlockCipherTemplate<DECRYPTION, Base> Decryption;
};

typedef Blowfish::Encryption BlowfishEncryption;
typedef Blowfish::Decryption BlowfishDecryption;

NAMESPACE_END

#endif
