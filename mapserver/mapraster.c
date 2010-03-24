/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  msDrawRasterLayer(): generic raster layer drawing.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *           Pete Olson (LMIC)            
 *           Steve Lime
 *
 ******************************************************************************
 * Copyright (c) 1996-2010 Regents of the University of Minnesota.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in 
 * all copies of this Software or works derived from this Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include <assert.h>
#include "mapserver.h"
#include "mapfile.h"
#include "mapresample.h"
#include "mapthread.h"

MS_CVSID("$Id$")

extern int msyyparse(void);
extern int msyylex(void);
extern char *msyytext;

extern int msyyresult; /* result of parsing, true/false */
extern int msyystate;
extern char *msyystring;

#ifdef USE_GDAL
#include "gdal.h"
#include "cpl_string.h"
#endif

#define MAXCOLORS 256
#define BUFLEN 1024
#define HDRLEN 8

#define	CVT(x) ((x) >> 8) /* converts to 8-bit color value */

#define NUMGRAYS 16

/************************************************************************/
/*                             msGetClass()                             */
/************************************************************************/

int msGetClass(layerObj *layer, colorObj *color)
{
  int i;
  char *tmpstr1=NULL;
  char tmpstr2[12]; /* holds either a single color index or something like 'rrr ggg bbb' */
  int status;
  int expresult;
  
  if((layer->numclasses == 1) && !(layer->class[0]->expression.string)) /* no need to do lookup */
    return(0);

  if(!color) return(-1);

  for(i=0; i<layer->numclasses; i++) {
    if (layer->class[i]->expression.string == NULL) /* Empty expression - always matches */
      return(i);
    switch(layer->class[i]->expression.type) {
    case(MS_STRING):
      sprintf(tmpstr2, "%d %d %d", color->red, color->green, color->blue);
      if(strcmp(layer->class[i]->expression.string, tmpstr2) == 0) return(i); /* matched */
      sprintf(tmpstr2, "%d", color->pen);
      if(strcmp(layer->class[i]->expression.string, tmpstr2) == 0) return(i); /* matched */
      break;
    case(MS_REGEX):
      if(!layer->class[i]->expression.compiled) {
	if(ms_regcomp(&(layer->class[i]->expression.regex), layer->class[i]->expression.string, MS_REG_EXTENDED|MS_REG_NOSUB) != 0) { /* compile the expression  */
	  msSetError(MS_REGEXERR, "Invalid regular expression.", "msGetClass()");
	  return(-1);
	}
	layer->class[i]->expression.compiled = MS_TRUE;
      }

      sprintf(tmpstr2, "%d %d %d", color->red, color->green, color->blue);
      if(ms_regexec(&(layer->class[i]->expression.regex), tmpstr2, 0, NULL, 0) == 0) return(i); /* got a match */
      sprintf(tmpstr2, "%d", color->pen);
      if(ms_regexec(&(layer->class[i]->expression.regex), tmpstr2, 0, NULL, 0) == 0) return(i); /* got a match */
      break;
    case(MS_EXPRESSION):
      tmpstr1 = strdup(layer->class[i]->expression.string);

      sprintf(tmpstr2, "%d", color->red);
      tmpstr1 = msReplaceSubstring(tmpstr1, "[red]", tmpstr2);
      sprintf(tmpstr2, "%d", color->green);
      tmpstr1 = msReplaceSubstring(tmpstr1, "[green]", tmpstr2);
      sprintf(tmpstr2, "%d", color->blue);
      tmpstr1 = msReplaceSubstring(tmpstr1, "[blue]", tmpstr2);

      sprintf(tmpstr2, "%d", color->pen);
      tmpstr1 = msReplaceSubstring(tmpstr1, "[pixel]", tmpstr2);

      msAcquireLock( TLOCK_PARSER );
      msyystate = MS_TOKENIZE_EXPRESSION; msyystring = tmpstr1;
      status = msyyparse();
      expresult = msyyresult;
      msReleaseLock( TLOCK_PARSER );

      free(tmpstr1);

      if( status != 0 ) return -1; /* error parsing expression. */
      if( expresult ) return i;    /* got a match? */
    }
  }

  return(-1); /* not found */
}

