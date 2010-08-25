#include "global.h"

#include <stdio.h>
#include <tchar.h>
#include <time.h>
#include "CreateZip.h"
#include "RageFile.h"
#include "RageUtil.h"

#define MAX_PATH 1024

// TODO: remove header "extra fields" (the 3 dates)
// Adapted for StepMania from http://www.codeproject.com/KB/files/zip_utils.aspx
//
// THIS FILE is almost entirely based upon code by info-zip.
// It has been modified by Lucian Wischik. The modifications
// were a complete rewrite of the bit of code that generates the
// layout of the zipfile, and support for zipping to/from memory
// or handles or pipes or pagefile or diskfiles, encryption, unicode.
// The original code may be found at http://www.info-zip.org
// The original copyright text follows.
//
//
//
// This is version 1999-Oct-05 of the Info-ZIP copyright and license.
// The definitive version of this document should be available at
// ftp://ftp.cdrom.com/pub/infozip/license.html indefinitely.
//
// Copyright (c) 1990-1999 Info-ZIP.  All rights reserved.
//
// For the purposes of this copyright and license, "Info-ZIP" is defined as
// the following set of individuals:
//
//   Mark Adler, John Bush, Karl Davis, Harald Denker, Jean-Michel Dubois,
//   Jean-loup Gailly, Hunter Goatley, Ian Gorman, Chris Herborth, Dirk Haase,
//   Greg Hartwig, Robert Heath, Jonathan Hudson, Paul Kienitz, David Kirschbaum,
//   Johnny Lee, Onno van der Linden, Igor Mandrichenko, Steve P. Miller,
//   Sergio Monesi, Keith Owens, George Petrov, Greg Roelofs, Kai Uwe Rommel,
//   Steve Salisbury, Dave Smith, Christian Spieler, Antoine Verheijen,
//   Paul von Behren, Rich Wales, Mike White
//
// This software is provided "as is," without warranty of any kind, express
// or implied.  In no event shall Info-ZIP or its contributors be held liable
// for any direct, indirect, incidental, special or consequential damages
// arising out of the use of or inability to use this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
//    1. Redistributions of source code must retain the above copyright notice,
//       definition, disclaimer, and this list of conditions.
//
//    2. Redistributions in binary form must reproduce the above copyright
//       notice, definition, disclaimer, and this list of conditions in
//       documentation and/or other materials provided with the distribution.
//
//    3. Altered versions--including, but not limited to, ports to new operating
//       systems, existing ports with new graphical interfaces, and dynamic,
//       shared, or static library versions--must be plainly marked as such
//       and must not be misrepresented as being the original source.  Such
//       altered versions also must not be misrepresented as being Info-ZIP
//       releases--including, but not limited to, labeling of the altered
//       versions with the names "Info-ZIP" (or any variation thereof, including,
//       but not limited to, different capitalizations), "Pocket UnZip," "WiZ"
//       or "MacZip" without the explicit permission of Info-ZIP.  Such altered
//       versions are further prohibited from misrepresentative use of the
//       Zip-Bugs or Info-ZIP e-mail addresses or of the Info-ZIP URL(s).
//
//    4. Info-ZIP retains the right to use the names "Info-ZIP," "Zip," "UnZip,"
//       "WiZ," "Pocket UnZip," "Pocket Zip," and "MacZip" for its own source and
//       binary releases.
//


typedef unsigned char uch;      // unsigned 8-bit value
typedef unsigned short ush;     // unsigned 16-bit value
typedef unsigned long ulg;      // unsigned 32-bit value
typedef size_t extent;          // file size
typedef unsigned Pos;   // must be at least 32 bits
typedef unsigned IPos; // A Pos is an index in the character window. Pos is used only for parameter passing

#ifndef EOF
#define EOF (-1)
#endif


// Error return values.  The values 0..4 and 12..18 follow the conventions
// of PKZIP.   The values 4..10 are all assigned to "insufficient memory"
// by PKZIP, so the codes 5..10 are used here for other purposes.
#define ZE_MISS         -1      // used by procname(), zipbare()
#define ZE_OK           0       // success
#define ZE_EOF          2       // unexpected end of zip file
#define ZE_FORM         3       // zip file structure error
#define ZE_MEM          4       // out of memory
#define ZE_LOGIC        5       // internal logic error
#define ZE_BIG          6       // entry too large to split
#define ZE_NOTE         7       // invalid comment format
#define ZE_TEST         8       // zip test (-T) failed or out of memory
#define ZE_ABORT        9       // user interrupt or termination
#define ZE_TEMP         10      // error using a temp file
#define ZE_READ         11      // read or seek error
#define ZE_NONE         12      // nothing to do
#define ZE_NAME         13      // missing or empty zip file
#define ZE_WRITE        14      // error writing to a file
#define ZE_CREAT        15      // couldn't open to write
#define ZE_PARMS        16      // bad command line
#define ZE_OPEN         18      // could not open a specified file to read
#define ZE_MAXERR       18      // the highest error number


// internal file attribute
#define UNKNOWN (-1)
#define BINARY  0
#define ASCII   1

#define STORE 0                 // Store method

#define CRCVAL_INITIAL  0L

// MSDOS file or directory attributes
#define MSDOS_HIDDEN_ATTR 0x02
#define MSDOS_DIR_ATTR 0x10

// Lengths of headers after signatures in bytes
#define LOCHEAD 26
#define CENHEAD 42
#define ENDHEAD 18

// Definitions for extra field handling:
#define EB_HEADSIZE       4     /* length of a extra field block header */
#define EB_LEN            2     /* offset of data length field in header */
#define EB_UT_MINLEN      1     /* minimal UT field contains Flags byte */
#define EB_UT_FLAGS       0     /* byte offset of Flags field */
#define EB_UT_TIME1       1     /* byte offset of 1st time value */
#define EB_UT_FL_MTIME    (1 << 0)      /* mtime present */
#define EB_UT_FL_ATIME    (1 << 1)      /* atime present */
#define EB_UT_FL_CTIME    (1 << 2)      /* ctime present */
#define EB_UT_LEN(n)      (EB_UT_MINLEN + 4 * (n))
#define EB_L_UT_SIZE    (EB_HEADSIZE + EB_UT_LEN(3))
#define EB_C_UT_SIZE    (EB_HEADSIZE + EB_UT_LEN(1))


