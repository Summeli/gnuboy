/* gnuboy4cute
 *
 * Copyright (C) 2010 Summeli <summeli@summeli.fi>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "dpadwidget.h"
#include "cuteDebug.h"

#include <QMouseEvent>
#include <qmath.h>

#define KCenter_x 90
#define KCenter_y 270

#include "input.h"
#include "keypad.h"

DPadWidget::DPadWidget(QWidget *parent)
    : QWidget(parent)
{
	__DEBUG_IN
	ui.setupUi(this);
	prevkeys = 0;
	__DEBUG_OUT
}

DPadWidget::~DPadWidget()
{

}


void DPadWidget::mousePressEvent(QMouseEvent* event )
	{
	__DEBUG_IN
	processbuttons( event );
	__DEBUG_OUT
	}

void DPadWidget::mouseReleaseEvent(QMouseEvent* event )
	{
	__DEBUG_IN
	emit(virtualKeyEvent(prevkeys,false));
	prevkeys = 0;
	__DEBUG_OUT
	}

void DPadWidget::mouseMoveEvent(QMouseEvent* event)
	{
	__DEBUG_IN
	processbuttons( event );
	__DEBUG_OUT
	}

void DPadWidget::processbuttons( QMouseEvent* event )
	{

	quint32 key = getgpKeys( event );
	if( key != prevkeys )
		{
		//release old keys
		quint32 release = prevkeys;
		release &= ~(key & prevkeys);
		emit(virtualKeyEvent(release,false));
		
		//send new key
		quint32 newkey = key;
		newkey &=  ~(key & prevkeys);
		emit(virtualKeyEvent(newkey,true));
		}
	prevkeys = key;

	}

quint32 DPadWidget::getgpKeys( QMouseEvent* event )
	{
	quint32 key = 0;
	quint32 ycoord = event->y();
	if(  ycoord <= 90 )
		{
		//select / start was pressed
		quint32 xcoord = event->x();
		if( xcoord <= 90 )
			{
			//select was pressed
			key = BIT_SEL;
			}
		else
			{
			//start was pressed
			key = BIT_ST;
			}
		}
	else
		{
		//handle the keypress as dpad
		qreal x = event->x() - KCenter_x;
		qreal y = event->y() - KCenter_y;
		
		qreal r = qAtan2(y,x);
		
		r = (r * 180 )/ KPi; //convert radians to degrees
	
		//lets use full circle instead of negative angles
		if (r < 0)
			{
			r = 360 + r;
			}
		
		qint32 angle = qRound(r);
	
	
		//360 degrees is divided into 8 sectors.
		if (angle > 337 || angle < 23)
			{
			//right key was pressed
			key += BIT_R;
			}
		else if (angle >= 23 && angle < 68)
			{
			//right and down was pressed
			key += BIT_R;
			key += BIT_D;
			}
		else if (angle >= 68 && angle < 113)
			{
			//Down key was pressed
			key += BIT_D;
			}
		else if (angle >= 113 && angle < 158)
			{
			//Down and left key was pressed
			key += BIT_D;
			key += BIT_L;
			}
		else if (angle >= 158 && angle < 203)
			{
			//Left key was pressed
			key += BIT_L;
			}
		else if (angle >= 203 && angle < 248)
			{
			//left and up key was pressed
			key += BIT_L;
			key += BIT_U;
			}
		else if (angle >= 248 && angle < 293)
			{
			//up key was BUTTON_UP
			key += BIT_U;
			}
		else if (angle >= 293 && angle <= 337)
			{
			//up and right key was pressed
			key += BIT_U;
			key += BIT_R;
			}
		}
		return key;
	}

