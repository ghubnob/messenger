#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "chatwin.h"
#include <QFile>
#include <QVBoxLayout>
#include <QStandardItemModel>
#include <QDir>
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QKeyEvent>
#include <QtGui>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , model(new QStandardItemModel(this))
{
    ui->setupUi(this);
    setWindowTitle("Messenger");
    setupListView();
    loadDialogs();

    connect(ui->listView, &QListView::doubleClicked, this, &MainWindow::openDialogFromList);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    ui->delDialogLabel->setText("");
    QString sender = ui->lineEdit->text();
    chatwin *chat = new chatwin();
    QString wintitle = QString("Chatting with %1").arg(sender);
    chat->changeChatLabel(sender);
    QString username = ui->name->text();
    chat->changeNameLabel(username);
    chat->setWindowTitle(wintitle);
    chat->show();
    chat->exec();
    if (chat->close()) chat->onClosed();
    model->clear();
    loadDialogs();
}


void MainWindow::loadDialogs()
{
    QDir dir = QDir::current();
    QStringList filters;
    filters << "*.db";
    dir.setNameFilters(filters);
    QStringList dbFiles = dir.entryList();
    foreach (QString dbFile, dbFiles) {
        QString suitableSender = dbFile;
        suitableSender.chop(3);
        QStandardItem *item = new QStandardItem(suitableSender);
        item->setData(suitableSender, Qt::UserRole);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        model->appendRow(item);
    }
}

void MainWindow::setupListView() {
    ui->listView->setModel(model);
    ui->listView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->listView, &QListView::customContextMenuRequested, this, &MainWindow::showContextMenu);
    QFont font = ui->listView->font();
    font.setPointSize(12);
    ui->listView->setFont(font);
    ui->listView->setMaximumHeight(480);
    ui->listView->setMaximumWidth(367);
}

void MainWindow::showContextMenu(const QPoint &pos)
{
    QModelIndex index = ui->listView->indexAt(pos);
    if (!index.isValid()) return;
    contextMenu = new QMenu(this);
    QAction *deleteAction = new QAction("Удалить диалог", this);
    connect(deleteAction, &QAction::triggered, this, &MainWindow::deleteDialog);
    contextMenu->addAction(deleteAction);
    contextMenu->exec(ui->listView->viewport()->mapToGlobal(pos));
}

void MainWindow::deleteDialog()
{
    QModelIndex index = ui->listView->currentIndex();
    if (index.isValid())
    {
        QSqlDatabase::removeDatabase("qt_sql_default_connection");
        QString filePath = QDir::currentPath()+"/"+model->data(index,Qt::DisplayRole).toString()+".db";
        if (QFile::remove(filePath)) {
            ui->delDialogLabel->setText("dialog deleted");
        } else QMessageBox::warning(this,"Messenger Error","error deleteng the dialog. try opening another dialog and then deleting this one");
        model->clear();
        loadDialogs();
    }
}

void MainWindow::openDialogFromList() {
    ui->delDialogLabel->setText("");
    QModelIndex index = ui->listView->currentIndex();
    if (index.isValid()) {
        QString sender = model->data(index, Qt::DisplayRole).toString();
        QString username = ui->name->text();
        chatwin *chat = new chatwin();
        QString wintitle = QString("Chatting with %1").arg(sender);
        chat->changeChatLabel(sender);
        chat->changeNameLabel(username);
        chat->setWindowTitle(wintitle);
        chat->show();
        chat->exec();
        if (chat->close()) chat->onClosed();
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Delete) deleteDialog();
    else QMainWindow::keyPressEvent(event);
}
