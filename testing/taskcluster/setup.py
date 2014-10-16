import os
from setuptools import setup, find_packages
import sys

version = '0.0.0'

# dependencies
with open('requirements.txt') as f:
    deps = f.read().splitlines()

setup(name='taskcluster_graph',
      version=version,
      description='',
      long_description='See http://marionette-client.readthedocs.org/',
      classifiers=[], # Get strings from http://pypi.python.org/pypi?%3Aaction=list_classifiers
      keywords='mozilla',
      license='MPL',
      packages=['logic'],
      install_requires=deps,
      )
