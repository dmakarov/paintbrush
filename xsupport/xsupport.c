/* SIMPLIFIED MOTIF INTERFACE (XSUPPORT PACKAGE).

   Created 4/94 by Apostolos Lerios - tolis@cs. 
   Modified 1/95 by Chase Garfinkle - chase@cs. 
   Modified 10/00 by Hiroshi Ishii - hishii@cs.
    - fixed red-blue byte swapping on Linux
    - added support for 15 and 16 bit displays
*/

#include "xsupport.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "Xm/RowColumn.h"
#include "Xm/PushB.h"
#include "Xm/LabelG.h"
#include "Xm/ToggleB.h"
#include "Xm/Scale.h"
#include "Xm/FileSB.h"
#include "Xm/DrawingA.h"
#include "Xm/Protocols.h"
#include "Xm/AtomMgr.h"
#ifdef __cplusplus
}
#endif
#include "renderer.icon"
#include "canvas.icon"

#include "blank.cursor"


/* SYSTEM CONFIGURATION. */

/* Test image display mode. */

static int TestImage=0;

/* Monitor gamma (8-bit mode only). */

static double Gamma=2.0;

/* Number of shades for each primary (8-bit mode only). */

static const int Reds=6;
static const int Greens=8;
static const int Blues=5;

static const int Shades=Reds*Greens*Blues;

/* Dithering pattern (8-bit mode only). */

static const int DitheringPattern[4][4]={
  {  0,  8,  2, 10},
  { 12,  4, 14,  6},
  {  3, 11,  1,  9},
  { 15,  7, 13,  5}
};

/* Number of buttons per row. */

static const int ButtonsPerRow=6;


/* GLOBAL VARIABLES. */

/* X toolkit application context. */

static XtAppContext AppContext;

/* X display. */

static Display *Disp;

/* Graphics context used to draw on canvases. */

static GC Gc;

/* Top-level shell widget */

static Widget Shell=0;

/* Flag indicating whether LiftOff() has been invoked. */

static int MainLoopStarted=0;

/* Number of bits per pixel. */

static int BitPlanes;

/* Dithered primary intensities (8-bit mode only). */

typedef int Intensities[256][4][4];

static Intensities RedIs;
static Intensities GreenIs;
static Intensities BlueIs;

/* Single-primary mode intensities (8-bit mode only). */

static int SingleIntensities[256];

/* Blank hardware cursor so software cursor can be defined */

static Cursor BlankCurs = 0;

/* PRIVATE WIDGET DATA AND ACCESSORS. */

/* Choice buttons. */

typedef struct {
  Widget Handle;
} ChoiceButtonExtension;

inline ChoiceButtonExtension *CBExt(ChoiceButton *CB) {
  return (ChoiceButtonExtension *)(CB->Private); }

/* Sliders. */

typedef struct {
  Widget Handle;
} SliderExtension;

inline SliderExtension *SExt(Slider *S) {
  return (SliderExtension *)(S->Private); }

/* Canvases. */

typedef struct {
  Widget Handle;
  XImage *Image;
  XtIntervalId Timer;
  int BrushX;
  int BrushY;
  unsigned int BrushState;
  Colormap CMap;
  unsigned long Mask;

  XColor Colors[Shades]; /* 8-bit mode. */
  int GammaCorrect;
} CanvasExtension;

inline CanvasExtension *CExt(Canvas *C) {
  return (CanvasExtension *)(C->Private); }


/* 8-BIT MODE UTILITIES. */

/* Given a pixel intensity I, from 0 to ILevels-1, it returns the
approximate gamma-corrected intensity in the range from 0 to 255
inclusive. */

inline static int GammaCorrected(int I,
                                 int ILevels) {

  return int(floor(pow(I/(ILevels-1.0),1.0/Gamma)*255.0+0.5));
}

/* Given a pixel intensity I, from 0 to ILevels-1, it returns the
approximate linearly-scaled intensity in the range from 0 to 255
inclusive. */

inline static int LinearScale(int I,
                              int ILevels) {

  return int(floor(I/(ILevels-1.0)*255.0+0.5));
}

/* Initializes the dithered primary intensity array Is such that
Is[I][X%4][Y%4] yields the scaled intensity (in the range from 0 to
ILevels-1) for a pixel with intensity I (in the range from 0 to 255),
and positioned at screen coordinates (X,Y). */

