/* ===========================================================================
   $Id$
 
   Project:  MapServer
   Purpose:  SWIG interface file for mapscript shapeObj extensions
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
*/


%{
#include "../../map.h"
#include "../../mapprimitive.h"
%}

%include "../../mapprimitive.h"


// extensions for shapeObj

%extend shapeObj 
{
  
    shapeObj(int type) 
    {
        shapeObj *shape;
        shape = (shapeObj *)malloc(sizeof(shapeObj));
        if (!shape)
            return NULL;

        msInitShape(shape);
        if (type >= 0) shape->type = type;

        return shape;
    }

    ~shapeObj() 
    {
        msFreeShape(self);
        free(self);		
    }

    int project(projectionObj *in, projectionObj *out) 
    {
        return msProjectShape(in, out, self);
    }

#ifdef NEXT_GENERATION_API
    lineObj *getLine(int i) {
#else
    lineObj *get(int i) {
#endif
    if (i<0 || i>=self->numlines)
        return NULL;
    else
        return &(self->line[i]);
  }

#ifdef NEXT_GENERATION_API
    int addLine(lineObj *line) {
#else
    int add(lineObj *line) {
#endif
        return msAddLine(self, line);
    }

    int draw(mapObj *map, layerObj *layer, imageObj *image) {
        return msDrawShape(map, layer, self, image, MS_TRUE);
    }

    void setBounds() 
    {    
        msComputeBounds(self);
        return;
    }

#ifdef NEXT_GENERATION_API
    %newobject copy;
    shapeObj *copy() 
    {
        shapeObj *shape;
        shape = (shapeObj *)malloc(sizeof(shapeObj));
        if (!shape)
            return NULL;
        msInitShape(shape);
        shape->type = self->type;
        msCopyShape(self, shape);
        return shape;
    }
#else
    int copy(shapeObj *dest) 
    {
        return(msCopyShape(self, dest));
    }
#endif

    char *getValue(int i) 
    { // returns an EXISTING value
        if (i >= 0 && i < self->numvalues)
            return (self->values[i]);
        else
            return NULL;
    }

    int contains(pointObj *point) 
    {
        if (self->type == MS_SHAPE_POLYGON)
            return msIntersectPointPolygon(point, self);
        
        return -1;
    }

    double distanceToPoint(pointObj *point) 
    {
        return msDistancePointToShape(point, self);
    }

    double distanceToShape(shapeObj *shape) 
    {
        return msDistanceShapeToShape(self, shape);
    }

    int intersects(shapeObj *shape) 
    {
        switch(self->type) {
            case(MS_SHAPE_LINE):
                switch(shape->type) {
                    case(MS_SHAPE_LINE):
	                    return msIntersectPolylines(self, shape);
                    case(MS_SHAPE_POLYGON):
	                    return msIntersectPolylinePolygon(self, shape);
                }
                break;
            case(MS_SHAPE_POLYGON):
                switch(shape->type) {
                    case(MS_SHAPE_LINE):
	                    return msIntersectPolylinePolygon(shape, self);
                    case(MS_SHAPE_POLYGON):
	                    return msIntersectPolygons(self, shape);
                }
                break;
            }

        return -1;
    }
    
}

