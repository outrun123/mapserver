/**********************************************************************
 * $Id$
 *
 * Name:     mapogr.cpp
 * Project:  MapServer
 * Language: C++
 * Purpose:  OGR Link
 * Author:   Daniel Morissette, DM Solutions Group (morissette@dmsolutions.ca)
 *
 **********************************************************************
 * Copyright (c) 2000, 2001, Daniel Morissette, DM Solutions Group Inc
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************
 * $Log$
 * Revision 1.44  2002/02/20 19:49:22  frank
 * fixed memory leak of OGRFeatures under some circumstanes
 *
 * Revision 1.43  2002/02/08 21:29:18  frank
 * ensure 3D geometries supported as well as 2D geometries
 *
 * Revision 1.42  2002/01/15 01:00:51  dan
 * Attempt at fixing thick polygon outline problem (bug 92).  Also rewrote
 * the symbol mapping for brushes and pens at the same time to use the same
 * rules as point symbols.
 *
 * Revision 1.41  2002/01/09 05:10:28  frank
 * avoid use of ms_error global
 *
 * Revision 1.40  2001/11/01 01:15:00  dan
 * Removed MS_LAYER_POLYLINE
 *
 * Revision 1.39  2001/10/15 19:25:02  dave
 * More access to the OpenGIS well-known text support already available
 * from Frank's OGR library. Had to add functions because current accessors
 * tailored to the needs of GDAL layers.
 * Added msLoadWKTProjectionString to mapogr.cpp and mapproject.h.
 * Added loadWKTProjection to mapscript.i in the layerObj and mapObj.
 *
 * Revision 1.38  2001/07/18 21:06:11  dan
 * Include USE_GD_FT in test for TTF fonts
 *
 * Revision 1.37  2001/07/04 19:06:39  dan
 * Styleitem AUTO: properly handle ogr-pen-1, the invisible pen.
 *
 * Revision 1.36  2001/07/03 04:55:40  dan
 * Fixed scale problem with text size, line width, etc, reading OGR Styles.
 *
 * Revision 1.35  2001/06/27 20:16:02  dan
 * Improved font and symbol name mapping for STYLEITEM AUTO
 *
 * Revision 1.34  2001/06/24 17:32:25  dan
 * Initial implementation of STYLEITEM AUTO for rendering maps using style 
 * info from the data source instead of static mapfile classes.  Still a few 
 * issues with fonts and symbols.
 *
 * Revision 1.33  2001/04/03 23:16:19  dan
 * Fixed args to calls to reprojection functions
 *
 * Revision 1.32  2001/03/22 22:45:09  dan
 * Forgot to change one instance of itemindexes to iteminfo
 *
 * Revision 1.31  2001/03/22 17:12:28  dan
 * Added msOGRLayerInitItemInfo() and msOGRLayerFreeItemInfo()
 *
 * Revision 1.30  2001/03/21 21:36:13  dan
 * Added msOGRLayerInitItemIndexes() (to keep itemindexes[] and item[] in sync)
 *
 * Revision 1.29  2001/03/21 04:40:49  sdlime
 * Fixed setting of itemindexes. Fixed TTF/FT preprocessor settings.
 *
 * Revision 1.28  2001/03/21 04:01:32  frank
 * added msOGCWKT2ProjectionObj(), update for other proj changes
 *
 * Revision 1.27  2001/03/18 17:21:13  dan
 * Set bounds member on returned shapes, and fixed filtering of incompatible
 * shape types in layers with mixed geometries.
 *
 * Revision 1.26  2001/03/15 04:48:35  dan
 * Fixed maplayer.c - had been committed with conflicts in it
 *
 * Revision 1.25  2001/03/12 19:02:46  dan
 * Added query-related stuff in PHP MapScript
 *
 * Revision 1.24  2001/03/12 16:05:07  sdlime
 * Fixed parameters for msLayerGetItems. Added preprocessor wrapper to OGR version of the same function.
 *
 * Revision 1.23  2001/03/11 22:01:32  dan
 * Implemented msOGRLayerGetItems()
 *
 * Revision 1.22  2001/03/05 23:15:46  sdlime
 * Many fixes in mapsde.c, now compiles. Switched shape index to long from int. Layer ...WhichShapes() functions now return MS_DONE if there is no overlap.
 *
 * Revision 1.21  2001/03/03 01:01:55  dan
 * Set shape->index in GetNextShape() for queries to work.
 *
 * Revision 1.20  2001/03/02 05:42:43  sdlime
 * Checking in Dan's new configure stuff. Updated code to essentially be GD versionless. Needs lot's of testing.
 *
 * Revision 1.19  2001/03/02 05:05:19  dan
 * Fixed problem reading LOCAL_CS SpatialRef. Handle as if no PROJECTION was set
 *
 * Revision 1.18  2001/03/02 03:48:08  frank
 * fixed to ensure it compiles with gdal and without ogr
 *
 * Revision 1.17  2001/03/01 22:54:07  dan
 * Test for geographic proj using IsGeographic() before conversion to PROJ4
 *
 * Revision 1.16  2001/03/01 22:37:14  dan
 * Added support for PROJECTION AUTO to use projection from dataset
 *
 * Revision 1.15  2001/02/28 04:56:39  dan
 * Support for OGR Label text, angle, size, implemented msOGRLayerGetShape
 * and reworked msLayerNextShape, added layer FILTER support, annotation, etc.
 *
 * Revision 1.14  2001/02/25 23:46:17  sdlime
 * Fully implemented FILTER option for shapefiles. Changed parameters for 
 * ...NextShapes and ...GetShapes functions.
 *
 * Revision 1.13  2001/02/23 21:58:00  dan
 * PHP MapScript working with new 3.5 stuff, but query stuff is disabled
 *
 * Revision 1.12  2001/02/20 20:48:44  dan
 * OGR support working for classified maps
 *
 * ...
 *
 * Revision 1.1  2000/08/25 18:41:05  dan
 * Added optional OGR support
 *
 **********************************************************************/

#include "map.h"
#include "mapproject.h"

#if defined(USE_OGR) || defined(USE_GDAL)
#  include "ogr_spatialref.h"
#  include "cpl_conv.h"
#  include "cpl_string.h"
#endif

#ifdef USE_OGR

#include "ogrsf_frmts.h"
#include "ogr_featurestyle.h"

typedef struct ms_ogr_layer_info_t
{
    char        *pszFname;
    int         nLayerIndex;
    OGRDataSource *poDS;
    OGRLayer    *poLayer;
    OGRFeature  *poLastFeature;
} msOGRLayerInfo;


/* ==================================================================
 * Geometry conversion functions
 * ================================================================== */

/**********************************************************************
 *                     ogrPointsAddPoint()
 **********************************************************************/