static void InitDitheredIntensities(int ILevels,
                                    Intensities Is) {

  int PatternCount=16*(ILevels-1)+1;
  for (int i=0;i<256;i++) {
    int Level=int(floor(i/255.0*(PatternCount-1.0)+0.5));
    int Threshold=Level%16;
    int AboveThreshold=Level/16;
    int BelowThreshold=AboveThreshold+1;
    for (int X=0;X<4;X++)
      for (int Y=0;Y<4;Y++)
        if (DitheringPattern[X][Y]>=Threshold)
          Is[i][X][Y]=AboveThreshold;
        else
          Is[i][X][Y]=BelowThreshold;
  }
}


/* WIDGET PRE-CALLBACKS AND EVENT HANDLERS. */

/* Choice buttons. */

static void ChoiceButtonPrecallback(Widget w,
                                    void (*Callback)(int),
                                    XmToggleButtonCallbackStruct *CbS) {
  (*Callback)(CbS->set);
}

/* Sliders. */

static void SliderPrecallback(Widget Slider,
                              void (*Callback)(float),
                              XmAnyCallbackStruct *) {

  short Decimals;
  int Value;
  XtVaGetValues(Slider,
                XmNvalue,&Value,
                XmNdecimalPoints,&Decimals,
                NULL);
  (*Callback)(Value/pow(10,Decimals));
}

/* Dialog buttons. */

static void ShowFileDialog(Widget,
                           Widget FileDialog,
                           XmAnyCallbackStruct *) {

  XtManageChild(FileDialog);
}

static void HideFileDialog(Widget,
                           Widget FileDialog,
                           XmAnyCallbackStruct *) {

  XtUnmanageChild(FileDialog);
}

static void DialogButtonPrecallback(Widget FileDialog,
                                    void (*Callback)(char *),
                                    XmFileSelectionBoxCallbackStruct *CbS) {

  /* Retrieve file name. */

  char *FileName;
  XmStringGetLtoR(CbS->value,XmSTRING_DEFAULT_CHARSET,&FileName);

  /* No file has been selected. */

  if (!(*FileName))
    return;

  /* In case the user has not specified the directory, we must supply it. */

  char *FileSpec;
  if ((*FileName)!='/') {
    char *Path;
    if (!XmStringGetLtoR(CbS->dir,XmSTRING_DEFAULT_CHARSET,&Path))
      return;
    FileSpec=XtMalloc(strlen(Path)+1+strlen(FileName)+1);
    sprintf(FileSpec,"%s/%s",Path,FileName);
    XtFree(Path);
    XtFree(FileName);
  } else
    FileSpec=FileName;

  /* Invoke user callback. */

  (*Callback)(FileSpec);

  /* Cleanup. */

  XtFree(FileSpec);
  XtUnmanageChild(FileDialog);
}

/* Canvases. */

static void DrawingExpose(Widget,
                          CanvasExtension *CE,
                          XmDrawingAreaCallbackStruct *CbS) {

  XPutImage(Disp,XtWindow(CE->Handle),Gc,CE->Image,
            CbS->event->xexpose.x,CbS->event->xexpose.y,
            CbS->event->xexpose.x,CbS->event->xexpose.y,
            CbS->event->xexpose.width,CbS->event->xexpose.height);
}

static void SetColormap(CanvasExtension *CE) {

  /* Calculate colormap entries. */

  for (int i=0;i<Shades;i++) {
    int (*Scaler)(int,int);    
    if (CE->GammaCorrect)
      Scaler=&GammaCorrected;
    else
      Scaler=&LinearScale;
    if (CE->Mask==ALL_COLORS) {
      CE->Colors[i].red=(*Scaler)(i%Reds,Reds)<<8;
      CE->Colors[i].green=(*Scaler)((i/Reds)%Greens,Greens)<<8;
      CE->Colors[i].blue=(*Scaler)(i/(Reds*Greens),Blues)<<8;
    } else if (CE->Mask==ONLY_RED) {
      CE->Colors[i].red=(*Scaler)(i,Shades)<<8;
      CE->Colors[i].green=CE->Colors[i].blue=0;
    } else if (CE->Mask==ONLY_GREEN) {
      CE->Colors[i].green=(*Scaler)(i,Shades)<<8;
      CE->Colors[i].red=CE->Colors[i].blue=0;
    } else if (CE->Mask==ONLY_BLUE) {
      CE->Colors[i].blue=(*Scaler)(i,Shades)<<8;
      CE->Colors[i].red=CE->Colors[i].green=0;
    }   
  }

  /* Update server. */

  XStoreColors(Disp,CE->CMap,CE->Colors,Shades);
}

