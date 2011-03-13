/* SAMPLE PAINT PROGRAM TO ILLUSTRATE XSUPPORT.

  Overview
  --------
  After studying this program you should know how to create
  canvases and various UI widgets (sliders, buttons, etc.) 
  using XSupport. 
  
  Organization
  ------------
  The top portion of this file defines global variables and call-back
  functions which XSupport references for program logic and UI state. 
  Once the UI is defined main() launches the program.

  A UI widget is generally defined by these three parts:

    1. Global call-back function(s)
	   Each widget must reference a function that is called if a user
	   activates the widget. This function is called when the user pushes 
	   a button.
  
    2. Global state variable(s)
	   Most widgets require a state variable. A check box for example
	   needs to know wheter it is on or off.

    3. A global widget specific data structure
	   Finally each widget has a particular data struct that
	   ties all of its helper functions and variable together.
	   These widget data structures contain additional qualifiers
	   to specify labels, min/max values, or step size.


  History
  -------
  Created 4/94 by Apostolos Lerios - tolis@cs. 
  Modified 1/95 by Chase Garfinkle - chase@cs. 
  Added Comments 9/02 by Georg Petschnigg - georgp@stanford.edu

*/


#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "xsupport/xsupport.h"

/* a macro for debugging purposes. */
/* #define DEBUG_PAINT */

#ifdef DEBUG_PAINT
#define DOUT(X) printf X
#else
#define DOUT(X)
#endif

#define MAX( X, Y ) (((X) > (Y)) ? (X) : (Y))
#define MIN( X, Y ) (((X) < (Y)) ? (X) : (Y))

/*****************************************************************************/
/* GLOBAL VARIABLES                                                          */
/*****************************************************************************/

typedef enum { HUE = 1, SAT = 2, VAL = 4 } HSV;

/* The brush modes: OP overpainting, TINT tintintg and SAMPLE see bellow */

typedef enum { OP = 0, TINT = 1, SAMPLE = 2 } BRUSH;

/* Some notes on SAMPLE mode.  I added this mode only after the demo.  So,
   obviously it's irrelevant for grading.  SAMPLE mode is when the user can
   click on the image canvas and the color of the visualization canvas becomes
   the color of the image canvas at the location of the user's click.  This
   allows the user to see the effect of the brush it would have on the image
   canvas.  This idea came to me only after the demo.  Originally I though that
   the challenge was to make the brush visualization always visible without
   modifying the visualization canvas.  And obviously the latter is impossible
   for every color of the brush.  This last addition took only 15 minutes to
   implement and improves the application's usability dramatically.  */

int brush_selection = OP;
int visualized_brush = OP;
int brush_component = HUE + SAT + VAL;

unsigned long DARK_CURSOR = 0;
unsigned long BRIGHT_CURSOR = 0xf3ff;

static void adjust_hsv();
static void adjust_rgb();

static void apply_brush( int X, int Y );
static void brush_visualization();
static void display_brush();

/* PUSHBUTTONS. */
static void QuitButton() 
{
	printf("Goodbye!\n");
	exit(0);
}

static void reset_canvas();
static void fill_canvas0();

PushButton PushButtons[] =
{
	{ NULL, "Quit", &QuitButton },
        { NULL, "Reset", &reset_canvas },
        { NULL, "Fill", &fill_canvas0 },
	{ NULL, NULL, NULL }
};


/* SLIDERS. */

/* These are the RGB and HSV components of a brush. */

int Rcomponent = 0x0;
int Gcomponent = 0x80;
int Bcomponent = 0x0;
float Hcomponent = 120;
float Scomponent = 1.0;
float Vcomponent = 0.5;

/* Geometry of the brush */

int brush_width  = 16;
int brush_height = 16;
float aspect_ratio = 1.0;

/* Parameters of the alpha function for weighted mask-driven tinting. */

float brush_thickness = 0.2;

/* Magnification factor of the brush for visualization purposes. */
int brush_magnif = 4;

unsigned long visual_canvas_color = 0xd0ebeb;

/* A buffer which contains a tinted image of a brush on the visualization
   canvas. The pixels from this buffer are used to draw a scaled visualization
   of the brush on the visualization canvas. */

unsigned long brush_pixels[400][400];

/* Functions that handle the sliders. */

static void
SliderRChanged(float NewValue) 
{
    Rcomponent = NewValue;
    adjust_hsv();
    display_brush();
}