/************************************************************************/
/*                          msGetClass_Float()                          */
/*                                                                      */
/*      Returns the class based on classification of a floating         */
/*      pixel value.                                                    */
/************************************************************************/

int msGetClass_Float(layerObj *layer, float fValue)
{
    int i;
    char *tmpstr1=NULL;
    char tmpstr2[100];
    int status, expresult;
  
    if((layer->numclasses == 1) && !(layer->class[0]->expression.string)) /* no need to do lookup */
        return(0);

    for(i=0; i<layer->numclasses; i++) {
        if (layer->class[i]->expression.string == NULL) /* Empty expression - always matches */
            return(i);

        switch(layer->class[i]->expression.type) {
          case(MS_STRING):
            sprintf(tmpstr2, "%18g", fValue );
            if(strcmp(layer->class[i]->expression.string, tmpstr2) == 0) return(i); /* matched */
            break;

          case(MS_REGEX):
            if(!layer->class[i]->expression.compiled) {
                if(ms_regcomp(&(layer->class[i]->expression.regex), layer->class[i]->expression.string, MS_REG_EXTENDED|MS_REG_NOSUB) != 0) { /* compile the expression  */
                    msSetError(MS_REGEXERR, "Invalid regular expression.", "msGetClass()");
                    return(-1);
                }
                layer->class[i]->expression.compiled = MS_TRUE;
            }

            sprintf(tmpstr2, "%18g", fValue );
            if(ms_regexec(&(layer->class[i]->expression.regex), tmpstr2, 0, NULL, 0) == 0) return(i); /* got a match */
            break;

          case(MS_EXPRESSION):
            tmpstr1 = strdup(layer->class[i]->expression.string);

            sprintf(tmpstr2, "%18g", fValue);
            tmpstr1 = msReplaceSubstring(tmpstr1, "[pixel]", tmpstr2);

            msAcquireLock( TLOCK_PARSER );
            msyystate = MS_TOKENIZE_EXPRESSION; msyystring = tmpstr1;
            status = msyyparse();
            expresult = msyyresult;
            msReleaseLock( TLOCK_PARSER );

            free(tmpstr1);

            if( status != 0 ) return -1;
            if( expresult ) return i;
        }
    }

    return(-1); /* not found */
}

/************************************************************************/
/*                            msAddColorGD()                            */
/*                                                                      */
/*      Function to add a color to an existing color map.  It first     */
/*      looks for an exact match, then tries to add it to the end of    */
/*      the existing color map, and if all else fails it finds the      */
/*      closest color.                                                  */
/************************************************************************/

