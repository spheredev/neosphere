import os
import shutil

out_dir = os.path.join(os.getcwd(), "bin")
tools_dir = os.path.join(os.getcwd(), "bin/cli")

msphere_src_dir = os.path.join(os.getcwd(), "src")
cell_src_dir = os.path.join(os.getcwd(), "cell")

minisphere = SConscript(dirs=[msphere_src_dir], variant_dir="obj/msphere")
cell = SConscript(dirs=[cell_src_dir], variant_dir="obj/cell")

if not os.path.exists(out_dir):
  os.makedirs(out_dir)
if not os.path.exists(tools_dir):
  os.makedirs(tools_dir)

Install(out_dir, minisphere)
Install(tools_dir, cell)
