lessThan(QT_VERSION, 5.2) {
	error("NovProg requires Qt 5.2 or greater")
}
TEMPLATE = app
QT += widgets
CONFIG += warn_on c++11

DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x050800
DEFINES += QT_NO_NARROWING_CONVERSIONS_IN_CONNECT

# Allow in-tree builds
!win32 {
	MOC_DIR = build
	OBJECTS_DIR = build
	RCC_DIR = build
}

# Set program version
VERSION = 3.1.1
DEFINES += VERSIONSTR=\\\"$${VERSION}\\\"

# Set program name
unix: !macx {
	TARGET = novprog
} else {
	TARGET = NovProg
}

# Specify program sources
HEADERS = src/data.h \
	src/graph.h \
	src/locale_dialog.h \
	src/novel_dialog.h \
	src/window.h

SOURCES = src/data.cpp \
	src/graph.cpp \
	src/locale_dialog.cpp \
	src/novel_dialog.cpp \
	src/window.cpp \
	src/main.cpp

# Generate translations
TRANSLATIONS = $$files(translations/novprog_*.ts)
qtPrepareTool(LRELEASE, lrelease)
updateqm.input = TRANSLATIONS
updateqm.output = ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}.qm
updateqm.commands = $$LRELEASE -silent ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
updateqm.CONFIG += no_link target_predeps
QMAKE_EXTRA_COMPILERS += updateqm

# Install program data
RESOURCES += icons/icon-application-menu.qrc

macx {
	ICON = icons/novprog.icns
} else:win32 {
	RC_FILE = icons/icon.rc
} else:unix {
	isEmpty(PREFIX) {
		PREFIX = /usr
	}
	isEmpty(BINDIR) {
		BINDIR = $$PREFIX/bin
	}
	isEmpty(DATADIR) {
		DATADIR = $$PREFIX/share
	}

	RESOURCES += icons/icon.qrc

	target.path = $$BINDIR

	icon.files = icons/hicolor/*
	icon.path = $$DATADIR/icons/hicolor

	pixmap.files = icons/novprog.xpm
	pixmap.path = $$DATADIR/pixmaps

	desktop.files = icons/novprog.desktop
	desktop.path = $$DATADIR/applications

	appdata.files = icons/novprog.appdata.xml
	appdata.path = $$DATADIR/metainfo

	qm.files = translations/*.qm
	qm.path = $$DATADIR/novprog/translations
	qm.CONFIG += no_check_exist

	man.files = doc/novprog.1
	man.path = $$DATADIR/man/man1

	INSTALLS += target icon pixmap desktop appdata qm man
}