static void RedrawCanvasImage(Canvas *C,
                              int FromX,
                              int ToX,
                              int FromY,
                              int ToY) {

  CanvasExtension *CE=CExt(C);

  int SwapRedAndBlue = CE->Image->red_mask != ONLY_RED;

  for (int Y=FromY;Y<=ToY;Y++)
    for (int X=FromX;X<=ToX;X++)
      if (TestImage)
        XPutPixel(CE->Image,X,Y,X%(1<<BitPlanes));
      else if (BitPlanes==24) {
        if(SwapRedAndBlue) {
          unsigned p = (CE->Mask)&PIXEL(C,X,Y);
          unsigned p1 = (GET_RED(p) << 16) | (GET_GREEN(p) << 8) | GET_BLUE(p);
          XPutPixel(CE->Image,X,Y,p1);
        } else {
          XPutPixel(CE->Image,X,Y,(CE->Mask)&PIXEL(C,X,Y));
        }
      } else if (BitPlanes==16) {
        unsigned p = (CE->Mask)&PIXEL(C,X,Y);
        unsigned p_r = GET_RED(p) >> 3;
        unsigned p_g = GET_GREEN(p) >> 2;
        unsigned p_b = GET_BLUE(p) >> 3;
        unsigned short p1 = (p_r << 11) | (p_g << 5) | (p_b);
        XPutPixel(CE->Image,X,Y,p1);
      } else if (BitPlanes==15) {
        unsigned p = (CE->Mask)&PIXEL(C,X,Y);
        unsigned p_r = GET_RED(p) >> 3;
        unsigned p_g = GET_GREEN(p) >> 3;
        unsigned p_b = GET_BLUE(p) >> 3;
        unsigned short p1 = (p_r << 10) | (p_g << 5) | (p_b);
        XPutPixel(CE->Image,X,Y,p1);
      } else {
        unsigned long Pixel=PIXEL(C,X,Y);
        int CMapEntry;
        if (CE->Mask==ALL_COLORS) {
          int Red=RedIs[GET_RED(Pixel)][X%4][Y%4];
          int Green=GreenIs[GET_GREEN(Pixel)][X%4][Y%4];
          int Blue=BlueIs[GET_BLUE(Pixel)][X%4][Y%4];
          CMapEntry=Blue*Reds*Greens+Green*Reds+Red;
        } else if (CE->Mask==ONLY_RED)
          CMapEntry=SingleIntensities[GET_RED(Pixel)];
        else if (CE->Mask==ONLY_GREEN)
          CMapEntry=SingleIntensities[GET_GREEN(Pixel)];
        else
          CMapEntry=SingleIntensities[GET_BLUE(Pixel)];
        XPutPixel(CE->Image,X,Y,CE->Colors[CMapEntry].pixel);
      }
}

static void AirbrushPuff(Canvas *C,
                         XtIntervalId *) {

  CanvasExtension *CE=CExt(C);
  (*(C->Callback))(CE->BrushX,CE->BrushY,CE->BrushState);
  if (C->PuffInterval>=0)
    CE->Timer=XtAppAddTimeOut(AppContext,C->PuffInterval,
                              XtTimerCallbackProc(AirbrushPuff),XtPointer(C));
}

static void DrawingPointerMotion(Widget,
                                 Canvas *C,
                                 XEvent *Event,
                                 Boolean *) {

  /* Dequeue upcoming motion events to avoid "swimming". */

  XEvent NextEvent;
  while ((XtAppPending(AppContext)&XtIMXEvent) &&
         XtAppPeekEvent(AppContext,&NextEvent) &&
         NextEvent.type==MotionNotify &&
         NextEvent.xmotion.state==Event->xmotion.state)
    XtAppNextEvent(AppContext,Event);

  /* Retain brush location. */

  CanvasExtension *CE=CExt(C);
  CE->BrushX=Event->xmotion.x;
  CE->BrushY=Event->xmotion.y;
  
  /* Invoke user callback. */

  (*(C->Callback))(CE->BrushX,CE->BrushY,CE->BrushState);
}

