include(../../plugins.pri)

TARGET = $$PLUGINS_PREFIX/CommandLineOptions/incdecvolumeoption

HEADERS += incdecvolumeoption.h

SOURCES += incdecvolumeoption.cpp

RESOURCES = translations/translations.qrc

LIBS += $$QMMPUI_LIB

target.path = $$LIB_DIR/qmmp/CommandLineOptions
INSTALLS += target
