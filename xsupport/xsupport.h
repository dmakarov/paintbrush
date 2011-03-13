/* SIMPLIFIED MOTIF INTERFACE (XSUPPORT PACKAGE) HEADER FILE.

   Apostolos Lerios - tolis@cs. */


/* Avoid multiple inclusions. */

#ifndef XSUPPORT_H
#define XSUPPORT_H


/* X Toolkit definitions. */

#include "X11/Intrinsic.h"


/* USER INTERFACE DATA STRUCTURES. */

/* A push button.

Name is the label that appears within the button. Callback is a void
function with no arguments that is called whenever the button is
activated. */

typedef struct {
  void *Private; /* FOR PRIVATE USE - DO NOT TOUCH! */

  char *Name;
  void (*Callback)(void);
} PushButton;

/* A file dialog button.

Name is the label that appears within the button. When the button is
pressed, a file selection dialog appears, asking the user to select a
file. Message is the string that appears next to the file selection
prompt. Callback is a void function with a string argument that is
called whenever the user selects a file; the name of the selected file
is passed to the callback function as its single argument. */

typedef struct {
  void *Private; /* FOR PRIVATE USE - DO NOT TOUCH! */

  char *Name;
  char *Message;
  void (*Callback)(char *);
} DialogButton;

/* Choice button sets.

Choice buttons come in sets. Each set of choice buttons has its member
buttons placed one next to the other. Each button has its own title,
and the set as a whole has a label which appears to the left of all
the buttons in a set. A set of choice buttons comes in two flavors:
check boxes and radio buttons. In the former case, each of the buttons
in the set can be turned on and off individually. In the latter case,
one and only one button in a set is always active at any instant in
time.

ChoiceButtonSet.Name is the label of the set. Members is the array of
buttons that are members of the set. Radio is a binary flag that when
set (1) makes the choice button set a set of radio buttons; otherwise,
when Radio is 0, the set contains check boxes. Each button has its own
label, given in ChoiceButton.Name. Set indicates whether the button is
initially on (1) or off (0); for radio buttons, if multiple buttons
have Set equal to 1, an inconsistent specification, then only the
first member of the set is turned on.  Callback is a void function
with one argument that is called whenever the button's status is
changed; the argument indicates the new button status (1 if the button
is now active, 0 if it was turned off). Note that the callback is NOT
called when the buttons are created (i.e. when the buttons whose Set
field is 1 are initially turned on). */

typedef struct {
  void *Private; /* FOR PRIVATE USE - DO NOT TOUCH! */

  char *Name;
  int Set;
  void (*Callback)(int);
} ChoiceButton;

typedef struct {
  void *Private; /* FOR PRIVATE USE - DO NOT TOUCH! */

  char *Name;
  ChoiceButton *Members;
  int Radio;
} ChoiceButtonSet;

/* A slider.

Name is the label that appears to the left and under the slider.
Minimum, Maximum, and InitialValue specify the minimum, maximum and
starting values of the slider. These numbers are all integers. In
order to display a slider that can range over floating point values,
you provide the significant digits in the integer variables, and set
Decimals to the number of the significant digits that should appear to
the right of the decimal point. For example, if you want a slider to
range from 0.00 to 1.00 (both inclusive), have an initial value of
0.59, and a precision of two decimals, you use 0 for Minimum, 100 for
Maximum, 59 for InitialValue, and 2 for Decimals. Callback is a void
function with a float argument that is called whenever the slider's
position has been permanently changed (i.e. NOT while the slider is
being dragged). The single float argument passed to the callback
contains the slider value exactly as displayed on the screen (i.e. the
adjustment for significant digits and decimals has been taken care
of). Hence, in the previous example, the callback argument would be a
float in the range 0 to 1 (both inclusive). */

typedef struct {
  void *Private; /* FOR PRIVATE USE - DO NOT TOUCH! */

  char *Name;
  int Minimum;
  int Maximum;
  int InitialValue;
  short Decimals;
  void (*Callback)(float);	
} Slider;