static void
SliderGChanged(float NewValue) 
{
    Gcomponent = NewValue;
    adjust_hsv();
    display_brush();
}

static void
SliderBChanged(float NewValue) 
{
    Bcomponent = NewValue;
    adjust_hsv();
    display_brush();
}

static void
slider_update_hue( float NewValue )
{
    Hcomponent = NewValue;
    adjust_rgb();
    display_brush();
}

static void
slider_update_sat( float NewValue )
{
    Scomponent = NewValue;
    adjust_rgb();
    display_brush();
}

static void
slider_update_val( float NewValue )
{
    Vcomponent = NewValue;
    adjust_rgb();
    display_brush();
}

static void
slider_update_brush_size( float NewValue )
{
    brush_width = NewValue;
    brush_height = brush_width / aspect_ratio;
    display_brush();
}

static void
slider_update_aspect_ratio( float NewValue )
{
    if ( brush_width < brush_height )
    {
        brush_width = brush_height;
    }
    else if ( brush_height < brush_width )
    {
        brush_height = brush_width;
    }
    aspect_ratio = NewValue;
    if ( aspect_ratio < 1.0 )
    {
        brush_width = (float) brush_width * aspect_ratio;
    }
    else if ( aspect_ratio > 1.0 )
    {
        brush_height = brush_height / aspect_ratio;
    }
    display_brush();
}

static void
slider_brush_magnification( float NewValue )
{
    brush_magnif = NewValue;
    display_brush();
}

static void
slider_brush_thickness( float NewValue )
{
    brush_thickness = NewValue;
    display_brush();
}

Slider Sliders[] =
{
	{ NULL, "Red",   0, 0xff, 0x0, 0, &SliderRChanged },
	{ NULL, "Green", 0, 0xff, 0x80, 0, &SliderGChanged },
	{ NULL, "Blue",  0, 0xff, 0x0, 0, &SliderBChanged },

	{ NULL, "Hue",	      0, 360, 120, 0, &slider_update_hue },
	{ NULL, "Saturation", 0, 100, 100, 2, &slider_update_sat },
	{ NULL, "Value",      0, 100,  50, 2, &slider_update_val },

	{ NULL, "Size",  1, 20, 16, 0, &slider_update_brush_size },
	{ NULL, "Ratio", 5, 20, 10, 1, &slider_update_aspect_ratio },
	{ NULL, "Scale", 2,  8,  4, 0, &slider_brush_magnification },

	{ NULL, "Thickness", 1, 6, 2, 1, &slider_brush_thickness },

	{ NULL, NULL, 0, 0, 0, 0, NULL }
};


/* CANVASES. */

static void move_cursor( int, int, unsigned int );
int mouse_action_delay = 2;

static void
mouse_action( int xx, int yy, unsigned int clicked )
{
    if ( SAMPLE == brush_selection && clicked )
    {
        move_cursor( xx, yy, clicked );
    }
    else if ( --mouse_action_delay ) return;
    move_cursor( xx, yy, clicked );
    mouse_action_delay = 2;
}

static void
do_nothing( int xx, int yy, unsigned int bd ){}

/* If the fourth parameter (the PuffInterval) is non-negative, the */
/* canvas will register mouse events every PuffInterval milliseconds, */
/* even if the mouse does not move and no button is pressed or */
/* released. */
Canvas Canvases[] =
{
	{ NULL, 256, 256, -1, NULL, &mouse_action, true },
	{ NULL, 384, 384, -1, NULL, &do_nothing, 0 },
	{ NULL, 0, 0, 0, NULL, NULL, 0 }
};


/* DIALOG BUTTONS. */

static void LoadPPM(char *Name) 
{
	
	Canvas NewCanvas;
	
	if (!LoadCanvas(Name,&NewCanvas)) 
	{
		printf("Load failed!\n");
		return;
	}
	ResizeCanvas(&Canvases[0], NewCanvas.Width, NewCanvas.Height);
	memcpy(Canvases[0].Pixels, NewCanvas.Pixels, 
		sizeof(long) * NewCanvas.Width * NewCanvas.Height);
	free(NewCanvas.Pixels);
	UpdateCanvas(&Canvases[0],0,Canvases[0].Width-1,0,Canvases[0].Height-1);
}

static void SavePPM(char *Name) 
{
	if (!SaveCanvas(Name,&Canvases[0]))
		printf("Save failed!\n");
}

