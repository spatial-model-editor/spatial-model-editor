TEMPLATE = subdirs

SUBDIRS = \
          core \
          gui \
          src \
          cli \
          test \
          benchmark \

core.subdir = src/core
gui.subdir = src/gui

gui.depends = core
cli.depends = core
src.depends = gui
test.depends = gui
benchmark.depends = core
