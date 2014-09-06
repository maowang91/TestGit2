#include "ProxyDataCollector.h"
using namespace  std;

//windows xpϵͳ��һ���������ܴ�����GDI����������Ĭ�����ֵ
#define MAX_PROCESS_GDI_COUNT 10000
//windows xpϵͳ��һ���������ܴ�����HANDLE����������Ĭ�����ֵ
#define MAX_PROCESS_HANDLE_COUNT 10000

//����
#define MAX_SYS_GDI_COUNT 63700
#define MAX_SYS_HANDLE_COUNT 340000

#define LOG_DIR "../LogResources/"

//test asdd

volatile bool bShouldExit = false;
volatile bool bShouldSuspend = false;
char *FileName = new char[128];


unsigned int ulIP;
char *pHostName;

int _network_ini_flag=0;

int iniHostInfo()
{
	int res = InitNetWork();
	if(res != 0)
	{
		CleanNetWork();
		return res;
	}

	res = GetHostInfo();
	if(res != 0)
	{
		CleanNetWork();
		return res;
	}

	CleanNetWork();
}

unsigned int getIP()
{
	if(_network_ini_flag==0){ iniHostInfo();_network_ini_flag=1;}
	return ulIP;
}

char * getProxyHostName()
{
	if(_network_ini_flag==0){ iniHostInfo();_network_ini_flag=1;}
	return pHostName;
}




/********************************************************************************************
** ��������InitNetWork
** ���룺��
** �������
** ������������ʼ��windows�׽��ֿ�
** ȫ�ֱ�������
** ����ģ�飺�� 
** ��  �ߣ�ð����<maowang1991@seu.edu.cn>
** ��  �ڣ�2014-04-21
** �޸��ˣ���
** ��  �ڣ���
** ��  ����Version 0.1
********************************************************************************************/
int InitNetWork()
{
	int ir = 0;
	WSADATA wsaData;
	// ��ʼ��windows�׽��ֿ�
	ir = ::WSAStartup(MAKEWORD(2,2),&wsaData);
	if(ir != 0)
	{
		cout<<"failed to initialize winsock , error code is : "<<ir<<endl;
		return 1;
	}
	return 0;
}

/********************************************************************************************
** ��������CleanNetWork
** ���룺��
** �������
** ����������������winsock.dll�ĵ���
** ȫ�ֱ�������
** ����ģ�飺�� 
** ��  �ߣ�ð����<maowang1991@seu.edu.cn>
** ��  �ڣ�2014-04-21
** �޸��ˣ���
** ��  �ڣ���
** ��  ����Version 0.1
********************************************************************************************/
void CleanNetWork()
{
	WSACleanup();
}

