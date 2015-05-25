#ifndef __VK_API_H__
#define __VK_API_H__

#include "queryurl.h"
#ifdef WIN32
#   include <windows.h>
#else
#   ifdef UNICODE
typedef wchar_t TCHAR, *PTCHAR;
#   else
typedef char TCHAR, *PTCHAR;
#   endif
typedef unsigned HINTERNET;
typedef unsigned int DWORD, *LPDWORD;           // here's the data items we'll use....
#endif



#include <stdio.h>
#include <typeinfo>

#define RAPIDJSON_ASSERT(a)
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/rapidjson.h"

#include "ExString.h"
#include <vector>

unsigned CurCodePage;

//unsigned SendQuery(HINTERNET hInternet, PTCHAR Url, void * Buf,unsigned SizeBuf, unsigned * SizeReaded = NULL);

template<typename T, typename Encoding>
std::basic_string<T> & operator<<(std::basic_string<T> & StrDest, rapidjson::GenericValue<Encoding> & Val)
{
	switch(Val.GetType())
	{
	case rapidjson::kStringType:
		{
			std::basic_string<T> Result;
			if(typeid(T) == typeid(wchar_t))
				ConvertCodePageString(CP_UTF8, 0,Val.GetString(), Result);
			else
				ConvertCodePageString(CP_UTF8, CurCodePage,Val.GetString(), Result);
			StrDest += Result;
		}
		break;
	case rapidjson::kTrueType:
		StrDest += STR(T,"true");
		break;
	case rapidjson::kFalseType:
		StrDest += STR(T,"false");
		break;
	case rapidjson::kNumberType:
		StrDest << Val.GetInt();
		break;
	case rapidjson::kNullType:
		StrDest += STR(T, "null");
		break;
	}
	return StrDest;
}


template<typename T, typename Encoding>
std::basic_ostream<T> & operator<<(std::basic_ostream<T> & StreamDest, rapidjson::GenericValue<Encoding> & Val)
{
	std::basic_string<T> Result;
	Result << Val;
	StreamDest << Result;
	return StreamDest;
}

class VK
{
	typedef std::basic_string<char> String;

	String _Password, _Login, _LastErr, access_token;
	DWORD user_id;
	void * QueryBuf, *StartResponseData;

	unsigned SizeQueryBuf;
	unsigned CurCodePage;
	unsigned uErr;

	typedef unsigned (*EVNT_HAND)(VK & Vk, rapidjson::Value & MsgEvnt);

	static unsigned __EmptyHandler(VK & Vk, rapidjson::Value & MsgEvnt)
	{
		return 0;
	}


public:
	EVNT_HAND MessageEvent;
	EVNT_HAND FriendBecomeOnline;
	EVNT_HAND FriendBecomeOffline;
	EVNT_HAND DefaultEvnt;
	EVNT_HAND EnterTextEvent;
	EVNT_HAND AllEvent;


	void * UserData;

	VK(String & Login, String & Password, unsigned CodePage)
	{
		_Password = Password;
		_Login = Login;
		StartResponseData = QueryBuf = malloc(SizeQueryBuf = 10000);
		if(QueryBuf == NULL)
		{
			_LastErr << "Not alloc query buf\n";
			SizeQueryBuf = 0;
			uErr = 25;
		}

		CurCodePage = CodePage;
		MessageEvent = __EmptyHandler;
		FriendBecomeOnline = __EmptyHandler;
		FriendBecomeOffline = __EmptyHandler;
		DefaultEvnt = __EmptyHandler;
		EnterTextEvent = __EmptyHandler;
		AllEvent = __EmptyHandler;
	}

	~VK()
	{
		if(QueryBuf != NULL)
			free(QueryBuf);
	}

	String & GetLastErr()
	{
		return _LastErr;
	}

	DWORD GetCurrentUsreId()
	{
		return user_id;
	}


