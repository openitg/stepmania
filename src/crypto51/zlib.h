#ifndef CRYPTOPP_ZLIB_H
#define CRYPTOPP_ZLIB_H

#include "adler32.h"
#include "zdeflate.h"
#include "zinflate.h"

NAMESPACE_BEGIN(CryptoPP)

/// ZLIB Compressor (RFC 1950)
class ZlibCompressor : public Deflator
{
public:
	ZlibCompressor(BufferedTransformation *attachment=NULL, unsigned int deflateLevel=DEFAULT_DEFLATE_LEVEL, unsigned int log2WindowSize=DEFAULT_LOG2_WINDOW_SIZE)
		: Deflator(attachment, deflateLevel, log2WindowSize) {}
	ZlibCompressor(const NameValuePairs &parameters, BufferedTransformation *attachment=NULL)
		: Deflator(parameters, attachment) {}

	unsigned int GetCompressionLevel() const;

private:
	void WritePrestreamHeader();
	void ProcessUncompressedData(const byte *string, unsigned int length);
	void WritePoststreamTail();

	Adler32 m_adler32;
};

/// ZLIB Decompressor (RFC 1950)
class ZlibDecompressor : public Inflator
{
public:
	typedef Inflator::Err Err;
	class HeaderErr : public Err {public: HeaderErr() : Err(INVALID_DATA_FORMAT, "ZlibDecompressor: header decoding error") {}};
	class Adler32Err : public Err {public: Adler32Err() : Err(DATA_INTEGRITY_CHECK_FAILED, "ZlibDecompressor: ADLER32 check error") {}};
	class UnsupportedAlgorithm : public Err {public: UnsupportedAlgorithm() : Err(INVALID_DATA_FORMAT, "ZlibDecompressor: unsupported algorithm") {}};
	class UnsupportedPresetDictionary : public Err {public: UnsupportedPresetDictionary() : Err(INVALID_DATA_FORMAT, "ZlibDecompressor: unsupported preset dictionary") {}};

	/*! \param repeat decompress multiple compressed streams in series
		\param autoSignalPropagation 0 to turn off MessageEnd signal
	*/
	ZlibDecompressor(BufferedTransformation *attachment = NULL, bool repeat = false, int autoSignalPropagation = -1);
	unsigned int GetLog2WindowSize() const {return m_log2WindowSize;}

private:
	unsigned int MaxPrestreamHeaderSize() const {return 2;}
	void ProcessPrestreamHeader();
	void ProcessDecompressedData(const byte *string, unsigned int length);
	unsigned int MaxPoststreamTailSize() const {return 4;}
	void ProcessPoststreamTail();

	unsigned int m_log2WindowSize;
	Adler32 m_adler32;
};

NAMESPACE_END

#endif
