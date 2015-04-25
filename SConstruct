import os
import shutil

msphere_src_dir = os.path.join(os.getcwd(), "src")
out_dir = os.path.join(os.getcwd(), "bin")

minisphere = SConscript(dirs=[msphere_src_dir], variant_dir="obj")

if not os.path.exists(out_dir):
  os.makedirs(out_dir)

Install(out_dir, minisphere)
Install(out_dir, os.path.join(os.getcwd(), "distro/system"))
Install(out_dir, os.path.join(os.getcwd(), "distro/documentation"))
Install(out_dir, os.path.join(os.getcwd(), "changelog.txt"))
