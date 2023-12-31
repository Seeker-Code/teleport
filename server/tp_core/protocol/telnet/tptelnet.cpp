﻿#include "telnet_proxy.h"
#include "tpp_env.h"

#include <teleport_const.h>
#include <json/json.h>

TPP_API ex_rv tpp_init(TPP_INIT_ARGS* init_args)
{
	if (!g_telnet_env.init(init_args))
		return TPE_FAILED;

	return 0;
}

TPP_API ex_rv tpp_start(void)
{
	if (!g_telnet_proxy.init())
		return TPE_FAILED;
	if (!g_telnet_proxy.start())
		return TPE_FAILED;

	return 0;
}

TPP_API ex_rv tpp_stop(void)
{
	g_telnet_proxy.stop();
	return 0;
}

TPP_API void tpp_timer(void) {
	// be called per one second.
	g_telnet_proxy.timer();
}

static ex_rv tpp_cmd_set_runtime_config(const char* param) {
    Json::CharReaderBuilder jcrb;
    std::unique_ptr<Json::CharReader> const jreader(jcrb.newCharReader());

    ex_astr err;
    Json::Value jp;
    if (!jreader->parse(param, param + strlen(param), &jp, &err))
        return TPE_JSON_FORMAT;

	if (!jp.isObject())
		return TPE_PARAM;

	if (jp["noop_timeout"].isNull() || !jp["noop_timeout"].isUInt())
		return TPE_PARAM;

	ex_u32 noop_timeout = jp["noop_timeout"].asUInt();
	// if (noop_timeout == 0)
	// 	return TPE_PARAM;

	g_telnet_proxy.set_cfg(noop_timeout * 60);

	return TPE_PARAM;
}

static ex_rv tpp_cmd_kill_sessions(const char* param) {
    Json::CharReaderBuilder jcrb;
    std::unique_ptr<Json::CharReader> const jreader(jcrb.newCharReader());

    ex_astr err;
    Json::Value jp;
    if (!jreader->parse(param, param + strlen(param), &jp, &err))
        return TPE_JSON_FORMAT;

	if (!jp.isArray())
		return TPE_PARAM;

	ex_astrs ss;
	int cnt = jp.size();
	for (int i = 0; i < cnt; ++i)
	{
		if (!jp[i].isString()) {
			return TPE_PARAM;
		}

		ss.push_back(jp[i].asString());
	}

	g_telnet_proxy.kill_sessions(ss);

	return TPE_PARAM;
}

TPP_API ex_rv tpp_command(ex_u32 cmd, const char* param) {
	switch (cmd) {
	case TPP_CMD_SET_RUNTIME_CFG:
		if (param == nullptr || strlen(param) == 0)
			return TPE_PARAM;
		return tpp_cmd_set_runtime_config(param);
	case TPP_CMD_KILL_SESSIONS:
		if (param == nullptr || strlen(param) == 0)
			return TPE_PARAM;
		return tpp_cmd_kill_sessions(param);
	default:
		return TPE_UNKNOWN_CMD;
	}
}
