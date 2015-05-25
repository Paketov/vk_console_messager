

#ifdef WIN32
#	include <winsock2.h>
#	include <ws2tcpip.h>

#	ifndef WSA_WERSION
#		define WSA_WERSION MAKEWORD(2, 2)
#	endif
#pragma comment(lib, "Ws2_32.lib")

#else
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <netinet/in.h>
#   include <arpa/inet.h>
#   include <netdb.h>
#   include <unistd.h>
#	define closesocket(socket)  close(socket)
#endif

#include "queryurl.h"


#ifdef WIN32
void * QUERY_URL::lpWSAData = NULL;
#endif


#define SET_CHAR_POINTER(Pointer, Val) ((Pointer)?((*Pointer) = Val):(Val))



int QUERY_URL::StartWsa()
{
#ifdef WIN32
	if(lpWSAData == NULL)
	{
		lpWSAData = malloc(sizeof(WSADATA));
		return WSAStartup(WSA_WERSION, (LPWSADATA)lpWSAData);
	}
#endif
	return 0;
}


void QUERY_URL::EndWsa()
{
#ifdef WIN32
	if(lpWSAData != NULL)
	{
		free(lpWSAData);
		WSACleanup();
	}
#endif
}

char * QUERY_URL::Init(char * Host, unsigned short Port)
{

	ctx = NULL;
	ssl = NULL;
	hSocket = -1;
	host_info_list = NULL;
	uErr = 0;

	char * _Pos;
	int Pos;
	char Buf[50], *NamePort;
	PortionSize = 500;
	if(Port != 0)
		itoa(_Port = Port,NamePort = Buf,10);
	else
		_Port = 0;
	
	if((_Pos = strstr(Host, "://")) != NULL)
	{
		if(strncmp(Host, "https",5) == 0)
		{
			NamePort = "443";
			_Port = 443;
		}
		else
		{
			NamePort = "80";
			_Port = 80;
		}
		_Pos += 3;
	}else 
	{
		if(_Port == 0)
		{
			NamePort = "80";
			_Port = 80;
		}
		_Pos = Host;
	}

	sscanf(_Pos,"%*[^/:]%n%*c%hu", &Pos, &_Port);

	HostName.append(_Pos, Pos);

	if(HostName.empty())
	{
		uErr = 1;
		_LastErr = "Invalid host name";
		return NULL;
	}

	if(StartWsa() != 0) 
	{
		uErr = 15;
		_LastErr = "Dont start wsa";
		return NULL;
	}

	struct addrinfo host_info = {0};
	host_info.ai_socktype = SOCK_STREAM;
	host_info.ai_family = AF_INET;

	if(getaddrinfo(HostName.c_str(), NamePort, &host_info, (addrinfo**)&host_info_list) != 0)
	{
		uErr = 2;
		_LastErr = "Dont get server info";
		goto ErrOut;
	}

	hSocket = socket(((addrinfo*)host_info_list)->ai_family, ((addrinfo*)host_info_list)->ai_socktype, ((addrinfo*)host_info_list)->ai_protocol);

	if(hSocket < 0)
	{
		uErr = 3;
		_LastErr = "Not open internet driver";
		goto ErrOut;
	}
	if(connect(hSocket, ((addrinfo*)host_info_list)->ai_addr, ((addrinfo*)host_info_list)->ai_addrlen) < 0) // установка соединения с сервером
	{
		uErr = 4;
		_LastErr = "Cannot connect with server";
ErrOut:

		freeaddrinfo(((addrinfo*)host_info_list));
		host_info_list = NULL;
		closesocket(hSocket);
		hSocket = -1;
		return NULL;
	}
	if(_Port == 443)
	{
		SSL_load_error_strings();
		SSL_library_init();

		ctx = SSL_CTX_new(SSLv23_client_method());
		ssl = SSL_new(ctx);

		if(SSL_set_fd(ssl, hSocket) == 0)
		{
			uErr = 14;
			_LastErr = "SSL not connect with socket";
			goto SSLErrOut;
		}
		if(SSL_connect(ssl) < 0)
		{
			uErr = 5;
			_LastErr = "SSL not connect with socket";
SSLErrOut:
			SSL_shutdown(ssl);
			SSL_free(ssl);
			ssl = NULL;
			SSL_CTX_free(ctx);
			ctx = NULL;
			freeaddrinfo(((addrinfo*)host_info_list));
			host_info_list = NULL;
			closesocket(hSocket);
			hSocket = -1;
		}
	}
	return _Pos + Pos; 
}