// Macros for writing machine integers to little-endian format
#define PUTSH(a,f) {char _putsh_c=(char)((a)&0xff); wfunc(param,&_putsh_c,1); _putsh_c=(char)((a)>>8); wfunc(param,&_putsh_c,1);}
#define PUTLG(a,f) {PUTSH((a) & 0xffff,(f)) PUTSH((a) >> 16,(f))}


// -- Structure of a ZIP file --
// Signatures for zip file information headers
#define LOCSIG     0x04034b50L
#define CENSIG     0x02014b50L
#define ENDSIG     0x06054b50L
#define EXTLOCSIG  0x08074b50L


#define MIN_MATCH  3
#define MAX_MATCH  258
// The minimum and maximum match lengths


#define WSIZE  (0x8000)
// Maximum window size = 32K. If you are really short of memory, compile
// with a smaller WSIZE but this reduces the compression ratio for files
// of size > WSIZE. WSIZE must be a power of two in the current implementation.
//

#define MIN_LOOKAHEAD (MAX_MATCH+MIN_MATCH+1)
// Minimum amount of lookahead, except at the end of the input file.
// See deflate.c for comments about the MIN_MATCH+1.
//

#define MAX_DIST  (WSIZE-MIN_LOOKAHEAD)
// In order to simplify the code, particularly on 16 bit machines, match
// distances are limited to MAX_DIST instead of WSIZE.
//


#define ZIP_FILENAME 2
#define ZIP_FOLDER   4



// ===========================================================================
// Constants
//

#define MAX_BITS 15
// All codes must not exceed MAX_BITS bits

#define MAX_BL_BITS 7
// Bit length codes must not exceed MAX_BL_BITS bits

#define LENGTH_CODES 29
// number of length codes, not counting the special END_BLOCK code

#define LITERALS  256
// number of literal bytes 0..255

#define END_BLOCK 256
// end of block literal code

#define L_CODES (LITERALS+1+LENGTH_CODES)
// number of Literal or Length codes, including the END_BLOCK code

#define D_CODES   30
// number of distance codes

#define BL_CODES  19
// number of codes used to transfer the bit lengths


#define LIT_BUFSIZE  0x8000
#define DIST_BUFSIZE  LIT_BUFSIZE
// Sizes of match buffers for literals/lengths and distances.  There are
// 4 reasons for limiting LIT_BUFSIZE to 64K:
//   - frequencies can be kept in 16 bit counters
//   - if compression is not successful for the first block, all input data is
//     still in the window so we can still emit a stored block even when input
//     comes from standard input.  (This can also be done for all blocks if
//     LIT_BUFSIZE is not greater than 32K.)
//   - if compression is not successful for a file smaller than 64K, we can
//     even emit a stored file instead of a stored block (saving 5 bytes).
//   - creating new Huffman trees less frequently may not provide fast
//     adaptation to changes in the input data statistics. (Take for
//     example a binary file with poorly compressible code followed by
//     a highly compressible string table.) Smaller buffer sizes give
//     fast adaptation but have of course the overhead of transmitting trees
//     more frequently.
//   - I can't count above 4
// The current code is general and allows DIST_BUFSIZE < LIT_BUFSIZE (to save
// memory at the expense of compression). Some optimizations would be possible
// if we rely on DIST_BUFSIZE == LIT_BUFSIZE.
//

#define REP_3_6      16
// repeat previous bit length 3-6 times (2 bits of repeat count)

#define REPZ_3_10    17
// repeat a zero length 3-10 times  (3 bits of repeat count)

#define REPZ_11_138  18
// repeat a zero length 11-138 times  (7 bits of repeat count)

#define HEAP_SIZE (2*L_CODES+1)
// maximum heap size


// ===========================================================================
// Local data used by the "bit string" routines.
//

#define Buf_size (8 * 2*sizeof(char))
// Number of bits used within bi_buf. (bi_buf may be implemented on
// more than 16 bits on some systems.)

// Output a 16 bit value to the bit stream, lower (oldest) byte first
#define PUTSHORT(state,w) \
{ if (state.bs.out_offset >= state.bs.out_size-1) \
	state.flush_outbuf(state.param,state.bs.out_buf, &state.bs.out_offset); \
	state.bs.out_buf[state.bs.out_offset++] = (char) ((w) & 0xff); \
	state.bs.out_buf[state.bs.out_offset++] = (char) ((ush)(w) >> 8); \
}

#define PUTBYTE(state,b) \
{ if (state.bs.out_offset >= state.bs.out_size) \
	state.flush_outbuf(state.param,state.bs.out_buf, &state.bs.out_offset); \
	state.bs.out_buf[state.bs.out_offset++] = (char) (b); \
}

// DEFLATE.CPP HEADER

#define HASH_BITS  15
// For portability to 16 bit machines, do not use values above 15.

#define HASH_SIZE (unsigned)(1<<HASH_BITS)
#define HASH_MASK (HASH_SIZE-1)
#define WMASK     (WSIZE-1)
// HASH_SIZE and WSIZE must be powers of two

#define NIL 0
// Tail of hash chains

#define FAST 4
#define SLOW 2
// speed options for the general purpose bit flag

#define TOO_FAR 4096
// Matches of length 3 are discarded if their distance exceeds TOO_FAR



#define EQUAL 0
// result of memcmp for equal strings


// ===========================================================================
// Local data used by the "longest match" routines.

#define H_SHIFT  ((HASH_BITS+MIN_MATCH-1)/MIN_MATCH)
// Number of bits by which ins_h and del_h must be shifted at each
// input step. It must be such that after MIN_MATCH steps, the oldest
// byte no longer takes part in the hash key, that is:
//   H_SHIFT * MIN_MATCH >= HASH_BITS

