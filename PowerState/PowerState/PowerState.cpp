#include <windows.h>
#include <strsafe.h>

void ShowError();

int main()
{
	SYSTEM_POWER_STATUS sps;
	memset(&sps,0,sizeof(SYSTEM_POWER_STATUS));

	if(GetSystemPowerStatus(&sps))
	{
		TCHAR szState[256];
		memset(szState,0,sizeof(szState));
		
		system("color 0a");
		wprintf_s(TEXT("\t\t\t***System Power Status***\n"));
		
		//AC Line State
		StringCbPrintf(szState,sizeof(szState),TEXT("AC line state: "));
		if (sps.ACLineStatus == 0)
			StringCbCat(szState,sizeof(szState),TEXT("Offline\n"));
		else if (sps.ACLineStatus == 1)
			StringCbCat(szState,sizeof(szState),TEXT("Online\n"));
		else
			StringCbCat(szState,sizeof(szState),TEXT("Unknown status\n"));
	

		//battery charge status
		StringCbCat(szState,sizeof(szState),TEXT("Battery charge status: "));

		//is it charging?
		if((sps.BatteryFlag & 0x8) == 0x8)
			StringCbCat(szState,sizeof(szState),TEXT("Charging\n\t\t       "));

		//high,low or critical?
		if((sps.BatteryFlag & 0x1) == 0x1)
			StringCbCat(szState,sizeof(szState),TEXT("High,the battery capacity is at more than 66 percent\n"));
		if((sps.BatteryFlag & 0x2) == 0x2)
			StringCbCat(szState,sizeof(szState),TEXT("Low,the battery capacity is at less than 33 percent\n"));
		if((sps.BatteryFlag & 0x4) == 0x4)
			StringCbCat(szState,sizeof(szState),TEXT("Critical,the battery capacity is at less than five percent\n"));

		//other status
		if((sps.BatteryFlag & 0x80) == 0x80)
			StringCbCat(szState,sizeof(szState),TEXT("There's no system battery in your system\n"));
		if(sps.BatteryFlag == 0xFF)
			StringCbCat(szState,sizeof(szState),TEXT("Unknown status\n"));
		if(sps.BatteryFlag == 0)
			StringCbCat(szState,sizeof(szState),TEXT("The battery is not being charged and the battery capacity\t\t       is between 33 percent and 66 percent\n"));
		wprintf_s(szState);

		//The percentage of full battery charge remaining
		memset(szState,0,sizeof(szState));
		if(sps.BatteryLifePercent > 0 && sps.BatteryLifePercent <= 100)
			StringCbPrintf(szState,sizeof(szState),TEXT("The percentage of full battery charge remaining: %d percent\n"),sps.BatteryLifePercent);
		else if(sps.BatteryLifePercent == 255)
			StringCbCat(szState,sizeof(szState),TEXT("The percentage of full battery charge remaining is unknown\n"));
		else
		{
			ShowError();
			exit(-1);
		}
		wprintf_s(szState);

		//battery life remaining
		memset(szState,0,sizeof(szState));
		if(sps.BatteryLifeTime != -1)
			StringCbPrintf(szState,sizeof(szState),TEXT("Battery life remaining: approximately %d minutes\n"),sps.BatteryLifeTime / 60);
		else
			StringCbCat(szState,sizeof(szState),TEXT("Battery life remaining is unknown\n"));
		wprintf_s(szState);

		//battery life when at full charge
		memset(szState,0,sizeof(szState));
		if(sps.BatteryFullLifeTime != -1)
			StringCbPrintf(szState,sizeof(szState),TEXT("Battery life at full charge: approximately %d minutes\n"),sps.BatteryFullLifeTime /60);
		else
			StringCbCat(szState,sizeof(szState),TEXT("Full battery life time is unknown\n"));
		wprintf(szState);
	}
	else
		ShowError();

	system("pause");
	return 0;
}

void ShowError()
{
		LPVOID lpMsgBuf;
		FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
		GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

		LPVOID lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,(lstrlen((LPCTSTR)lpMsgBuf) + 256) * sizeof(TCHAR)); 
		StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf),
		TEXT("An error occured while retrieving system power state:\n%s"),(LPTSTR)lpMsgBuf); 

		wprintf_s((LPCTSTR)lpDisplayBuf);

		LocalFree(lpMsgBuf);
		LocalFree(lpDisplayBuf);
}

