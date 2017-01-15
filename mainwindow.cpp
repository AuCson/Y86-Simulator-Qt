#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "core.h"
#include <cstring>
#include <string>
#include <iostream>
#include <cctype>

#define histstat Logic->hist_stat

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QColor c = QColor::fromRgb(252,252,252);
    ui->centralWidget->setPalette(QColor::fromRgb(245,245,245));
    ui->centralWidget->setAutoFillBackground(true);
    ui->tab_3->setPalette(c);
    ui->tab_3->setAutoFillBackground(true);
    ui->tab_2->setPalette(c);
    ui->tab_2->setAutoFillBackground(true);
    ui->tab_6->setPalette(c);
    ui->tab_6->setAutoFillBackground(true);
}


MainWindow::~MainWindow()
{
    delete ui;
}


/*
 *
 * Mainwindow Basic Logics -----------------------------------
 *
 *
*/

void MainWindow::init_disp()
{
    for(int i=0;i<6;++i)
        histstat[i]=-1;
    ui->tdisas->clear();

    qlog.clear();

    //address disp
    ui->addrtable->clear();
    ui->addrbox->clear();
    addrcnt = 0;
    ui->addrtable->setRowCount(10);
    ui->addrtable->setColumnCount(2);
    ui->addrtable->setColumnWidth(0,110);
    ui->addrtable->setColumnWidth(1,110);
    QStringList header;
    header<<"Address"<<"Value";
    ui->addrtable->setHorizontalHeaderLabels(header);

    //disass disp
    disabledisass = disabledisass_buf;
    disass(ui->Code->toPlainText().toStdString());
    ui->lstack->clear();
}

void MainWindow::do_file_read()
{
    FILE* lg = fopen("runlog.txt","a");
    Logic = new Core();
    Logic->init();
    qlog.clear();

    QString filename = QFileDialog::getOpenFileName(this);
    char buf[1000];
    std::strcpy(buf,filename.toStdString().c_str());
    FILE* fp = fopen(buf,"rb");
    ui->Code->setVisible(true);
    if(!fp)
    {
        QMessageBox::warning(this,"Error","File not load");
        return;
    }
    Logic->inslen = 0;
    while(fread(&Logic->instr[Logic->inslen],sizeof(unsigned char),1,fp))
        Logic->inslen++;
    for(int i=0;i<Logic->inslen;++i)
        Logic->mem[i] = Logic->instr[i];

    fprintf(lg,"%d",Logic->inslen);
    show_code();
    init_disp();
    showpipe();
    showcpi();
    f_pc_show();


}

void MainWindow::showcpi()
{
    double t = Logic->cpi();
    double t_inv = 1.00/t;
    ui->cpi->setValue((int)(100/t));
    ui->cpil->setText(QString::number(t_inv));
}


void MainWindow::show_code()
{
    std::string s;
    for(int i=0;i<Logic->inslen;++i)
    {
        unsigned char a = Logic->instr[i] >> 4;
        unsigned char b = Logic->instr[i] & 0xF;
        if(a<=9)
            s.push_back(a + '0');
        else
            s.push_back(a + 'A' - 10);
        if(b<=9)
            s.push_back(b + '0');
        else
            s.push_back(b + 'A' - 10);
    }

    QString qs = QString::fromStdString(s);
    ui->Code->setText(qs);
}
void MainWindow::read_ascii(std::string s)
{
    if(s.size()%2!=0)
    {
        QMessageBox::warning(this,"warning","Illegal input");
        return;
    }
    qlog.clear();
    Logic->init();
    Logic->inslen = s.size()/2;
    for(int i=0;i<s.size();i+=2)
    {
        char buf[3];
        buf[0]=s[i];
        buf[1]=s[i+1];
        buf[2]='\0';
        int t;
        sscanf(buf,"%x",&t);
        Logic->instr[i/2] = (unsigned char)t;
        Logic->mem[i/2] = (unsigned char)t;
    }
    show_code();
    init_disp();
    showpipe();
    f_pc_show();
    QMessageBox::information(this,"info","Code loaded");
}

int MainWindow::program_finished()
{
    if(Logic->W_stat==Logic->SAOK)
        return 0;
    else return Logic->W_stat;
}


