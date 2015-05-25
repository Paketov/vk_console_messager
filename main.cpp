
#include "vk_api.h"

#ifdef WIN32
#include <windows.h>
#endif


#include <stdio.h>

#include "ExString.h"




typedef std::basic_string<char> String;



String Login;
String Password;
String access_token;
DWORD user_id = 0;
DWORD DefaultUserId= 0;
void * QueryBuf;
unsigned SizeQueryBuf;


#define SKIP_SPACE(Cur) for(;(*Cur == ' ' && *Cur != '\0');Cur++)

struct WAIT_MSG_PARAMS
{
    DWORD FilterId;
    DWORD ShowName;
    DWORD IsLoop;
};

PTCHAR GetCommandParametr(PTCHAR Cur,PTCHAR  Parametr, PTCHAR FormatParametr,void * ParametrBuf);


unsigned MassageHandler(VK & Vk, rapidjson::Value & MsgEvnt)
{
    if(((WAIT_MSG_PARAMS*)Vk.UserData)->FilterId)
    {
        if(MsgEvnt[3].GetInt() == ((WAIT_MSG_PARAMS*)Vk.UserData)->FilterId)
            std::cout << MsgEvnt[6] << "\n";
    }
    else if(((WAIT_MSG_PARAMS*)Vk.UserData)->ShowName)
    {
        String Name;
        Vk.GetUserName(MsgEvnt[3].GetInt(),Name);
        std::cout << "Get message from: " << Name << "\nText: " << MsgEvnt[6] << "\n";
    }
    else
        std::cout << "Get message from id: " << MsgEvnt[3] << "\nText: " << MsgEvnt[6] << "\n";

    if(((WAIT_MSG_PARAMS*)Vk.UserData)->IsLoop == 0)
      return 1;
    return 0;
}

int main(int argc, char * argv[])
{
    unsigned CurCodePage = 0;
#ifdef WIN32
    CurCodePage = 1251;
    SetConsoleCP(CurCodePage);
    SetConsoleOutputCP(CurCodePage);
    setlocale(LC_ALL, ".1251");
#else
    CurCodePage = CP_UTF8;
#endif

    if(argc <= 1)
    {
        std::cout << "Enter parametrs: vk_messager login:YOURLOGIN password:YOURPASSWORD\n";
        return 1;
    }

    for(unsigned short i = 1; i < argc; i++)
    {
        if(strstr(argv[i], "login=") == argv[i])
        {
            Login << (argv[i] + 6);
        }
        else if(strstr(argv[i], "password=") == argv[i])
        {
            Password << (argv[i] + 9);
        }
        else if(strstr(argv[i], "default_user_id=") == argv[i])
        {
            sscanf(argv[i] + 16, "%u", &DefaultUserId);
        }
        else
        {
            std::wcerr << "Bad parametr:" << argv[i];
            return 2;
        }
    }

    VK VkSession(Login, Password, CurCodePage);

	unsigned Result;
    if((Result = VkSession.Login()) != 0)
    {
        std::cerr << VkSession.GetLastErr();
        return Result;
    }

    VkSession.MessageEvent = MassageHandler;


    /*
    msg [user_id="<user who receives the message>"] [title="<Title for message>"] <Youre message>
    set_user_id <id user default>
    wait_msg [loop=<0|1>] [id_filter=<user_id_filter>] [show_name=1]
    */

    String strCommand, strParametrs;
    VkSession.GetUserName(VkSession.GetCurrentUsreId(), strParametrs);

    std::cout << "User " << strParametrs << " was logged\n";
    for(;;)
    {
        std::cout << "VK>";
        std::cin >> strCommand;
        if(strCommand.length() == 0)
            continue;

        std::getline(std::cin, strParametrs);
        if(strCommand == "set_user_id")			//
        {
            sscanf(strParametrs.c_str(), "%u", &DefaultUserId);
            if(DefaultUserId == 0)
                std::cerr << "Bad command, try again.\n";
            continue;
        }
        else if(strCommand == "wait_msg")		     //Wait message
        {
            PTCHAR Cur = &strParametrs[0];

            WAIT_MSG_PARAMS Param = {0};
            PTCHAR TempCur;
GetWaitParams:
            if((TempCur = GetCommandParametr(Cur, "id_filter", "%u", &Param.FilterId)) != NULL)
            {
                Cur = TempCur;
                goto GetWaitParams;
            }
            else if((TempCur = GetCommandParametr(Cur, "loop", "%u", &Param.IsLoop)) != NULL)
            {
                Cur = TempCur;
                goto GetWaitParams;
            }
            else if((TempCur = GetCommandParametr(Cur, "show_name", "%u",&Param.ShowName)) != NULL)
            {
                Cur = TempCur;
                goto GetWaitParams;
            }

            VkSession.UserData = &Param;
            if(VkSession.WaitMessages(Param.IsLoop) != 0)
                std::cerr << VkSession.GetLastErr();

        }
        else if(strCommand == "msg")				//Send Message
        {
            PTCHAR Cur = &strParametrs[0];
            SKIP_SPACE(Cur);
            if(*Cur == '\0')
            {
                std::cerr << "Bad command, try again.\n";
                continue;
            }
            PTCHAR TempCur;
            String Title;
            DWORD IdGetter = 0;
            unsigned SizeBuf;
GetMsgParams:
            if((TempCur = GetCommandParametr(Cur, "user_id", "%u", &IdGetter)) != NULL)
            {
                Cur = TempCur;
                goto GetMsgParams;
            }
            else if((TempCur = GetCommandParametr(Cur, "title", "%*[^\"]%n", &SizeBuf)) != NULL)
            {
                Title.resize(SizeBuf);
                GetCommandParametr(Cur, "title", "%[^\"]", &Title[0]);
                Cur = TempCur;
                goto GetMsgParams;
            }

            SKIP_SPACE(Cur);
            if((IdGetter == 0) && (DefaultUserId != 0))
                IdGetter = DefaultUserId;
            else
            {
                std::cerr << "Bad command, need user id. Try again.\n";
                continue;
            }
            String Msg(Cur);
            unsigned Result = VkSession.SendMessage(Msg, Title, IdGetter);
            if(Result != 0)
            {
                std::cerr << VkSession.GetLastErr();
                return Result;
            }
        }
        else
        {
            std::cerr << "Unknown command.\n";
            continue;
        }
    }
	QUERY_URL::EndWsa();
    return 0;
}



PTCHAR GetCommandParametr(PTCHAR Cur,PTCHAR  Parametr, PTCHAR FormatParametr,void * ParametrBuf)
{
    SKIP_SPACE(Cur);
    if(strstr(Cur, Parametr) != Cur)
        return NULL;
    Cur += strlen(Parametr);
    if(*Cur != '=')
        return NULL;
    Cur++;
    if(*Cur == '\"')
    {
        Cur++;
        PTCHAR NextQuotes = strchr(Cur,'\"');
        if(NextQuotes == NULL)
            return NULL;
        *NextQuotes = '\0';
        sscanf(Cur,FormatParametr, ParametrBuf);
        *NextQuotes = '\"';
        Cur = NextQuotes + 1;
    }
    else
    {
        TCHAR FormatBuf[100];
        strcpy(FormatBuf,FormatParametr);
        strcat(FormatBuf, "%n");
        int CountReaded;
        if(sscanf(Cur,FormatBuf, ParametrBuf, &CountReaded) == 0)
            return NULL;
        Cur += CountReaded;
    }
    return Cur;
}


