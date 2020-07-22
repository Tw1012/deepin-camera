/*
* Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     hujianbo <hujianbo@uniontech.com>
*
* Maintainer: hujianbo <hujianbo@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "thumbnailsbar.h"
#include "camview.h"
#include <sys/time.h>
#include <QCollator>
#include <DLabel>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <DDesktopServices>
#include <QDateTime>
#include <QApplication>
#include <QMimeData>
#include <QClipboard>
#include <QKeyEvent>
#include <QShortcut>
#include <QThread>

//QMap<QString, QPixmap> m_imagemap;
QMap<int, ImageItem *> g_indexImage;
int g_indexNow = 0;
QSet<int> g_setIndex;
extern QString g_strFileName;
extern bool g_bFoundDevice;

ThumbnailsBar::ThumbnailsBar(DWidget *parent) : DFloatingWidget(parent)
{
    //this->grabKeyboard(); //获取键盘事件的关键处理
    setFocus(Qt::OtherFocusReason);
    setFocusPolicy(Qt::StrongFocus);
    //this->setFramRadius(18);
    QShortcut *shortcut = new QShortcut(QKeySequence("ctrl+c"), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(onShortcutCopy()));
    QShortcut *shortcutDel = new QShortcut(QKeySequence(Qt::Key_Delete), this);
    connect(shortcutDel, SIGNAL(activated()), this, SLOT(onShortcutDel()));

    m_nStatus = STATNULL;
    m_nActTpye = ActTakePic;
    m_nItemCount = 0;
    m_nMaxItem = 0;
    m_hBOx = new QHBoxLayout();
    m_hBOx->setSpacing(ITEM_SPACE);
    m_mainLayout = new QHBoxLayout();
    m_mainLayout->setContentsMargins(0, 0, 0, 0);

    setBlurBackgroundEnabled(true); //设置磨砂效果

    m_mainLayout->setObjectName(QStringLiteral("horizontalLayout_4"));

    m_mainLayout->setObjectName(QStringLiteral("horizontalLayout_5"));

    m_mainLayout->addLayout(m_hBOx, Qt::AlignLeft);

    m_lastButton = new DPushButton(this);
//    m_lastButton->setStyleSheet("border:2px groove gray;border-radius:10px;padding:2px 4px;");
    //m_lastButton->setStyleSheet("border-radius:8px;");
    m_lastButton->setFixedWidth(LAST_BUTTON_WIDTH);
    m_lastButton->setFixedHeight(LAST_BUTTON_HEIGHT);
    QIcon iconPic(":/images/icons/light/button/photograph.svg");
    m_lastButton->setIcon(iconPic);
    m_lastButton->setIconSize(QSize(33, 33));

    DPalette pa = m_lastButton->palette();
    QColor clo("#0081FF");
    pa.setColor(DPalette::Dark, clo);
    pa.setColor(DPalette::Light, clo);
    m_lastButton->setPalette(pa);

    m_lastButton->setToolTip(tr("Take photo"));
    m_lastButton->setToolTipDuration(500);//0.5s消失

    connect(m_lastButton, SIGNAL(clicked()), this, SLOT(onBtnClick()));

    m_mainLayout->addWidget(m_lastButton, Qt::AlignRight);
    this->setLayout(m_mainLayout);

    this->setContextMenuPolicy(Qt::CustomContextMenu);
}

void ThumbnailsBar::setBtntooltip()
{
    m_lastButton->setToolTip(tr("Take photo"));
    m_lastButton->setToolTipDuration(500);
}

void ThumbnailsBar::onFoldersChanged(const QString &strDirectory)
{
    Q_UNUSED(strDirectory);
    m_nItemCount = 0;
    qDebug() << m_nMaxItem;
    int nLetAddCount = (m_nMaxItem - LAST_BUTTON_WIDTH - LAST_BUTTON_SPACE * 2) / (THUMBNAIL_WIDTH + 2) - 1;

    QLayoutItem *child;
    while ((child = m_hBOx->takeAt(0)) != nullptr) {
        ImageItem *tmp = dynamic_cast<ImageItem *>(child->widget());
        tmp->deleteLater();
        delete tmp;
        tmp = nullptr;
        //setParent为NULL，防止删除之后界面不消失
        if (child->widget()) {
            child->widget()->setParent(nullptr);
        }
    }
    g_indexImage.clear();
    //获取所选文件类型过滤器
    QStringList filters;
    filters << QString("*.jpg") << QString("*.mp4") << QString("*.webm");
    int tIndex = 0;
    QString strFolder;
    for (int i = m_strlstFolders.size(); i >= 1; i--) {
        strFolder = m_strlstFolders[i - 1];
        QDir dir(strFolder);
        //按时间逆序排序
        dir.setNameFilters(filters);
        dir.setSorting(QDir::Time /*| QDir::Reversed*/);
        if (dir.exists()) {
            //使用dir就不要使用迭代器了，会让dir的设置失效
            //QDirIterator dir_iterator(dir);
            QFileInfoList list = dir.entryInfoList();
            for (int i = 0; i < list.size(); ++i) {
                if (nLetAddCount <= m_nItemCount) {
                    break;
                }

                QString strFile = list.at(i).filePath();
                QFileInfo fileInfo = list.at(i);
                if (fileInfo.suffix() == "mp4" || fileInfo.suffix() == "webm") {
                    QString strFileName = fileInfo.fileName();
                    if (strFileName.compare(g_strFileName) == 0) {
                        continue; //mp4文件此时还不完整，读取generateThumbnail会崩溃
                    }
                }
                ImageItem *pLabel = new ImageItem(tIndex, fileInfo.filePath());
                g_indexImage.insert(tIndex, pLabel);
                tIndex++;

                m_hBOx->addWidget(pLabel);

                m_nItemCount++;
            }
        }
    }
    emit fitToolBar();
}