DialogButton DialogButtons[] =
{
    { NULL, "Load PPM", "Load image from PPM file:", &LoadPPM },
    { NULL, "Save PPM", "Save image to PPM file:", &SavePPM },
    { NULL, NULL, NULL, NULL }
};


/* CHOICE BUTTONS. */
/* Checkboxes. */

static void
cbox_hue( int Set )
{
    brush_component = ( brush_component & ~HUE ) + HUE * Set;
    display_brush();
}

static void
cbox_sat( int Set )
{
    brush_component = ( brush_component & ~SAT ) + SAT * Set;
    display_brush();
}

static void
cbox_val( int Set )
{
    brush_component = ( brush_component & ~VAL ) + VAL * Set;
    display_brush();
}

ChoiceButton CheckBoxChoices[] =
{
	{ NULL, "Hue", 1, &cbox_hue },
	{ NULL, "Sat", 1, &cbox_sat },
	{ NULL, "Val", 1, &cbox_val },
	{ NULL, NULL, 0, NULL }
};

/* Radio buttons. */

static void RadioButton1Changed(int Set) 
{
    brush_selection = OP;
    display_brush();
}

static void RadioButton2Changed(int Set) 
{
    brush_selection = TINT;
    display_brush();
}

static void RadioButton3Changed(int Set) 
{
    visualized_brush = brush_selection;
    brush_selection = SAMPLE;
}

ChoiceButton RadioButtonChoices[]=
{
	{ NULL, "Overpainting", 1, &RadioButton1Changed },
	{ NULL, "Tinting",	0, &RadioButton2Changed },
	{ NULL, "Sampling",	0, &RadioButton3Changed },
	{ NULL, NULL, 0, NULL }
};

/* Mode-selecting radio buttons. */

static CanvasMode Mode=ALL_COLORS;
static int GammaCorrect=1;

static void Update() { SetCanvasMode(&Canvases[0],Mode,GammaCorrect); }

static void SetGammaCorrectionOn(int Set) 
{
	if (Set && !GammaCorrect) 
	{
		GammaCorrect=1;
		Update();
	}
}

static void SetGammaCorrectionOff(int Set) 
{
	if (Set && GammaCorrect) 
	{
		GammaCorrect=0;
		Update();
	}
}

ChoiceButton GammaCorrectionChoices[] =
{
	{ NULL, "On", 1, &SetGammaCorrectionOn },
	{ NULL, "Off", 0, &SetGammaCorrectionOff },
	{ NULL, NULL, 0, NULL }
};

static void SetAllColorsMode(int Set) 
{
	if (Set && Mode!=ALL_COLORS) 
	{
		Mode=ALL_COLORS;
		Update();
	}
}

static void SetOnlyRedMode(int Set) 
{
	if (Set && Mode!=ONLY_RED) 
	{
		Mode=ONLY_RED;
		Update();
	}
}

static void SetOnlyGreenMode(int Set) 
{
	if (Set && Mode!=ONLY_GREEN) 
	{
		Mode=ONLY_GREEN;
		Update();
	}
}

static void SetOnlyBlueMode(int Set) 
{
	if (Set && Mode!=ONLY_BLUE) 
	{
		Mode=ONLY_BLUE;
		Update();
	}
}

ChoiceButton CanvasModeChoices[] =
{
	{ NULL, "All Colors", 1, &SetAllColorsMode },
	{ NULL, "Red", 0, &SetOnlyRedMode },
	{ NULL, "Green", 0, &SetOnlyGreenMode },
	{ NULL, "Blue", 0, &SetOnlyBlueMode },
	{ NULL, NULL, 0, NULL }
};

/* Collect sets. */

ChoiceButtonSet ChoiceButtons[] = 
{
    { NULL, "Brush: ", RadioButtonChoices, 1 },
    { NULL, "Components: ", CheckBoxChoices, 0 },
    { NULL, "Gamma correct (8-bit mode only)", GammaCorrectionChoices, 1 },
    { NULL, "Canvas mode: ", CanvasModeChoices, 1 },
    { NULL, NULL, NULL, 0}
};

/*
 *  Implementation:
 */

/* Reset the canvas to its initial state. */

