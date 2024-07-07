#include "chatwin.h"
#include "qsqlerror.h"
#include "ui_chatwin.h"
#include <QMenu>
#include <QContextMenuEvent>
#include <QAction>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QTextStream>
#include <QFile>
#include <QDateTime>
#include <QStandardItemModel>
#include <QCryptographicHash>
#include <QSqlQuery>

chatwin::chatwin(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::chatwin)
    , listView(new QListView(this)), model(new QStandardItemModel(this))
{
    ui->setupUi(this);
    setupUi();
    setupListView();
    connect(ui->testmsgline, &QLineEdit::returnPressed, this, [=]() {
        ui->testbutton_sender->setDefault(true);
    });
    connect(ui->msgline, &QLineEdit::returnPressed, this, [=]() {
        ui->pushButton->setDefault(true);
    });
}

chatwin::~chatwin()
{
    db.close(); // Закрываем подключение к базе данных при удалении объекта
    delete ui;
}

void chatwin::changeChatLabel(QString &text) {
    ui->label->setText(text);
    setupDatabase(); // Переносим вызов setupDatabase сюда
    loadMessagesFromDatabase();
}

void chatwin::setupUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(listView);
    setLayout(layout);
}

void chatwin::setupListView() {
    model = new QStandardItemModel(this);
    listView->setModel(model);
    listView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(listView, &QListView::customContextMenuRequested, this, &chatwin::showContextMenu);
    QFont font = listView->font();
    font.setPointSize(10);
    listView->setFont(font);
    listView->setMaximumHeight(511);
    listView->setMaximumWidth(341);
}

void chatwin::showContextMenu(const QPoint &pos)
{
    QModelIndex index = listView->indexAt(pos);
    if (!index.isValid()) return;
    contextMenu = new QMenu(this);
    QAction *deleteAction = new QAction("Удалить сообщение", this);
    connect(deleteAction, &QAction::triggered, this, &chatwin::deleteMessage);
    contextMenu->addAction(deleteAction);
    QString sender = model->data(index, Qt::UserRole).toString();
    if (sender != ui->label->text()) {
        QAction *editAction = new QAction("Изменить сообщение", this);
        connect(editAction, &QAction::triggered, this, &chatwin::editMessage);
        contextMenu->addAction(editAction);
    }
    contextMenu->exec(listView->viewport()->mapToGlobal(pos));
}

void chatwin::deleteMessage()
{
    QModelIndex index = listView->currentIndex();
    if (index.isValid())
    {
        model->removeRow(index.row());
    }
}

void chatwin::editMessage()
{
    QModelIndex index = listView->currentIndex();
    if (index.isValid())
    {
        QString oldMessage = model->data(index, Qt::DisplayRole).toString();
        QStringList messageParts = oldMessage.split('\n');
        QString oldText = messageParts[1];
        QString newText = QInputDialog::getText(this, "Изменить сообщение", "Новое сообщение:", QLineEdit::Normal, oldText);
        if (!newText.isEmpty())
        {
            QString sender = model->data(index, Qt::UserRole).toString();
            QString time = model->data(index, Qt::UserRole + 1).toString();
            QStandardItem *item = model->itemFromIndex(index);
            item->setText(QString("%1, %2\n%3").arg(sender, time, newText + " (edited)"));
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        }
    }
}

void chatwin::on_pushButton_clicked()
{
    QString last_message = ui->msgline->text();
    if (!last_message.isEmpty()) {
        QString currentTime = QDateTime::currentDateTime().toString("hh:mm");
        QStandardItem *item = new QStandardItem();
        QString sender = "You";
        item->setText(QString("%1, %2\n%3").arg(sender, currentTime, last_message));
        item->setData(sender, Qt::UserRole);
        item->setData(currentTime, Qt::UserRole + 1);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        model->appendRow(item);
        saveMessageToDatabase(sender, currentTime, last_message);
        ui->msgline->setText("");
    }
}

void chatwin::saveMessageToDatabase(const QString &sender, const QString &time, const QString &text)
{
    QSqlQuery query;
    query.prepare("INSERT INTO messages (sender, time, text) VALUES (:sender, :time, :text)");
    query.bindValue(":sender", sender);
    query.bindValue(":time", time);
    query.bindValue(":text", text);
    if (!query.exec()) {
        qDebug() << "save error" << query.lastError().text();
    }
}

void chatwin::loadMessagesFromDatabase()
{
    QSqlQuery query("SELECT sender, time, text FROM messages");
    while (query.next()) {
        QString sender = query.value(0).toString();
        QString time = query.value(1).toString();
        QString text = query.value(2).toString();
        QStandardItem *item = new QStandardItem();
        item->setText(QString("%1, %2\n%3").arg(sender, time, text));
        item->setData(sender, Qt::UserRole);
        item->setData(time, Qt::UserRole + 1);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        model->appendRow(item);
    }
}

void chatwin::on_testbutton_sender_clicked()
{
    QString last_message = ui->testmsgline->text();
    if (!last_message.isEmpty()) {
        QString currentTime = QDateTime::currentDateTime().toString("hh:mm");
        QStandardItem *item = new QStandardItem();
        item->setText(QString(ui->label->text() + ", %1\n%2").arg(currentTime, last_message));
        item->setData(ui->label->text(), Qt::UserRole);
        item->setData(currentTime, Qt::UserRole + 1);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        model->appendRow(item);
        saveMessageToDatabase(ui->label->text(), currentTime, last_message);
        ui->testmsgline->setText("");
    }
}

void chatwin::changeNameLabel(QString &name) {
    ui->name->setText(name);
}

void chatwin::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Delete) deleteMessage();
    else QDialog::keyPressEvent(event);
}

void chatwin::setupDatabase() {
    if (QSqlDatabase::contains("qt_sql_default_connection")) {
        QSqlDatabase::database().close();
        QSqlDatabase::removeDatabase("qt_sql_default_connection");
        // QSqlDatabase::removeDatabase();
    }

    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(ui->label->text() + ".db");
    if (!db.open()) {
        qDebug() << "Database Error" << db.lastError().text();
        return;
    }
    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS messages ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "sender TEXT, "
               "time TEXT, "
               "text TEXT)");
}

void chatwin::messagesToDatabaseSave() {
    QSqlQuery query;
    query.exec("DELETE FROM messages");
    for (int i = 0; i < model->rowCount(); ++i) {
        QStandardItem *item = model->item(i);
        QString sender = item->data(Qt::UserRole).toString();
        QString time = item->data(Qt::UserRole + 1).toString();
        QString text = item->text().split('\n').last();
        saveMessageToDatabase(sender, time, text);
    }
}

void chatwin::onClosed() {
    messagesToDatabaseSave();
}