static void ogrPointsAddPoint(lineObj *line, double dX, double dY, 
                              int lineindex, rectObj *bounds)
{
    /* Keep track of shape bounds */
    if (line->numpoints == 0 && lineindex == 0)
    {
        bounds->minx = bounds->maxx = dX;
        bounds->miny = bounds->maxy = dY;
    }
    else
    {
        if (dX < bounds->minx)  bounds->minx = dX;
        if (dX > bounds->maxx)  bounds->maxx = dX;
        if (dY < bounds->miny)  bounds->miny = dY;
        if (dY > bounds->maxy)  bounds->maxy = dY;
    }

    line->point[line->numpoints].x = dX;
    line->point[line->numpoints].y = dY;
    line->numpoints++;
}

/**********************************************************************
 *                     ogrGeomPoints()
 **********************************************************************/
static int ogrGeomPoints(OGRGeometry *poGeom, shapeObj *outshp) 
{
  int   i;
  int   numpoints;
  lineObj line={0,NULL};

  if (poGeom == NULL)   
      return 0;

/* ------------------------------------------------------------------
 * Count total number of points
 * ------------------------------------------------------------------ */
  if (poGeom->getGeometryType() == wkbPoint
      || poGeom->getGeometryType() == wkbPoint25D )
  {
      numpoints = 1;
  }
  else if (poGeom->getGeometryType() == wkbLineString
           || poGeom->getGeometryType() == wkbLineString25D )
  {
      numpoints = ((OGRLineString*)poGeom)->getNumPoints();
  }
  else if (poGeom->getGeometryType() == wkbPolygon
           || poGeom->getGeometryType() == wkbPolygon25D )
  {
      OGRPolygon *poPoly = (OGRPolygon *)poGeom;
      OGRLinearRing *poRing = poPoly->getExteriorRing();

      numpoints=0;
      if (poRing)
          numpoints = poRing->getNumPoints();

      for (int iRing=0; iRing < poPoly->getNumInteriorRings(); iRing++)
      {
          poRing = poPoly->getInteriorRing(iRing);
          if (poRing)
              numpoints += poRing->getNumPoints();
      }
  }
  else
  {
      msSetError(MS_OGRERR, 
                 (char*)CPLSPrintf("OGRGeometry type `%s' not supported yet.", 
                                   poGeom->getGeometryName()),
                 "ogrGeomPoints()");
      return(-1);
  }

/* ------------------------------------------------------------------
 * alloc buffer and filter/transform points
 * ------------------------------------------------------------------ */
  line.numpoints = 0;
  line.point = (pointObj *)malloc(sizeof(pointObj)*numpoints);
  if(!line.point) 
  {
      msSetError(MS_MEMERR, "Unable to allocate temporary point cache.", 
                 "ogrGeomPoints()");
      return(-1);
  }
   
  if (poGeom->getGeometryType() == wkbPoint)
  {
      OGRPoint *poPoint = (OGRPoint *)poGeom;
      ogrPointsAddPoint(&line, poPoint->getX(), poPoint->getY(), 
                        outshp->numlines, &(outshp->bounds));
  }
  else if (poGeom->getGeometryType() == wkbLineString)
  {
      OGRLineString *poLine = (OGRLineString *)poGeom;
      for(i=0; i<numpoints; i++)
          ogrPointsAddPoint(&line, poLine->getX(i), poLine->getY(i),
                            outshp->numlines, &(outshp->bounds));
  }
  else if (poGeom->getGeometryType() == wkbPolygon)
  {
      OGRPolygon *poPoly = (OGRPolygon *)poGeom;
      OGRLinearRing *poRing;

      for (int iRing=-1; iRing < poPoly->getNumInteriorRings(); iRing++)
      {
          if (iRing == -1)
              poRing = poPoly->getExteriorRing();
          else
              poRing = poPoly->getInteriorRing(iRing);

          if (poRing)
          {
              for(i=0; i<poRing->getNumPoints(); i++)
                  ogrPointsAddPoint(&line, poRing->getX(i), poRing->getY(i),
                                    outshp->numlines, &(outshp->bounds));
          }
      }
  }

  msAddLine(outshp, &line);
  free(line.point);

  outshp->type = MS_SHAPE_POINT;

  return(0);
}


/**********************************************************************
 *                     ogrGeomLine()
 *
 * Recursively convert any OGRGeometry into a shapeObj.  Each part becomes
 * a line in the overall shapeObj.
 **********************************************************************/
static int ogrGeomLine(OGRGeometry *poGeom, shapeObj *outshp,
                       int bCloseRings) 
{
  if (poGeom == NULL)
      return 0;

/* ------------------------------------------------------------------
 * Use recursive calls for complex geometries
 * ------------------------------------------------------------------ */
  if (poGeom->getGeometryType() == wkbPolygon 
      || poGeom->getGeometryType() == wkbPolygon25D )
  {
      OGRPolygon *poPoly = (OGRPolygon *)poGeom;
      OGRLinearRing *poRing;

      if (outshp->type == MS_SHAPE_NULL)
          outshp->type = MS_SHAPE_POLYGON;

      for (int iRing=-1; iRing < poPoly->getNumInteriorRings(); iRing++)
      {
          if (iRing == -1)
              poRing = poPoly->getExteriorRing();
          else
              poRing = poPoly->getInteriorRing(iRing);

          if (ogrGeomLine(poRing, outshp, bCloseRings) == -1)
          {
              return -1;
          }
      }
  }
  else if (poGeom->getGeometryType() == wkbGeometryCollection ||
           poGeom->getGeometryType() == wkbMultiLineString ||
           poGeom->getGeometryType() == wkbMultiPolygon )
  {
      OGRGeometryCollection *poColl = (OGRGeometryCollection *)poGeom;

      for (int iGeom=0; iGeom < poColl->getNumGeometries(); iGeom++)
      {
          if (ogrGeomLine(poColl->getGeometryRef(iGeom),
                          outshp, bCloseRings) == -1)
          {
              return -1;
          }
      }
  }
/* ------------------------------------------------------------------
 * OGRPoint
 * ------------------------------------------------------------------ */
  else if (poGeom->getGeometryType() == wkbPoint
           || poGeom->getGeometryType() == wkbPoint25D )
  {
      /* Hummmm a point when we're drawing lines/polygons... just drop it! */
  }
/* ------------------------------------------------------------------
 * OGRLinearRing/OGRLineString ... both are of type wkbLineString
 * ------------------------------------------------------------------ */
  else if (poGeom->getGeometryType() == wkbLineString
           || poGeom->getGeometryType() == wkbLineString25D )
  {
      OGRLineString *poLine = (OGRLineString *)poGeom;
      int       j, numpoints;
      lineObj   line={0,NULL};
      double    dX, dY;

      if ((numpoints = poLine->getNumPoints()) < 2)
          return 0;

      if (outshp->type == MS_SHAPE_NULL)
          outshp->type = MS_SHAPE_LINE;

      line.numpoints = 0;
      line.point = (pointObj *)malloc(sizeof(pointObj)*(numpoints+1));
      if(!line.point) 
      {
          msSetError(MS_MEMERR, "Unable to allocate temporary point cache.", 
                     "ogrGeomLine");
          return(-1);
      }

      for(j=0; j<numpoints; j++)
      {
          dX = line.point[j].x = poLine->getX(j); 
          dY = line.point[j].y = poLine->getY(j);

          /* Keep track of shape bounds */
          if (j == 0 && outshp->numlines == 0)
          {
              outshp->bounds.minx = outshp->bounds.maxx = dX;
              outshp->bounds.miny = outshp->bounds.maxy = dY;
          }
          else
          {
              if (dX < outshp->bounds.minx)  outshp->bounds.minx = dX;
              if (dX > outshp->bounds.maxx)  outshp->bounds.maxx = dX;
              if (dY < outshp->bounds.miny)  outshp->bounds.miny = dY;
              if (dY > outshp->bounds.maxy)  outshp->bounds.maxy = dY;
          }

      }
      line.numpoints = numpoints; 

      if (bCloseRings &&
          ( line.point[line.numpoints-1].x != line.point[0].x ||
            line.point[line.numpoints-1].y != line.point[0].y  ) )
      {
          line.point[line.numpoints].x = line.point[0].x;
          line.point[line.numpoints].y = line.point[0].y;
          line.numpoints++;
      }

      msAddLine(outshp, &line);
      free(line.point);
  }
  else
  {
      msSetError(MS_OGRERR, 
                 (char*)CPLSPrintf("OGRGeometry type `%s' not supported.",
                                   poGeom->getGeometryName()),
                 "ogrGeomLine()");
      return(-1);
  }

  return(0);
}


