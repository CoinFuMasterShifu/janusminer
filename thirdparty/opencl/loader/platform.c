#ifdef __linux__
#include "linux/icd_linux.c"
#include "linux/icd_linux_envvars.c"
#else
#include "windows/icd_windows.c"
#include "windows/icd_windows_apppackage.c"
#include "windows/icd_windows_dxgk.c"
#include "windows/icd_windows_envvars.c"
#include "windows/icd_windows_hkr.c"
#endif
