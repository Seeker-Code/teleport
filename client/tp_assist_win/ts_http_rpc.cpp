﻿#include "stdafx.h"

#pragma warning(disable:4091)

#include <commdlg.h>
#include <ShlObj.h>

#include <teleport_const.h>

#include "ts_http_rpc.h"
#include "dlg_main.h"
#include "ts_ver.h"
#include "ts_env.h"
#include "ts_utils.h"

/*
1.
SecureCRT支持设置标签页的标题，命令行参数 /N "tab name"就可以
Example:
To launch a new Telnet session, displaying the name "Houston, TX" on the tab, use the following:
/T /N "Houston, TX" /TELNET 192.168.0.6

2.
多次启动的SecureCRT放到一个窗口的不同标签页中，使用参数：  /T
  SecureCRT.exe /T /N "TP#ssh://192.168.1.3" /SSH2 /L root /PASSWORD 1234 120.26.109.25

3.
telnet客户端的启动：
  putty.exe telnet://administrator@127.0.0.1:52389
如果是SecureCRT，则需要
  SecureCRT.exe /T /N "TP#telnet://192.168.1.3" /SCRIPT X:\path\to\startup.vbs /TELNET 127.0.0.1 52389
其中，startup.vbs的内容为：
---------文件开始---------
#$language = "VBScript"
#$interface = "1.0"
Sub main
  crt.Screen.Synchronous = True
  crt.Screen.WaitForString "ogin: "
  crt.Screen.Send "SESSION-ID" & VbCr
  crt.Screen.Synchronous = False
End Sub
---------文件结束---------

4. 为了让putty的窗口标签显示正常的IP，可以尝试在连接成功后，主动向服务端发送下列命令：
    PS1="\[\e]0;${debian_chroot:+($debian_chroot)}\u@192.168.1.2: \w\a\]$PS1"
手工测试了，ubuntu服务器可以，不知道是否能够支持所有的Linux。SecureCRT对此表示忽略。
*/

//#define RDP_CLIENT_SYSTEM_BUILTIN
// #define RDP_CLIENT_SYSTEM_ACTIVE_CONTROL
//#define RDP_CLIENT_FREERDP


TsHttpRpc g_http_interface;

//void http_rpc_main_loop() {
//
//        if (!g_http_interface.init_http()) {
//            EXLOGE("[ERROR] can not start HTTP-RPC listener, maybe port %d is already in use.\n", TS_HTTP_RPC_PORT);
//            return;
//        }
//
//        EXLOGW("======================================================\n");
//        EXLOGW("[rpc] TeleportAssist-HTTP-RPC ready on 127.0.0.1:%d\n", TS_HTTP_RPC_PORT);
//
//        g_http_interface.run();
//
//        EXLOGW("[rpc] HTTP-Server main loop end.\n");
//}
//
//void http_rpc_stop() {
//        g_http_interface.stop();
//}


TsHttpRpc::TsHttpRpc() {
    m_stop = false;
    m_port = 0;
    mg_mgr_init(&m_mg_mgr, nullptr);
}

TsHttpRpc::~TsHttpRpc() {
    mg_mgr_free(&m_mg_mgr);
}

bool TsHttpRpc::init() {
    struct mg_connection* nc = nullptr;

    for (int port = TS_HTTP_RPC_PORT_MIN; port < TS_HTTP_RPC_PORT_MAX; ++port) {
        char addr[128] = { 0 };
        ex_strformat(addr, 128, "tcp://127.0.0.1:%d", port);

        nc = mg_bind(&m_mg_mgr, addr, _mg_event_handler);
        if (!nc) {
            EXLOGW("[rpc] can not listen on 127.0.0.1:%d\n", port);
            continue;
        }

        m_port = port;
        break;
    }

    if (m_port == 0)
    {
        EXLOGE("[rpc] can not listen on 127.0.0.1 port %d~%d.\n", TS_HTTP_RPC_PORT_MIN, TS_HTTP_RPC_PORT_MAX);
        return false;
    }


    nc->user_data = this;

    mg_set_protocol_http_websocket(nc);

    return _on_init();
}

