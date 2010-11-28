/************************************************************************
 *
 * Copyright (C) 2006, 2007, 2008, 2010 Graeme Gott <graeme@gottcode.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ***********************************************************************/

#include "graph.h"

#include "data.h"

#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QStyle>

#include <cmath>

//-----------------------------------------------------------------------------

Bar::Bar(int x, int y, int w, int h, int value, const QDate& day, const QColor& color) :
	QGraphicsRectItem(x, y, w, h)
{
	setPen(Qt::NoPen);
	setBrush(color);
	setToolTip(tr("%1\n%L2 words").arg(day.toString("MMM d")).arg(value));
#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
	setAcceptHoverEvents(true);
#else
	setAcceptsHoverEvents(true);
#endif
	setZValue(1);
}

//-----------------------------------------------------------------------------

void Bar::hoverEnterEvent(QGraphicsSceneHoverEvent* e)
{
	setOpacity(0.5);
	QGraphicsRectItem::hoverEnterEvent(e);
}

//-----------------------------------------------------------------------------

void Bar::hoverLeaveEvent(QGraphicsSceneHoverEvent* e)
{
	setOpacity(1.0);
	QGraphicsRectItem::hoverLeaveEvent(e);
}

//-----------------------------------------------------------------------------

Graph::Graph(QWidget* parent, Database* data) :
	QGraphicsView(parent),
	m_data(data)
{
	m_scene = new QGraphicsScene;
	m_scene->setBackgroundBrush(Qt::white);
	setScene(m_scene);
	setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
}

//-----------------------------------------------------------------------------

void Graph::draw()
{
	// Remove all current items
	m_scene->clear();
	if (m_data->currentNovel().isEmpty()) {
		return;
	}

	// Detemine height of scene
	int rows = qMax(m_data->currentValue(Database::Total), m_data->finalGoal()) / 10000;
	int goal_row = m_data->finalGoal() / 10000;
	int columns = m_data->startDate().daysTo(m_data->endDate()) + 1;
	int graph_height = (rows + 1) * 25;
	int graph_width = (columns * 9) + 14;
	m_scene->setSceneRect(0, 0, graph_width, graph_height);

	// Draw baseline
	QPen pen(QColor(102, 102, 102), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
	m_scene->addLine(0, graph_height + 0.5, graph_width, graph_height + 0.5, pen);

	// Draw ticks
	for (int c = 0; c < columns; ++c) {
		int h = (c % 7 == 0) ? 5 : 2;
		int x = (c * 9) + 12;
		m_scene->addLine(x + 0.5, graph_height + 0.5, x + 0.5, graph_height + h + 0.5, pen);
	}

	// Draw lines
	pen.setColor(QColor(204, 204, 204));
	pen.setStyle(Qt::DotLine);
	for (int i = 1; i <= rows; ++i) {
		if (i > goal_row) {
			pen.setColor(QColor(153, 204, 255));
		}
		int y = graph_height - (i * 25);
		m_scene->addLine(0, y + 0.5, graph_width, y + 0.5, pen);
	}

	// Add line labels
	QFont label_font;
	label_font.setFamily("Serif");
	label_font.setPointSize(7);
	for (int i = 1; i <= rows; ++i) {
		QGraphicsTextItem* text = new QGraphicsTextItem(QString("%L1").arg(i * 10000));
		if (i <= goal_row) {
			text->setDefaultTextColor(QColor(102, 102, 102));
		} else {
			text->setDefaultTextColor(QColor(153, 204, 255));
		}
		text->setFont(label_font);
		QRectF bound = text->boundingRect();
		int h = static_cast<int>(bound.height() * 0.5f);
		int w = std::ceil(bound.width());
		if ((graph_width + w) > m_scene->width()) {
			m_scene->setSceneRect(0, 0, graph_width + w, m_scene->height());
		}
		text->setPos(graph_width, graph_height - (i * 25) - h);
		m_scene->addItem(text);
	}

	// Draw bars
	int prev_value = 0, value, minimum, x, h;
	QColor color;
	QDate day = m_data->startDate();
	for (int c = 0; c < columns; ++c) {
		value = m_data->value(day, Database::Total);
		minimum = m_data->minimumValue(day, Database::Total);
		h = value / 400;
		x = (c * 9) + 8;
		if (value < prev_value) {
			color.setRgb(255, 0, 0);
		} else if (value >= m_data->finalGoal()) {
			color.setRgb(153, 204, 255);
		} else if (value >= minimum) {
			color.setRgb(0, 140, 0);
		} else {
			color.setRgb(255, 170, 0);
		}
		prev_value = value;
		m_scene->addItem(new Bar(x, graph_height - h, 8, h, value, day, color));

		h = minimum / 400;
		QGraphicsRectItem* rect = m_scene->addRect(x, graph_height - h, 8, h, Qt::NoPen, Qt::black);
		rect->setOpacity(0.15);
		rect->setZValue(2);

		day = day.addDays(1);
	}

	// Add bar labels
	day = m_data->startDate();
	int length = day.daysTo(m_data->endDate()) / 7;
	for (int i = 0; i <= length; ++i) {
		QGraphicsTextItem* text = new QGraphicsTextItem(day.toString("MMM d"));
		text->setDefaultTextColor(QColor(102, 102, 102));
		text->setFont(label_font);
		int h = std::ceil(text->boundingRect().height());
		if ((graph_height + h + 4) > m_scene->height()) {
			m_scene->setSceneRect(0, 0, m_scene->width(), graph_height + h + 4);
		}
		text->setPos(i * 63, graph_height + 4);
		m_scene->addItem(text);
		day = day.addDays(7);
	}
}

//-----------------------------------------------------------------------------