void MainWindow::stepi()
{
    if(program_finished())
    {
        QMessageBox::information(this,"VisualY++","Program finished");
        return;
    }

    Logic->logs.clear();
    Logic->PipeLogic();
    Logic->stageW();
    //QMessageBox::information(this,"VisualY++","w finished");
    showpipe();
    Logic->stageM();
    //QMessageBox::information(this,"VisualY++","m finished");
    showpipe();
    Logic->stageE();
    //QMessageBox::information(this,"VisualY++","e finished");
    showpipe();
    Logic->stageD();
    //QMessageBox::information(this,"VisualY++","d finished");
    showpipe();
    Logic->stageF();
    //QMessageBox::information(this,"VisualY++","f finished");
    Logic->CLK++;

    showall();
    backup();

    if(isgamemode && Logic->REG[Logic->RESI]==3)
    {
        QMessageBox::information(this,"Visual Y++","You win!");
        isgamemode = 0;
    }
}

/*
 *
 * Run / Stop / backup / undo Handler -------------------------------------
 *
*/
void MainWindow::autorunhandler()
{
    while(isrunning)
    {
        QElapsedTimer t;
        t.start();
        while(t.elapsed() * speed<10000)
            QApplication::processEvents();
        stepi();
        if(!isrunning || program_finished())
        {
            if(program_finished())
                QMessageBox::information(this,"VisualY++","Program finished");
            isrunning =0;
            break;
        }
    }

}

void MainWindow::backup()
{
    Core* t = new Core(*Logic);
    hist.push_back(t);
    if(hist.size()>20)
    {
        t = hist.front();
        delete t;
        hist.pop_front();
    }
}

void MainWindow::undo()
{

    if(hist.size()<=1)
    {
        QMessageBox::warning(this,"VisualY++","Cannot undo");
        return;
    }
    hist.pop_back();
    delete Logic;
    Logic = new Core(*hist.back());
    showpipe();
    qlog+=">>>undo\n";
    ui->tlog->setText(qlog);
    ui->tlog->moveCursor(QTextCursor::End);
    showcpi();
    showstack();
    f_pc_show(1);
}


/*
 *
 *
 *  Display Logic-------------------------------------------------------------------
 *
 *
*/


void MainWindow::showall()
{
    f_pc_show();
    showpipe();
    showlog();
    showcpi();
    showstack();
}

void MainWindow::f_pc_show(int noupdate)
{
    ui->lcycle->setText(QString::number(Logic->CLK));

    if(disabledisass) return;

    for(int i=0;i<ui->tdisas->rowCount();++i)
    {
        if(ui->tdisas->item(i,1)!=NULL && ui->tdisas->item(i,0)!=NULL && ui->tdisas->item(i,2)!=NULL)
        {
            ui->tdisas->item(i,2)->setBackgroundColor(QColor::fromRgb(255,255,255));
            ui->tdisas->item(i,1)->setBackgroundColor(QColor::fromRgb(255,255,255));
            ui->tdisas->item(i,0)->setBackgroundColor(QColor::fromRgb(255,255,255));
            ui->tdisas->item(i,2)->setFont(QFont("等线",9,QFont::Normal));
            ui->tdisas->item(i,1)->setFont(QFont("等线",9,QFont::Normal));
            ui->tdisas->item(i,0)->setFont(QFont("等线",9,QFont::Normal));
            ui->tdisas->item(i,2)->setText("");
        }
    }
    int x;
    int r;

    if(!noupdate)
    {
        for(int i =0;i<100;++i)
        {
            QString t = ui->tdisas->item(i,0)->text();
            if(t.isEmpty())
                break;
            sscanf(t.toStdString().c_str(),"%x",&x);

            if(Logic->f_pc >= Logic->inslen)
            {
                r = -1;
                break;
            }
            if(x == Logic->f_pc)
            {
                r = i;
                break;
            }
        }
        for(int i=4;i>=0;--i)
        {
            if(histstat[i]>=0)
                ui->tdisas->item(histstat[i],2)->setText("");
            if(!(i==0 && Logic->D_stall))
                histstat[i+1]=histstat[i];

        }

        if(Logic->D_bubble)
            histstat[1]=-1;
        if(Logic->E_bubble)
            histstat[2]=-1;
        if(Logic->M_bubble)
            histstat[3]=-1;
        histstat[0] = r;
    }
    char name[5][3] = {"F","D","E","M","W"};
    for(int i=0;i<5;++i)
    {
        if(histstat[i]!=-1 && ui->tdisas->item(histstat[i],1)!=NULL)
        {
            ui->tdisas->setItem(histstat[i],2,new QTableWidgetItem(name[i]));
            ui->tdisas->item(histstat[i],2)->setBackgroundColor(QColor::fromRgb(249,236,168));
            ui->tdisas->item(histstat[i],1)->setBackgroundColor(QColor::fromRgb(249,236,168));
            ui->tdisas->item(histstat[i],0)->setBackgroundColor(QColor::fromRgb(249,236,168));
            ui->tdisas->item(histstat[i],2)->setFont(QFont("等线",9,QFont::Bold));
            ui->tdisas->item(histstat[i],1)->setFont(QFont("等线",9,QFont::Bold));
            ui->tdisas->item(histstat[i],0)->setFont(QFont("等线",9,QFont::Bold));
        }
    }

}

