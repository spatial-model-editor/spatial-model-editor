TEMPLATE = subdirs

SUBDIRS = \
          core \
          gui \
          src \
          test \

core.subdir = src/core
gui.subdir = src/gui

gui.depends = core
src.depends = gui
test.depends = gui
