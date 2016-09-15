//--
// This file is part of Sonic Pi: http://sonic-pi.net
// Full project source: https://github.com/samaaron/sonic-pi
// License: https://github.com/samaaron/sonic-pi/blob/master/LICENSE.md
//
// Copyright 2013, 2014, 2015, 2016 by Sam Aaron (http://sam.aaron.name).
// All rights reserved.
//
// Permission is granted for use, copying, modification, and
// distribution of modified versions of this work as long as this
// notice is included.
//++


// Standard stuff
#include <iostream>
#include <math.h>
#include <sstream>
#include <fstream>

// Qt 5 only

// Qt stuff
#include <QSysInfo>
#include <QPainter>
#include <QDate>
#include <QDesktopServices>
#include <QDesktopWidget>


#include <QDir>
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QIcon>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QDockWidget>
#include <QPoint>
#include <QSettings>
#include <QSize>
#include <QStatusBar>
#include <QTextEdit>
#include <QTextBrowser>
#include <QToolBar>
#include <QProcess>
#include <QFont>
#include <QTabWidget>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QSplashScreen>
#include <QPixmap>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QGridLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QCheckBox>
#include <QScrollArea>
#include <QShortcut>
#include <QToolButton>
#include <QScrollBar>
#include <QSignalMapper>
#include <QSplitter>

// QScintilla stuff
#include <Qsci/qsciapis.h>
#include <Qsci/qsciscintilla.h>

#include "sonicpilexer.h"
#include "sonicpiapis.h"
#include "sonicpiscintilla.h"
#include "sonicpitheme.h"

#include "oschandler.h"
#include "sonicpilog.h"
#include "sonic_pi_udp_osc_server.h"
#include "sonic_pi_tcp_osc_server.h"

// OSC stuff
#include "oscpkt.hh"
#include "udp.hh"
using namespace oscpkt;// OS specific stuff
#if defined(Q_OS_WIN)
  #include <QtConcurrent/QtConcurrentRun>
  void sleep(int x) { Sleep((x)*1000); }
#elif defined(Q_OS_MAC)
  #include <QtConcurrent/QtConcurrentRun>
#else
//assuming Raspberry Pi
  #include <cmath>
  #include <QtConcurrentRun>
#endif

#if QT_VERSION >= 0x050400
  #include <QWindow>
#endif

#include "mainwindow.h"

#ifdef Q_OS_MAC
MainWindow::MainWindow(QApplication &app, bool i18n, QMainWindow* splash)
#else
MainWindow::MainWindow(QApplication &app, bool i18n, QSplashScreen* splash)
#endif
{
  QString root_path = rootPath();

#if defined(Q_OS_WIN)
  ruby_path = QDir::toNativeSeparators(root_path + "/app/server/native/windows/ruby/bin/ruby.exe");
#elif defined(Q_OS_MAC)
  ruby_path = root_path + "/app/server/native/osx/ruby/bin/ruby";
#else
  ruby_path = root_path + "/app/server/native/raspberry/ruby/bin/ruby";
#endif

  QFile file(ruby_path);
  if(!file.exists()) {
    // fallback to user's locally installed ruby
    ruby_path = "ruby";
  }

  ruby_server_path = QDir::toNativeSeparators(root_path + "/app/server/bin/sonic-pi-server.rb");
  sample_path = QDir::toNativeSeparators(root_path + "/etc/samples");

  sp_user_path           = QDir::toNativeSeparators(sonicPiHomePath() + "/.sonic-pi");
  sp_user_tmp_path       = QDir::toNativeSeparators(sp_user_path + "/.writableTesterPath");
  log_path               = QDir::toNativeSeparators(sp_user_path + "/log");
  server_error_log_path  = QDir::toNativeSeparators(log_path + "/server-errors.log");
  server_output_log_path = QDir::toNativeSeparators(log_path + "/server-output.log");
  gui_log_path           = QDir::toNativeSeparators(log_path + QDir::separator() + "gui.log");
  scsynth_log_path       = QDir::toNativeSeparators(log_path + QDir::separator() + "scsynth.log");

  init_script_path        = QDir::toNativeSeparators(root_path + "/app/server/bin/init-script.rb");
  exit_script_path        = QDir::toNativeSeparators(root_path + "/app/server/bin/exit-script.rb");


  // Clear out old tasks from previous sessions if they still exist
  std::cout << "[GUI] - running init script" << std::endl;
  QProcess *initProcess = new QProcess();
  initProcess->start(ruby_path, QStringList(init_script_path));
  initProcess->waitForFinished();

  QFile tmpFile(sp_user_tmp_path);
  if (!tmpFile.open(QIODevice::WriteOnly)) {
    homeDirWritable = false;
  }
  else {
    homeDirWritable = true;
    tmpFile.close();
  }

  loaded_workspaces = false;
  is_recording = false;
  show_rec_icon_a = false;
  restoreDocPane = false;
  focusMode = false;
  version = "2.11.0-relp-electric";
  latest_version = "";
  version_num = 0;
  latest_version_num = 0;
  this->splash = splash;
  this->i18n = i18n;
  guiID = QUuid::createUuid().toString();
  QSettings settings("uk.ac.cam.cl", "Sonic Pi");
  defaultTextBrowserStyle = "QTextBrowser { selection-color: white; selection-background-color: deeppink; padding-left:10; padding-top:10; padding-bottom:10; padding-right:10 ; background:white;}";

  QThreadPool::globalInstance()->setMaxThreadCount(3);
  app.installEventFilter(this);
  app.processEvents();

  protocol = UDP;
  if(protocol == TCP){
    clientSock = new QTcpSocket(this);
  }


  this->setWindowFlags(Qt::Widget | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
  this->setAttribute(Qt::WA_NoSystemBackground, true);
  this->setAttribute(Qt::WA_TranslucentBackground, true);


  setupLogPathAndRedirectStdOut();
  printAsciiArtLogo();

  setupTheme();
  lexer = new SonicPiLexer(theme);

  setupWindowStructure();
  createShortcuts();
  createToolBar();
  createStatusBar();
  createInfoPane();
  setWindowTitle(tr("Sonic Pi / Custom"));
  initPrefsWindow();

  readSettings();
  updateDarkMode();



  setUnifiedTitleAndToolBarOnMac(true);
  setWindowIcon(QIcon(":images/icon-smaller.png"));

  show_tabs->setChecked(false);
  show_buttons->setChecked(false);

  updateTabsVisibility();
  updateButtonVisibility();
  updateLogVisibility();
    
  initDocsWindow();

  //setup autocompletion
  autocomplete->loadSamples(sample_path);

  OscHandler* handler = new OscHandler(this, outputPane, errorPane, theme);

  if(protocol == UDP){
    sonicPiOSCServer = new SonicPiUDPOSCServer(this, handler);
    osc_thread = QtConcurrent::run(sonicPiOSCServer, &SonicPiOSCServer::start);
  }
  else{
    sonicPiOSCServer = new SonicPiTCPOSCServer(this, handler);
    sonicPiOSCServer->start();
  }


  // Wait to hear back from the server before continuing
  startRubyServer();
  if (waitForServiceSync()){
    // We have a connection! Finish up loading app...

    createTmpBufferDir();

    loadWorkspaces();
    requestVersion();

    splashClose();

    showWindow();
    updateDarkMode();
    updateFullScreenMode();
    showWelcomeScreen();
    
    changeWrapMode();
    
    connect(&app, SIGNAL( aboutToQuit() ), this, SLOT( onExitCleanup() ) );
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(heartbeatOSC()));
    timer->start(1000);
  }
}

void MainWindow::createTmpBufferDir() {
  //Create temporary file store (if possible) for sharing buffers locally with server.
  //Attempt to use a standard dir SonicPiGUIBufferStore - but if that cant' be
  //created, then attempt to use a unique dir name.
  tmp_file_store = QDir::toNativeSeparators(QDir::tempPath() + "/" + "sonic-pi-gui-buffer-store");
  std::cout << "[GUI] - Making tmp dir for sending buffers locally: " << tmp_file_store.toStdString() << std::endl;
  tmpFileStoreAvailable = QDir().mkdir(tmp_file_store);
  if(!tmpFileStoreAvailable) {
    std::cout << "[GUI] - Unable to create tmp dir: " << tmp_file_store.toStdString() << std::endl;
    tmp_file_store = QDir::toNativeSeparators(QDir::tempPath() + "/" + "sonic-pi-gui-buffer-store-" + guiID.mid(1, 13));
    std::cout << "[GUI] - Making unique tmp dir for sending buffers locally: " << tmp_file_store.toStdString() << std::endl;
    tmpFileStoreAvailable = QDir().mkdir(tmp_file_store);
    if(!tmpFileStoreAvailable) {
      std::cout << "[GUI] - Still unable to create tmp dir: " << tmp_file_store.toStdString() << std::endl;
    }
  }
}

void MainWindow::showWelcomeScreen() {
  QSettings settings("uk.ac.cam.cl", "Sonic Pi");
if(settings.value("first_time", 1).toInt() == 1) {
    QTextBrowser* startupPane = new QTextBrowser;
    startupPane->setFixedSize(600, 615);
    startupPane->setWindowIcon(QIcon(":images/icon-smaller.png"));
    startupPane->setWindowTitle(tr("Welcome to Sonic Pi"));
    addUniversalCopyShortcuts(startupPane);
    startupPane->document()->setDefaultStyleSheet(readFile(":/theme/light/doc-styles.css"));
    startupPane->setSource(QUrl("qrc:///html/startup.html"));
    startupPane->setStyleSheet(defaultTextBrowserStyle);
    docWidget->show();
    startupPane->show();
  }
}

void MainWindow::setupTheme() {
  // Syntax highlighting
  QString themeFilename = QDir::homePath() + QDir::separator() + ".sonic-pi" + QDir::separator() + "theme.properties";
  QFile themeFile(themeFilename);
  if(themeFile.exists()){
    std::cout << "[GUI] - using custom editor colours" << std::endl;
    QSettings settings(themeFilename, QSettings::IniFormat);
    theme = new SonicPiTheme(this, &settings, settings.value("prefs/dark-mode").toBool());
  }
  else{

    std::cout << "[GUI] - using default editor colours" << std::endl;
    QSettings settings("uk.ac.cam.cl", "Sonic Pi");
    theme = new SonicPiTheme(this, 0, settings.value("prefs/dark-mode").toBool());

  }
}

void MainWindow::setupWindowStructure() {

  setUnifiedTitleAndToolBarOnMac(true);
  setWindowIcon(QIcon(":images/icon-smaller.png"));

  rec_flash_timer = new QTimer(this);
  connect(rec_flash_timer, SIGNAL(timeout()), this, SLOT(toggleRecordingOnIcon()));

  // Setup output and error panes

  outputPane = new SonicPiLog;
  errorPane = new QTextBrowser;
  errorPane->setOpenExternalLinks(true);
  update_info = new QLabel(tr("Sonic Pi update info"));
  update_info->setWordWrap(true);

  // Window layout
  tabs = new QTabWidget();
  tabs->setTabsClosable(false);
  tabs->setMovable(false);
  tabs->setTabPosition(QTabWidget::South);

  lexer->setAutoIndentStyle(SonicPiScintilla::AiMaintain);

  // create workspaces and add them to the tabs
  // workspace shortcuts
  signalMapper = new QSignalMapper (this) ;
  retSignalMapper = new QSignalMapper (this) ;
  for(int ws = 0; ws < workspace_max; ws++) {
    std::string s;


    SonicPiScintilla *workspace = new SonicPiScintilla(lexer, theme);

    //tab completion when in list
    QShortcut *indentLine = new QShortcut(QKeySequence("Tab"), workspace);
    connect (indentLine, SIGNAL(activated()), signalMapper, SLOT(map())) ;
    signalMapper -> setMapping (indentLine, (QObject*)workspace);

    QShortcut *newLineAndIndent = new QShortcut(QKeySequence("Return"), workspace);
    connect (newLineAndIndent, SIGNAL(activated()), retSignalMapper, SLOT(map())) ;
    retSignalMapper -> setMapping (newLineAndIndent, (QObject*)workspace);


    //transpose chars
    QShortcut *transposeChars = new QShortcut(ctrlKey('t'), workspace);
    connect (transposeChars, SIGNAL(activated()), workspace, SLOT(transposeChars())) ;

    //move line or selection up and down
    QShortcut *moveLineUp = new QShortcut(ctrlMetaKey('p'), workspace);
    connect (moveLineUp, SIGNAL(activated()), workspace, SLOT(moveLineOrSelectionUp())) ;

    QShortcut *moveLineDown = new QShortcut(ctrlMetaKey('n'), workspace);
    connect (moveLineDown, SIGNAL(activated()), workspace, SLOT(moveLineOrSelectionDown())) ;

    // Contextual help
    QShortcut *contextHelp = new QShortcut(ctrlKey('i'), workspace);
    connect (contextHelp, SIGNAL(activated()), this, SLOT(helpContext()));

    QShortcut *contextHelp2 = new QShortcut(QKeySequence("F1"), workspace);
    connect (contextHelp2, SIGNAL(activated()), this, SLOT(helpContext()));


    // Font zooming
    QShortcut *fontZoom = new QShortcut(metaKey('='), workspace);
    connect (fontZoom, SIGNAL(activated()), workspace, SLOT(zoomFontIn()));

    QShortcut *fontZoom2 = new QShortcut(metaKey('+'), workspace);
    connect (fontZoom2, SIGNAL(activated()), workspace, SLOT(zoomFontIn()));


    QShortcut *fontZoomOut = new QShortcut(metaKey('-'), workspace);
    connect (fontZoomOut, SIGNAL(activated()), workspace, SLOT(zoomFontOut()));

    QShortcut *fontZoomOut2 = new QShortcut(metaKey('_'), workspace);
    connect (fontZoomOut2, SIGNAL(activated()), workspace, SLOT(zoomFontOut()));

    //set Mark
#ifdef Q_OS_MAC
    QShortcut *setMark = new QShortcut(QKeySequence("Meta+Space"), workspace);
#else
    QShortcut *setMark = new QShortcut(QKeySequence("Ctrl+Space"), workspace);
#endif
    connect (setMark, SIGNAL(activated()), workspace, SLOT(setMark())) ;

    //escape
    QShortcut *escape = new QShortcut(ctrlKey('g'), workspace);
    QShortcut *escape2 = new QShortcut(QKeySequence("Escape"), workspace);
    connect(escape, SIGNAL(activated()), this, SLOT(escapeWorkspaces()));
    connect(escape2, SIGNAL(activated()), this, SLOT(escapeWorkspaces()));


    //quick nav by jumping up and down 10 lines at a time
    QShortcut *forwardTenLines = new QShortcut(shiftMetaKey('u'), workspace);
    connect(forwardTenLines, SIGNAL(activated()), workspace, SLOT(forwardTenLines()));
    QShortcut *backTenLines = new QShortcut(shiftMetaKey('d'), workspace);
    connect(backTenLines, SIGNAL(activated()), workspace, SLOT(backTenLines()));

    //cut to end of line
    QShortcut *cutToEndOfLine = new QShortcut(ctrlKey('k'), workspace);
    connect(cutToEndOfLine, SIGNAL(activated()), workspace, SLOT(cutLineFromPoint()));

    //Emacs live copy and cut
    QShortcut *copyToBuffer = new QShortcut(metaKey(']'), workspace);
    connect(copyToBuffer, SIGNAL(activated()), workspace, SLOT(copyClear()));
    QShortcut *cutToBuffer = new QShortcut(ctrlKey(']'), workspace);
    connect(cutToBuffer, SIGNAL(activated()), workspace, SLOT(cut()));

    QShortcut *pasteToBufferWin = new QShortcut(ctrlKey('v'), workspace);
    connect(pasteToBufferWin, SIGNAL(activated()), workspace, SLOT(paste()));

    //comment line
    QShortcut *toggleLineComment= new QShortcut(metaKey('/'), workspace);
    connect(toggleLineComment, SIGNAL(activated()), this, SLOT(toggleCommentInCurrentWorkspace()));

    //upcase next word
    QShortcut *upcaseWord= new QShortcut(metaKey('u'), workspace);
    connect(upcaseWord, SIGNAL(activated()), workspace, SLOT(upcaseWordOrSelection()));

    //downcase next word
    QShortcut *downcaseWord= new QShortcut(metaKey('l'), workspace);
    connect(downcaseWord, SIGNAL(activated()), workspace, SLOT(downcaseWordOrSelection()));

    QString w = QString(tr("Buffer %1")).arg(QString::number(ws));
    workspaces[ws] = workspace;
    tabs->addTab(workspace, w);
  }

  connect(retSignalMapper, SIGNAL(mapped(QObject*)), this, SLOT(returnAndIndentLine(QObject*)));
  connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(changeTab(int)));
  connect(signalMapper, SIGNAL(mapped(QObject*)), this, SLOT(completeSnippetListOrIndentLine(QObject*)));

  QFont font("Monospace");
  font.setStyleHint(QFont::Monospace);
  lexer->setDefaultFont(font);

  autocomplete = new SonicPiAPIs(lexer);
  // adding universal shortcuts to outputpane seems to
  // steal events from doc system!?
  // addUniversalCopyShortcuts(outputPane);
