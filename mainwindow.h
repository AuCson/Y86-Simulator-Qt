#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <list>
#include "core.h"
#include "QMessageBox"
#include "QFileDialog"
#include "QTextStream"
#include "QString"
#include "QElapsedTimer"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void newLogic(){Logic = new Core;}
    void stepi();
    void ui_stepi();
    void do_file_read();
    void backup();
    void showpipe();
    int program_finished();
    void show_address();
    void show_code();
    void read_ascii(std::string s);
    void showlog();
    void autorunhandler();
    void writestatlog();
    void undo();
    void disass(std::string cmd);
    void f_pc_show(int noupdate = 0);
    void showcpi();
    void init_disp();
    void updateaddr();
    void showstack();
    void showall();
    std::string regname(int i,int &error);
    int gethex(const std::string &s,int i,int n);

private slots:
    void on_Next_clicked();

    void on_actionLoad_2_triggered();
    void on_pushButton_clicked();

    void on_bloadascode_clicked();
    void on_bauto_clicked();

    void on_horizontalSlider_actionTriggered(int action);

    void on_actionUndo_triggered();

    void on_actionSave_Log_triggered();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_4_clicked();

    void on_pushButton_5_clicked();

    void on_addrbox_activated();

    void on_bdisass_clicked();

    void on_disableass_clicked();

    void on_disablecallstk_clicked();

    void on_flush_auto_clicked();

private:
    Ui::MainWindow *ui;
    Core* Logic;
    std::list<Core*> hist;

    QString qlog;
    int isrunning = 0;
    int speed = 10;
    int isgamemode = 0;
    int assprev = 0;
    int histstat[6];
    int addrcnt = 0;
    char buf[1000];

    int disabledisass = 0;
    int disablecallstk = 0;
    int disabledisass_buf = 0;
    int flush_auto = 0;
};

#endif // MAINWINDOW_H
