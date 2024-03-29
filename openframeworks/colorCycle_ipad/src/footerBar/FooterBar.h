/*
 *  FooterBar.h
 *  iPhoneEmptyExample
 *
 *  Created by Lukasz Karluk on 19/08/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once

#include "ofMain.h"
#include "FooterBarViewController.h"

class FooterBar
{

public:
	
	 FooterBar()	{};
	~FooterBar()	{};
	
	//----------------------------------------
	
	FooterBarViewController*	controller;
	
	//----------------------------------------
	
	void setup ()
	{
		CGRect appRect = [ [ UIScreen mainScreen ] applicationFrame ];
		
		controller = [ [ FooterBarViewController alloc ] init ];
		controller.view.frame = appRect;
		
		UIWindow* window = ofxiPhoneGetUIWindow();
		[ window addSubview: controller.view ];
//		[ controller.view release ];
		
//		ofAddListener( ofEvents.update, this, &FooterBar :: update );
	};
	
	void show ()
	{
		[ controller show ];
	};
	
	void hide ()
	{
		[ controller hide ];
	};
	
	void toggleShow ()
	{
		[ controller toggleShow ];
	};
	
	//----------------------------------------

	bool isShowing ()
	{
		return [ controller isShowing ];
	}
	
	bool isShuffleSelected ()
	{
		return [ controller isShuffleSelected ];
	}
	
	bool isColorSelected ()
	{
		return [ controller isColorSelected ];
	}

	bool isAddSelected ()
	{
		return [ controller isAddSelected ];
	}

	bool isRemoveSelected ()
	{
		return [ controller isRemoveSelected ];
	}
	
	bool isInfoSelected ()
	{
		return [ controller isInfoSelected ];
	}
	
	bool isHideSelected ()
	{
		return [ controller isHideSelected ];
	}
	
	ofPoint getHidePoint ()
	{
		CGPoint hidePoint = [ controller getHidePoint ];
		
		ofPoint p;
		p.x = hidePoint.x;
		p.y = hidePoint.y;
		return p;
	}
	
	bool isPhotoSaved ()
	{
		return [ controller isPhotoSaved ];
	}
	
private:
	
	void update ( ofEventArgs &e )
	{
		// checks.
	};
	
};