#if 0
bool TsHttpRpc::init_https() {
    ex_wstr file_ssl_cert = g_env.m_exec_path;
    ex_path_join(file_ssl_cert, true, L"cfg", L"localhost.pem", NULL);
    ex_wstr file_ssl_key = g_env.m_exec_path;
    ex_path_join(file_ssl_key, true, L"cfg", L"localhost.key", NULL);
    ex_astr _ssl_cert;
    ex_wstr2astr(file_ssl_cert, _ssl_cert);
    ex_astr _ssl_key;
    ex_wstr2astr(file_ssl_key, _ssl_key);

    const char *err = NULL;
    struct mg_bind_opts bind_opts;
    memset(&bind_opts, 0, sizeof(bind_opts));
    bind_opts.ssl_cert = _ssl_cert.c_str();
    bind_opts.ssl_key = _ssl_key.c_str();
    bind_opts.error_string = &err;


    char addr[128] = { 0 };
    ex_strformat(addr, 128, "tcp://127.0.0.1:%d", TS_HTTPS_RPC_PORT);

    struct mg_connection* nc = nullptr;
    nc = mg_bind_opt(&m_mg_mgr, addr, _mg_event_handler, bind_opts);
    if (!nc) {
        EXLOGE("[rpc] TsHttpRpc::init 127.0.0.1:%d\n", TS_HTTPS_RPC_PORT);
        return false;
    }
    nc->user_data = this;

    mg_set_protocol_http_websocket(nc);

    return _on_init();
}
#endif

bool TsHttpRpc::_on_init() {
    char file_name[MAX_PATH] = { 0 };
    if (!GetModuleFileNameA(nullptr, file_name, MAX_PATH))
        return false;

    int len = strlen(file_name);

    if (file_name[len] == '\\')
        file_name[len] = '\0';

    char* match = strrchr(file_name, '\\');
    if (match)
        *match = '\0';

    m_content_type_map[".js"] = "application/javascript";
    m_content_type_map[".png"] = "image/png";
    m_content_type_map[".jpeg"] = "image/jpeg";
    m_content_type_map[".jpg"] = "image/jpeg";
    m_content_type_map[".gif"] = "image/gif";
    m_content_type_map[".ico"] = "image/x-icon";
    m_content_type_map[".json"] = "image/json";
    m_content_type_map[".html"] = "text/html";
    m_content_type_map[".css"] = "text/css";
    m_content_type_map[".tif"] = "image/tiff";
    m_content_type_map[".tiff"] = "image/tiff";
    m_content_type_map[".svg"] = "text/html";

    return true;
}

void TsHttpRpc::run(void) {
    while (!m_stop) {
        mg_mgr_poll(&m_mg_mgr, 500);
    }
}

void TsHttpRpc::stop(void) {
    m_stop = true;
}

void TsHttpRpc::_mg_event_handler(struct mg_connection *nc, int ev, void *ev_data) {
    struct http_message *hm = (struct http_message*)ev_data;

    TsHttpRpc* _this = (TsHttpRpc*)nc->user_data;
    if (!_this) {
        EXLOGE("[ERROR] invalid http request.\n");
        return;
    }

    switch (ev) {
    case MG_EV_HTTP_REQUEST:
    {
        ex_astr uri;
        ex_chars _uri;
        _uri.resize(hm->uri.len + 1);
        memset(&_uri[0], 0, hm->uri.len + 1);
        memcpy(&_uri[0], hm->uri.p, hm->uri.len);
        uri = &_uri[0];

#ifdef EX_DEBUG
        char* dbg_method = nullptr;
        if (hm->method.len == 3 && 0 == memcmp(hm->method.p, "GET", hm->method.len))
            dbg_method = "GET";
        else if (hm->method.len == 4 && 0 == memcmp(hm->method.p, "POST", hm->method.len))
            dbg_method = "POST";
        else
            dbg_method = "UNSUPPORTED-HTTP-METHOD";

        EXLOGV("[rpc] got %s request: %s\n", dbg_method, uri.c_str());
#endif
        ex_astr ret_buf;
        bool b_is_html = false;

//         if (uri == "/") {
//             ex_wstr page = L"<html lang=\"zh_CN\"><head><meta charset=\"utf-8\"/><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/><title>Teleport助手</title>\n<style type=\"text/css\">\n.box{padding:20px;margin:40px;border:1px solid #78b17c;background-color:#e4ffe5;}\n</style>\n</head><body><div class=\"box\">Teleport助手工作正常！</div></body></html>";
//             ex_wstr2astr(page, ret_buf, EX_CODEPAGE_UTF8);
// 
//             mg_printf(nc, "HTTP/1.0 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n%s", ret_buf.size() - 1, &ret_buf[0]);
//             nc->flags |= MG_F_SEND_AND_CLOSE;
//             return;
//         }

        if (uri == "/") {
            uri = "/status.html";
            b_is_html = true;
        }
        else if (uri == "/config") {
            uri = "/index.html";
            b_is_html = true;
        }

        ex_astr temp;
        int offset = uri.find("/", 1);
        if (offset > 0) {
            temp = uri.substr(1, offset - 1);

            if (temp == "api") {
                ex_astr method;
                ex_astr json_param;
                int rv = _this->_parse_request(hm, method, json_param);
                if (0 != rv) {
                    EXLOGE("[ERROR] http-rpc got invalid request.\n");
                    _this->_create_json_ret(ret_buf, rv);
                } else {
                    _this->_process_js_request(method, json_param, ret_buf);
                }

                mg_printf(nc, "HTTP/1.0 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: %ld\r\nContent-Type: application/json\r\n\r\n%s", ret_buf.length(), &ret_buf[0]);
                nc->flags |= MG_F_SEND_AND_CLOSE;
                return;
            }
        }


        ex_astr file_suffix;
        offset = uri.rfind(".");
        if (offset > 0) {
            file_suffix = uri.substr(offset, uri.length());
        }

        ex_wstr2astr(g_env.m_site_path, temp);
        ex_astr index_path = temp + uri;


        FILE* file = ex_fopen(index_path.c_str(), "rb");
        if (file) {
            unsigned long file_size = 0;
            char* buf = nullptr;
            size_t ret = 0;

            fseek(file, 0, SEEK_END);
            file_size = ftell(file);
            buf = new char[file_size];
            memset(buf, 0, file_size);
            fseek(file, 0, SEEK_SET);
            ret = fread(buf, 1, file_size, file);
            fclose(file);

            ex_astr content_type = _this->get_content_type(file_suffix);

            mg_printf(nc, "HTTP/1.0 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: %ld\r\nContent-Type: %s\r\n\r\n", file_size, content_type.c_str());
            mg_send(nc, buf, (int)file_size);
            delete[]buf;
            nc->flags |= MG_F_SEND_AND_CLOSE;
            return;
        } else if (b_is_html) {
            ex_wstr page = L"<html lang=\"zh_CN\"><html><head><title>404 Not Found</title></head><body bgcolor=\"white\"><center><h1>404 Not Found</h1></center><hr><center><p>Teleport Assistor configuration page not found.</p></center></body></html>";
            ex_wstr2astr(page, ret_buf, EX_CODEPAGE_UTF8);

            mg_printf(nc, "HTTP/1.0 404 File Not Found\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: %ld\r\nContent-Type: text/html\r\n\r\n%s", ret_buf.size() - 1, &ret_buf[0]);
            nc->flags |= MG_F_SEND_AND_CLOSE;
            return;
        }

    }
    break;
    default:
        break;
    }
}