int msAddColorGD(mapObj *map, gdImagePtr img, int cmt, int r, int g, int b)
{
  int c; 
  int ct = -1;
  int op = -1;
  long rd, gd, bd, dist;
  long mindist = 3*255*255;  /* init to max poss dist */

  if( gdImageTrueColor( img ) )
      return gdTrueColor( r, g, b );
  
  /*
  ** We want to avoid using a color that matches a transparent background
  ** color exactly.  If this is the case, we will permute the value slightly.
  ** When perterbing greyscale images we try to keep them greyscale, otherwise
  ** we just perterb the red component.
  */
  if( map->outputformat && map->outputformat->transparent 
      && map->imagecolor.red == r 
      && map->imagecolor.green == g 
      && map->imagecolor.blue == b )
  {
      if( r == 0 && g == 0 && b == 0 )
      {
          r = g = b = 1;
      }
      else if( r == g && r == b )
      {
          r = g = b = r-1;
      }
      else if( r == 0 )
      {
          r = 1;
      }
      else
      {
          r = r-1;
      }
  }

  /* 
  ** Find the nearest color in the color table.  If we get an exact match
  ** return it right away.
  */
  for (c = 0; c < img->colorsTotal; c++) {

    if (img->open[c]) {
      op = c; /* Save open slot */
      continue; /* Color not in use */
    }
    
    /* don't try to use the transparent color */
    if (map->outputformat && map->outputformat->transparent 
        && img->red  [c] == map->imagecolor.red
        && img->green[c] == map->imagecolor.green
        && img->blue [c] == map->imagecolor.blue )
        continue;

    rd = (long)(img->red  [c] - r);
    gd = (long)(img->green[c] - g);
    bd = (long)(img->blue [c] - b);
/* -------------------------------------------------------------------- */
/*      special case for grey colors (r=g=b). we will try to find       */
/*      either the nearest grey or a color that is almost grey.         */
/* -------------------------------------------------------------------- */
    if (r == g && r == b)
    {
        if (img->red == img->green && img->red ==  img->blue)
          dist = rd*rd;
        else
          dist = rd * rd + gd * gd + bd * bd;
    }
    else
      dist = rd * rd + gd * gd + bd * bd;

    if (dist < mindist) {
      if (dist == 0) {
	return c; /* Return exact match color */
      }
      mindist = dist;
      ct = c;
    }
  }

  /* no exact match, is the closest within our "color match threshold"? */
  if( mindist <= cmt*cmt )
      return ct;

  /* no exact match.  If there are no open colors we return the closest
     color found.  */
  if (op == -1) {
    op = img->colorsTotal;
    if (op == gdMaxColors) { /* No room for more colors */
      return ct; /* Return closest available color */
    }
    img->colorsTotal++;
  }

  /* allocate a new exact match */
  img->red  [op] = r;
  img->green[op] = g;
  img->blue [op] = b;
  img->open [op] = 0;

  return op; /* Return newly allocated color */  
}

#ifdef USE_AGG
int msAddColorAGG(mapObj *map, gdImagePtr img, int cmt, int r, int g, int b)
{
	 return msAddColorGD( map, img, cmt, r, g, b );
}
#endif

/************************************************************************/
/*                        msDrawRasterLayerLow()                        */
/*                                                                      */
/*      Check for various file types and act appropriately.  Handle     */
/*      tile indexing.                                                  */
/************************************************************************/

