////////////////////////////////////////////////////////////////////////////////
// $Workfile: ZipString.cpp $
// $Archive: /ZipArchive/ZipString.cpp $
// $Date$ $Author$
////////////////////////////////////////////////////////////////////////////////
// This source file is part of the ZipArchive library source distribution and
// is Copyright 2000-2002 by Tadeusz Dracz (http://www.artpol-software.com/)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// For the licensing details see the file License.txt
////////////////////////////////////////////////////////////////////////////////

#include "ZipString.h"
	
ZIPSTRINGCOMPARE GetCZipStrCompFunc(bool bCaseSensitive, bool bCollate)
{
	if (bCollate)
		return bCaseSensitive ? & CZipString::Collate : & CZipString::CollateNoCase;
	else
		return bCaseSensitive ? & CZipString::Compare : & CZipString::CompareNoCase;
}