#if QT_VERSION >= 0x050400
  //requires Qt 5
  new QShortcut(ctrlKey('='), outputPane, SLOT(zoomIn()));
  new QShortcut(ctrlKey('-'), outputPane, SLOT(zoomOut()));
#endif
  addUniversalCopyShortcuts(errorPane);
  outputPane->setReadOnly(true);
  errorPane->setReadOnly(true);
  outputPane->setLineWrapMode(QPlainTextEdit::NoWrap);
#if defined(Q_OS_WIN)
  outputPane->setFontFamily("Courier New");
#elif defined(Q_OS_MAC)
  outputPane->setFontFamily("Menlo");
#else
  outputPane->setFontFamily("Bitstream Vera Sans Mono");
#endif

  if(!theme->font("LogFace").isEmpty()){
      outputPane->setFontFamily(theme->font("LogFace"));
  }

  outputPane->document()->setMaximumBlockCount(1000);
  errorPane->document()->setMaximumBlockCount(1000);

#if QT_VERSION >= 0x050400
  //zoomable QPlainTextEdit requires QT 5.4
  outputPane->zoomIn(1);
#endif
  outputPane->setTextColor(QColor(theme->color("LogInfoForeground")));
  outputPane->appendPlainText("\n");
  //outputPane->append(asciiArtLogo());

  errorPane->zoomIn(1);
  errorPane->setMaximumHeight(130);
  errorPane->setMinimumHeight(130);

  // hudPane = new QTextBrowser;
  // hudPane->setMinimumHeight(130);
  // hudPane->setHtml("<center><img src=\":/images/logo.png\" height=\"113\" width=\"138\"></center>");
  // hudPane->setStyleSheet(defaultTextBrowserStyle);
  // hudWidget = new QDockWidget(this);
  // hudWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
  // hudWidget->setAllowedAreas(Qt::RightDockWidgetArea);
  // hudWidget->setTitleBarWidget(new QWidget());
  // addDockWidget(Qt::RightDockWidgetArea, hudWidget);
  // hudWidget->setWidget(hudPane);
  // hudWidget->setObjectName("hud");

  scopeWidget = new QDockWidget(tr("Scope"),this);
  scopeWidget->setFocusPolicy(Qt::NoFocus);
  scopeWidget->setAllowedAreas(Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
  scopeWidget->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
  scopeInterface = new Scope();
  scopeInterface->pause();
  scopeWidget->setWidget(scopeInterface);
  scopeWidget->setObjectName("scope");
  addDockWidget(Qt::RightDockWidgetArea, scopeWidget);

  prefsWidget = new QDockWidget(tr("Preferences"), this);
  prefsWidget->setFocusPolicy(Qt::NoFocus);
  prefsWidget->setAllowedAreas(Qt::RightDockWidgetArea);
  prefsWidget->setFeatures(QDockWidget::DockWidgetClosable);

  prefsCentral = new QWidget;
  prefsWidget->setWidget(prefsCentral);
  QSizePolicy prefsSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
  prefsCentral->setSizePolicy(prefsSizePolicy);
  addDockWidget(Qt::RightDockWidgetArea, prefsWidget);
  prefsWidget->hide();
  prefsWidget->setObjectName("prefs");

  outputWidget = new QDockWidget(tr("Log"), this);
  outputWidget->setTitleBarWidget(new QWidget());
  outputWidget->setFocusPolicy(Qt::NoFocus);
  outputWidget->setFeatures(QDockWidget::NoDockWidgetFeatures);
  outputWidget->setAllowedAreas(Qt::RightDockWidgetArea);
  outputWidget->setWidget(outputPane);
  addDockWidget(Qt::RightDockWidgetArea, outputWidget);
  outputWidget->setObjectName("output");
  outputTitleWidget = outputWidget->titleBarWidget();

  blankWidget = new QWidget();
  outputWidgetTitle = outputWidget->titleBarWidget();

  docsCentral = new QTabWidget;
  docsCentral->setFocusPolicy(Qt::NoFocus);
  docsCentral->setTabsClosable(false);
  docsCentral->setMovable(false);
  docsCentral->setTabPosition(QTabWidget::South);

  docPane = new QTextBrowser;
  QSizePolicy policy = docPane->sizePolicy();
  policy.setHorizontalStretch(QSizePolicy::Maximum);
  docPane->setSizePolicy(policy);
  docPane->setMinimumHeight(200);
  docPane->setOpenExternalLinks(true);
  docPane->setStyleSheet(defaultTextBrowserStyle);

  QShortcut *up = new QShortcut(ctrlKey('p'), docPane);
  up->setContext(Qt::WidgetShortcut);
  connect(up, SIGNAL(activated()), this, SLOT(docScrollUp()));
  QShortcut *down = new QShortcut(ctrlKey('n'), docPane);
  down->setContext(Qt::WidgetShortcut);
  connect(down, SIGNAL(activated()), this, SLOT(docScrollDown()));

  docPane->setSource(QUrl("qrc:///html/doc.html"));

  addUniversalCopyShortcuts(docPane);

  docsplit = new QSplitter;

  docsplit->addWidget(docsCentral);
  docsplit->addWidget(docPane);

  docWidget = new QDockWidget(tr("Help"), this);
  docWidget->setFocusPolicy(Qt::NoFocus);
  docWidget->setAllowedAreas(Qt::BottomDockWidgetArea);
  docWidget->setWidget(docsplit);
  docWidget->setObjectName("help");

  addDockWidget(Qt::BottomDockWidgetArea, docWidget);
  docWidget->hide();

  statusBar()->hide();

  // Currently causes a segfault when dragging doc pane out of main
  // window:
  // connect(docWidget, SIGNAL(visibilityChanged(bool)), this,
  // SLOT(helpClosed(bool)));

  mainWidgetLayout = new QVBoxLayout;
  mainWidgetLayout->addWidget(tabs);
  mainWidgetLayout->addWidget(errorPane);
  mainWidget = new QWidget;
  mainWidget->setFocusPolicy(Qt::NoFocus);
  errorPane->hide();
  mainWidget->setLayout(mainWidgetLayout);
  setCentralWidget(mainWidget);
}

void MainWindow::escapeWorkspaces() {
  resetErrorPane();

  for (int w=0; w < workspace_max; w++) {
    workspaces[w]->escapeAndCancelSelection();
    workspaces[w]->clearLineMarkers();
  }
}

void MainWindow::changeTab(int id){
  tabs->setCurrentIndex(id);
}

void MainWindow::toggleFullScreenMode() {
  full_screen->toggle();
  updateFullScreenMode();
}

void MainWindow::updateFullScreenMode(){
  if (full_screen->isChecked()) {
    mainWidgetLayout->setMargin(0);
    outputWidget->setTitleBarWidget(blankWidget);
    this->setWindowFlags(Qt::FramelessWindowHint);
    int currentScreen = QApplication::desktop()->screenNumber(this);

#if defined(Q_OS_MAC)
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
#else
    this->setWindowFlags(Qt::FramelessWindowHint);
#endif
    this->setWindowState(Qt::WindowFullScreen);
    this->show();

#if QT_VERSION >= 0x050400
    //requires Qt5
    this->windowHandle()->setScreen(qApp->screens()[currentScreen]);
#endif

    this->setWindowState(Qt::WindowFullScreen);
  }
  else {
    mainWidgetLayout->setMargin(9);
    outputWidget->setTitleBarWidget(outputWidgetTitle);
    this->setWindowState(windowState() & ~(Qt::WindowFullScreen));
#ifdef Q_OS_WIN
    this->setWindowFlags(Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
			 Qt::WindowMinimizeButtonHint |
			 Qt::WindowMaximizeButtonHint |
			 Qt::WindowCloseButtonHint);
#else

#if defined(Q_OS_MAC)
    this->setWindowFlags(Qt::WindowTitleHint | Qt::NoDropShadowWindowHint);
#else
    this->setWindowFlags(Qt::WindowTitleHint);
#endif

#endif
    this->show();
  }
}

void MainWindow::toggleFocusMode() {
  focusMode = !focusMode;
  updateFocusMode();
}

void MainWindow::updateFocusMode(){
  if (focusMode) {
    full_screen->setChecked(true);
    show_tabs->setChecked(false);
    show_buttons->setChecked(false);
    show_log->setChecked(false);
  }
  else {
    full_screen->setChecked(false);
    show_tabs->setChecked(true);
    show_buttons->setChecked(true);
    show_log->setChecked(true);
  }

  updateFullScreenMode();
  updateTabsVisibility();
  updateButtonVisibility();
  updateLogVisibility();
}

void MainWindow::toggleScopePaused() {
  scopeInterface->togglePause();
}

void MainWindow::allJobsCompleted() {
  scopeInterface->pause();
}

void MainWindow::toggleLogVisibility() {
  show_log->toggle();
  updateLogVisibility();
}

void MainWindow::updateLogVisibility(){
  if(show_log->isChecked()) {
    outputWidget->show();
  }
  else{
    outputWidget->close();
  }
}

void MainWindow::toggleTabsVisibility() {
  show_tabs->toggle();
  updateTabsVisibility();
}

void MainWindow::updateTabsVisibility(){
  QTabBar *tabBar = tabs->findChild<QTabBar *>();

  if(show_tabs->isChecked()) {
    tabBar->show();
  }
  else{
    tabBar->hide();
  }
}

void MainWindow::toggleButtonVisibility() {

  show_buttons->toggle();
  updateButtonVisibility();

}

void MainWindow::updateButtonVisibility(){
  if (show_buttons->isChecked()) {
    toolBar->show();
  }
  else {
    toolBar->close();
  }
}

void MainWindow::completeSnippetListOrIndentLine(QObject* ws){
  SonicPiScintilla *spws = ((SonicPiScintilla*)ws);
  if(spws->isListActive()) {
    spws->tabCompleteifList();
  }
  else {
    completeSnippetOrIndentCurrentLineOrSelection(spws);
  }
}


void MainWindow::returnAndIndentLine(QObject* ws){
  SonicPiScintilla *spws = ((SonicPiScintilla*)ws);

  if(spws->isListActive()) {
    spws->tabCompleteifList();
  }
  else {
    if(auto_indent_on_run->isChecked()) {
      newlineAndIndent(spws);
    } else {
      spws->newLine();
    }
  }
}



void MainWindow::newlineAndIndent(SonicPiScintilla* ws) {
  int point_line, point_index, first_line;
  ws->getCursorPosition(&point_line, &point_index);
  first_line = ws->firstVisibleLine();

  std::string code = ws->text().toStdString();

  Message msg("/buffer-newline-and-indent");
  msg.pushStr(guiID.toStdString());
  std::string filename = workspaceFilename(ws);
  msg.pushStr(filename);
  msg.pushStr(code);
  msg.pushInt32(point_line);
  msg.pushInt32(point_index);
  msg.pushInt32(first_line);
  sendOSC(msg);
}

void MainWindow::completeSnippetOrIndentCurrentLineOrSelection(SonicPiScintilla* ws) {
  int start_line, finish_line, point_line, point_index;
  ws->getCursorPosition(&point_line, &point_index);
  if(ws->hasSelectedText()) {
    statusBar()->showMessage(tr("Indenting selection..."), 2000);
    int unused_a, unused_b;
    ws->getSelection(&start_line, &unused_a, &finish_line, &unused_b);
  } else {
    statusBar()->showMessage(tr("Indenting line..."), 2000);
    start_line = point_line;
    finish_line = point_line;
  }


  std::string code = ws->text().toStdString();

  Message msg("/buffer-section-complete-snippet-or-indent-selection");
  msg.pushStr(guiID.toStdString());
  std::string filename = workspaceFilename(ws);
  msg.pushStr(filename);
  msg.pushStr(code);
  msg.pushInt32(start_line);
  msg.pushInt32(finish_line);
  msg.pushInt32(point_line);
  msg.pushInt32(point_index);
  sendOSC(msg);
}

void MainWindow::toggleCommentInCurrentWorkspace() {
  SonicPiScintilla *ws = (SonicPiScintilla*)tabs->currentWidget();
  toggleComment(ws);
}

