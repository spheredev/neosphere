import os
import shutil

system_dir = os.path.join(os.getcwd(), "system")
minisphere_src_dir = os.path.join(os.getcwd(), "minisphere")

minisphere = SConscript(dirs=[minisphere_src_dir])

msphere_install = Install(os.getcwd(), minisphere)

if not os.path.exists(system_dir):
  os.makedirs(system_dir)

sys_install = Install(os.getcwd(), os.path.join(minisphere_src_dir, "assets/system"))

Depends(minisphere, sys_install)
