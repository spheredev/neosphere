import os
import shutil

out_dir = os.path.join(os.getcwd(), "bin")

msphere_src_dir = os.path.join(os.getcwd(), "src/minisphere")
cell_src_dir = os.path.join(os.getcwd(), "src/cell")
ssj_src_dir = os.path.join(os.getcwd(), "src/ssj")

minisphere = SConscript(dirs=[msphere_src_dir], variant_dir="obj/minisphere")
cell = SConscript(dirs=[cell_src_dir], variant_dir="obj/cell")
ssj = SConscript(dirs=[ssj_src_dir], variant_dir="obj/ssj")

if not os.path.exists(out_dir):
  os.makedirs(out_dir)

Install(out_dir, minisphere)
Install(out_dir, cell)
Install(out_dir, ssj)

