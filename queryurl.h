#ifndef QUERYURL_H_INCLUDED
#define QUERYURL_H_INCLUDED


#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "ExString.h"


class QUERY_URL
{

	typedef std::basic_string<char> String;
	String _LastErr, HostName;
	unsigned uErr, PortionSize;
	unsigned short _Port;
	void * host_info_list;
	int hSocket;

	SSL_CTX *ctx; //For https connection
	SSL *ssl;     //For https connection

#ifdef WIN32
	static void * lpWSAData;
#endif

	static int StartWsa();

	char * Init(char * Host, unsigned short Port);

	QUERY_URL(){}
public:

	static void EndWsa();

	QUERY_URL(String & Host, unsigned short Port = 0)
	{
	   Init((char*)Host.c_str(), Port);
	}

	QUERY_URL(char * Host, unsigned short Port = 0)
	{
	   Init(Host, Port);
	}

	~QUERY_URL();

	inline bool SendQuery(const String & InQuery)
	{
		return SendQuery((void*)InQuery.c_str(), InQuery.length());
	}

	bool SendQuery(const void * QueryBuf, unsigned SizeBuf);

	bool TakeResponse(void * Buf, unsigned SizeBuf, unsigned * SizeReaded = NULL);

	bool TakeResponse(QUERY_URL::String & StrBuf);

	inline bool SendAndTakeQuery(void * SendBuf, unsigned SizeSendBuf, void * Buf, unsigned SizeBuf, unsigned * SizeReaded = NULL)
	{
		if(!SendQuery(SendBuf,SizeSendBuf))
			return false;
		return TakeResponse(Buf,SizeBuf,SizeReaded);
	}

	inline bool SendAndTakeQuery(String & strQuery, void * Buf, unsigned SizeBuf, unsigned * SizeReaded = NULL)
	{
		if(!SendQuery((void*)strQuery.c_str(), strQuery.length()))
			return false;
		return TakeResponse(Buf, SizeBuf, SizeReaded);
	}

	inline bool SendAndTakeQuery(void * SendBuf, unsigned SizeSendBuf, String & Result)
	{
		if(!SendQuery(SendBuf,SizeSendBuf))
			return false;
		return TakeResponse(Result);
	}

	inline bool SendAndTakeQuery(char * SendStr, String & Result)
	{
		if(!SendQuery(SendStr,strlen(SendStr)))
			return false;
		return TakeResponse(Result);
	}

	inline bool SendAndTakeQuery(char * SendStr, void * Buf, unsigned SizeBuf, unsigned * SizeReaded = NULL)
	{
		if(!SendQuery(SendStr,strlen(SendStr)))
			return false;
		return TakeResponse(Buf,SizeBuf,SizeReaded);
	}


	inline bool SendAndTakeQuery(String & strQuery, String & Result)
	{
		if(!SendQuery((void*)strQuery.c_str(), strQuery.length()))
			return false;
		return TakeResponse(Result);
	}

	inline String & GetLastErrMsg()
	{
		return _LastErr;
	}

	inline String & GetHostName()
	{
		return HostName;
	}

	inline void SetPortionSize(unsigned NewSize)
	{
		PortionSize = NewSize;
	}

	inline unsigned short GetPort()
	{
		return _Port;
	}

	inline unsigned GetLastErr()
	{
		return uErr;
	}

	inline void ClearErr()
	{
		uErr = 0;
		_LastErr.clear();
	}

	QUERY_URL & operator<<(const String & Val)
	{
        SendQuery(Val);
		return *this;
	}

	QUERY_URL & operator<<(const char * Val)
	{
        SendQuery(Val, strlen(Val));
		return *this;
	}

	QUERY_URL & operator<<(int Val)
	{
		char Buf[200];
		itoa(Val,Buf,10);
        SendQuery(Buf, strlen(Buf));
		return *this;
	}

	QUERY_URL & operator>>(String & Val)
	{
        TakeResponse(Val);
		return *this;
	}

	static void * SingleSendAndGetQuery(std::string & Url,void * QueryBuf, unsigned SizeQueryBuf, unsigned * SizeReaded = NULL, char ** Err = NULL);

};



#endif // QUERYURL_H_INCLUDED
