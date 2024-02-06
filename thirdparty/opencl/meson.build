include_opencl=include_directories( [ './include','./loader'])
src_opencl_windows = files([
      'loader/windows/icd_windows.c',
      'loader/windows/icd_windows_apppackage.c',
      'loader/windows/icd_windows_dxgk.c',
      'loader/windows/icd_windows_envvars.c',
      'loader/windows/icd_windows_hkr.c',
      ])

src_opencl_linux = files(
  [
    'loader/linux/icd_linux.c',
    'loader/linux/icd_linux_envvars.c',
  ])
src_opencl = files(
  [
    'loader/platform.c',
    'loader/icd.c',
    'loader/icd_dispatch.c',
    'loader/icd_dispatch_generated.c',
    ]
  )
