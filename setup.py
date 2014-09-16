import os

from setuptools.extension import Extension
from setuptools.command.build_ext import build_ext
from setuptools import setup


def read(fname):
    with open(os.path.join(os.path.dirname(__file__), fname)) as f:
        return f.read()

ext_modules = [
    Extension(
        name='skipdict',
        sources=['skipdict.c', 'skiplist.c'],
        depends=['skiplist.h'],
    ),
]


setup(name="skipdict",
      version="1.0-dev",
      description="Python dictionary object permanently sorted by value.",
      long_description="\n\n".join((read('README.rst'), read('CHANGES.rst'))),
      license="BSD",
      author="Malthe Borch",
      author_email="mborch@gmail.com",
      cmdclass=dict(build_ext=build_ext),
      ext_modules=ext_modules,
      classifiers=[
          'Development Status :: 4 - Beta',
          'Intended Audience :: Developers',
          'License :: OSI Approved :: MIT License',
          'Programming Language :: C',
          'Programming Language :: Python :: 2.7',
          "Programming Language :: Python :: 3.3",
          "Programming Language :: Python :: 3.4",
          'Topic :: Software Development :: Libraries :: Python Modules',
      ],
      url="https://github.com/malthe/skipdict",
      zip_safe=False,
      test_suite="tests",
      py_modules=['skipdict'])
