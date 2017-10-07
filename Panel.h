
#ifndef _panel_h_
#define _panel_h_

#include <Arduino.h>

#include <Adafruit_GFX.h>    // Core graphics library
#include <SPI.h>       // needed for display
#include <Adafruit_ILI9341.h>
#include <Wire.h>      // needed for FT6206
#include <Adafruit_FT6206.h>
#include <avr/pgmspace.h> // needed for PROGMEME

// The FT6206 uses hardware I2C (SCL/SDA)
extern Adafruit_FT6206 ctp; // = Adafruit_FT6206(); 

// The display also uses hardware SPI, plus #9 & #10
#define TFT_CS 10
#define TFT_DC 9
extern Adafruit_ILI9341 tft; // = Adafruit_ILI9341(TFT_CS, TFT_DC); 

// screen size
// ... don't really need both of these
#define MAX_X 240
#define MAX_Y 320
const uint16_t maxx = 240;
const uint16_t maxy = 320;

// touch size
#define PENRADIUS 3

// middle of screen needs to equal 127
#define OFFSET 7

// color definitions
#define WHITE 0xFFFF
#define RED 0xF800
#define YELLOW 0xFF80
#define ORANGE 0xFC40
#define GREEN 0x07E5
#define CYAN 0x07DF
#define BLUE 0x01DF
#define PURPLE 0x81D7
#define PINK 0xF81B
#define GRAY 0x6B6D
#define DARK_GRAY 0x18E3
#define BLACK 0x0000

const uint16_t BG_COLOR = BLACK;
const uint16_t FG_COLOR1 = WHITE;
const uint16_t FG_COLOR2 = CYAN;
const uint16_t FG_COLOR3 = PINK;
//*****************************************************************************
//
// Panel class
// 
// Parent class for all the GUI objects
//
//*****************************************************************************
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// FIX ME should add start position in constructor
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
class Panel
{
	private:
		bool _enable;
		Panel *_next;
		// child isn't always necessary, maybe remove for some applications?
		Panel *_child;		

	protected:
		// display is 240 x 320 so one dim could be uint8_t
		uint16_t _x, _y, _w, _h; 
		bool (*_method)(uint16_t x, uint16_t y, Panel *ppanel); 
		virtual void updatePanel( uint16_t x, uint16_t y  ) = 0;

	public:
		Panel( uint16_t x, uint16_t y, uint16_t w, uint16_t h,
			bool (*method)(uint16_t x, uint16_t y, Panel *ppanel) );
		~Panel();
		virtual void drawPanel( void ) = 0;
		bool isTouched( uint16_t x, uint16_t y );
		// inline functions
		inline uint16_t getX( void ){ return _x; }
		inline uint16_t getY( void ){ return _y; }
		inline uint16_t getW( void ){ return _w; }
		inline uint16_t getH( void ){ return _h; }
		inline Panel* getNext( void ){ return _next; }
		inline void setNext( Panel *ppanel ){ _next = ppanel; }
		// Child is a panel controlled by this panel
		// e.g. a button controls the graph
		inline Panel* getChild( void ){ return _child; }
		inline void setChild( Panel *ppanel ){ _child = ppanel; }
		inline bool getEnable( void ){ return _enable; }
		inline void setEnable( bool en ){ _enable = en; }
		inline virtual uint8_t getMin( void ){ return -1; }
		inline virtual uint8_t getMax( void ){ return -1; }
}; 

//*****************************************************************************
//
// Button class
//
//*****************************************************************************
class Button: public Panel
{
	private:
		uint16_t _color;
		void updatePanel( uint16_t x, uint16_t y  ); 
		bool _state;
		unsigned long _old_millis = 0;

	public:
		Button( uint16_t x, uint16_t y, uint16_t w, uint16_t h,
			bool (*method)(uint16_t x, uint16_t y, Panel *ppanel), 
			uint16_t color );
		void drawPanel( void );

};

//*****************************************************************************
//
// Fader class
//
// creates fader parallel to x axis
// use for increasing / decreasing effect or signal
//
//*****************************************************************************
class Fader: public Panel
{
	private:
		const uint8_t _border = 2; // potentially unnecessary
		uint16_t _color;
		// bottom point of fader
		uint8_t _value;
		// dimensions of fader graphic
		const uint8_t _x_dim = 60;
		uint8_t _y_dim;
		// min and max x values of fader
		uint8_t _min; // lowest center of fader
		uint8_t _max; // highest center of fader
		// save old position so less needs to be erased
		uint16_t _old_val;
		void updatePanel( uint16_t x, uint16_t y  ); 

	public:
		Fader( uint16_t x, uint16_t y, uint16_t w, uint16_t h,
			bool (*method)(uint16_t x, uint16_t y, Panel *ppanel), 
			uint16_t color );
		void drawPanel( void );
		inline uint8_t getMin(void) { return _min; }
		inline uint8_t getMax(void) { return _max; }
};

//*****************************************************************************
//
// Sketch class
//
// creates a place for user to draw input 
//
//*****************************************************************************
class Sketch: public Panel
{
	private:
		uint16_t _color;
		bool _state;
		void updatePanel( uint16_t x, uint16_t y  ); 

	public:
		Sketch( uint16_t x, uint16_t y, uint16_t w, uint16_t h,
			bool (*method)(uint16_t x, uint16_t y, Panel *ppanel), 
			uint16_t color );
		void drawPanel( void );

};
//*****************************************************************************
//
// Knob class
//
// Creates a knob, starts at middle position 
// ideal for blending between two signals
//
//*****************************************************************************
class Knob: public Panel
{
	private:
		uint16_t _color;
		bool _state;
		const uint8_t _border = 2; 
		uint8_t _r;
		uint16_t _r2;
		// save old position so less will need to be erased
		uint16_t _old_xplot; 
		uint16_t _old_yplot;
		void updatePanel( uint16_t x, uint16_t y  ); 

	public:
		Knob( uint16_t x, uint16_t y, uint16_t w, uint16_t h,
			bool (*method)(uint16_t x, uint16_t y, Panel *ppanel), 
			uint16_t color );
		void drawPanel( void );
		inline uint8_t getMax(void) { return _r; }
};

//*****************************************************************************
//
// Menu class
//
// Holds linked list of panels
// order doesn't matter, this will just be a convenient way to call all the 
// isTouched() and drawPanel() functions at the same time, as well as group 
// different menus together
//
//*****************************************************************************
class Menu
{
	private:
		Panel *_head;
		Panel *_tail;

	public:
		Menu();
		~Menu();
		void addPanel( Panel *ppanel );
		void drawMenu( void );
		void isTouched( uint16_t x, uint16_t y );

};

//-----------------------------------------------------------------------------
// 
// getTheta( x, y, Panel* )
//
// returns theta value [0,255] for ind = false
// returns index to asin21 [0, 20] for ind = true
//
//-----------------------------------------------------------------------------
uint8_t getTheta( uint16_t x, uint16_t y, Panel *ppanel, bool ind = false );

#endif // _panel_h_
