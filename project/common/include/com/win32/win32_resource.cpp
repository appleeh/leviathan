#include <stdio.h>
#include "windows.h"
#include "psapi.h"
#include "pdh.h"
#include "win32_resource.h"


//int common::win32::get_storage_usage(char * dev_path)
//{
//	unsigned long long total=0, free = 0;
//	double usage = 0;
//	struct statvfs sfs;
//	if ( statvfs ( dev_path, &sfs) != -1 ) {
//		total = (unsigned long long)sfs.f_bsize * sfs.f_blocks;
//		free = (unsigned long long)sfs.f_bsize * sfs.f_bfree;
//	}
//	usage = (total - free) * 100;
//	usage = usage/total;
//	int res = ceil(usage);
//	return res;
//}

//int common::win32::get_virtualmem_info(unsigned long *totMem, unsigned long *usedMem)
//{
//	MEMORYSTATUSEX memInfo;
//	memInfo.dwLength = sizeof(MEMORYSTATUSEX);
//	GlobalMemoryStatusEx(&memInfo);
//	*totMem = memInfo.ullTotalPageFile;
//	*usedMem = memInfo.ullTotalPageFile - memInfo.ullAvailPageFile;
//
//	return memInfo.dwMemoryLoad;
//}

int common::win32::get_mem_info(unsigned long *totMem, unsigned long *usedMem)
{
	MEMORYSTATUSEX memInfo;
	memInfo.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memInfo);

	DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
	DWORDLONG physMemUsed = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
	return memInfo.dwMemoryLoad;
}


static PDH_HQUERY cpuQuery;
static PDH_HCOUNTER cpuTotal;
void common::win32::init_cpu_machine() {
	PdhOpenQuery(NULL, NULL, &cpuQuery);
	PdhAddCounter(cpuQuery, "\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
	PdhCollectQueryData(cpuQuery);
}
int common::win32::get_cpu_usage_machine()
{
		PDH_FMT_COUNTERVALUE counterVal;

		PdhCollectQueryData(cpuQuery);
		PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
		return (int) counterVal.doubleValue;
}



static ULARGE_INTEGER gs_lastCPU, gs_lastSysCPU, gs_lastUserCPU;
static int gs_numProcessors;
static HANDLE gs_self;

void common::win32::init_cpu_process() {
	SYSTEM_INFO sysInfo;
	FILETIME ftime, fsys, fuser;
	GetSystemInfo(&sysInfo);
	gs_numProcessors = sysInfo.dwNumberOfProcessors;

	GetSystemTimeAsFileTime(&ftime);
	memcpy(&gs_lastCPU, &ftime, sizeof(FILETIME));


	gs_self = GetCurrentProcess();
	GetProcessTimes(gs_self, &ftime, &ftime, &fsys, &fuser);
	memcpy(&gs_lastSysCPU, &fsys, sizeof(FILETIME));
	memcpy(&gs_lastUserCPU, &fuser, sizeof(FILETIME));
}

int common::win32::get_cpu_usage_process()
{
	FILETIME ftime, fsys, fuser;
	ULARGE_INTEGER now, sys, user;
	long long percent;

	GetSystemTimeAsFileTime(&ftime);
	memcpy(&now, &ftime, sizeof(FILETIME));


	GetProcessTimes(gs_self, &ftime, &ftime, &fsys, &fuser);
	memcpy(&sys, &fsys, sizeof(FILETIME));
	memcpy(&user, &fuser, sizeof(FILETIME));
	percent = (sys.QuadPart - gs_lastSysCPU.QuadPart) + (user.QuadPart - gs_lastUserCPU.QuadPart);
	percent /= (now.QuadPart - gs_lastCPU.QuadPart);
	percent /= gs_numProcessors;
	gs_lastCPU = now;
	gs_lastUserCPU = user;
	gs_lastSysCPU = sys;

	return (int)(percent * 100);

}