int TsHttpRpc::_parse_request(struct http_message* req, ex_astr& func_cmd, ex_astr& func_args) {
    if (!req)
        return TPE_FAILED;

    bool is_get = true;
    if (req->method.len == 3 && 0 == memcmp(req->method.p, "GET", req->method.len))
        is_get = true;
    else if (req->method.len == 4 && 0 == memcmp(req->method.p, "POST", req->method.len))
        is_get = false;
    else
        return TPE_HTTP_METHOD;

    ex_astrs strs;

    size_t pos_start = 1;	// 跳过第一个字节，一定是 '/'

    size_t i = 0;
    for (i = pos_start; i < req->uri.len; ++i) {
        if (req->uri.p[i] == '/') {
            if (i - pos_start > 0) {
                ex_astr tmp_uri;
                tmp_uri.assign(req->uri.p + pos_start, i - pos_start);
                strs.push_back(tmp_uri);
            }
            pos_start = i + 1;	// 跳过当前找到的分隔符
        }
    }
    if (pos_start < req->uri.len) {
        ex_astr tmp_uri;
        tmp_uri.assign(req->uri.p + pos_start, req->uri.len - pos_start);
        strs.push_back(tmp_uri);
    }

    if (strs.empty() || strs[0] != "api")
        return TPE_PARAM;

    if (is_get) {
        if (2 == strs.size()) {
            func_cmd = strs[1];
        } else if (3 == strs.size()) {
            func_cmd = strs[1];
            func_args = strs[2];
        } else {
            return TPE_PARAM;
        }
    } else {
        if (2 == strs.size()) {
            func_cmd = strs[1];
        } else {
            return TPE_PARAM;
        }

        if (req->body.len > 0) {
            func_args.assign(req->body.p, req->body.len);
        }
    }

    if (func_args.length() > 0) {
        // 将参数进行 url-decode 解码
        int len = func_args.length() * 2;
        ex_chars sztmp;
        sztmp.resize(len);
        memset(&sztmp[0], 0, len);
        if (-1 == ex_url_decode(func_args.c_str(), func_args.length(), &sztmp[0], len, 0))
            return TPE_HTTP_URL_ENCODE;

        func_args = &sztmp[0];
    }

    EXLOGV("[rpc] method=%s, json_param=%s\n", func_cmd.c_str(), func_args.c_str());

    return TPE_OK;
}

void TsHttpRpc::_process_js_request(const ex_astr& func_cmd, const ex_astr& func_args, ex_astr& buf) {
    if (func_cmd == "get_version") {
        _rpc_func_get_version(func_args, buf);
    //} else if (func_cmd == "run") {
    //    _rpc_func_run_client(func_args, buf);
    //} else if (func_cmd == "rdp_play") {
    //    _rpc_func_rdp_play(func_args, buf);
    } else if (func_cmd == "get_config") {
        _rpc_func_get_config(func_args, buf);
    } else if (func_cmd == "set_config") {
        _rpc_func_set_config(func_args, buf);
    } else if (func_cmd == "file_action") {
        _rpc_func_file_action(func_args, buf);
    } else {
        EXLOGE("[rpc] got unknown command: %s\n", func_cmd.c_str());
        _create_json_ret(buf, TPE_UNKNOWN_CMD);
    }
}

