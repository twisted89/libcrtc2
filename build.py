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
third_party_dir = os.path.join(root_dir, '3dparty')
fetch_cmd = 'fetch'
gclient_cmd = 'gclient'
gn_cmd = 'gn'
ninja_cmd = 'ninja'
dist_folders = []

if sys.platform.startswith('linux'):
  current_platform = 'linux'
elif sys.platform.startswith('win32'):
  current_platform = 'win32'
  fetch_cmd = 'fetch.bat'
  gclient_cmd = 'gclient.bat'
  gn_cmd = 'gn.bat'
  ninja_cmd = 'ninja.bat'
  os.environ['DEPOT_TOOLS_WIN_TOOLCHAIN'] = '0'
  os.environ['vs2022_install'] = 'C:\\Program Files\\Microsoft Visual Studio\\2022\\Professional'
else:
  current_platform = sys.platform

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

def build_archive(dist_dir, platform, package_name):
  print('Building archive...')
  pkg_path = os.path.join(dist_dir, 'libcrtc-' + platform + '-' + package_name + '.tar.gz')
  with tarfile.open(pkg_path, "w:gz") as tar:
    for f in dist_folders:
      tar.add(f[0], f[1])
  print('Archive saved to ' + pkg_path)
      
def build(project, target_platform, cpu, is_debug):
  if(project == 'crtc'):
    dist_dir = os.path.join(root_dir, 'dist', target_platform, cpu)
    dist_folders.append((dist_dir, target_platform + '_' + cpu))
  else:
    dist_dir = os.path.join(root_dir, 'lib', target_platform, cpu)
  out_dir = os.path.join(root_dir, 'out')

  depot_tools_dir = os.path.join(third_party_dir, 'depot_tools')
  if (target_platform == 'android'):
    webrtc_dir = os.path.join(third_party_dir, 'webrtc_android')
  else:
    webrtc_dir = os.path.join(third_party_dir, 'webrtc')

  webrtc_src_dir = os.path.join(webrtc_dir, 'src')
  webrtc_crtc_dir = os.path.join(webrtc_src_dir, 'crtc')
  webrtc_sync = os.path.join(third_party_dir, '.webrtc_sync_' + target_platform)

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
      if (target_platform == 'android'):
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

    if not os.path.exists(webrtc_crtc_dir):
      os.symlink(root_dir, webrtc_crtc_dir)

    open(webrtc_sync, 'a').close()
    
  os.chdir(webrtc_src_dir)

  if (target_platform == 'android'):
    if search_text('./buildtools/third_party/libunwind/BUILD.gn', 'visibility += ["//build/config:common_deps"]') == False:
      print('Patching libunwind visibility...')
      replace_text('./buildtools/third_party/libunwind/BUILD.gn', 'visibility = [ "//buildtools/third_party/libc++abi" ]', 'visibility = [ "//buildtools/third_party/libc++abi" ]\n  visibility += ["//build/config:common_deps"]')
      replace_text('./build/config/BUILD.gn', 'if (use_custom_libcxx) {', 'if (is_android) {\n    public_deps += [ "//buildtools/third_party/libunwind" ]\n  } else if (use_custom_libcxx) {')
    else:
      print('Libunwind visibility appears to be patched...')

  gn_flags = '--args=rtc_include_tests=false is_component_build=false rtc_use_h264=true ffmpeg_branding="Chrome" rtc_enable_protobuf=false treat_warnings_as_errors=false use_custom_libcxx=false'

  if is_debug == True:
    gn_flags += ' is_debug=true'
  else:
    gn_flags += ' is_debug=false'

  if (target_platform == 'linux'):
    gn_flags += ' use_ozone=true'
    gn_flags += ' is_desktop_linux=false'
    gn_flags += ' rtc_use_gtk=false'

  if (target_platform != current_platform):
    gn_flags += ' target_os="' + target_platform + '"'

  if (cpu != platform.machine()):
    gn_flags += ' target_cpu="' + cpu + '"'

  subprocess.check_call([gn_cmd, 'gen', os.path.join(out_dir, target_platform, cpu), gn_flags])

  os.chdir(webrtc_dir)

  subprocess.check_call([ninja_cmd, '-C', os.path.join(out_dir, target_platform, cpu), project])

  os.chdir(root_dir)
  

  if(project == 'crtc'):
    dist_lib_dir = os.path.join(dist_dir, 'lib')
    dist_include_dir = os.path.join(dist_dir, 'include')
  
    os.makedirs(dist_dir, exist_ok=True)
    os.makedirs(dist_include_dir, exist_ok=True)
    os.makedirs(dist_lib_dir, exist_ok=True)
    shutil.copy(os.path.join(root_dir, 'include', 'crtc.h'), dist_include_dir)
    if target_platform == 'android':
      shutil.copy(os.path.join(os.path.join(out_dir, target_platform, cpu, 'lib.unstripped'), 'libcrtc.so'), dist_lib_dir)
    elif target_platform == 'linux':
      shutil.copy(os.path.join(os.path.join(out_dir, target_platform, cpu), 'libcrtc.so'), dist_lib_dir)
    elif target_platform == 'win32':
      shutil.copy(os.path.join(os.path.join(out_dir, target_platform, cpu), 'crtc.dll'), dist_lib_dir)
      shutil.copy(os.path.join(os.path.join(out_dir, target_platform, cpu), 'crtc.dll.lib'), os.path.join(dist_lib_dir, 'crtc.lib'))
    elif target_platform == 'darwin':
      shutil.copy(os.path.join(os.path.join(out_dir, target_platform, cpu), 'libcrtc.dylib'), dist_lib_dir)
  else:
    if os.path.exists(dist_dir):
      shutil.rmtree(dist_dir)
    os.makedirs(dist_dir, exist_ok=True)
    
    if target_platform == 'win32':
    shutil.copy(os.path.join(os.path.join(out_dir, target_platform, cpu, 'obj'), 'webrtc.lib'), os.path.join(dist_dir, 'webrtc.lib'))
    else:
    shutil.copy(os.path.join(os.path.join(out_dir, target_platform, cpu, 'obj'), 'libwebrtc.a'), dist_dir)
    
