#ifndef CHATWIN_H
#define CHATWIN_H

#include <QDialog>
#include <QListView>
#include <QStandardItemModel>
#include <QSqlDatabase>

namespace Ui {
class chatwin;
}

class chatwin : public QDialog
{
    Q_OBJECT

public:
    explicit chatwin(QWidget *parent = nullptr);
    ~chatwin();

    void changeChatLabel(QString &text);
    void changeNameLabel(QString &name);
    void onClosed();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void on_pushButton_clicked();
    void showContextMenu(const QPoint &pos);
    void deleteMessage();
    void editMessage();
    void onItemClicked(const QModelIndex &index);
    void on_testbutton_sender_clicked();
    void on_picbutton_clicked();

private:
    Ui::chatwin *ui;
    QListView *listView;
    QStandardItemModel *model;
    QMenu *contextMenu;
    QString formatMessage(const QString &sender, const QString &time, const QString &text);
    QSqlDatabase db;

    void setupUi();
    void setupListView();
    void saveMessageToDatabase(const QStandardItem *item);
    void messagesToDatabaseSave();
    void loadMessagesFromDatabase();
    void setupDatabase();

    QByteArray encryptMessage(const QString &message, const QByteArray &key);
    QString decryptMessage(const QByteArray &encryptedMessage, const QByteArray &key);
    QByteArray generateKey(const QString &userKey);
};

#endif // CHATWIN_H
