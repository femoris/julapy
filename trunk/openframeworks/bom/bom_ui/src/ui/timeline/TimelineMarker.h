/*
 *  TimelineMarker.h
 *  emptyExample
 *
 *  Created by lukasz karluk on 21/10/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once

#include "ofMain.h"
#include "ofxFlash.h"

#include "EventDataItem.h"

#include "Btn.h"

class TimelineMarker : public ofxSprite
{
	
public:
	
	TimelineMarker()
	{
		setup();
	}
	
	~TimelineMarker() 
	{
		//
	};
	
	//==================================================
	
	EventDataItem	data;
	
	ofImage*		markerRain;
	ofImage*		markerTemp;
	ofPoint			offset;
	
	Btn				btn;
	
	int				tabType;
	
	ofEvent<int>	markerPressEvent;
	
	//==================================================

	void populate ( EventDataItem data )
	{
		this->data = data;
	}
	
	void setup()
	{
		markerRain = (ofImage*)ofxAssets :: getInstance()->getAsset( "timeline_bar_marker_rain" );
		markerTemp = (ofImage*)ofxAssets :: getInstance()->getAsset( "timeline_bar_marker_temp" );
		
		offset.x = -16;
		offset.y = -9;
		
		btn.setSize( 32, 53 );
		ofAddListener( btn.btnPressEvent, this, &TimelineMarker :: btnPressed );
		
		//-- model.
		
		Model* model;
		model = Model :: getInstance();
		
		tabType = model->getTabType();
		
		ofAddListener( model->tabTypeChangeEvent, this, &TimelineMarker :: tabTypeChanged );
	}
	
	void tabTypeChanged ( int& tabType )
	{
		this->tabType = tabType;
	}
	
	void setPos( int x, int y )
	{
		this->x = x + offset.x;
		this->y = y + offset.y;
		
		btn.setPos( this->x, this->y );
	}
	
	void update ()
	{
		//
	}
	
	void draw ()
	{
		ofSetColor( 0xFFFFFF );
		
		ofEnableAlphaBlending();
		
		if( tabType == TIMELINE_TAB_RAIN )
		{
			markerRain->draw( x, y );
		}
		else if( tabType == TIMELINE_TAB_TEMP )
		{
			markerTemp->draw( x, y );
		}
		
//		drawBounds();
		
		ofDisableAlphaBlending();
	}
	
	void drawBounds ()
	{
		ofNoFill();
		ofSetColor( 0x00FF00 );
		ofRect( btn.x, btn.y, btn.width, btn.height );
	}
	
	//==================================================
	
	void btnPressed ( int & btnId )
	{
		ofNotifyEvent( markerPressEvent, data.id, this );
	}
};