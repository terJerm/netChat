QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    applyfriendwid.cpp \
    chatdialog_code.cpp \
    chatfriendlistdelegate.cpp \
    chatfriendlistmodel.cpp \
    chatfriendlistview.cpp \
    chatmessagelistdelegate.cpp \
    chatmessagelistmodel.cpp \
    chatmessagelistview.cpp \
    chattextedit.cpp \
    chatuserlistdelegate.cpp \
    chatuserlistmodel.cpp \
    chatuserlistview.cpp \
    customdialog.cpp \
    defer.cpp \
    findpasswddialog.cpp \
    friendinfowidget.cpp \
    friendrequestdelegate.cpp \
    friendrequestmodel.cpp \
    global.cpp \
    httpmge.cpp \
    logdialog.cpp \
    main.cpp \
    mainwindow.cpp \
    myhoverbutton.cpp \
    mytipwidget.cpp \
    passwordlineedit.cpp \
    registerdialog.cpp \
    sendfiledialog.cpp \
    stackedwidgetanimator.cpp \
    tcpfilemgr.cpp \
    tcpmgr.cpp \
    timerbtn.cpp \
    usermgr.cpp

HEADERS += \
    applyfriendwid.h \
    chatdialog_code.h \
    chatfriendlistdelegate.h \
    chatfriendlistmodel.h \
    chatfriendlistview.h \
    chatmessagelistdelegate.h \
    chatmessagelistmodel.h \
    chatmessagelistview.h \
    chattextedit.h \
    chatuserlistdelegate.h \
    chatuserlistmodel.h \
    chatuserlistview.h \
    customdialog.h \
    defer.h \
    findpasswddialog.h \
    friendinfowidget.h \
    friendrequestdelegate.h \
    friendrequestmodel.h \
    global.h \
    httpmge.h \
    logdialog.h \
    mainwindow.h \
    myhoverbutton.h \
    mytipwidget.h \
    passwordlineedit.h \
    registerdialog.h \
    sendfiledialog.h \
    singleton.h \
    stackedwidgetanimator.h \
    tcpfilemgr.h \
    tcpmgr.h \
    timerbtn.h \
    usermgr.h

FORMS += \
    applyfriendwid.ui \
    chatdialog_code.ui \
    findpasswddialog.ui \
    logdialog.ui \
    mainwindow.ui \
    registerdialog.ui \
    sendfiledialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    re.qrc

DISTFILES += \
    config.ini

win32:CONFIG(debug, debug|release) {
    # 源文件路径（工程目录下的 config.ini）
    SourceConfig = $${PWD}/config.ini
    SourceConfig = $$replace(SourceConfig, /, \\)

    # 目标路径（exe 所在目录，debug 模式）
    TargetDir = $${OUT_PWD}/debug  # 如果是 release 模式，改为 release
    TargetDir = $$replace(TargetDir, /, \\)

    # 执行拷贝
    QMAKE_POST_LINK += copy /Y \"$$SourceConfig\" \"$$TargetDir\"
}