/********************************************************************************************
** ��������GetHostInfo
** ���룺��
** �������
** ������������ȡ����IP��ַ��Ϣ
** ȫ�ֱ�������
** ����ģ�飺�� 
** ��  �ߣ�ð����<maowang1991@seu.edu.cn>
** ��  �ڣ�2014-04-21
** �޸��ˣ���
** ��  �ڣ���
** ��  ����Version 0.1
********************************************************************************************/
int GetHostInfo()
{
	int ir = 0;
	DWORD dwError;
	char szPath[128] = "";
	struct hostent *hostInfo;
	char **pAlias;
	struct in_addr addr;

	ir = gethostname(szPath,sizeof(szPath));
	pHostName=new char[strlen(szPath)+1];
	memset(pHostName,0,strlen(szPath)+1);

	strcpy(pHostName,szPath);
	if(ir == SOCKET_ERROR)
	{
		dwError = WSAGetLastError();
		if (dwError != 0) 
		{
			if (dwError == WSAEFAULT)
			{
				cout<<"The host name parameter is NULL pointer"<<endl;
				return 1;
			} 
			else if (dwError == WSANOTINITIALISED) 
			{
				cout<<"A successful WSAStartup call must occur before using this function\n";
				return 1;
			}
			else if(dwError == WSAENETDOWN)
			{
				cout<<"The network subsystem has failed"<<endl;
				return 1;
			}
			else if(dwError == WSAEINPROGRESS)
			{
				cout<<"A blocking Windows Sockets 1.1 call is in progress"<<endl;
				return 1;
			}
			else 
			{
				cout<<"Function failed with error:"<<dwError<<endl;
				return 1;
			}
		}
	}

	hostInfo = gethostbyname(szPath);
	if(hostInfo == NULL)
	{
		dwError = WSAGetLastError();
		if (dwError != 0) 
		{
			if (dwError == WSAHOST_NOT_FOUND)
			{
				cout<<"Host not found\n";
				return 1;
			} else if (dwError == WSANO_DATA) 
			{
				cout<<"No data record found\n";
				return 1;
			} else 
			{
				cout<<"Function failed with error: "<<dwError<<endl;
				return 1;
			}
		}
	}
	else
	{
		if (hostInfo->h_addrtype == AF_INET)
		{
			if (hostInfo->h_addr_list[0] != 0)
			{
				addr.s_addr = *(u_long *) hostInfo->h_addr_list[0];
				//strcpy(ip,inet_ntoa((addr)));
				ulIP=addr.s_addr ;
			}
		}
		else if (hostInfo->h_addrtype == AF_NETBIOS)
		{   
			cout<<"NETBIOS address was returned\n";
		} 
		return 0;
	}
}

/********************************************************************************************
** ��������GetSysGdiAndHandleCount
** ���룺nGDINums��nHandleNums
**        nGDINums--����GDI��������
**        nHandleNums--����handle����
** �������
** ������������ȡ����ϵͳGDI��������,���ڼ�DDS�������ֻ��Ҫ��һ�������͹�������������
** ȫ�ֱ�������
** ����ģ�飺�� 
** ��  �ߣ�ð����<maowang1991@seu.edu.cn>
** ��  �ڣ�2014-04-21
** �޸��ˣ���
** ��  �ڣ���
** ��  ����Version 0.1
********************************************************************************************/
void GetSysGdiAndHandleCount(int &nGDINums,int &nHandleNums)
{
	//���н��̵�GDI��������֮��
	nGDINums = 0;   
	//���н��̵�HANDLE��������֮��
	nHandleNums = 0;
	//ϵͳ�еĽ�����
	int nProcess   = 0; 
	//�ض�����GDI��������
	int nProGDINum = 0;
	//�ض����̾������
	int nProHandleNum = 0;
	char temp[128];
	//��ŵ�ǰʱ��
	char szCurrentDateTime[32];

	DWORD aProID[1024];
	DWORD cbNeeded;
	list<HANDLE> handleList;
	list<HANDLE>::iterator ir;

	list<DWORD> proIdList;
	list<DWORD>::iterator pir;

	::EnumProcesses ( aProID, sizeof(aProID), &cbNeeded );

	/*ϵͳ�н�������*/
	nProcess = cbNeeded / sizeof ( DWORD );
	//cout<<"total process num is : "<<nProcess<<endl;

	//��ȡϵͳ��ǰʱ�䣬����ʱ��д���ļ�
	GetTime(szCurrentDateTime);

	//ͳ�ƾ����ΪNULL�ģ�DDS�����̶�����
	//�����������list
	int countP = 0;
	handleList.clear();
	for (int i=0; i < nProcess; i ++ )
	{
		HANDLE hPro = ::OpenProcess (PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,  FALSE,  aProID[i]  );
		if(hPro != NULL)
		{
			++countP;
			handleList.push_back(hPro);
			proIdList.push_back(aProID[i]);
		}
		//CloseHandle (hPro);
	}

	memset(temp,0,128);
	sprintf(temp,"#%s\n%d\n",szCurrentDateTime,countP);
	WriteToFile(temp);

	/*ͳ�Ʒ���������ÿ�����̵�GDI������*/
	for (ir = handleList.begin(); ir != handleList.end(); ++ir )
	{
		HANDLE hpro = *ir;
		DWORD proId = *pir;
		memset(temp,0,128);
		//CString szProcessName = GetProcessName(hpro);
		//string s(szProcessName.GetBuffer());
		char *processName = GetProcessName(hpro);
		//char *chr=new char[szProcessName.GetLength()];
		//WideCharToMultiByte(CP_ACP,0,szProcessName.GetBuffer(),-1,chr,szProcessName.GetLength(),NULL,NULL);
		//string s=chr;


		//szProcessName.ReleaseBuffer();
		nProHandleNum = GetProcessHandleCount(hpro);
		nProGDINum = GetProcessGdiCount(hpro);
		sprintf(temp,"%d %s %d %d\n",proId,processName,nProHandleNum,nProGDINum);
		WriteToFile(temp);
		delete []processName;
		nHandleNums += nProHandleNum;
		nGDINums += nProGDINum;
		CloseHandle (hpro);
	}
	handleList.clear();
	proIdList.clear();
	memset(temp,0,128);
	sprintf(temp,"%d %d\n\n",nHandleNums,nGDINums);
	WriteToFile(temp);
}