void MainWindow::toggleComment(SonicPiScintilla* ws) {
  int start_line, finish_line, point_line, point_index;
  ws->getCursorPosition(&point_line, &point_index);
  if(ws->hasSelectedText()) {
    statusBar()->showMessage(tr("Toggle selection comment..."), 2000);
    int unused_a, unused_b;
    ws->getSelection(&start_line, &unused_a, &finish_line, &unused_b);
  } else {
    statusBar()->showMessage(tr("Toggle line comment..."), 2000);
    start_line = point_line;
    finish_line = point_line;
  }


  std::string code = ws->text().toStdString();

  Message msg("/buffer-section-toggle-comment");
  msg.pushStr(guiID.toStdString());
  std::string filename = workspaceFilename(ws);
  msg.pushStr(filename);
  msg.pushStr(code);
  msg.pushInt32(start_line);
  msg.pushInt32(finish_line);
  msg.pushInt32(point_line);
  msg.pushInt32(point_index);
  sendOSC(msg);
}

QString MainWindow::rootPath() {
  // diversity is the spice of life
#if defined(Q_OS_MAC)
  return QCoreApplication::applicationDirPath() + "/../..";
#elif defined(Q_OS_WIN)
  return QCoreApplication::applicationDirPath() + "/../../../..";
#else
  return QCoreApplication::applicationDirPath() + "/../../..";
#endif
}

void MainWindow::startRubyServer(){

  // kill any zombie processes that may exist
  // better: test to see if UDP ports are in use, only kill/sleep if so
  // best: kill SCSynth directly if needed
  std::cout << "[GUI] - shutting down any old SuperCollider servers..." << std::endl;
  Message msg("/exit");
  msg.pushStr(guiID.toStdString());
  sendOSC(msg);
  sleep(2);

  serverProcess = new QProcess();

  QStringList args;
  args << ruby_server_path;


  if(protocol == TCP){
    args << "-t";
  }

  std::cout << "[GUI] - launching Sonic Pi Server" << std::endl;
  if(homeDirWritable) {
    serverProcess->setStandardErrorFile(server_error_log_path);
    serverProcess->setStandardOutputFile(server_output_log_path);
  }
  serverProcess->start(ruby_path, args);
  // Register server pid for potential zombie clearing
  QStringList regServerArgs;
#if QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)
  regServerArgs << QDir::toNativeSeparators(rootPath() + "/app/server/bin/task-register.rb")<< QString::number(serverProcess->processId());
#endif
  QProcess *regServerProcess = new QProcess();
  regServerProcess->start(ruby_path, regServerArgs);
  regServerProcess->waitForFinished();
#if QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)
  std::cout << "[GUI] - Ruby server pid registered: "<< serverProcess->processId() << std::endl;
#endif

  if (!serverProcess->waitForStarted()) {
    invokeStartupError(tr("The Sonic Pi Server could not be started!"));
    return;
  }
}

bool MainWindow::waitForServiceSync() {
  QString contents;
  std::cout << "[GUI] - waiting for Sonic Pi Server to boot..." << std::endl;
  bool server_booted = false;
  if (!homeDirWritable) {
    // we can't monitor the logs so hope for the best!
    sleep(15);
    server_booted = true;
  } else {
    for(int i = 0; i < 60; i ++) {
      contents = readFile(server_output_log_path);
      if (contents.contains("Sonic Pi Server successfully booted.")) {
        std::cout << std::endl << "[GUI] - Sonic Pi Server successfully booted." << std::endl;
        server_booted = true;
        break;
      } else {
        std::cout << ".";
        sleep(1);
      }
    }
  }

  if (!server_booted) {
      std::cout << std::endl << "[GUI] - Critical error! Could not boot Sonic Pi Server." << std::endl;
      invokeStartupError("Critical error! - Could not boot Sonic Pi Server.");
      return false;
  }

  int timeout = 60;
  std::cout << "[GUI] - waiting for Sonic Pi Server to respond..." << std::endl;
  while (sonicPiOSCServer->waitForServer() && timeout-- > 0) {
    sleep(1);
    std::cout << ".";
    if(sonicPiOSCServer->isIncomingPortOpen()) {
      Message msg("/ping");
      msg.pushStr(guiID.toStdString());
      msg.pushStr("QtClient/1/hello");
      sendOSC(msg);
    }
  }
  if (!sonicPiOSCServer->isServerStarted()) {
      std::cout << std::endl <<  "[GUI] - Critical error! Could not connect to Sonic Pi Server." << std::endl;
      invokeStartupError("Critical server error - could not connect to Sonic Pi Server!");
      return false;
  } else {
    std::cout << std::endl << "[GUI] - Sonic Pi Server connection established" << std::endl;
    return true;
  }

}

void MainWindow::splashClose() {
#if defined(Q_OS_MAC)
  splash->close();
#else
  splash->finish(this);
#endif
}

void MainWindow::showWindow() {
  QSettings settings("uk.ac.cam.cl", "Sonic Pi");
  if(settings.value("first_time", 1).toInt() == 1) {
    showMaximized();
  } else {
    showNormal();

  }
  changeShowLineNumbers();
}

void MainWindow::update_mixer_invert_stereo() {
  if (mixer_invert_stereo->isChecked()) {
    mixerInvertStereo();
  } else {
    mixerStandardStereo();
  }
}

void MainWindow::update_mixer_force_mono() {
  if (mixer_force_mono->isChecked()) {
    mixerMonoMode();
  } else {
    mixerStereoMode();
  }
}

void MainWindow::update_check_updates() {
  if (check_updates->isChecked()) {
    enableCheckUpdates();
  } else {
    disableCheckUpdates();
  }
}

