#ifndef __TS_ENV_H__
#define __TS_ENV_H__

#include <ex.h>

class TsEnv
{
public:
	TsEnv();
	~TsEnv();

	bool init(void);

public:
	ex_wstr m_exec_file;
	ex_wstr m_exec_path;
	ex_wstr m_cfg_file;
	ex_wstr m_log_path;
	ex_wstr m_tools_path;
};

extern TsEnv g_env;

#endif // __TS_ENV_H__