static void DrawingButtonPress(Widget,
                               Canvas *C,
                               XButtonEvent *Event,
                               Boolean *) {

  if (Event->button==Button1) {
    CanvasExtension *CE=CExt(C);
    CE->BrushX=Event->x;
    CE->BrushY=Event->y;
    CE->BrushState=Event->state|Button1Mask;
    AirbrushPuff(C,0);
  }
}

static void DrawingButtonRelease(Widget,
                                 Canvas *C,
                                 XButtonEvent *Event,
                                 Boolean *) {

  if (Event->button==Button1) {
    if (CExt(C)->Timer) {
      XtRemoveTimeOut(CExt(C)->Timer);
      CExt(C)->Timer=0;
    }
    CExt(C)->BrushState=0;
    (*(C->Callback))(Event->x,Event->y,0);
  }
}

static void DrawingCross(Widget,
                         Canvas *C,
                         XCrossingEvent *Event,
                         Boolean *) {

  CanvasExtension *CE=CExt(C);

  /* In the case of overlapping windows, the cursor might exit the
canvas window within the canvas boundaries. In this case, and if there
is no chance of an identical event being called at some later point in
time due to button release, notify the client. */

  if (Event->type==LeaveNotify && !(CE->BrushState)) 
    (*(C->Callback))(-1,-1,0);
  else {
    CE->BrushX=Event->x;
    CE->BrushY=Event->y;
    (*(C->Callback))(Event->x,Event->y,CE->BrushState);
  }
}


/* EXTERNAL INTERFACE. */