void MainWindow::initPrefsWindow() {

  prefTabs = new QTabWidget();
  tabs->setTabsClosable(false);
  tabs->setMovable(false);
  tabs->setTabPosition(QTabWidget::South);

  QGridLayout *grid = new QGridLayout;

  QGroupBox *volBox = new QGroupBox(tr("Raspberry Pi System Volume"));
  volBox->setToolTip(tr("Use this slider to change the system volume of your Raspberry Pi."));

  QGroupBox *advancedAudioBox = new QGroupBox(tr("Advanced Audio"));
  advancedAudioBox->setToolTip(tr("Advanced audio settings for working with\nexternal PA systems when performing with Sonic Pi."));
  mixer_invert_stereo = new QCheckBox(tr("Invert stereo"));
  mixer_invert_stereo->setToolTip(tr("Toggle stereo inversion.\nIf enabled, audio sent to the left speaker will\nbe routed to the right speaker and visa versa."));
  connect(mixer_invert_stereo, SIGNAL(clicked()), this, SLOT(update_mixer_invert_stereo()));
  mixer_force_mono = new QCheckBox(tr("Force mono"));
  mixer_force_mono->setToolTip(tr("Toggle mono mode.\nIf enabled both right and left audio is mixed and\nthe same signal is sent to both speakers.\nUseful when working with external systems that\ncan only handle mono."));
  connect(mixer_force_mono, SIGNAL(clicked()), this, SLOT(update_mixer_force_mono()));

  QVBoxLayout *advanced_audio_box_layout = new QVBoxLayout;
  advanced_audio_box_layout->addWidget(mixer_invert_stereo);
  advanced_audio_box_layout->addWidget(mixer_force_mono);

  // audio_box->addWidget(radio2);
  // audio_box->addWidget(radio3);
  // audio_box->addStretch(1);
  advancedAudioBox->setLayout(advanced_audio_box_layout);


  QGroupBox *audioOutputBox = new QGroupBox(tr("Raspberry Pi Audio Output"));
  audioOutputBox->setToolTip(tr("Your Raspberry Pi has two forms of audio output.\nFirstly, there is the headphone jack of the Raspberry Pi itself.\nSecondly, some HDMI monitors/TVs support audio through the HDMI port.\nUse these buttons to force the output to the one you want."));
  rp_force_audio_default = new QRadioButton(tr("&Default"));
  rp_force_audio_headphones = new QRadioButton(tr("&Headphones"));
  rp_force_audio_hdmi = new QRadioButton(tr("&HDMI"));


  connect(rp_force_audio_default, SIGNAL(clicked()), this, SLOT(setRPSystemAudioAuto()));
  connect(rp_force_audio_headphones, SIGNAL(clicked()), this, SLOT(setRPSystemAudioHeadphones()));
  connect(rp_force_audio_hdmi, SIGNAL(clicked()), this, SLOT(setRPSystemAudioHDMI()));

  QVBoxLayout *audio_box = new QVBoxLayout;
  audio_box->addWidget(rp_force_audio_default);
  audio_box->addWidget(rp_force_audio_headphones);
  audio_box->addWidget(rp_force_audio_hdmi);
  audio_box->addStretch(1);
  audioOutputBox->setLayout(audio_box);

  QHBoxLayout *vol_box = new QHBoxLayout;
  rp_system_vol = new QSlider(this);
  connect(rp_system_vol, SIGNAL(valueChanged(int)), this, SLOT(changeRPSystemVol(int)));
  vol_box->addWidget(rp_system_vol);
  volBox->setLayout(vol_box);

  QGroupBox *debug_box = new QGroupBox(tr("Logging"));
  debug_box->setToolTip(tr("Configure debug behaviour"));

  QGroupBox *synths_box = new QGroupBox(tr("Synths and FX"));
  synths_box->setToolTip(tr("Modify behaviour of synths and FX"));

  print_output = new QCheckBox(tr("Log synths"));
  print_output->setToolTip(tr("Toggle log messages.\nIf disabled, activity such as synth and sample\ntriggering will not be printed to the log by default."));

  clear_output_on_run = new QCheckBox(tr("Clear log on run"));
  clear_output_on_run->setToolTip(tr("Toggle log clearing on run.\nIf enabled, the log is cleared each\ntime the run button is pressed."));

  log_cues = new QCheckBox(tr("Log cues"));
  log_cues->setToolTip(tr("Enable or disable logging of cues.\nIf disabled, cues will still trigger.\nHowever, they will not be visible in the logs."));

  log_auto_scroll = new QCheckBox(tr("Log auto scroll"));
  log_auto_scroll->setToolTip(tr("Toggle log auto scrolling.\nIf enabled the log is scrolled to the bottom after every new message is displayed."));
  connect(log_auto_scroll, SIGNAL(clicked()), this, SLOT(updateLogAutoScroll()));

  check_args = new QCheckBox(tr("Safe mode"));
  check_args->setToolTip(tr("Toggle synth argument checking functions.\nIf disabled, certain synth opt values may\ncreate unexpectedly loud or uncomfortable sounds."));


  enable_external_synths_cb = new QCheckBox(tr("Enable external synths and FX"));
  enable_external_synths_cb->setToolTip(tr("When enabled, Sonic Pi will allow\nsynths and FX loaded via load_synthdefs\nto be triggered.\n\nWhen disabled, Sonic Pi will complain\nwhen you attempt to use a synth or FX\nwhich isn't recognised."));

  synth_trigger_timing_guarantees_cb = new QCheckBox(tr("Enforce timing guarantees"));
  synth_trigger_timing_guarantees_cb->setToolTip(tr("When enabled, Sonic Pi will refuse\nto trigger synths and FX if\nit is too late to do so\n\nWhen disabled, Sonic Pi will always\nattempt to trigger synths and FX\neven when a little late."));

  QVBoxLayout *debug_box_layout = new QVBoxLayout;
  debug_box_layout->addWidget(print_output);
  debug_box_layout->addWidget(log_cues);
  debug_box_layout->addWidget(log_auto_scroll);
  debug_box_layout->addWidget(clear_output_on_run);
  debug_box->setLayout(debug_box_layout);

  QVBoxLayout *synths_box_layout = new QVBoxLayout;
  synths_box_layout->addWidget(check_args);
  synths_box_layout->addWidget(synth_trigger_timing_guarantees_cb);
  synths_box_layout->addWidget(enable_external_synths_cb);


  synths_box->setLayout(synths_box_layout);

  QGroupBox *transparency_box = new QGroupBox(tr("Transparency"));
  QGridLayout *transparency_box_layout = new QGridLayout;
  gui_transparency_slider = new QSlider(this);
  connect(gui_transparency_slider, SIGNAL(valueChanged(int)), this, SLOT(changeGUITransparency(int)));
  transparency_box_layout->addWidget(gui_transparency_slider);
  transparency_box->setLayout(transparency_box_layout);


  QGroupBox *update_box = new QGroupBox(tr("Updates"));
  QSizePolicy updatesPrefSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
  check_updates = new QCheckBox(tr("Check for updates"));
  update_box->setSizePolicy(updatesPrefSizePolicy);
  check_updates->setToolTip(tr("Toggle automatic update checking.\nThis check involves sending anonymous information about your platform and version."));
  check_updates_now = new QPushButton(tr("Check now"));
  check_updates_now->setToolTip(tr("Force a check for updates now.\nThis check involves sending anonymous information about your platform and version."));
  visit_sonic_pi_net = new QPushButton(tr("Get update"));
  visit_sonic_pi_net->setToolTip(tr("Visit http://sonic-pi.net to download new version"));
  visit_sonic_pi_net->setVisible(false);
  check_updates_now->setMaximumWidth(110);
  visit_sonic_pi_net->setMaximumWidth(150);

  QGroupBox *update_info_box = new QGroupBox(tr("Update Info"));
  update_info_box->setMaximumWidth(350);
  QVBoxLayout *update_info_box_layout = new QVBoxLayout;
  update_info_box_layout->addWidget(update_info);
  update_info_box->setLayout(update_info_box_layout);


  connect(check_updates, SIGNAL(clicked()), this, SLOT(update_check_updates()));
  connect(visit_sonic_pi_net, SIGNAL(clicked()), this, SLOT(open_sonic_pi_net()));
  connect(check_updates_now, SIGNAL(clicked()), this, SLOT(check_for_updates_now()));

  QVBoxLayout *update_box_layout = new QVBoxLayout;
  update_box_layout->addWidget(check_updates);

  update_box_layout->addWidget(check_updates_now);
  update_box_layout->addWidget(visit_sonic_pi_net);
  update_box->setLayout(update_box_layout);

  QGroupBox *editor_box = new QGroupBox();
  QGroupBox *editor_display_box = new QGroupBox(tr("Show and Hide"));
  editor_display_box->setToolTip(tr("Configure editor display options."));
  QGroupBox *editor_look_feel_box = new QGroupBox(tr("Look and Feel"));
  editor_look_feel_box->setToolTip(tr("Configure editor look and feel."));
  QGroupBox *automation_box = new QGroupBox(tr("Automation"));
  automation_box->setToolTip(tr("Configure automation features."));
  auto_indent_on_run = new QCheckBox(tr("Auto-align"));
  auto_indent_on_run->setToolTip(tr("Automatically align code on Run"));

  show_line_numbers = new QCheckBox(tr("Show line numbers"));
  show_line_numbers->setToolTip(tr("Toggle line number visibility."));
  show_log = new QCheckBox(tr("Show log"));
  show_log->setToolTip(tooltipStrShiftMeta('L', tr("Toggle visibility of the log.")));
  show_log->setChecked(true);
  show_buttons = new QCheckBox(tr("Show buttons"));
  show_buttons->setToolTip(tooltipStrShiftMeta('B', tr("Toggle visibility of the control buttons.")));
  show_buttons->setChecked(true);
  show_tabs = new QCheckBox(tr("Show tabs"));
  show_tabs->setChecked(true);
  show_tabs->setToolTip(tr("Toggle visibility of the buffer selection tabs."));
  full_screen = new QCheckBox(tr("Full screen"));
  full_screen->setToolTip(tooltipStrShiftMeta('F', tr("Toggle full screen mode.")));

  wrap_mode = new QCheckBox(tr("Word wrap"));
  wrap_mode->setToolTip(tr("Wrap long lines. Useful to avoid lots of scrolling."));
  connect(wrap_mode, SIGNAL(clicked()), this, SLOT(changeWrapMode()));

  dark_mode = new QCheckBox(tr("Dark mode"));
  dark_mode->setToolTip(tooltipStrShiftMeta('M', tr("Toggle dark mode.")) + QString(tr("\nDark mode is perfect for live coding in night clubs.")));
  connect(show_line_numbers, SIGNAL(clicked()), this, SLOT(changeShowLineNumbers()));
  connect(show_log, SIGNAL(clicked()), this, SLOT(updateLogVisibility()));
  connect(show_buttons, SIGNAL(clicked()), this, SLOT(updateButtonVisibility()));
  connect(full_screen, SIGNAL(clicked()), this, SLOT(updateFullScreenMode()));
  connect(show_tabs, SIGNAL(clicked()), this, SLOT(updateTabsVisibility()));
  connect(dark_mode, SIGNAL(clicked()), this, SLOT(updateDarkMode()));

  QVBoxLayout *editor_display_box_layout = new QVBoxLayout;
  QVBoxLayout *editor_box_look_feel_layout = new QVBoxLayout;
  QVBoxLayout *automation_box_layout = new QVBoxLayout;
  QGridLayout *gridEditorPrefs = new QGridLayout;
  editor_display_box_layout->addWidget(show_line_numbers);
  editor_display_box_layout->addWidget(show_log);
  editor_display_box_layout->addWidget(show_buttons);
  editor_display_box_layout->addWidget(show_tabs);
  editor_box_look_feel_layout->addWidget(dark_mode);
  editor_box_look_feel_layout->addWidget(full_screen);
  editor_box_look_feel_layout->addWidget(wrap_mode);
  editor_display_box->setLayout(editor_display_box_layout);
  editor_look_feel_box->setLayout(editor_box_look_feel_layout);

  automation_box_layout->addWidget(auto_indent_on_run);
  automation_box->setLayout(automation_box_layout);

  gridEditorPrefs->addWidget(editor_display_box, 0, 0);
  gridEditorPrefs->addWidget(editor_look_feel_box, 0, 1);
  gridEditorPrefs->addWidget(automation_box, 1, 1);


  editor_box->setLayout(gridEditorPrefs);
  grid->addWidget(prefTabs, 0, 0);

#if defined(Q_OS_LINUX)
  QGroupBox *audio_prefs_box = new QGroupBox();
  QGridLayout *audio_prefs_box_layout = new QGridLayout;

  audio_prefs_box_layout->addWidget(audioOutputBox, 0, 0);
  audio_prefs_box_layout->addWidget(volBox, 0, 1);
  audio_prefs_box->setLayout(audio_prefs_box_layout);
  prefTabs->addTab(audio_prefs_box, tr("Audio"));
#endif

  QGroupBox *studio_prefs_box = new QGroupBox();
  QGridLayout *studio_prefs_box_layout = new QGridLayout;

  studio_prefs_box_layout->addWidget(synths_box, 0, 0);
  studio_prefs_box_layout->addWidget(debug_box, 0, 1);
  studio_prefs_box_layout->addWidget(advancedAudioBox, 1, 1);

  studio_prefs_box->setLayout(studio_prefs_box_layout);

  prefTabs->addTab(editor_box, tr("Editor"));
  prefTabs->addTab(studio_prefs_box, tr("Studio"));

  QGroupBox *performance_box = new QGroupBox();
  performance_box->setToolTip(tr("Settings useful for performing with Sonic Pi"));

  QGroupBox *scope_box = new QGroupBox();
  QGridLayout* scope_box_layout = new QGridLayout();

  scopeSignalMap = new QSignalMapper(this);
  QSettings settings("uk.ac.cam.cl", "Sonic Pi");
  for( auto name : scopeInterface->getScopeNames() )
  {
    QCheckBox* cb = new QCheckBox( tr(name.toLocal8Bit().data()) );
    cb->setChecked( scopeInterface->enableScope( name, settings.value("prefs/scope/show-"+name.toLower(), true).toBool() ) );
    scopeSignalMap->setMapping( cb, cb );
    scope_box_layout->addWidget(cb);
    connect(cb, SIGNAL(clicked()), scopeSignalMap, SLOT(map()));
  }
  connect( scopeSignalMap, SIGNAL(mapped(QWidget*)), this, SLOT(toggleScope(QWidget*)));
  show_scope_axes = new QCheckBox(tr("Show Axes"));
  show_scope_axes->setChecked(true);
  scope_box_layout->addWidget(show_scope_axes);
  connect(show_scope_axes, SIGNAL(clicked()), this, SLOT(toggleScopeAxes()));

  scope_box->setLayout(scope_box_layout);
  prefTabs->addTab(scope_box, tr("Scope"));


#if defined(Q_OS_WIN)
  // do nothing
#elif defined(Q_OS_MAC)
  QGridLayout *performance_box_layout = new QGridLayout;
  performance_box_layout->addWidget(transparency_box, 0, 0);
  performance_box->setLayout(performance_box_layout);
  prefTabs->addTab(performance_box, tr("Performance"));
#else
    // assuming Raspberry Pi
    // do nothing
#endif


  QGroupBox *update_prefs_box = new QGroupBox();
  QGridLayout *update_prefs_box_layout = new QGridLayout;

  update_prefs_box_layout->addWidget(update_info_box, 0, 0);
  update_prefs_box_layout->addWidget(update_box, 0, 1);
  update_prefs_box->setLayout(update_prefs_box_layout);
  prefTabs->addTab(update_prefs_box, tr("Updates"));

  if (!i18n) {
    QGroupBox *translation_box = new QGroupBox("Translation");
    QVBoxLayout *translation_box_layout = new QVBoxLayout;
    QLabel *go_translate = new QLabel;
    go_translate->setOpenExternalLinks(true);
    go_translate->setText(
      "Sonic Pi hasn't been translated to " +
      QLocale::languageToString(QLocale::system().language()) +
      " yet.<br>" +
      "You can help " +
      "<a href=\"https://github.com/samaaron/sonic-pi/blob/master/TRANSLATION.md\">" +
      "translate the Sonic Pi GUI</a> to your language."
    );
    go_translate->setTextFormat(Qt::RichText);
    translation_box_layout->addWidget(go_translate);
    translation_box->setLayout(translation_box_layout);

    grid->addWidget(translation_box, 3, 0, 1, 2);
  }

  prefsCentral->setLayout(grid);

  // Read in preferences from previous session
  check_args->setChecked(settings.value("prefs/check-args", true).toBool());
  print_output->setChecked(settings.value("prefs/print-output", true).toBool());
  clear_output_on_run->setChecked(settings.value("prefs/clear-output-on-run", true).toBool());
  log_cues->setChecked(settings.value("prefs/log-cues", true).toBool());
  log_auto_scroll->setChecked(settings.value("prefs/log-auto-scroll", true).toBool());
  show_line_numbers->setChecked(settings.value("prefs/show-line-numbers", true).toBool());

  enable_external_synths_cb->setChecked(settings.value("prefs/enable-external-synths", false).toBool());
  synth_trigger_timing_guarantees_cb->setChecked(settings.value("prefs/synth-trigger-timing-guarantees", false).toBool());

  wrap_mode->setChecked(settings.value("prefs/wrap-mode", false).toBool());

  dark_mode->setChecked(settings.value("prefs/dark-mode", false).toBool());
  mixer_force_mono->setChecked(settings.value("prefs/mixer-force-mono", false).toBool());
  mixer_invert_stereo->setChecked(settings.value("prefs/mixer-invert-stereo", false).toBool());

  rp_force_audio_default->setChecked(settings.value("prefs/rp/force-audio-default", true).toBool());
  rp_force_audio_headphones->setChecked(settings.value("prefs/rp/force-audio-headphones", false).toBool());
  rp_force_audio_hdmi->setChecked(settings.value("prefs/rp/force-audio-hdmi", false).toBool());

  check_updates->setChecked(settings.value("prefs/rp/check-updates", true).toBool());

  auto_indent_on_run->setChecked(settings.value("prefs/auto-indent-on-run", true).toBool());

  gui_transparency_slider->setValue(settings.value("prefs/gui_transparency", 0).toInt());

  int stored_vol = settings.value("prefs/rp/system-vol", 50).toInt();
  rp_system_vol->setValue(stored_vol);

  //show_left_scope->setChecked( scopeInterface->enableScope( "Left", settings.value("prefs/scope/show-left", true).toBool() ) );
  //show_right_scope->setChecked( scopeInterface->enableScope( "Right", settings.value("prefs/scope/show-right", true).toBool() ) );
  show_scope_axes->setChecked( scopeInterface->setScopeAxes( settings.value("prefs/scope/show-axes", true).toBool() ) );

  // Ensure prefs are honoured on boot
  update_mixer_invert_stereo();
  update_mixer_force_mono();
  changeRPSystemVol(stored_vol);
  update_check_updates();
  updateLogAutoScroll();

  if(settings.value("prefs/rp/force-audio-default", true).toBool()) {
    setRPSystemAudioAuto();
  }
  if(settings.value("prefs/rp/force-audio-headphones", false).toBool()) {
    setRPSystemAudioHeadphones();
  }
  if(settings.value("prefs/rp/force-audio-hdmi", false).toBool()) {
    setRPSystemAudioHDMI();
  }


}

void MainWindow::setMessageBoxStyle() {
  // Set text color to black and background colors to white for the error message display
  QPalette p = QApplication::palette();
  p.setColor(QPalette::WindowText,"#000");
  p.setColor(QPalette::ButtonText,"#000");
  p.setColor(QPalette::Text,"#000");
  p.setColor(QPalette::Base,"#FFF");
  QApplication::setPalette(p);
}

void MainWindow::invokeStartupError(QString msg) {
  if(startup_error_reported) {
    return;
  }

  startup_error_reported = true;
  sonicPiOSCServer->stop();
  QMetaObject::invokeMethod(this, "startupError",
			    Qt::QueuedConnection,
			    Q_ARG(QString, msg));
}

