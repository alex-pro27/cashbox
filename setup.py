from distutils.core import setup, Extension, DEBUG

cpp_args = []

sfc_module = Extension(
    'cashbox', sources = ['cashbox.cpp'],
    include_dirs=[],
    language='c++',
    extra_compile_args = cpp_args,
)

setup(
    name = 'cashbox', version = '1.0.3',
    description = 'Python Package with superfastcode C++ extension',
    #ext_modules = [sfc_module],
    data_files=[('lib/site-packages',['./PiritLib.dll', './cashbox.pyd'])],
)