void LiftOff(int *argc,
             char **argv,
             PushButton *PushButtons,
             DialogButton *DialogButtons,
             ChoiceButtonSet *ChoiceButtonSets,
             Slider *Sliders,
             Canvas *Canvases) {


  /* ERROR CHECKING. */

  if (MainLoopStarted) {
    fprintf(stderr,"LiftOff() cannot be called twice!\n");
    return;
  }


  /* COMMAND-LINE ARGUMENTS. */

  int Use8Bit=0;
  for (int i=1;i<*argc;i++) {
    if (!strcmp(argv[i],"-help")) {
      fprintf(stderr,"xsupport package options (defaults in parens):\n");
      fprintf(stderr,"\n");
      fprintf(stderr,"-help: displays this screen (No).\n");
      fprintf(stderr,"-gamma: sets monitor gamma (8-bit mode only) (2.0).\n");
      fprintf(stderr,"-8bit: enforce 8-bit mode (No).\n");
      fprintf(stderr,"\n");
      fprintf(stderr,"And all the standard X command-line arguments.\n");
      fprintf(stderr,"\n");
      return;
    } else if (!strcmp(argv[i],"-gamma")) {
      if (++i==*argc) {
        fprintf(stderr,
                "-gamma should precede monitor gamma.\n");
        break;
      }
      Gamma=atof(argv[i]);
      if (Gamma<=0.0) {
        fprintf(stderr,"Monitor gamma must be positive.\n");
        exit(1);
      }
    } else if (!strcmp(argv[i],"-8bit"))
      Use8Bit=1;
  }


  /* MAIN WINDOW. */

  Shell=XtVaAppInitialize(&AppContext,argv[0],NULL,0,
			  argc,argv,NULL,
			  XmNallowShellResize,True,      
			  XmNiconName,argv[0],
			  NULL);
  Disp=XtDisplay(Shell);
  Pixmap Icon=XCreateBitmapFromData(Disp,DefaultRootWindow(Disp),renderer_bits,
                                    renderer_width,renderer_height);
  XtVaSetValues(Shell,
                XmNiconPixmap,Icon,
                NULL);
  Atom DeleteAtom=XmInternAtom(Disp,"WM_DELETE_WINDOW",False);
  Widget Main=XtVaCreateWidget("Main",xmRowColumnWidgetClass,Shell,
                               NULL);


  /* PUSH BUTTONS. */

  int ButtonCount=0;
  Widget Row=0;

  for (ButtonCount=0;PushButtons[ButtonCount].Callback;ButtonCount++)
    ;

  if (ButtonCount > 0) {
    const Cardinal rows = ButtonCount/ButtonsPerRow + 
	(ButtonCount%ButtonsPerRow != 0);
    Row=XtVaCreateManagedWidget("PushButtons",xmRowColumnWidgetClass, Main,
				XmNorientation,XmHORIZONTAL,
				XmNnumColumns,rows,
				XmNpacking,XmPACK_COLUMN,
				NULL);
  }

  for (int i=0;PushButtons[i].Callback;i++) {
    /* Create button. */

    Widget Button=
      XtVaCreateManagedWidget(PushButtons[i].Name,xmPushButtonWidgetClass,Row,
                              NULL);
    XtAddCallback(Button,XmNactivateCallback,
                  XtCallbackProc(PushButtons[i].Callback),0);
  }


  /* DIALOG BUTTONS. */

  ButtonCount = 0;

  for (ButtonCount=0;DialogButtons[ButtonCount].Callback;ButtonCount++)
    ;
  if (ButtonCount > 0) {
    const Cardinal rows = ButtonCount/ButtonsPerRow + 
	(ButtonCount%ButtonsPerRow != 0);
    Row=XtVaCreateManagedWidget("DialogButtons",xmRowColumnWidgetClass, Main,
				XmNorientation,XmHORIZONTAL,
				XmNnumColumns,rows,
				XmNpacking,XmPACK_COLUMN,
				NULL);
  }
  for (int i=0;DialogButtons[i].Callback;i++) {
    /* Create button. */

    Widget Button=
      XtVaCreateManagedWidget(DialogButtons[i].Name,xmPushButtonWidgetClass,
                              Row,NULL);

    /* Create file selection dialog. */

    Widget FileDialog=XmCreateFileSelectionDialog(Shell,"",NULL,0);
    XmString Str1=XmStringCreateSimple(DialogButtons[i].Message);
    XmString Str2=XmStringCreateSimple("File Selection");
    XtVaSetValues(FileDialog,
                  XmNselectionLabelString,Str1,
                  XmNdialogTitle,Str2,
                  NULL);
    XmStringFree(Str2);
    XmStringFree(Str1);
    XtVaSetValues(XtParent(FileDialog),
                  XmNdeleteResponse,XmDO_NOTHING,
                  NULL);
    XmAddWMProtocolCallback(XtParent(FileDialog),DeleteAtom,
                            XtCallbackProc(HideFileDialog),
                            caddr_t(FileDialog));
    XtAddCallback(FileDialog,XmNokCallback,
                  XtCallbackProc(DialogButtonPrecallback),
                  XtPointer(DialogButtons[i].Callback));
    XtAddCallback(FileDialog,XmNcancelCallback,
                  XtCallbackProc(HideFileDialog),caddr_t(FileDialog));
    XtUnmanageChild(XmFileSelectionBoxGetChild(FileDialog,
                                               XmDIALOG_HELP_BUTTON));

    /* Attach dialog to button. */
    
    XtAddCallback(Button,XmNactivateCallback,
                  XtCallbackProc(ShowFileDialog),XtPointer(FileDialog));
  }


  /* CHOICE BUTTONS. */
  Widget ChoiceSets = NULL;
  if (ChoiceButtonSets[0].Members) {
      ChoiceSets = XtVaCreateManagedWidget("ChoiceButtonSets",
					   xmRowColumnWidgetClass,
					   Main,
					   XmNorientation,XmVERTICAL,
					   NULL);
  }

  int i;
  for (i = 0;ChoiceButtonSets[i].Members;i++) {

    /* Create group. */

    Row=XtVaCreateWidget(ChoiceButtonSets[i].Name,xmRowColumnWidgetClass,
			 ChoiceSets,
                         XmNorientation,XmHORIZONTAL,
                         XmNradioBehavior,ChoiceButtonSets[i].Radio,
                         NULL);
    XtVaSetValues(Row,
                  XmNisHomogeneous,False,
                  XmNpacking,XmPACK_TIGHT,
                  NULL);
    XtVaCreateManagedWidget(ChoiceButtonSets[i].Name,xmLabelGadgetClass,Row,
                            NULL);

    /* Create buttons within group. */

    ChoiceButton *Members=ChoiceButtonSets[i].Members;
    int Set=0;
    for (int j=0;Members[j].Callback;j++) {
      Members[j].Private=
        (ChoiceButtonExtension *)(malloc(sizeof(ChoiceButtonExtension)));
      CBExt(&Members[j])->Handle=
        XtVaCreateManagedWidget(Members[j].Name,xmToggleButtonWidgetClass,Row,
                                NULL);
      if (Members[j].Set) {
        if (ChoiceButtonSets[i].Radio && Set)
          fprintf(stderr,"Cannot set multiple radio buttons in same set.\n");
        else {
          XtVaSetValues(CBExt(&Members[j])->Handle,
                        XmNset,1,
                        NULL);
          Set=1;
        }
      }
      XtAddCallback(CBExt(&Members[j])->Handle,XmNvalueChangedCallback,
                    XtCallbackProc(ChoiceButtonPrecallback),
                    XtPointer(Members[j].Callback));
    }
    XtManageChild(Row);
  }


  /* SLIDERS. */

  for (i=0;Sliders[i].Callback;i++)
    ;
  if (i > 0) {
    const Cardinal rows = i/3 + (i%3 != 0);    
    /* Create group of sliders (3 sliders per row). */

    Row=XtVaCreateManagedWidget("Sliders",xmRowColumnWidgetClass,Main,
				XmNorientation,XmHORIZONTAL,
				XmNnumColumns,rows,
				XmNpacking,XmPACK_COLUMN,
				NULL);

  }
  for (i=0;Sliders[i].Callback;i++) {
    Sliders[i].Private=(SliderExtension *)(malloc(sizeof(SliderExtension)));
    /* Create slider. */

    XmString Str=XmStringCreateSimple(Sliders[i].Name);
    SExt(&Sliders[i])->Handle=
         XtVaCreateManagedWidget("",xmScaleWidgetClass,Row,
                                 XmNmaximum,Sliders[i].Maximum,
                                 XmNminimum,Sliders[i].Minimum,
                                 XmNdecimalPoints,
                                 Sliders[i].Decimals,
                                 XmNvalue,Sliders[i].InitialValue,
                                 XmNorientation,XmHORIZONTAL,
                                 XmNtitleString,Str,
                                 XmNshowValue,True,
                                 NULL);
    XtAddCallback(SExt(&Sliders[i])->Handle,XmNvalueChangedCallback,
                  XtCallbackProc(SliderPrecallback),
                  XtPointer(Sliders[i].Callback));
    XmStringFree(Str);
  }


  /* CANVASES. */

  /* Identify appropriate visual and initialize system accordingly. */

  XVisualInfo VInfo;
  if (!Use8Bit &&
      XMatchVisualInfo(Disp,DefaultScreen(Disp),24,TrueColor,&VInfo))
    BitPlanes=24;
  else if (!Use8Bit &&
      XMatchVisualInfo(Disp,DefaultScreen(Disp),16,TrueColor,&VInfo))
    BitPlanes=16;
  else if (!Use8Bit &&
      XMatchVisualInfo(Disp,DefaultScreen(Disp),15,TrueColor,&VInfo))
    BitPlanes=15;
  else if (XMatchVisualInfo(Disp,DefaultScreen(Disp),8,PseudoColor,&VInfo)) {
    BitPlanes=8;
    InitDitheredIntensities(Reds,RedIs);
    InitDitheredIntensities(Greens,GreenIs);
    InitDitheredIntensities(Blues,BlueIs);
    for (int i=0;i<256;i++)
      SingleIntensities[i]=int(floor(i/255.0*(Shades-1.0)+0.5));
  } else {
    fprintf(stderr,"Display not supported.\n");
    exit(1);
  }

  /* Create blank cursor. */

  if (!BlankCurs) {
    Icon=XCreateBitmapFromData(Disp,DefaultRootWindow(Disp),
                               blank_bits,blank_width,blank_height);
    XColor AnyColor;
    BlankCurs=XCreatePixmapCursor(Disp,Icon,Icon,&AnyColor,&AnyColor,0,0);
  }

  /* Define canvas icon. */

  Icon=XCreateBitmapFromData(Disp,DefaultRootWindow(Disp),
                             canvas_bits,canvas_width,canvas_height);

  /* Create canvases. */

  for (i=0;Canvases[i].Callback;i++) {
    Canvases[i].Private=(CanvasExtension *)(malloc(sizeof(CanvasExtension)));
    CanvasExtension *CE=CExt(&Canvases[i]);
    CE->BrushState=0;
    CE->Timer=0;
    CE->Mask=ALL_COLORS;

    /* Create colormap. */

    CE->CMap=XCreateColormap(Disp,DefaultRootWindow(Disp),
                             VInfo.visual,AllocNone);

    /* Create canvas shell. */

    Widget Top=XtVaAppCreateShell("Canvas","Canvas",topLevelShellWidgetClass,
                                  Disp,
                                  XmNdeleteResponse,XmDO_NOTHING,
                                  XmNiconPixmap,Icon,
                                  XmNiconName,"Canvas",
                                  XmNwidth,Canvases[i].Width,
                                  XmNheight,Canvases[i].Height,
#if 0
                                  XmNallowShellResize,False,
                                  XmNminWidth,Canvases[i].Width,
                                  XmNminHeight,Canvases[i].Height,
                                  XmNmaxWidth,Canvases[i].Width,
                                  XmNmaxHeight,Canvases[i].Height,
#endif
                                  XmNvisual,VInfo.visual,
                                  XmNcolormap,CE->CMap,
                                  XmNdepth,BitPlanes,
                                  NULL);
    XmAddWMProtocolCallback(Top,DeleteAtom,XtCallbackProc(exit),0);

    /* Reserve and initialize colormap entries (8-bit mode only). */

    if (BitPlanes==8) {
      CE->GammaCorrect=1;
      unsigned long Pixels[Shades];
      if (!XAllocColorCells(Disp,CE->CMap,False,NULL,0,Pixels,Shades)) {
        fprintf(stderr,"Too many RGB shades.\n");
        exit(1);
      }
      for (int j=0;j<Shades;j++) {
        CE->Colors[j].pixel=Pixels[j];
        CE->Colors[j].flags=DoRed|DoGreen|DoBlue;
      }
      SetColormap(CE);
    }

    /* Create canvas image. */

    unsigned long *Image=
      (unsigned long *)(malloc(Canvases[i].Width*Canvases[i].Height*
                               sizeof(unsigned long)));
    if (!Image) {
      fprintf(stderr,"Not enough memory for canvas.\n");
      exit(1);
    }
    CE->Image=
      XCreateImage(Disp,VInfo.visual,BitPlanes,ZPixmap,0,(char *)(Image),
                   Canvases[i].Width,Canvases[i].Height,32,0);

    /* Create canvas. */

    CE->Handle=XtVaCreateManagedWidget("",xmDrawingAreaWidgetClass,Top,
                                       XmNwidth,Canvases[i].Width,
                                       XmNheight,Canvases[i].Height,
                                       NULL);
    XtAddCallback(CE->Handle,XmNexposeCallback,
                  XtCallbackProc(DrawingExpose),XtPointer(CE));
    XtAddEventHandler(CE->Handle,PointerMotionMask,False,
                      XtEventHandler(DrawingPointerMotion),
                      XtPointer(&Canvases[i]));
    XtAddEventHandler(CE->Handle,ButtonPressMask,False,
                      XtEventHandler(DrawingButtonPress),
                      XtPointer(&Canvases[i]));
    XtAddEventHandler(CE->Handle,ButtonReleaseMask,False,
                      XtEventHandler(DrawingButtonRelease),
                      XtPointer(&Canvases[i]));
    XtAddEventHandler(CE->Handle,EnterWindowMask,False,
                      XtEventHandler(DrawingCross),
                      XtPointer(&Canvases[i]));
    XtAddEventHandler(CE->Handle,LeaveWindowMask,False,
                      XtEventHandler(DrawingCross),
                      XtPointer(&Canvases[i]));

    /* Pop up window, and use the invisible cursor. */

    XtRealizeWidget(Top);
    if (Canvases[i].DisableHardwareCursor)
      XDefineCursor(Disp,XtWindow(Top),BlankCurs);

    /* Create graphics context. */

    if (i==0)
      Gc=XCreateGC(Disp,XtWindow(Top),0,NULL);

    /* Display initial image. */

    RedrawCanvasImage(&Canvases[i],
                      0,Canvases[i].Width-1,0,Canvases[i].Height-1);
  }


  /* MAIN WINDOW. */

  XtManageChild(Main);
  XtRealizeWidget(Shell);


  /* PASS CONTROL TO MOTIF. */

  MainLoopStarted=1;
  XtAppMainLoop(AppContext);
}