static void
reset_canvas()
{
    ResizeCanvas( &Canvases[0], 256, 256 );
    for ( int ii = 0; ii < 256; ++ii )
    {
        for ( int jj = 0; jj < 256; ++jj )
	{
            SET_RED  ( PIXEL( &Canvases[0], ii, jj ), jj );
            SET_GREEN( PIXEL( &Canvases[0], ii, jj ), ii );
            SET_BLUE ( PIXEL( &Canvases[0], ii, jj ),  0 );
        }
    }
    UpdateCanvas( &Canvases[0], 0, Canvases[0].Width-1, 0, Canvases[0].Height-1 );
}

/*  Fill a canvas with a color in pixel parameter. */

static void
fill_canvas( Canvas* canvas, unsigned long pixel )
{
    for ( int ii = 0; ii < canvas->Width; ++ii )
    {
        for ( int jj = 0; jj < canvas->Height; ++jj )
	{
            PIXEL( canvas, ii, jj ) = pixel;
        }
    }
}

/* Redraw the canvas filling it with the color of the brush.  */

static void
fill_canvas0()
{
    unsigned long pixel;
    SET_RED( pixel, Rcomponent );
    SET_GREEN( pixel, Gcomponent );
    SET_BLUE( pixel, Bcomponent );
    fill_canvas( &Canvases[0], pixel );
    UpdateCanvas( &Canvases[0], 0, Canvases[0].Width - 1, 0, Canvases[0].Height - 1 );
}

/*  Fill the visualization canvas with its color. */

static void
init_visual_canvas()
{
    fill_canvas( &Canvases[1], visual_canvas_color );
}

/* Convert a pixel in RGB to HSV. The caller of rgb2hsv is responsible for
   setting hue to a meaningful value, when saturation is 0. */

static void
rgb2hsv( float rr, float gg, float bb, float* hue, float* sat, float* val )
{
    float max = MAX( MAX( rr, gg ), bb );
    float min = MIN( MIN( rr, gg ), bb );

    *val = max;
    *sat = ( 0.0 != max ) ? ( ( max - min ) / max ) : 0.0;

    if ( 0.0 != *sat )
    {
        if ( rr == max )
        {
            *hue = ( gg - bb ) / ( max - min );
        }
        else if ( gg == max )
        {
            *hue = 2.0 + ( bb - rr ) / ( max - min );
        }
        else
        {
            *hue = 4.0 + ( rr - gg ) / ( max - min );
        }
        *hue *= 60;
        if( *hue < 0.0 )
        {
            *hue += 360.0;
        }
    }
}

/* Convert a pixel in HSV to RGB. */

static void
hsv2rgb( float hue, float sat, float val, float* rr, float* gg, float* bb )
{
    if ( 0.0 == sat )
    {
        *rr = val;
        *gg = val;
        *bb = val;
        return;
    }

    if ( 360.0 == hue)
    {
        hue = 0.0;
    }

    hue /= 60.0;
    int ii = hue;
    float ff = hue - ii;
    float pp = val * ( 1.0 - sat );
    float qq = val * ( 1.0 - ( sat * ff ) );
    float tt = val * ( 1.0 - ( sat * ( 1.0 - ff ) ) );
    switch ( ii )
    {
    case 0: *rr = val; *gg = tt;  *bb = pp; break;
    case 1: *rr = qq;  *gg = val; *bb = pp; break;
    case 2: *rr = pp;  *gg = val; *bb = tt; break;
    case 3: *rr = pp;  *gg = qq;  *bb = val; break;
    case 4: *rr = tt;  *gg = pp;  *bb = val; break;
    case 5: *rr = val; *gg = pp;  *bb = qq; break;
    }
}

/*  When the brush color changed in RGB, this function is called to update HSV
    sliders.  Hue is deliberately left undefined. If Saturation changed to 0.0,
    hue retains its old value. */

static void
adjust_hsv()
{
    float rr = Rcomponent / 255.0;
    float gg = Gcomponent / 255.0;
    float bb = Bcomponent / 255.0;

    rgb2hsv( rr, gg, bb, &Hcomponent, &Scomponent, &Vcomponent );

    SetSlider( &Sliders[3], Hcomponent );
    SetSlider( &Sliders[4], Scomponent * 100.0 );
    SetSlider( &Sliders[5], Vcomponent * 100.0 );
}

/* When the brush color is changed by one of the HSV sliders this function is
   called to adjust RGB sliders. */

