/* ===========================================================================
   $Id$
 
   Project:  MapServer
   Purpose:  SWIG interface file for the MapServer mapscript module
   Author:   Steve Lime 
             Sean Gillies, sgillies@frii.com
             
   ===========================================================================
   Copyright (c) 1996-2001 Regents of the University of Minnesota.
   
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:
 
   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.
 
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
   ===========================================================================

   Keep an eye out for changes in this file -- we are beginning to move
   class extension defintions to their own files in mapscript/swiginc.  See
   for example hashtable.i and owsrequest.i.

   ===========================================================================
*/
   
// language specific initialization
#ifdef SWIGTCL8
%module Mapscript
%{

/* static global copy of Tcl interp */
static Tcl_Interp *SWIG_TCL_INTERP;

%}

%init %{
#ifdef USE_TCL_STUBS
  if (Tcl_InitStubs(interp, "8.1", 0) == NULL) {
    return TCL_ERROR;
  }
  /* save Tcl interp pointer to be used in getImageToVar() */
  SWIG_TCL_INTERP = interp;
#endif
%}
#endif

%module mapscript
%{
#include "../../map.h"
#include "../../maptemplate.h"
#include "../../mapogcsld.h"
#include "../../mapows.h"
#include "../../cgiutil.h"
#include "../../mapcopy.h"
%}

%include typemaps.i
%include constraints.i


/******************************************************************************
   Supporting 'None' as an argument to attribute accessor functions
 ******************************************************************************
 *
   Typemaps to support NULL in attribute accessor functions
   provided to Steve Lime by David Beazley and tested for Python
   only by Sean Gillies.
 *
   With the use of these typemaps and some filtering on the mapscript
   wrapper code to change the argument parsing of *_set() methods from
   "Os" to "Oz", we can execute statements like
 *
     layer.group = None
 *
 *****************************************************************************/

#ifdef __cplusplus
%typemap(memberin) char * {
  if ($1) delete [] $1;
  if ($input) {
     $1 = ($1_type) (new char[strlen($input)+1]);
     strcpy((char *) $1,$input);
  } else {
     $1 = 0;
  }
}
%typemap(memberin,warning="451:Setting const char * member may leak
memory.") const char * {
  if ($input) {
     $1 = ($1_type) (new char[strlen($input)+1]);
     strcpy((char *) $1,$input);
  } else {
     $1 = 0;
  }
}
%typemap(globalin) char * {
  if ($1) delete [] $1;
  if ($input) {
     $1 = ($1_type) (new char[strlen($input)+1]);
     strcpy((char *) $1,$input);
  } else {
     $1 = 0;
  }
}
%typemap(globalin,warning="451:Setting const char * variable may leak
memory.") const char * {
  if ($input) {
     $1 = ($1_type) (new char[strlen($input)+1]);
     strcpy((char *) $1,$input);
  } else {
     $1 = 0;
  }
}
#else
%typemap(memberin) char * {
  if ($1) free((char*)$1);
  if ($input) {
     $1 = ($1_type) malloc(strlen($input)+1);
     strcpy((char*)$1,$input);
  } else {
     $1 = 0;
  }
}
%typemap(memberin,warning="451:Setting const char * member may leak
memory.") const char * {
  if ($input) {
     $1 = ($1_type) malloc(strlen($input)+1);
     strcpy((char*)$1,$input);
  } else {
     $1 = 0;
  }
}
%typemap(globalin) char * {
  if ($1) free((char*)$1);
  if ($input) {
     $1 = ($1_type) malloc(strlen($input)+1);
     strcpy((char*)$1,$input);
  } else {
     $1 = 0;
  }
}
%typemap(globalin,warning="451:Setting const char * variable may leak
memory.") const char * {
  if ($input) {
     $1 = ($1_type) malloc(strlen($input)+1);
     strcpy((char*)$1,$input);
  } else {
     $1 = 0;
  }
}
#endif // __cplusplus

// Python-specific module code included here
#ifdef SWIGPYTHON
%{
#include "pygdioctx/pygdioctx.h"
%}
%include "pymodule.i"
#endif // SWIGPYTHON

// Ruby-specific module code included here
#ifdef SWIGRUBY
%include "rbmodule.i"
#endif

// Next Generation class names
#ifdef NEXT_GENERATION_NAMES
%rename(Map) map_obj;
%rename(Layer) layer_obj;
%rename(Class) class_obj;
%rename(Style) styleObj;
%rename(Image) imageObj;
%rename(Point) pointObj;
%rename(Line) lineObj;
%rename(Shape) shapeObj;
%rename(OutputFormat) outputFormatObj;
%rename(Symbol) symbolObj;
%rename(Color) colorObj;
%rename(Rect) rectObj;
%rename(Projection) projectionObj;
%rename(Shapefile) shapefileObj;
%rename(SymbolSet) symbolSetObj;
%rename(FontSet) fontSetObj;
#endif

