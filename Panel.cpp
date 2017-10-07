#include "Panel.h"

// The FT6206 uses hardware I2C (SCL/SDA)
Adafruit_FT6206 ctp = Adafruit_FT6206(); 

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC); 

// arc sine array
const PROGMEM uint8_t asin21[] = { 0, 18, 26, 32, 37, 42, 46, 51, 55, 59, 63, 
	67, 71, 75, 80, 84, 89, 94, 100, 108, 127 };

// arc cos array 
const PROGMEM uint8_t acos21[] = {255, 218, 202, 190, 179, 170, 160, 152, 
	143, 135, 127, 119, 111, 102, 94, 85, 75, 64, 52, 36, 0};

// sine array
// note that index of asin21 lines up with index of sin21, cos21
// 		for -pi/2 <= i <= pi/2
// 		e.g., sin21, cos21 only have half of period, the part used for 
const PROGMEM uint8_t sin21[] = { 0, 6, 12, 19, 24, 31, 37, 44, 50, 56, 63, 69,
	75, 81, 89, 95, 101, 107, 114, 121, 128 };
// cos array
const PROGMEM uint8_t cos21[] = { 64, 91, 102, 109, 114, 119, 122, 124, 126, 
	127, 127, 127, 126, 125, 122, 119, 115, 110, 103, 92, 64 };

// length of array
const uint8_t TRIG_LEN = 21;

//*****************************************************************************
//
// Panel class
//
//*****************************************************************************

//-----------------------------------------------------------------------------
// 
// Constructor
//
//-----------------------------------------------------------------------------
Panel::Panel( uint16_t x, uint16_t y, uint16_t w, uint16_t h,
			bool (*method)(uint16_t x, uint16_t y, Panel *ppanel) ) :
			_x(x), _y(y), _w(w), _h(h), _method(method)
{
	_next 	= NULL;
	_child 	= NULL;
	_enable	= true;
}

//-----------------------------------------------------------------------------
// 
// Destructor
//
//-----------------------------------------------------------------------------
Panel::~Panel()
{
	tft.fillRect(_x, _y, _w, _h, BG_COLOR);
}

//-----------------------------------------------------------------------------
// 
// isTouched
//
// If the panel is touched, then the corresponding function will be called
//
//-----------------------------------------------------------------------------
bool Panel::isTouched( uint16_t x, uint16_t y )
{
	if( !_enable )
	{
		return false;
	}
	// if it is within the range of the panel
	if( (x >= _x) && (x < _x + _w) && (y >= _y) && (y < _y+ _h))
	{
		//(*_method)(x, y, this);
		updatePanel(x, y);
		return true;
	}
	else 
		return false;
}


//*****************************************************************************
//
// Button class
//
//*****************************************************************************

//-----------------------------------------------------------------------------
// 
// Constructor
//
//-----------------------------------------------------------------------------
Button::Button( uint16_t x, uint16_t y, uint16_t w, uint16_t h,
			bool (*method)(uint16_t x, uint16_t y, Panel *ppanel), 
			uint16_t color ) :
			Panel(x, y, w, h, method),
			_color(color)
{
	_state = false;
}
//-----------------------------------------------------------------------------
// 
// drawPanel
//
// Draws button on screen
//
//-----------------------------------------------------------------------------
void Button::drawPanel( void )
{
	// don't need edges of button to be filled
	tft.fillRect(_x+1, _y+1, _w-1, _h-1, _color);
	tft.drawRect(_x, _y, _w, _h, BG_COLOR);
}

//-----------------------------------------------------------------------------
// 
// updatePanel
//
// Draws button on screen
//
//-----------------------------------------------------------------------------
void Button::updatePanel( uint16_t x, uint16_t y)
{
	// button pressing requires delay, otherwise would constantly be switching
	unsigned long _current_millis = millis();
	if(_current_millis < _old_millis + 400)
	{
		return;
	}
	_old_millis = _current_millis;
	// call bound method
	(*_method)(x, y, this);
	// toggle button
	_state = !_state;
	if ( _state )
	{
		tft.drawRect(_x, _y, _w, _h, FG_COLOR3);
	}
	else
	{
		tft.drawRect(_x, _y, _w, _h, BG_COLOR);
	}	

}


//*****************************************************************************
//
// Fader class
//
//*****************************************************************************

//-----------------------------------------------------------------------------
// 
// Constructor
//
//-----------------------------------------------------------------------------
Fader::Fader( uint16_t x, uint16_t y, uint16_t w, uint16_t h,
			bool (*method)(uint16_t x, uint16_t y, Panel *ppanel), 
			uint16_t color ) :
			Panel(x, y, w, h, method),
			_color(color)
{
	_value = _x + _border; // maybe should be x instead of 0?
	_old_val = _value;
	_y_dim = h - 2 * _border;
	_min = _x_dim/2 + _border;
	_max = MAX_X - (_x_dim/2 + _border);
}