/********************************************************************************************
** ��������GetProcessHandleCount
** ���룺hPro--���̾��
** �������
** ������������ȡ���̾������
** ȫ�ֱ�������
** ����ģ�飺�� 
** ��  �ߣ�ð����<maowang1991@seu.edu.cn>
** ��  �ڣ�2014-04-21
** �޸��ˣ���
** ��  �ڣ���
** ��  ����Version 0.1
********************************************************************************************/
int GetProcessHandleCount(HANDLE hPro)
{
	DWORD handleCount = 0;
	GetProcessHandleCount(hPro,&handleCount);
	return handleCount;
}

/********************************************************************************************
** ��������GetProcessGdiCount
** ���룺hPro--���̾��
** �������
** ������������ȡ�ض����̵�GDI��������
** ȫ�ֱ�������
** ����ģ�飺�� 
** ��  �ߣ�ð����<maowang1991@seu.edu.cn>
** ��  �ڣ�2014-04-21
** �޸��ˣ���
** ��  �ڣ���
** ��  ����Version 0.1
********************************************************************************************/
int GetProcessGdiCount(HANDLE hPro)
{
	return ::GetGuiResources ( hPro, GR_GDIOBJECTS );
}

/********************************************************************************************
** ��������GetProcessName
** ���룺hPro--���̾��
** �������
** ������������ȡ�ض����̵Ľ�����
** ȫ�ֱ�������
** ����ģ�飺�� 
** ��  �ߣ�ð����<maowang1991@seu.edu.cn>
** ��  �ڣ�2014-04-21
** �޸��ˣ���
** ��  �ڣ���
** ��  ����Version 0.1
********************************************************************************************/
char *GetProcessName(HANDLE hPro)
{
	HMODULE hMod;
	DWORD ByteCnt;
	TCHAR szProcessName[MAX_PATH] = TEXT("<UNKNOWN>");
	int nSize = sizeof(szProcessName)/sizeof(TCHAR);
	char *processName = new char[128];
	memset(processName,0,128);

	if ( EnumProcessModules( hPro, &hMod, sizeof(hMod), &ByteCnt) )
	{
		GetModuleBaseName( hPro, hMod, szProcessName, nSize);
		strcpy(processName,szProcessName);
		return processName;
	}
}