#define max_insert_length  max_lazy_match
// Insert new strings in the hash table only if the match length
// is not greater than this length. This saves time but degrades compression.
// max_insert_length is used only for compression levels <= 3.



const int extra_lbits[LENGTH_CODES] // extra bits for each length code
= {0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};

const int extra_dbits[D_CODES] // extra bits for each distance code
= {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};

const int extra_blbits[BL_CODES]// extra bits for each bit length code
= {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,3,7};

const uch bl_order[BL_CODES] = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
// The lengths of the bit length codes are sent in order of decreasing
// probability, to avoid transmitting the lengths for unused bit length codes.


typedef struct config {
	ush good_length; // reduce lazy search above this match length
	ush max_lazy;    // do not perform lazy search above this match length
	ush nice_length; // quit search above this match length
	ush max_chain;
} config;

// Values for max_lazy_match, good_match, nice_match and max_chain_length,
// depending on the desired pack level (0..9). The values given below have
// been tuned to exclude worst case performance for pathological files.
// Better values may be found for specific files.
//

const config configuration_table[10] = {
	//  good lazy nice chain
	{0,    0,  0,    0},  // 0 store only
	{4,    4,  8,    4},  // 1 maximum speed, no lazy matches
	{4,    5, 16,    8},  // 2
	{4,    6, 32,   32},  // 3
	{4,    4, 16,   16},  // 4 lazy matches */
	{8,   16, 32,   32},  // 5
	{8,   16, 128, 128},  // 6
	{8,   32, 128, 256},  // 7
	{32, 128, 258, 1024}, // 8
	{32, 258, 258, 4096}};// 9 maximum compression */

	// Note: the deflate() code requires max_lazy >= MIN_MATCH and max_chain >= 4
	// For deflate_fast() (levels <= 3) good is ignored and lazy has a different meaning.







	// Data structure describing a single value and its code string.
	typedef struct ct_data {
		union {
			ush  freq;       // frequency count
			ush  code;       // bit string
		} fc;
		union {
			ush  dad;        // father node in Huffman tree
			ush  len;        // length of bit string
		} dl;
	} ct_data;

	typedef struct tree_desc {
		ct_data *dyn_tree;      // the dynamic tree
		ct_data *static_tree;   // corresponding static tree or NULL
		const int *extra_bits;  // extra bits for each code or NULL
		int     extra_base;     // base index for extra_bits
		int     elems;          // max number of elements in the tree
		int     max_length;     // max bit length for the codes
		int     max_code;       // largest code with non zero frequency
	} tree_desc;




	class TTreeState
	{
	public:
		TTreeState();

		ct_data dyn_ltree[HEAP_SIZE];    // literal and length tree
		ct_data dyn_dtree[2*D_CODES+1];  // distance tree
		ct_data static_ltree[L_CODES+2]; // the static literal tree...
		// ... Since the bit lengths are imposed, there is no need for the L_CODES
		// extra codes used during heap construction. However the codes 286 and 287
		// are needed to build a canonical tree (see ct_init below).
		ct_data static_dtree[D_CODES]; // the static distance tree...
		// ... (Actually a trivial tree since all codes use 5 bits.)
		ct_data bl_tree[2*BL_CODES+1];  // Huffman tree for the bit lengths

		tree_desc l_desc;
		tree_desc d_desc;
		tree_desc bl_desc;

		ush bl_count[MAX_BITS+1];  // number of codes at each bit length for an optimal tree

		int heap[2*L_CODES+1]; // heap used to build the Huffman trees
		int heap_len;               // number of elements in the heap
		int heap_max;               // element of largest frequency
		// The sons of heap[n] are heap[2*n] and heap[2*n+1]. heap[0] is not used.
		// The same heap array is used to build all trees.

		uch depth[2*L_CODES+1];
		// Depth of each subtree used as tie breaker for trees of equal frequency

		uch length_code[MAX_MATCH-MIN_MATCH+1];
		// length code for each normalized match length (0 == MIN_MATCH)

		uch dist_code[512];
		// distance codes. The first 256 values correspond to the distances
		// 3 .. 258, the last 256 values correspond to the top 8 bits of
		// the 15 bit distances.

		int base_length[LENGTH_CODES];
		// First normalized length for each code (0 = MIN_MATCH)

		int base_dist[D_CODES];
		// First normalized distance for each code (0 = distance of 1)

		uch l_buf[LIT_BUFSIZE];  // buffer for literals/lengths
		ush d_buf[DIST_BUFSIZE]; // buffer for distances

		uch flag_buf[(LIT_BUFSIZE/8)];
		// flag_buf is a bit array distinguishing literals from lengths in
		// l_buf, and thus indicating the presence or absence of a distance.

		unsigned last_lit;    // running index in l_buf
		unsigned last_dist;   // running index in d_buf
		unsigned last_flags;  // running index in flag_buf
		uch flags;            // current flags not yet saved in flag_buf
		uch flag_bit;         // current bit used in flags
		// bits are filled in flags starting at bit 0 (least significant).
		// Note: these flags are overkill in the current code since we don't
		// take advantage of DIST_BUFSIZE == LIT_BUFSIZE.

		ulg opt_len;          // bit length of current block with optimal trees
		ulg static_len;       // bit length of current block with static trees

		ulg cmpr_bytelen;     // total byte length of compressed file
		ulg cmpr_len_bits;    // number of bits past 'cmpr_bytelen'

		ulg input_len;        // total byte length of input file
		// input_len is for debugging only since we can get it by other means.

		ush *file_type;       // pointer to UNKNOWN, BINARY or ASCII
		//  int *file_method;     // pointer to DEFLATE or STORE
	};

	TTreeState::TTreeState()
	{ 
		tree_desc a = {dyn_ltree, static_ltree, extra_lbits, LITERALS+1, L_CODES, MAX_BITS, 0};  l_desc = a;
		tree_desc b = {dyn_dtree, static_dtree, extra_dbits, 0,          D_CODES, MAX_BITS, 0};  d_desc = b;
		tree_desc c = {bl_tree, NULL,       extra_blbits, 0,         BL_CODES, MAX_BL_BITS, 0};  bl_desc = c;
		last_lit=0;
		last_dist=0;
		last_flags=0;
	}



	class TBitState
	{
	public:
		int flush_flg;
		//
		unsigned bi_buf;
		// Output buffer. bits are inserted starting at the bottom (least significant
		// bits). The width of bi_buf must be at least 16 bits.
		int bi_valid;
		// Number of valid bits in bi_buf.  All bits above the last valid bit
		// are always zero.
		char *out_buf;
		// Current output buffer.
		unsigned out_offset;
		// Current offset in output buffer.
		// On 16 bit machines, the buffer is limited to 64K.
		unsigned out_size;
		// Size of current output buffer
		ulg bits_sent;   // bit length of the compressed data  only needed for debugging???
	};







	class TDeflateState
	{ 
	public:
		TDeflateState() {window_size=0;}

		uch    window[2L*WSIZE];
		// Sliding window. Input bytes are read into the second half of the window,
		// and move to the first half later to keep a dictionary of at least WSIZE
		// bytes. With this organization, matches are limited to a distance of
		// WSIZE-MAX_MATCH bytes, but this ensures that IO is always
		// performed with a length multiple of the block size. Also, it limits
		// the window size to 64K, which is quite useful on MSDOS.
		// To do: limit the window size to WSIZE+CBSZ if SMALL_MEM (the code would
		// be less efficient since the data would have to be copied WSIZE/CBSZ times)
		Pos    prev[WSIZE];
		// Link to older string with same hash index. To limit the size of this
		// array to 64K, this link is maintained only for the last 32K strings.
		// An index in this array is thus a window index modulo 32K.
		Pos    head[HASH_SIZE];
		// Heads of the hash chains or NIL. If your compiler thinks that
		// HASH_SIZE is a dynamic value, recompile with -DDYN_ALLOC.

		ulg window_size;
		// window size, 2*WSIZE except for MMAP or BIG_MEM, where it is the
		// input file length plus MIN_LOOKAHEAD.

		long block_start;
		// window position at the beginning of the current output block. Gets
		// negative when the window is moved backwards.

		int sliding;
		// Set to false when the input file is already in memory

		unsigned ins_h;  // hash index of string to be inserted

		unsigned int prev_length;
		// Length of the best match at previous step. Matches not greater than this
		// are discarded. This is used in the lazy match evaluation.

		unsigned strstart;         // start of string to insert
		unsigned match_start; // start of matching string
		int      eofile;           // flag set at end of input file
		unsigned lookahead;        // number of valid bytes ahead in window

		unsigned max_chain_length;
		// To speed up deflation, hash chains are never searched beyond this length.
		// A higher limit improves compression ratio but degrades the speed.

		unsigned int max_lazy_match;
		// Attempt to find a better match only when the current match is strictly
		// smaller than this value. This mechanism is used only for compression
		// levels >= 4.

		unsigned good_match;
		// Use a faster search when the previous match is longer than this

		int nice_match; // Stop searching when current match exceeds this
	};

	typedef __int64 lutime_t;       // define it ourselves since we don't include time.h

	typedef struct iztimes {
		lutime_t atime,mtime,ctime;
	} iztimes; // access, modify, create times

	typedef struct zlist {
		ush vem, ver, flg, how;       // See central header in zipfile.c for what vem..off are
		ulg tim, crc, siz, len;
		extent nam, ext, cext, com;   // offset of ext must be >= LOCHEAD
		ush dsk, att, lflg;           // offset of lflg must be >= LOCHEAD
		ulg atx, off;
		char name[MAX_PATH];          // File name in zip file
		char *extra;                  // Extra field (set only if ext != 0)
		char *cextra;                 // Extra in central (set only if cext != 0)
		char *comment;                // Comment (set only if com != 0)
		char iname[MAX_PATH];         // Internal file name after cleanup
		char zname[MAX_PATH];         // External version of internal name
		int mark;                     // Marker for files to operate on
		int trash;                    // Marker for files to delete
		int dosflag;                  // Set to force MSDOS file attributes
		struct zlist *nxt;        // Pointer to next header in list
	} TZipFileInfo;






	void __cdecl Trace(const char *x, ...) {va_list paramList; va_start(paramList, x); paramList; va_end(paramList);}
	void __cdecl Tracec(bool ,const char *x, ...) {va_list paramList; va_start(paramList, x); paramList; va_end(paramList);}




	struct TState;
	typedef unsigned (*READFUNC)(TState &state, char *buf,unsigned size);
	typedef unsigned (*FLUSHFUNC)(void *param, const char *buf, unsigned *size);
	typedef unsigned (*WRITEFUNC)(void *param, const char *buf, unsigned size);
	struct TState
	{
		void *param;
		int level; bool seekable;
		READFUNC readfunc; FLUSHFUNC flush_outbuf;
		TTreeState ts; TBitState bs; TDeflateState ds;
		const char *err;
	};



	int putlocal(struct zlist *z, WRITEFUNC wfunc,void *param)
	{ // Write a local header described by *z to file *f.  Return a ZE_ error code.
		PUTLG(LOCSIG, f);
		PUTSH(z->ver, f);
		PUTSH(z->lflg, f);
		PUTSH(z->how, f);
		PUTLG(z->tim, f);
		PUTLG(z->crc, f);
		PUTLG(z->siz, f);
		PUTLG(z->len, f);
		PUTSH(z->nam, f);
		PUTSH(z->ext, f);
		size_t res = (size_t)wfunc(param, z->iname, (unsigned int)z->nam);
		if (res!=z->nam) return ZE_TEMP;
		if (z->ext)
		{ res = (size_t)wfunc(param, z->extra, (unsigned int)z->ext);
		if (res!=z->ext) return ZE_TEMP;
		}
		return ZE_OK;
	}

	int putextended(struct zlist *z, WRITEFUNC wfunc, void *param)
	{ // Write an extended local header described by *z to file *f. Returns a ZE_ code
		PUTLG(EXTLOCSIG, f);
		PUTLG(z->crc, f);
		PUTLG(z->siz, f);
		PUTLG(z->len, f);
		return ZE_OK;
	}

	int putcentral(struct zlist *z, WRITEFUNC wfunc, void *param)
	{ // Write a central header entry of *z to file *f. Returns a ZE_ code.
		PUTLG(CENSIG, f);
		PUTSH(z->vem, f);
		PUTSH(z->ver, f);
		PUTSH(z->flg, f);
		PUTSH(z->how, f);
		PUTLG(z->tim, f);
		PUTLG(z->crc, f);
		PUTLG(z->siz, f);
		PUTLG(z->len, f);
		PUTSH(z->nam, f);
		PUTSH(z->cext, f);
		PUTSH(z->com, f);
		PUTSH(z->dsk, f);
		PUTSH(z->att, f);
		PUTLG(z->atx, f);
		PUTLG(z->off, f);
		if ((size_t)wfunc(param, z->iname, (unsigned int)z->nam) != z->nam ||
			(z->cext && (size_t)wfunc(param, z->cextra, (unsigned int)z->cext) != z->cext) ||
			(z->com && (size_t)wfunc(param, z->comment, (unsigned int)z->com) != z->com))
			return ZE_TEMP;
		return ZE_OK;
	}


	int putend(int n, ulg s, ulg c, extent m, char *z, WRITEFUNC wfunc, void *param)
	{ // write the end of the central-directory-data to file *f.
		PUTLG(ENDSIG, f);
		PUTSH(0, f);
		PUTSH(0, f);
		PUTSH(n, f);
		PUTSH(n, f);
		PUTLG(s, f);
		PUTLG(c, f);
		PUTSH(m, f);
		// Write the comment, if any
		if (m && wfunc(param, z, (unsigned int)m) != m) return ZE_TEMP;
		return ZE_OK;
	}





	const ulg crc_table[256] = {
		0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
			0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
			0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
			0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
			0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
			0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
			0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
			0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
			0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
			0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
			0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
			0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
			0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
			0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
			0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
			0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
			0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
			0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
			0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
			0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
			0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
			0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
			0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
			0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
			0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
			0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
			0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
			0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
			0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
			0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
			0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
			0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
			0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
			0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
			0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
			0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
			0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
			0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
			0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
			0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
			0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
			0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
			0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
			0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
			0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
			0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
			0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
			0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
			0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
			0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
			0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
			0x2d02ef8dL
	};