/* A canvas.

A canvas is a rectangular area holding the contents of an image. Width
and Height set respectively the horizontal and vertical dimensions of
the canvas. Pixels is the array of pixel values. Conceptually, it is a
2-D array of total size Width*Height, with one element of unsigned
long type per image pixel. The red, green, and blue components of a
pixel are encoded within this single long, each component taking up 8
bits.  We do not ask you to access the elements of the Pixels array
directly, since it is easy to get confused both when trying to index
the array and also when trying to extract the primary color components
from a pixel; instead, use the ensuing macros.

Callback is a void function taking three arguments. It is called when
the user is using the mouse in an effort to influence the canvas
window. Hence, Callback is called when:

 - The mouse enters or leaves the canvas.

 - The user is not holding down the left mouse button and the mouse is
 moving within the canvas.

 - The user presses the left mouse button within the canvas.

 - The left mouse button is being held down and PuffInterval is
 non-negative. Moreover, the button must have been depressed while the
 mouse was within the canvas. In this case, you get a callback
 whenever the mouse moves. Also, irrespective of the mouse movement,
 the callback is called every PuffInterval milliseconds.

 - The user releases the left mouse button anywhere on the screen.
 Moreover, the button must have been depressed while the mouse was
 within the canvas.

Callback's first two arguments contain the location of the mouse in
canvas coordinates; these are defined so that (0,0) is the top left
corner of the canvas window. As inferred from above, Callback may be
called with its first two arguments specifying a coordinate outside
the active canvas area, i.e. outside the range 0 to Width-1
horizontally or 0 to Height-1 vertically. In particular, you can
assume that this will always happen when Callback is called as a
result of the user moving the mouse out of the canvas.

The third argument passed to Callback is set to the status of the left
mouse button and the modifier keys. This status changes over time, and
it is important to know exactly what value is passed to Callback: if
the left mouse button is held down, you get the status at the time
that the button was depressed; if the left mouse button is up, you get
the current status of the button and the modifier keys.

This third argument is a mask of bit flags. You can use the following
constants to find the active buttons and/or modifiers:

 Button1Mask: Left mouse button.
 ControlMask: Control key.
 LockMask:    Caps Lock key.
 ShiftMask:   Shift key.

Note that if the user presses the left mouse button in the canvas,
Callback is guaranteed to be called both when the button is pressed,
AND when the button is released (whether the button is released within
the canvas or outside). 

The hardware cursor (an arrow by default) can be disabled by setting
DisambleHardwareCursor to a non-zero value. 
*/

typedef struct {
  void *Private; /* FOR PRIVATE USE - DO NOT TOUCH! */

  int Width;
  int Height;
  int PuffInterval;
  unsigned long *Pixels;
  void (*Callback)(int, int, unsigned int);
  int DisableHardwareCursor; /* boolean */
} Canvas;


/* MACROS. */

/* The following macros retrieve the primary color components from one
canvas pixel P. */

#define GET_RED(P)   ((P)&0x0FF)
#define GET_GREEN(P) (((P)>>8)&0x0FF)
#define GET_BLUE(P)  (((P)>>16)&0x0FF)
#define GET_ALPHA(P) (((P)>>24)&0x0FF)

/* The following macros set the primary color components of the canvas
pixel P to C. C must lie between 0 and 255 (both endpoints
inclusive). */

#define SET_RED(P,C)   (P=(((P)&0xFFFFFF00)|(C)))
#define SET_GREEN(P,C) (P=(((P)&0xFFFF00FF)|((C)<<8)))
#define SET_BLUE(P,C)  (P=(((P)&0xFF00FFFF)|((C)<<16)))
#define SET_ALPHA(P,C) (P=(((P)&0x00FFFFFF)|((C)<<24)))


/* The following macro retrieves the pixel at coordinates (X,Y) of the
canvas C (where C is a pointer to a canvas structure). (0,0) are the
coordinates of the top left corner of the canvas, while the bottom
right corner is located at (C->Width-1,C->Height-1). */

#define PIXEL(C,X,Y) ((C)->Pixels[(Y)*(C)->Width+(X)])


/* FUNCTION DECLARATIONS. */

/* C++ linkage convention established, if and only if you use C++. */