static void
adjust_rgb()
{
    float rr = Rcomponent / 255.0;
    float gg = Gcomponent / 255.0;
    float bb = Bcomponent / 255.0;

    hsv2rgb( Hcomponent, Scomponent, Vcomponent, &rr, &gg, &bb );

    Rcomponent = 255 * rr;
    Gcomponent = 255 * gg;
    Bcomponent = 255 * bb;
    SetSlider( &Sliders[0], Rcomponent );
    SetSlider( &Sliders[1], Gcomponent );
    SetSlider( &Sliders[2], Bcomponent );
}

/* This is a simple overpainting brush procedure. */

static void
overpaint( int X0, int Y0, int X1, int Y1, Canvas* canvas )
{
    for ( int ii = X0; ii < X1; ++ii )
    {
        for ( int jj = Y0; jj < Y1; ++jj )
        {
            unsigned long pixel = PIXEL( canvas, ii, jj );
            SET_RED  ( pixel, Rcomponent );
            SET_GREEN( pixel, Gcomponent );
            SET_BLUE ( pixel, Bcomponent );
            PIXEL( canvas, ii, jj ) = pixel;
        }
    }
}

/* Compute the value of a pixel in a tinted brushing procedure. */

static unsigned long
tint_pixel( float br_hue, float br_sat, float br_val, float alpha, unsigned long pixel )
{
    float rr, gg, bb;
    float hh = br_hue, ss, vv;

    // this is the canvas pixel conversion to HSV.
    rr = GET_RED  ( pixel ) / 255.0;
    gg = GET_GREEN( pixel ) / 255.0;
    bb = GET_BLUE ( pixel ) / 255.0;
    rgb2hsv( rr, gg, bb, &hh, &ss, &vv );

    // this is the new pixel in HSV.
    if ( ( brush_component & HUE ) && ( 0.0 != br_sat ) )
    {
        float tmp_hue = br_hue;
        if ( 0.0 != ss )
        {
            if ( hh < tmp_hue && tmp_hue - hh > 180 )
            {
                tmp_hue -= 360.0;
            }
            else if ( hh > tmp_hue && hh - tmp_hue > 180 )
            {
                hh -= 360.0;
            }
            hh = ( 1.0 - alpha ) * hh + alpha * tmp_hue;
            if ( hh < 0.0 )
            {
                hh += 360.0;
            }
        }
        /* otherwise hh already set to be the same as brush's. */
    }
    /* If the canvas color is neutral and HUE change is not requested then it
       should remain neutral, even though the brush may be not neutral. */
    if ( ( brush_component & SAT ) &&  ( ( 0.0 != ss ) || ( brush_component & HUE ) ) )
    {
        ss = ( 1.0 - alpha ) * ss + alpha * br_sat;
    }
    if ( brush_component & VAL )
    {
        vv = ( 1.0 - alpha ) * vv + alpha * br_val;
    }

    // this is the new canvas pixel conversion to RGB.
    hsv2rgb( hh, ss, vv, &rr, &gg, &bb );
    SET_RED  ( pixel, (unsigned long)(rr * 255) );
    SET_GREEN( pixel, (unsigned long)(gg * 255) );
    SET_BLUE ( pixel, (unsigned long)(bb * 255) );

    if ( 1.0 < rr || 0.0 > rr || 1.0 < gg || 0.0 > gg || 1.0 < bb || 0.0 > bb )
    {
        DOUT(( "FATAL ERROR: (rgb) (%4.2f, %4.2f, %4.2f) (hsv) (%4.0f, %4.2f, %4.2f)\n",
               rr, gg, bb, hh, ss, vv ));
    }

    return pixel;
}

/* Return the weighted mask based on a brush pixel coordinates. */

static float
compute_alpha( int xx, int yy )
{
    const float pi = 3.14159;
    float bw = brush_width;
    float bh = brush_height;
    float fx = xx;
    float fy = yy;
    fx = pi * fx / bw;
    fy = pi * fy / bh;

    float alpha = ( sin( fx ) + sin( fy ) ) - 1;
    if ( alpha < 0.0 )
    {
        alpha = 0.0;
    }
    return alpha;
}

