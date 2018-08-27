import io
import os
import platform
import subprocess
import sys

from distutils import log
from distutils.command.build_scripts import build_scripts
from distutils.dep_util import newer
from distutils.util import convert_path
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from setuptools.command.build_py import build_py


class CustomBuildExt(build_ext):

    def build_extension(self, ext):
        compiler_type = self.compiler.compiler_type

        if compiler_type == "unix":
            if ext.extra_compile_args is None:
                ext.extra_compile_args = []
            # fixes segmentation fault when python (and thus the extension
            # module) is compiled with -O3 and tree vectorize:
            # https://github.com/khaledhosny/psautohint/issues/16
            ext.extra_compile_args.append("-fno-tree-vectorize")

        build_ext.build_extension(self, ext)


class CustomBuildPy(build_py):

    def run(self):
        meson = ["meson",
                 "--buildtype=release",
                 "--strip",
                 "--default-library=static",
                 "_build",
                 "libpsautohint"]
        ninja = ["ninja", "-C", "_build"]
        try:
            subprocess.check_call(" ".join(meson), shell=True)
            subprocess.check_call(" ".join(ninja), shell=True)
        except subprocess.CalledProcessError:
            print('psautohint: Error executing build command.')
            sys.exit(1)

        build_py.run(self)


class CustomBuildScripts(build_scripts):

    def copy_scripts(self):
        self.mkpath(self.build_dir)
        outfiles = []
        updated_files = []
        for script in self.scripts:
            script = convert_path(script)
            outfile = os.path.join(self.build_dir, os.path.basename(script))
            outfiles.append(outfile)

            if not self.force and not newer(script, outfile):
                log.debug("psautohint: not copying %s (up-to-date)", script)
                continue

            try:
                f = io.open(script, "rb")
            except OSError:
                if not self.dry_run:
                    raise
                f = None
            else:
                first_line = f.readline()
                if not first_line:
                    f.close()
                    self.warn("psautohint: %s is an empty file (skipping)"
                              % script)
                    continue

            if f:
                f.close()
            updated_files.append(outfile)
            self.copy_file(script, outfile)

        return outfiles, updated_files


module1 = Extension("psautohint._psautohint",
                    include_dirs=[
                        "libpsautohint/include",
                    ],
                    sources=[
                        "python/psautohint/_psautohint.c",
                        "libpsautohint/src/ac.c",
                        "libpsautohint/src/acfixed.c",
                        "libpsautohint/src/auto.c",
                        "libpsautohint/src/bbox.c",
                        "libpsautohint/src/charpath.c",
                        "libpsautohint/src/charpathpriv.c",
                        "libpsautohint/src/charprop.c",
                        "libpsautohint/src/check.c",
                        "libpsautohint/src/control.c",
                        "libpsautohint/src/eval.c",
                        "libpsautohint/src/fix.c",
                        "libpsautohint/src/flat.c",
                        "libpsautohint/src/fontinfo.c",
                        "libpsautohint/src/gen.c",
                        "libpsautohint/src/head.c",
                        "libpsautohint/src/logging.c",
                        "libpsautohint/src/memory.c",
                        "libpsautohint/src/merge.c",
                        "libpsautohint/src/misc.c",
                        "libpsautohint/src/optable.c",
                        "libpsautohint/src/pick.c",
                        "libpsautohint/src/psautohint.c",
                        "libpsautohint/src/read.c",
                        "libpsautohint/src/report.c",
                        "libpsautohint/src/shuffle.c",
                        "libpsautohint/src/stemreport.c",
                        "libpsautohint/src/write.c",
                    ],
                    depends=[
                        "libpsautohint/include/psautohint.h",
                        "libpsautohint/src/ac.h",
                        "libpsautohint/src/basic.h",
                        "libpsautohint/src/bbox.h",
                        "libpsautohint/src/charpath.h",
                        "libpsautohint/src/fontinfo.h",
                        "libpsautohint/src/logging.h",
                        "libpsautohint/src/memory.h",
                        "libpsautohint/src/opcodes.h",
                        "libpsautohint/src/optable.h",
                        "libpsautohint/src/winstdint.h",
                        "libpsautohint/src/version.h",
                    ],
                    )

with io.open("README.md", encoding="utf-8") as readme:
    long_description = readme.read()

if platform.system() == 'Windows':
    extension = '.exe'
else:
    extension = ''

VERSION_TEMPLATE = """\
/* file generated by setuptools_scm
   don't change, don't track in version control */
#define PSAUTOHINT_VERSION "{version}"
"""

setup(name="psautohint",
      use_scm_version={
          "write_to": "libpsautohint/src/version.h",
          "write_to_template": VERSION_TEMPLATE,
      },
      description="Python wrapper for Adobe's PostScript autohinter",
      long_description=long_description,
      long_description_content_type='text/markdown',
      url='https://github.com/adobe-type-tools/psautohint',
      author='Adobe Type team & friends',
      author_email='afdko@adobe.com',
      license='Apache License, Version 2.0',
      package_dir={'': 'python'},
      packages=['psautohint'],
      ext_modules=[module1],
      scripts=["_build/autohintexe{}".format(extension)],
      entry_points={
          'console_scripts': [
              "psautohint = psautohint.__main__:main",
          ],
      },
      setup_requires=["setuptools_scm"],
      install_requires=[
          'fonttools>=3.1.2',
          'ufoLib',
      ],
      extras_require={
          "testing": [
              "pytest >= 3.0.0, <4",
              "pytest-cov >= 2.5.1, <3",
              "pytest-xdist >= 1.22.2, <2",
              "pytest-randomly >= 1.2.3, <2",
          ],
      },
      cmdclass={
          'build_ext': CustomBuildExt,
          'build_py': CustomBuildPy,
          'build_scripts': CustomBuildScripts,
      },
      classifiers=[
          'Development Status :: 4 - Beta',
          'Environment :: Console',
          'Intended Audience :: Developers',
          'License :: OSI Approved :: Apache Software License',
          'Natural Language :: English',
          'Operating System :: OS Independent',
          'Programming Language :: Python',
          'Programming Language :: Python :: 2',
          'Programming Language :: Python :: 3',
          'Topic :: Text Processing :: Fonts',
          'Topic :: Multimedia :: Graphics',
          'Topic :: Multimedia :: Graphics :: Graphics Conversion',
      ],
      )
