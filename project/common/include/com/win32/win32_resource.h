
#ifndef _LIB_UNIX_RESOURCE_H
#define _LIB_UNIX_RESOURCE_H


namespace common {
/*#undef unix*/
namespace win32 {

	//int get_storage_usage(char * dev_path);
	void init_cpu_machine();
	void init_cpu_process();
	int get_cpu_usage_process();
	int get_cpu_usage_machine();
	int get_mem_info(unsigned long *totMem, unsigned long *usedMem);

} // namespace unix

} // namespace common

#endif // _LIB_UNIX_RESOURCE_H

