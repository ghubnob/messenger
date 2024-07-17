#include "chatwin.h"
#include "qbuffer.h"
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
#include <QDir>
#include <QFileDialog>

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
    db.close();
    delete ui;
}

void chatwin::changeChatLabel(QString &text) {
    ui->label->setText(text);
    setupDatabase();
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
    connect(listView, &QListView::doubleClicked, this, &chatwin::onItemClicked);
    QFont font = listView->font();
    font.setPointSize(10);
    listView->setFont(font);
    listView->setMaximumHeight(541);
    listView->setMaximumWidth(501);
}

void chatwin::showContextMenu(const QPoint &pos)
{
    QModelIndex index = listView->indexAt(pos);
    if (!index.isValid()) return;
    contextMenu = new QMenu(this);
    QAction *deleteAction = new QAction("Удалить сообщение", this);
    if (model->data(index, Qt::DecorationRole).isValid()) {
        connect(deleteAction, &QAction::triggered, this, &chatwin::deleteMessage);
        contextMenu->addAction(deleteAction);
    } else {
        connect(deleteAction, &QAction::triggered, this, &chatwin::deleteMessage);
        contextMenu->addAction(deleteAction);
        QString sender = model->data(index, Qt::UserRole).toString();
        if (sender != ui->label->text()) {
            QAction *editAction = new QAction("Изменить сообщение", this);
            connect(editAction, &QAction::triggered, this, &chatwin::editMessage);
            contextMenu->addAction(editAction);
        }
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
        saveMessageToDatabase(item);
        ui->msgline->setText("");
    }
}

void chatwin::loadMessagesFromDatabase()
{
    QSqlQuery query("SELECT sender, time, text, image FROM messages");
    while (query.next()) {
        QString sender = query.value(0).toString();
        QString time = query.value(1).toString();
        QString text = query.value(2).toString();
        QByteArray imageData = query.value(3).toByteArray();

        QStandardItem *item = new QStandardItem();
        item->setData(sender, Qt::UserRole);
        item->setData(time, Qt::UserRole + 1);

        if (!imageData.isEmpty()) {
            QPixmap pixmap;
            pixmap.loadFromData(imageData);
            QPixmap scaledPixmap = pixmap.scaled(250, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            item->setData(scaledPixmap, Qt::DecorationRole);
            item->setText(QString("%1, %2").arg(sender, time));
        } else {
            item->setText(QString("%1, %2\n%3").arg(sender, time, text));
        }

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
        saveMessageToDatabase(item);
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
               "text TEXT, "
               "image BLOB)");
}


void chatwin::messagesToDatabaseSave() {
    QSqlQuery query;
    query.exec("DELETE FROM messages");
    for (int i = 0; i < model->rowCount(); ++i) {
        QStandardItem *item = model->item(i);
        saveMessageToDatabase(item);
    }
}

void chatwin::onClosed() {
    if (model->rowCount()==0) {
        QSqlDatabase::database().close();
        QString filePath = QDir::currentPath()+"/"+ui->label->text()+".db";
        QFile::remove(filePath);
    } else messagesToDatabaseSave();
}

void chatwin::on_picbutton_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Выбрать изображение"), "", tr("Images (*.png *.xpm *.jpg)"));
    if (!filePath.isEmpty()) {
        QPixmap pixmap(filePath);
        if (!pixmap.isNull()) {
            QPixmap scaledPixmap = pixmap.scaled(250, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            QStandardItem *item = new QStandardItem();
            item->setData(QVariant(scaledPixmap), Qt::DecorationRole);
            QString currentTime = QDateTime::currentDateTime().toString("hh:mm");
            QString sender = "You";
            item->setData(sender, Qt::UserRole);
            item->setData(currentTime, Qt::UserRole + 1);
            item->setText(QString("%1, %2").arg(sender, currentTime));
            model->appendRow(item);
        }
    }
}

void chatwin::saveMessageToDatabase(const QStandardItem *item)
{
    QString sender = item->data(Qt::UserRole).toString();
    QString time = item->data(Qt::UserRole + 1).toString();
    QVariant decoration = item->data(Qt::DecorationRole);

    QSqlQuery query;
    if (decoration.isValid()) {
        QPixmap pixmap = decoration.value<QPixmap>();
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        pixmap.save(&buffer, "PNG");

        query.prepare("INSERT INTO messages (sender, time, image) VALUES (:sender, :time, :image)");
        query.bindValue(":sender", sender);
        query.bindValue(":time", time);
        query.bindValue(":image", byteArray);
    } else {
        QString text = item->text().split('\n').last();
        query.prepare("INSERT INTO messages (sender, time, text) VALUES (:sender, :time, :text)");
        query.bindValue(":sender", sender);
        query.bindValue(":time", time);
        query.bindValue(":text", text);
    }

    if (!query.exec()) {
        qDebug() << "save message error" << query.lastError().text();
    }
}


void chatwin::onItemClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;
    QVariant decoration = model->data(index, Qt::DecorationRole);
    if (decoration.isValid()) {
        QPixmap pixmap = decoration.value<QPixmap>();
        if (!pixmap.isNull()) {
            QLabel *imageLabel = new QLabel;
            pixmap = pixmap.scaled(500, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            imageLabel->setPixmap(pixmap);
            imageLabel->setWindowFlags(Qt::Window);
            imageLabel->show();
        }
    }
}