def arch_menu(project, platform):
  print()

  choice = input("""
        1: All
        2: x86
        3: x64
        4: arm
        5: arm64

        Select an architecture """)

  if choice == "1":
    build(project, platform, 'x86', False)
    build(project, platform, 'x64', False)
    build(project, platform, 'arm', False)
    build(project, platform, 'arm64', False)
    #build_archive(os.path.join(root_dir, 'lib'), platform, 'all')
  elif choice == "2":
    build(project, platform, 'x86', False)
    #build_archive(os.path.join(root_dir, 'lib'), platform, 'x86')
  elif choice=="3":
    build(project, platform, 'x64', False)
    #build_archive(os.path.join(root_dir, 'lib'), platform, 'x64')
  elif choice=="4":
    build(project, platform, 'arm', False)
    #build_archive(os.path.join(root_dir, 'lib'), platform, 'arm')
  elif choice=="5":    
    build(project, platform, 'arm64', False)
    #build_archive(os.path.join(root_dir, 'lib'), platform, 'arm64')
  else:
    arch_menu(project, platform)
    
def platform_menu(project)
  choice = input("""
    1: Windows
    2: Linux
    3: Android
    4: Mac

    Select a platform: """)
        
  if choice == "1":
      arch_menu(project, 'win32')
  elif choice == "2":
      arch_menu(project, 'linux')
  elif choice=="3":
      arch_menu(project, 'android')
  elif choice=="4":
      arch_menu(project, 'darwin')
  else:
      platform_menu(project)

def main():
  print("************CRTC2**************")
  print()
  
    choice = input("""
        1: CRTC2 Shared Library
        2: WebRTC Static Library

        Select an project to build """)

            
  if choice == "1":
      platform_menu('crtc')
  elif choice == "2":
      platform_menu('webrtc')
  else:
      main()
      
main()