#define CRC32(c, b) (crc_table[((int)(c) ^ (b)) & 0xff] ^ ((c) >> 8))
#define DO1(buf)  crc = CRC32(crc, *buf++)
#define DO2(buf)  DO1(buf); DO1(buf)
#define DO4(buf)  DO2(buf); DO2(buf)
#define DO8(buf)  DO4(buf); DO4(buf)

	ulg crc32(ulg crc, const uch *buf, extent len)
	{
		if (buf==NULL) return 0L;
		crc = crc ^ 0xffffffffL;
		while (len >= 8) {DO8(buf); len -= 8;}
		if (len) do {DO1(buf);} while (--len);
		return crc ^ 0xffffffffL;  // (instead of ~c for 64-bit machines)
	}


	class TZip
	{ 
	public:
		TZip() : pfout(NULL),zfis(0),hfin(0),writ(0),oerr(false),hasputcen(false),ooffset(0)
		{
		}
		~TZip()
		{
		}

		// These variables say about the file we're writing into
		// We can write to pipe, file-by-handle, file-by-name, memory-to-memmapfile
		RageFile *pfout;             // if valid, we'll write here (for files or pipes)
		unsigned ooffset;         // for hfout, this is where the pointer was initially
		ZRESULT oerr;             // did a write operation give rise to an error?
		unsigned writ;            // how have we written. This is maintained by Add, not write(), to avoid confusion over seeks
		unsigned int opos;        // current pos in the mmap
		unsigned int mapsize;     // the size of the map we created
		bool hasputcen;           // have we yet placed the central directory?
		unsigned long keys[3];    // keys are initialised inside Add()
		//
		TZipFileInfo *zfis;       // each file gets added onto this list, for writing the table at the end

		ZRESULT Create(void *z,unsigned long flags);
		static unsigned sflush(void *param,const char *buf, unsigned *size);
		static unsigned swrite(void *param,const char *buf, unsigned size);
		unsigned int write(const char *buf,unsigned int size);
		bool oseek(unsigned int pos);
		ZRESULT Close();

		// some variables to do with the file currently being read:
		// I haven't done it object-orientedly here, just put them all
		// together, since OO didn't seem to make the design any clearer.
		ulg attr; iztimes times; ulg timestamp;  // all open_* methods set these
		long isize,ired;         // size is not set until close() on pips
		ulg crc;                                 // crc is not set until close(). iwrit is cumulative
		RageFile *hfin;           // for input files and pipes
		const char *bufin; unsigned int lenin,posin; // for memory
		// and a variable for what we've done with the input: (i.e. compressed it!)
		ulg csize;                               // compressed size, set by the compression routines
		// and this is used by some of the compression routines
		char buf[16384];


		ZRESULT open_file(const TCHAR *fn);
		ZRESULT open_dir();
		ZRESULT set_times();
		unsigned read(char *buf, unsigned size);
		ZRESULT iclose();

		ZRESULT ideflate(TZipFileInfo *zfi);
		ZRESULT istore();

		ZRESULT Add(const TCHAR *odstzn, void *src, unsigned long flags);
		ZRESULT AddCentral();

	};



	ZRESULT TZip::Create(void *z,unsigned long flags)
	{ 
		if (pfout!=0 || writ!=0 || oerr!=ZR_OK || hasputcen) 
			return ZR_NOTINITED;
		//
		if (flags==ZIP_FILENAME)
		{ 
			const TCHAR *fn = (const TCHAR*)z;
			pfout = new RageFile();
			if( !pfout->Open( fn, RageFile::WRITE ) )
			{
				pfout = NULL; 
				return ZR_NOFILE;
			}
			ooffset=0;
			return ZR_OK;
		}
		else 
			return ZR_ARGS;
	}

	unsigned TZip::sflush(void *param,const char *buf, unsigned *size)
	{ // static
		if (*size==0) return 0;
		TZip *zip = (TZip*)param;
		unsigned int writ = zip->write(buf,*size);
		if (writ!=0) *size=0;
		return writ;
	}
	unsigned TZip::swrite(void *param,const char *buf, unsigned size)
	{ // static
		if (size==0) return 0;
		TZip *zip=(TZip*)param; return zip->write(buf,size);
	}
	unsigned int TZip::write(const char *buf,unsigned int size)
	{ 
		const char *srcbuf=buf;
		if (pfout != NULL)
		{
			unsigned long writ = pfout->Write( srcbuf, size );
			return writ;
		}
		oerr=ZR_NOTINITED;
		return 0;
	}

	bool TZip::oseek(unsigned int pos)
	{ 
		oerr=ZR_SEEK; 
		return false;
	}

	ZRESULT TZip::Close()
	{
		// if the directory hadn't already been added through a call to GetMemory,
		// then we do it now
		ZRESULT res=ZR_OK; 
		if (!hasputcen) 
			res=AddCentral(); 
		hasputcen=true;
		SAFE_DELETE( pfout );
		return res;
	}