void MainWindow::showstack()
{
   if(disablecallstk) return;
    //call stack disp
   QStringList header;
   header.clear();
   header<<"Stack addr"<<"Func name"<<"Cycle";
   ui->lstack->clear();
   ui->lstack->setRowCount(100);
   ui->lstack->setColumnCount(3);
   ui->lstack->setColumnWidth(0,100);
   ui->lstack->setColumnWidth(1,100);
   ui->lstack->setHorizontalHeaderLabels(header);
   for(int i=0;i<Logic->stkaddr.size();++i)
   {
       sprintf(buf,"%x",Logic->stkaddr[i]);
       ui->lstack->setItem(i,0,new QTableWidgetItem (buf));
       sprintf(buf,"%x",Logic->stkfunc[i]);
       ui->lstack->setItem(i,1,new QTableWidgetItem (buf));
       ui->lstack->setItem(i,2,new QTableWidgetItem(QString::number(Logic->stkclk[i]-2)));
   }
}
void MainWindow::showlog()
{
    QString s = ">>>Begin cycle ";
    s+= QString::number(Logic->CLK);
    s+="\n";
    for(auto i = Logic->logs.begin();i!=Logic->logs.end();++i)
    {
        s+=QString::fromStdString(*i);
        s+="\n";
    }
    s+=">>>end\n\n";
    qlog+=s;

    ui->tlog->setText(qlog);
    ui->tlog->moveCursor(QTextCursor::End);
    if(qlog.size()>1000 && flush_auto)
    {
        qlog.clear();
    }

}

inline QString ascii(int ch)
{
    char temp[10];
    sprintf(temp,"%x",ch);
    return QString(temp);
}
inline QString asciistat(char c)
{
    switch(c)
    {
    case 0:return QString("BUB");
    case 1:return QString("AOK");
    case 2:return QString("HLT");
    case 3:return QString("ADR");
    case 4:return QString("INS");
    default:return QString("???");
    }
}
inline QString asciireg(char c)
{
    switch(c)
    {
    case 0:return QString("0/%eax");
    case 1:return QString("1/%ecx");
    case 2:return QString("2/%edx");
    case 3:return QString("3/%ebx");
    case 4:return QString("4/%esp");
    case 5:return QString("5/%ebp");
    case 6:return QString("6/%edi");
    case 7:return QString("7/%esi");
    case 0xF:return QString("F/NONE");
    default:return QString("?/?");
    }
}

