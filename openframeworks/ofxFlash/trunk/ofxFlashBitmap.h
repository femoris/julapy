/*
 *  ofxFlashBitmap.h
 *  emptyExample
 *
 *  Created by lukasz karluk on 1/11/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once

#include "ofxFlashDisplayObject.h"

class ofxFlashBitmap : public ofxFlashDisplayObject
{
	
public:
	
	 ofxFlashBitmap( ofImage* image );
	~ofxFlashBitmap();
	
	//-------------------------------------------
	
	ofImage*	bitmapImage;
	
	//-------------------------------------------
	
	void setup		();
	void update		();
	void draw		();
	
private:
	
	void drawBitmap	();
	
};