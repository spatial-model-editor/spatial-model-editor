#include "guiutils.hpp"

#include <QFileDialog>
#include <QInputDialog>
#include <QListWidget>
#include <QMessageBox>
#include <QString>
#include <QTreeWidget>
#include <QWidget>

#include "logger.hpp"
#include "tiff.hpp"

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

QImage getImageFromUser(QWidget *parent, const QString &title) {
  QImage img;
  QString filename = QFileDialog::getOpenFileName(
      parent, title, "",
      "Image Files (*.tif *.tiff *.gif *.jpg *.jpeg *.png *.bmp);; All files "
      "(*.*)",
      nullptr, QFileDialog::Option::DontUseNativeDialog);
  if (filename.isEmpty()) {
    return img;
  }
  SPDLOG_DEBUG("  - import file {}", filename.toStdString());
  // first try using tiffReader
  utils::TiffReader tiffReader(filename.toStdString());
  if (tiffReader.size() == 0) {
    SPDLOG_DEBUG(
        "    -> tiffReader could not read file, trying QImage::load()");
    bool success = img.load(filename);
    if (!success) {
      SPDLOG_DEBUG("    -> QImage::load() could not read file - giving up");
      QMessageBox::warning(
          parent, "Could not open image file",
          QString("Failed to open image file '%1'\nError message: '%2'")
              .arg(filename)
              .arg(tiffReader.getErrorMessage()));
    }
  } else if (tiffReader.size() == 1) {
    SPDLOG_DEBUG("    -> tiffReader read single image file successfully");
    img = tiffReader.getImage();
  } else {
    bool ok;
    int i = QInputDialog::getInt(
        parent, "Import tiff image",
        "Please choose the page to use from this multi-page tiff", 0, 0,
        static_cast<int>(tiffReader.size()) - 1, 1, &ok);
    if (ok) {
      SPDLOG_DEBUG("    -> tiffReader read image {}/{} from file successfully",
                   i + 1, tiffReader.size());
      img = tiffReader.getImage(static_cast<std::size_t>(i));
    }
  }
  return img;
}