void MainWindow::showpipe()
{

    ui->lwdste->setText(asciireg(Logic->W_dstE));
    ui->lwdstm->setText(asciireg(Logic->W_dstM));
    ui->lwstat->setText(asciistat(Logic->W_stat));
    ui->lwicode->setText(ascii(Logic->W_icode));
    ui->lwvale->setText(ascii(Logic->W_valE));
    ui->lwvalm->setText(ascii(Logic->W_valM));
    ui->lmstat->setText(asciistat(Logic->M_stat));
    ui->lmicode->setText(ascii(Logic->M_icode));
    ui->lmcnd->setText(ascii(Logic->M_Cnd));
    ui->lmvale->setText(ascii(Logic->M_valE));
    ui->lmvala->setText(ascii(Logic->M_valA));
    ui->lmdstm->setText(asciireg(Logic->M_dstM));
    ui->lmdste->setText(asciireg(Logic->M_dstE));
    ui->ledstm->setText(asciireg(Logic->E_dstM));
    ui->ledste->setText(asciireg(Logic->E_dstE));
    ui->lesrca->setText(asciireg(Logic->E_srcA));
    ui->lesrcb->setText(asciireg(Logic->E_srcB));
    ui->levala->setText(ascii(Logic->E_valA));
    ui->levalb->setText(ascii(Logic->E_valB));
    ui->levalc->setText(ascii(Logic->E_valC));
    ui->leicode->setText(ascii(Logic->E_icode));
    ui->leifun->setText(ascii(Logic->E_ifun));
    ui->lestat->setText(asciistat(Logic->E_stat));
    ui->ldstat->setText(asciistat(Logic->D_stat));
    ui->ldicode->setText(ascii(Logic->D_icode));
    ui->ldifun->setText(ascii(Logic->D_ifun));
    ui->ldra->setText(asciireg(Logic->D_rA));
    ui->ldrb->setText(asciireg(Logic->D_rB));
    ui->ldvalc->setText(ascii(Logic->D_valC));
    ui->ldvalp->setText(ascii(Logic->D_valP));
    ui->lpredpc->setText(ascii(Logic->f_predPC));
    ui->lfpc->setText(ascii(Logic->f_pc));
    ui->lfstat->setText(asciistat(Logic->f_stat));
    ui->lficode->setText(ascii(Logic->f_icode));
    ui->lfifun->setText(ascii(Logic->f_ifun));
    ui->lstat->setText(asciistat(Logic->stat));
    ui->leax->setText(ascii(Logic->REG[0]));
    ui->lecx->setText(ascii(Logic->REG[1]));
    ui->ledx->setText(ascii(Logic->REG[2]));
    ui->lebx->setText(ascii(Logic->REG[3]));
    ui->lesp->setText(ascii(Logic->REG[4]));
    ui->lebp->setText(ascii(Logic->REG[5]));
    ui->lesi->setText(ascii(Logic->REG[6]));
    ui->ledi->setText(ascii(Logic->REG[7]));
    ui->lzso->setText(ascii(Logic->ZF) + ascii(Logic->SF) + ascii(Logic->OF));

}

void MainWindow::show_address()
{

    QString text = ui->addrbox->currentText();

    ui->addrtable->setRowCount(10+addrcnt);

    for(int i=0;i<(int)ui->addrbox->count()-1;++i)
    {
        if(ui->addrbox->itemText(i)==text)
            return;
    }
    ui->addrtable->setItem(addrcnt,0,new QTableWidgetItem(text));
    addrcnt++;

    updateaddr();
}

void MainWindow::updateaddr()
{
    for(int i=0;i<addrcnt;++i)
    {
        QString showv;
        QString text = ui->addrtable->item(i,0)->text();
        std::string str = text.toStdString();
        char buf[100];
        strcpy(buf,str.c_str());
        int addr;
        sscanf(buf,"%x",&addr);
        if(Logic->mem.find(addr)==Logic->mem.end())
           showv = "Can't access";
        else
        {
            sprintf(buf,"%x/%x/%x/%x",Logic->mem[addr],Logic->mem.count(addr+1)?Logic->mem[addr+1]:0,
                    Logic->mem.count(addr+2)?Logic->mem[addr+2]:0,Logic->mem.count(addr+3)?Logic->mem[addr+3]:0);
            showv = QString::fromStdString(std::string(buf));
        }
        ui->addrtable->setItem(i,1,new QTableWidgetItem(showv));
    }
}

/*
 *
 * Disassembler Logic -----------------------------------------------------
 *
 *
*/

int MainWindow::gethex(const std::string &s,int i,int n)
{
    int ans = 0,t;
    for(int j=0;j<n;j+=2)
    {
        char bufh[3];
        bufh[0]=s[i+j];
        bufh[1] = s[i+j+1];
        bufh[2]='\0';
        sscanf(bufh,"%x",&t);
        ans += t << (4*j);
    }
    return ans;
}

std::string MainWindow::regname(int i,int &error)
{
    if(i<0||i>=8)
    {
        error = 1;
        return Logic->regname[8];
    }
    else
        return Logic->regname[i];
}

