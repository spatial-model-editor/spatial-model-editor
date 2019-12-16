#include "guiutils.hpp"

#include <QListWidget>
#include <QMessageBox>
#include <QString>
#include <QTreeWidget>
#include <QWidget>

QMessageBox *newYesNoMessageBox(const QString &title, const QString &text,
                                QWidget *parent) {
  auto *msgbox = new QMessageBox(parent);
  msgbox->setIcon(QMessageBox::Question);
  msgbox->setWindowTitle(title);
  msgbox->setText(text);
  msgbox->addButton(QMessageBox::Yes);
  msgbox->addButton(QMessageBox::No);
  msgbox->setDefaultButton(QMessageBox::Yes);
  msgbox->setAttribute(Qt::WA_DeleteOnClose);
  return msgbox;
}

void selectMatchingOrFirstItem(QListWidget *list, const QString &text) {
  if (list->count() == 0) {
    return;
  }
  if (auto l = list->findItems(text, Qt::MatchExactly); !l.isEmpty()) {
    list->setCurrentItem(l[0]);
  } else {
    list->setCurrentRow(0);
  }
}

void selectFirstChild(QTreeWidget *tree) {
  for (int i = 0; i < tree->topLevelItemCount(); ++i) {
    if (const auto *p = tree->topLevelItem(i);
        (p != nullptr) && (p->childCount() > 0)) {
      tree->setCurrentItem(p->child(0));
      return;
    }
  }
}

void selectMatchingOrFirstChild(QTreeWidget *list, const QString &text) {
  // look for a child widget with the desired text
  if (!text.isEmpty()) {
    for (auto *item :
         list->findItems(text, Qt::MatchExactly | Qt::MatchRecursive, 0)) {
      if (item->parent() != nullptr) {
        list->setCurrentItem(item);
        return;
      }
    }
  }
  // if we didn't find a match, then just select the first child item
  selectFirstChild(list);
}