/********************************************************************************************
** ��������GetTime
** ���룺hPro--���̾��
** �������
** ������������ȡϵͳ��ǰʱ��,�涨�����ַ����鳤��Ϊ32
** ȫ�ֱ�������
** ����ģ�飺�� 
** ��  �ߣ�ð����<maowang1991@seu.edu.cn>
** ��  �ڣ�2014-04-21
** �޸��ˣ���
** ��  �ڣ���
** ��  ����Version 0.1
********************************************************************************************/
void GetTime(char *szCurrentDateTime)
{
	memset(szCurrentDateTime,0,32);
	SYSTEMTIME systm;     
	GetLocalTime(&systm);     
	sprintf(szCurrentDateTime, "%4d-%.2d-%.2d %.2d:%.2d:%.2d",     
		systm.wYear, systm.wMonth, systm.wDay,     
		systm.wHour, systm.wMinute, systm.wSecond); 
}

/********************************************************************************************
** ��������WriteToFile
** ���룺str--��Ҫд���ļ�������
** �������
** ������������str�д�ŵ�����д���ļ�
** ȫ�ֱ�������
** ����ģ�飺�� 
** ��  �ߣ�ð����<maowang1991@seu.edu.cn>
** ��  �ڣ�2014-04-21
** �޸��ˣ���
** ��  �ڣ���
** ��  ����Version 0.1
********************************************************************************************/
//д���ļ�
void WriteToFile(char *str)
{
	FILE *fp;
	if((fp = fopen(FileName,"rb")) == NULL)
	{
		//fclose(fp);
		fp = fopen(FileName,"wb");
		fclose(fp);
	}
	else
	{
		fclose(fp);
	}
	fp = fopen(FileName,"ab");
	fprintf(fp,"%s",str);
	fclose(fp);
}

/********************************************************************************************
** ��������WorkThread
** ���룺lParam
** �������
** �����������̺߳���
** ȫ�ֱ�������
** ����ģ�飺�� 
** ��  �ߣ�ð����<maowang1991@seu.edu.cn>
** ��  �ڣ�2014-04-21
** �޸��ˣ���
** ��  �ڣ���
** ��  ����Version 0.1
********************************************************************************************/
DWORD WINAPI WorkThread(LPVOID lParam)
{
	int *interval = (int *)lParam;
	while(!bShouldExit)
	{
		if(!bShouldSuspend)
		{
			int nGDINums,nHandleNums;
			GetSysGdiAndHandleCount(nGDINums,nHandleNums);
		}
		Sleep(*interval);
	}
	bShouldExit = false;
//	cout<<"thread exit"<<endl;
	return 0;
}

/********************************************************************************************
** ��������SetFileName
** ���룺htimestamp��ltimestamp
** �������
** ���������������ļ�����
** ȫ�ֱ�������
** ����ģ�飺�� 
** ��  �ߣ�ð����<maowang1991@seu.edu.cn>
** ��  �ڣ�2014-04-21
** �޸��ˣ���
** ��  �ڣ���
** ��  ����Version 0.1
********************************************************************************************/
void SetFileName(unsigned htimestamp,unsigned ltimestamp)
{
	memset(FileName,0,128);
	sprintf(FileName,"%s%d_%d.dat",LOG_DIR,htimestamp,ltimestamp);
}



DWORD lpThreadId;
HANDLE hThread;
int interval = 1000;


void startWatch_PDC(unsigned htimestamp,unsigned ltimestamp)
{
	cout<<"start watching..."<<endl;
	SetFileName(htimestamp,ltimestamp);
	while(bShouldExit)
	{
		Sleep(1000);
	}
	hThread = CreateThread(NULL,0,WorkThread,&interval,CREATE_SUSPENDED,&lpThreadId);
	//shouldExit = false;
	ResumeThread(hThread);
}
void pauseWatch_PDC()
{
	cout<<"pause watching..."<<endl;
	bShouldSuspend = true;
}
void continueWatch_PDC()
{
	cout<<"continue watching..."<<endl;
	bShouldSuspend = false;
}
void stopWatch_PDC()
{
	cout<<"stop watching..."<<endl;
	bShouldExit = true;
}