void MainWindow::disass(std::string s)
{
    if(s.size()==0 || disabledisass)
        return;
    ui->tdisas->clear();
    int i=0,r=0;
    int segerr = 0;
    char buf[1000];
    char a[4][5]={"addl","subl","andl","xorl"};
    char b[7][5]={"jmp","jle","jl","je","jne","jge","jg"};
    ui->tdisas->setColumnCount(3);
    ui->tdisas->setRowCount(s.size()/2);
    ui->tdisas->setColumnWidth(0,50);
    ui->tdisas->setColumnWidth(1,180);
    ui->tdisas->setColumnWidth(2,40);
    QStringList header;
    header<<"Addr"<<"Instr"<<"Status";
    ui->tdisas->setHorizontalHeaderLabels(header);
    while(i<s.size())
    {
        int err = 0;
        if(isspace(s[i]))
        {
            i++;
            continue;
        }
        sprintf(buf,"%x",i/2);
        ui->tdisas->setItem(r,0,new QTableWidgetItem(buf));
        char t[12];
        t[0]=s[i];
        t[1]=0;
        switch (s[i]) {
        case '0':
            ui->tdisas->setItem(r,1,new QTableWidgetItem("halt"));
            i+=2;
            break;
        case '1':
            ui->tdisas->setItem(r,1,new QTableWidgetItem("nop"));
            i+=2;
            break;
        case '2':
            if(i+3>=s.size()){i+=4;segerr = 1;break;}
            sprintf(buf,"rrmovl %s,%s",regname(s[i+2]-'0',err).c_str(),regname(s[i+3]-'0',err).c_str());
            ui->tdisas->setItem(r,1,new QTableWidgetItem(buf));
            if(err){i+=2;break;}
            i+=4;
            break;
        case '3':
            if(i+11>=s.size()){i+=12;segerr = 1;break;}
            sprintf(buf,"irmovl $0x%x,%s",gethex(s,i+4,8),regname(s[i+3]-'0',err).c_str());
            ui->tdisas->setItem(r,1,new QTableWidgetItem(buf));
            ;
            if(err){i+=2;break;}
            i+=12;
            break;
        case '4':
            if(i+11>=s.size()){i+=12;segerr = 1;break;}
            sprintf(buf,"rmmovl %s,0x%x(%s)",regname(s[i+2]-'0',err).c_str(),gethex(s,i+4,8),regname(s[i+3]-'0',err).c_str());
            ui->tdisas->setItem(r,1,new QTableWidgetItem(buf));
            if(err){i+=2;break;}
            i+=12;
            break;
        case '5':
            if(i+11>=s.size()){i+=12;segerr = 1;break;}
            sprintf(buf,"mrmovl 0x%x(%s),%s",gethex(s,i+4,8),regname(s[i+3]-'0',err).c_str(),regname(s[i+2]-'0',err).c_str());
            ui->tdisas->setItem(r,1,new QTableWidgetItem(buf));
            if(err){i+=2;break;}
            i+=12;
            break;
        case '6':
            if(i+3>=s.size()){i+=4;segerr = 1;break;}
            sprintf(buf,"%s %s,%s",a[s[i+1]-'0'],regname(s[i+2]-'0',err).c_str(),regname(s[i+3]-'0',err).c_str());
            ui->tdisas->setItem(r,1,new QTableWidgetItem(buf));
            if(err){i+=2;break;}
            i+=4;
            break;
        case '7':
            if(i+9>=s.size()){i+=10;segerr = 1;break;}
            sprintf(buf,"%s 0x%x",b[s[i+1]-'0'],gethex(s,i+2,8));
            ui->tdisas->setItem(r,1,new QTableWidgetItem(buf));
            i+=10;
            break;
        case '8':
            if(i+9>=s.size()){i+=10;segerr = 1;break;}
            sprintf(buf,"call 0x%x",gethex(s,i+2,8));
            ui->tdisas->setItem(r,1,new QTableWidgetItem(buf));
            i+=10;
            break;
        case '9':
            ui->tdisas->setItem(r,1,new QTableWidgetItem("ret"));
            i+=2;
            break;
        case 'A':case 'a':
            if(i+3>=s.size()){i+=4;segerr = 1;break;}
            sprintf(buf,"pushl %s",regname(s[i+2]-'0',err).c_str());
            ui->tdisas->setItem(r,1,new QTableWidgetItem(buf));
            if(err){i+=2;break;}
            i+=4;
            break;
        case 'B':case'b':
            if(i+3>=s.size()){i+=4;segerr = 1;break;}
            sprintf(buf,"popl %s",regname(s[i+2]-'0',err).c_str());
            ui->tdisas->setItem(r,1,new QTableWidgetItem(buf));
            if(err){i+=2;break;}
            i+=4;
            break;
        case 'C':case'c':
            if(i+3>=s.size()){i+=4;segerr = 1;break;}
            sprintf(buf,"cmpl %s,%s",regname(s[i+2]-'0',err).c_str(),regname(s[i+3]-'0',err).c_str());
            ui->tdisas->setItem(r,1,new QTableWidgetItem(buf));
            if(err){i+=2;break;}
            i+=4;
            break;
        case 'D':case 'd':
            ui->tdisas->setItem(r,1,new QTableWidgetItem("leave"));
            i+=2;
            break;
        default:
            ui->tdisas->setItem(r,1,new QTableWidgetItem("???"));
            i+=2;
            break;
        }

        r++;

    }
    if(segerr)
    {
        QMessageBox::warning(this,"warning","The code contains illegal use of legal operations. Dynamic disassembler highlighting is disabled.");
        ui->disableass->setChecked(1);
        disabledisass = disabledisass_buf = 1;
    }
}