/**********************************************************************
 *                     ogrConvertGeometry()
 *
 * Convert OGR geometry into a shape object doing the best possible 
 * job to match OGR Geometry type and layer type.
 *
 * If layer type is incompatible with geometry, then shape is returned with
 * shape->type = MS_SHAPE_NULL
 **********************************************************************/
static int ogrConvertGeometry(OGRGeometry *poGeom, shapeObj *outshp,
                              enum MS_LAYER_TYPE layertype) 
{
/* ------------------------------------------------------------------
 * Process geometry according to layer type
 * ------------------------------------------------------------------ */
  int nStatus = MS_SUCCESS;

  if (poGeom == NULL)
  {
      // Empty geometry... this is not an error... we'll just skip it
      return MS_SUCCESS;
  }

  switch(layertype) 
  {
/* ------------------------------------------------------------------
 *      POINT layer - Any geometry can be converted to point/multipoint
 * ------------------------------------------------------------------ */
    case MS_LAYER_POINT:
      if(ogrGeomPoints(poGeom, outshp) == -1)
      {
          nStatus = MS_FAILURE; // Error message already produced.
      }
      break;
/* ------------------------------------------------------------------
 *      LINE layer
 * ------------------------------------------------------------------ */
    case MS_LAYER_LINE:
      if(ogrGeomLine(poGeom, outshp, MS_FALSE) == -1)
      {
          nStatus = MS_FAILURE; // Error message already produced.
      }
      if (outshp->type != MS_SHAPE_LINE && outshp->type != MS_SHAPE_POLYGON)
          outshp->type = MS_SHAPE_NULL;  // Incompatible type for this layer
      break;
/* ------------------------------------------------------------------
 *      POLYGON layer
 * ------------------------------------------------------------------ */
    case MS_LAYER_POLYGON:
      if(ogrGeomLine(poGeom, outshp, MS_TRUE) == -1)
      {
          nStatus = MS_FAILURE; // Error message already produced.
      }
      if (outshp->type != MS_SHAPE_POLYGON)
          outshp->type = MS_SHAPE_NULL;  // Incompatible type for this layer
      break;
/* ------------------------------------------------------------------
 *      MS_ANNOTATION layer - return real feature type
 * ------------------------------------------------------------------ */
    case MS_LAYER_ANNOTATION:
    case MS_LAYER_QUERY:
      switch(poGeom->getGeometryType())
      {
        case wkbPoint:
        case wkbMultiPoint:
          if(ogrGeomPoints(poGeom, outshp) == -1)
          {
              nStatus = MS_FAILURE; // Error message already produced.
          }
          break;
        default:
          // Handle any non-point types as lines/polygons ... ogrGeomLine()
          // will decide the shape type
          if(ogrGeomLine(poGeom, outshp, MS_FALSE) == -1)
          {
              nStatus = MS_FAILURE; // Error message already produced.
          }
      }
      break;

    default:
      msSetError(MS_MISCERR, "Unknown or unsupported layer type.", 
                 "msOGRLayerNextShape()");
      nStatus = MS_FAILURE;
  } /* switch layertype */

  return nStatus;
}

/* ==================================================================
 * Attributes handling functions
 * ================================================================== */

// Special field index codes for handling text string and angle coming from
// OGR style strings.
#define MSOGR_LABELTEXTNAME     "OGR:LabelText"
#define MSOGR_LABELTEXTINDEX    -100
#define MSOGR_LABELANGLENAME    "OGR:LabelAngle"
#define MSOGR_LABELANGLEINDEX   -101
#define MSOGR_LABELSIZENAME     "OGR:LabelSize"
#define MSOGR_LABELSIZEINDEX    -102


/**********************************************************************
 *                     msOGRGetValues()
 *
 * Load selected item (i.e. field) values into a char array
 *
 * Some special attribute names are used to return some OGRFeature params
 * like for instance stuff encoded in the OGRStyleString.
 * For now the following pseudo-attribute names are supported:
 *  "OGR:TextString"  OGRFeatureStyle's text string if present
 *  "OGR:TextAngle"   OGRFeatureStyle's text angle, or 0 if not set
 **********************************************************************/
static char **msOGRGetValues(layerObj *layer, OGRFeature *poFeature)
{
  char **values;
  int i;

  if(layer->numitems == 0) 
      return(NULL);

  if(!layer->iteminfo)  // Should not happen... but just in case!
      if (msOGRLayerInitItemInfo(layer) != MS_SUCCESS)
          return NULL;

  if((values = (char **)malloc(sizeof(char *)*layer->numitems)) == NULL) 
  {
    msSetError(MS_MEMERR, NULL, "msOGRGetValues()");
    return(NULL);
  }

  OGRStyleMgr *poStyleMgr = NULL;
  OGRStyleLabel *poLabelStyle = NULL;
  int *itemindexes = (int*)layer->iteminfo;

  for(i=0;i<layer->numitems;i++)
  {
    if (itemindexes[i] >= 0)
    {
        // Extract regular attributes
        values[i] = strdup(poFeature->GetFieldAsString(itemindexes[i]));
    }
    else
    {
        // Handle special OGR attributes coming from StyleString
        if (!poStyleMgr)
        {
            poStyleMgr = new OGRStyleMgr(NULL);
            poStyleMgr->InitFromFeature(poFeature);
            OGRStyleTool *poStylePart = poStyleMgr->GetPart(0);
            if (poStylePart && poStylePart->GetType() == OGRSTCLabel)
                poLabelStyle = (OGRStyleLabel*)poStylePart;
            else if (poStylePart)
                delete poStylePart;
        }
        GBool bDefault;
        if (poLabelStyle && itemindexes[i] == MSOGR_LABELTEXTINDEX)
        {
            values[i] = strdup(poLabelStyle->TextString(bDefault));
            //msDebug(MSOGR_LABELTEXTNAME " = \"%s\"\n", values[i]);
        }
        else if (poLabelStyle && itemindexes[i] == MSOGR_LABELANGLEINDEX)
        {
            values[i] = strdup(poLabelStyle->GetParamStr(OGRSTLabelAngle,
                                                         bDefault));
            //msDebug(MSOGR_LABELANGLENAME " = \"%s\"\n", values[i]);
        }
        else if (poLabelStyle && itemindexes[i] == MSOGR_LABELSIZEINDEX)
        {
            values[i] = strdup(poLabelStyle->GetParamStr(OGRSTLabelSize,
                                                         bDefault));
            //msDebug(MSOGR_LABELSIZENAME " = \"%s\"\n", values[i]);
        }
        else
        {
          msSetError(MS_OGRERR,"Invalid field index!?!","msOGRGetValues()");
          return(NULL);
        }
    }
  }

  if (poStyleMgr)
      delete poStyleMgr;
  if (poLabelStyle)
      delete poLabelStyle;

  return(values);
}

