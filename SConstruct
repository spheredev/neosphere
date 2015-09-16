import os
import shutil

out_dir = os.path.join(os.getcwd(), "bin")

msphere_src_dir = os.path.join(os.getcwd(), "src")
cell_src_dir = os.path.join(os.getcwd(), "cell")

minisphere = SConscript(dirs=[msphere_src_dir], variant_dir="obj/msphere")
cell = SConscript(dirs=[cell_src_dir], variant_dir="obj/cell")

if not os.path.exists(out_dir):
  os.makedirs(out_dir)

Install(out_dir, minisphere)
Install(out_dir, cell)