void MainWindow::startupError(QString msg) {
  splashClose();

  setMessageBoxStyle();
  QString gui_log;
  QString scsynth_log;
  QString server_output_log;
  QString server_error_log;
  if(homeDirWritable) {
    gui_log = readFile(gui_log_path);
    scsynth_log = readFile(scsynth_log_path);
    server_output_log = readFile(server_output_log_path);
    server_error_log = readFile(server_error_log_path);
  }
  else {
    gui_log = "Permissions error: unable to access log";
    scsynth_log = "Permissions error: unable to access log";
    server_output_log = "Permissions error: unable to access log";
    server_error_log = "Permissions error: unable to access log";
  }

  QMessageBox *box = new QMessageBox(QMessageBox::Warning,
				     tr("Server boot error..."), tr("Sonic Pi Boot Error\n\nApologies, a critical error occurred during startup") + ":\n\n " + msg + "\n\n" + tr("Please consider reporting a bug at") + "\nhttp://github.com/samaaron/sonic-pi/issues");
  QString error_report = "Sonic Pi Boot Error Report\n==================\n\n\nSystem Information\n----------------\n\n* Sonic Pi version: " + version + "\n* OS: " + osDescription() + "\n\n\nGUI Log\n-------\n\n**`" + gui_log_path + "`**\n```\n" + gui_log + "\n```\n\n\nServer Errors\n-------------\n\n**`" + server_error_log_path + "`**\n```\n" + server_error_log + "\n```\n\n\nServer Output\n-------------\n\n**`" + server_output_log_path + "`**\n```\n" + server_output_log + "\n```\n\n\nScsynth Output\n--------------\n\n**`" + scsynth_log_path + "`**\n```\n" + scsynth_log + "\n```\n";
  box->setDetailedText(error_report);

  QGridLayout* layout = (QGridLayout*)box->layout();
  QSpacerItem* hSpacer = new QSpacerItem(200, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
  layout->addItem(hSpacer, layout->rowCount(), 0, 1, layout->columnCount());
  box->exec();
  std::cout << "[GUI] - Aborting. Sorry about this." << std::endl;
  close();
}

void MainWindow::replaceBuffer(QString id, QString content, int line, int index, int first_line) {
  SonicPiScintilla* ws = filenameToWorkspace(id.toStdString());
  ws->replaceBuffer(content, line, index, first_line);
}

void MainWindow::replaceLines(QString id, QString content, int start_line, int finish_line, int point_line, int point_index) {
  SonicPiScintilla* ws = filenameToWorkspace(id.toStdString());
  ws->replaceLines(start_line, finish_line, content);
  ws->setCursorPosition(point_line, point_index);
}

QString MainWindow::osDescription() {
#if QT_VERSION >= 0x050400
  return QSysInfo::prettyProductName();
#else
  // prettyProductName requires QT 5.4
  //
  return QString("Assuming Linux");
#endif
}

std::string MainWindow::number_name(int i) {
  switch(i) {
  case 0: return "zero";
  case 1: return "one";
  case 2: return "two";
  case 3: return "three";
  case 4: return "four";
  case 5: return "five";
  case 6: return "six";
  case 7: return "seven";
  case 8: return "eight";
  case 9: return "nine";
  default: assert(false); return "";
  }
}

std::string MainWindow::workspaceFilename(SonicPiScintilla* text)
{
  for(int i = 0; i < workspace_max; i++) {
    if(text == workspaces[i]) {
      return "workspace_" + number_name(i);
    }
  }
  return "default";
}

void MainWindow::loadWorkspaces()
{
  std::cout << "[GUI] - loading workspaces" << std::endl;

  for(int i = 0; i < workspace_max; i++) {
    Message msg("/load-buffer");
    msg.pushStr(guiID.toStdString());
    std::string s = "workspace_" + number_name(i);
    msg.pushStr(s);
    sendOSC(msg);
  }
}

void MainWindow::saveWorkspaces()
{
  std::cout << "[GUI] - saving workspaces" << std::endl;

  for(int i = 0; i < workspace_max; i++) {
    std::string code = workspaces[i]->text().toStdString();
    Message msg("/save-buffer");
    msg.pushStr(guiID.toStdString());
    std::string s = "workspace_" + number_name(i);
    msg.pushStr(s);
    msg.pushStr(code);
    sendOSC(msg);
  }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  writeSettings();
  std::cout.rdbuf(coutbuf); // reset to stdout before exiting
  event->accept();
}

QString MainWindow::currentTabLabel()
{
  return tabs->tabText(tabs->currentIndex());
}


bool MainWindow::loadFile()
{
  QString selfilter = QString("%1 (*.rb *.txt)").arg(tr("Buffer files"));
  QSettings settings("uk.ac.cam.cl", "Sonic Pi");
  QString lastDir = settings.value("lastDir", QDir::homePath() + "/Desktop").toString();
  QString fileName = QFileDialog::getOpenFileName(this, tr("Load Sonic Pi Buffer"), lastDir, QString("%1 (*.rb *.txt);;%2 (*.txt);;%3 (*.rb);;%4 (*.*)").arg(tr("Buffer files")).arg(tr("Text files")).arg(tr("Ruby files")).arg(tr("All files")), &selfilter) ;
  if(!fileName.isEmpty()){
    QFileInfo fi=fileName;
    settings.setValue("lastDir", fi.dir().absolutePath());
    SonicPiScintilla* p = (SonicPiScintilla*)tabs->currentWidget();
    loadFile(fileName, p);
    return true;
  } else {
    return false;
  }
}

bool MainWindow::saveAs()
{
  QString selfilter = QString("%1 (*.rb *.txt)").arg(tr("Buffer files"));
  QSettings settings("uk.ac.cam.cl", "Sonic Pi");
  QString lastDir = settings.value("lastDir", QDir::homePath() + "/Desktop").toString();
  QString fileName = QFileDialog::getSaveFileName(this, tr("Save Current Buffer"), lastDir, QString("%1 (*.rb *.txt);;%2 (*.txt);;%3 (*.rb);;%4 (*.*)").arg(tr("Buffer files")).arg(tr("Text files")).arg(tr("Ruby files")).arg(tr("All files")), &selfilter) ;

  if(!fileName.isEmpty()){
    QFileInfo fi=fileName;
    settings.setValue("lastDir", fi.dir().absolutePath());
    if (!fileName.contains(QRegExp("\\.[a-z]+$"))) {
        fileName = fileName + ".txt";
      }
    return saveFile(fileName, (SonicPiScintilla*)tabs->currentWidget());
  } else {
    return false;
  }
}

void MainWindow::sendOSC(Message m)
{
  int TIMEOUT = 30000;
  int PORT_NUM = 4557;

  if(protocol == UDP){
    UdpSocket sock;
    sock.connectTo("localhost", PORT_NUM);
    if (!sock.isOk()) {
        std::cerr << "[GUI] - Error connection to port " << PORT_NUM << ": " << sock.errorMessage() << "\n";
    } else {
        PacketWriter pw;
        pw.addMessage(m);
        sock.sendPacket(pw.packetData(), pw.packetSize());
    }
  }
  else{
    if (clientSock->state() != QAbstractSocket::ConnectedState){
      clientSock->connectToHost("localhost", PORT_NUM,  QIODevice::ReadWrite);
    }

    if(!clientSock->waitForConnected(TIMEOUT)){
      std::cerr <<  "[GUI] - Timeout, could not connect" << "\n";
      clientSock->abort();
      return;
    }

    if(clientSock->state() == QAbstractSocket::ConnectedState){
      PacketWriter pw;
      pw.addMessage(m);

      int bytesWritten = clientSock->write(pw.packetDataForStream(), pw.packetSize()+sizeof(uint32_t));
      clientSock->waitForBytesWritten();

      if (bytesWritten < 0){
        std::cerr <<  "[GUI] - Failed to send bytes" << "\n";
      }

    } else {
      std::cerr << "[GUI] - Client gone away: " << "\n";
    }
  }
}

void MainWindow::resetErrorPane() {
  errorPane->clear();
  errorPane->hide();
}

void MainWindow::runCodeWithFile()
{


  // revert back to using pure OSC
  // if we don't have access to the
  // temporary file store
  if(!tmpFileStoreAvailable) {
    return runCode();
  }

  std::string filename = workspaceFilename( (SonicPiScintilla*)tabs->currentWidget());
  QString tmppath = QDir::toNativeSeparators(tmp_file_store + "/" + QString::fromStdString(filename));

  QFile outFile(tmppath);
  outFile.open(QIODevice::WriteOnly | QIODevice::Text);

  // revert back to using pure OSC
  // if we can't open the temporary file
  if(!outFile.isOpen()){
    std::cout << "[GUI] - unable to open " << tmppath.toStdString() << " to transmit buffer code. Falling back to UDP OSC..." << std::endl;
    return runCode();
  }

  scopeInterface->resume();
  update();
  if(auto_indent_on_run->isChecked()) {
    beautifyCode();
  }
  SonicPiScintilla *ws = (SonicPiScintilla*)tabs->currentWidget();
  ws->highlightAll();
  lexer->highlightAll();
  ws->clearLineMarkers();
  resetErrorPane();
  statusBar()->showMessage(tr("Running Code..."), 1000);
  QString code = ws->text();
  Message msg("/save-and-run-buffer-via-local-file");
  msg.pushStr(guiID.toStdString());

  msg.pushStr(filename);

  if(!print_output->isChecked()) {
    code = "use_debug false #__nosave__ set by Qt GUI user preferences.\n" + code ;
  }

  if(!log_cues->isChecked()) {
    code = "use_cue_logging false #__nosave__ set by Qt GUI user preferences.\n" + code ;
  }

  if(check_args->isChecked()) {
    code = "use_arg_checks true #__nosave__ set by Qt GUI user preferences.\n" + code ;
  }

  if(enable_external_synths_cb->isChecked()) {
     code = "use_external_synths true #__nosave__ set by Qt GUI user preferences.\n" + code ;
  }

  if(synth_trigger_timing_guarantees_cb->isChecked()) {
     code = "use_timing_guarantees true #__nosave__ set by Qt GUI user preferences.\n" + code ;
  }

  if(clear_output_on_run->isChecked()){
    outputPane->clear();
  }


  QTextStream outStream(&outFile);
  outStream << code;
  outFile.close();

  msg.pushStr(tmppath.toStdString());
  msg.pushStr(filename);
  sendOSC(msg);

  QTimer::singleShot(500, this, SLOT(unhighlightCode()));
}


void MainWindow::runCode()
{
  scopeInterface->resume();
  update();
  if(auto_indent_on_run->isChecked()) {
    beautifyCode();
  }
  SonicPiScintilla *ws = (SonicPiScintilla*)tabs->currentWidget();
  ws->highlightAll();
  lexer->highlightAll();
  ws->clearLineMarkers();
  resetErrorPane();
  statusBar()->showMessage(tr("Running Code..."), 1000);
  std::string code = ws->text().toStdString();
  Message msg("/save-and-run-buffer");
  msg.pushStr(guiID.toStdString());
  std::string filename = workspaceFilename( (SonicPiScintilla*)tabs->currentWidget());
  msg.pushStr(filename);

  if(!print_output->isChecked()) {
    code = "use_debug false #__nosave__ set by Qt GUI user preferences.\n" + code ;
  }

  if(!log_cues->isChecked()) {
    code = "use_cue_logging false #__nosave__ set by Qt GUI user preferences.\n" + code ;
  }

  if(check_args->isChecked()) {
    code = "use_arg_checks true #__nosave__ set by Qt GUI user preferences.\n" + code ;
  }

  if(enable_external_synths_cb->isChecked()) {
     code = "use_external_synths true #__nosave__ set by Qt GUI user preferences.\n" + code ;
  }

  if(synth_trigger_timing_guarantees_cb->isChecked()) {
     code = "use_timing_guarantees true #__nosave__ set by Qt GUI user preferences.\n" + code ;
  }

  if(clear_output_on_run->isChecked()){
    outputPane->clear();
  }


  msg.pushStr(code);
  msg.pushStr(filename);
  sendOSC(msg);

  QTimer::singleShot(500, this, SLOT(unhighlightCode()));
}

void MainWindow::unhighlightCode()
{
  SonicPiScintilla *ws = (SonicPiScintilla*)tabs->currentWidget();
  ws->unhighlightAll();

  lexer->unhighlightAll();
}

void MainWindow::zoomCurrentWorkspaceIn()
{
  statusBar()->showMessage(tr("Zooming In..."), 2000);
  SonicPiScintilla* ws = ((SonicPiScintilla*)tabs->currentWidget());
  ws->zoomFontIn();
}

void MainWindow::zoomCurrentWorkspaceOut()
{
  statusBar()->showMessage(tr("Zooming Out..."), 2000);
  SonicPiScintilla* ws = ((SonicPiScintilla*)tabs->currentWidget());
  ws->zoomFontOut();
}

void MainWindow::beautifyCode()
{
  statusBar()->showMessage(tr("Beautifying..."), 2000);
  SonicPiScintilla* ws = ((SonicPiScintilla*)tabs->currentWidget());
  std::string code = ws->text().toStdString();
  int line = 0;
  int index = 0;
  ws->getCursorPosition(&line, &index);
  int first_line = ws->firstVisibleLine();
  Message msg("/buffer-beautify");
  msg.pushStr(guiID.toStdString());
  std::string filename = workspaceFilename( (SonicPiScintilla*)tabs->currentWidget());
  msg.pushStr(filename);
  msg.pushStr(code);
  msg.pushInt32(line);
  msg.pushInt32(index);
  msg.pushInt32(first_line);
  sendOSC(msg);
}

void MainWindow::reloadServerCode()
{
  statusBar()->showMessage(tr("Reloading..."), 2000);
  Message msg("/reload");
  msg.pushStr(guiID.toStdString());
  sendOSC(msg);
}

void MainWindow::check_for_updates_now() {
  statusBar()->showMessage(tr("Checking for updates..."), 2000);
  Message msg("/check-for-updates-now");
  msg.pushStr(guiID.toStdString());
  sendOSC(msg);
}

void MainWindow::enableCheckUpdates()
{
  statusBar()->showMessage(tr("Enabling update checking..."), 2000);
  Message msg("/enable-update-checking");
  msg.pushStr(guiID.toStdString());
  sendOSC(msg);
}

void MainWindow::disableCheckUpdates()
{
  statusBar()->showMessage(tr("Disabling update checking..."), 2000);
  Message msg("/disable-update-checking");
  msg.pushStr(guiID.toStdString());
  sendOSC(msg);
}

void MainWindow::mixerHpfEnable(float freq)
{
  statusBar()->showMessage(tr("Enabling Mixer HPF..."), 2000);
  Message msg("/mixer-hpf-enable");
  msg.pushStr(guiID.toStdString());
  msg.pushFloat(freq);
  sendOSC(msg);
}

void MainWindow::mixerHpfDisable()
{
  statusBar()->showMessage(tr("Disabling Mixer HPF..."), 2000);
  Message msg("/mixer-hpf-disable");
  msg.pushStr(guiID.toStdString());
  sendOSC(msg);
}

void MainWindow::mixerLpfEnable(float freq)
{
  statusBar()->showMessage(tr("Enabling Mixer LPF..."), 2000);
  Message msg("/mixer-lpf-enable");
  msg.pushStr(guiID.toStdString());
  msg.pushFloat(freq);
  sendOSC(msg);
}

void MainWindow::mixerLpfDisable()
{
  statusBar()->showMessage(tr("Disabling Mixer LPF..."), 2000);
  Message msg("/mixer-lpf-disable");
  msg.pushStr(guiID.toStdString());
  sendOSC(msg);
}

void MainWindow::mixerInvertStereo()
{
  statusBar()->showMessage(tr("Enabling Inverted Stereo..."), 2000);
  Message msg("/mixer-invert-stereo");
  msg.pushStr(guiID.toStdString());
  sendOSC(msg);
}

void MainWindow::mixerStandardStereo()
{
  statusBar()->showMessage(tr("Enabling Standard Stereo..."), 2000);
  Message msg("/mixer-standard-stereo");
  msg.pushStr(guiID.toStdString());
  sendOSC(msg);
}

void MainWindow::mixerMonoMode()
{
  statusBar()->showMessage(tr("Mono Mode..."), 2000);
  Message msg("/mixer-mono-mode");
  msg.pushStr(guiID.toStdString());
  sendOSC(msg);
}

void MainWindow::mixerStereoMode()
{
  statusBar()->showMessage(tr("Stereo Mode..."), 2000);
  Message msg("/mixer-stereo-mode");
  msg.pushStr(guiID.toStdString());
  sendOSC(msg);
}

void MainWindow::stopCode()
{
  stopRunningSynths();
  statusBar()->showMessage(tr("Stopping..."), 2000);
}

void MainWindow::scope()
{
  if(scopeWidget->isVisible())
  {
    scopeWidget->hide();
  } else {
    scopeWidget->show();
  }
}

void MainWindow::about()
{
  // todo: this is returning true even after the window disappears
  // Qt::Tool windows get closed automatically when app loses focus
  if(infoWidg->isVisible()) {
    infoWidg->hide();
  } else {
    infoWidg->raise();
    infoWidg->show();
  }
}


void MainWindow::help()
{
  if(docWidget->isVisible()) {
    hidingDocPane = true;
    docWidget->hide();
  } else {
    docWidget->show();
  }
}

void MainWindow::helpContext()
{
  if (!docWidget->isVisible())
    docWidget->show();
  SonicPiScintilla *ws = ((SonicPiScintilla*)tabs->currentWidget());
  QString selection = ws->selectedText();
  if (selection == "") { // get current word instead
    int line, pos;
    ws->getCursorPosition(&line, &pos);
    QString text = ws->text(line);
    selection = ws->wordAtLineIndex(line, pos);
  }
  selection = selection.toLower();
  if (selection[0] == ':')
    selection = selection.mid(1);

  if (helpKeywords.contains(selection)) {
    struct help_entry entry = helpKeywords[selection];
    QListWidget *list = helpLists[entry.pageIndex];

    // force current row to be changed
    // by setting it to a different value to
    // entry.entryIndex and then setting it
    // back. That way it always gets displayed
    // in the GUI :-)
    if (entry.entryIndex == 0) {
      list->setCurrentRow(1);
    } else {
      list->setCurrentRow(0);
    }
    docsCentral->setCurrentIndex(entry.pageIndex);
    list->setCurrentRow(entry.entryIndex);
  }
}



#if defined(Q_OS_MAC)
void MainWindow::changeGUITransparency(int val)
#else
void MainWindow::changeGUITransparency(int)
#endif
{
#if defined(Q_OS_MAC)
  //scale it linearly from 0 -> 100 to 0.3 -> 1
  //setWindowOpacity((0.7 * ((100 - (float)val) / 100.0))  + 0.3);
  // 0 -> 100  to 0 to 255
  int newAlpha = round(255.0 * ((float)val / 100));
  std::cout << "Alpha" << (int)newAlpha;
  outputPane->setAlphaLevel(newAlpha);
  for(int i=0; i < tabs->count(); i++){
    SonicPiScintilla *ws = (SonicPiScintilla *)tabs->widget(i);
    ws->setAlphaLevel(newAlpha);
    ws->redraw();
  }
#else
    // do nothing
#endif


}


#if defined(Q_OS_LINUX)
void MainWindow::changeRPSystemVol(int val)
#else
void MainWindow::changeRPSystemVol(int)
#endif
{
#if defined(Q_OS_WIN)
  // do nothing
#elif defined(Q_OS_MAC)
  // do nothing
#else
  //assuming Raspberry Pi
  QProcess *p = new QProcess();
  float v = (float) val;
  // handle the fact that the amixer percentage range isn't linear
  float vol_float = std::pow(v/100.0, (float)1./3.) * 100.0;
  std::ostringstream ss;
  ss << vol_float;
  statusBar()->showMessage(tr("Updating System Volume..."), 2000);
  QString prog = "amixer cset numid=1 " + QString::fromStdString(ss.str()) + '%';
  p->start(prog);
#endif

}

void MainWindow::toggleScope( QWidget* qw )
{
  QCheckBox* cb = static_cast<QCheckBox*>(qw);
  QSettings settings("uk.ac.cam.cl", "Sonic Pi");
  settings.setValue("prefs/scope/show-"+cb->text().toLower(), cb->isChecked() );
  scopeInterface->enableScope( cb->text(), cb->isChecked() );
}

void MainWindow::toggleLeftScope()
{
  //scopeInterface->enableScope("Left",show_left_scope->isChecked());
}

void MainWindow::toggleRightScope()
{
  //scopeInterface->enableScope("Right",show_right_scope->isChecked());
}

void MainWindow::toggleScopeAxes()
{
  scopeInterface->setScopeAxes(show_scope_axes->isChecked());
}

void MainWindow::toggleDarkMode() {
  dark_mode->toggle();
  updateDarkMode();
}

void MainWindow::updateLogAutoScroll() {
  bool val = log_auto_scroll->isChecked();
  outputPane->forceScrollDown(val);
  if(val) {
    statusBar()->showMessage(tr("Log Auto Scroll on..."), 2000);
    } else {
    statusBar()->showMessage(tr("Log Auto Scroll off..."), 2000);
  }
 }

void MainWindow::updateDarkMode(){
  SonicPiTheme *currentTheme = lexer->theme;



  QString css = readFile(QString(":/theme/%1/doc-styles.css").arg(dark_mode->isChecked() ? "dark" : "light"));
  docPane->document()->setDefaultStyleSheet(css);
  docPane->reload();
  foreach(QTextBrowser* pane, infoPanes) {
    pane->document()->setDefaultStyleSheet(css);
    pane->reload();
  }
  errorPane->document()->setDefaultStyleSheet(css);

  // switch themes
  if(dark_mode->isChecked()){
    currentTheme->darkMode();
  } else {
    currentTheme->lightMode();
  }

  // clear stylesheets
  this->setStyleSheet("");
  infoWidg->setStyleSheet("");
  mainWidget->setStyleSheet("");
  statusBar()->setStyleSheet("");
  outputPane->setStyleSheet("");
  outputWidget->setStyleSheet("");
  prefsWidget->setStyleSheet("");
  tabs->setStyleSheet("");
  prefTabs->setStyleSheet("");
  docsCentral->setStyleSheet("");
  docWidget->setStyleSheet("");
  toolBar->setStyleSheet("");
  scopeWidget->setStyleSheet("");

  docPane->setStyleSheet(defaultTextBrowserStyle);

  QPalette p = QApplication::palette();
  p.setColor(QPalette::WindowText,      currentTheme->color("WindowForeground"));
  p.setColor(QPalette::Window,          currentTheme->color("WindowBackground"));
  p.setColor(QPalette::Base,            currentTheme->color("Base"));
  p.setColor(QPalette::AlternateBase,   currentTheme->color("AlternateBase"));
  p.setColor(QPalette::Text,            currentTheme->color("Foreground"));
  p.setColor(QPalette::HighlightedText, currentTheme->color("HighlightedForeground"));
  p.setColor(QPalette::Highlight,       currentTheme->color("HighlightedBackground"));
  p.setColor(QPalette::ToolTipBase,     currentTheme->color("ToolTipBase"));
  p.setColor(QPalette::ToolTipText,     currentTheme->color("ToolTipText"));
  p.setColor(QPalette::Button,          currentTheme->color("Button"));
  p.setColor(QPalette::ButtonText,      currentTheme->color("ButtonText"));
  p.setColor(QPalette::Shadow,          currentTheme->color("Shadow"));
  p.setColor(QPalette::Light,           currentTheme->color("Light"));
  p.setColor(QPalette::Midlight,        currentTheme->color("Midlight"));
  p.setColor(QPalette::Mid,             currentTheme->color("Mid"));
  p.setColor(QPalette::Dark,            currentTheme->color("Dark"));

  QApplication::setPalette(p);

  QString windowColor = currentTheme->color("WindowBackground").name();
  QString windowForegroundColor = currentTheme->color("WindowForeground").name();
  QString paneColor = currentTheme->color("PaneBackground").name();
  QString logForegroundColor = currentTheme->color("LogForeground").name();
  QString logBackgroundColor = currentTheme->color("LogBackground").name();
  QString windowBorderColor = currentTheme->color("WindowBorder").name();
  QString windowInternalBorderColor = currentTheme->color("WindowInternalBorder").name();

  QString buttonColor = currentTheme->color("Button").name();
  QString buttonBorderColor = currentTheme->color("ButtonBorder").name();
  QString buttonTextColor = currentTheme->color("ButtonText").name();
  QString pressedButtonColor = currentTheme->color("PressedButton").name();
  QString pressedButtonTextColor = currentTheme->color("PressedButtonText").name();

  QString scrollBarColor = currentTheme->color("ScrollBar").name();
  QString scrollBarBackgroundColor = "transparent";

  QString tabColor = currentTheme->color("Tab").name();
  QString tabTextColor = currentTheme->color("TabText").name();
  QString tabSelectedColor = currentTheme->color("TabSelected").name();
  QString tabSelectedTextColor = currentTheme->color("TabSelectedText").name();

  QString toolTipTextColor = currentTheme->color("ToolTipText").name();
  QString toolTipBaseColor = currentTheme->color("ToolTipBase").name();

  QString statusBarColor = currentTheme->color("StatusBar").name();
  QString statusBarTextColor = currentTheme->color("StatusBarText").name();

  QString sliderColor = currentTheme->color("Slider").name();
  QString sliderBackgroundColor = currentTheme->color("SliderBackground").name();

  QString menuColor = currentTheme->color("Menu").name();
  QString menuTextColor = currentTheme->color("MenuText").name();
  QString menuSelectedColor = currentTheme->color("MenuSelected").name();
  QString menuSelectedTextColor = currentTheme->color("MenuSelectedText").name();

  QString selectionForegroundColor = currentTheme->color("SelectionForeground").name();
  QString selectionBackgroundColor = currentTheme->color("SelectionBackground").name();
  QString errorBackgroundColor = currentTheme->color("ErrorBackground").name();


  QString buttonStyling = QString(
    "QPushButton{"
    "  padding: 5px;"
    "  background-color: %1;"
    "  border-radius: 3px;"
    "  border-color: %2;"
    "  border-width: 2px;"
    "}"
    ""
    "QPushButton::pressed{"
    "  background-color: %3;"
    "  color: %4;"
    "}").arg( buttonColor, buttonBorderColor, pressedButtonColor, pressedButtonTextColor );


  QString splitterStyling = QString(
    "QSplitter::handle:vertical{"
    "  height: 6px;"
    "  image: url(images/vsplitter.png);"
    "}"
    ""
    "QSplitter::handle:horizontal {"
    "  width: 6px;"
    "  image: url(images/hsplitter.png);"
    "}");


  QString scrollStyling = QString(
    "QScrollBar:horizontal, QScrollBar:vertical{"
    "  background-color: %1;"
    "}"
    ""
    "QScrollBar::handle:horizontal {"
    "  background: %2;"
    "  border: 2px solid %3;"
    "  min-width: 80%;"
    "}"
    ""
    "QScrollBar::handle:vertical {"
    "  background: %2;"
    "  border: 2px solid %3;"
    "  min-width: 80%;"
    "}"
    ""
    "QScrollBar::add-page:horizontal,"
    "QScrollBar::sub-page:horizontal,"
    "QScrollBar::add-page:vertical,"
    "QScrollBar::sub-page:vertical{"
    "  background: none;"
    "}").arg(scrollBarBackgroundColor, scrollBarColor, paneColor);


  QString tabStyling = QString(
    "QTabBar::tab{"
    "  background: %1;"
    "  color: %2;"
    "}"
    ""
    "QTabBar::tab:selected{"
    "  background: %3;"
    "  color: %4"
    "}"
    ""
    "QTabWidget::tab-bar{"
    "  alignment: center;"
    "}"
    ""
    "QTabWidget::pane{"
    "  border: 0px;"
    "}").arg(tabColor, tabTextColor, tabSelectedColor, tabSelectedTextColor);


  QString widgetTitleStyling = QString(
    "QDockWidget::title{"
    "  color: %3;"
    "  border-bottom: 1px solid %2;"
    "  text-align: center;"
    "  background: %1;"
    "  font-size 10px;"
    "}"
    ""
    "QDockWidget{"
    "  font-size:10px;"
    "}").arg(windowColor, windowBorderColor, windowForegroundColor);


  QString toolTipStyling = QString(
    "QToolTip {"
    "  color: %1;"
    "  background-color: %2;"
    "  border: 0px;"
    "}").arg(toolTipTextColor, toolTipBaseColor);


  QString toolButtonStyling = QString(
    "QToolButton:hover{"
    "  background: transparent;"
    "}");


  QString sliderStyling = QString(
    "QSlider::groove:vertical{"
    "  margin: 2px 0;"
    "  background: %2;"
    "  border-radius: 3px;"
    "}"
    ""
    "QSlider::handle:vertical {"
    "  border: 1px solid %1;"
    "  border-radius: 3px;"
    "  height: 30px;"
    "  background-color: %1;"
    "}").arg(sliderColor, sliderBackgroundColor);


  QString menuStyling = QString(
    "QMenu{"
    "  background: %1;"
    "  color: %2;"
    "}"
    ""
    "QMenu:selected{"
    "  background: %3;"
    "  color: %4"
    "}").arg(menuColor, menuTextColor, menuSelectedColor, menuSelectedTextColor);


  QString windowStyling = QString(
    "QMainWindow::separator{"
    "  border: 1px solid %1;"
    "}"
    ""
    "QMainWindow{"
    "  background-color: %1;"
    "  color: %2;"
    "}").arg(windowBorderColor, windowColor);


  QString frameStyling = QString(
    "QFrame {"
    "  border: 2px solid %1;"
    "}").arg(paneColor);


  QString qwtplotStyling = QString(
    "QwtPlot{"
    "  background-color: %1;"
    "}").arg(paneColor);

  this->setStyleSheet(buttonStyling +
                      splitterStyling +
                      toolTipStyling +
                      scrollStyling +
                      toolButtonStyling +
                      sliderStyling +
                      menuStyling +
                      windowStyling +
                      widgetTitleStyling);


  statusBar()->setStyleSheet( QString(
    "QStatusBar{"
    "  background-color: %1;"
    "  color: %2;"
    "}").arg(statusBarColor, statusBarTextColor));


  // Colour log messages as info by default
  outputPane->setStyleSheet( QString(
    "QPlainTextEdit{"
    "  background-color: %1;"
    "  color: %2;"
    "  border: 0px;"
    "}").arg(logBackgroundColor, logForegroundColor));

  outputWidget->setStyleSheet(widgetTitleStyling);


  prefsWidget->setStyleSheet( QString(widgetTitleStyling + buttonStyling + QString(
    "QGroupBox:title{"
    "  subcontrol-origin: margin;"
    "  top:0px;"
    "  padding: 0px 0 20px 5px;"
    "  font-size: 11px;"
    "  color: %1;"
    "  background-color: transparent;"
    "}"
    ""
    "QGroupBox{"
    "  padding: 0 0 0 0;"
    "  subcontrol-origin: margin;"
    "  margin-top: 15px;"
    "  margin-bottom: 0px;"
    "  font-size: 11px;"
    "  background-color: %2;"
    "  border: 1px solid %3;"
    "  color: %1;"
    "}"
    ""
    "QWidget{"
    "  background-color: %2;"
    "}").arg(windowForegroundColor, windowColor, windowInternalBorderColor)));

  tabs->setStyleSheet(tabStyling);
  prefTabs->setStyleSheet(tabStyling);
  docsCentral->setStyleSheet(tabStyling);

  docWidget->setStyleSheet( QString(widgetTitleStyling + QString(
    "QDockWidget QListView {"
    "  color: %2;"
    "  background: %1;"
    "  selection-color: %3;"
    "  selection-background-color: %4;"
    "}").arg(paneColor, windowForegroundColor, selectionForegroundColor, selectionBackgroundColor)));


  docPane->setStyleSheet( QString(
    "QTextBrowser {"
    "  selection-color: %2;"
    "  selection-background-color: %3;"
    "  padding-left: 10;"
    "  padding-top: 10;"
    "  padding-bottom: 10;"
    "  padding-right: 10;"
    "  background: %1"
    "}").arg(paneColor, selectionForegroundColor, selectionBackgroundColor));


  infoWidg->setStyleSheet( QString(scrollStyling + tabStyling + QString(
    "QTextEdit{"
    "  background-color: %1;"
    "}").arg(paneColor)));


  toolBar->setStyleSheet( QString(
    "QToolBar{"
    "  background-color: %1;"
    "  border-bottom: 1px solid %2;"
    "}").arg(windowColor,windowBorderColor));


  errorPane->setStyleSheet( QString(
    "QTextEdit{"
    "  background-color: %1;"
    "}"
    ""
    ".error-background{"
    "  background-color: %2"
    "}").arg(paneColor, errorBackgroundColor));

  scopeWidget->setStyleSheet( QString(frameStyling + qwtplotStyling));



  for(int i=0; i < tabs->count(); i++){
    SonicPiScintilla *ws = (SonicPiScintilla *)tabs->widget(i);
    ws->setFrameShape(QFrame::NoFrame);
  }

  foreach(QTextBrowser* pane, infoPanes) {
    pane->setStyleSheet(QString(scrollStyling +
      "QTextBrowser{"
      "  padding-left: 10;"
      "  padding-top: 10;"
      "  padding-bottom: 10;"
      "  padding-right: 10;"
      "}"));
  }


  for(int i=0; i < tabs->count(); i++){
    SonicPiScintilla *ws = (SonicPiScintilla *)tabs->widget(i);
    ws->redraw();
  }

  lexer->unhighlightAll();
}

void MainWindow::changeShowLineNumbers(){
  for(int i=0; i < tabs->count(); i++){
    SonicPiScintilla *ws = (SonicPiScintilla *)tabs->widget(i);
    if (show_line_numbers->isChecked()){

      ws->showLineNumbers();
    } else {
      ws->hideLineNumbers();
    }
  }
}

void MainWindow::changeWrapMode(){
    for(int i=0; i < tabs->count(); i++){
      SonicPiScintilla *ws = (SonicPiScintilla *)tabs->widget(i);
      if (wrap_mode->isChecked()){

        ws->wordWrapOn();
      } else {
        ws->wordWrapOff();
      }
    }
}

void MainWindow::setRPSystemAudioHeadphones()
{
#if defined(Q_OS_WIN)
  //do nothing
#elif defined(Q_OS_MAC)
  //do nothing
#else
  //assuming Raspberry Pi
  statusBar()->showMessage(tr("Switching To Headphone Audio Output..."), 2000);
  QProcess *p = new QProcess();
  QString prog = "amixer cset numid=3 1";
  p->start(prog);
#endif
}

void MainWindow::setRPSystemAudioHDMI()
{

#if defined(Q_OS_WIN)
  //do nothing
#elif defined(Q_OS_MAC)
  //do nothing
#else
  //assuming Raspberry Pi
  statusBar()->showMessage(tr("Switching To HDMI Audio Output..."), 2000);
  QProcess *p = new QProcess();
  QString prog = "amixer cset numid=3 2";
  p->start(prog);
#endif
}

void MainWindow::setRPSystemAudioAuto()
{
#if defined(Q_OS_WIN)
  //do nothing
#elif defined(Q_OS_MAC)
  //do nothing
#else
  //assuming Raspberry Pi
  statusBar()->showMessage(tr("Switching To Default Audio Output..."), 2000);
  QProcess *p = new QProcess();
  QString prog = "amixer cset numid=3 0";
  p->start(prog);
#endif
}

void MainWindow::showPrefsPane()
{
  if(prefsWidget->isVisible()) {
    prefsWidget->hide();
  } else {
    prefsWidget->show();
  }
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
#if defined(Q_OS_WIN)
  if (event->modifiers() & Qt::ControlModifier) {
    SonicPiScintilla* ws = ((SonicPiScintilla*)tabs->currentWidget());
    if (event->angleDelta().y() > 0)
      ws->zoomFontIn();
    else
      ws->zoomFontOut();
  }
#else
  (void)event;
#endif
}

void MainWindow::stopRunningSynths()
{
  Message msg("/stop-all-jobs");
  msg.pushStr(guiID.toStdString());
  sendOSC(msg);
}

void MainWindow::clearOutputPanels()
{
  outputPane->clear();
  errorPane->clear();
}

QKeySequence MainWindow::ctrlKey(char key)
{
#ifdef Q_OS_MAC
  return QKeySequence(QString("Meta+%1").arg(key));
#else
  return QKeySequence(QString("Ctrl+%1").arg(key));
#endif
}

// Cmd on Mac, Alt everywhere else
QKeySequence MainWindow::metaKey(char key)
{
#ifdef Q_OS_MAC
  return QKeySequence(QString("Ctrl+%1").arg(key));
#else
  return QKeySequence(QString("alt+%1").arg(key));
#endif
}

Qt::Modifier MainWindow::metaKeyModifier()
{
#ifdef Q_OS_MAC
  return Qt::CTRL;
#else
  return Qt::ALT;
#endif
}

QKeySequence MainWindow::shiftMetaKey(char key)
{
#ifdef Q_OS_MAC
  return QKeySequence(QString("Shift+Ctrl+%1").arg(key));
#else
  return QKeySequence(QString("Shift+alt+%1").arg(key));
#endif
}

QKeySequence MainWindow::ctrlMetaKey(char key)
{
#ifdef Q_OS_MAC
  return QKeySequence(QString("Ctrl+Meta+%1").arg(key));
#else
  return QKeySequence(QString("Ctrl+alt+%1").arg(key));
#endif
}

char MainWindow::int2char(int i){
  return '0' + i;
}

 QString MainWindow::tooltipStrShiftMeta(char key, QString str) {
#ifdef Q_OS_MAC
    return QString("%1 (⇧⌘%2)").arg(str).arg(key);
#else
    return QString("%1 (Shift-alt-%2)").arg(str).arg(key);
#endif
}

 QString MainWindow::tooltipStrMeta(char key, QString str) {
#ifdef Q_OS_MAC
    return QString("%1 (⌘%2)").arg(str).arg(key);
#else
    return QString("%1 (alt-%2)").arg(str).arg(key);
#endif
}


// set tooltips, connect event handlers, and add shortcut if applicable
void MainWindow::setupAction(QAction *action, char key, QString tooltip,
			     const char *slot)
{
  QString shortcut, tooltipKey;
  tooltipKey = tooltip;
  if (key != 0) {
    tooltipKey = tooltipStrMeta(key, tooltip);
  }

  action->setToolTip(tooltipKey);
  action->setStatusTip(tooltip);
  connect(action, SIGNAL(triggered()), this, slot);

  if (key != 0) {
    // create a QShortcut instead of setting the QAction's shortcut
    // so it will still be active with the toolbar hidden
    new QShortcut(metaKey(key), this, slot);
  }
}

void MainWindow::createShortcuts()
{
  new QShortcut(metaKey('{'), this, SLOT(tabPrev()));
  new QShortcut(metaKey('}'), this, SLOT(tabNext()));
  new QShortcut(QKeySequence("F8"), this, SLOT(reloadServerCode()));

  new QShortcut(QKeySequence("F9"), this, SLOT(toggleButtonVisibility()));
  new QShortcut(shiftMetaKey('B'), this, SLOT(toggleButtonVisibility()));
  new QShortcut(QKeySequence("F10"), this, SLOT(toggleFocusMode()));
  new QShortcut(shiftMetaKey('F'), this, SLOT(toggleFullScreenMode()));
  new QShortcut(shiftMetaKey('M'), this, SLOT(toggleDarkMode()));
  new QShortcut(QKeySequence("F11"), this, SLOT(toggleLogVisibility()));
  new QShortcut(shiftMetaKey('L'), this, SLOT(toggleLogVisibility()));
  new QShortcut(QKeySequence("F12"),this, SLOT(toggleScopePaused()));
}

void MainWindow::createToolBar()
{
  // Run
  QAction *runAct = new QAction(QIcon(":/images/run.png"), tr("Run"), this);
  setupAction(runAct, 'R', tr("Run the code in the current buffer"),
	      SLOT(runCodeWithFile()));
  new QShortcut(QKeySequence(metaKeyModifier() + Qt::Key_Return), this, SLOT(runCodeWithFile()));

  // Stop
  QAction *stopAct = new QAction(QIcon(":/images/stop.png"), tr("Stop"), this);
  setupAction(stopAct, 'S', tr("Stop all running code"), SLOT(stopCode()));

  // Save
  QAction *saveAsAct = new QAction(QIcon(":/images/save.png"), tr("Save As..."), this);
  setupAction(saveAsAct, 0, tr("Save current buffer as an external file"), SLOT(saveAs()));

  // Load
  QAction *loadFileAct = new QAction(QIcon(":/images/load.png"), tr("Load"), this);
  setupAction(loadFileAct, 0, tr("Load an external file in the current buffer"), SLOT(loadFile()));

  // Record
  recAct = new QAction(QIcon(":/images/rec.png"), tr("Start Recording"), this);
  setupAction(recAct, 0, tr("Start recording to WAV audio file"), SLOT(toggleRecording()));

  // Align
  QAction *textAlignAct = new QAction(QIcon(":/images/align.png"),
			     tr("Auto-Align Text"), this);
  setupAction(textAlignAct, 'M', tr("Improve readability of code"), SLOT(beautifyCode()));

  // Font Size Increase
  QAction *textIncAct = new QAction(QIcon(":/images/size_up.png"),
			    tr(""), this);
  setupAction(textIncAct, 0, tr(""), SLOT(zoomCurrentWorkspaceIn()));
  textIncAct->setToolTip(tooltipStrMeta('+', tr("Increase Text Size")));

  // Font Size Decrease
  QAction *textDecAct = new QAction(QIcon(":/images/size_down.png"),
			    tr(""), this);

  // Scope
  QAction *scopeAct = new QAction(QIcon(":/images/scope.png"), tr("Scope"), this);
  setupAction(scopeAct, 0, tr("Toggle the visibilty of the audio oscilloscope. "), SLOT(scope()));

    // Info
  QAction *infoAct = new QAction(QIcon(":/images/info.png"), tr("Info"), this);
  setupAction(infoAct, 0, tr("See information about Sonic Pi"),
	      SLOT(about()));


  // Help
  QAction *helpAct = new QAction(QIcon(":/images/help.png"), tr("Help"), this);
  setupAction(helpAct, 'I', tr("Toggle the visibility of the help pane"), SLOT(help()));

  // Preferences
  QAction *prefsAct = new QAction(QIcon(":/images/prefs.png"), tr("Prefs"), this);
  setupAction(prefsAct, 'P', tr("Toggle the visibility of the preferences pane"),
	      SLOT(showPrefsPane()));


  setupAction(textDecAct, 0, tr(""), SLOT(zoomCurrentWorkspaceOut()));
  textDecAct->setToolTip(tooltipStrMeta('-', tr("Decrease Text Size")));

  QWidget *spacer = new QWidget();
  spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

  toolBar = addToolBar(tr("Tools"));
  toolBar->setObjectName("toolbar");
  toolBar->setIconSize(QSize(270/3, 111/3));
  toolBar->addAction(runAct);
  toolBar->addAction(stopAct);
  toolBar->addAction(recAct);

  toolBar->addAction(saveAsAct);
  toolBar->addAction(loadFileAct);

  toolBar->addWidget(spacer);

  toolBar->addAction(textDecAct);
  toolBar->addAction(textIncAct);

  dynamic_cast<QToolButton*>(toolBar->widgetForAction(textDecAct))->setAutoRepeat(true);
  dynamic_cast<QToolButton*>(toolBar->widgetForAction(textIncAct))->setAutoRepeat(true);

  //  toolBar->addAction(textAlignAct);
  toolBar->addAction(scopeAct);
  toolBar->addAction(infoAct);
  toolBar->addAction(helpAct);
  toolBar->addAction(prefsAct);
}

QString MainWindow::readFile(QString name)
{
  QFile file(name);
  if (!file.open(QFile::ReadOnly | QFile::Text)) {
    std::cerr << "[GUI] - could not open file " << name.toStdString() << "\n";
    return "";
  }

  QTextStream st(&file);
  st.setCodec("UTF-8");
  return st.readAll();
}

void MainWindow::createInfoPane() {
  QTabWidget* infoTabs = new QTabWidget(this);

  QStringList urls, tabs;

  urls << "qrc:///html/info.html"
       << "qrc:///info/CORETEAM.html"
       << "qrc:///info/CONTRIBUTORS.html"
       << "qrc:///info/COMMUNITY.html"
       << "qrc:///info/LICENSE.html"
       << "qrc:///info/CHANGELOG.html";

  tabs << tr("About")
       << tr("Core Team")
       << tr("Contributors")
       << tr("Community")
       << tr("License")
       << tr("History");

  for (int t=0; t < urls.size(); t++) {
    QTextBrowser *pane = new QTextBrowser;
    infoPanes.append(pane);
    addUniversalCopyShortcuts(pane);
    pane->setOpenExternalLinks(true);
    pane->setFixedSize(600, 615);
    pane->setSource(QUrl(urls[t]));
    infoTabs->addTab(pane, tabs[t]);
  }

  infoTabs->setTabPosition(QTabWidget::South);

  QBoxLayout *infoLayout = new QBoxLayout(QBoxLayout::LeftToRight);
  infoLayout->addWidget(infoTabs);

  infoWidg = new QWidget;
  infoWidg->setWindowIcon(QIcon(":images/icon-smaller.png"));
  infoWidg->setLayout(infoLayout);
  infoWidg->setWindowFlags(Qt::Tool | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint);
  infoWidg->setWindowTitle(tr("Sonic Pi - Info"));

  QAction *closeInfoAct = new QAction(this);
  closeInfoAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_W));
  connect(closeInfoAct, SIGNAL(triggered()), this, SLOT(about()));
  infoWidg->addAction(closeInfoAct);
}

