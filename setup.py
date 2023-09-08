from setuptools import setup
from setuptools.command.install import install
import shutil
import os
import site

class CustomInstall(install):
    def run(self):
        install.run(self)  # run the original install
        src_file = 'libgnirsioc.so'
        site_packages = site.getsitepackages()[0]  # Get the first site-packages directory
        target_dir = os.path.join(site_packages)
        if not os.path.exists(target_dir):
            os.makedirs(target_dir)
        shutil.copy(src_file, target_dir)

setup(
    name='your_package',
    version='0.1',
    cmdclass={'install': CustomInstall},
    # other metadata
)
