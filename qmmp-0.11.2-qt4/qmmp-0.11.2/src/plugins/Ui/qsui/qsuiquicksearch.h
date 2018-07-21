/***************************************************************************
 *   Copyright (C) 2017 by Ilya Kotov                                      *
 *   forkotov02@ya.ru                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef QSUIQUICKSEARCH_H
#define QSUIQUICKSEARCH_H

#include <QWidget>
#include <QList>

class QLineEdit;
class PlayListManager;
class ListWidget;
class QToolButton;

/**
    @author Ilya Kotov <forkotov02@ya.ru>
*/
class QSUIQuickSearch : public QWidget
{
    Q_OBJECT
public:
    explicit QSUIQuickSearch(ListWidget *listWidget, QWidget *parent = 0);

private slots:
    void onTextChanged(const QString &str);

private:
    QLineEdit *m_lineEdit;
    PlayListManager *m_manager;
    ListWidget *m_listWidget;
    QToolButton *m_clearButton;

};

#endif // QSUIQUICKSEARCH_H