	unsigned Login()
	{
		_LastErr.clear();
		rapidjson::Document ResponseJSON;
		unsigned Result = 0;
		String LoginRequest;
		LoginRequest << "https://oauth.vk.com/token?grant_type=password&client_id=3697615&client_secret=AlVXZFMUqyrnABp8ncuU&username="
			<< _Login << "&password=" << _Password;

		//stringsprintf(LoginRequest,1000,TEXT("https://oauth.vk.com/authorize?client_id=3697615&scope=notify,friends,photos,audio,video,docs,notes,pages,status,offers,questions,wall,groups,messages,notifications,stats,ads,offline&redirect_uri=http://api.vk.com/blank.html&display=page&response_type=token"), Login, Password);


		if(!SendQuery(LoginRequest))
			return uErr;

		ResponseJSON.Parse((char*)StartResponseData);
		if(!ResponseJSON["error"].IsNull())
		{
			_LastErr << "You login or password inncorrect. Server message: " << ResponseJSON["error_description"] << "\n";
			return 3;
		}
		rapidjson::Value& vAccessToken = ResponseJSON["access_token"];

		if(!vAccessToken.IsString())
		{
			_LastErr << "Unknown server response\n";
			return 4;
		}
		access_token.clear();
		access_token << vAccessToken;

		rapidjson::Value& vUserId = ResponseJSON["user_id"];
		if(vUserId.IsNumber())
			user_id = vUserId.GetInt();
		return Result;
	}


	unsigned SendMessage(String & Message, String & Title, DWORD IdGetter)
	{
		_LastErr.clear();
		unsigned CountTries = 0;
		String ResUrl;
		for(; CountTries < 2; CountTries++)
		{
			String ConvertedMsg;
			CodeUrl(Message, ConvertedMsg, CurCodePage);
			ResUrl.clear();
			ResUrl << "https://api.vk.com/method/messages.send?user_id=" << IdGetter
				<< "&access_token=" << access_token << "&v=5.28&message=" << ConvertedMsg;
			if(Title.length() != 0)
			{
				CodeUrl(Title, ConvertedMsg);
				ResUrl << "&title=" << ConvertedMsg;
			}
			if(!SendQuery(ResUrl))
				return uErr;
			rapidjson::Document ResponseJSON;
			ResponseJSON.Parse((char*)StartResponseData);
			rapidjson::Value& s = ResponseJSON["error"];
			unsigned Result;
			if(!s.IsNull())
			{
				if(s["error_code"] == 10)
				{
					if((Result = Login()) != 0)
						return Result;
					continue;
				}
				else
					_LastErr << "Server returned error " << s["error_code"] << " with message: " << s["error_msg"] << "\n";
			}
			break;
		}

		return 0;
	}

	unsigned GetUserName(DWORD UserId, String & Name)
	{
		_LastErr.clear();
		String Url;
		Url << "https://api.vk.com/method/users.get?&user_ids=" << UserId;
		if(!SendQuery(Url))
			return uErr;

		rapidjson::Document ResponseJSON;
		ResponseJSON.Parse((char*)StartResponseData);

		if(ResponseJSON["error"].IsObject())
		{
			_LastErr << "Server returned error " << ResponseJSON["error"]["error_code"] << " with message: " << ResponseJSON["error"]["error_msg"] << "\n";
			return 9;
		}
		rapidjson::Value & v = ResponseJSON["response"][0];
		Name << v["first_name"] << " " << v["last_name"];
		return 0;
	}

