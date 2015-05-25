#ifndef __EXSTRING_H__
#define __EXSTRING_H__

#include <string>
#include <sstream>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <wchar.h>
#include <typeinfo>

#ifndef WIN32
#       include <cerrno>
#       ifndef ANDROID
#           include <iconv.h>
#       endif

#       define CP_UTF7                   3       // UTF-7 translation
#       define CP_UTF8                   0       // UTF-8 translation
#       define CP_1251                   1
#       define CP_KOI8                   2

#endif


#define STR(c,Str) ((typeid(c) == typeid(wchar_t))?((c*)L ## Str):((c*)Str))


template<typename OutString>
void ConvertCodePageString(unsigned InCp, unsigned OutCp, const wchar_t * InStr, OutString & OutStr)
{
	std::basic_string<wchar_t> TmpStr(InStr);
	ConvertCodePageString(InCp, OutCp, TmpStr, OutStr);
}

template<typename InString, typename OutString>
void ConvertCodePageString(unsigned InCp, unsigned OutCp, const InString & InStr, OutString & OutStr)
{
	typedef typename InString::value_type _InCharType;
	typedef typename OutString::value_type _OutCharType;
#ifndef ANDROID
#ifndef WIN32
	const char * CodePageStr[] =
	{
		"UTF-8",
		"CP1251",
		"KOI-8",
		"UTF-7"
	};
	iconv_t convert_hnd = (iconv_t)-1;
	size_t SizeInBuf,  SizeOutBuf;
#endif

	if((typeid(_InCharType) == typeid(wchar_t)) && (typeid(_OutCharType) == typeid(wchar_t)))
	{
		OutStr = (_OutCharType*)InStr.c_str();
		return;
	}
	else if(typeid(_InCharType) == typeid(wchar_t))
	{
#ifdef WIN32
		unsigned utf8_size = WideCharToMultiByte(OutCp, 0, (LPCWSTR)InStr.c_str(),InStr.length(), NULL, 0,NULL, NULL);
		OutStr.resize(utf8_size);
		WideCharToMultiByte(OutCp, 0, (LPCWSTR)InStr.c_str(),InStr.length(), (LPSTR)&OutStr[0], utf8_size,NULL, NULL);
#else
		convert_hnd = iconv_open(CodePageStr[OutCp], "WCHAR_T");
		if(convert_hnd == (iconv_t)-1)
			return;
		SizeInBuf = (SizeOutBuf = InStr.length() + 1) * sizeof(_InCharType);
		OutStr.resize(SizeOutBuf);
		SizeOutBuf *= sizeof(_OutCharType);
#endif
	}
	else if(typeid(_OutCharType) == typeid(wchar_t))
	{

#ifdef WIN32
		unsigned size = MultiByteToWideChar(InCp, 0, (LPCSTR)InStr.c_str(),InStr.length(), NULL, 0);
		OutStr.resize(size);
		MultiByteToWideChar(InCp, 0, (LPCSTR)InStr.c_str(), InStr.length(), (LPWSTR)&OutStr[0], size);
#else
		convert_hnd = iconv_open("WCHAR_T",CodePageStr[InCp] );
		if(convert_hnd == (iconv_t)-1)
			return;
		SizeInBuf = (InStr.length() + 1) * sizeof(_InCharType);
		OutStr.resize(SizeInBuf);
		SizeOutBuf = SizeInBuf * sizeof(_OutCharType);
#endif
	}
	else
	{
		if(InCp == OutCp)
		{
			OutStr = (_OutCharType*)InStr.c_str();
			return;
		}
#ifdef WIN32
		unsigned size = MultiByteToWideChar(InCp, 0, (LPCSTR)InStr.c_str(),InStr.length(), NULL, 0);
		std::wstring unicode_str(size, '\0');
		MultiByteToWideChar(InCp, 0, (LPCSTR)InStr.c_str(), InStr.length(), &unicode_str[0], size);

		int utf8_size = WideCharToMultiByte(OutCp, 0, unicode_str.c_str(),unicode_str.length(), NULL, 0,NULL, NULL);
		OutStr.resize(utf8_size);
		WideCharToMultiByte(OutCp, 0, unicode_str.c_str(),unicode_str.length(),(LPSTR)&OutStr[0], utf8_size,NULL, NULL);
#else
		convert_hnd = iconv_open(CodePageStr[OutCp], CodePageStr[InCp]);
		if(convert_hnd == (iconv_t)-1)
			return;
		SizeInBuf = (InStr.length() + 1) * sizeof(_InCharType);
		OutStr.resize(SizeInBuf);
		SizeOutBuf = SizeInBuf * sizeof(_OutCharType);
#endif
	}

#ifndef WIN32

	for(char * InBuf = (char*)InStr.c_str(), *OutBuf = (char*)OutStr.c_str(); SizeInBuf != 0;)
	{
		if(iconv(convert_hnd, &InBuf, &SizeInBuf, &OutBuf, &SizeOutBuf) == (size_t)-1)
		{
			if(errno == 7)
			{
				unsigned j = (unsigned)OutBuf - (unsigned)OutStr.c_str();
				OutStr.resize(OutStr.length() + 20);
				SizeOutBuf = 20 * sizeof(_OutCharType);
				OutBuf = (char*)OutStr.c_str() + j;
			}
			else if(SizeInBuf == 0)
				break;
			else
			{
				OutStr.clear();
				break;
			}
		}
	}

	iconv_close(convert_hnd);
#endif
#else
	OutStr = (_OutCharType*)InStr.c_str();
#endif

}