#ifdef __cplusplus
extern "C" {
#endif

/* The control loop launcher.

Builds an application window containing all the widgets you specify in
PushButtons, DialogButtons, ChoiceButtonSets, Sliders, and Canvases.
Each of these arguments is an array of structures. The last element of
the arrays for PushButtons, DialogButtons, Sliders, and Canvases
should contain a NULL Callback. The last element of the
ChoiceButtonSets array should contain a NULL array of Members.
Finally, for each element of the ChoiceButtonSets array (except for
the last), the last element of its Members array should contain a NULL
Callback.

argc is a POINTER to the first parameter of main(), and argv is the
second parameter of main(). The command-line arguments are passed to
Motif which uses them to set several attributes of your
widgets. Hence, the xsupport package understands all standard X
arguments. In addition, xsupport decodes the following custom
arguments:

 -help: shows a summary of the switches xsupport understands.

 -8bit: forces the display to 8-bit mode. xsupport automatically
 detects 24-bit screens. If the screen can support 24-bit drawing,
 xsupport will draw canvases in full RGB; otherwise, xsupport uses an
 8-bit colormap to display images. However, the "-8bit" flag overrides
 this automatic detection scheme, and enforces 8-bit colormap display
 on all screens.

 -gamma <g>: sets the monitor gamma to g. This value is used only when
 the display is in 8-bit mode. The default value is 2.0. When you
 force 8-bit mode on a 24-bit screen, using the "-8bit" switch, set
 the monitor gamma to 1.0 (or disable gamma correction using
 SetCanvasMode()) since 24-bit screens perform gamma correction
 automatically in hardware.

LiftOff() never returns if it succeeds; as soon as it creates the
widgets, it passes control to the X Toolkit and Motif, and then calls
your callbacks when appropriate. */

void LiftOff(int *argc,
	     char **argv,
	     PushButton *PushButtons,
	     DialogButton *DialogButtons,
	     ChoiceButtonSet *ChoiceButtonSets,
	     Slider *Sliders,
	     Canvas *Canvases);

/* Control modifiers.

SetChoice() is used to set the state of the choice button C (1 for on,
0 for off). The callback of the choice button is NOT called. Apply
this function ONLY to check boxes; if you apply it to radio buttons,
you may end up having more than one radio button selected in the set.

SetSlider() is used to set the value of the slider S to Value. As the
InitialValue field of the Slider structure, Value is an integer
containing only the significant digits of the actual
value. SetSlider() does not invoke the callback of the slider.

Both routines should be called after LiftOff() has been executed. */

void SetChoice(ChoiceButton *C,
	       int State);

void SetSlider(Slider *S,
	       int Value);

/* File-canvas interface.

LoadCanvas() reads the contents of the file named FileName into the
canvas C. The Pixels array of C is NOT deallocated by LoadCanvas().
However, LoadCanvas() does allocate a new Pixels array (using
malloc()), big enough to fit the image read.

SaveCanvas() saves the canvas C into the file named FileName. The
Pixels array of C is not modified by SaveCanvas().

Both routines return 1 if and only if they complete successfully. */

int LoadCanvas(char *Filename,
	       Canvas *C);

int SaveCanvas(char *Filename,
	       Canvas *C);

/* Canvas redrawing.

Draws on the screen a portion of the canvas C. In particular, it draws
a rectangle with its top left corner located at (FromX,FromY), and its
bottom right corner at (ToX,ToY). It is your responsibility to
guarantee that FromX, FromY, ToX, and ToY are all set to values within
the canvas bounds.

If you call UpdateCanvas() for just a few pixels on the screen, then
these updated pixels may not show up on the screen right away. This is
a side effect of the X window system bufferring scheme. Moving the
mouse inside the canvas, or clicking on the canvas, will force the
screen update (the output buffer of X is flushed). It is possible to
force X to empty its buffers by calling Flush(), but UpdateCanvas()
does not do this; the reason is that forcing X to flush its buffers
after every UpdateCanvas() reduces the system performance
significantly. For the same reason, you are advised to refrain from
using Flush(), and if you feel the compelling need to do so, do it for
debugging only.

This routine should be called after LiftOff() has been executed. */

void UpdateCanvas(Canvas *C,
		  int FromX,
		  int ToX,
		  int FromY,
		  int ToY);

void Flush(void);

/* Canvas mode setting.

Sets the image reproduction mode of canvas C to Mode. A canvas is
normally displayed on the screen in ALL_COLORS mode, whereby it
reproduces as faithfully as possible the image described in its Pixels
array. When a canvas is in ONLY_RED, ONLY_GREEN, or ONLY_BLUE mode, it
displays respectively only the red, green, or blue component of each
image pixel.

When xsupport is executing with an 8-bit display, image reproduction
can take place with or without gamma correction. The GammaCorrect
binary flag indicates whether gamma correction should be used for
canvas C; by default, gamma correction is applied (GammaCorrect is
1). 24-bit displays are assumed to take care of gamma correction in
hardware, and hence xsupport never tries to gamma correct
(GammaCorrect is ignored for all canvases).

When xsupport is executing with an 8-bit display, dithering will not
be used in the ONLY_RED, ONLY_GREEN, or ONLY_BLUE modes, and the 256
shades of a primary will be displayed directly on the screen. (To be
more precise, the 256 shades of a primary will be mapped onto the 240
possible shades that can be displayed in a Motif window.)

This routine should be called after LiftOff() has been executed. */

typedef enum {
  ALL_COLORS=0x00FFFFFF,
  ONLY_RED=0x000000FF,
  ONLY_GREEN=0x0000FF00,
  ONLY_BLUE=0x00FF0000
} CanvasMode;

void SetCanvasMode(Canvas *C,
		   CanvasMode Mode,
		   int GammaCorrect);

/* ResizeCanvas 

changes the size of the canvas and amount of memory allocated for it.
*/

void ResizeCanvas(Canvas *C,
		  int NewWidth, int NewHeight);


/* Change the sensitivity of a control at the given index so
   it is grayed (insensitive) or not, depending on the boolean "grayed".
   For example, to set the 3rd push button to gray, use:
	SetSensitive(PUSHBUTTONS, 2, TRUE);   

   If you want to gray out the second radio button in the group that you
   calleded "Radio Buttons: " in your ChoiceButtonSet, you can use:
	SetSensitive(CHOICEBUTTONSETS "." "Radio Buttons: ", 1, TRUE);   
*/

#define PUSHBUTTONS		"*.Main.PushButtons"
#define DIALOGBUTTONS		"*.Main.DialogButtons"
#define CHOICEBUTTONSETS	"*.Main.ChoiceButtonSets"
#define SLIDERS			"*.Main.Sliders"

void SetSensitive(char * container, int index, int grayed);


#ifdef __cplusplus
}
#endif

#endif
