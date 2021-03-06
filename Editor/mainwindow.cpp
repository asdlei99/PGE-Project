/*
 * Platformer Game Engine by Wohlstand, a free platform for game making
 * Copyright (c) 2014-2020 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QtConcurrent>
#include <QFutureWatcher>
#include <QEventLoop>
#include <utility>
#include <stdexcept>

#include <common_features/app_path.h>
#include <common_features/themes.h>
#include <common_features/spash_screen.h>
#include <main_window/dock/toolboxes.h>
#include <common_features/logger_sets.h>
#include <common_features/main_window_ptr.h>

#include <ui_mainwindow.h>
#include "mainwindow.h"

MainWindow::MainWindow(QMdiArea *parent) :
    QMainWindow(parent),
    m_messageBoxer(this),
    ui(new Ui::MainWindow)
{
    MainWinConnect::pMainWin = this;
    m_isAppInited = false;
    m_isAppRestartRequested = false;

    this->setAttribute(Qt::WA_QuitOnClose, true);
    this->setAttribute(Qt::WA_DeleteOnClose, true);
    this->hide();
    setDefaults(); // Apply default common settings

    LogDebug(QString("Set UI..."));
    ui->setupUi(this);

    LogDebug(QString("Setting Lang..."));
    setDefLang();

    LogDebug(QString("Setting UI Defaults..."));
    setUiDefults(); //Apply default UI settings

#ifdef Q_OS_MACX
    foreach(QAction* act, ui->menuBar->actions())
        act->setVisible(false);
    ui->Exit->setEnabled(false);
    ui->actionAbout->setEnabled(false);
    ui->actionApplication_settings->setEnabled(false);
#endif
}

bool MainWindow::isAppRestartRequested()
{
    return m_isAppRestartRequested;
}


bool MainWindow::initEverything(const QString &configDir, const QString &themePack)
{
    currentConfigDir = configDir;

    configs.setConfigPath( configDir );

    try
    {
        if(!configs.loadBasics())
            throw("Fail to load basic configurations");

        Themes::loadTheme(themePack);

        /*********************Splash Screen**********************/
        QPixmap splashimg(configs.splash_logo.isEmpty()?
                          Themes::Image(Themes::splash) :
                          configs.splash_logo);

        EditorSpashScreen splash(splashimg);
        splash.setCursor(Qt::ArrowCursor);
        splash.setDisabled(true);

        for(auto &animation : configs.animations)
        {
            //QPoint pt(416,242);
            QPoint pt(int(animation.x), int(animation.y));
            //QPixmap img = QPixmap("coin.png");
            splash.addAnimation(pt,
                                animation.img,
                                int(animation.frames),
                                int(animation.speed));
        }

        QObject::connect(&configs, SIGNAL(progressMax(int)),
                       &splash, SLOT(progressMax(int)), Qt::QueuedConnection);
        QObject::connect(&configs, SIGNAL(progressTitle(QString)),
                       &splash, SLOT(progressTitle(QString)), Qt::QueuedConnection);
        QObject::connect(&configs, SIGNAL(progressValue(int)),
                       &splash, SLOT(progressValue(int)), Qt::QueuedConnection);
        QObject::connect(&configs, SIGNAL(progressPartsTotal(int)),
                       &splash, SLOT(progressPartsMax(int)), Qt::QueuedConnection);
        QObject::connect(&configs, SIGNAL(progressPartNumber(int)),
                       &splash, SLOT(progressPartsVal(int)), Qt::QueuedConnection);

        /*********************Loading of config pack**********************/
        QFutureWatcher<bool> isOk;
        QEventLoop waitLoop;
        QObject::connect(&isOk, SIGNAL(finished()), &waitLoop, SLOT(quit()));
        // Do the loading in a thread
        isOk.setFuture(QtConcurrent::run(&this->configs, &DataConfig::loadFullConfig));
        /*********************Loading of config pack**********************/

        /*********************Splash Screen**********************/

        splash.show();
        splash.startAnimations();

        // Now wait until the config load in finished.
        waitLoop.exec();

        /*********************Splash Screen end**********************/
        splash.finish(this);
        /*********************Splash Screen end**********************/
        if(!isOk.result())
        {
            throw("Error loading configuration package");
        }

        if(configs.check())
        {
            QMessageBox::warning(this, tr("Configuration error"),
                tr("Configuration package is loaded with errors."), QMessageBox::Ok);
            on_actionCurConfig_triggered();
        }

        LogDebug(QString("Initializing dock widgets..."));
        splash.progressTitle(tr("Initializing dock widgets..."));

        //Apply objects into tools
        dock_LvlSectionProps->initDefaults();
        //dock_LvlItemBox->setLvlItemBoxes();
        //dock_WldItemBox->setWldItemBoxes();
        dock_LvlEvents->reloadSoundsList();
        dock_WldItemProps->resetExitTypesList();
        dock_TilesetBox->setTileSetBox(true);

        // Initialize config pack-side UI settings
        setUiDefultsConfigPack();

        LogDebug(QString("Initialize the testing sub-system..."));
        initTesting();

        LogDebug(QString("Initalizing plugins..."));
        splash.progressTitle(tr("Initalizing plugins..."));
        initPlugins();

        // Load Editor's settings
        loadSettings();

        // Apply setup
        applySetup(true);

        LogDebug(QString("Loading theme..."));
        splash.progressTitle(tr("Loading theme..."));
        applyCurrentTheme();

        LogDebug(QString("Finishing loading..."));
        splash.progressTitle(tr("Finishing loading..."));

        m_isAppInited = true;
    }
    catch(const char *e)
    {
        LogWriter::logLevel = PGE_LogLevel::Debug; //Force debug log
        QMessageBox::critical(this, tr("Configuration error"),
                              tr("Configuration can't be loaded:\n"
                                 "%1.\n"
                                 "See %2 for more information.")
                              .arg(e)
                              .arg(LogWriter::DebugLogFile),
                              QMessageBox::Ok);
        LogFatal(QString("Initialization failure: %1").arg(e));
        LogFatal("<Error, application closed>");
        m_isAppInited = false;
        return false;
    }
    catch(...)
    {
        LogWriter::logLevel = PGE_LogLevel::Debug; //Force debug log
        QMessageBox::critical(this, tr("Configuration error"),
                              tr("Configuration can't be loaded.\nSee %1 for more information.")
                              .arg(LogWriter::DebugLogFile),
                              QMessageBox::Ok);
        LogFatal("<Error, application closed>");
        m_isAppInited = false;
        return false;
    }