//-----------------------------------------------------------------------------
// 
// drawPanel
//
//-----------------------------------------------------------------------------

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// FIX ME has issues when _x != 0, when length is not maxx
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
void Fader::drawPanel( void )
{
	// draw fader track
	tft.drawFastHLine(_min, _y + _h/3, _max - _min, FG_COLOR1);
	tft.drawFastHLine(_min, _y + 2*_h/3, _max - _min, FG_COLOR1);
	// draw fader
	tft.drawRect(_value, _y + _border, _x_dim, _y_dim, _color);
	// "erase" track where fader is	
	tft.drawFastHLine(_value + 1, _y + _h/3, _x_dim - 2 , BG_COLOR);
	tft.drawFastHLine(_value + 1, _y + 2*_h/3, _x_dim - 2, BG_COLOR);
}

//-----------------------------------------------------------------------------
// 
// updatePanel
//
//-----------------------------------------------------------------------------
void Fader::updatePanel( uint16_t x, uint16_t y )
{
	// determine where track should be drawn in after being covered by fader
	// does not go below bottom of track
	uint16_t track_min = _old_val > _min ? _old_val : _min;
	uint16_t track_w;
	// call bound method
	(*_method)(x, y, this);
	// does not go above top of track
	if( track_min + _x_dim < _max )
	{
		track_w = _x_dim;
	}
	else
	{
		track_w = _max - track_min;
	}
	// want to make sure everything stays on screen
	if(x > _min)
	{
		if( x < _max )
		{
			_value = x - _x_dim/2;
		}
		else
		{
			_value = MAX_X - _x_dim - _border;
		}
	}
	else
	{
		_value = _border;
	}


	// erase previous rect
	tft.drawRect(_old_val, _y + _border, _x_dim, _y_dim, BG_COLOR);
	// fill in lines / erase line inside fader
	tft.drawFastHLine(track_min, _y + _h/3, track_w, FG_COLOR1);
	tft.drawFastHLine(track_min, _y + 2*_h/3, track_w, FG_COLOR1);
	tft.drawFastHLine(_value, _y + _h/3, _x_dim, BG_COLOR);
	tft.drawFastHLine(_value, _y + 2*_h/3, _x_dim, BG_COLOR);
	// Draw new fader
	tft.drawRect(_value, _y + _border, _x_dim, _y_dim, _color);

	_old_val = _value;
}


//*****************************************************************************
//
// Sketch class
//
//*****************************************************************************

//-----------------------------------------------------------------------------
// 
// Constructor
//
//-----------------------------------------------------------------------------
Sketch::Sketch( uint16_t x, uint16_t y, uint16_t w, uint16_t h,
			bool (*method)(uint16_t x, uint16_t y, Panel *ppanel), 
			uint16_t color ) :
			Panel(x, y, w, h, method),
			_color(color)
{
}

//-----------------------------------------------------------------------------
// 
// drawPanel
//
//-----------------------------------------------------------------------------
void Sketch::drawPanel( void )
{
	tft.drawRect(_x, _y, _w, _h, FG_COLOR1);
	tft.fillRect(_x + 1, _y + 1, _w - 2, _h - 2, DARK_GRAY);
	// don't be dumb and make it small or it will probably have issues
	uint16_t x0 = _x + _w/2;
	uint16_t y0 = _y + _h/2;
	uint8_t xi = _w/8;
	uint8_t yi = _h/8;
	uint8_t x_ax = _w/16;
	uint8_t y_ax = _h/16;

	for( uint16_t i = 0; i < 8; i++ )
	{
		tft.drawFastHLine( i*xi, y0, x_ax, FG_COLOR1 ); 
		tft.drawFastVLine( x0, i*yi + _y, y_ax, FG_COLOR1 );
	}
}

//-----------------------------------------------------------------------------
// 
// updatePanel
//
//-----------------------------------------------------------------------------
void Sketch::updatePanel( uint16_t x, uint16_t y )
{
	// call bound method
	(*_method)(x, y, this);
	tft.fillCircle(x, y, PENRADIUS, _color);
}

//*****************************************************************************
//
// Knob class
//
//*****************************************************************************

//-----------------------------------------------------------------------------
// 
// Constructor
//
//-----------------------------------------------------------------------------
Knob::Knob( uint16_t x, uint16_t y, uint16_t w, uint16_t h,
			bool (*method)(uint16_t x, uint16_t y, Panel *ppanel), 
			uint16_t color ) :
			Panel(x, y, w, h, method),
			_color(color)
{
	if( _w > _h ) // this needs to be a square
	{
		// put corner of circle in middle of range
		_x += (_w - _h)/2;
		_w = _h;
		_r = _h/2;
	}
	else
	{
		// put corner of circle in middle of range
		_y += (_h - _w)/2;
		_h = _w;
		_r = _w/2;
	}
	_r2 = _r * _r;

	_old_xplot = 0;
	_old_yplot = 0;
}

//-----------------------------------------------------------------------------
// 
// drawPanel
//
//-----------------------------------------------------------------------------
void Knob::drawPanel( void )
{
	tft.drawRoundRect( _x, _y, 2*_r, 2*_r, _r, FG_COLOR1 );
	// put value in center of circle
	tft.fillCircle( _x + _w - 2 * _border , _y + _r, _border, _color );
}