#endif  /* USE_OGR */

/**********************************************************************
 *                    msLoadWKTProjectionString()
 *
 * Init a MapServer projectionObj using an OGC WKT string.
 * Like the existing msOGCWKT2ProjectionObj only _without_ the
 * checks for projection AUTO used by gdal layers.
 *
 * Returns MS_SUCCESS/MS_FAILURE
 **********************************************************************/
int msLoadWKTProjectionString( const char *pszWKT, 
                            projectionObj *proj )

{
#if defined(USE_OGR) || defined(USE_GDAL)

    OGRSpatialReference		oSRS;
    char			*pszAltWKT = (char *) pszWKT;

    if( oSRS.importFromWkt( &pszAltWKT ) != OGRERR_NONE )
    {
        msSetError(MS_OGRERR, 
                   "Ingestion of WKT string failed.",
                   "msLoadWKTProjectionString()");
        return MS_FAILURE;
    }

#ifdef USE_PROJ
    // First flush the projargs[]... 
    msFreeProjection( proj );

    if (oSRS.IsLocal())
    {
        // Dataset had no set projection or is NonEarth (LOCAL_CS)... 
        // Nothing else to do. Leave proj empty and no reprojection will happen!
        return MS_SUCCESS;  
    }

    // Export OGR SRS to a PROJ4 string
    char *pszProj = NULL;

    if (oSRS.exportToProj4( &pszProj ) != OGRERR_NONE ||
      pszProj == NULL || strlen(pszProj) == 0)
    {
      msSetError(MS_OGRERR, "Conversion from OGR SRS to PROJ4 failed.",
                 "msLoadWKTProjectionString()");
      CPLFree(pszProj);
      return(MS_FAILURE);
    }

    msDebug( "AUTO = %s\n", pszProj );

    if( msLoadProjectionString( proj, pszProj ) != 0 )
      return MS_FAILURE;

    CPLFree(pszProj);
#endif

    return MS_SUCCESS;

#else
    msSetError(MS_OGRERR, 
               "Not implemented since neither OGR nor GDAL is enabled.",
               "msLoadWKTProjectionString()");
    return MS_FAILURE;
#endif
}


#if defined(USE_OGR) || defined(USE_GDAL)

/**********************************************************************
 *                     msOGRSpatialRef2ProjectionObj()
 *
 * Init a MapServer projectionObj using an OGRSpatialRef
 * Works only with PROJECTION AUTO
 *
 * Returns MS_SUCCESS/MS_FAILURE
 **********************************************************************/
int msOGRSpatialRef2ProjectionObj(OGRSpatialReference *poSRS,
                                  projectionObj *proj)
{
  if (proj->numargs == 0  || !EQUAL(proj->args[0], "auto"))
      return MS_SUCCESS;  // Nothing to do!

#ifdef USE_PROJ
  // First flush the "auto" name from the projargs[]... 
  msFreeProjection( proj );

  if (poSRS == NULL || poSRS->IsLocal())
  {
      // Dataset had no set projection or is NonEarth (LOCAL_CS)... 
      // Nothing else to do. Leave proj empty and no reprojection will happen!
      return MS_SUCCESS;  
  }

  // Export OGR SRS to a PROJ4 string
  char *pszProj = NULL;

  if (poSRS->exportToProj4( &pszProj ) != OGRERR_NONE ||
      pszProj == NULL || strlen(pszProj) == 0)
  {
      msSetError(MS_OGRERR, "Conversion from OGR SRS to PROJ4 failed.",
                 "msOGRSpatialRef2ProjectionObj()");
      CPLFree(pszProj);
      return(MS_FAILURE);
  }

  msDebug( "AUTO = %s\n", pszProj );

  if( msLoadProjectionString( proj, pszProj ) != 0 )
      return MS_FAILURE;

  CPLFree(pszProj);
#endif

  return MS_SUCCESS;
}
#endif // defined(USE_OGR) || defined(USE_GDAL)

/**********************************************************************
 *                     msOGCWKT2ProjectionObj()
 *
 * Init a MapServer projectionObj using an OGC WKT definition.
 * Works only with PROJECTION AUTO
 *
 * Returns MS_SUCCESS/MS_FAILURE
 **********************************************************************/

int msOGCWKT2ProjectionObj( const char *pszWKT, 
                            projectionObj *proj )

{
#if defined(USE_OGR) || defined(USE_GDAL)

    OGRSpatialReference		oSRS;
    char			*pszAltWKT = (char *) pszWKT;

    if( oSRS.importFromWkt( &pszAltWKT ) != OGRERR_NONE )
    {
        msSetError(MS_OGRERR, 
                   "Ingestion of WKT string failed.",
                   "msOGCWKT2ProjectionObj()");
        return MS_FAILURE;
    }

    return msOGRSpatialRef2ProjectionObj( &oSRS, proj );
#else
    msSetError(MS_OGRERR, 
               "Not implemented since neither OGR nor GDAL is enabled.",
               "msOGCWKT2ProjectionObj()");
    return MS_FAILURE;
#endif
}

/* ==================================================================
 * Here comes the REAL stuff... the functions below are called by maplayer.c
 * ================================================================== */

/**********************************************************************
 *                     msOGRLayerOpen()
 *
 * Open OGR data source for the specified map layer.
 *
 * An OGR connection string is:   <dataset_filename>[,<layer_index>]
 *  <dataset_filename>   is file format specific
 *  <layer_index>        (optional) is the OGR layer index
 *                       default is 0, the first layer.
 *
 * One can use the "ogrinfo" program to find out the layer indices in a dataset
 *
 * Returns MS_SUCCESS/MS_FAILURE
 **********************************************************************/
