# -*- coding: utf8 -*-

import os
import platform
import sys
import json
import configparser

from . import colorconsole as cc

if platform.system().lower() == 'windows':
    try:
        import winreg
    except ImportError:
        cc.e('Can not load module `winreg`, so I can not locate toolchain for you.')


class Env(object):
    BITS_32 = 32
    BITS_64 = 64

    def __init__(self):
        _this_path = os.path.abspath(os.path.dirname(__file__))

        self.root_path = os.path.abspath(os.path.join(_this_path, '..', '..', '..'))
        self.build_path = os.path.abspath(os.path.join(_this_path, '..', '..'))
        self.builder_path = os.path.join(self.build_path, 'builder')

        self.py_ver = platform.python_version_tuple()
        self.py_ver_str = '%s%s' % (self.py_ver[0], self.py_ver[1])
        self.py_ver_dot = '%s.%s' % (self.py_ver[0], self.py_ver[1])
        self.py_exec = sys.executable

        self.bits = self.BITS_32
        self.bits_str = 'x86'

        _bits = platform.architecture()[0]
        if _bits == '64bit':
            self.bits = self.BITS_64
            self.bits_str = 'x64'

        self.is_win = False
        self.is_win_x64 = False
        self.is_linux = False
        self.is_macos = False

        _os = platform.system().lower()
        self.plat = ''
        if _os == 'windows':
            self.is_win = True
            self.plat = 'windows'
            self.is_win_x64 = 'PROGRAMFILES(X86)' in os.environ
        elif _os == 'linux':
            self.is_linux = True
            self.plat = 'linux'
        elif _os == 'darwin':
            self.is_macos = True
            self.plat = 'macos'

    def init(self, warn_miss_tool=False):
        if not self._load_config(warn_miss_tool):
            return False

        if not self._load_version():
            return False

        return True

    def _load_config(self, warn_miss_tool):
        _cfg_file = 'config.{}.json'.format(self.plat)
        _cfg_file = os.path.join(self.root_path, _cfg_file)
        if not os.path.exists(_cfg_file):
            cc.e('can not load configuration.\n\nplease copy `config.json.in` to `config.{}.json` and modify it to fit your condition and try again.'.format(self.plat))
            return False

        try:
            with open(_cfg_file, 'r') as f:
                _cfg = json.loads(f.read())
        except:
            cc.e('can ot load configuration file, not in JSON format.')
            return False

        if 'toolchain' not in _cfg:
            cc.e('invalid configuration file: need `toolchain` section.')
            return False

        _tmp = _cfg['toolchain']
        if self.is_win:
            if 'wget' in _tmp:
                self.wget = _tmp['wget']
            else:
                self.wget = None

            if self.wget is None or not os.path.exists(self.wget):
                if warn_miss_tool:
                    cc.w(' - can not find `wget.exe`, you can get it at https://eternallybored.org/misc/wget/')

            if '7z' in _tmp:
                self.zip7 = _tmp['7z']
            else:
                self.zip7 = None
            if self.zip7 is None or not os.path.exists(self.zip7):
                if warn_miss_tool:
                    cc.w(' - can not find `7z.exe`, you can get it at http://www.7-zip.org')

            if 'nasm' in _tmp:
                self.nasm = _tmp['nasm']
            else:
                self.nasm = self._get_nasm()

            if self.nasm is None or not os.path.exists(self.nasm):
                if warn_miss_tool:
                    cc.w(' - can not locate `nasm`, so I can not build openssl.')
            else:
                _nasm_path = os.path.abspath(os.path.join(self.nasm, '..'))
                os.environ['path'] = os.environ['path'] + ';' + _nasm_path

            if 'perl' in _tmp:
                self.perl = _tmp['perl']
            else:
                self.perl = self._get_perl()

            if self.perl is None or not os.path.exists(self.perl):
                if warn_miss_tool:
                    cc.w(' - can not locate `perl`, so I can not build openssl.')

            self.visual_studio_path = self._get_visual_studio_path()
            if self.visual_studio_path is None or not os.path.exists(self.visual_studio_path):
                if warn_miss_tool:
                    cc.w(' - can not locate Visual Studio installation, so I can build nothing.')

            if 'msbuild' in _tmp:
                self.msbuild = _tmp['msbuild']
            else:
                self.msbuild = self._get_msbuild()

            if self.msbuild is None or not os.path.exists(self.msbuild):
                if warn_miss_tool:
                    cc.w(' - can not locate `MSBuild`, so I can build nothing.')

            if 'nsis' in _tmp:
                self.nsis = _tmp['nsis']
            else:
                self.nsis = self._get_nsis()

            if self.nsis is None or not os.path.exists(self.nsis):
                if warn_miss_tool:
                    cc.w(' - can not locate `nsis`, so I can not make installer.')

            if 'cmake' in _tmp:
                self.cmake = _tmp['cmake']
            else:
                self.cmake = 'c:\\cmake\\bin\\cmake.exe'

            if 'qt_path' in _tmp:
                self.qt = _tmp['qt_path']
            else:
                self.qt = None

            if self.qt is None or not os.path.exists(self.qt):
                if warn_miss_tool:
                    cc.w(' - can not locate `qt_path`, so I can not build tp-player.')

        elif self.is_linux:
            if 'cmake' in _tmp:
                self.cmake = _tmp['cmake']
            else:
                self.cmake = '/usr/bin/cmake'

            if not os.path.exists(self.cmake):
                if warn_miss_tool:
                    cc.e(' - can not locate `cmake`, so I can not build binary from source.')

        elif self.is_macos:
            if 'qt_path' in _tmp:
                self.qt = _tmp['qt_path']
            else:
                self.qt = None

            if self.qt is None or not os.path.exists(self.qt):
                if warn_miss_tool:
                    cc.w(' - can not locate `qt`, so I can not build tp-player.')

            if 'cmake' in _tmp:
                self.cmake = _tmp['cmake']
            else:
                self.cmake = '/usr/local/bin/cmake'

            if not os.path.exists(self.cmake):
                if warn_miss_tool:
                    cc.e(' - can not locate `cmake`, so I can not build binary from source.')

        return True

    def _load_version(self):
        _ver_file = os.path.join(self.root_path, 'external', 'version.ini')
        if not os.path.exists(_ver_file):
            cc.e('can not load version configuration for external.')
            return False

        _cfg = configparser.ConfigParser()
        _cfg.read(_ver_file)
        if 'external_ver' not in _cfg.sections():
            cc.e('invalid configuration file: need `external_ver` section.')
            return False

        _tmp = _cfg['external_ver']
        try:
            _v_openssl = _tmp['openssl'].split(',')
            self.ver_ossl = _v_openssl[0].strip()
            self.ver_ossl_number = _v_openssl[1].strip()

            _v_zlib = _tmp['zlib'].split(',')
            self.ver_zlib = _v_zlib[0].strip()
            self.ver_zlib_number = _v_zlib[1].strip()

            self.ver_libuv = _tmp['libuv']
            self.ver_mbedtls = _tmp['mbedtls']
            # self.ver_sqlite = _tmp['sqlite']
            self.ver_libssh = _tmp['libssh']
            self.ver_jsoncpp = _tmp['jsoncpp']
            self.ver_mongoose = _tmp['mongoose']
        except KeyError:
            cc.e('invalid configuration file: not all necessary external version are set.')
            return False

        return True

    # def _get_msbuild(self):
    #     # 14.0 = VS2015
    #     # 12.0 = VS2012
    #     #  4.0 = VS2008
    #     chk = ['14.0', '12.0', '4.0']

    #     p = None
    #     for c in chk:
    #         p = self._winreg_read(winreg.HKEY_LOCAL_MACHINE, r'SOFTWARE\Microsoft\MSBuild\ToolsVersions\{}'.format(c), r'MSBuildToolsPath')
    #         if p is not None:
    #             break

    #     return os.path.join(p[0], 'MSBuild.exe') if p is not None else None

    # def _get_visual_studio_path(self):
    #     chk = ['14.0', '12.0', '4.0']
    #     p = None
    #     for c in chk:
    #         p = self._winreg_read(winreg.HKEY_LOCAL_MACHINE, r'SOFTWARE\Microsoft\VisualStudio\{}'.format(c), r'ShellFolder')
    #         if p is not None:
    #             break

    #     return p[0] if p is not None else None

    def _get_visual_studio_path(self):
        p = self._winreg_read(winreg.HKEY_LOCAL_MACHINE, r'SOFTWARE\WOW6432Node\Microsoft\VisualStudio\SxS\VS7', r'15.0')
        return p[0] if p is not None else None

    def _get_msbuild(self):
        vs2017 = self._get_visual_studio_path()
        if vs2017 is None:
            return None
        return os.path.join(vs2017, 'MSBuild', '15.0', 'Bin', 'MSBuild.exe')

    def _get_perl(self):
        p = self._winreg_read(winreg.HKEY_LOCAL_MACHINE, r'SOFTWARE\perl', 'BinDir')
        return p[0] if p is not None else None

    def _get_nasm(self):
        p = self._winreg_read(winreg.HKEY_CURRENT_USER, r'SOFTWARE\nasm', '')
        return os.path.join(p[0], 'nasm.exe') if p is not None else None

    def _get_nsis(self):
        p = self._winreg_read(winreg.HKEY_LOCAL_MACHINE, r'SOFTWARE\NSIS\Unicode', '')
        if p is None:
            p = self._winreg_read(winreg.HKEY_LOCAL_MACHINE, r'SOFTWARE\NSIS', '')
        return os.path.join(p[0], 'makensis.exe') if p is not None else None

    def _winreg_read(self, base, path, key):
        try:
            if self.is_win_x64:
                hkey = winreg.CreateKeyEx(base, path, 0, winreg.KEY_READ | winreg.KEY_WOW64_32KEY)
            else:
                hkey = winreg.CreateKeyEx(base, path, 0, winreg.KEY_READ)

            value = winreg.QueryValueEx(hkey, key)
            return value

        except OSError:
            return None


env = Env()
del Env

if __name__ == '__main__':
    pass