void SetChoice(ChoiceButton *CB,
               int State) {

  if (!MainLoopStarted) {
    fprintf(stderr,"Cannot set choice button before LiftOff() is called.\n");
    return;
  }
  XtVaSetValues(CBExt(CB)->Handle,
                XmNset,State,
                NULL);
}

void SetSlider(Slider *S,
               int Value) {

  if (!MainLoopStarted) {
    fprintf(stderr,"Cannot set slider before LiftOff() is called.\n");
    return;
  }
  XtVaSetValues(SExt(S)->Handle,
                XmNvalue,Value,
                NULL);
}

void UpdateCanvas(Canvas *C,
                  int FromX,
                  int ToX,
                  int FromY,
                  int ToY) {

  if (!MainLoopStarted) {
    fprintf(stderr,"Cannot update canvas before LiftOff() is called.\n");
    return;
  }
  CanvasExtension *CE=CExt(C);
  RedrawCanvasImage(C,FromX,ToX,FromY,ToY);
  XPutImage(Disp,XtWindow(CE->Handle),Gc,CE->Image,FromX,FromY,
            FromX,FromY,ToX-FromX+1,ToY-FromY+1);
}

void Flush(void) {

  XFlush(Disp);
}

void SetCanvasMode(Canvas *C,
                   CanvasMode Mode,
                   int GammaCorrect) {

  if (!MainLoopStarted) {
    fprintf(stderr,"Cannot set canvas mode before LiftOff() is called.\n");
    return;
  }
  CanvasExtension *CE=CExt(C);
  CE->Mask=Mode;
  if (BitPlanes==8) {
    CE->GammaCorrect=GammaCorrect;
    SetColormap(CE);
  }
  UpdateCanvas(C,0,C->Width-1,0,C->Height-1);
}

