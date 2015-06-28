#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QScopedPointer>
#include "ConnectionLib/Server/pgenet_server.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_bntStartServer_clicked();

    void addText(QString text);

    void on_bntSendDbgText_clicked();

private:
    Ui::MainWindow *ui;
    QScopedPointer<PGENET_Server> m_server;
};

#endif // MAINWINDOW_H