void ThumbnailsBar::onBtnClick()
{
    if (!g_bFoundDevice) {
        return;
    }
    if (m_nActTpye == ActTakePic) {
        if (m_nStatus == STATPicIng) {
            m_nStatus = STATNULL;
            m_lastButton->setToolTip(tr("Take photo"));
            m_lastButton->setToolTipDuration(500);
            emit enableTitleBar(3);
            emit takePic();
            emit enableSettings(true);
        } else {
            m_nStatus = STATPicIng;
            m_lastButton->setToolTip(tr("Stop Taking photo"));
            m_lastButton->setToolTipDuration(500);
            //1、标题栏视频按钮置灰不可选
            emit enableTitleBar(1);
            emit takePic();
            emit enableSettings(false);
        }

    } else if (m_nActTpye == ActTakeVideo) {
        if (m_nStatus == STATVdIng) {
            m_nStatus = STATNULL;
            emit enableTitleBar(4);
            emit enableSettings(true);
            emit takeVd();
        } else {
            m_nStatus = STATVdIng;
            this->hide();
            //1、标题栏拍照按钮置灰不可选
            emit enableTitleBar(2);
            //2、禁用设置功能
            emit enableSettings(false);
            //3、录制
            emit takeVd();
        }

    } else {
        return;
    }
}

void ThumbnailsBar::onShortcutCopy()
{
    QStringList paths;
    if (g_setIndex.isEmpty()) {
        paths = QStringList(g_indexImage.value(g_indexNow)->getPath());
        qDebug() << "sigle way";
    } else {
        QSet<int>::iterator it;
        for (it = g_setIndex.begin(); it != g_setIndex.end(); ++it) {
            paths << g_indexImage.value(*it)->getPath();
            qDebug() << g_indexImage.value(*it)->getPath();
        }
    }

    QClipboard *cb = qApp->clipboard();
    QMimeData *newMimeData = new QMimeData();
    QByteArray gnomeFormat = QByteArray("copy\n");
    QString text;
    QList<QUrl> dataUrls;
    for (QString path : paths) {
        if (!path.isEmpty())
            text += path + '\n';
        dataUrls << QUrl::fromLocalFile(path);

        gnomeFormat.append(QUrl::fromLocalFile(path).toEncoded()).append("\n");
    }

    newMimeData->setText(text.endsWith('\n') ? text.left(text.length() - 1) : text);
    newMimeData->setUrls(dataUrls);

    gnomeFormat.remove(gnomeFormat.length() - 1, 1);
    //本系统(UOS)特有
    newMimeData->setData("x-special/gnome-copied-files", gnomeFormat);
    cb->setMimeData(newMimeData, QClipboard::Clipboard);
}

void ThumbnailsBar::onShortcutDel()
{
    if (g_setIndex.isEmpty()) {
        if (g_indexImage.size() <= 0) {
            return;
        }
        DDesktopServices::trash(g_indexImage.value(g_indexNow)->getPath());
        g_indexNow = 0;//如果需要，可以通过+1的方式往后挪，超出范围就变为0
    } else {
        if (g_setIndex.size() <= 0) {
            return;
        }
        QSet<int>::iterator it;
        for (it = g_setIndex.begin(); it != g_setIndex.end(); ++it) {
            DDesktopServices::trash(g_indexImage.value(*it)->getPath());
            g_indexImage.remove(*it);
        }
        g_setIndex.clear();
        g_indexNow = 0;
    }
}

void ThumbnailsBar::ChangeActType(int nType)
{
    if (m_nActTpye == nType) {
        return;
    }
    m_nActTpye = nType;
    if (nType == ActTakePic) {
        QIcon iconPic(":/images/icons/light/button/photograph.svg");
        m_lastButton->setIcon(iconPic);
        m_lastButton->setIconSize(QSize(33, 33));

        DPalette pa = m_lastButton->palette();
        QColor clo("#0081FF");
        pa.setColor(DPalette::Dark, clo);
        pa.setColor(DPalette::Light, clo);
        m_lastButton->setPalette(pa);

        m_lastButton->setToolTip(tr("Take photo"));
    } else if (nType == ActTakeVideo) {
        QIcon iconPic(":/images/icons/light/button/transcribe.svg");
        m_lastButton->setIcon(iconPic);
        m_lastButton->setIconSize(QSize(33, 33));

        DPalette pa = m_lastButton->palette();
        QColor clo("#FF0000");
        pa.setColor(DPalette::Dark, clo);
        pa.setColor(DPalette::Light, clo);
        m_lastButton->setPalette(pa);

        m_lastButton->setToolTip(tr("Record video"));
    } else {
        return;
    }

}

void ThumbnailsBar::addPath(QString strPath)
{
    if (!m_strlstFolders.contains(strPath)) {
        m_strlstFolders.push_back(strPath);
        onFoldersChanged("");
    }
}

void ThumbnailsBar::mousePressEvent(QMouseEvent *ev) //点击空白处的处理
{
    Q_UNUSED(ev);
}
