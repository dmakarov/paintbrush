/* PPM FILE READER.

   Chase Garfinkle - chase@cs
   Apostolos Lerios - tolis@cs.   */


#include "xsupport.h"

#include <stdlib.h>
#include <stdio.h>


/* EXTERNAL INTERFACE. */

int LoadCanvas(char *Filename,
	       Canvas *C) {

  /* Open file. */

  FILE *Input=fopen(Filename,"r");
  if (!Input)
    return 0;

  /* Read magic number. */

  fscanf(Input,"P6\n");

  /* Comment line(s). */

  int Data;
  while ((Data=getc(Input))=='#')
    fscanf(Input,"%*[^\n]\n");
  ungetc(Data,Input);

  /* Image dimensions. */

  if (fscanf(Input,"%d %d",&C->Width,&C->Height)!=2 ||
      C->Width==0 || C->Height==0)
    return 0;

  /* Maximum value in image. */

  fscanf(Input,"%*d%*c");

  /* Allocate space for the image. */

  int Size=C->Width*C->Height;
  C->Pixels=(unsigned long *)(malloc(Size*sizeof(unsigned long)));

  /* Load image. */

  unsigned long *Buffer=C->Pixels;
  for (int i=0;i<Size;i++,Buffer++) {
    char Data;
    fread(&Data,1,1,Input);
    SET_RED(*Buffer,Data);
    fread(&Data,1,1,Input);
    SET_GREEN(*Buffer,Data);
    fread(&Data,1,1,Input);
    SET_BLUE(*Buffer,Data);
  }
  fclose(Input);
  return 1;
}

int SaveCanvas(char *Filename,
	       Canvas *C) {

  /* Open file. */

  FILE *Output=fopen(Filename,"w");
  if (!Output)
    return 0;

  /* Print header. */
    
  fprintf(Output,"P6\n");
  fprintf(Output,"# Comment Line\n");
  fprintf(Output,"%d %d\n",C->Width,C->Height);
  fprintf(Output,"255\n");

  /* Save image. */

  int Size=C->Width*C->Height;
  unsigned long *Buffer=C->Pixels;
  for (int i=0;i<Size;i++,Buffer++) {
    char Data=char(GET_RED(*Buffer));
    fwrite(&Data,1,1,Output);
    Data=char(GET_GREEN(*Buffer));
    fwrite(&Data,1,1,Output);
    Data=char(GET_BLUE(*Buffer));
    fwrite(&Data,1,1,Output);
  }
  fclose(Output);
  return 1;
}
