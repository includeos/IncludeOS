#!/usr/bin/env python

from setuptools import setup, find_packages

setup(name='vmrunner',
      version='0.1',
      scripts=['vmrunner/boot'],
      packages=find_packages(),
      include_package_data=True,
      zip_safe=False
      )
