# -*- coding: utf-8 -*-

import getopt
import json
import os
import platform
import sys

from builder.core.env import env
import builder.core.colorconsole as cc
import builder.core.utils as utils
from builder.core.context import *

options = list()
options_idx = 0
ctx = BuildContext()


def main():
    cc.set_default(sep='', end='\n')

    if not env.init(warn_miss_tool=True):
        return

    action = None
    argv = sys.argv[1:]
    if len(argv) >= 1:
        for i in range(len(argv)):
            if 'debug' == argv[i]:
                ctx.set_target(TARGET_DEBUG)
            elif 'release' == argv[i]:
                ctx.set_target(TARGET_RELEASE)
            elif argv[i] in ctx.dist_all:
                ctx.set_dist(argv[i])
            else:
                action = argv[i]

    make_options()

    if action is not None:
        if action == '-h' or action == '--help':
            max_name_len = 0
            for x in options:
                if x['id'] != '--SPLIT-LINE--':
                    max_name_len = max(len(x['name']), max_name_len)
            max_name_len += 4

            for x in options:
                if x['id'] != '--SPLIT-LINE--':
                    name_pad = max_name_len - len(x['name'])
                    cc.o((cc.CR_INFO, x['name']), (cc.CR_VERBOSE, ' ' * name_pad), (cc.CR_VERBOSE, x['disp']))

            return

        # cc.v(action)
        opt = select_option_by_name(action)
        if opt is None:
            cc.e('unknown config: ', action)
            return

        do_opt(opt)
        return

    show_logo()
    while True:
        x = show_menu()
        if x == 'q':
            break

        try:
            x = int(x)
        except:
            cc.e('invalid input.')
            continue

        opt = select_option_by_id(int(x))
        # if 'config' == opt['name']:
        #     if make_config():
        #         make_options()
        #     continue

        if opt is None:
            cc.e('unknown selection: ', x)
            continue

        do_opt(opt)

        cc.w('\ntask finished, press Enter to continue or Q to quit...', end='')
        try:
            x = input()
        except EOFError:
            x = 'q'
        if x == 'q':
            break


def clean_all():
    # cc.e('sorry, clean not implemented yet.')
    utils.remove(os.path.join(env.root_path, 'out'))


def clean_everything():
    utils.remove(os.path.join(env.root_path, 'out'))
    utils.remove(os.path.join(env.root_path, 'external', 'jsoncpp'))
    utils.remove(os.path.join(env.root_path, 'external', 'libuv'))
    utils.remove(os.path.join(env.root_path, 'external', 'mbedtls'))
    utils.remove(os.path.join(env.root_path, 'external', 'mongoose'))
    utils.remove(os.path.join(env.root_path, 'external', 'openssl'))
    utils.remove(os.path.join(env.root_path, 'external', 'python'))
    utils.remove(os.path.join(env.root_path, 'external', 'libssh-win-static', 'lib'))
    utils.remove(os.path.join(env.root_path, 'external', 'libssh-win-static', 'src'))
    utils.remove(os.path.join(env.root_path, 'external', 'linux', 'tmp'))
    utils.remove(os.path.join(env.root_path, 'external', 'linux', 'release', 'lib', 'libmbedcrypto.a'))
    utils.remove(os.path.join(env.root_path, 'external', 'linux', 'release', 'lib', 'libmbedtls.a'))
    utils.remove(os.path.join(env.root_path, 'external', 'linux', 'release', 'lib', 'libmbedx509.a'))
    utils.remove(os.path.join(env.root_path, 'external', 'linux', 'release', 'lib', 'libsqlite3.a'))
    utils.remove(os.path.join(env.root_path, 'external', 'linux', 'release', 'lib', 'libssh.a'))
    utils.remove(os.path.join(env.root_path, 'external', 'linux', 'release', 'lib', 'libssh_threads.a'))
    utils.remove(os.path.join(env.root_path, 'external', 'linux', 'release', 'lib', 'libuv.a'))