int msOGRLayerOpen(layerObj *layer) 
{
#ifdef USE_OGR

  if (layer->ogrlayerinfo != NULL)
  {
      return MS_SUCCESS;  // Nothing to do... layer is already opened
  }

/* ------------------------------------------------------------------
 * Register OGR Drivers, only once per execution
 * ------------------------------------------------------------------ */
  static int bDriversRegistered = MS_FALSE;
  if (!bDriversRegistered)
      OGRRegisterAll();
  bDriversRegistered = MS_TRUE;

/* ------------------------------------------------------------------
 * Attempt to open OGR dataset
 * ------------------------------------------------------------------ */
  int   nLayerIndex = 0;
  char  **params;
  int   numparams;
  OGRDataSource *poDS;
  OGRLayer    *poLayer;

  if(layer->connection==NULL || 
     (params = split(layer->connection, ',', &numparams))==NULL || 
     numparams < 1) 
  {
      msSetError(MS_OGRERR, "Error parsing OGR connection information.", 
                 "msOGRLayerOpen()");
      return(MS_FAILURE);
  }

  msDebug("msOGRLayerOpen(%s)...\n", layer->connection);

  poDS = OGRSFDriverRegistrar::Open( params[0] );
  if( poDS == NULL )
  {
      msSetError(MS_OGRERR, 
                 (char*)CPLSPrintf("Open failed for OGR connection `%s'.  "
                                   "File not found or unsupported format.", 
                                   layer->connection),
                 "msOGRLayerOpen()");
      return(MS_FAILURE);
  }

  if(numparams > 1) 
      nLayerIndex = atoi(params[1]);

  poLayer = poDS->GetLayer(nLayerIndex);
  if (poLayer == NULL)
  {
      msSetError(MS_OGRERR, 
             (char*)CPLSPrintf("GetLayer(%d) failed for OGR connection `%s'.",
                               nLayerIndex, layer->connection),
                 "msOGRLayerOpen()");
      delete poDS;
      return(MS_FAILURE);
  }

/* ------------------------------------------------------------------
 * If projection was "auto" then set proj to the dataset's projection
 * ------------------------------------------------------------------ */
#ifdef USE_PROJ
  if (layer->projection.numargs > 0 && 
      EQUAL(layer->projection.args[0], "auto"))
  {
      OGRSpatialReference *poSRS = poLayer->GetSpatialRef();

      if (msOGRSpatialRef2ProjectionObj(poSRS,
                                        &(layer->projection)) != MS_SUCCESS)
      {
          errorObj *ms_error = msGetErrorObj();

          msSetError(MS_OGRERR, 
                     "%s  "
                     "PROJECTION AUTO cannot be used for this "
                     "OGR connection (`%s').",
                     "msOGRLayerOpen()",
                     ms_error->message, layer->connection);
          delete poDS;
          return(MS_FAILURE);
      }
  }
#endif

/* ------------------------------------------------------------------
 * OK... open succeded... alloc and fill msOGRLayerInfo inside layer obj
 * ------------------------------------------------------------------ */
  msOGRLayerInfo *psInfo =(msOGRLayerInfo*)CPLCalloc(1,sizeof(msOGRLayerInfo));
  layer->ogrlayerinfo = psInfo;

  psInfo->pszFname = CPLStrdup(params[0]);
  psInfo->nLayerIndex = nLayerIndex;
  psInfo->poDS = poDS;
  psInfo->poLayer = poLayer;

  // Cleanup and exit;
  msFreeCharArray(params, numparams);

  return MS_SUCCESS;

#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

  msSetError(MS_MISCERR, "OGR support is not available.", "msOGRLayerOpen()");
  return(MS_FAILURE);

#endif /* USE_OGR */
}

/**********************************************************************
 *                     msOGRLayerClose()
 **********************************************************************/
int msOGRLayerClose(layerObj *layer) 
{
#ifdef USE_OGR
  msOGRLayerInfo *psInfo =(msOGRLayerInfo*)layer->ogrlayerinfo;

  if (psInfo)
  {
    msDebug("msOGRLayerClose(%s).\n", layer->connection);

    CPLFree(psInfo->pszFname);

    if (psInfo->poLastFeature)
        delete psInfo->poLastFeature;

    /* Destroying poDS automatically closes files, destroys the layer, etc. */
    delete psInfo->poDS;

    CPLFree(psInfo);
    layer->ogrlayerinfo = NULL;
  }

  return MS_SUCCESS;

#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

  msSetError(MS_MISCERR, "OGR support is not available.", "msOGRLayerClose()");
  return(MS_FAILURE);

#endif /* USE_OGR */
}

/**********************************************************************
 *                     msOGRLayerWhichShapes()
 *
 * Init OGR layer structs ready for calls to msOGRLayerNextShape().
 *
 * Returns MS_SUCCESS/MS_FAILURE, or MS_DONE if no shape matching the
 * layer's FILTER overlaps the selected region.
 **********************************************************************/
int msOGRLayerWhichShapes(layerObj *layer, rectObj rect) 
{
#ifdef USE_OGR
  msOGRLayerInfo *psInfo =(msOGRLayerInfo*)layer->ogrlayerinfo;

  if (psInfo == NULL || psInfo->poLayer == NULL)
  {
    msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!", 
               "msOGRLayerWhichShapes()");
    return(MS_FAILURE);
  }

/* ------------------------------------------------------------------
 * Set Spatial filter... this may result in no features being returned
 * if layer does not overlap current view.
 *
 * __TODO__ We should return MS_DONE if no shape overlaps the selected 
 * region and matches the layer's FILTER expression, but there is currently
 * no _efficient_ way to do that with OGR.
 * ------------------------------------------------------------------ */
  OGRLinearRing oSpatialFilter;

  oSpatialFilter.setNumPoints(5);
  oSpatialFilter.setPoint(0, rect.minx, rect.miny);
  oSpatialFilter.setPoint(1, rect.maxx, rect.miny);
  oSpatialFilter.setPoint(2, rect.maxx, rect.maxy);
  oSpatialFilter.setPoint(3, rect.minx, rect.maxy);
  oSpatialFilter.setPoint(4, rect.minx, rect.miny);

  psInfo->poLayer->SetSpatialFilter( &oSpatialFilter );

/* ------------------------------------------------------------------
 * Reset current feature pointer
 * ------------------------------------------------------------------ */
  psInfo->poLayer->ResetReading();

  return MS_SUCCESS;

#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

  msSetError(MS_MISCERR, "OGR support is not available.", 
             "msOGRLayerWhichShapes()");
  return(MS_FAILURE);

#endif /* USE_OGR */
}

/**********************************************************************
 *                     msOGRLayerGetItems()
 *
 * Load item (i.e. field) names in a char array.
 **********************************************************************/
