#ifndef __TS_WEB_RPC_H__
#define __TS_WEB_RPC_H__

#include "ts_session.h"

#include <json/json.h>

// ����log��־״̬
bool ts_web_rpc_register_core();

// ��������ID��ȡԶ��������Ϣ������������IP���˿ڣ��û����������˽Կ��Э����RDP��SSH�ȵȣ�
uint32_t ts_web_rpc_get_conn_info(int conn_id, TS_CONNECT_INFO& info);

// ��¼�Ự�Ŀ�ʼ
bool ts_web_rpc_session_begin(TS_CONNECT_INFO& info, int& record_id);

// update session state
bool ts_web_rpc_session_update(int id, int protocol_sub_type, int state);

//session ����
bool ts_web_rpc_session_end(const char* sid, int id, int ret_code);

uint32_t ts_web_rpc_get_connection_config(const char* token, const char* password, const char* client_ip, std::string& sid);

#endif // __TS_WEB_RPC_H__
