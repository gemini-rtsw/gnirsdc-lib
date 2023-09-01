from setuptools import setup, find_packages

setup(
    name="libgnirsioc",
    version="0.1",
    packages=find_packages(),
    package_data={'': ['release/lib/linux-x86_64/libgnirsioc.so']},
)