int msDrawRasterLayerLow(mapObj *map, layerObj *layer, imageObj *image,
                         rasterBufferObj *rb ) 
{
/* -------------------------------------------------------------------- */
/*      As of MapServer 6.0 GDAL is required for rendering raster       */
/*      imagery.                                                        */
/* -------------------------------------------------------------------- */
#if !defined(USE_GDAL)
  msSetError(MS_MISCERR, 
             "Attempt to render a RASTER (or WMS) layer but without\n"
             "GDAL support enabled.  Raster rendering requires GDAL.",
             "msDrawRasterLayerLow()" );
  return MS_FAILURE;

#else /* defined(USE_GDAL) */
  int status, i, done;
  char *filename=NULL, tilename[MS_MAXPATHLEN];

  layerObj *tlp=NULL; /* pointer to the tile layer either real or temporary */
  int tileitemindex=-1, tilelayerindex=-1;
  shapeObj tshp;

  char szPath[MS_MAXPATHLEN];
  int final_status = MS_SUCCESS;

  rectObj searchrect;
  char *pszTmp = NULL;
  char tiAbsFilePath[MS_MAXPATHLEN];
  char *tiAbsDirPath = NULL;
  GDALDatasetH  hDS;
  double	adfGeoTransform[6];
  const char *close_connection;

  msGDALInitialize();

  if(layer->debug > 0 || map->debug > 1)
    msDebug( "msDrawRasterLayerLow(%s): entering.\n", layer->name );

  if(!layer->data && !layer->tileindex) {
    if(layer->debug == MS_TRUE) 
        msDebug( "msDrawRasterLayerLow(%s): layer data and tileindex NULL ... doing nothing.", layer->name );
    return(0);
  }

  if((layer->status != MS_ON) && (layer->status != MS_DEFAULT)) {
    if(layer->debug == MS_TRUE) 
        msDebug( "msDrawRasterLayerLow(%s): not status ON or DEFAULT, doing nothing.", layer->name );
    return(0);
  }

  if(map->scaledenom > 0) {
    if((layer->maxscaledenom > 0) && (map->scaledenom > layer->maxscaledenom)) {
      if(layer->debug == MS_TRUE) 
          msDebug( "msDrawRasterLayerLow(%s): skipping, map scale %.2g > MAXSCALEDENOM=%g\n", 
                   layer->name, map->scaledenom, layer->maxscaledenom );
      return(0);
    }
    if((layer->minscaledenom > 0) && (map->scaledenom <= layer->minscaledenom)) {
      if(layer->debug == MS_TRUE) 
          msDebug( "msDrawRasterLayerLow(%s): skipping, map scale %.2g < MINSCALEDENOM=%g\n", 
                   layer->name, map->scaledenom, layer->minscaledenom );
      return(0);
    }
  }

  if(layer->maxscaledenom <= 0 && layer->minscaledenom <= 0) {
    if((layer->maxgeowidth > 0) && ((map->extent.maxx - map->extent.minx) > layer->maxgeowidth)) {
      if(layer->debug == MS_TRUE) 
          msDebug( "msDrawRasterLayerLow(%s): skipping, map width %.2g > MAXSCALEDENOM=%g\n", layer->name, 
                   (map->extent.maxx - map->extent.minx), layer->maxgeowidth );
      return(0);
    }
    if((layer->mingeowidth > 0) && ((map->extent.maxx - map->extent.minx) < layer->mingeowidth)) {
      if(layer->debug == MS_TRUE) 
          msDebug( "msDrawRasterLayerLow(%s): skipping, map width %.2g < MINSCALEDENOM=%g\n", layer->name, 
          (map->extent.maxx - map->extent.minx), layer->mingeowidth );
      return(0);
    }
  }

  if(layer->tileindex) { /* we have an index file */
    
    msInitShape(&tshp);
    
    tilelayerindex = msGetLayerIndex(layer->map, layer->tileindex);
    if(tilelayerindex == -1) { /* the tileindex references a file, not a layer */

      /* so we create a temporary layer */
      tlp = (layerObj *) malloc(sizeof(layerObj));
      if(!tlp) {
        msSetError(MS_MEMERR, "Error allocating temporary layerObj.", "msDrawRasterLayerLow()");
        return(MS_FAILURE);
      }
      initLayer(tlp, map);

      /* set a few parameters for a very basic shapefile-based layer */
      tlp->name = strdup("TILE");
      tlp->type = MS_LAYER_TILEINDEX;
      tlp->data = strdup(layer->tileindex);
      if (layer->filteritem)
        tlp->filteritem = strdup(layer->filteritem);
      if (layer->filter.string)
      {
          if (layer->filter.type == MS_EXPRESSION)
          {
              pszTmp = 
                (char *)malloc(sizeof(char)*(strlen(layer->filter.string)+3));
              sprintf(pszTmp,"(%s)",layer->filter.string);
              msLoadExpressionString(&tlp->filter, pszTmp);
              free(pszTmp);
          }
          else if (layer->filter.type == MS_REGEX || 
                   layer->filter.type == MS_IREGEX)
          {
              pszTmp = 
                (char *)malloc(sizeof(char)*(strlen(layer->filter.string)+3));
              sprintf(pszTmp,"/%s/",layer->filter.string);
              msLoadExpressionString(&tlp->filter, pszTmp);
              free(pszTmp);
          }
          else
            msLoadExpressionString(&tlp->filter, layer->filter.string);
                   
          tlp->filter.type = layer->filter.type;
      }

    } else {
    	if ( msCheckParentPointer(layer->map,"map")==MS_FAILURE )
			return MS_FAILURE;
	tlp = (GET_LAYER(layer->map, tilelayerindex));
    }
    status = msLayerOpen(tlp);
    if(status != MS_SUCCESS)
    {
        final_status = status;
        goto cleanup;
    }

    status = msLayerWhichItems(tlp, MS_FALSE, layer->tileitem);
    if(status != MS_SUCCESS)
    {
        final_status = status;
        goto cleanup;
    }
 
    /* get the tileitem index */
    for(i=0; i<tlp->numitems; i++) {
      if(strcasecmp(tlp->items[i], layer->tileitem) == 0) {
        tileitemindex = i;
        break;
      }
    }
    if(i == tlp->numitems) { /* didn't find it */
        msSetError(MS_MEMERR, 
                   "Could not find attribute %s in tileindex.", 
                   "msDrawRasterLayerLow()", 
                   layer->tileitem);
        final_status = MS_FAILURE;
        goto cleanup;
    }
 
    searchrect = map->extent;
#ifdef USE_PROJ
    /* if necessary, project the searchrect to source coords */
    if((map->projection.numargs > 0) && (layer->projection.numargs > 0)) msProjectRect(&map->projection, &layer->projection, &searchrect);
#endif
    status = msLayerWhichShapes(tlp, searchrect);
    if (status != MS_SUCCESS) {
        /* Can be either MS_DONE or MS_FAILURE */
        if (status != MS_DONE) 
            final_status = status;

        goto cleanup;
    }
  }

  done = MS_FALSE;
  while(done != MS_TRUE) { 
    if(layer->tileindex) {
      status = msLayerNextShape(tlp, &tshp);
      if( status == MS_FAILURE)
      {
          final_status = MS_FAILURE;
          break;
      }

      if(status == MS_DONE) break; /* no more tiles/images */
       
      if(layer->data == NULL || strlen(layer->data) == 0 ) /* assume whole filename is in attribute field */
          strcpy( tilename, tshp.values[tileitemindex] );
      else
          sprintf(tilename, "%s/%s", tshp.values[tileitemindex], layer->data);
      filename = tilename;
      
      msFreeShape(&tshp); /* done with the shape */
    } else {
      filename = layer->data;
      done = MS_TRUE; /* only one image so we're done after this */
    }

    if(strlen(filename) == 0) continue;

    /*
    ** If using a tileindex then build the path relative to that file if SHAPEPATH is not set.
    */
    if(layer->tileindex && !map->shapepath) { 
      msTryBuildPath(tiAbsFilePath, map->mappath, layer->tileindex); /* absolute path to tileindex file */
      tiAbsDirPath = msGetPath(tiAbsFilePath); /* tileindex file's directory */
      msBuildPath(szPath, tiAbsDirPath, filename); 
      free(tiAbsDirPath);
    } else {
      msTryBuildPath3(szPath, map->mappath, map->shapepath, filename);
    }

    msAcquireLock( TLOCK_GDAL );
    hDS = GDALOpenShared(szPath, GA_ReadOnly );

    /*
    ** If GDAL doesn't recognise it, and it wasn't successfully opened 
    ** Generate an error.
    */
    if(hDS == NULL) {
        int ignore_missing = msMapIgnoreMissingData(map);

        msReleaseLock( TLOCK_GDAL );

        if(ignore_missing == MS_MISSING_DATA_FAIL) {
          msSetError(MS_IOERR, "Corrupt, empty or missing file '%s' for layer '%s'", "msDrawRasterLayerLow()", szPath, layer->name);
          return(MS_FAILURE); 
        }
        else if( ignore_missing == MS_MISSING_DATA_LOG ) {
          if( layer->debug || layer->map->debug ) {
            msDebug( "Corrupt, empty or missing file '%s' for layer '%s' ... ignoring this missing data.\n", szPath, layer->name );
          }
          continue;
        }
        else if( ignore_missing == MS_MISSING_DATA_IGNORE ) {
          continue;
        }
        else {
          /* never get here */
          msSetError(MS_IOERR, "msIgnoreMissingData returned unexpected value.", "msDrawRasterLayerLow()");
          return(MS_FAILURE);
        }
    }        

    /*
    ** Generate the projection information if using AUTO.
    */
    if (layer->projection.numargs > 0 && 
        EQUAL(layer->projection.args[0], "auto"))
    {
        const char *pszWKT;
        
        pszWKT = GDALGetProjectionRef( hDS );
        
        if( pszWKT != NULL && strlen(pszWKT) > 0 )
        {
            if( msOGCWKT2ProjectionObj(pszWKT, &(layer->projection),
                                       layer->debug ) != MS_SUCCESS )
            {
                char	szLongMsg[MESSAGELENGTH*2];
                errorObj *ms_error = msGetErrorObj();
                
                sprintf( szLongMsg, 
                         "%s\n"
                         "PROJECTION AUTO cannot be used for this "
                         "GDAL raster (`%s').",
                         ms_error->message, filename);
                szLongMsg[MESSAGELENGTH-1] = '\0';
                
                msSetError(MS_OGRERR, "%s","msDrawRasterLayer()",
                           szLongMsg);
                
                msReleaseLock( TLOCK_GDAL );
                final_status = MS_FAILURE;
                break;
            }
        }
    }

    msGetGDALGeoTransform( hDS, map, layer, adfGeoTransform );

    /* 
    ** We want to resample if the source image is rotated, if
    ** the projections differ or if resampling has been explicitly
    ** requested, or if the image has north-down instead of north-up.
    */
#ifdef USE_PROJ
    if( ((adfGeoTransform[2] != 0.0 || adfGeoTransform[4] != 0.0
          || adfGeoTransform[5] > 0.0 || adfGeoTransform[1] < 0.0 )
         && layer->transform )
        || msProjectionsDiffer( &(map->projection), 
                                &(layer->projection) ) 
        || CSLFetchNameValue( layer->processing, "RESAMPLE" ) != NULL )
    {
        status = msResampleGDALToMap( map, layer, image, rb, hDS );
    }
    else
#endif
    {
        if( adfGeoTransform[2] != 0.0 || adfGeoTransform[4] != 0.0 )
        {
            if( layer->debug || map->debug )
                msDebug( 
                    "Layer %s has rotational coefficients but we\n"
                    "are unable to use them, projections support\n"
                    "needs to be built in.", 
                    layer->name );
            
        }
        status = msDrawRasterLayerGDAL(map, layer, image, rb, hDS );
    }

    if( status == -1 )
    {
        GDALClose( hDS );
        msReleaseLock( TLOCK_GDAL );
        final_status = MS_FAILURE;
        break;
    }

    /*
    ** Should we keep this file open for future use?  
    ** default to keeping open for single data files, and 
    ** to closing for tile indexes
    */

    close_connection = msLayerGetProcessingKey( layer, 
                                                "CLOSE_CONNECTION" );
            
    if( close_connection == NULL && layer->tileindex == NULL )
        close_connection = "DEFER";
    
    if( close_connection != NULL
        && strcasecmp(close_connection,"DEFER") == 0 )
    {
        GDALDereferenceDataset( hDS );
    }
    else
    {
        GDALClose( hDS );
    }
    msReleaseLock( TLOCK_GDAL );
  } /* next tile */

cleanup:
  if(layer->tileindex) { /* tiling clean-up */
    msLayerClose(tlp);
    if(tilelayerindex == -1) {
      freeLayer(tlp);
      free(tlp);
    }
  }

  return final_status;

#endif /* defined(USE_GDAL) */
}