// grab mapserver declarations to wrap
%include "../../mapprimitive.h"
%include "../../mapshape.h"
%include "../../mapproject.h"
%include "../../map.h"
%include "../../mapows.h"

// try wrapping mapsymbol.h
%include "../../mapsymbol.h"

// wrap the errorObj and a few functions
%include "../../maperror.h"

%apply Pointer NONNULL { mapObj *map };
%apply Pointer NONNULL { layerObj *layer };

// Map zooming convenience methods included here
%include "../swiginc/mapzoom.i"

// Language-specific extensions to mapserver classes are included here

#ifdef SWIGPYTHON
%include "pyextend.i"
#endif //SWIGPYTHON

#ifdef SWIGRUBY
%include "rbextend.i"
#endif

// A few things neccessary for automatically wrapped functions
%newobject msGetErrorString;

//
// class extensions for errorObj
//
%extend errorObj {
  errorObj() {    
    return msGetErrorObj();
  }

  ~errorObj() {}
 
  errorObj *next() {
    errorObj *ep;	

    if(self == NULL || self->next == NULL) return NULL;

    ep = msGetErrorObj();
    while(ep != self) {
      // We reached end of list of active errorObj and didn't find the errorObj... this is bad!
      if(ep->next == NULL) return NULL;
      ep = ep->next;
    }
    
    return ep->next;
  }
}

// mapObj extensions have been moved to swiginc/map.i
%include "../swiginc/map.i"


/* Full support for symbols and addition of them to the map symbolset
   is done to resolve MapServer bug 579
   http://mapserver.gis.umn.edu/bugs/show_bug.cgi?id=579 */

%extend symbolObj {
    symbolObj(char *symbolname, const char *imagefile=NULL) {
        symbolObj *symbol;
        symbol = (symbolObj *) malloc(sizeof(symbolObj));
        initSymbol(symbol);
        symbol->name = strdup(symbolname);
        if (imagefile) {
            msLoadImageSymbol(symbol, imagefile);
        }
        return symbol;
    }

    ~symbolObj() {
        if (self->name) free(self->name);
        if (self->img) gdImageDestroy(self->img);
        if (self->font) free(self->font);
        if (self->imagepath) free(self->imagepath);
    }

    int setPoints(lineObj *line) {
        int i;
        for (i=0; i<line->numpoints; i++) {
            MS_COPYPOINT(&(self->points[i]), &(line->point[i]));
        }
        self->numpoints = line->numpoints;
        return self->numpoints;
    }

    %newobject getPoints;
    lineObj *getPoints() {
        int i;
        lineObj *line;
        line = (lineObj *) malloc(sizeof(lineObj));
        line->point = (pointObj *) malloc(sizeof(pointObj)*(self->numpoints));
        for (i=0; i<self->numpoints; i++) {
            line->point[i].x = self->points[i].x;
            line->point[i].y = self->points[i].y;
        }
        line->numpoints = self->numpoints;
        return line;
    }

    int setStyle(int index, int value) {
        if (index < 0 || index > MS_MAXSTYLELENGTH) {
            msSetError(MS_SYMERR, "Can't set style at index %d.", "setStyle()", index);
            return MS_FAILURE;
        }
        self->style[index] = value;
        return MS_SUCCESS;
    }

}

%extend symbolSetObj {

    symbolSetObj(const char *symbolfile=NULL) {
        symbolSetObj *symbolset;
        mapObj *temp_map=NULL;
        symbolset = (symbolSetObj *) malloc(sizeof(symbolSetObj));
        msInitSymbolSet(symbolset);
        if (symbolfile) {
            symbolset->filename = strdup(symbolfile);
            temp_map = msNewMapObj();
            msLoadSymbolSet(symbolset, temp_map);
            symbolset->map = NULL;
            msFreeMap(temp_map);
        }
        return symbolset;
    }
   
    ~symbolSetObj() {
        msFreeSymbolSet(self);
    }

    symbolObj *getSymbol(int i) {
        if(i >= 0 && i < self->numsymbols)	
            return (symbolObj *) &(self->symbol[i]);
        else
            return NULL;
    }

    symbolObj *getSymbolByName(char *symbolname) {
        int i;

        if (!symbolname) return NULL;

        i = msGetSymbolIndex(self, symbolname, MS_TRUE);
        if (i == -1)
            return NULL; // no such symbol
        else
            return (symbolObj *) &(self->symbol[i]);
    }

    int index(char *symbolname) {
        return msGetSymbolIndex(self, symbolname, MS_TRUE);
    }

    int appendSymbol(symbolObj *symbol) {
        return msAppendSymbol(self, symbol);
    }
 
    %newobject removeSymbol;
    symbolObj *removeSymbol(int index) {
        return (symbolObj *) msRemoveSymbol(self, index);
    }

    int save(const char *filename) {
        return msSaveSymbolSet(self, filename);
    }

}

