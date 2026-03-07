extern "C" int _CrtDbgReport(int reportType, // NOLINT(readability-identifier-naming)
                             const char* filename, int linenumber, const char* moduleName, const char* format, ...)
{
     (void)reportType;
     (void)filename;
     (void)linenumber;
     (void)moduleName;
     (void)format;
     return 0;
}
