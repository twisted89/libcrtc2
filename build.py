#!/usr/bin/env python

import os
import sys
import re
import subprocess
import shutil
import tarfile
import platform
import fileinput
from io import BytesIO
from urllib.request import urlopen
from zipfile import ZipFile

root_dir = os.path.dirname(os.path.realpath(__file__))
dist_dir = os.path.join(root_dir, 'dist')
dist_include_dir = os.path.join(dist_dir, 'include')
dist_lib_dir = os.path.join(dist_dir, 'lib')
out_dir = os.path.join(root_dir, 'out')
third_party_dir = os.path.join(root_dir, '3dparty')

fetch_cmd = 'fetch'
gclient_cmd = 'gclient'
gn_cmd = 'gn'
ninja_cmd = 'ninja'

target_platform = sys.platform

if sys.platform.startswith('linux'):
  target_platform = 'linux'
elif sys.platform.startswith('win32'):
  target_platform = 'win32'
  fetch_cmd = 'fetch.bat'
  gclient_cmd = 'gclient.bat'
  gn_cmd = 'gn.bat'
  ninja_cmd = 'ninja.bat'
  os.environ['DEPOT_TOOLS_WIN_TOOLCHAIN'] = '0'
  os.environ['vs2022_install'] = 'C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional'

target_os = target_platform
target_cpu = platform.machine()

if (target_cpu == 'x86_64'):
  target_cpu = 'x64'
elif (target_cpu == 'i386'):
  target_cpu = 'x86'

release_name = 'master'

if (os.environ.get('WEBRTC_TARGET_OS')):
  target_os = os.environ['WEBRTC_TARGET_OS']

if (os.environ.get('WEBRTC_TARGET_CPU')):
  target_cpu = os.environ['WEBRTC_TARGET_CPU']

pkg_name = 'libcrtc-' + release_name + '-' + target_os  + '-' + target_cpu + '.tar.gz'

def build_archive():
  with tarfile.open(os.path.join(dist_dir, pkg_name), "w:gz") as tar:
    tar.add(dist_include_dir, arcname='include')
    tar.add(dist_lib_dir, arcname='lib')
    
def search_text(file, searchExp):
  with open(file, 'r') as f:
    if searchExp in f.read():
      return True
    else:
      return False
    
def replace_text(file, searchExp, replaceExp):
  for line in fileinput.input(file, inplace=True, backup='.bak'):
    if searchExp in line:
      print(line.replace(searchExp, replaceExp))
    else:
      print(line, end='')
     
depot_tools_dir = os.path.join(third_party_dir, 'depot_tools')
if (target_os == 'android'):
  webrtc_dir = os.path.join(third_party_dir, 'webrtc_android')
else:
  webrtc_dir = os.path.join(third_party_dir, 'webrtc')

webrtc_src_dir = os.path.join(webrtc_dir, 'src')
webrtc_crtc_dir = os.path.join(webrtc_src_dir, 'crtc')
webrtc_sync = os.path.join(third_party_dir, '.webrtc_sync_' + target_os)

if not os.path.exists(third_party_dir):
  os.mkdir(third_party_dir)

if not os.path.exists(depot_tools_dir):
    subprocess.check_call(['git', 'clone', 'https://chromium.googlesource.com/chromium/tools/depot_tools.git', depot_tools_dir])

os.environ['PATH'] += os.pathsep + depot_tools_dir

if not os.path.exists(webrtc_dir):
  os.mkdir(webrtc_dir)