	unsigned WaitMessages(bool isLoop)
	{
		_LastErr.clear();
		String Url;
		rapidjson::Document ResponseJSON;
		unsigned Result = 0;

TryGetLongPoolInfo:

		Url << "https://api.vk.com/method/messages.getLongPollServer?&access_token=" << access_token << "&use_ssl=1&need_pts=0";
		if(!SendQuery(Url))
			return uErr;

		ResponseJSON.Parse((char*)StartResponseData);
		rapidjson::Value& s = ResponseJSON["error"];
		if(s.IsObject())
		{
			if(s["error_code"] == 10)
			{
				if((Result = Login()) != 0)
					return Result;
				goto TryGetLongPoolInfo;
			}
			else
				_LastErr << "Server returned error " << s["error_code"] << " with message: " << s["error_msg"] << "\n";
		}

		rapidjson::Value& ResponseObj = ResponseJSON["response"];
		if(!ResponseObj.IsObject())
		{
			_LastErr << "Unicknown server response.\n";
			return 20;
		}

		rapidjson::Value& SessionKey = ResponseObj["key"];
		if(!SessionKey.IsString())
		{
			_LastErr << "Unicknown server response.\n";
			return 20;
		}
		String LongPoolKey;
		LongPoolKey << SessionKey;

		rapidjson::Value & LongPoolSrv = ResponseObj["server"];
		if(!LongPoolSrv.IsString())
		{
			_LastErr << "Unicknown server response.\n";
			return 20;
		}
		String LongPoolServer;
		LongPoolServer << LongPoolSrv;

		unsigned LongPoolTs;
		rapidjson::Value & LngPoolTs = ResponseObj["ts"];
		if(!LngPoolTs.IsNumber())
		{
			_LastErr << "Unicknown server response.\n";
			return 20;
		}
		LongPoolTs = LngPoolTs.GetInt();


		QUERY_URL LongPoolConnect(LongPoolServer, 443);
		if(LongPoolConnect.GetLastErr())
		{
			_LastErr << LongPoolConnect.GetLastErrMsg();
			return LongPoolConnect.GetLastErr();
		}
		char * q = strstr((char*)LongPoolServer.c_str(),"/");
		String QueryForLongPool, HttpBody;
		String Return;
		for(;;)
		{
			QueryForLongPool.clear();
			HttpBody.clear();
			HttpBody << "act=a_check&key=" << LongPoolKey << "&ts=" << LongPoolTs << "&wait=25&mode=2";

			CreatePostHttpQueryForLongPool(QueryForLongPool, LongPoolConnect.GetHostName(),q,HttpBody);
			unsigned SizeReaded = 0;
			if(!LongPoolConnect.SendAndTakeQuery(QueryForLongPool, QueryBuf, SizeQueryBuf, &SizeReaded))
			{
				_LastErr << LongPoolConnect.GetLastErrMsg();
				return LongPoolConnect.GetLastErr();
			}

			((char*)QueryBuf)[SizeReaded] = '\0';
			StartResponseData = strstr((char*)QueryBuf, "\r\n\r\n");
			if(StartResponseData != NULL)
				StartResponseData = (char*)StartResponseData + 4;
			else
				StartResponseData = QueryBuf;
			rapidjson::Document longPoolResponse;
			longPoolResponse.Parse((char*)StartResponseData);
			rapidjson::Value & LngPoolTs = longPoolResponse["ts"];
			if(!LngPoolTs.IsNumber())
			{
				_LastErr << "Unicknown server response.\n";
				return 20;
			}
			LongPoolTs = LngPoolTs.GetInt();
			rapidjson::Value & LngPoolUpdates = longPoolResponse["updates"];
			if(LngPoolUpdates.Empty())
				continue;
			for(unsigned CurEvent = 0, CountEvent = LngPoolUpdates.Size(); CurEvent < CountEvent; CurEvent++)
			{
				rapidjson::Value & ArrEvent = LngPoolUpdates[CurEvent];
				unsigned r = 0;
				//Проверяем 0й эхлемент массива событий
				r = AllEvent(*this ,ArrEvent);
				if(r != 0)
					return r;
				switch(ArrEvent[0].GetInt())
				{
				case 4://Если пришло сообщение
					////4,$message_id,$flags,$from_id,$timestamp,$subject,$text,$attachments — добавление нового сообщения
					r = MessageEvent(*this ,ArrEvent);
					break;
				case 8:
					r = FriendBecomeOnline(*this ,ArrEvent);
					break;
				case 9:
					r = FriendBecomeOffline(*this ,ArrEvent);
					break;
				case 61:
					r = EnterTextEvent(*this ,ArrEvent);
					break;
				default:
					r = DefaultEvnt(*this ,ArrEvent);
				}
				if(r != 0)
					return r;
			}


		}
		return 0;
	}
	//Server response example: {"response":{"key":"24afec4a0d7821e64141d827c51c41f6861d94c8","server":"imv4.vk.com\/im0602","ts":1780739490}}
	//{"ts":1780739520,"updates":[]}
	template<typename T1, typename T2, typename T3, typename T4>
	void CreatePostHttpQueryForLongPool(T1 & strHttpRequest, T2 & HostName, T3 & Query, T4 & Body)
	{

		//Head
		strHttpRequest << "POST " << Query << " HTTP/1.1\r\n";
		strHttpRequest << "Host: " << HostName << "\r\n";
		strHttpRequest << "Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n";
		strHttpRequest << "Content-Length: " << Body.length() << "\r\n";
		strHttpRequest << "Connection: keep-alive\r\n";
		strHttpRequest << "Pragma: no-cache\r\n";
		strHttpRequest << "Cache-Control: no-cache\r\n";

		strHttpRequest << "\r\n";

		//Body
		strHttpRequest << Body;
	}


private:

	bool SendQuery(String & Url,unsigned * SizeReaded = NULL)
	{
		char * Err;
		StartResponseData = QUERY_URL::SingleSendAndGetQuery(Url,QueryBuf, SizeQueryBuf, SizeReaded, &Err);
		if(StartResponseData == NULL)
		{
			_LastErr = Err;
			return false;
		}
		_LastErr.clear();
		return true;
	}
};





#endif