#define ZIP_ATTR_READONLY 0x01
#define ZIP_ATTR_HIDDEN 0x02
#define ZIP_ATTR_SYSTEM 0x04
#define ZIP_ATTR_DIRECTORY 0x10
#define ZIP_ATTR_ARCHIVE 0x20
#define ZIP_ATTR_DIRECTORY2 0x40000000
#define ZIP_ATTR_NORMAL_FILE 0x80000000
#define ZIP_ATTR_READABLE 0x01000000
#define ZIP_ATTR_WRITEABLE 0x00800000
#define ZIP_ATTR_EXECUTABLE 0x00400000

	ZRESULT TZip::open_file(const TCHAR *fn)
	{ 
		hfin=0; bufin=0; crc=CRCVAL_INITIAL; isize=0; csize=0; ired=0;
		if (fn==0) 
			return ZR_ARGS;
		hfin = new RageFile();
		if( !hfin->Open(fn) )
		{
			SAFE_DELETE( hfin );
			return ZR_NOFILE;
		}
		isize = hfin->GetFileSize();
		attr= ZIP_ATTR_NORMAL_FILE | ZIP_ATTR_READABLE | ZIP_ATTR_WRITEABLE;
		return set_times();
	}

	ZRESULT TZip::open_dir()
	{ 
		hfin=0; bufin=0; crc=CRCVAL_INITIAL; isize=0; csize=0; ired=0;
		attr= ZIP_ATTR_DIRECTORY2 | ZIP_ATTR_READABLE | ZIP_ATTR_WRITEABLE | ZIP_ATTR_DIRECTORY;
		isize = 0;
		return set_times();
	}

	
	void filetime2dosdatetime(const tm st, unsigned short *dosdate,unsigned short *dostime)
	{ 
		// date: bits 0-4 are day of month 1-31. Bits 5-8 are month 1..12. Bits 9-15 are year-1980
		// time: bits 0-4 are seconds/2, bits 5-10 are minute 0..59. Bits 11-15 are hour 0..23
		*dosdate = (unsigned short)(((st.tm_year+1900-1980)&0x7f) << 9);
		*dosdate |= (unsigned short)(((st.tm_mon+1)&0xf) << 5);
		*dosdate |= (unsigned short)((st.tm_mday&0x1f));
		*dostime = (unsigned short)((st.tm_hour&0x1f) << 11);
		*dostime |= (unsigned short)((st.tm_min&0x3f) << 5);
		*dostime |= (unsigned short)((st.tm_sec*2)&0x1f);
	}

	ZRESULT TZip::set_times()
	{
		time_t rawtime;
		tm *ptm;
		time ( &rawtime );
		ptm = localtime ( &rawtime );

		unsigned short dosdate,dostime; 
		filetime2dosdatetime(*ptm,&dosdate,&dostime);
		times.atime = time(NULL);
		times.mtime = times.atime;
		times.ctime = times.atime;
		timestamp = (unsigned short)dostime | (((unsigned long)dosdate)<<16);
		return ZR_OK;
	}

	unsigned TZip::read(char *buf, unsigned size)
	{ 
		if (bufin!=0)
		{ 
			if (posin>=lenin) return 0; // end of input
			ulg red = lenin-posin;
			if (red>size) 
				red=size;
			memcpy(buf, bufin+posin, red);
			posin += red;
			ired += red;
			crc = crc32(crc, (uch*)buf, red);
			return red;
		}
		else if (hfin!=0)
		{ 
			int red = hfin->Read(buf,size);
			if (red <= 0)
				return 0;
			ired += red;
			crc = crc32(crc, (uch*)buf, red);
			return red;
		}
		else
		{
			oerr=ZR_NOTINITED; 
			return 0;
		}
	}

	ZRESULT TZip::iclose()
	{ 
		if (hfin!=0)
			SAFE_DELETE( hfin); 
		bool mismatch = (isize!=-1 && isize!=ired);
		isize=ired; // and crc has been being updated anyway
		if (mismatch) 
			return ZR_MISSIZE;
		else 
			return ZR_OK;
	}


	ZRESULT TZip::istore()
	{ 
		ulg size=0;
		for (;;)
		{
			unsigned int cin=read(buf,16384); 
			if (cin<=0 || cin==(unsigned int)EOF) 
				break;
			unsigned int cout = write(buf,cin); 
			if (cout!=cin) 
				return ZR_MISSIZE;
			size += cin;
		}
		csize=size;
		return ZR_OK;
	}





	bool has_seeded=false;
	ZRESULT TZip::Add(const TCHAR *odstzn, void *src,unsigned long flags)
	{
		if (oerr)
			return ZR_FAILED;
		if (hasputcen)
			return ZR_ENDED;

		// zip has its own notion of what its names should look like: i.e. dir/file.stuff
		TCHAR dstzn[MAX_PATH]; _tcscpy(dstzn,odstzn);
		if (*dstzn==0) 
			return ZR_ARGS;
		TCHAR *d=dstzn; 
		while (*d!=0) {if (*d=='\\') *d='/'; d++;}
		bool isdir = (flags==ZIP_FOLDER);
		bool needs_trailing_slash = (isdir && dstzn[_tcslen(dstzn)-1]!='/');
		int method=STORE;

		// now open whatever was our input source:
		ZRESULT openres;
		if (flags==ZIP_FILENAME) 
			openres=open_file((const TCHAR*)src);
		else if (flags==ZIP_FOLDER) 
			openres=open_dir();
		else 
			return ZR_ARGS;
		if (openres!=ZR_OK) 
			return openres;

		// A zip "entry" consists of a local header (which includes the file name),
		// then the compressed data, and possibly an extended local header.

		// Initialize the local header
		TZipFileInfo zfi; zfi.nxt=NULL;
		strcpy(zfi.name,"");
#ifdef UNICODE
		WideCharToMultiByte(CP_UTF8,0,dstzn,-1,zfi.iname,MAX_PATH,0,0);
#else
		strcpy(zfi.iname,dstzn);
#endif
		zfi.nam=strlen(zfi.iname);
		if (needs_trailing_slash) {strcat(zfi.iname,"/"); zfi.nam++;}
		strcpy(zfi.zname,"");
		zfi.extra=NULL; zfi.ext=0;   // extra header to go after this compressed data, and its length
		zfi.cextra=NULL; zfi.cext=0; // extra header to go in the central end-of-zip directory, and its length
		zfi.comment=NULL; zfi.com=0; // comment, and its length
		zfi.mark = 1;
		zfi.dosflag = 0;
		zfi.att = (ush)BINARY;
		zfi.vem = (ush)0xB17; // 0xB00 is win32 os-code. 0x17 is 23 in decimal: zip 2.3
		zfi.ver = (ush)20;    // Needs PKUNZIP 2.0 to unzip it
		zfi.tim = timestamp;
		// Even though we write the header now, it will have to be rewritten, since we don't know compressed size or crc.
		zfi.crc = 0;            // to be updated later
		zfi.flg = 8;            // 8 means 'there is an extra header'. Assume for the moment that we need it.
		zfi.lflg = zfi.flg;     // to be updated later
		zfi.how = (ush)method;  // to be updated later
		zfi.siz = (ulg)(method==STORE && isize>=0 ? isize : 0); // to be updated later
		zfi.len = (ulg)(isize);  // to be updated later
		zfi.dsk = 0;
		zfi.atx = attr;
		zfi.off = writ+ooffset;         // offset within file of the start of this local record
		// stuff the 'times' structure into zfi.extra

		// nb. apparently there's a problem with PocketPC CE(zip)->CE(unzip) fails. And removing the following block fixes it up.
		char xloc[EB_L_UT_SIZE]; zfi.extra=xloc;  zfi.ext=EB_L_UT_SIZE;
		char xcen[EB_C_UT_SIZE]; zfi.cextra=xcen; zfi.cext=EB_C_UT_SIZE;
		xloc[0]  = 'U';
		xloc[1]  = 'T';
		xloc[2]  = EB_UT_LEN(3);       // length of data part of e.f.
		xloc[3]  = 0;
		xloc[4]  = EB_UT_FL_MTIME | EB_UT_FL_ATIME | EB_UT_FL_CTIME;
		xloc[5]  = (char)(times.mtime);
		xloc[6]  = (char)(times.mtime >> 8);
		xloc[7]  = (char)(times.mtime >> 16);
		xloc[8]  = (char)(times.mtime >> 24);
		xloc[9]  = (char)(times.atime);
		xloc[10] = (char)(times.atime >> 8);
		xloc[11] = (char)(times.atime >> 16);
		xloc[12] = (char)(times.atime >> 24);
		xloc[13] = (char)(times.ctime);
		xloc[14] = (char)(times.ctime >> 8);
		xloc[15] = (char)(times.ctime >> 16);
		xloc[16] = (char)(times.ctime >> 24);
		memcpy(zfi.cextra,zfi.extra,EB_C_UT_SIZE);
		zfi.cextra[EB_LEN] = EB_UT_LEN(1);


		// (1) Start by writing the local header:
		int r = putlocal(&zfi,swrite,this);
		if (r!=ZE_OK) 
		{
			iclose(); 
			return ZR_WRITE;
		}
		writ += 4 + LOCHEAD + (unsigned int)zfi.nam + (unsigned int)zfi.ext;
		if (oerr!=ZR_OK)
		{
			iclose();
			return oerr;
		}

		// (1.5) if necessary, write the encryption header
		keys[0]=305419896L;
		keys[1]=591751049L;
		keys[2]=878082192L;
		// generate some random bytes
		if (!has_seeded) 
			srand(0);
		char encbuf[12]; 
		for (int i=0; i<12; i++) 
			encbuf[i]=(char)((rand()>>7)&0xff);
		encbuf[11] = (char)((zfi.tim>>8)&0xff);

		//(2) Write deflated/stored file to zip file
		ZRESULT writeres=ZR_OK;
		if (!isdir && method==STORE) 
			writeres=istore();
		else if (isdir) 
			csize=0;
		else
			FAIL_M("deflate removed");
		iclose();
		writ += csize;
		if (oerr!=ZR_OK) 
			return oerr;
		if (writeres!=ZR_OK)
			return ZR_WRITE;

		// (3) Either rewrite the local header with correct information...
		bool first_header_has_size_right = (zfi.siz==csize);
		zfi.crc = crc;
		zfi.siz = csize;
		zfi.len = isize;
		// (4) ... or put an updated header at the end
		if (zfi.how != (ush) method) 
			return ZR_NOCHANGE;
		if (method==STORE && !first_header_has_size_right) 
			return ZR_NOCHANGE;
		if ((r = putextended(&zfi, swrite,this)) != ZE_OK) 
			return ZR_WRITE;
		writ += 16L;
		zfi.flg = zfi.lflg; // if flg modified by inflate, for the central index

		if (oerr!=ZR_OK)
			return oerr;

		// Keep a copy of the zipfileinfo, for our end-of-zip directory
		char *cextra = new char[zfi.cext]; memcpy(cextra,zfi.cextra,zfi.cext); zfi.cextra=cextra;
		TZipFileInfo *pzfi = new TZipFileInfo; memcpy(pzfi,&zfi,sizeof(zfi));
		if (zfis==NULL) 
			zfis=pzfi;
		else 
		{
			TZipFileInfo *z=zfis; 
			while (z->nxt!=NULL) 
				z=z->nxt; 
			z->nxt=pzfi;
		}
		return ZR_OK;
	}

	ZRESULT TZip::AddCentral()
	{ // write central directory
		int numentries = 0;
		ulg pos_at_start_of_central = writ;
		//ulg tot_unc_size=0, tot_compressed_size=0;
		bool okay=true;
		for (TZipFileInfo *zfi=zfis; zfi!=NULL; )
		{
			if (okay)
			{
				int res = putcentral(zfi, swrite,this);
				if (res!=ZE_OK) 
					okay=false;
			}
			writ += 4 + CENHEAD + (unsigned int)zfi->nam + (unsigned int)zfi->cext + (unsigned int)zfi->com;
			//tot_unc_size += zfi->len;
			//tot_compressed_size += zfi->siz;
			numentries++;
			//
			TZipFileInfo *zfinext = zfi->nxt;
			if (zfi->cextra!=0) delete[] zfi->cextra;
			delete zfi;
			zfi = zfinext;
			}
			ulg center_size = writ - pos_at_start_of_central;
			if (okay)
			{ 
				int res = putend(numentries, center_size, pos_at_start_of_central+ooffset, 0, NULL, swrite,this);
				if (res!=ZE_OK) okay=false;
				writ += 4 + ENDHEAD + 0;
			}
			if (!okay) 
				return ZR_WRITE;
		return ZR_OK;
	}





	ZRESULT lasterrorZ=ZR_OK;

	unsigned int FormatZipMessageZ(ZRESULT code, char *buf,unsigned int len)
	{ 
		if (code==ZR_RECENT) code=lasterrorZ;
		const char *msg="unknown zip result code";
		switch (code)
		{ case ZR_OK: msg="Success"; break;
		case ZR_NODUPH: msg="Culdn't duplicate handle"; break;
		case ZR_NOFILE: msg="Couldn't create/open file"; break;
		case ZR_NOALLOC: msg="Failed to allocate memory"; break;
		case ZR_WRITE: msg="Error writing to file"; break;
		case ZR_NOTFOUND: msg="File not found in the zipfile"; break;
		case ZR_MORE: msg="Still more data to unzip"; break;
		case ZR_CORRUPT: msg="Zipfile is corrupt or not a zipfile"; break;
		case ZR_READ: msg="Error reading file"; break;
		case ZR_ARGS: msg="Caller: faulty arguments"; break;
		case ZR_PARTIALUNZ: msg="Caller: the file had already been partially unzipped"; break;
		case ZR_MEMSIZE: msg="Caller: not enough space allocated for memory zipfile"; break;
		case ZR_FAILED: msg="Caller: there was a previous error"; break;
		case ZR_ENDED: msg="Caller: additions to the zip have already been ended"; break;
		case ZR_ZMODE: msg="Caller: mixing creation and opening of zip"; break;
		case ZR_NOTINITED: msg="Zip-bug: internal initialisation not completed"; break;
		case ZR_SEEK: msg="Zip-bug: trying to seek the unseekable"; break;
		case ZR_MISSIZE: msg="Zip-bug: the anticipated size turned out wrong"; break;
		case ZR_NOCHANGE: msg="Zip-bug: tried to change mind, but not allowed"; break;
		case ZR_FLATE: msg="Zip-bug: an internal error during flation"; break;
		}
		unsigned int mlen=(unsigned int)strlen(msg);
		if (buf==0 || len==0) return mlen;
		unsigned int n=mlen; if (n+1>len) n=len-1;
		strncpy(buf,msg,n); buf[n]=0;
		return mlen;
	}


	TZip *CreateZipInternal(void *z,unsigned long flags)
	{ 
		TZip *zip = new TZip();
		lasterrorZ = zip->Create(z,flags);
		if (lasterrorZ!=ZR_OK)
		{
			delete zip;
			return 0;
		}
		return zip;
	}
	CreateZip::CreateZip(const TCHAR *fn)
	{
		hz=CreateZipInternal((void*)fn,ZIP_FILENAME);
	}


	ZRESULT ZipAddInternal(TZip* zip,const TCHAR *dstzn, void *src, unsigned long flags)
	{ 
		lasterrorZ = zip->Add(dstzn,src,flags);
		return lasterrorZ;
	}
	ZRESULT CreateZip::ZipAdd(const TCHAR *dstzn, const TCHAR *fn)
	{
		return ZipAddInternal(hz,dstzn,(void*)fn,ZIP_FILENAME);
	}
	ZRESULT CreateZip::ZipAddFolder(const TCHAR *dstzn)
	{
		return ZipAddInternal(hz,dstzn,0,ZIP_FOLDER);
	}


	ZRESULT CreateZip::CloseZip()
	{ 
		if (hz==0)
		{
			lasterrorZ=ZR_ARGS;
			return ZR_ARGS;
		}
		TZip *zip = hz;
		lasterrorZ = zip->Close();
		delete zip;
		return lasterrorZ;
	}