if not os.path.exists(webrtc_sync):
  os.chdir(webrtc_dir)

  if not os.path.exists(webrtc_src_dir):
    if (target_os == 'android'):
      subprocess.call([fetch_cmd, '--nohooks', '--nohistory', 'webrtc_android'])
    else:
      subprocess.call([fetch_cmd, '--nohooks', '--nohistory', 'webrtc'])
    
  else:
    os.chdir(webrtc_src_dir)

    subprocess.check_call(['git', 'fetch', 'origin'])
    subprocess.check_call(['git', 'reset', '--hard', 'origin/master'])
    subprocess.check_call(['git', 'checkout', 'origin/master'])
    subprocess.check_call(['git', 'clean', '-f'])

    os.chdir(webrtc_dir)
  
  subprocess.check_call([gclient_cmd, 'sync', '--with_branch_heads', '--force'])

  os.chdir(webrtc_src_dir)

  if os.path.exists(os.path.join(webrtc_src_dir, 'BUILD.gn')):
    os.remove(os.path.join(webrtc_src_dir, 'BUILD.gn'))

  if not os.path.exists(webrtc_crtc_dir):
    os.symlink(root_dir, webrtc_crtc_dir)

  os.symlink(os.path.join(root_dir, 'root.gn'), os.path.join(webrtc_src_dir, 'BUILD.gn'))
  open(webrtc_sync, 'a').close()
  
os.chdir(webrtc_src_dir)

if (target_os == 'android'):
  if search_text('./buildtools/third_party/libunwind/BUILD.gn', 'visibility += ["//build/config:common_deps"]') == False:
    print('Patching libunwind visibility...')
    replace_text('./buildtools/third_party/libunwind/BUILD.gn', 'visibility = [ "//buildtools/third_party/libc++abi" ]', 'visibility = [ "//buildtools/third_party/libc++abi" ]\n  visibility += ["//build/config:common_deps"]')
    replace_text('./build/config/BUILD.gn', 'if (use_custom_libcxx) {', 'if (is_android) {\n    public_deps += [ "//buildtools/third_party/libunwind" ]\n  } else if (use_custom_libcxx) {')
  else:
    print('Libunwind visibility appears to be patched...')

gn_flags = '--args=rtc_include_tests=false is_component_build=false rtc_use_h264=true ffmpeg_branding="Chrome" rtc_enable_protobuf=false treat_warnings_as_errors=false use_custom_libcxx=false'

if os.environ.get('WEBRTC_DEBUG') == 'true':
  gn_flags += ' is_debug=true'
else:
  gn_flags += ' is_debug=false'

if (target_os == 'linux'):
  gn_flags += ' use_ozone=true'
  gn_flags += ' is_desktop_linux=false'
  gn_flags += ' rtc_use_gtk=false'

if (target_os != target_platform):
  gn_flags += ' target_os="' + target_os + '"'

if (target_cpu != platform.machine()):
  gn_flags += ' target_cpu="' + target_cpu + '"'

subprocess.check_call([gn_cmd, 'gen', os.path.join(out_dir, target_os, target_cpu), gn_flags])

os.chdir(webrtc_dir)

if os.environ.get('WEBRTC_EXAMPLES') == 'true':
  subprocess.check_call([ninja_cmd, '-C', os.path.join(out_dir, target_os, target_cpu), 'crtc-examples'])
else:
  subprocess.check_call([ninja_cmd, '-C', os.path.join(out_dir, target_os, target_cpu), 'crtc'])

os.chdir(root_dir)
if not os.path.exists(dist_dir):
  os.mkdir(dist_dir)

if os.path.exists(dist_include_dir):
  shutil.rmtree(dist_include_dir)

os.mkdir(dist_include_dir)

if os.path.exists(dist_lib_dir):
  shutil.rmtree(dist_lib_dir)
  
os.mkdir(dist_lib_dir)

shutil.copy(os.path.join(root_dir, 'include', 'crtc.h'), dist_include_dir)

if sys.platform.startswith('linux'):
  shutil.copy(os.path.join(os.path.join(out_dir, target_os, target_cpu), 'libcrtc.so'), dist_lib_dir)

elif sys.platform.startswith('win32'):
  shutil.copy(os.path.join(os.path.join(out_dir, target_os, target_cpu), 'crtc.dll'), dist_lib_dir)
  shutil.copy(os.path.join(os.path.join(out_dir, target_os, target_cpu), 'crtc.dll.lib'), os.path.join(dist_lib_dir, 'crtc.lib'))

elif sys.platform.startswith('darwin'):
  shutil.copy(os.path.join(os.path.join(out_dir, target_os, target_cpu), 'libcrtc.dylib'), dist_lib_dir)

build_archive()
