/*
 *  RibbonType.cpp
 *  openFrameworks
 *
 *  Created by lukasz karluk on 30/06/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "RibbonType.h"

RibbonType :: RibbonType()
{
	initCharacters();
	
	setKerning( 1.0 );
	wrapRibbonSurface( false );
}

RibbonType :: ~RibbonType()
{
	//
}

void RibbonType :: initCharacters()
{
	string supportedCharacter = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	
	charactersTotal	= supportedCharacter.size();
	characters		= new char[ supportedCharacter.size() + 1 ];
	strcpy( characters, supportedCharacter.c_str() );
}

////////////////////////////////////////////////////////////
//	LOAD / PREP FONT.
////////////////////////////////////////////////////////////

void RibbonType :: loadTrueTypeFont( string fontName, int size )
{
	fontSize = size;
	font.loadFont( fontName, fontSize, true, true, true );
	
	for( int i=0; i<charactersTotal; i++ )
	{
		characterContours.push_back( font.getCharacterAsPoints( characters[ i ] ) );
		
		string str = "";
		str += characters[ i ];
		ofRectangle rect = font.getStringBoundingBox( str, 0, 0 );
		characterRectangles.push_back( CharacterRect() );
		characterRectangles.back().width	= rect.width;
		characterRectangles.back().height	= rect.height;
	}
}

void RibbonType :: setRibbinColorArray( float *ribbonColorArray )
{
	ribbonColors		= ribbonColorArray;
	useRibbonColorArray	= true;
}

void RibbonType :: drawTypeOnRibbon( string copy, float *ribbonPositionArray, float *ribbonDirectionArray, int ribbonSize )
{
	ribbonPositions		= ribbonPositionArray;
	ribbonDirections	= ribbonDirectionArray;
	ribbonLength		= ribbonSize;
	ribbonLengths		= new float[ ribbonLength ];
	contourStartIndex	= 0;
	
	calcRibbonLengths();
	
	char *cstr;
	cstr = new char[ copy.size() + 1 ];
	strcpy( cstr, copy.c_str() );
	
	float x = 0;
	
	for( int i=0; i<copy.size(); i++ )
	{
		if( wrapSurface )
		{
			drawLetterWrap( cstr[ i ], x, fontSize * 0.5 );
		}
		else
		{
			drawLetterAsPlane( cstr[ i ], x, fontSize * 0.5 );
		}
		
		if( cstr[ i ] == ' ' )
		{
			x += fontSize;
		}
		else
		{
			int characterIndex = getCharacterIndex( cstr[ i ] );
			if( characterIndex > -1 )
			{
				CharacterRect rect;
				rect = characterRectangles.at( characterIndex );
				x += rect.width + kerning;
			}
		}
	}
	
	useRibbonColorArray = false;
	
	delete[] cstr;
	delete[] ribbonLengths;
}

////////////////////////////////////////////////////////////
//	LIFE HACKER METHODS - MAKING LIFE EASIER.
////////////////////////////////////////////////////////////

void RibbonType :: calcRibbonLengths()
{
	for( int i=0; i<ribbonLength; i++ )
	{
		int k = i * 3;
		
		if( i == 0 )
		{
			ribbonLengths[ i ] = 0;
		}
		else
		{
			ofxVec3f p1		= ofxVec3f( ribbonPositions[ k - 3 ], ribbonPositions[ k - 2 ], ribbonPositions[ k - 1 ] );
			ofxVec3f p2		= ofxVec3f( ribbonPositions[ k + 0 ], ribbonPositions[ k + 1 ], ribbonPositions[ k + 2 ] );
			ofxVec3f p21	= p2 - p1;
			
			ribbonLengths[ i ] = ribbonLengths[ i - 1 ] + p21.length();
		}
	}
}

int RibbonType :: getCharacterIndex ( int letter )
{
	for( int i=0; i<charactersTotal; i++ )
	{
		if( characters[ i ] == letter )
		{
			return i;
		}
	}
	return -1;
}

////////////////////////////////////////////////////////////
//	DRAWING ROUTINE.
////////////////////////////////////////////////////////////

void RibbonType :: drawLetterWrap( int letter, float xOffset, float yOffset )
{
	int characterIndex = getCharacterIndex( letter );
	if( characterIndex == -1 )
	{
		return;
	}
	
	ofTTFCharacter ttfChar;
	ttfChar = characterContours.at( characterIndex );
	
	bool outsideOfRibbon = false;
	int contourMaxIndex = contourStartIndex;
	
	ofBeginShape();
	
	for( int k=0; k<ttfChar.contours.size(); k++ )
	{
		if( k != 0 )
		{
			ofNextContour(true);
		}
		
		for( int i=0; i<ttfChar.contours[ k ].pts.size(); i++ )
		{
			float cx = ttfChar.contours[ k ].pts[ i ].x + xOffset;
			float cy = ttfChar.contours[ k ].pts[ i ].y + yOffset;
			
			ofxVec3f cp;	// contour point.
			ofxVec3f p1;	// current ribbon point position.
			ofxVec3f p2;	// next ribbon point position.
			ofxVec3f p21;	// direction from p1 to p2;
			
			int j;
			for( j=contourStartIndex; j<( ribbonLength - 1 ) * 3; j+=3 )
			{
				p1	= ofxVec3f( ribbonPositions[ j + 0 ], ribbonPositions[ j + 1 ], ribbonPositions[ j + 2 ] );
				p2	= ofxVec3f( ribbonPositions[ j + 3 ], ribbonPositions[ j + 4 ], ribbonPositions[ j + 5 ] );
				p21	= p2 - p1;
				
				int ribbonLengthIndex = (int)( j / 3 );
				float lx = ribbonLengths[ ribbonLengthIndex ];		// lower x bounds.
				float ux = ribbonLengths[ ribbonLengthIndex + 1 ];	// upper x bounds.
				
				if( cx >= lx && cx < ux )	// found! contour lies between p1 and p2.
				{
					if( j > contourMaxIndex )
					{
						contourMaxIndex = j;
					}
					
					float p = ( cx - lx ) / ( ux - lx );
					
					cp = p1 + p21 * p;
					
					break;
				}
				
				if( j == ( ribbonLength - 2 ) * 3 )	// last point.
				{
					outsideOfRibbon = true;
				}
			}
			
			if( outsideOfRibbon )
			{
				break;
			}
			
			ofxVec3f cd = ofxVec3f( ribbonDirections[ j + 0 ], ribbonDirections[ j + 1 ], ribbonDirections[ j + 2 ] );
			cd *= cy;
			cp += cd;
			
			ofVertex( cp.x, cp.y, cp.z );
		}
		
		if( outsideOfRibbon )
		{
			break;
		}
	}
	
	contourStartIndex = contourMaxIndex;
	
	if( !outsideOfRibbon )
	{
		ofEndShape( true );
	}
}


void RibbonType :: drawLetterAsPlane( int letter, float xOffset, float yOffset )
{
	int characterIndex = getCharacterIndex( letter );
	if( characterIndex == -1 )
	{
		return;
	}
	
	ofTTFCharacter ttfChar;
	ttfChar = characterContours.at( characterIndex );
	
	CharacterRect charRect;
	charRect = characterRectangles.at( characterIndex );
	
	bool outsideOfRibbon = false;
	
	ofxVec3f cp;	// contour point.
	ofxVec3f cd;	// contour direction.
	ofxVec3f p1;	// current ribbon point position.
	ofxVec3f p2;	// next ribbon point position.
	ofxVec3f p21;	// direction from p1 to p2;

	float px = xOffset + charRect.width * 0.5;
	
	int j;
	for( j=contourStartIndex; j<( ribbonLength - 1 ) * 3; j+=3 )
	{
		int ribbonLengthIndex = (int)( j / 3 );
		float lx = ribbonLengths[ ribbonLengthIndex ];		// lower x bounds.
		float ux = ribbonLengths[ ribbonLengthIndex + 1 ];	// upper x bounds.
		
		if( px >= lx && px < ux )	// found! contour lies between p1 and p2.
		{
			contourStartIndex = j;	// TODO :: this can be optimised further so that it searches for the index at the end of the letter.

			p1	= ofxVec3f( ribbonPositions[ j + 0 ], ribbonPositions[ j + 1 ], ribbonPositions[ j + 2 ] );
			p2	= ofxVec3f( ribbonPositions[ j + 3 ], ribbonPositions[ j + 4 ], ribbonPositions[ j + 5 ] );
			p21	= p2 - p1;
			
			float p = ( px - lx ) / ( ux - lx );
			
			cp = p1 + p21 * p;
			
			cd = ofxVec3f( ribbonDirections[ j + 0 ], ribbonDirections[ j + 1 ], ribbonDirections[ j + 2 ] );
			
			break;
		}
		
		if( j == ( ribbonLength - 2 ) * 3 )	// last point.
		{
			outsideOfRibbon = true;
		}
	}
	
	if( outsideOfRibbon )
	{
		return;
	}
	
	glPushMatrix();
	
	ofxVec3f upVec		= ofxVec3f( cd );;
	ofxVec3f rightVec	= p21.getNormalized();
	ofxVec3f outVec		= upVec.getCrossed( rightVec );
	
	float *mat = new float[ 16 ];
	mat[0]	= rightVec.x;
	mat[1]	= rightVec.y;
	mat[2]	= rightVec.z;
	mat[3]	= 0;
	mat[4]	= upVec.x;
	mat[5]	= upVec.y;
	mat[6]	= upVec.z;
	mat[7]	= 0;
	mat[8]	= outVec.x;
	mat[9]	= outVec.y;
	mat[10]	= outVec.z;
	mat[11]	= 0;
	mat[12]	= cp.x;
	mat[13]	= cp.y;
	mat[14]	= cp.z;
	mat[15]	= 1;
	
	glMultMatrixf( mat );
	
	delete[] mat;
	
	if( useRibbonColorArray )
	{
		int k = ( j / 3 ) * 4;
		glColor4f( ribbonColors[ k + 0 ], ribbonColors[ k + 1 ], ribbonColors[ k + 2 ], ribbonColors[ k + 3 ] );
	}
	
	ofBeginShape();
	
	for( int k=0; k<ttfChar.contours.size(); k++ )
	{
		if( k != 0 )
		{
			ofNextContour(true);
		}
		
		for( int i=0; i<ttfChar.contours[ k ].pts.size(); i++ )
		{
			float cx = ttfChar.contours[ k ].pts[ i ].x - charRect.width * 0.5;
			float cy = ttfChar.contours[ k ].pts[ i ].y + yOffset;
			
			ofVertex( cx, cy, 0 );
		}
	}

	ofEndShape( true );
	
	glPopMatrix();
}

////////////////////////////////////////////////////////////
//	SETTERS.
////////////////////////////////////////////////////////////

void RibbonType :: setKerning( float value )
{
	kerning = value;
}

void RibbonType :: wrapRibbonSurface( bool value )
{
	wrapSurface = value;
}
