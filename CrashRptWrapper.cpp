#include "CrashRpt.h"
#include <stdio.h>
#include <tchar.h>

#include "CrashRptWrapper.h"

int CrashRptWrapper::install()
{
	CR_INSTALL_INFO info;
	memset(&info, 0, sizeof(CR_INSTALL_INFO));
	info.cb = sizeof(CR_INSTALL_INFO);
	info.pszAppName = _T(CrashRptWrapper::appName);
	info.pszAppVersion = _T(CrashRptWrapper::appVersion);
	info.pszEmailSubject = _T(CrashRptWrapper::emailSubject);
	info.pszEmailTo = _T(CrashRptWrapper::emailTo);
	info.uPriorities[CR_SMAPI] = 3; // Try send report over Simple MAPI
	// Install all available exception handlers
	info.dwFlags |= CR_INST_ALL_POSSIBLE_HANDLERS;

	// Install crash reporting
	int nResult = crInstall(&info);
	if (nResult != 0)
	{
		// Something goes wrong. Get error message.
		TCHAR szErrorMsg[512] = _T("");
		crGetLastErrorMsg(szErrorMsg, 512);
		_tprintf_s(_T("%s\n"), szErrorMsg);
		return 1;
	}

	return 0;
}

void CrashRptWrapper::uninstall()
{
	crUninstall();
}

void CrashRptWrapper::emulateCrash()
{
	crEmulateCrash(CR_SEH_EXCEPTION);
}