void MainWindow::toggleRecordingOnIcon() {
  show_rec_icon_a = !show_rec_icon_a;
  if(show_rec_icon_a) {
    recAct->setIcon(QIcon(":/images/recording_a.png"));
  } else {
    recAct->setIcon(QIcon(":/images/recording_b.png"));
  }
}

void MainWindow::toggleRecording() {
  is_recording = !is_recording;
  if(is_recording) {
    recAct->setStatusTip(tr("Stop Recording"));
    recAct->setToolTip(tr("Stop Recording"));
    rec_flash_timer->start(500);
    Message msg("/start-recording");
    msg.pushStr(guiID.toStdString());
    sendOSC(msg);
  } else {
    rec_flash_timer->stop();
    recAct->setStatusTip(tr("Start Recording"));
    recAct->setToolTip(tr("Start Recording"));
    recAct->setIcon(QIcon(":/images/rec.png"));
    Message msg("/stop-recording");
    msg.pushStr(guiID.toStdString());
    sendOSC(msg);
    QSettings settings("uk.ac.cam.cl", "Sonic Pi");
    QString lastDir = settings.value("lastDir", QDir::homePath() + "/Desktop").toString();
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Recording"), lastDir, tr("Wavefile (*.wav)"));
    if (!fileName.isEmpty()) {
      QFileInfo fi=fileName;
      settings.setValue("lastDir", fi.dir().absolutePath());
      Message msg("/save-recording");
      msg.pushStr(guiID.toStdString());
      msg.pushStr(fileName.toStdString());
      sendOSC(msg);
    } else {
      Message msg("/delete-recording");
      msg.pushStr(guiID.toStdString());
      sendOSC(msg);
    }
  }
}