def clean_external():
    # utils.remove(os.path.join(env.root_path, 'out'))
    utils.remove(os.path.join(env.root_path, 'external', 'jsoncpp'))
    utils.remove(os.path.join(env.root_path, 'external', 'libuv'))
    utils.remove(os.path.join(env.root_path, 'external', 'mbedtls'))
    utils.remove(os.path.join(env.root_path, 'external', 'mongoose'))
    # utils.remove(os.path.join(env.root_path, 'external', 'openssl'))
    # utils.remove(os.path.join(env.root_path, 'external', 'python'))
    # utils.remove(os.path.join(env.root_path, 'external', 'libssh-win-static', 'lib'))
    # utils.remove(os.path.join(env.root_path, 'external', 'libssh-win-static', 'src'))
    # utils.remove(os.path.join(env.root_path, 'external', 'linux', 'tmp'))
    utils.remove(os.path.join(env.root_path, 'external', 'linux', 'release', 'lib', 'libmbedcrypto.a'))
    utils.remove(os.path.join(env.root_path, 'external', 'linux', 'release', 'lib', 'libmbedtls.a'))
    utils.remove(os.path.join(env.root_path, 'external', 'linux', 'release', 'lib', 'libmbedx509.a'))
    utils.remove(os.path.join(env.root_path, 'external', 'linux', 'release', 'lib', 'libsqlite3.a'))
    utils.remove(os.path.join(env.root_path, 'external', 'linux', 'release', 'lib', 'libssh.a'))
    utils.remove(os.path.join(env.root_path, 'external', 'linux', 'release', 'lib', 'libssh_threads.a'))

    utils.remove(os.path.join(env.root_path, 'external', 'linux', 'release', 'lib', 'libuv.a'))


def do_opt(opt):
    arg = ''

    if 'ver' == opt['name']:
        script = 'build-version.py'

    elif 'pysrt' == opt['name']:
        script = 'build-pysrt.py'

    elif opt['name'] in ['ext-client', 'ext-server', 'clear-ext-client', 'clear-ext-server']:
        script = 'build-external.py'
        arg = '%s %s %s' % (opt['name'], ctx.target_path, opt['bits'])

    elif 'server' == opt['name']:
        script = 'build-server.py'
        arg = '%s %s server' % (ctx.target_path, opt['bits'])

    elif 'server-installer' == opt['name']:
        script = 'build-installer.py'
        arg = '%s %s server-installer' % (ctx.dist, opt['bits'])

    elif 'client' == opt['name']:
        script = 'build-assist.py'
        arg = '%s %s exe' % (ctx.target_path, opt['bits'])

    elif 'client-installer' == opt['name']:
        script = 'build-assist.py'
        arg = '%s %s installer' % (ctx.dist, opt['bits'])

    else:
        cc.e('unknown option: ', opt['name'])
        return

    cmd = '%s -B %s %s' % (env.py_exec, os.path.join(env.builder_path, script), arg)
    os.system(cmd)


def select_option_by_name(name):
    for x in options:
        if x['id'] != '--SPLIT-LINE--':
            if name == x['name']:
                return x

    return None


def select_option_by_id(_id):
    for x in options:
        if x['id'] == _id:
            return x

    return None


def add_option(bits, name, disp):
    global options, options_idx
    options_idx += 1
    options.append({'id': options_idx, 'name': name, 'disp': disp, 'bits': bits})


def add_split(title=None):
    global options
    options.append({'id': '--SPLIT-LINE--', 'title': title})