void TsHttpRpc::_create_json_ret(ex_astr& buf, int errcode) {
    // 返回： {"code":123}

    Json::Value jr_root;
    jr_root["code"] = errcode;

    // buf = jr_writer.write(jr_root);
    Json::StreamWriterBuilder jwb;
    std::unique_ptr<Json::StreamWriter> jwriter(jwb.newStreamWriter());
    ex_aoss os;
    jwriter->write(jr_root, &os);
    buf = os.str();
}

void TsHttpRpc::_create_json_ret(ex_astr& buf, Json::Value& jr_root) {
//     Json::FastWriter jr_writer;
//     buf = jr_writer.write(jr_root);
    Json::StreamWriterBuilder jwb;
    std::unique_ptr<Json::StreamWriter> jwriter(jwb.newStreamWriter());
    ex_aoss os;
    jwriter->write(jr_root, &os);
    buf = os.str();
}

#if 0
void TsHttpRpc::_rpc_func_url_protocol(const ex_astr& args, ex_astr& buf)
{
	//处理urlprotocol调用访式
	// 将参数进行 url-decode 解码
	std::string func_args = args;
	if (func_args.length() > 0)
	{
		int len = func_args.length() * 2;
		ex_chars sztmp;
		sztmp.resize(len);
		memset(&sztmp[0], 0, len);
		if (-1 == ts_url_decode(func_args.c_str(), func_args.length(), &sztmp[0], len, 0))
			return ;

		func_args = &sztmp[0];
	}
	EXLOGD(("%s\n"), func_args.c_str());
	//处理传参过来的teleport://{}/,只保留参数部份
	std::string urlproto_appname = TP_URLPROTO_APP_NAME;
	urlproto_appname += "://{";
	func_args.erase(0, urlproto_appname.length());//去除第一个URLPROTO_APP_NAME以及://字符
	int pos = func_args.length() - 1;
	if (func_args.substr(pos, 1) == "/")
		func_args.erase(pos - 1, 2);//去除最后一个}/字符
	else
		func_args.erase(pos, 1);

	//由于命令行、ie浏览器参数传递时会把原来json结构中的"号去掉，需要重新格式化参数为json格式
	if (func_args.find("\"", 0) == std::string::npos) {
		std::vector<std::string> strv;
		SplitString(func_args, strv, ",");
		func_args = "";
		for (std::vector<std::string>::size_type i = 0; i < strv.size(); i++) {
			std::vector<std::string> strv1;
			SplitString(strv[i], strv1, ":");
			strv1[0] = "\"" + strv1[0] + "\"";
			if (!isDegital(strv1[1]) && strtolower(strv1[1]) != "true" && strtolower(strv1[1]) != "false")
				strv1[1] = "\"" + strv1[1] + "\"";

			strv[i] = strv1[0] + ":" + strv1[1];
			if (i == 0)
				func_args = strv[i];
			else
				func_args += "," + strv[i];
		}
	}
	func_args = "{" + func_args + "}";
	EXLOGD(("%s\n"), func_args.c_str());
	//调用TsHttpRpc类里的_rpc_func_run_client启动客户端
	_rpc_func_run_client(func_args, buf);
}
#endif

