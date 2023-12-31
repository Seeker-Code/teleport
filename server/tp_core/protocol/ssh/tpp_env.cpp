#include "tpp_env.h"

TppSshEnv g_ssh_env;

TppSshEnv::TppSshEnv() noexcept:
        bind_port(0) {
}

TppSshEnv::~TppSshEnv() = default;

bool TppSshEnv::_on_init(TPP_INIT_ARGS* args) {
    ex_path_join(replay_path, false, L"ssh", NULL);

    auto ps = args->cfg->GetSection(L"protocol-ssh");
    if (!ps) {
        EXLOGE("[ssh] invalid config(2).\n");
        return false;
    }

    ex_wstr tmp;
    if (!ps->GetStr(L"bind-ip", tmp)) {
        bind_ip = TS_SSH_PROXY_HOST;
    }
    else {
        ex_wstr2astr(tmp, bind_ip);
    }

    if (!ps->GetInt(L"bind-port", bind_port)) {
        bind_port = TS_SSH_PROXY_PORT;
    }

    return true;
}