template<typename OutString>
void ConvertCodePageString(unsigned InCp, unsigned OutCp, const char * InStr, OutString & OutStr)
{
	std::basic_string<char> TmpStr(InStr);
	ConvertCodePageString(InCp, OutCp, TmpStr, OutStr);
}

template<typename T>
std::basic_string<T> & operator<<(std::basic_string<T> & StrDest, const int Val)
{
	T Buf[200];
	if(typeid(T) == typeid(wchar_t))
		swprintf((wchar_t*)Buf,200,L"%i", Val);
	else
		sprintf((char*)Buf,"%i", Val);
	StrDest += Buf;
	return StrDest;
}

template<typename T>
std::basic_string<T> & operator<<(std::basic_string<T> & StrDest, std::basic_string<T> & Val)
{
	StrDest += Val;
	return StrDest;
}

template<typename T>
std::basic_string<T> & operator<<(std::basic_string<T> & StrDest,const T * Val)
{
	StrDest += Val;
	return StrDest;
}

template<typename T>
int & operator<<(int & Dest, std::basic_string<T> & InStr)
{
	if(typeid(T) == typeid(wchar_t))
		swscanf((wchar_t*)InStr.c_str(),L"%i", &Dest);
	else
		sscanf((char*)InStr.c_str(),"%i", &Dest);
	return Dest;
}

template<typename _Elem, typename _Traits = std::char_traits<_Elem>, typename _Ax = std::allocator<_Elem> >
class ex_basic_string: public std::basic_string<_Elem,_Traits,_Ax>
{
public:
	template<typename Tp>
	inline ex_basic_string(Tp Val) : std::basic_string<_Elem,_Traits,_Ax>::basic_string(Val) {}

	template<typename Tp, typename Tp2>
	inline ex_basic_string(Tp Val, Tp2 Val2) : std::basic_string<_Elem,_Traits,_Ax>::basic_string(Val, Val2) {}

	inline ex_basic_string() {};

};

template<typename InString, typename OutString>
void CodeUrl(const InString & InStr, OutString & OutStr, unsigned InCodePage = CP_UTF8)
{
	typedef typename OutString::value_type _InCharType;
	if(typeid(_InCharType) == typeid(wchar_t))
	{
		std::string a;
		ConvertCodePageString(0,CP_UTF8,InStr, a);
		unsigned SizeOutStr = a.length() * 3;
		OutStr.resize(SizeOutStr);
		if(typeid(_InCharType) == typeid(wchar_t))
		{
			for(unsigned i = 0, e = a.length(); i < e; i++)
				swprintf((wchar_t*)&OutStr[i * 3],4,L"%%%02x",a[i] & 0xff);
		}
		else
		{
			for(unsigned i = 0, e = a.length(); i < e; i++)
				sprintf((char*)&OutStr[i * 3],"%%%02x",a[i] & 0xff);
		}
	}
	else
	{
		std::string a;
		if(InCodePage != CP_UTF8)
			ConvertCodePageString(InCodePage,CP_UTF8,InStr, a);
		else
			a = (char*)InStr.c_str();
		unsigned SizeOutStr = a.length() * 3;
		OutStr.resize(SizeOutStr);
		if(typeid(_InCharType) == typeid(wchar_t))
		{
			for(unsigned i = 0, e = a.length(); i < e; i++)
				swprintf((wchar_t*)&OutStr[i * 3],4,L"%%%02x",a[i] & 0xff);
		}
		else
		{
			for(unsigned i = 0, e = a.length(); i < e; i++)
				sprintf((char*)&OutStr[i * 3],"%%%02x",a[i] & 0xff);
		}
	}
}

#endif
