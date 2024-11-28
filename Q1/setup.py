from setuptools import setup, Extension

module = Extension('metricsreader',
                  sources=['metricsreader.c'])

setup(name='metricsreader',
      version='1.0',
      ext_modules=[module])