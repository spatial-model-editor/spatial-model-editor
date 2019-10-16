TEMPLATE = subdirs

SUBDIRS = \
          src \
          app \
          test \

app.depends = src
test.depends = src
