
verusoptlib = static_library('verusopt', 
  ['verusopt/verus_clhash_opt.cpp'],
  cpp_args: ['-mpclmul', '-msse4', '-msse4.1', '-msse4.2', '-mssse3', '-mavx', '-maes', '-O3'],
  include_directories : [include_spdlog, include_wh,include_json, include_httplib,include_trezorcrypto],
)

cpp_args = ['-msse4.1']
# if get_option('enable-gpu-miner') == true

kernel_files = files(
  './gpu/third_party/hashcat/inc_vendor.h',
  './gpu/third_party/hashcat/inc_types.h',
  './gpu/third_party/hashcat/inc_platform.h',
  './gpu/third_party/hashcat/inc_common.h',
  './gpu/third_party/hashcat/inc_hash_sha256.h',
  './gpu/third_party/hashcat/inc_platform.cl',
  './gpu/third_party/hashcat/inc_common.cl',
  './gpu/third_party/hashcat/inc_hash_sha256.cl',
  './gpu/kernel.cl',
  )
if get_option('opencl-legacy') == true
  cpp_args = [cpp_args,'-DOPENCL_LEGACY']
endif
kernelcat = run_command( ['sh', files('./gpu/generate_kernel.sh'), kernel_files], check: true).stdout().strip()
conf_data = configuration_data()
conf_data.set('KERNEL_SRC', kernelcat)
configure_file(input : 'gpu/kernel.hpp.in',
  output : 'kernel.hpp',
  configuration : conf_data)
gpusrc = [
  './gpu/start_gpu_miner.cpp',
  './gpu/worker.cpp'
  ]
opencl_dep = dependency('OpenCL')
dep = [ 
  opencl_dep]

 executable('wart-miner', vcs_dep, 
   [
     './api_call.cpp', 
     './cmdline/cmdline.cpp', 
     './device_pool.cpp', 
     './cpu/verus_worker.cpp', 
     './cpu/verus_job.cpp', 
     './verus_pool.cpp', 
     './log/trace.cpp', 
     './mine_job.cpp', 
     './main.cpp', 
     gpusrc,
     # cpusrc,
     src_trezorcrypto,
     src_wh, 
     src_spdlog
     ],
   c_args: ['-march=native', '-msse4.1'],
   cpp_args: cpp_args,
   include_directories : [include_trezorcrypto,include_spdlog, include_wh,include_json, include_httplib],
  link_with: [verusoptlib],
   dependencies: dep,
   install : true)
