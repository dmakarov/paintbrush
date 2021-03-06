# paintbrush

[<img src="https://travis-ci.org/dmakarov/paintbrush.png?branch=master">](https://travis-ci.org/dmakarov/paintbrush)

Functionality:

This program implements two painting brushes.  One is a simple overpainting
rectangular brush and the other is weighted mask-driven tinting brush.  Brushes
are applied by depressing the mouse left button and moving the mouse pointer
over the image while keeping the mouse left button depressed.

Both brushes allow to change their size and aspect ratio.  The color of each
brush can be controlled by three RGB or three HSV sliders.  The RGB and HSV
sliders are interdependent.  This means if the color is changed in one color
space the sliders that represent color in the other color space are adjusted
correspondingly.

For the tinting brush the user can select or unselect each of HSV components.
If a component is not selected, it is not affected on the image canvas when the
brush is applied to the image.  The user can increase or decrease the
transparency (or thickness) of the tinting brush using the 'Thickness' slider.
These controls have no effect on the overpainting brush.

Visualization of the brush is implemented on a separate canvas.  The
visualization allows the user to see the magnified image of brush given a
momentary setting of its mode, shape, size, color and transparency, the latter
for the tinting brush only.  The brush displayed in its original and scaled
sizes.  For scaled size the user can control the magnification of the brush
image by using a slider.  After the demo was already done, I added Sampling
mode, which allows the user to click on the image canvas and change the color of
the visualization canvas background.  In the sampling mode the image of the
brush is adjusted automatically to the changing color of the background.  This
allows the user to see the brush visualization in the same color as it would
have on the image canvas at the location of the user's click.

Finally, a software cursor is implemented.  The program disables the hardware
mouse pointer when it enters the image canvas and draws a custom cursor.  The
cursor admits the contour shape of a tinting brush.  It has two color states,
bright and dark. In bright state the cursor color is bright yellow and it seen
well on darker areas of the image.  When the cursor moved to the lighter areas
of the image its color dynamically changed to black and v.v.  When the brush
size or shape changed the cursor modified correspondingly.  Except, for the
overpainting brush it does not become rectangular cursor.


Implementation details:

The fall-off function of the tinging brush is

alpha = MAX(0, (sin(pi * x/W) + sin(pi * y/H) - 1)).

In this expression, x is the x coordinate of the array of pixels that represent
the rectangle of the brush, y is the y coordinate, W is the width and H is the
height of the brush.

The transparency change is accomplished by multiplying the alpha by a number
from the [0.1, 0.8] interval.

The software cursor is implemented by saving the original pixels of the canvas,
drawing the cursor pixels and restoring the original pixels of the canvas when
cursor moved.

The implementation is very straightforward and described in some detail in the
source file paint.c.  The implementation is in C and does not rely on any
special packages, other than xsupport.

The application was tested primarily by running it and using different controls
of the GUI.  Some debug prints are added to display inconsistent states of the
application, if they ever occur.