void MainWindow::createStatusBar()
{
  versionLabel = new QLabel(this);
  versionLabel->setText("Sonic Pi");
  statusBar()->showMessage(tr("Ready..."));
  statusBar()->addPermanentWidget(versionLabel);
}

void MainWindow::readSettings() {
  // Pref settings are read in MainWindow::initPrefsWindow()
  QSettings settings("uk.ac.cam.cl", "Sonic Pi");
  QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
  QSize size = settings.value("size", QSize(400, 400)).toSize();
  resize(size);
  move(pos);

  int index = settings.value("workspace", 0).toInt();
  if (index < tabs->count())
    tabs->setCurrentIndex(index);

  for (int w=0; w < workspace_max; w++) {
    // default zoom is 13
    int zoom = settings.value(QString("workspace%1zoom").arg(w), 13)
      .toInt();
    if (zoom < -5) zoom = -5;
    if (zoom > 20) zoom = 20;

    workspaces[w]->setProperty("zoom", QVariant(zoom));
    workspaces[w]->zoomTo(zoom);
  }

  docsplit->restoreState(settings.value("docsplitState").toByteArray());

  restoreState(settings.value("windowState").toByteArray());
  restoreGeometry(settings.value("windowGeom").toByteArray());

}

void MainWindow::writeSettings()
{
  QSettings settings("uk.ac.cam.cl", "Sonic Pi");
  settings.setValue("pos", pos());
  settings.setValue("size", size());
  settings.setValue("first_time", 0);


  settings.setValue("prefs/check-args", check_args->isChecked());
  settings.setValue("prefs/print-output", print_output->isChecked());
  settings.setValue("prefs/clear-output-on-run", clear_output_on_run->isChecked());
  settings.setValue("prefs/log-cues", log_cues->isChecked());
  settings.setValue("prefs/log-auto-scroll", log_auto_scroll->isChecked());
  settings.setValue("prefs/show-line-numbers", show_line_numbers->isChecked());
  settings.setValue("prefs/enable-external-synths", enable_external_synths_cb->isChecked());
  settings.setValue("prefs/synth-trigger-timing-guarantees", synth_trigger_timing_guarantees_cb->isChecked());
  settings.setValue("prefs/dark-mode", dark_mode->isChecked());
  settings.setValue("prefs/wrap-mode", wrap_mode->isChecked());
  settings.setValue("prefs/mixer-force-mono", mixer_force_mono->isChecked());
  settings.setValue("prefs/mixer-invert-stereo", mixer_invert_stereo->isChecked());
  settings.setValue("prefs/", mixer_invert_stereo->isChecked());

  settings.setValue("prefs/rp/force-audio-default", rp_force_audio_default->isChecked());
  settings.setValue("prefs/rp/force-audio-headphones", rp_force_audio_headphones->isChecked());
  settings.setValue("prefs/rp/force-audio-hdmi", rp_force_audio_hdmi->isChecked());
  settings.setValue("prefs/rp/system-vol", rp_system_vol->value());

  settings.setValue("prefs/rp/check-updates", check_updates->isChecked());
  settings.setValue("prefs/auto-indent-on-run", auto_indent_on_run->isChecked());
  settings.setValue("prefs/gui_transparency", gui_transparency_slider->value());
  settings.setValue("workspace", tabs->currentIndex());

  for (int w=0; w < workspace_max; w++) {
    settings.setValue(QString("workspace%1zoom").arg(w),
		      workspaces[w]->property("zoom"));
  }

  settings.setValue("docsplitState", docsplit->saveState());
  settings.setValue("windowState", saveState());
  settings.setValue("windowGeom", saveGeometry());

  //settings.setValue("prefs/scope/show-left", show_left_scope->isChecked() );
  //settings.setValue("prefs/scope/show-right", show_right_scope->isChecked() );
  settings.setValue("prefs/scope/show-axes", show_scope_axes->isChecked() );
}