// class extensions for layerObj, always within the context of a map,
// have been moved to swiginc/layer.i

%include "../swiginc/layer.i"
%include "../swiginc/class.i"
%include "../swiginc/style.i"
%include "../swiginc/rect.i"
%include "../swiginc/image.i"
%include "../swiginc/shape.i"

//
// class extensions for pointObj, useful many places
//
%extend pointObj {
  pointObj(double x=0.0, double y=0.0, double m=2e-38) {
      pointObj *p;
      p = (pointObj *)malloc(sizeof(pointObj));
      if (!p) return NULL;
      p->x = x;
      p->y = y;
      if (m > 1e-38)
          p->m = m;
      return p;
  }

  ~pointObj() {
    free(self);
  }

  int project(projectionObj *in, projectionObj *out) {
    return msProjectPoint(in, out, self);
  }	

  int draw(mapObj *map, layerObj *layer, imageObj *image, int classindex, char *text) {
    return msDrawPoint(map, layer, self, image, classindex, text);
  }

  double distanceToPoint(pointObj *point) {
    return msDistancePointToPoint(self, point);
  }

  double distanceToSegment(pointObj *a, pointObj *b) {
    return msDistancePointToSegment(self, a, b);
  }

  double distanceToShape(shapeObj *shape) {
    return msDistancePointToShape(self, shape);
  }

  int setXY(double x, double y, double m=2e-38) {
    self->x = x;
    self->y = y;
    if (m > 1e-38)
      self->m = m;
    return MS_SUCCESS;
  }
}