def make_options():
    if ctx.host_os in ['windows']:
        add_split('prepare external [build once]')
        # add_option('x86', 'external', '[OBSOLETE] Build external dependency')
        add_option('x86', 'ext-client', '[client] Build external libraries for client')
        # add_split('prepare for server [build once]')
        add_option('x86', 'pysrt', '[server] Make Python-Runtime for python%s-x86' % env.py_ver_str)
        add_option('x86', 'ext-server', '[server] Build external libraries for server')
        add_split('version [build every release]')
        add_option('x86', 'ver', 'Update version setting')
        add_split('client side')
        # add_option('x86', 'assist-exe', '[OBSOLETE] Assist Execute [%s]' % ctx.target_path)
        add_option('x86', 'client', 'Build client applications [%s]' % ctx.target_path)
        # add_option('x86', 'assist-rdp', 'Teleport RDP [%s]' % ctx.target_path)
        # add_option('x86', 'assist-installer', '[OBSOLETE] Assist Installer')
        add_option('x86', 'client-installer', 'Make client installer')
        add_split('server side')
        add_option('x86', 'pysrt', 'Make Python-Runtime for python%s-x86' % env.py_ver_str)
        add_option('x86', 'ext-server', 'Build external libraries for server')
        # add_option('x86', 'server', 'Teleport Server [%s]' % ctx.target_path)
        add_option('x86', 'server-installer', 'Teleport Installer for %s' % ctx.host_os)
        # add_option('x86', 'installer', '[OBSOLETE] Teleport Installer for %s' % ctx.host_os)
        add_split('clear')
        add_option('x86', 'clear-ext-client', 'Clear external libraries for client')
        add_option('x86', 'clear-ext-server', 'Clear external libraries for server')
    elif ctx.host_os == 'macos':
        add_split('client side')
        add_option('x64', 'ext-client', 'build external libraries for client')
        add_option('x64', 'client', 'build client applications [%s]' % ctx.target_path)
        add_option('x64', 'client-installer', 'make client installer')
        add_split('server side')
        add_option('x64', 'ext-server', '(DEV-ONLY) build external libraries for server')
        add_option('x64', 'server', '(DEV-ONLY) build server applications for MacOS [%s]' % ctx.target_path)
        add_split('clear')
        add_option('x64', 'clear-ext-client', 'clear external libraries for client')
        add_option('x64', 'clear-ext-server', 'clear external libraries for server')
        add_split('misc')
        add_option('x64', 'ver', 'update version setting')
    else:
        add_split('prepare for server [build once]')
        add_option('x64', 'pysrt', 'Make Python-Runtime for python%s-x64' % env.py_ver_str)
        add_split('client side')
        # add_option('x64', 'ext-client', 'build external libraries for client')
        add_option('x64', 'client', 'build client applications [%s]' % ctx.target_path)
        # add_option('x64', 'client-installer', 'make client installer')
        add_split('server side')
        add_option('x64', 'ext-server', 'build external libraries for server')
        add_option('x64', 'server', 'build server applications [%s]' % ctx.target_path)
        add_option('x64', 'server-installer', 'make server installer for %s' % ctx.host_os)
        add_split('clear')
        # add_option('x64', 'clear-ext-client', 'Clear external libraries for client')
        add_option('x64', 'clear-ext-server', 'clear external libraries for server')
        add_split('misc')
        add_option('x64', 'ver', 'update version setting')


def get_input(msg, log_func=cc.w):
    log_func(msg, end=' ')
    try:
        return input()
    except EOFError:
        return ''


def show_logo():
    cc.v('[]==========================================================[]')
    cc.v(' | Teleport Projects Builder v2.0                           |')
    cc.v(' | auth: apex.liu@qq.com                                    |')
    cc.v('[]==========================================================[]')


def show_menu():
    cc.v('\n=====================[ MENU ]===============================')
    for o in options:
        if o['id'] == '--SPLIT-LINE--':
            if o['title'] is not None:
                cc.w('\n  {}:'.format(o['title']))
            else:
                cc.v('\n  ----------------------------------------------------------')
            continue

        cc.o((cc.CR_NORMAL, '  ['), (cc.CR_INFO, '%2d' % o['id']), (cc.CR_NORMAL, '] ', o['disp']))

    cc.v('\n  ----------------------------------------------------------')
    cc.o((cc.CR_NORMAL, '  ['), (cc.CR_INFO, ' Q'), (cc.CR_NORMAL, '] exit'))

    cc.w('\nselect action: ', end='')
    try:
        x = input()
    except EOFError:
        x = 'q'

    cc.n('')
    return x.lower()


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass
    except RuntimeError as e:
        cc.e(e.__str__())
    except:
        cc.f('got an exception.')