QUERY_URL::~QUERY_URL()
{
	if(ssl)
	{
		SSL_shutdown(ssl);
		SSL_free(ssl);
	}
	if(ctx)
		SSL_CTX_free(ctx);
	if(host_info_list)
		freeaddrinfo(((addrinfo*)host_info_list));
	if(hSocket != -1)
		closesocket(hSocket);
}


bool QUERY_URL::SendQuery(const void * QueryBuf, unsigned SizeBuf)
{
	if(ssl != NULL)
	{
		if(SSL_write(ssl, QueryBuf,SizeBuf) < 0)
		{
			uErr = 7;
			_LastErr = "Not send https query";
			return false;
		}
	}else if(send(hSocket, (char*)QueryBuf,SizeBuf, 0) < 0)
	{
			uErr = 8;
			_LastErr = "Not send http query";
			return false;
	}
	return true;
}

bool QUERY_URL::TakeResponse(void * Buf, unsigned SizeBuf, unsigned * SizeReaded)
{
	int ReadedSize;
	if(ssl != NULL)
	{
		if((ReadedSize = SSL_read(ssl, Buf, SizeBuf)) < 0)
		{
			uErr = 9;
			_LastErr = "Not get data from https server";
			return false;
		}
	}else if((ReadedSize = recv(hSocket, (char*)Buf, SizeBuf, 0)) == -1)
	{
			uErr = 10;
			_LastErr = "Not get data from http server";
			return false;
	}
	if(SizeReaded != NULL)
		*SizeReaded = ReadedSize;
	return true;
}

bool QUERY_URL::TakeResponse(QUERY_URL::String & StrBuf)
{
	uErr = 0;
	if(StrBuf.capacity() < PortionSize)
		StrBuf.resize(PortionSize);

	char * Buf = (char*)StrBuf.c_str();
	unsigned CurSize = 0;

	if(ssl != NULL)
	{
		while(true)
		{
			int ReadedSize = SSL_read(ssl, Buf, PortionSize);
			if(ReadedSize < 0)
			{
				uErr = 9;
				_LastErr = "Not get data from https server";
				return false;
			}else if(ReadedSize == 0)
				break;
			else
			{
				CurSize += ReadedSize;
				StrBuf.resize(CurSize + PortionSize);
				Buf = (char*)StrBuf.c_str() + CurSize;  
			}
		}
	}else
	{
		while(true)
		{
			int ReadedSize = recv(hSocket, Buf, PortionSize, 0);
			if(ReadedSize == -1)
			{
				uErr = 10;
				_LastErr = "Not get data from http server";
				return false;
			}else if(ReadedSize == 0)
				break;
			else
			{
				CurSize += ReadedSize;
				StrBuf.resize(CurSize + PortionSize);
				Buf = (char*)StrBuf.c_str() + CurSize;  
			}
		}
	}
	*Buf = '\0';
	return true;
}



void * QUERY_URL::SingleSendAndGetQuery(std::string & Url, void * QueryBuf, unsigned SizeQueryBuf, unsigned * SizeReaded, char ** Err)
{

	QUERY_URL CurQuery;
	char * c;
	if((c = CurQuery.Init((char*)Url.c_str(),0))== NULL)
	{
		*Err = (char*)CurQuery.GetLastErrMsg().c_str();
		return NULL;
	}
	if(*c == '\0')
		c = "/";

	std::string strHttpRequest;
	strHttpRequest << "GET " << c << " HTTP/1.0\r\nHost: " << CurQuery.GetHostName() << "\r\n\r\n";
	unsigned sr;
	if(!CurQuery.SendAndTakeQuery(strHttpRequest,QueryBuf,SizeQueryBuf,&sr))
	{
		*Err = (char*)CurQuery.GetLastErrMsg().c_str();
		return NULL;
	}
	((char*)QueryBuf)[sr] = '\0';
	if(SizeReaded != NULL)
		*SizeReaded = sr;

	c = strstr((char*)QueryBuf,"\r\n\r\n");
	if(c == NULL)
		return (char*)QueryBuf + strlen((char*)QueryBuf);
	else
		return c + 4;
}