//
// class extensions for lineObj (eg. a line or group of points), useful many places
//
%extend lineObj {
  lineObj() {
    lineObj *line;

    line = (lineObj *)malloc(sizeof(lineObj));
    if(!line)
      return(NULL);

    line->numpoints=0;
    line->point=NULL;

    return line;
  }

  ~lineObj() {
    free(self->point);
    free(self);		
  }

  int project(projectionObj *in, projectionObj *out) {
    return msProjectLine(in, out, self);
  }

#ifdef NEXT_GENERATION_API
  pointObj *getPoint(int i) {
#else
  pointObj *get(int i) {
#endif
    if(i<0 || i>=self->numpoints)
      return NULL;
    else
      return &(self->point[i]);
  }

#ifdef NEXT_GENERATION_API
  int addPoint(pointObj *p) {
#else
  int add(pointObj *p) {
#endif
    if(self->numpoints == 0) { /* new */	
      self->point = (pointObj *)malloc(sizeof(pointObj));      
      if(!self->point)
	return MS_FAILURE;
    } else { /* extend array */
      self->point = (pointObj *)realloc(self->point, sizeof(pointObj)*(self->numpoints+1));
      if(!self->point)
	return MS_FAILURE;
    }

    self->point[self->numpoints].x = p->x;
    self->point[self->numpoints].y = p->y;
    self->numpoints++;

    return MS_SUCCESS;
  }

#ifdef NEXT_GENERATION_API
  int setPoint(int i, pointObj *p) {
#else
  int set(int i, pointObj *p) {
#endif
    if(i<0 || i>=self->numpoints) // invalid index
      return MS_FAILURE;

    self->point[i].x = p->x;
    self->point[i].y = p->y;
    return MS_SUCCESS;    
  }
}


//
// class extensions for shapefileObj
//
%extend shapefileObj {
  shapefileObj(char *filename, int type=-1) {    
    shapefileObj *shapefile;
    int status;

    shapefile = (shapefileObj *)malloc(sizeof(shapefileObj));
    if(!shapefile)
      return NULL;

    if(type == -1)
      status = msSHPOpenFile(shapefile, "rb", filename);
    else if(type == -2)
      status = msSHPOpenFile(shapefile, "rb+", filename);
    else
      status = msSHPCreateFile(shapefile, filename, type);

    if(status == -1) {
      msSHPCloseFile(shapefile);
      free(shapefile);
      return NULL;
    }
 
    return(shapefile);
  }

  ~shapefileObj() {
    msSHPCloseFile(self);
    free(self);  
  }

  int get(int i, shapeObj *shape) {
    if(i<0 || i>=self->numshapes)
      return MS_FAILURE;

    msFreeShape(shape); /* frees all lines and points before re-filling */
    msSHPReadShape(self->hSHP, i, shape);

    return MS_SUCCESS;
  }

    %newobject getShape;
    shapeObj *getShape(int i)
    {
        int retval;
        shapeObj *shape;
        shape = (shapeObj *)malloc(sizeof(shapeObj));
        if (!shape)
            return NULL;
        msInitShape(shape);
        shape->type = self->type;
        msSHPReadShape(self->hSHP, i, shape);
        return shape;

    }

  int getPoint(int i, pointObj *point) {
    if(i<0 || i>=self->numshapes)
      return MS_FAILURE;

    msSHPReadPoint(self->hSHP, i, point);
    return MS_SUCCESS;
  }

  int getTransformed(mapObj *map, int i, shapeObj *shape) {
    if(i<0 || i>=self->numshapes)
      return MS_FAILURE;

    msFreeShape(shape); /* frees all lines and points before re-filling */
    msSHPReadShape(self->hSHP, i, shape);
    msTransformShapeToPixel(shape, map->extent, map->cellsize);

    return MS_SUCCESS;
  }

  void getExtent(int i, rectObj *rect) {
    msSHPReadBounds(self->hSHP, i, rect);
  }

  int add(shapeObj *shape) {
    return msSHPWriteShape(self->hSHP, shape);	
  }	

  int addPoint(pointObj *point) {    
    return msSHPWritePoint(self->hSHP, point);	
  }
}


// 
// class extensions for outputFormatObj
//
%extend outputFormatObj {
    outputFormatObj(const char *driver, char *name=NULL) {
    outputFormatObj *format;

    format = msCreateDefaultOutputFormat(NULL, driver);
    if( format != NULL ) 
        format->refcount++;
    if (name != NULL)
        format->name = strdup(name);
    
    return format;
  }

  ~outputFormatObj() {
    if( --self->refcount < 1 )
      msFreeOutputFormat( self );
  }

  void setExtension( const char *extension ) {
    msFree( self->extension );
    self->extension = strdup(extension);
  }

  void setMimetype( const char *mimetype ) {
    msFree( self->mimetype );
    self->mimetype = strdup(mimetype);
  }

  void setOption( const char *key, const char *value ) {
    msSetOutputFormatOption( self, key, value );
  }

    %newobject getOption;
    char *getOption(const char *key, const char *value="") {
        const char *retval;
        retval = msGetOutputFormatOption(self, key, value);
        return strdup(retval);
    }
}

//
// class extensions for projectionObj
//
%extend projectionObj {
  projectionObj(char *string) {
    int status;
    projectionObj *proj=NULL;

    proj = (projectionObj *)malloc(sizeof(projectionObj));
    if(!proj) return NULL;
    msInitProjection(proj);

    status = msLoadProjectionString(proj, string);
    if(status == -1) {
      msFreeProjection(proj);
      free(proj);
      return NULL;
    }

    return proj;
  }

  ~projectionObj() {
    msFreeProjection(self);
    free(self);		
  }
}


//
// class extensions for labelCacheObj - TP mods
//
%extend labelCacheObj {
  void freeCache() {
    msFreeLabelCache(self);    
  }
}

//
// class extensions for DBFInfo - TP mods
//
%extend DBFInfo {
    char *getFieldName(int iField) {
        static char pszFieldName[1000];
	int pnWidth;
	int pnDecimals;
	msDBFGetFieldInfo(self, iField, &pszFieldName[0], &pnWidth, &pnDecimals);
	return pszFieldName;
    }

    int getFieldWidth(int iField) {
        char pszFieldName[1000];
	int pnWidth;
	int pnDecimals;
	msDBFGetFieldInfo(self, iField, &pszFieldName[0], &pnWidth, &pnDecimals);
	return pnWidth;
    }

    int getFieldDecimals(int iField) {
        char pszFieldName[1000];
	int pnWidth;
	int pnDecimals;
	msDBFGetFieldInfo(self, iField, &pszFieldName[0], &pnWidth, &pnDecimals);
	return pnDecimals;
    }

    int getFieldType(int iField) {
	return msDBFGetFieldInfo(self, iField, NULL, NULL, NULL);
    }    
}

// Color
%include "../swiginc/color.i"

// OWSRequest
%include "../swiginc/owsrequest.i"

// HashTable
%include "../swiginc/hashtable.i"