static void
tinting( int OX, int OY, int X0, int Y0, int X1, int Y1, Canvas* canvas )
{
    float br_hue;
    float br_sat;
    float br_val;

    // This is the brush in HSV.
    rgb2hsv( Rcomponent / 255.0, Gcomponent / 255.0, Bcomponent / 255.0,
             &br_hue,		 &br_sat,	     &br_val );

    int I0 = OX - brush_width / 2;
    int J0 = OY - brush_height / 2;
    int i0 = X0 - I0;
    int i1 = X1 - I0;
    int j0 = Y0 - J0;
    int j1 = Y1 - J0;
    int xx = X0;
    int yy = Y0;

    if ( i0 >= i1 || j0 >= j1 )
    {
        DOUT(( "BRUSH AREA (%d,%d) (%d,%d)\n", i0, j0, i1, j1 ));
        DOUT(( "OX, OY (%d,%d) X0, Y0 : (%d,%d) X1, Y1 : (%d,%d)\n",
               OX, OY, X0, Y0, X1, Y1 ));
    }

    for ( int ii = i0; ii < i1; ++ii )
    {
        yy = Y0;
        for ( int jj = j0; jj < j1; ++jj )
        {
            float alpha = brush_thickness * compute_alpha( ii, jj );
            PIXEL( canvas, xx, yy ) = tint_pixel( br_hue, br_sat, br_val, alpha, PIXEL( canvas, xx, yy ) );
            ++yy;
        }
        ++xx;
    }
} // tinting


/*  Actually apply the brush to the canvas. */

static void
apply_brush( int X, int Y )
{
/*     DOUT(( "BRUSH ORIGIN (%d, %d) in [%d x %d] ", */
/*            X, Y, Canvases[0].Width, Canvases[0].Height )); */
/*     DOUT(( "SIZE [%d : %d] COLOR <%d, %d, %d>\n", */
/*            brush_width, brush_height, Rcomponent, Gcomponent, Bcomponent )); */
    if ( SAMPLE == brush_selection )
        return;

    int CW = Canvases[0].Width;
    int CH = Canvases[0].Height;
    int LX = X - brush_width / 2;
    if ( LX >= CW ) return;
    int RX = LX + brush_width;
    if ( RX <= 0 ) return;
    int BY = Y - brush_height / 2;
    if ( BY >= CH ) return;
    int TY = BY + brush_height;
    if ( TY <= 0 ) return;
    int X0 = ( LX < 0 ) ? 0 : LX;
    int Y0 = ( BY < 0 ) ? 0 : BY;
    int X1 = ( RX < CW ) ? RX : CW;
    int Y1 = ( TY < CH ) ? TY : CH;

    if ( TINT == brush_selection && brush_component )
    {
        tinting( X, Y, X0, Y0, X1, Y1, &Canvases[0] );
    }
    else
    {
        overpaint( X0, Y0, X1, Y1, &Canvases[0] );
    }

/*     DOUT(( "UPDATE (%d, %d) x (%d, %d)\n", X0, Y0, X1 - 1, Y1 - 1 )); */

    UpdateCanvas( &Canvases[0], X0, X1 - 1, Y0, Y1 - 1 );
} // apply_brush

/* Prepare the image of a brush for visualization. */

static void
brush_visualization()
{
    Canvas* canvas = &Canvases[1];
    int OX = canvas->Width / 2;
    int OY = canvas->Height / 2;
    int X0 = OX - brush_width / 2;
    int X1 = X0 + brush_width;
    int Y0 = OY - brush_height / 2;
    int Y1 = Y0 + brush_height;

    init_visual_canvas();
    if ( TINT == brush_selection && brush_component )
    {
        /* to make it look closer to what it may look like on canvas apply the
           tinting several times. */
        tinting( OX, OY, X0, Y0, X1, Y1, canvas );
        tinting( OX, OY, X0, Y0, X1, Y1, canvas );
        tinting( OX, OY, X0, Y0, X1, Y1, canvas );
        tinting( OX, OY, X0, Y0, X1, Y1, canvas );
        tinting( OX, OY, X0, Y0, X1, Y1, canvas );
    }
    else
    {
        overpaint( X0, Y0, X1, Y1, canvas );
    }

    for ( int ii = 0; ii < brush_width; ++ii )
    {
        for ( int jj = 0; jj < brush_height; ++ jj )
        {
            brush_pixels[ii][jj] = PIXEL( canvas, X0 + ii, Y0 + jj );
        }
    }

    int bw = brush_magnif * brush_width;
    int bh = brush_magnif * brush_height;
    X0 = ( canvas->Width - bw ) / 2;
    X1 = X0 + bw;
    Y0 = ( canvas->Height - bh ) / 2;
    Y1 = Y0 + bh;

    for ( int ii = 0; ii < brush_width; ++ii )
    {
        for ( int jj = 0; jj < brush_height; ++ jj )
        {
            unsigned long pixel = brush_pixels[ii][jj];
            /* 1:1 brush in the corner */
            PIXEL( canvas, 40 + ii, 40 + jj ) = pixel;
            /* magnified brush */
            for ( int mm = 0; mm < brush_magnif; ++mm )
            {
                for ( int nn = 0; nn < brush_magnif; ++nn )
                {
                    int XX = X0 + brush_magnif * ii + mm;
                    int YY = Y0 + brush_magnif * jj + nn;
                    PIXEL( canvas, XX, YY ) = pixel;
                }
            }
        }
    }
}

