/*
	SPDX-FileCopyrightText: 2006-2019 Graeme Gott <graeme@gottcode.org>

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "window.h"

#include "data.h"
#include "graph.h"
#include "novel_dialog.h"

#include <QAction>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QSettings>
#include <QSpinBox>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

//-----------------------------------------------------------------------------

Window::Window()
{
	setContextMenuPolicy(Qt::NoContextMenu);

	if (iconSize().width() == 26) {
		setIconSize(QSize(24,24));
	}

	QWidget* contents = new QWidget(this);
	setCentralWidget(contents);

	m_data = new Database(this);
	m_novels = new QComboBox(this);
	m_novels->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
#if (QT_VERSION >= QT_VERSION_CHECK(5,14,0))
	connect(m_novels, &QComboBox::textActivated, this, &Window::load);
#else
	connect(m_novels, qOverload<const QString&>(&QComboBox::activated), this, &Window::load);
#endif

	QTabWidget* graphs = new QTabWidget(this);

	QWidget* total_group = new QWidget(this);
	m_total_graph = new Graph(m_data, Database::Total, total_group);
	m_total_progress = new QProgressBar(total_group);
	graphs->addTab(total_group, tr("Total"));

	QWidget* daily_group = new QWidget(this);
	m_daily_graph = new Graph(m_data, Database::Daily, daily_group);
	m_daily_progress = new QProgressBar(daily_group);
	graphs->addTab(daily_group, tr("Daily"));

	m_wordcount = new QSpinBox(this);
	m_wordcount->setCorrectionMode(QSpinBox::CorrectToNearestValue);
	m_wordcount->setRange(0, 9999999);
	m_wordcount->setFocus();
	connect(m_wordcount, &QSpinBox::editingFinished, this, &Window::wordcountEdited);

	QLabel* wordcount_label = new QLabel(tr("Word count:"), this);

#ifndef Q_OS_MAC
	QToolBar* actions = new QToolBar(this);
	actions->setFloatable(false);
	actions->setMovable(false);
	actions->addWidget(m_novels);
	addToolBar(actions);

	QMenu* menu = new QMenu(this);
	menu->addAction(tr("&New Novel"), this, &Window::newNovel, QKeySequence::New);
	m_edit_button = menu->addAction(tr("&Edit Novel"), this, &Window::editNovel);
	m_delete_button = menu->addAction(tr("&Delete Novel"), this, &Window::deleteNovel);
	menu->addSeparator();
	menu->addAction(tr("&Quit"), this, &Window::close, QKeySequence::Quit);

	QToolButton* menubutton = new QToolButton(this);
	{
		QIcon fallback(":/application-menu/64.png");
		fallback.addFile(":/application-menu/48.png");
		fallback.addFile(":/application-menu/32.png");
		fallback.addFile(":/application-menu/24.png");
		fallback.addFile(":/application-menu/22.png");
		fallback.addFile(":/application-menu/16.png");
		menubutton->setIcon(QIcon::fromTheme("application-menu", fallback));
	}
	menubutton->setPopupMode(QToolButton::InstantPopup);
	menubutton->setMenu(menu);
	actions->addWidget(menubutton);
#else
	QMenu* menu = menuBar()->addMenu(tr("&Novel"));
	menu->addAction(tr("&New"), this, &Window::newNovel, QKeySequence::New);
	m_edit_button = menu->addAction(tr("&Edit"), this, &Window::editNovel);
	m_delete_button = menu->addAction(tr("&Delete"), this, &Window::deleteNovel);
	menu->addSeparator();
	QAction* quit = menu->addAction(tr("&Quit"), this, &Window::close, QKeySequence::Quit);
	quit->setMenuRole(QAction::QuitRole);
#endif

	QVBoxLayout* daily_layout = new QVBoxLayout(daily_group);
	daily_layout->addWidget(m_daily_graph);
	daily_layout->addWidget(m_daily_progress);

	QVBoxLayout* total_layout = new QVBoxLayout(total_group);
	total_layout->addWidget(m_total_graph);
	total_layout->addWidget(m_total_progress);

	QGridLayout* layout = new QGridLayout(contents);
	layout->setColumnStretch(0, 1);
	layout->setColumnStretch(1, 1);
#ifdef Q_OS_MAC
	layout->addWidget(m_novels, 0, 0, 1, 2);
#endif
	layout->addWidget(graphs, 1, 0, 1, 2);
	layout->addWidget(wordcount_label, 2, 0, Qt::AlignRight | Qt::AlignVCenter);
	layout->addWidget(m_wordcount, 2, 1, Qt::AlignLeft| Qt::AlignVCenter);

	restoreGeometry(QSettings().value("Geometry").toByteArray());

	reloadList();
	if (m_data->currentNovel().isEmpty()) {
		newNovel();
	}
	load(m_data->currentNovel());
}

//-----------------------------------------------------------------------------

void Window::closeEvent(QCloseEvent* event)
{
	QSettings().setValue("Geometry", saveGeometry());
	QWidget::closeEvent(event);
}

//-----------------------------------------------------------------------------

void Window::newNovel()
{
	NovelDialog add_dialog(QString(), m_data, this);
	if (add_dialog.exec() == QDialog::Accepted) {
		reloadList();
		load(m_data->currentNovel());
	}
}

//-----------------------------------------------------------------------------

void Window::editNovel()
{
	NovelDialog edit_dialog(m_data->currentNovel(), m_data, this);
	if (edit_dialog.exec() == QDialog::Accepted) {
		reloadList();
		novelModified();
	}
}

//-----------------------------------------------------------------------------

void Window::deleteNovel()
{
	QMessageBox mbox(window());
	mbox.setWindowTitle(tr("Delete novel?"));
	mbox.setText(tr("Delete progress for the novel '%1'?").arg(m_novels->currentText()));
	mbox.setInformativeText(tr("This action cannot be undone."));
	mbox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	mbox.setDefaultButton(QMessageBox::No);
	mbox.setIcon(QMessageBox::Warning);
	if (mbox.exec() == QMessageBox::Yes) {
		if (m_data->deleteNovel()) {
			QStringList list = m_data->novels();
			if (!list.isEmpty()) {
				m_data->setCurrentNovel(list.first());
			}
			reloadList();
			load(m_data->currentNovel());
		}
	}
}

//-----------------------------------------------------------------------------

void Window::load(const QString& novel)
{
	m_data->setCurrentNovel(novel);
	m_wordcount->setValue(m_data->currentValue(Database::Total));
	novelModified();
}

//-----------------------------------------------------------------------------

void Window::novelModified()
{
	m_total_graph->draw();
	m_daily_graph->draw();

	// Update total progressbar
	int goal = m_data->goal(Database::Total) - m_data->startValue();
	int value = std::max(0, m_data->currentValue(Database::Total) - m_data->startValue());
	m_total_progress->setMaximum(std::max(1, goal));
	m_total_progress->setValue(std::min(value, goal));

	// Update daily progressbar
	goal = m_data->goal(Database::Daily);
	value = m_data->currentValue(Database::Daily);
	m_daily_progress->setMaximum(std::max(1, goal));
	m_daily_progress->setValue(std::min(value, goal));
}

//-----------------------------------------------------------------------------

void Window::wordcountEdited()
{
	int value = m_wordcount->value();
	if (value != m_data->currentValue(Database::Total)) {
		m_data->setCurrentValue(value);
		novelModified();
	}
}

//-----------------------------------------------------------------------------

void Window::reloadList()
{
	m_novels->clear();
	m_novels->addItems(Database::novels());
	int index = m_novels->findText(m_data->currentNovel());
	bool found = (index != -1);
	m_edit_button->setEnabled(found);
	m_delete_button->setEnabled(found);
	m_wordcount->setEnabled(found);
	if (found) {
		m_novels->setCurrentIndex(index);
	}
}

//-----------------------------------------------------------------------------
