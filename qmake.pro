TEMPLATE = subdirs

SUBDIRS = \
          src \
          app \
          test \
          test_unity \

app.depends = src
test.depends = src
test_unity.depends = src
