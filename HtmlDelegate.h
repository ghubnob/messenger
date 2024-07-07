#ifndef HTMLDELEGATE_H
#define HTMLDELEGATE_H

#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QPainter>

class HtmlDelegate : public QStyledItemDelegate {
public:
    HtmlDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        if (index.isValid()) {
            QTextDocument doc;
            doc.setHtml(index.data().toString());
            painter->save();

            painter->translate(option.rect.topLeft());
            doc.drawContents(painter);

            painter->restore();
        } else {
            QStyledItemDelegate::paint(painter, option, index);
        }
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        if (index.isValid()) {
            QTextDocument doc;
            doc.setHtml(index.data().toString());
            return QSize(doc.idealWidth(), doc.size().height());
        } else {
            return QStyledItemDelegate::sizeHint(option, index);
        }
    }
};

#endif // HTMLDELEGATE_H