int msOGRLayerGetItems(layerObj *layer)
{
#ifdef USE_OGR
  msOGRLayerInfo *psInfo =(msOGRLayerInfo*)layer->ogrlayerinfo;
  OGRFeatureDefn *poDefn;
  int i;

  if((poDefn = psInfo->poLayer->GetLayerDefn()) == NULL ||
     (layer->numitems = poDefn->GetFieldCount()) == 0) 
  {
    msSetError(MS_OGRERR, "Layer contains no fields.", "msOGRLayerGetItems()");
    return(MS_FAILURE);
  }

  if((layer->items = (char**)malloc(sizeof(char *)*layer->numitems)) == NULL) 
  {
    msSetError(MS_MEMERR, NULL, "msOGRLayerGetItems()");
    return(MS_FAILURE);
  }

  for(i=0;i<layer->numitems;i++)
  {
      OGRFieldDefn *poField = poDefn->GetFieldDefn(i);
      layer->items[i] = strdup(poField->GetNameRef());
  }                                                                           

  return msOGRLayerInitItemInfo(layer);

#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

  msSetError(MS_MISCERR, "OGR support is not available.", 
             "msOGRLayerGetItems()");
  return(MS_FAILURE);

#endif /* USE_OGR */
}

/**********************************************************************
 *                     msOGRLayerInitItemInfo()
 *
 * Init the itemindexes array after items[] has been reset in a layer.
 **********************************************************************/
int msOGRLayerInitItemInfo(layerObj *layer)
{
#ifdef USE_OGR
  msOGRLayerInfo *psInfo =(msOGRLayerInfo*)layer->ogrlayerinfo;
  int   i;
  OGRFeatureDefn *poDefn;

  if (layer->numitems == 0)
      return MS_SUCCESS;

  if((poDefn = psInfo->poLayer->GetLayerDefn()) == NULL) 
  {
    msSetError(MS_OGRERR, "Layer contains no fields.",  
               "msOGRLayerInitItemInfo()");
    return(MS_FAILURE);
  }

  if (layer->iteminfo)
      free(layer->iteminfo);
  if((layer->iteminfo = (int *)malloc(sizeof(int)*layer->numitems))== NULL) 
  {
    msSetError(MS_MEMERR, NULL, "msOGRLayerInitItemInfo()");
    return(MS_FAILURE);
  }

  int *itemindexes = (int*)layer->iteminfo;
  for(i=0;i<layer->numitems;i++) 
  {
      // Special case for handling text string and angle coming from
      // OGR style strings.  We use special attribute names.
      if (EQUAL(layer->items[i], MSOGR_LABELTEXTNAME))
          itemindexes[i] = MSOGR_LABELTEXTINDEX;
      else if (EQUAL(layer->items[i], MSOGR_LABELANGLENAME))
          itemindexes[i] = MSOGR_LABELANGLEINDEX;
      else if (EQUAL(layer->items[i], MSOGR_LABELSIZENAME))
          itemindexes[i] = MSOGR_LABELSIZEINDEX;
      else
          itemindexes[i] = poDefn->GetFieldIndex(layer->items[i]);
      if(itemindexes[i] == -1)
      {
          msSetError(MS_OGRERR, 
                     (char*)CPLSPrintf("Invalid Field name: %s", 
                                       layer->items[i]), 
                     "msOGRLayerInitItemInfo()");
          return(MS_FAILURE);
      }
  }

  return(MS_SUCCESS);
#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

  msSetError(MS_MISCERR, "OGR support is not available.", 
             "msOGRLayerInitItemInfo()");
  return(MS_FAILURE);

#endif /* USE_OGR */
}

/**********************************************************************
 *                     msOGRLayerFreeItemInfo()
 *
 * Free the itemindexes array in a layer.
 **********************************************************************/
void msOGRLayerFreeItemInfo(layerObj *layer)
{
#ifdef USE_OGR

  if (layer->iteminfo)
      free(layer->iteminfo);
  layer->iteminfo = NULL;

#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

  msSetError(MS_MISCERR, "OGR support is not available.", 
             "msOGRLayerFreeItemInfo()");

#endif /* USE_OGR */
}


/**********************************************************************
 *                     msOGRLayerNextShape()
 *
 * Returns shape sequentially from OGR data source.
 * msOGRLayerWhichShape() must have been called first.
 *
 * Returns MS_SUCCESS/MS_FAILURE
 **********************************************************************/
int msOGRLayerNextShape(layerObj *layer, shapeObj *shape) 
{
#ifdef USE_OGR
  msOGRLayerInfo *psInfo =(msOGRLayerInfo*)layer->ogrlayerinfo;
  OGRFeature *poFeature = NULL;

  if (psInfo == NULL || psInfo->poLayer == NULL)
  {
    msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!", 
               "msOGRLayerNextShape()");
    return(MS_FAILURE);
  }

/* ------------------------------------------------------------------
 * Read until we find a feature that matches attribute filter and 
 * whose geometry is compatible with current layer type.
 * ------------------------------------------------------------------ */
  msFreeShape(shape);
  shape->type = MS_SHAPE_NULL;

  while (shape->type == MS_SHAPE_NULL)
  {
      if( poFeature )
          delete poFeature;

      if( (poFeature = psInfo->poLayer->GetNextFeature()) == NULL )
      {
          return MS_DONE;  // No more features to read
      }

      if(layer->numitems > 0) 
      {
          shape->values = msOGRGetValues(layer, poFeature);
          shape->numvalues = layer->numitems;
          if(!shape->values)
          {
              delete poFeature;
              return(MS_FAILURE);
          }
      }

      if (msEvalExpression(&(layer->filter), layer->filteritemindex, 
                           shape->values, layer->numitems) == MS_TRUE)
      {
          // Feature matched filter expression... process geometry
          // shape->type will be set if geom is compatible with layer type
          if (ogrConvertGeometry(poFeature->GetGeometryRef(), shape,
                                 layer->type) == MS_SUCCESS)
          {
              if (shape->type != MS_SHAPE_NULL)
                  break; // Shape is ready to be returned!
          }
          else
          {
              msFreeShape(shape);
              delete poFeature;
              return MS_FAILURE; // Error message already produced.
          }
      }

      // Feature rejected... free shape to clear attributes values.
      msFreeShape(shape);
      shape->type = MS_SHAPE_NULL;
  }

  shape->index = poFeature->GetFID();

  // Keep ref. to last feature read in case we need style info.
  if (psInfo->poLastFeature)
      delete psInfo->poLastFeature;
  psInfo->poLastFeature = poFeature;

  return MS_SUCCESS;

#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

  msSetError(MS_MISCERR, "OGR support is not available.", 
             "msOGRLayerNextShape()");
  return(MS_FAILURE);

#endif /* USE_OGR */
}

/**********************************************************************
 *                     msOGRLayerGetShape()
 *
 * Returns shape from OGR data source by id.
 *
 * Returns MS_SUCCESS/MS_FAILURE
 **********************************************************************/
int msOGRLayerGetShape(layerObj *layer, shapeObj *shape, int tile, long record)
{
#ifdef USE_OGR
  msOGRLayerInfo *psInfo =(msOGRLayerInfo*)layer->ogrlayerinfo;
  OGRFeature *poFeature;

  if (psInfo == NULL || psInfo->poLayer == NULL)
  {
    msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!", 
               "msOGRLayerNextShape()");
    return(MS_FAILURE);
  }

