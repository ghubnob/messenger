#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListView>
#include <QStandardItemModel>
#include <QMenu>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();
    void deleteDialog();
    void openDialogFromList();
    void showContextMenu(const QPoint &pos);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    Ui::MainWindow *ui;
    QStandardItemModel *model;
    QMenu *contextMenu;
    QSqlDatabase db;

    void setupListView();
    void loadDialogs();
};

#endif // MAINWINDOW_H
