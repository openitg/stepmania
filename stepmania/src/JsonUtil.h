/* JasonFileUtil - . */

#ifndef JsonUtil_H
#define JsonUtil_H

class RageFileBasic;
#include "json/value.h"


namespace JsonUtil
{
	bool LoadFromString( Json::Value &root, RString sData, RString &sErrorOut );
	bool LoadFromStringShowErrors( Json::Value &root, const RString sData );
	bool LoadFromFileShowErrors( Json::Value &root, const RString &sFile );
	bool LoadFromFileShowErrors( Json::Value &root, RageFileBasic &f );

	bool WriteFile( const Json::Value &root, const RString &sFile, bool bMinified );

	template<class T>
	static void SerializeVectorObjects(const vector<T> &v, void fn(const T &, Json::Value &), Json::Value &root)
	{
		root = Json::Value(Json::arrayValue);
		root.resize( v.size() );
		for( unsigned i=0; i<v.size(); i++ )
			fn( v[i], root[i] );
	}

	template<class T>
	static void SerializeVectorPointers(const vector<const T*> &v, void fn(const T &, Json::Value &), Json::Value &root)
	{
		root = Json::Value(Json::arrayValue);
		root.resize( v.size() );
		for( unsigned i=0; i<v.size(); i++ )
			fn( *v[i], root[i] );
	}

	template<typename V, typename T>
	static void SerializeArray(const V &v, void fn(const T &, Json::Value &), Json::Value &root)
	{
		root = Json::Value(Json::arrayValue);
		root.resize( v.size() );
		int i=0;
		for( V::const_iterator iter=v.begin(); iter!=v.end(); iter++ )
			fn( *iter, root[i++] );
	}

	template <typename V>
	static void SerializeArrayValues(const V &v, Json::Value &root)
	{
		root = Json::Value(Json::arrayValue);
		root.resize( v.size() );
		int i=0;
		for( V::const_iterator iter=v.begin(); iter!=v.end(); iter++ )
			root[i++] = *iter;
	}

	template <typename V>
	static void SerializeArrayObjects(const V &v, Json::Value &root)
	{
		root = Json::Value(Json::arrayValue);
		root.resize( v.size() );
		int i=0;
		for( V::const_iterator iter=v.begin(); iter!=v.end(); iter++ )
			iter->Serialize( root[i++] );
	}

	// Serialize a map that has a non-string key type
	template <typename V>
	static void SerializeMapAsArray(const V &v, const RString &sKeyName, const RString &sValueName, Json::Value &root)
	{
		root = Json::Value(Json::arrayValue);
		root.resize( v.size() );
		int i=0;
		for( V::const_iterator iter=v.begin(); iter!=v.end(); iter++ )
		{
			Json::Value &v = root[i++];
			iter->first.Serialize( v[sKeyName] );
			iter->second.Serialize( v[sValueName] );
		}
	}

	template<class T>
	static void DeserializeVectorObjects(vector<T> &v, void fn(T &, const Json::Value &), const Json::Value &root)
	{
		v.resize( root.size() );
		for( unsigned i=0; i<v.size(); i++ )
			fn( v[i], root[i] );
	}

	template <typename V>
	static void DeserializeArrayObjects( V &v, const Json::Value &root)
	{
		v.resize( root.size() );
		for( unsigned i=0; i<v.size(); i++ )
			v[i].Deserialize( root[i] );
	}

	template<class T>
	static void DeserializeVectorPointers(vector<T*> &v, void fn(T &, const Json::Value &), const Json::Value &root)
	{
		for( unsigned i=0; i<v.size(); i++ )
			SAFE_DELETE(v[i]);		
		v.resize( root.size() );
		for( unsigned i=0; i<v.size(); i++ )
		{
			v[i] = new T;
			fn( *v[i], root[i] );
		}
	}

	template<class T>
	static void DeserializeVectorValues(vector<T> &v, const Json::Value &root)
	{
		v.resize( root.size() );
		for( unsigned i=0; i<v.size(); i++ )
			v[i] = root[i].asString();
	}
}

#endif

/*
 * (c) 2010-2010 Chris Danford
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
