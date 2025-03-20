from setuptools import setup
from setuptools.command.install import install
import shutil
import os
import site
import glob
import sysconfig

class CustomInstall(install):
    def run(self):
        install.run(self)  # run the original install

        # Copy all files from release/lib/linux-x86_64/ to standard library path
        src_dir = 'release/lib/linux-x86_64/'
        std_lib_path = '/lib64'
        target_stdlib_dir = os.path.join(std_lib_path)

        if not os.path.exists(target_stdlib_dir):
            os.makedirs(target_stdlib_dir)
        
        for file in glob.glob(os.path.join(src_dir, '*')):
            shutil.copy(file, target_stdlib_dir)

        # Copy only libgnirsioc.so to site-packages
        src_file = 'release/lib/linux-x86_64/libgnirsioc.so'
        site_packages = site.getsitepackages()[0]
        target_site_packages_dir = os.path.join(site_packages)

        if not os.path.exists(target_site_packages_dir):
            os.makedirs(target_site_packages_dir)

        shutil.copy(src_file, target_site_packages_dir)

setup(
    name='gnirsdc-lib',
    version='0.1.2',
    cmdclass={'install': CustomInstall},
    # other metadata
)