/* ------------------------------------------------------------------
 * Handle shape geometry... 
 * ------------------------------------------------------------------ */
  msFreeShape(shape);
  shape->type = MS_SHAPE_NULL;

  if( (poFeature = psInfo->poLayer->GetFeature(record)) == NULL )
  {
      return MS_FAILURE;
  }

  // shape->type will be set if geom is compatible with layer type
  if (ogrConvertGeometry(poFeature->GetGeometryRef(), shape,
                         layer->type) != MS_SUCCESS)
  {
      return MS_FAILURE; // Error message already produced.
  }
  
  if (shape->type == MS_SHAPE_NULL)
  {
      msSetError(MS_OGRERR, 
                 "Requested feature is incompatible with layer type",
                 "msOGRLayerGetShape()");
      return MS_FAILURE;
  }

/* ------------------------------------------------------------------
 * Process shape attributes
 * ------------------------------------------------------------------ */
  if(layer->numitems > 0) 
  {
      shape->values = msOGRGetValues(layer, poFeature);
      shape->numvalues = layer->numitems;
      if(!shape->values) return(MS_FAILURE);
  }   

  shape->index = poFeature->GetFID();

  // Keep ref. to last feature read in case we need style info.
  if (psInfo->poLastFeature)
      delete psInfo->poLastFeature;
  psInfo->poLastFeature = poFeature;

  return MS_SUCCESS;
#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

  msSetError(MS_MISCERR, "OGR support is not available.", 
             "msOGRLayerGetShape()");
  return(MS_FAILURE);

#endif /* USE_OGR */
}

/**********************************************************************
 *                     msOGRLayerGetExtent()
 *
 * Returns the layer extents.
 *
 * Returns MS_SUCCESS/MS_FAILURE
 **********************************************************************/
int msOGRLayerGetExtent(layerObj *layer, rectObj *extent) 
{
#ifdef USE_OGR
  msOGRLayerInfo *psInfo =(msOGRLayerInfo*)layer->ogrlayerinfo;
  OGREnvelope oExtent;

  if (psInfo == NULL || psInfo->poLayer == NULL)
  {
    msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!", 
               "msOGRLayerGetExtent()");
    return(MS_FAILURE);
  }

/* ------------------------------------------------------------------
 * Call OGR's GetExtent()... note that for some formats this will
 * result in a scan of the whole layer and can be an expensive call.
 * ------------------------------------------------------------------ */
  if (psInfo->poLayer->GetExtent(&oExtent, TRUE) != OGRERR_NONE)
  {
    msSetError(MS_MISCERR, "Unable to get extents for this layer.", 
               "msOGRLayerGetExtent()");
    return(MS_FAILURE);
  }

  extent->minx = oExtent.MinX;
  extent->miny = oExtent.MinY;
  extent->maxx = oExtent.MaxX;
  extent->maxy = oExtent.MaxY;

  return MS_SUCCESS;
#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

  msSetError(MS_MISCERR, "OGR support is not available.", 
             "msOGRLayerGetExtent()");
  return(MS_FAILURE);

#endif /* USE_OGR */
}


/**********************************************************************
 *                     msOGRGetSymbolId()
 *
 * Returns a MapServer symbol number matching one of the symbols from
 * the OGR symbol id string.  If not found then try to locate the
 * default symbol name, and if not found return 0.
 **********************************************************************/
#ifdef USE_OGR
static int msOGRGetSymbolId(symbolSetObj *symbolset, const char *pszSymbolId, 
                            const char *pszDefaultSymbol)
{
    // Symbol name mapping:
    // First look for the native symbol name, then the ogr-...
    // generic name, and in last resort try pszDefaultSymbol if
    // provided by user.
    char  **params;
    int   numparams;
    int   nSymbol = -1;

    if (pszSymbolId && pszSymbolId[0] != '\0' &&
        (params = split(pszSymbolId, '.', &numparams))!=NULL)
    {
        for(int j=0; j<numparams && nSymbol == -1; j++)
        {
            nSymbol = msGetSymbolIndex(symbolset, params[j]);
        }
        msFreeCharArray(params, numparams);
    }
    if (nSymbol == -1 && pszDefaultSymbol)
    {
        nSymbol = msGetSymbolIndex(symbolset, (char*)pszDefaultSymbol);
    }
    if (nSymbol == -1)
        nSymbol = 0;

    return nSymbol;
}
#endif

/**********************************************************************
 *                     msOGRLayerGetAutoStyle()
 *
 * Fills a classObj with style info from the specified shape.
 * For optimal results, this should be called immediately after 
 * GetNextShape() or GetShape() so that the shape doesn't have to be read
 * twice.
 *
 * The returned classObj is a ref. to a static structure valid only until
 * the next call and that shouldn't be freed by the caller.
 **********************************************************************/
