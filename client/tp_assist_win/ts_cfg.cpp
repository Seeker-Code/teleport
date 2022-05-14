﻿#include "stdafx.h"
#include "ts_cfg.h"
#include "ts_env.h"


TsCfg g_cfg;

TsCfg::TsCfg()
{}

TsCfg::~TsCfg()
{}

bool TsCfg::init(void) {
	ex_astr file_content;
	if (!ex_read_text_file(g_env.m_cfg_file, file_content)) {
		EXLOGE("can not load config file.\n");
		return false;
	}

	if (!_load(file_content))
		return false;

	if (!m_root.isObject()) {
		EXLOGE("invalid config file, not in json format?\n");
		return false;
	}

	if (m_root["file_version"].isNull()) {
		EXLOGE("invalid config file, maybe need create new one?\n");
		return false;
	}

	if (!m_root["file_version"].isInt()) {

	}

	return true;
}

bool TsCfg::save(const ex_astr& new_value)
{
	if (!_load(new_value))
		return false;

    Json::StreamWriterBuilder jwb;
	jwb["indentation"] = "    ";
	jwb["emitUTF8"] = true;
    std::unique_ptr<Json::StreamWriter> jwriter(jwb.newStreamWriter());
    ex_aoss os;
    jwriter->write(m_root, &os);
    ex_astr val = os.str();

	if (!ex_write_text_file(g_env.m_cfg_file, val)) {
		EXLOGE("can not save config file.\n");
		return false;
	}

	return true;
}

bool TsCfg::_parse_app(const Json::Value& m_root, const ex_astr& str_app, APP_CONFIG& cfg) {
	const Json::Value& jApp = m_root[str_app.c_str()];
	if (!jApp.isObject())
		return false;

	if (!jApp["selected"].isString()) {
		EXLOGE("invalid config, error 2.\n");
		return false;
	}
	ex_astr _selected = jApp["selected"].asCString();;

	if (!jApp["available"].isArray() || jApp["available"].size() == 0) {
		EXLOGE("invalid config, error 3.\n");
		return false;
	}
	const Json::Value& jAppList = jApp["available"];

	ex_astr tmp_val;
	for (size_t i = 0; i < jAppList.size(); ++i) {
		if (
			!jAppList[i]["name"].isString()
			|| !jAppList[i]["app"].isString()
			|| !jAppList[i]["cmdline"].isString()
			|| !jAppList[i]["desc"].isArray()
			) {
			EXLOGE("invalid config, error 4.\n");
			return false;
		}

		if (jAppList[i]["name"].asString().empty()) {
			EXLOGE("invalid config, need name.\n");
			return false;
		}

		if (jAppList[i]["display"].isNull() || jAppList[i]["display"].asString().empty()) {
			tmp_val = jAppList[i]["name"].asString();
		}
		else
			tmp_val = jAppList[i]["display"].asString();
		ex_astr2wstr(tmp_val, cfg.display);

		if (jAppList[i]["name"].asString() != _selected)
			continue;

		tmp_val = jAppList[i]["name"].asString();
		ex_astr2wstr(tmp_val, cfg.name);

		// cfg.display = jAppList[i]["display"].asCString();
		tmp_val = jAppList[i]["app"].asCString();
		ex_astr2wstr(tmp_val, cfg.application);
		tmp_val = jAppList[i]["cmdline"].asCString();
		ex_astr2wstr(tmp_val, cfg.cmdline);

		if (jAppList[i]["desc"].size() > 0) {
			const Json::Value& jAppDescList = jAppList[i]["desc"];

			for (size_t j = 0; j < jAppDescList.size(); ++j) {
				if (!jAppDescList[j].isString())
					return false;
				ex_wstr w_tmp;
				ex_astr2wstr(jAppDescList[j].asString(), w_tmp);
				cfg.description.push_back(w_tmp);
			}
		}

		break;
	}

	return true;
}

bool TsCfg::_load(const ex_astr& str_json) {
    Json::CharReaderBuilder jcrb;
    std::unique_ptr<Json::CharReader> const jreader(jcrb.newCharReader());
    const char *str_json_begin = str_json.c_str();

    ex_astr err;
	if (!jreader->parse(str_json_begin, str_json_begin + str_json.length(), &m_root, &err)) {
		EXLOGE("can not parse new config data, not in json format? %s\n", err.c_str());
		return false;
	}

	//===================================
	// check config
	//===================================
	if (!_parse_app(m_root, "ssh", ssh))
		return false;
	if (!_parse_app(m_root, "sftp", sftp))
		return false;
	if(!_parse_app(m_root, "telnet", telnet))
	    return false;
	if (!_parse_app(m_root, "rdp", rdp))
		return false;

	return true;
}