//-----------------------------------------------------------------------------
// 
// updatePanel
//
//-----------------------------------------------------------------------------
void Knob::updatePanel( uint16_t x, uint16_t y )
{
	// diameter of circle to be displayed
	static uint8_t d_border = 2*_r - 3*_border;
	//uint8_t _r_border2 = _r_border * _r_border;
	uint8_t index;
	uint8_t cost; // cosine and sine values
	uint8_t sint;
	uint16_t xplot;
	uint16_t yplot;
	(*_method)(x, y, this);
	
	if ( x - _x < _r/2 )
	{
		x = _r/2 + _x;
	}
	index = getTheta(x, y, this, true);
	sint = pgm_read_byte_near( sin21 + index );
	cost = pgm_read_byte_near( cos21 + index );
	// kind of like mapping from -r to r
	xplot = map(sint, 0, 128, 0, d_border);
	// although could probably save space here and just map
	// from 0 to r
	// although maybe some kind of sin function will be needed again 
	yplot = map(cost, 0, 128, 0, d_border);
	if(_old_xplot) // erase previous mark
	{
		tft.fillCircle( _old_xplot, _old_yplot, _border, BG_COLOR );
	}
	else // first touch
	{
		tft.fillCircle( _x + _w - 2 * _border , _y + _r, _border, BG_COLOR );
	}
	// add offset due to position of knob 
	xplot += (_x+_border);

	if ( y - _r > _y ) // "positive" horizontal
	{
		yplot += (_y - _border);
	}
	else // "negative" horizontal
	{
		yplot = _y + _w + _border - yplot;
	}
	_old_xplot = xplot;
	_old_yplot = yplot;
	tft.fillCircle( xplot, yplot, _border, _color );//_border
}


//*****************************************************************************
//
// Menu class
//
//*****************************************************************************

//-----------------------------------------------------------------------------
// 
// Constructor
//
//-----------------------------------------------------------------------------
Menu::Menu( void )
{
	_head = NULL;
	_tail = NULL;
}

//-----------------------------------------------------------------------------
// 
// Destructor
//
//-----------------------------------------------------------------------------
Menu::~Menu( void )
{
	Panel *ppanel;
	do 
	{
		ppanel  = _head;
		_head = ppanel->getNext();
		delete ppanel;
	} while( ppanel != NULL );
}

//-----------------------------------------------------------------------------
// 
// addPanel
//
// Adds a panel to the menu
//
//-----------------------------------------------------------------------------
void Menu::addPanel( Panel *ppanel)
{
	// if first node
	if( _head == NULL )
	{
		_head = ppanel;
		_tail = ppanel;
	}
	else
	{
		_tail->setNext(ppanel);
		_tail = ppanel;
	}
}
//-----------------------------------------------------------------------------
// 
// drawMenu
//
// calls drawPanel() function of all panels
//
//-----------------------------------------------------------------------------
void Menu::drawMenu( void )
{
	Panel *ppanel = _head;
	while( ppanel != NULL )
	{
		ppanel->drawPanel();
		ppanel = ppanel->getNext();
	}
}

//-----------------------------------------------------------------------------
// 
// isTouched
//
// calls isTouched() functions of all panels
//
//-----------------------------------------------------------------------------
void Menu::isTouched( uint16_t x, uint16_t y )
{
	Panel *ppanel = _head;
	while( ppanel != NULL )
	{
		// return as soon as first panel is touched
		// panels shouldn't overlap anyways
		if( ppanel->isTouched(x, y) )
		{
			return;
		}
		ppanel = ppanel->getNext();
	}
}


//-----------------------------------------------------------------------------
// 
// getTheta
//
// necessary in order to use the Knob class
//
//-----------------------------------------------------------------------------
uint8_t getTheta( uint16_t x, uint16_t y, Panel* ppanel, bool ind )
{
	uint16_t _x = ppanel->getX();
	uint16_t _y = ppanel->getY();
	uint8_t _r = ppanel->getMax();
	// x and y relative to knob edges
	uint16_t x_knob = x - _x;
	uint16_t y_knob = y - _y;
	uint8_t x_index;
	uint8_t y_index;
	uint8_t theta;

	if( ind ) // return index instead of theta
	{
		x_index = map(x_knob, 0, 2*_r, 0, TRIG_LEN); 
		return x_index;
	}
	if(x_knob < _r){
		x_index = map(x_knob, 0, 2*_r, 0, TRIG_LEN);
		if( ind ){
			return x_index;
		} 
		theta = pgm_read_byte_near(asin21 + x_index);
		if( y_knob < _r ){ // need to reflect angle across horizontal axis
			theta = 255 - theta;
		}
	}
	else{
		y_index = map(y_knob, 0, 2*_r, 0, TRIG_LEN);
		if( ind ){
			return y_index;
		}
		theta = pgm_read_byte_near(acos21 + y_index);
	}
	return theta;
}