/*  Actually update the canvas with the image of a visualized brush. */

static void
display_brush()
{
    // if none of the HUE, SAT or VAL are selected don't redisplay the brush.
    if ( 0 == brush_component && SAMPLE != brush_selection )
    {
        return;
    }
    bool restore_sample_mode = false;
    if ( SAMPLE == brush_selection )
    {
        restore_sample_mode = true;
        brush_selection = visualized_brush;
    }
    brush_visualization();
    UpdateCanvas( &Canvases[1], 0, Canvases[1].Width - 1, 0, Canvases[1].Height - 1 );
    if ( restore_sample_mode )
    {
        brush_selection = SAMPLE;
    }
}

/* A very dumb way to estimate the darkness of the background.  We need this to
   show the cursor in a bright color when it's over dark part of the canvas.  */

static bool
is_dark_canvas( int X0, int Y0, int X1, int Y1, Canvas* canvas )
{
    int nn = 0;
    float val = 0;
    float hue = 0;
    float sat = 0;
    for ( int ii = X0; ii < X1; ++ii )
    {
        for( int jj = Y0; jj < Y1; ++jj )
        {
            ++nn;
            float rr, gg, bb, hh = 0, ss, vv;
            unsigned long pixel = PIXEL( canvas, ii, jj );
            rr = GET_RED  ( pixel ) / 255.0;
            gg = GET_GREEN( pixel ) / 255.0;
            bb = GET_BLUE ( pixel ) / 255.0;
            rgb2hsv( rr, gg, bb, &hh, &ss, &vv );
            val += vv;
            hue += hh;
            sat += ss;
        }
    }
    val /= nn;
    hue /= nn;
    sat /= nn;
    if ( (55 < hue && hue < 65 && val > 0.6) || val > 0.9 )
    {
        return false;
    }
    return true;
}

/*  Draw a custom cursor.  Nothing smart here either. Very straightforward
    implmenetation. Save the canavas pixels before they are overwritten with the
    cursor pixels.  Next time when the cursor moves, restore the old pixels
    first, possibly paint the brush if the button is down and finally draw the
    cursor pixels in the new location.  We use alpha ranges close to the brush
    outter edge to determine which pixels belong to the cursor. */

static struct pixbuf {
    int xx, yy;
    unsigned long pixel;
} canvas_pixel[50];

struct alpharanges{
    float left, right;
} alphas[] = { {0.0, 1.0}, /* 0 */ {0.0, 1.0}, /* 1 */ {0.0, 1.0}, /* 2 */ {0.0, 1.0}, /* 3 */
               {0.0, 1.0}, /* 4 */ {0.0, 0.1}, /* 5 */ {0.0, 0.1}, /* 6 */ {.01, 0.1}, /* 7 */
               {.01, 0.1}, /* 8 */ {.02, 0.1}, /* 9 */ {.02, .08}, /* 0 */ {.02, .07}, /* 1 */
               {.02, .06}, /* 2 */ {.02, .05}, /* 3 */ {.02, .05}, /* 4 */ {.02, .05}, /* 5 */
               {.02, .05}, /* 6 */ {.02, .05}, /* 7 */ {.02, .05}, /* 8 */ {.02, .05}, /* 9 */
};