/************************************************************************/
/*                         msDrawReferenceMap()                         */
/************************************************************************/

/* TODO : this will msDrawReferenceMapGD */
imageObj *msDrawReferenceMap(mapObj *map) {
  double cellsize;
  int c=-1, oc=-1;
  int x1,y1,x2,y2;
  char szPath[MS_MAXPATHLEN];

  imageObj   *image = NULL;
  gdImagePtr img=NULL;

  image = msImageLoadGD( msBuildPath(szPath, map->mappath, 
                                     map->reference.image) );
  if( image == NULL )
      return NULL;

  if (map->web.imagepath)
      image->imagepath = strdup(map->web.imagepath);
  if (map->web.imageurl)
      image->imageurl = strdup(map->web.imageurl);

  img = image->img.gd;
  
  /* make sure the extent given in mapfile fits the image */
  cellsize = msAdjustExtent(&(map->reference.extent), 
                            image->width, image->height);

  /* Allocate a fake bg color because when using gd-1.8.4 with a PNG reference */
  /* image, the box color could end up being set to color index 0 which is  */
  /* transparent (yes, that's odd!). */
  gdImageColorAllocate(img, 255,255,255);

  /* allocate some colors */
  if( MS_VALID_COLOR(map->reference.outlinecolor) )
    oc = gdImageColorAllocate(img, map->reference.outlinecolor.red, map->reference.outlinecolor.green, map->reference.outlinecolor.blue);
  if( MS_VALID_COLOR(map->reference.color) )
    c = gdImageColorAllocate(img, map->reference.color.red, map->reference.color.green, map->reference.color.blue); 

  /* convert map extent to reference image coordinates */
  x1 = MS_MAP2IMAGE_X(map->extent.minx,  map->reference.extent.minx, cellsize);
  x2 = MS_MAP2IMAGE_X(map->extent.maxx,  map->reference.extent.minx, cellsize);  
  y1 = MS_MAP2IMAGE_Y(map->extent.maxy,  map->reference.extent.maxy, cellsize);
  y2 = MS_MAP2IMAGE_Y(map->extent.miny,  map->reference.extent.maxy, cellsize);

  /* if extent are smaller than minbox size */
  /* draw that extent on the reference image */
  if( (abs(x2 - x1) > map->reference.minboxsize) || 
          (abs(y2 - y1) > map->reference.minboxsize) )
  {
      if( map->reference.maxboxsize == 0 || 
              ((abs(x2 - x1) < map->reference.maxboxsize) && 
                  (abs(y2 - y1) < map->reference.maxboxsize)) )
      {
          if(c != -1)
              gdImageFilledRectangle(img,x1,y1,x2,y2,c);
          if(oc != -1)
              gdImageRectangle(img,x1,y1,x2,y2,oc);
      }
  
  }
  else /* else draw the marker symbol */
  {
      if( map->reference.maxboxsize == 0 || 
              ((abs(x2 - x1) < map->reference.maxboxsize) && 
                  (abs(y2 - y1) < map->reference.maxboxsize)) )
      {
	  styleObj style;

          initStyle(&style);
          style.color = map->reference.color;
          style.outlinecolor = map->reference.outlinecolor;
          style.size = map->reference.markersize;

          /* if the marker symbol is specify draw this symbol else draw a cross */
          if(map->reference.marker != 0)
          {
              pointObj *point = NULL;

              point = malloc(sizeof(pointObj));
              point->x = (double)(x1 + x2)/2;
              point->y = (double)(y1 + y2)/2;

	      style.symbol = map->reference.marker;

              msDrawMarkerSymbol(&map->symbolset, image, point, &style, 1.0);
              free(point);
          }
          else if(map->reference.markername != NULL)
          {              
              pointObj *point = NULL;

              point = malloc(sizeof(pointObj));
              point->x = (double)(x1 + x2)/2;
              point->y = (double)(y1 + y2)/2;

	      style.symbol = msGetSymbolIndex(&map->symbolset,  map->reference.markername, MS_TRUE);

              msDrawMarkerSymbol(&map->symbolset, image, point, &style, 1.0);
              free(point);
          }
          else
          {
              int x21, y21;
              /* determine the center point */
              x21 = MS_NINT((x1 + x2)/2);
              y21 = MS_NINT((y1 + y2)/2);

              /* get the color */
              if(c == -1)
                  c = oc;

              /* draw a cross */
              if(c != -1)
              {
                  gdImageLine(img, x21-8, y21, x21-3, y21, c);
                  gdImageLine(img, x21, y21-8, x21, y21-3, c);
                  gdImageLine(img, x21, y21+3, x21, y21+8, c);
                  gdImageLine(img, x21+3, y21, x21+8, y21, c);
              }
          }
      }
  }

  return(image);
}
