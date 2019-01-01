#pragma once
#include <QPainter>
#include <QTextDocument>
#include <QStyledItemDelegate>
class HTMLDelegate : public QStyledItemDelegate {
protected:
  void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
  QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const;
};
