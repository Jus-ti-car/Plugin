// Supplier writes to registry key SOFTWARE\\Task2
//

#include "pch.h"
#include <iostream>
#include <windows.h>
#include <string> 
#include <string.h> 
#include <list>
#define SERVICE
#include "../api.h"
#include "../plugin_id.h"

typedef struct _DllListElement
{
	GUID pluginId;
	const wchar_t* DllName;
	pfn_GetCopyright GetCopyright;
} DllListElement;

std::list<DllListElement> g_DllList = {
	{ plugin1_id, L"Plugin1_2.dll", nullptr },
	{ plugin2_id, L"Pugin2.dll", nullptr }
};


int main()
{
	HKEY hKey = NULL;
	LPCTSTR strKeyName = L"SOFTWARE\\Task2";

	// Writing
	for (DllListElement element : g_DllList) {
		// Make string from GUID
		wchar_t dllName[512] = L"\0";
		wchar_t* string;

		StringFromIID(element.pluginId, &string);
		wcscat_s(dllName, 512, element.DllName);
		LPCTSTR guid_value = (LPCTSTR) string;

		//Open the key
		long sts = RegOpenKeyEx(HKEY_CURRENT_USER, strKeyName, 0, KEY_ALL_ACCESS, &hKey);

		//If failed, create the key
		if (ERROR_NO_MATCH == sts || ERROR_FILE_NOT_FOUND == sts)
		{
			std::wcout << "Creating registry key " << strKeyName << std::endl;

			LONG st = RegCreateKeyEx(HKEY_CURRENT_USER, strKeyName, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, NULL);

			if (ERROR_SUCCESS != st)
				std::wcout << "Can't create registry key" << strKeyName << std::endl << "\tERROR: " << st << std::endl;
			else
				std::wcout << "Key created" << std::endl;

		}
		else if (ERROR_SUCCESS != sts)
		{
			std::wcout << "Cannot open registry key " << strKeyName << std::endl << "\tERROR: " << sts << std::endl;

		}

		// Setting value 
		LONG st = RegSetValueEx(hKey, guid_value, 0, REG_SZ, (LPBYTE)dllName, wcslen(dllName) * sizeof(wchar_t));

		if (ERROR_SUCCESS != st)
			std::wcout << "Can't write to registry key" << strKeyName << std::endl << "\tERROR: " << st << std::endl;
		else
			std::wcout << "Value: " << dllName << " was written" << std::endl;

	}

	RegCloseKey(hKey);
}