#ifndef CLASS_CRASHRPTWRAPPER
#define CLASS_CRASHRPTWRAPPER

/// A CrashRpt wrapper class
class CrashRptWrapper
{
  public:
	static const char *appName;
	static const char *appVersion;
	static const char *emailSubject;
	static const char *emailTo;

	static int install();
	static void uninstall();
	static void emulateCrash();
};

#endif
