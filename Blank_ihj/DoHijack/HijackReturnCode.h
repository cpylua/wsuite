#pragma once

#define HIJACK_CREATE	1
#define HIJACK_DELETE	2
#define HIJACK_DEL_BLANK	3
#define HIJACK_ADD_BLANK	4

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

#include <windows.h>

enum HijackReturnCode
{
	HijackFailed,
	HijackCreateOk,
	HijackDeleteOk
};

struct HijackInfo
{
	DWORD cEntry;
	//DWORD cbFileMappingSize;
	DWORD dwOption;
};

#include "resource.h"