static void
move_cursor( int XX, int YY, unsigned int ButtonDown )
{
    struct alpharanges* alpha_range = &alphas[ ( ( brush_width + brush_height ) / 2 ) - 1];

    static int saved_pixels = -1;
    static int PX0 = -1;
    static int PX1 = -1;
    static int PY0 = -1;
    static int PY1 = -1;

    bool out_of_screen = false;
    unsigned long cursor_color = DARK_CURSOR;
    Canvas* canvas = &Canvases[0];

    int CW = canvas->Width;
    int CH = canvas->Height;
    int LX = XX - brush_width / 2;
    if ( LX >= CW ) out_of_screen = true;
    int RX = LX + brush_width;
    if ( RX <= 0 ) out_of_screen = true;
    int BY = YY - brush_height / 2;
    if ( BY >= CH ) out_of_screen = true;
    int TY = BY + brush_height;
    if ( TY <= 0 ) out_of_screen = true;
    int X0 = ( LX < 0 ) ? 0 : LX;
    int Y0 = ( BY < 0 ) ? 0 : BY;
    int X1 = ( RX < CW ) ? RX : CW;
    int Y1 = ( TY < CH ) ? TY : CH;

    int I0 = XX - brush_width / 2;
    int J0 = YY - brush_height / 2;
    int i0 = X0 - I0;
    int i1 = X1 - I0;
    int j0 = Y0 - J0;
    int j1 = Y1 - J0;

    for ( ; saved_pixels >= 0; --saved_pixels )
    {
        struct pixbuf* pb = &canvas_pixel[saved_pixels];
        PIXEL( canvas, pb->xx, pb->yy ) = pb->pixel;
    }

    if ( PX0 >= 0 )
    {
        UpdateCanvas( canvas, PX0, PX1, PY0, PY1 );
    }
    if ( out_of_screen )
    {
        return;
    }
    if ( ButtonDown )
    {
        if ( SAMPLE == brush_selection )
        {
            visual_canvas_color = PIXEL( canvas, XX, YY );
            display_brush();
        }
        else
        {
            apply_brush( XX, YY );
        }
    }

    if ( is_dark_canvas( X0, Y0, X1, Y1, canvas ) )
    {
        cursor_color = BRIGHT_CURSOR;
    }
    else
    {
        cursor_color = DARK_CURSOR;
    }
    for ( int ii = i0, xx = X0; ii < i1; ++ii, ++xx )
    { 
        for( int jj = j0, yy = Y0; jj < j1; ++jj, ++yy )
        {
            float alpha = 0.2 * compute_alpha( ii, jj );
            if ( alpha_range->left < alpha && alpha < alpha_range->right )
            {
                if ( xx < canvas->Width && yy < canvas->Height && 0 <= xx && 0 <= yy)
                {
                    struct pixbuf* pb = &canvas_pixel[++saved_pixels];
                    pb->xx = xx;
                    pb->yy = yy;
                    pb->pixel = PIXEL( canvas, xx, yy );
                    PIXEL( canvas, xx, yy ) = cursor_color;
                }
                else
                {
                    DOUT(( "cursor (%d, %d)\n", xx, yy ));
                }
            }
        }
    }
    PX0 = X0;
    PX1 = X1 - 1;
    PY0 = Y0;
    PY1 = Y1 - 1;
    UpdateCanvas( canvas, X0, X1 - 1, Y0, Y1 - 1 );
}

/*****************************************************************************/
/* MAIN PROGRAM START                                                        */
/*****************************************************************************/

/* Main initializes the canvases (defined globably) fills them with a
   red-green ramp and then calls LiftOff() to start the User Interface
   thread.
*/

int
main( int argc, char *argv[] )
{
	
    int i, buf_width, buf_height, num_canvases, r, g;
    int buf_size;
    unsigned long* buf;

    /* Assign red-green ramp to the canvas pixels */
    num_canvases = 2;
    for (i = 0; i < num_canvases; i++) 
    {
        buf_width = Canvases[i].Width;
        buf_height = Canvases[i].Height;
        buf_size = buf_width * buf_height;

        /* Be sure to allocate your canvas before lift_off() */

        Canvases[i].Pixels = (unsigned long *)malloc(buf_size * sizeof(unsigned long));

        buf = Canvases[i].Pixels;
        /* Fill buffers with red-green ramp */
        for (r = 0; r < buf_height; r++) 
        {
            for (g = 0; g < buf_width; g++, buf++) 
	    {
                *buf = g*256 + r;
            }
        }
    }
    init_visual_canvas();
    brush_visualization();

    LiftOff( &argc, argv, PushButtons, DialogButtons, ChoiceButtons, Sliders, Canvases );

    return 0;
}