#if 0
void TsHttpRpc::_rpc_func_run_client(const ex_astr& func_args, ex_astr& buf) {
    // 入参：{"ip":"192.168.5.11","port":22,"uname":"root","uauth":"abcdefg","authmode":1,"protocol":2}
    //   authmode: 1=password, 2=private-key
    //   protocol: 1=rdp, 2=ssh
    // SSH返回： {"code":0, "data":{"sid":"0123abcde"}}
    // RDP返回： {"code":0, "data":{"sid":"0123abcde0A"}}

    //Json::Reader jreader;
    Json::Value jsRoot;

    Json::CharReaderBuilder jcrb;
    std::unique_ptr<Json::CharReader> const jreader(jcrb.newCharReader());
    const char *str_json_begin = func_args.c_str();
    ex_astr err;

    //if (!jreader.parse(func_args.c_str(), jsRoot)) {
    if (!jreader->parse(str_json_begin, str_json_begin + func_args.length(), &jsRoot, &err)) {
        _create_json_ret(buf, TPE_JSON_FORMAT);
        return;
    }
    if (!jsRoot.isObject()) {
        _create_json_ret(buf, TPE_PARAM);
        return;
    }

    // 判断参数是否正确
    if (!jsRoot["teleport_ip"].isString()
        || !jsRoot["teleport_port"].isNumeric() || !jsRoot["remote_host_ip"].isString()
        || !jsRoot["session_id"].isString() || !jsRoot["protocol_type"].isNumeric() || !jsRoot["protocol_sub_type"].isNumeric()
        || !jsRoot["protocol_flag"].isNumeric()
        ) {
        _create_json_ret(buf, TPE_PARAM);
        return;
    }

    int pro_type = jsRoot["protocol_type"].asUInt();
    int pro_sub = jsRoot["protocol_sub_type"].asInt();
    ex_u32 protocol_flag = jsRoot["protocol_flag"].asUInt();

    ex_astr teleport_ip = jsRoot["teleport_ip"].asCString();
    int teleport_port = jsRoot["teleport_port"].asUInt();

    ex_astr remote_host_name = jsRoot["remote_host_name"].asCString();

    ex_astr real_host_ip = jsRoot["remote_host_ip"].asCString();
    ex_astr sid = jsRoot["session_id"].asCString();

    ex_wstr w_exe_path;
    WCHAR w_szCommandLine[MAX_PATH] = { 0 };


    ex_wstr w_sid;
    ex_astr2wstr(sid, w_sid);
    ex_wstr w_teleport_ip;
    ex_astr2wstr(teleport_ip, w_teleport_ip);
    ex_wstr w_real_host_ip;
    ex_astr2wstr(real_host_ip, w_real_host_ip);
    ex_wstr w_remote_host_name;
    ex_astr2wstr(remote_host_name, w_remote_host_name);
    WCHAR w_port[32] = { 0 };
    swprintf_s(w_port, _T("%d"), teleport_port);

    ex_wstr tmp_rdp_file; // for .rdp file

    if (pro_type == TP_PROTOCOL_TYPE_RDP) {
        //==============================================
        // RDP
        //==============================================

        bool flag_clipboard = ((protocol_flag & TP_FLAG_RDP_CLIPBOARD) == TP_FLAG_RDP_CLIPBOARD);
        bool flag_disk = ((protocol_flag & TP_FLAG_RDP_DISK) == TP_FLAG_RDP_DISK);
        bool flag_console = ((protocol_flag & TP_FLAG_RDP_CONSOLE) == TP_FLAG_RDP_CONSOLE);

        int rdp_w = 800;
        int rdp_h = 640;
        bool rdp_console = false;

        if (!jsRoot["rdp_width"].isNull()) {
            if (jsRoot["rdp_width"].isNumeric()) {
                rdp_w = jsRoot["rdp_width"].asUInt();
            } else {
                _create_json_ret(buf, TPE_PARAM);
                return;
            }
        }

        if (!jsRoot["rdp_height"].isNull()) {
            if (jsRoot["rdp_height"].isNumeric()) {
                rdp_h = jsRoot["rdp_height"].asUInt();
            } else {
                _create_json_ret(buf, TPE_PARAM);
                return;
            }
        }

        if (!jsRoot["rdp_console"].isNull()) {
            if (jsRoot["rdp_console"].isBool()) {
                rdp_console = jsRoot["rdp_console"].asBool();
            } else {
                _create_json_ret(buf, TPE_PARAM);
                return;
            }
        }

        if (!flag_console)
            rdp_console = false;


        int split_pos = sid.length() - 2;
        ex_astr real_sid = sid.substr(0, split_pos);
        ex_astr str_pwd_len = sid.substr(split_pos, sid.length());
        int n_pwd_len = strtol(str_pwd_len.c_str(), nullptr, 16);
        n_pwd_len -= real_sid.length();
        n_pwd_len -= 2;
        char szPwd[256] = { 0 };
        for (int i = 0; i < n_pwd_len; i++) {
            szPwd[i] = '*';
        }

        ex_astr2wstr(real_sid, w_sid);

        w_exe_path = _T("\"");
        w_exe_path += g_cfg.rdp_app + _T("\" ");

        ex_wstr rdp_name = g_cfg.rdp_name;
        if (rdp_name == L"mstsc") {
            w_exe_path += g_cfg.rdp_cmdline;

            int width = 0;
            int higth = 0;
            int cx = 0;
            int cy = 0;

            int display = 1;
            int iWidth = GetSystemMetrics(SM_CXSCREEN);
            int iHeight = GetSystemMetrics(SM_CYSCREEN);

            if (rdp_w == 0 || rdp_h == 0) {
                //全屏
                width = iWidth;
                higth = iHeight;
                display = 2;
            } else {
                width = rdp_w;
                higth = rdp_h;
                display = 1;
            }

            cx = (iWidth - width) / 2;
            cy = (iHeight - higth) / 2;
            if (cx < 0) {
                cx = 0;
            }
            if (cy < 0) {
                cy = 0;
            }

            // 			int console_mode = 0;
            // 			if (rdp_console)
            // 				console_mode = 1;

            std::string psw51b;
            if (!calc_psw51b(szPwd, psw51b)) {
                EXLOGE("calc password failed.\n");
                _create_json_ret(buf, TPE_FAILED);
                return;
            }

            real_sid = "01" + real_sid;

            char sz_rdp_file_content[4096] = { 0 };
            sprintf_s(sz_rdp_file_content, 4096, rdp_content.c_str()
                , (flag_console && rdp_console) ? 1 : 0
                , display, width, higth
                , cx, cy, cx + width + 100, cy + higth + 100
                , teleport_ip.c_str(), teleport_port
                , flag_clipboard ? 1 : 0
                , flag_disk ? "*" : ""
                , real_sid.c_str()
                , psw51b.c_str()
            );

            char sz_file_name[MAX_PATH] = { 0 };
            char temp_path[MAX_PATH] = { 0 };
            DWORD ret = GetTempPathA(MAX_PATH, temp_path);
            if (ret <= 0) {
                EXLOGE("fopen failed (%d).\n", GetLastError());
                _create_json_ret(buf, TPE_FAILED);
                return;
            }

            ex_astr temp_host_ip = real_host_ip;
            ex_replace_all(temp_host_ip, ".", "-");

            sprintf_s(sz_file_name, MAX_PATH, ("%s%s.rdp"), temp_path, temp_host_ip.c_str());

            FILE* f = NULL;
            if (fopen_s(&f, sz_file_name, "wt") != 0) {
                EXLOGE("fopen failed (%d).\n", GetLastError());
                _create_json_ret(buf, TPE_OPENFILE);
                return;
            }
            // Write a string into the file.
            fwrite(sz_rdp_file_content, strlen(sz_rdp_file_content), 1, f);
            fclose(f);
            ex_astr2wstr(sz_file_name, tmp_rdp_file);

            // 变量替换
            ex_replace_all(w_exe_path, _T("{tmp_rdp_file}"), tmp_rdp_file);
        } else if (g_cfg.rdp_name == L"freerdp") {
            w_exe_path += L"{size} {console} {clipboard} {drives} ";
            w_exe_path += g_cfg.rdp_cmdline;

            ex_wstr w_screen;

            if (rdp_w == 0 || rdp_h == 0) {
                //全屏
                w_screen = _T("/f");
            } else {
                char sz_size[64] = { 0 };
                ex_strformat(sz_size, 63, "/size:%dx%d", rdp_w, rdp_h);
                ex_astr2wstr(sz_size, w_screen);
            }

            // 			wchar_t* w_console = NULL;
            // 
            // 			if (flag_console && rdp_console)
            // 			{
            // 				w_console = L"/admin";
            // 			}
            // 			else
            // 			{
            // 				w_console = L"";
            // 			}

            ex_wstr w_password;
            ex_astr2wstr(szPwd, w_password);
            w_exe_path += L" /p:";
            w_exe_path += w_password;

            w_sid = L"02" + w_sid;

            w_exe_path += L" /gdi:sw"; // 使用软件渲染，gdi:hw使用硬件加速，但是会出现很多黑块（录像回放时又是正常的！）
            w_exe_path += L" -grab-keyboard"; // [new style] 防止启动FreeRDP后，失去本地键盘响应，必须得先最小化一下FreeRDP窗口（不过貌似不起作用）

            // 变量替换
            ex_replace_all(w_exe_path, _T("{size}"), w_screen);

            if (flag_console && rdp_console)
                ex_replace_all(w_exe_path, _T("{console}"), L"/admin");
            else
                ex_replace_all(w_exe_path, _T("{console}"), L"");

            //ex_replace_all(w_exe_path, _T("{clipboard}"), L"+clipboard");

            if (flag_clipboard)
                ex_replace_all(w_exe_path, _T("{clipboard}"), L"/clipboard");
            else
                ex_replace_all(w_exe_path, _T("{clipboard}"), L"-clipboard");

            if (flag_disk)
                ex_replace_all(w_exe_path, _T("{drives}"), L"/drives");
            else
                ex_replace_all(w_exe_path, _T("{drives}"), L"-drives");
        } else {
            _create_json_ret(buf, TPE_FAILED);
            return;
        }
    } else if (pro_type == TP_PROTOCOL_TYPE_SSH) {
        //==============================================
        // SSH
        //==============================================

        if (pro_sub == TP_PROTOCOL_TYPE_SSH_SHELL) {
            w_exe_path = _T("\"");
            w_exe_path += g_cfg.ssh_app + _T("\" ");
            w_exe_path += g_cfg.ssh_cmdline;
        } else {
            w_exe_path = _T("\"");
            w_exe_path += g_cfg.scp_app + _T("\" ");
            w_exe_path += g_cfg.scp_cmdline;
        }
    } else if (pro_type == TP_PROTOCOL_TYPE_TELNET) {
        //==============================================
        // TELNET
        //==============================================
        w_exe_path = _T("\"");
        w_exe_path += g_cfg.telnet_app + _T("\" ");
        w_exe_path += g_cfg.telnet_cmdline;
    }

    ex_replace_all(w_exe_path, _T("{host_ip}"), w_teleport_ip.c_str());
    ex_replace_all(w_exe_path, _T("{host_port}"), w_port);
    ex_replace_all(w_exe_path, _T("{user_name}"), w_sid.c_str());
    ex_replace_all(w_exe_path, _T("{host_name}"), w_remote_host_name.c_str());
    ex_replace_all(w_exe_path, _T("{real_ip}"), w_real_host_ip.c_str());
    ex_replace_all(w_exe_path, _T("{assist_tools_path}"), g_env.m_tools_path.c_str());


    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    Json::Value root_ret;
    ex_astr utf8_path;
    ex_wstr2astr(w_exe_path, utf8_path, EX_CODEPAGE_UTF8);
    root_ret["path"] = utf8_path;

    if (!CreateProcess(NULL, (wchar_t *)w_exe_path.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        EXLOGE(_T("CreateProcess() failed. Error=0x%08X.\n  %s\n"), GetLastError(), w_exe_path.c_str());
        root_ret["code"] = TPE_START_CLIENT;
        _create_json_ret(buf, root_ret);
        return;
    }

    root_ret["code"] = TPE_OK;
    _create_json_ret(buf, root_ret);
}

void TsHttpRpc::_rpc_func_rdp_play(const ex_astr& func_args, ex_astr& buf) {
    //Json::Reader jreader;
    Json::Value jsRoot;

    Json::CharReaderBuilder jcrb;
    std::unique_ptr<Json::CharReader> const jreader(jcrb.newCharReader());
    const char *str_json_begin = func_args.c_str();
    ex_astr err;

    //if (!jreader.parse(func_args.c_str(), jsRoot)) {
    if (!jreader->parse(str_json_begin, str_json_begin + func_args.length(), &jsRoot, &err)) {
        _create_json_ret(buf, TPE_JSON_FORMAT);
        return;
    }

    // 判断参数是否正确
    if (!jsRoot["rid"].isInt()
        || !jsRoot["web"].isString()
        || !jsRoot["sid"].isString()
        ) {
        _create_json_ret(buf, TPE_PARAM);
        return;
    }

    int rid = jsRoot["rid"].asInt();
    ex_astr a_url_base = jsRoot["web"].asCString();
    ex_astr a_sid = jsRoot["sid"].asCString();

    char cmd_args[1024] = { 0 };
    ex_strformat(cmd_args, 1023, "%s/%d", a_sid.c_str(), rid);

    ex_wstr w_url_base;
    ex_astr2wstr(a_url_base, w_url_base);
    ex_wstr w_cmd_args;
    ex_astr2wstr(cmd_args, w_cmd_args);

    ex_wstr w_exe_path;
    w_exe_path = _T("\"");
    w_exe_path += g_env.m_exec_path + _T("\\tp-player.exe\"");
    w_exe_path += _T(" \"");
    w_exe_path += w_url_base;
    w_exe_path += _T("/");
    w_exe_path += w_cmd_args;

    Json::Value root_ret;
    ex_astr utf8_path;
    ex_wstr2astr(w_exe_path, utf8_path, EX_CODEPAGE_UTF8);
    root_ret["cmdline"] = utf8_path;

    EXLOGD(w_exe_path.c_str());

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    if (!CreateProcess(NULL, (wchar_t *)w_exe_path.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        EXLOGE(_T("CreateProcess() failed. Error=0x%08X.\n  %s\n"), GetLastError(), w_exe_path.c_str());
        root_ret["code"] = TPE_START_CLIENT;
        _create_json_ret(buf, root_ret);
        return;
    }

    root_ret["code"] = TPE_OK;
    _create_json_ret(buf, root_ret);
    return;
}
#endif

void TsHttpRpc::_rpc_func_get_config(const ex_astr& func_args, ex_astr& buf) {
    Json::Value jr_root;
    jr_root["code"] = 0;
    jr_root["data"] = g_cfg.get_root();
    _create_json_ret(buf, jr_root);
}

void TsHttpRpc::_rpc_func_set_config(const ex_astr& func_args, ex_astr& buf) {
    //Json::Reader jreader;
    Json::Value jsRoot;
    Json::CharReaderBuilder jcrb;
    std::unique_ptr<Json::CharReader> const jreader(jcrb.newCharReader());
    const char *str_json_begin = func_args.c_str();
    ex_astr err;

    //if (!jreader.parse(func_args.c_str(), jsRoot)) {
    if (!jreader->parse(str_json_begin, str_json_begin + func_args.length(), &jsRoot, &err)) {
        _create_json_ret(buf, TPE_JSON_FORMAT);
        return;
    }

    if (!g_cfg.save(func_args))
        _create_json_ret(buf, TPE_FAILED);
    else
        _create_json_ret(buf, TPE_OK);
}

void TsHttpRpc::_rpc_func_file_action(const ex_astr& func_args, ex_astr& buf) {

    //Json::Reader jreader;
    Json::Value jsRoot;

    Json::CharReaderBuilder jcrb;
    std::unique_ptr<Json::CharReader> const jreader(jcrb.newCharReader());
    const char *str_json_begin = func_args.c_str();
    ex_astr err;

    //if (!jreader.parse(func_args.c_str(), jsRoot)) {
    if (!jreader->parse(str_json_begin, str_json_begin + func_args.length(), &jsRoot, &err)) {
        _create_json_ret(buf, TPE_JSON_FORMAT);
        return;
    }
    // 判断参数是否正确
    if (!jsRoot["action"].isNumeric()) {
        _create_json_ret(buf, TPE_PARAM);
        return;
    }
    int action = jsRoot["action"].asUInt();

    HWND hParent = GetForegroundWindow();
    if (NULL == hParent)
        hParent = g_hDlgMain;

    BOOL ret = FALSE;
    wchar_t wszReturnPath[MAX_PATH] = _T("");

    if (action == 1 || action == 2) {
        OPENFILENAME ofn;
        ex_wstr wsDefaultName;
        ex_wstr wsDefaultPath;
        StringCchCopy(wszReturnPath, MAX_PATH, wsDefaultName.c_str());

        ZeroMemory(&ofn, sizeof(ofn));

        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrTitle = _T("选择文件");
        ofn.hwndOwner = hParent;
        ofn.lpstrFilter = _T("可执行程序 (*.exe)\0*.exe\0");
        ofn.lpstrFile = wszReturnPath;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrInitialDir = wsDefaultPath.c_str();
        ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST;

        if (action == 1) {
            ofn.Flags |= OFN_FILEMUSTEXIST;
            ret = GetOpenFileName(&ofn);
        } else {
            ofn.Flags |= OFN_OVERWRITEPROMPT;
            ret = GetSaveFileName(&ofn);
        }
    } else if (action == 3) {
        BROWSEINFO bi;
        ZeroMemory(&bi, sizeof(BROWSEINFO));
        bi.hwndOwner = NULL;
        bi.pidlRoot = NULL;
        bi.pszDisplayName = wszReturnPath; //此参数如为NULL则不能显示对话框
        bi.lpszTitle = _T("选择目录");
        bi.ulFlags = BIF_RETURNONLYFSDIRS;
        bi.lpfn = NULL;
        bi.iImage = 0;   //初始化入口参数bi结束
        LPITEMIDLIST pIDList = SHBrowseForFolder(&bi);//调用显示选择对话框
        if (pIDList) {
            ret = true;
            SHGetPathFromIDList(pIDList, wszReturnPath);
        } else {
            ret = false;
        }
    } else if (action == 4) {
        ex_wstr wsDefaultName;
        ex_wstr wsDefaultPath;

        if (wsDefaultPath.length() == 0) {
            _create_json_ret(buf, TPE_PARAM);
            return;
        }

        ex_wstr::size_type pos = 0;

        while (ex_wstr::npos != (pos = wsDefaultPath.find(L"/", pos))) {
            wsDefaultPath.replace(pos, 1, L"\\");
            pos += 1;
        }

        ex_wstr wArg = L"/select, \"";
        wArg += wsDefaultPath;
        wArg += L"\"";
        if ((int)ShellExecute(hParent, _T("open"), _T("explorer"), wArg.c_str(), NULL, SW_SHOW) > 32)
            ret = true;
        else
            ret = false;
    }

    if (ret) {
        if (action == 1 || action == 2 || action == 3) {
            ex_astr utf8_path;
            ex_wstr2astr(wszReturnPath, utf8_path, EX_CODEPAGE_UTF8);
            Json::Value root;
            root["code"] = TPE_OK;
            root["path"] = utf8_path;
            _create_json_ret(buf, root);

            return;
        } else {
            _create_json_ret(buf, TPE_OK);
            return;
        }
    } else {
        _create_json_ret(buf, TPE_FAILED);
        return;
    }
}

void TsHttpRpc::_rpc_func_get_version(const ex_astr& func_args, ex_astr& buf) {
    Json::Value root_ret;
    ex_wstr w_version = TP_ASSIST_VER;
    ex_astr version;
    ex_wstr2astr(w_version, version, EX_CODEPAGE_UTF8);
    root_ret["version"] = version;
    root_ret["code"] = TPE_OK;
    _create_json_ret(buf, root_ret);
    return;
}