void ResizeCanvas(Canvas *C,
                  int NewWidth, int NewHeight) {
  if (!MainLoopStarted) {
    fprintf(stderr,"Cannot set canvas mode before LiftOff() is called.\n");
    return;
  }
  CanvasExtension *CE=CExt(C);
  char * NewXImageBuffer = (char *) malloc(NewWidth * NewHeight * 
                                           sizeof(unsigned long));
  if (!NewXImageBuffer) {
    fprintf(stderr,"Insufficient memory to allocate new canvas.\n");
    return;
  }
  unsigned long * NewBuffer = (unsigned long *) 
       malloc(NewWidth * NewHeight * sizeof(unsigned long));
  if (!NewBuffer) {
    fprintf(stderr,"Insufficient memory to allocate new canvas.\n");
    return;
  }
  
  free(CE->Image->data);
  CE->Image->data = NewXImageBuffer;
  CE->Image->bytes_per_line = NewWidth * CE->Image->bits_per_pixel / 8;
  CE->Image->width = NewWidth;
  CE->Image->height = NewHeight;
  C->Width = NewWidth;
  C->Height = NewHeight;

  C->Pixels = NewBuffer;
  XtVaSetValues(CE->Handle,
                XmNwidth,NewWidth,
                XmNheight,NewHeight,
                NULL);
  XtVaSetValues(XtParent(CE->Handle),
                XmNwidth,NewWidth,
                XmNheight,NewHeight,
                NULL);
  XResizeWindow(XtDisplay(CE->Handle), XtWindow(XtParent(CE->Handle)), 
                NewWidth, NewHeight);
}

void SetSensitive(char * container, int index, int grayed) {
    if (Shell) {
	Widget parent = XtNameToWidget(Shell, container);
	if (parent) {
	    Cardinal count;       
	    WidgetList list;
	    XtVaGetValues(parent, 
			  XmNnumChildren, &count, 
			  XmNchildren, &list, 
			  NULL);
	    if (list) {
		if (0 <= index && index < signed(count)) {
		    Widget Button = list[index];
		    XtSetSensitive(Button, !grayed);
		} else {
		    fprintf(stderr, "SetSensitive: Index for sensitivity out of range\n");
		}
	    } else {
		fprintf(stderr, "SetSensitive: Couldn't find widge list for sensitivity\n");
	    }
	} else { 
	    fprintf(stderr, "SetSensitive: Couldn't find %s container\n", container);
	}
    } else {
	fprintf(stderr, "SetSensitive: no shell widget\n");
    }
}