/*
 * Slot functions ---------------------------------------------------------------
 *
*/
void MainWindow::on_actionSave_Log_triggered()
{

}

void MainWindow::on_pushButton_2_clicked()
{
    QString s = ui->leax->text();
    Logic->REG[0] = s.toInt();
}


void MainWindow::on_pushButton_3_clicked()
{
    QString s = ui->lecx->text();
    Logic->REG[1] = s.toInt();
}

void MainWindow::on_pushButton_4_clicked()
{
    QString s = ui->ledx->text();
    Logic->REG[2] = s.toInt();
}

void MainWindow::on_pushButton_5_clicked()
{
    QMessageBox::information(this,"VisualY++","You started Game: Guess the numbers!\ninput 3 numbers[0-5] at eax,ecx,edx, and esi shows the number of correct numbers\nGo to Editor and load the code!");
    isgamemode = 1;
    ui->Code->setText("30 f4 00 01 00 00\
                      30 f5 02 00 00 00\
                      40 54 00 00 00 00\
                      30 f5 05 00 00 00\
                      40 54 04 00 00 00\
                      30 f5 01 00 00 00\
                      40 54 08 00 00 00\
                      50 34 00 00 00 00\
                      61 03\
                      74 3F 00 00 00\
                      30 f5 01 00 00 00\
                      60 56 \
                      50 34 04 00 00 00\
                      61 13\
                      74 54 00 00 00\
                      30 f5 01 00 00 00\
                      60 56\
                      50 34 08 00 00 00\
                      61 23\
                      74 69 00 00 00\
                      30 f5 01 00 00 00\
                      60 56\
                      30 f5 03 00 00 00\
                      61 65\
                      30 f6 00 00 00 00\
                      74 24 00 00 00 00\
                      10\
                      10\
                      30 f7 01 00 00 00");
}

void MainWindow::on_addrbox_activated()
{
    show_address();
}

void MainWindow::on_bdisass_clicked()
{
    disass(ui->Code->toPlainText().toStdString());
}

void MainWindow::on_disableass_clicked()
{
    disabledisass_buf = ui->disableass->checkState()==Qt::Checked;
}

void MainWindow::on_disablecallstk_clicked()
{
    disablecallstk = ui->disablecallstk->checkState() == Qt::Checked;
}

void MainWindow::on_flush_auto_clicked()
{
    flush_auto = ui->flush_auto->checkState() == Qt::Checked;
}

void MainWindow::on_Next_clicked()
{
    stepi();
}


void MainWindow::on_actionLoad_2_triggered()
{
    do_file_read();
}

void MainWindow::on_bauto_clicked()
{
    isrunning = !isrunning;
    autorunhandler();
}

void MainWindow::on_horizontalSlider_actionTriggered(int action)
{
    speed = ui->horizontalSlider->value();
}

void MainWindow::on_actionUndo_triggered()
{
    undo();
}

void MainWindow::on_pushButton_clicked()
{

}

void MainWindow::on_bloadascode_clicked()
{
    std::string text = ui->Code->toPlainText().toStdString();

    std::string s;
    for(int i=0;i<text.size();i++)
    {
        if(!isspace(text[i]))
        {
            s.push_back(text[i]);
        }
    }
    if(s.empty())
        return;
    read_ascii(s);


}
