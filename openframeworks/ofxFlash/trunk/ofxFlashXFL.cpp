/*
 *  ofxFlashFXL.cpp
 *  emptyExample
 *
 *  Created by lukasz karluk on 3/11/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "ofxFlashXFL.h"

struct DOMBitmapItem
{
	string	name;
	string	itemID;
	string	sourceExternalFilepath;
	int		sourceLastImported;
	string	sourcePlatform;
	int		externalFileSize;
	string	originalCompressionType;
	int		quality;
	string	href;
	string	bitmapDataHRef;
	int		frameRight;
	int		frameBottom;
};

///////////////////////////////////////////
//	
///////////////////////////////////////////

ofxFlashXFL :: ofxFlashXFL()
{
	//
}

ofxFlashXFL :: ~ofxFlashXFL()
{
	//
}

///////////////////////////////////////////
//	
///////////////////////////////////////////


vector<DOMBitmapItem>	domBitmapItems;


///////////////////////////////////////////
//	LOAD XFL.
///////////////////////////////////////////

void ofxFlashXFL :: loadXFL ( string pathToXFL )
{
	this->pathToXFL = pathToXFL;
	
	xml.loadFile( pathToXFL + "DOMDocument.xml" );
	
	loadXFLMedia();
}

void ofxFlashXFL :: loadXFLMedia ()
{
	xml.pushTag( "DOMDocument", 0 );
	xml.pushTag( "media", 0 );
	
	int numOfMediaTags;
	numOfMediaTags = xml.getNumTags( "DOMBitmapItem" );
	
	for( int i=0; i<numOfMediaTags; i++ )
	{
		DOMBitmapItem item;
		item.name						= xml.getAttribute( "DOMBitmapItem", "name",					"", i );
		item.itemID						= xml.getAttribute( "DOMBitmapItem", "itemID",					"", i );
		item.sourceExternalFilepath		= xml.getAttribute( "DOMBitmapItem", "sourceExternalFilepath",	"", i );
		item.sourceLastImported			= xml.getAttribute( "DOMBitmapItem", "sourceLastImported",		0,  i );
		item.sourcePlatform				= xml.getAttribute( "DOMBitmapItem", "sourcePlatform",			"", i );
		item.externalFileSize			= xml.getAttribute( "DOMBitmapItem", "externalFileSize",		0,  i );
		item.originalCompressionType	= xml.getAttribute( "DOMBitmapItem", "originalCompressionType",	"", i );
		item.quality					= xml.getAttribute( "DOMBitmapItem", "quality",					0,  i );
		item.href						= xml.getAttribute( "DOMBitmapItem", "href",					"", i );
		item.bitmapDataHRef				= xml.getAttribute( "DOMBitmapItem", "bitmapDataHRef",			"", i );
		item.frameRight					= xml.getAttribute( "DOMBitmapItem", "frameRight",				0,  i );
		item.frameBottom				= xml.getAttribute( "DOMBitmapItem", "frameBottom",				0,  i );
		
		domBitmapItems.push_back( item );
	}
}

///////////////////////////////////////////
//	LOAD ASSETS.
///////////////////////////////////////////

void ofxFlashXFL :: loadAssets ()
{
	ofxFlashLibrary* library;
	library = ofxFlashLibrary :: getInstance();
	
	for( int i=0; i<domBitmapItems.size(); i++ )
	{
		const DOMBitmapItem& item = domBitmapItems[ i ];
		int mediaType	= determineMediaType( item.sourceExternalFilepath );
		string path		= pathToXFL + item.sourceExternalFilepath;
		
		if( mediaType == OFX_FLASH_LIBRARY_TYPE_IMAGE )
		{
			library->addImage( item.name, path );
		}
		else if( mediaType == OFX_FLASH_LIBRARY_TYPE_VIDEO )
		{
			library->addVideo( item.name, path );
		}
		else if( mediaType == OFX_FLASH_LIBRARY_TYPE_SOUND )
		{
			library->addSound( item.name, path );
		}
	}
}

int ofxFlashXFL :: determineMediaType ( string fileName )
{
	return OFX_FLASH_LIBRARY_TYPE_IMAGE;
}

///////////////////////////////////////////
//	BUILD.
///////////////////////////////////////////

void ofxFlashXFL :: build ()
{
	ofxFlashStage* stage;
	stage = ofxFlashStage :: getInstance();
	
	//
}