#ifdef Q_OS_MACX
    for(QAction* menu : ui->menuBar->actions())
    {
        menu->setVisible(true);
        for(QAction* item : menu->menu()->actions())
            item->setIconVisibleInMenu(false);
    }
    ui->Exit->setMenuRole(QAction::QuitRole);
    ui->Exit->setVisible(true);
    ui->Exit->setEnabled(true);
    ui->actionAbout->setEnabled(true);
    ui->actionApplication_settings->setEnabled(true);
    ui->actionApplication_settings->setMenuRole(QAction::PreferencesRole);
#endif

    return true;
}

MainWindow::~MainWindow()
{
#ifdef Q_OS_WIN
    if(pge_thumbbar)
        delete pge_thumbbar;
#endif
    GlobalSettings::cleanUp();
    m_messageBoxer.disconnectAll();
    closeTesting();
    delete ui;
    MainWinConnect::pMainWin = nullptr;
}


//////////////////SLOTS///////////////////////////

//Exit from application
void MainWindow::on_Exit_triggered()
{
    if(!MainWindow::close())
        return;
}

void MainWindow::formatErrorMsgBox(QString filePath, QString errorMessage, long lineNum, QString lineContents)
{
    QMessageBox::warning(this, QObject::tr("Bad File"),
                PGESTRING( QObject::tr("Bad file format\nFile: %1").arg(filePath+"\n")  //Print Bad data string
                        +( errorMessage.isEmpty()?"":errorMessage+"\n" ) //Print special error message
                        +( lineNum < 0 ? "":QObject::tr("Line Number: %1").arg(lineNum)+"\n" )         //Print Line With error
                        +( lineContents.isEmpty()?"":QObject::tr("Line Data: %1").arg(lineContents)) ),
                         QMessageBox::Ok);
}

//Toolbar context menu
void MainWindow::on_MainWindow_customContextMenuRequested(const QPoint &pos)
{
    LogDebug(QString("Main Menu's context menu called! %1 %2").arg(pos.x()).arg(pos.y()));

    QMenu *cu = new QMenu(this);
    QAction *test= cu->addAction("Nothing");
    test->setEnabled(false);
    QAction *test2= cu->addAction("Nothing");
    test2->setEnabled(false);
    QAction *test3= cu->addAction("Nothing");
    test3->setEnabled(false);

    cu->exec(pos);
}


void MainWindow::showStatusMsg(QString msg, int time)
{
    statusBar()->showMessage(msg, time);
}

void MainWindow::on_actionRefresh_menu_and_toolboxes_triggered()
{
    updateMenus(ui->centralWidget->activeSubWindow(), true);
}