int msOGRLayerGetAutoStyle(mapObj *map, layerObj *layer, classObj *c,
                           int tile, long record)
{
#ifdef USE_OGR
  msOGRLayerInfo *psInfo =(msOGRLayerInfo*)layer->ogrlayerinfo;

  if (psInfo == NULL || psInfo->poLayer == NULL)
  {
    msSetError(MS_MISCERR, "Assertion failed: OGR layer not opened!!!", 
               "msOGRLayerGetAutoStyle()");
    return(MS_FAILURE);
  }

/* ------------------------------------------------------------------
 * Read shape or reuse ref. to last shape read.
 * ------------------------------------------------------------------ */
  if (psInfo->poLastFeature == NULL || 
      psInfo->poLastFeature->GetFID() != record)
  {
      if (psInfo->poLastFeature)
          delete psInfo->poLastFeature;

      psInfo->poLastFeature = psInfo->poLayer->GetFeature(record);
  }

/* ------------------------------------------------------------------
 * Reset style info in the class to defaults
 * the only members we don't touch are name, expression, and join/query stuff
 * ------------------------------------------------------------------ */
  resetClassStyle(c);

  // __TODO__ label cache incompatible with styleitem feature.
  layer->labelcache = MS_OFF;

/* ------------------------------------------------------------------
 * Handle each part
 * ------------------------------------------------------------------ */
  if (psInfo->poLastFeature)
  {
      GBool bIsNull, bIsBrush=FALSE, bIsPen=FALSE;
      int r=0,g=0,b=0,t=0;

      OGRStyleMgr *poStyleMgr = new OGRStyleMgr(NULL);
      poStyleMgr->InitFromFeature(psInfo->poLastFeature);
      // msDebug("OGRStyle: %s\n", psInfo->poLastFeature->GetStyleString());

      int numParts = poStyleMgr->GetPartCount();
      for(int i=0; i<numParts; i++)
      {
          OGRStyleTool *poStylePart = poStyleMgr->GetPart(i);
          if (!poStylePart)
              continue;

          // We want all size values returned in pixels.
          //
          // The scale factor that OGR expect is the ground/paper scale
          // e.g. if 1 ground unit = 0.01 paper unit then scale=1/0.01=100
          // cellsize if number of ground units/pixel, and OGR assumes that
          // there is 72*39.37 pixels/ground units (since meter is assumed 
          // for ground... but what ground units we have does not matter
          // as long as use the same assumptions everywhere)
          // That gives scale = cellsize*72*39.37

          poStylePart->SetUnit(OGRSTUPixel, map->cellsize*72.0*39.37);

          if (poStylePart->GetType() == OGRSTCLabel)
          {
              OGRStyleLabel *poLabelStyle = (OGRStyleLabel*)poStylePart;

              loadExpressionString(&(c->text), 
                                   (char*)poLabelStyle->TextString(bIsNull));

              c->label.angle = poLabelStyle->Angle(bIsNull);

              c->label.size = (int)poLabelStyle->Size(bIsNull);

              // msDebug("** Label size=%d, angle=%f **\n", c->label.size, c->label.angle);

              // OGR default is anchor point = LL, so label is at UR of anchor
              c->label.position = MS_UR;

              if (poLabelStyle->GetRGBFromString(poLabelStyle->
                                                 ForeColor(bIsNull),r,g,b,t))
              {
                  c->label.color = msAddColor(map, r,g,b);
              }
              if (poLabelStyle->GetRGBFromString(poLabelStyle->
                                                 BackColor(bIsNull),r,g,b,t) 
                  && !bIsNull)
              {
                  c->label.backgroundcolor = msAddColor(map, r,g,b);
              }

              // Label font... do our best to use TrueType fonts, otherwise
              // fallback on bitmap fonts.
#if defined(USE_GD_TTF) || defined (USE_GD_FT)
              const char *pszName;
              if ((pszName = poLabelStyle->FontName(bIsNull)) != NULL && 
                  !bIsNull && pszName[0] != '\0' &&
                  msLookupHashTable(map->fontset.fonts, (char*)pszName) != NULL)
              {
                  c->label.type = MS_TRUETYPE;
                  c->label.font = strdup(pszName);
                  // msDebug("** Using '%s' TTF font **\n", pszName);
              }
              else if (msLookupHashTable(map->fontset.fonts,"default") != NULL)
              {
                  c->label.type = MS_TRUETYPE;
                  c->label.font = strdup("default");
                  // msDebug("** Using 'default' TTF font **\n");
              }
              else
#endif
              {
                  c->label.type = MS_BITMAP;
                  c->label.size = MS_MEDIUM;
                  // msDebug("** Using 'medium' BITMAP font **\n");
              }
          }
          else if (poStylePart->GetType() == OGRSTCPen)
          {
              OGRStylePen *poPenStyle = (OGRStylePen*)poStylePart;
              bIsPen = TRUE;

              const char *pszPenName = poPenStyle->Id(bIsNull);
              if (bIsNull) pszPenName = NULL;
              int nPenColor = -1;
              int nPenSymbol = 0;
              int nPenSize = 1;

              // Check for Pen Pattern "ogr-pen-1": the invisible pen
              // If that's what we have then set pen color to -1
              if (pszPenName && strstr(pszPenName, "ogr-pen-1") != NULL)
              {
                  nPenColor = -1;
              }
              else
              {
                  if (poPenStyle->GetRGBFromString(poPenStyle->
                                               Color(bIsNull),r,g,b,t))
                  {
                      nPenColor = msAddColor(map, r,g,b);
                      // msDebug("** PEN COLOR = %d %d %d (%d)**\n", r,g,b, nPenColor);
                  }

                  nPenSize = (int)poPenStyle->Width(bIsNull);
                  if (bIsNull)
                      nPenSize = 1;
                  if (pszPenName!=NULL || nPenSize > 1)
                  {
                      // Thick line or patterned line style
                      //
                      // First try to match pen name in symbol file
                      // If not found then look for a "default-circle" symbol
                      // that we'll use for producing thick lines.  
                      // Otherwise symbol will be set to 0 and line will 
                      // be 1 pixel wide.
                      nPenSymbol = msOGRGetSymbolId(&(map->symbolset),
                                                    pszPenName, 
                                           (nPenSize>1)?"default-circle":NULL);
                  }
              }

              if (bIsBrush)
              {
                  // This is a multipart symbology, so pen defn goes in the
                  // overlaysymbol params (also set outlinecolor just in case)
                  c->outlinecolor = c->overlayoutlinecolor = nPenColor;
                  c->overlaysize = nPenSize;
                  c->overlaysymbol = nPenSymbol;
              }
              else
              {
                  // Single part symbology
                  c->outlinecolor = c->color = nPenColor;
                  c->symbol = nPenSymbol;
                  c->size = nPenSize;
              }

          }
          else if (poStylePart->GetType() == OGRSTCBrush)
          {
              OGRStyleBrush *poBrushStyle = (OGRStyleBrush*)poStylePart;
              bIsBrush = TRUE;

              if (poBrushStyle->GetRGBFromString(poBrushStyle->
                                                 ForeColor(bIsNull),r,g,b,t))
              {
                  c->color = msAddColor(map, r,g,b);
                  // msDebug("** BRUSH COLOR = %d %d %d (%d)**\n", r,g,b,c->color);
              }
              if (poBrushStyle->GetRGBFromString(poBrushStyle->
                                                 BackColor(bIsNull),r,g,b,t) 
                  && !bIsNull)
              {
                  c->backgroundcolor = msAddColor(map, r,g,b);
              }

              // Symbol name mapping:
              // First look for the native symbol name, then the ogr-...
              // generic name.  
              // If none provided or found then use 0: solid fill
              
              const char *pszName = poBrushStyle->Id(bIsNull);
              if (bIsNull)
                  pszName = NULL;
              c->symbol = msOGRGetSymbolId(&(map->symbolset), pszName, NULL);

          }
          else if (poStylePart->GetType() == OGRSTCSymbol)
          {
              OGRStyleSymbol *poSymbolStyle = (OGRStyleSymbol*)poStylePart;

              if (poSymbolStyle->GetRGBFromString(poSymbolStyle->
                                                  Color(bIsNull),r,g,b,t))
              {
                  c->color = msAddColor(map, r,g,b);
              }

              c->size = (int)poSymbolStyle->Size(bIsNull);

              // Symbol name mapping:
              // First look for the native symbol name, then the ogr-...
              // generic name, and in last resort try "default-marker" if
              // provided by user.
              const char *pszName = poSymbolStyle->Id(bIsNull);
              if (bIsNull)
                  pszName = NULL;

              c->symbol = msOGRGetSymbolId(&(map->symbolset),
                                           pszName, 
                                           "default-marker");
          }

          delete poStylePart;

      }

      if (poStyleMgr)
          delete poStyleMgr;

  }

  return MS_SUCCESS;
#else
/* ------------------------------------------------------------------
 * OGR Support not included...
 * ------------------------------------------------------------------ */

  msSetError(MS_MISCERR, "OGR support is not available.", 
             "msOGRLayerGetAutoStyle()");
  return(MS_FAILURE);

#endif /* USE_OGR */
}