void MainWindow::loadFile(const QString &fileName, SonicPiScintilla* &text)
{
  QFile file(fileName);
  if (!file.open(QFile::ReadOnly)) {
    setMessageBoxStyle();
    QMessageBox::warning(this, tr("Sonic Pi"),
			 tr("Cannot read file %1:\n%2.")
			 .arg(fileName)
			 .arg(file.errorString()));
    updateDarkMode();
    return;
  }

  QTextStream in(&file);
  QApplication::setOverrideCursor(Qt::WaitCursor);
  text->setText(in.readAll());
  QApplication::restoreOverrideCursor();
  statusBar()->showMessage(tr("File loaded..."), 2000);
}

bool MainWindow::saveFile(const QString &fileName, SonicPiScintilla* text)
{
  QFile file(fileName);
  if (!file.open(QFile::WriteOnly)) {
    setMessageBoxStyle();
    QMessageBox::warning(this, tr("Sonic Pi"),
			 tr("Cannot write file %1:\n%2.")
			 .arg(fileName)
			 .arg(file.errorString()));
    updateDarkMode();
    return false;
  }

  QTextStream out(&file);

  QApplication::setOverrideCursor(Qt::WaitCursor);
  QString code = text->text();
#if defined(Q_OS_WIN)
  code.replace("\n", "\r\n"); // CRLF for Windows users
  code.replace("\r\r\n", "\r\n"); // don't double-replace if already encoded
#endif
  out << code;
  QApplication::restoreOverrideCursor();

  statusBar()->showMessage(tr("File saved..."), 2000);
  return true;
}

SonicPiScintilla* MainWindow::filenameToWorkspace(std::string filename)
{
  std::string s;

  for(int i = 0; i < workspace_max; i++) {
    s = "workspace_" + number_name(i);
    if(filename == s) {
      return workspaces[i];
    }
  }
  return workspaces[0];
}

void MainWindow::onExitCleanup()
{

  setupLogPathAndRedirectStdOut();
  std::cout << "[GUI] - stopping OSC server" << std::endl;
  sonicPiOSCServer->stop();
  if(protocol == TCP){
    clientSock->close();
  }
  if(serverProcess->state() == QProcess::NotRunning) {
    std::cout << "[GUI] - warning, server process is not running." << std::endl;
  } else {
    if (loaded_workspaces) {
      // this should be a synchorous call to avoid the following sleep
      saveWorkspaces();
    }
    sleep(1);
    std::cout << "[GUI] - asking server process to exit..." << std::endl;
    Message msg("/exit");
    msg.pushStr(guiID.toStdString());
    sendOSC(msg);
  }
  if(protocol == UDP){
    osc_thread.waitForFinished();
  }
  sleep(2);

  // ensure all child processes are nuked if they didn't die gracefully
  std::cout << "[GUI] - executing exit script" << std::endl;
  QProcess* exitProcess = new QProcess();
  exitProcess->start(ruby_path, QStringList(exit_script_path));
  exitProcess->waitForFinished();

  std::cout << "[GUI] - exiting. Cheerio :-)" << std::endl;
  std::cout.rdbuf(coutbuf); // reset to stdout before exiting
}

void MainWindow::heartbeatOSC() {

  Message msg("/gui-heartbeat");
  msg.pushStr(guiID.toStdString());
  sendOSC(msg);
}


void MainWindow::updateDocPane(QListWidgetItem *cur) {
  QString url = cur->data(32).toString();
  docPane->setSource(QUrl(url));
}

void MainWindow::updateDocPane2(QListWidgetItem *cur, QListWidgetItem *prev) {
  (void)prev;
  updateDocPane(cur);
}

void MainWindow::addHelpPage(QListWidget *nameList,
                             struct help_page *helpPages, int len) {
  int i;
  struct help_entry entry;
  entry.pageIndex = docsCentral->count()-1;

  for(i = 0; i < len; i++) {
    QListWidgetItem *item = new QListWidgetItem(helpPages[i].title);
    item->setData(32, QVariant(helpPages[i].url));
    item->setSizeHint(QSize(item->sizeHint().width(), 25));
    nameList->addItem(item);
    entry.entryIndex = nameList->count()-1;

    if (helpPages[i].keyword != NULL) {
      helpKeywords.insert(helpPages[i].keyword, entry);
      // magic numbers ahoy
      // to be revamped along with the help system
      switch (entry.pageIndex) {
      case 2:
        autocomplete->addSymbol(SonicPiAPIs::Synth, helpPages[i].keyword);
        break;
      case 3:
        autocomplete->addSymbol(SonicPiAPIs::FX, helpPages[i].keyword);
        break;
      case 5:
        autocomplete->addKeyword(SonicPiAPIs::Func, helpPages[i].keyword);
        break;
      }
    }
  }
}

QListWidget *MainWindow::createHelpTab(QString name) {
  QListWidget *nameList = new QListWidget;
  connect(nameList,
	  SIGNAL(itemPressed(QListWidgetItem*)),
	  this, SLOT(updateDocPane(QListWidgetItem*)));
  connect(nameList,
	  SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
	  this, SLOT(updateDocPane2(QListWidgetItem*, QListWidgetItem*)));

  QShortcut *up = new QShortcut(ctrlKey('p'), nameList);
  up->setContext(Qt::WidgetShortcut);
  connect(up, SIGNAL(activated()), this, SLOT(helpScrollUp()));
  QShortcut *down = new QShortcut(ctrlKey('n'), nameList);
  down->setContext(Qt::WidgetShortcut);
  connect(down, SIGNAL(activated()), this, SLOT(helpScrollDown()));

  QBoxLayout *layout = new QBoxLayout(QBoxLayout::LeftToRight);
  layout->addWidget(nameList);
  layout->setStretch(1, 1);
  QWidget *tabWidget = new QWidget;
  tabWidget->setLayout(layout);
  docsCentral->addTab(tabWidget, name);
  helpLists.append(nameList);
  return nameList;
}

void MainWindow::helpScrollUp() {
  int section = docsCentral->currentIndex();
  int entry = helpLists[section]->currentRow();

  if (entry > 0)
    entry--;
  helpLists[section]->setCurrentRow(entry);
}

void MainWindow::helpScrollDown() {
  int section = docsCentral->currentIndex();
  int entry = helpLists[section]->currentRow();

  if (entry < helpLists[section]->count()-1)
    entry++;
  helpLists[section]->setCurrentRow(entry);
}

void MainWindow::docScrollUp() {
  docPane->verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
}

void MainWindow::docScrollDown() {
  docPane->verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
}

void MainWindow::helpClosed(bool visible) {
  if (visible) return;
  // redock on close
  if (!hidingDocPane)
    docWidget->setFloating(false);
  hidingDocPane = false;
}

void MainWindow::tabNext() {
  int index = tabs->currentIndex();
  if (index == tabs->count()-1)
    index = 0;
  else
    index++;
  QMetaObject::invokeMethod(tabs, "setCurrentIndex", Q_ARG(int, index));
}

void MainWindow::tabPrev() {
  int index = tabs->currentIndex();
  if (index == 0)
    index = tabs->count() - 1;
  else
    index--;
  QMetaObject::invokeMethod(tabs, "setCurrentIndex", Q_ARG(int, index));
}

void MainWindow::setLineMarkerinCurrentWorkspace(int num) {
  if(num > 0) {
    SonicPiScintilla *ws = (SonicPiScintilla*)tabs->currentWidget();
    ws->setLineErrorMarker(num - 1);
  }
}

void MainWindow::setUpdateInfoText(QString t) {
  update_info->setText(t);
}

void MainWindow::addUniversalCopyShortcuts(QTextEdit *te){
  new QShortcut(ctrlKey('c'), te, SLOT(copy()));
  new QShortcut(ctrlKey('a'), te, SLOT(selectAll()));

  new QShortcut(metaKey('c'), te, SLOT(copy()));
  new QShortcut(metaKey('a'), te, SLOT(selectAll()));
}

QString MainWindow::asciiArtLogo(){
  return readFile(":/images/logo.txt");
}

void MainWindow::printAsciiArtLogo(){
  QString s = asciiArtLogo();
#if QT_VERSION >= 0x050400
  qDebug().noquote() << s;
#else
  //noquote requires QT 5.4
  qDebug() << s;
#endif
}

void MainWindow::requestVersion() {
    Message msg("/version");
    msg.pushStr(guiID.toStdString());
    sendOSC(msg);
}

void MainWindow::open_sonic_pi_net() {
  QDesktopServices::openUrl(QUrl("http://sonic-pi.net", QUrl::TolerantMode));
}

void MainWindow::updateVersionNumber(QString v, int v_num,QString latest_v, int latest_v_num, QDate last_checked, QString platform) {
  version = v;
  version_num = v_num;
  latest_version = latest_v;
  latest_version_num = latest_v_num;

  // update status bar
  versionLabel->setText(QString("Sonic Pi " + v + " on " + platform + " "));

  // update preferences
  QString last_update_check = tr("Last checked %1").arg(last_checked.toString());

  QString preamble = tr("Sonic Pi checks for updates\nevery two weeks.");

  QString print_version = tr("This is Sonic Pi %1");
  QString new_version = tr("Version %2 is now available!");

  if(v_num < latest_v_num) {
    setUpdateInfoText(QString(preamble + "\n\n" + print_version + "\n\n" + new_version).arg(version, latest_version));
    visit_sonic_pi_net->setText(tr("New version available!\nGet Sonic Pi %1").arg(latest_version));
    check_updates_now->setVisible(false);
    visit_sonic_pi_net->setVisible(true);
  }
  else {
    setUpdateInfoText(QString(preamble + "\n\n" + print_version + "\n\n" + last_update_check).arg(version));
  }
}


void MainWindow::setupLogPathAndRedirectStdOut() {
  QDir().mkdir(sp_user_path);
  QDir().mkdir(log_path);

  if(homeDirWritable) {
    coutbuf = std::cout.rdbuf();
    stdlog.open(gui_log_path.toStdString().c_str());
    std::cout.rdbuf(stdlog.rdbuf());
  }
}


bool MainWindow::eventFilter(QObject *obj, QEvent *evt)
{
    if(obj==qApp && ( evt->type() == QEvent::ApplicationActivate ))
    {
      statusBar()->showMessage(tr("Welcome back. Now get your live code on..."), 2000);
      update();
    }
    return QMainWindow::eventFilter(obj, evt);
}

QString MainWindow::sonicPiHomePath() {
  QString path = qgetenv("SONIC_PI_HOME").constData();
  if (path.isEmpty()) {
    return QDir::homePath();
  }
  else {
    return path;
  }
}
#include "ruby_help.h"
