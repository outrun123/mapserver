/******************************************************************************
 *
 * Project:  MapServer
 * Purpose:  MapServer CGI mainline.
 * Author:   Steve Lime and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2004 Regents of the University of Minnesota.
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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.142  2004/12/09 21:20:19  frank
 * clear error list before processing new request in fastcgi mode
 *
 * Revision 1.141  2004/11/11 19:55:24  sdlime
 * Fixed enhancement request 1004, plus a bit more. ERROR and EMPTY properties are now treated as URL templates so for example you could pass the map xy value for a bombed query (perhaps to another service). Also added [errmsg] and [errmsg_esc] tags to the templating code so that the current error stack can be output. Various error messages are delimited by semi-colons.
 *
 * Revision 1.140  2004/11/09 21:34:31  assefa
 * Used to send html output with emebed flash file when format was set to flash.
 * Now send directly swf.
 *
 * Revision 1.139  2004/11/09 16:24:19  frank
 * avoid lots of casting warnings
 *
 * Revision 1.138  2004/11/04 21:47:36  frank
 * Removed unused variable.
 *
 * Revision 1.137  2004/11/03 17:07:13  frank
 * Free mapServObj properly in mapserv.c in OWS dispatch case to fix minor
 * memory leaks.
 *
 * Revision 1.136  2004/11/02 21:01:00  assefa
 * Add a 2nd optional argument to msLoadMapContext function (Bug 1023).
 *
 * Revision 1.135  2004/11/01 23:24:57  assefa
 * Work on Bug 1023 : only layers coming from a mapcontext use the
 * wms_name to set status on/off.
 *
 * Revision 1.134  2004/10/29 22:32:36  frank
 * Added a bit of debugging for new requests in fastcgi mode, with the
 * process id.
 *
 * Revision 1.133  2004/10/28 20:12:31  frank
 * added win32/fastcgi support using atexit()
 *
 * Revision 1.132  2004/10/28 19:12:51  frank
 * Implemented signal catching on unix (non-win32) platforms.  The signal
 * catcher calls msCleanup() which most importantly will close database
 * connections open in the pool.  This is most useful for the fastcgi case, but
 * it could be good in a pure cgi case too where a long running cgi is
 * killed for some reason.
 *
 * Revision 1.131  2004/10/28 18:16:17  dan
 * Fixed WMS GetLegendGraphic which was returning an exception (GD error)
 * when requested layer was out of scale (bug 1006)
 *
 * Revision 1.130  2004/10/21 04:30:54  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 */

#ifdef USE_FASTCGI
#define NO_FCGI_DEFINES
#include "fcgi_stdio.h"
#endif

#include "mapserv.h"

#ifndef WIN32
#include <signal.h>
#endif

MS_CVSID("$Id$")


mapservObj* msObj;

int writeLog(int show_error)
{
  FILE *stream;
  int i;
  time_t t;
  char szPath[MS_MAXPATHLEN];

  if(!msObj->Map) return(0);
  if(!msObj->Map->web.log) return(0);
  
  if((stream = fopen(msBuildPath(szPath, msObj->Map->mappath, 
                                   msObj->Map->web.log),"a")) == NULL) {
    msSetError(MS_IOERR, msObj->Map->web.log, "writeLog()");
    return(-1);
  }

  t = time(NULL);
  fprintf(stream,"%s,",chop(ctime(&t)));
  fprintf(stream,"%d,",(int)getpid());
  
  if(getenv("REMOTE_ADDR") != NULL)
    fprintf(stream,"%s,",getenv("REMOTE_ADDR"));
  else
    fprintf(stream,"NULL,");
 
  fprintf(stream,"%s,",msObj->Map->name);
  fprintf(stream,"%d,",msObj->Mode);

  fprintf(stream,"%f %f %f %f,", msObj->Map->extent.minx, msObj->Map->extent.miny, msObj->Map->extent.maxx, msObj->Map->extent.maxy);

  fprintf(stream,"%f %f,", msObj->MapPnt.x, msObj->MapPnt.y);

  for(i=0;i<msObj->NumLayers;i++)
    fprintf(stream, "%s ", msObj->Layers[i]);
  fprintf(stream,",");

  if(show_error == MS_TRUE)
    msWriteError(stream);
  else
    fprintf(stream, "normal execution");

  fprintf(stream,"\n");

  fclose(stream);
  return(0);
}

void writeError()
{
  errorObj *ms_error = msGetErrorObj();

  writeLog(MS_TRUE);

  if(!msObj->Map) {
    msIO_printf("Content-type: text/html%c%c",10,10);
    msIO_printf("<HTML>\n");
    msIO_printf("<HEAD><TITLE>MapServer Message</TITLE></HEAD>\n");
    msIO_printf("<!-- %s -->\n", msGetVersion());
    msIO_printf("<BODY BGCOLOR=\"#FFFFFF\">\n");
    msWriteError(stdout);
    msIO_printf("</BODY></HTML>");
    msCleanup();
    exit(0);
  }

  if((ms_error->code == MS_NOTFOUND) && (msObj->Map->web.empty)) {
    // msRedirect(msObj->Map->web.empty);
    if(msReturnURL(msObj, msObj->Map->web.empty, BROWSE) != MS_SUCCESS) {
      msIO_printf("Content-type: text/html%c%c",10,10);
      msIO_printf("<HTML>\n");
      msIO_printf("<HEAD><TITLE>MapServer Message</TITLE></HEAD>\n");
      msIO_printf("<!-- %s -->\n", msGetVersion());
      msIO_printf("<BODY BGCOLOR=\"#FFFFFF\">\n");
      msWriteError(stdout);
      msIO_printf("</BODY></HTML>");
    }
  } else {
    if(msObj->Map->web.error) {      
      // msRedirect(msObj->Map->web.error);
      if(msReturnURL(msObj, msObj->Map->web.error, BROWSE) != MS_SUCCESS) {
	msIO_printf("Content-type: text/html%c%c",10,10);
	msIO_printf("<HTML>\n");
	msIO_printf("<HEAD><TITLE>MapServer Message</TITLE></HEAD>\n");
	msIO_printf("<!-- %s -->\n", msGetVersion());
	msIO_printf("<BODY BGCOLOR=\"#FFFFFF\">\n");
	msWriteError(stdout);
	msIO_printf("</BODY></HTML>");
      }
    } else {
      msIO_printf("Content-type: text/html%c%c",10,10);
      msIO_printf("<HTML>\n");
      msIO_printf("<HEAD><TITLE>MapServer Message</TITLE></HEAD>\n");
      msIO_printf("<!-- %s -->\n", msGetVersion());
      msIO_printf("<BODY BGCOLOR=\"#FFFFFF\">\n");
      msWriteError(stdout);
      msIO_printf("</BODY></HTML>");
    }
  }

  // Clean-up (the following are not stored as part of the msObj)
  if(QueryItem) free(QueryItem);
  if(QueryString) free(QueryString);
  if(QueryLayer) free(QueryLayer);
  if(SelectLayer) free(SelectLayer);
  if(QueryFile) free(QueryFile);

  msFreeMapServObj(msObj);
  msCleanup();

  exit(0); // bail
}

/*
** Converts a string (e.g. form parameter) to a double, first checking the format against
** a regular expression. Dumps an error immediately if the format test fails.
*/
static double getNumeric(char *s)
{
  char *err;
  double rv; 

  rv = strtod(s, &err);
  if (*err) {
    msSetError(MS_TYPEERR, NULL, "getNumeric()");
    writeError();
  }
  return rv;
}

/*
** Extract Map File name from params and load it.  
** Returns map object or NULL on error.
*/
mapObj *loadMap()
{
  int i,j,k;
  mapObj *map = NULL;
  char *tmpstr;
  

  for(i=0;i<msObj->request->NumParams;i++) // find the mapfile parameter first
    if(strcasecmp(msObj->request->ParamNames[i], "map") == 0) break;
  
  if(i == msObj->request->NumParams) {
    if(getenv("MS_MAPFILE")) // has a default file has not been set
      map = msLoadMap(getenv("MS_MAPFILE"), NULL);
    else {
      msSetError(MS_WEBERR, "CGI variable \"map\" is not set.", "loadMap()"); // no default, outta here
      writeError();
    }
  } else {
    if(getenv(msObj->request->ParamValues[i])) // an environment references the actual file to use
      map = msLoadMap(getenv(msObj->request->ParamValues[i]), NULL);
    else
      map = msLoadMap(msObj->request->ParamValues[i], NULL);
  }

  if(!map) writeError();

  // check for any %variable% substitutions here, also do any map_ changes, we do this here so WMS/WFS 
  // services can take advantage of these "vendor specific" extensions
  for(i=0;i<msObj->request->NumParams;i++) {
    if(strncasecmp(msObj->request->ParamNames[i],"map_",4) == 0) { // check to see if there are any additions to the mapfile
      if(msLoadMapString(map, msObj->request->ParamNames[i], msObj->request->ParamValues[i]) == -1) writeError();
    }

    tmpstr = (char *)malloc(sizeof(char)*strlen(msObj->request->ParamNames[i]) + 3);
    sprintf(tmpstr,"%%%s%%", msObj->request->ParamNames[i]);
    
    for(j=0; j<map->numlayers; j++) {
      if(map->layers[j].data && (strstr(map->layers[j].data, tmpstr) != NULL)) map->layers[j].data = gsub(map->layers[j].data, tmpstr, msObj->request->ParamValues[i]);
      if(map->layers[j].connection && (strstr(map->layers[j].connection, tmpstr) != NULL)) map->layers[j].connection = gsub(map->layers[j].connection, tmpstr, msObj->request->ParamValues[i]);
      if(map->layers[j].filter.string && (strstr(map->layers[j].filter.string, tmpstr) != NULL)) map->layers[j].filter.string = gsub(map->layers[j].filter.string, tmpstr, msObj->request->ParamValues[i]);
      for(k=0; k<map->layers[j].numclasses; k++)
	if(map->layers[j].class[k].expression.string && (strstr(map->layers[j].class[k].expression.string, tmpstr) != NULL)) map->layers[j].class[k].expression.string = gsub(map->layers[j].class[k].expression.string, tmpstr, msObj->request->ParamValues[i]);
    }
    
    free(tmpstr);
  }

  //check to see if a ogc map context is passed as argument. if there
  //is one load it

  for(i=0;i<msObj->request->NumParams;i++) 
  {
      if(strcasecmp(msObj->request->ParamNames[i],"context") == 0) 
      {
          if (msObj->request->ParamValues[i] && 
              strlen(msObj->request->ParamValues[i]) > 0)
          {
              if (strncasecmp(msObj->request->ParamValues[i],"http",4) == 0)
              {
                  if (msGetConfigOption(map, "CGI_CONTEXT_URL"))
                    msLoadMapContextURL(map, msObj->request->ParamValues[i], MS_FALSE);
              }
              else
                msLoadMapContext(map, msObj->request->ParamValues[i], MS_FALSE); 
          }
      }
  } 
        
  return map;
}

/*
** Process CGI parameters.
*/
void loadForm()
{
  int i,j,n;
  char **tokens=NULL;
  int rosa_type=0;

  for(i=0;i<msObj->request->NumParams;i++) { // now process the rest of the form variables
    if(strlen(msObj->request->ParamValues[i]) == 0)
      continue;
    
    if(strcasecmp(msObj->request->ParamNames[i],"queryfile") == 0) {      
      QueryFile = strdup(msObj->request->ParamValues[i]);
      continue;
    }
    
    if(strcasecmp(msObj->request->ParamNames[i],"savequery") == 0) {
      msObj->SaveQuery = MS_TRUE;
      continue;
    }
    
    /* Insecure as implemented, need to save someplace non accessible by everyone in the universe
        if(strcasecmp(msObj->request->ParamNames[i],"savemap") == 0) {      
         msObj->SaveMap = MS_TRUE;
         continue;
        }
    */

    if(strcasecmp(msObj->request->ParamNames[i],"zoom") == 0) {
      msObj->Zoom = getNumeric(msObj->request->ParamValues[i]);      
      if((msObj->Zoom > MAXZOOM) || (msObj->Zoom < MINZOOM)) {
	msSetError(MS_WEBERR, "Zoom value out of range.", "loadForm()");
	writeError();
      }
      continue;
    }

    if(strcasecmp(msObj->request->ParamNames[i],"zoomdir") == 0) {
      msObj->ZoomDirection = (int)getNumeric(msObj->request->ParamValues[i]);
      if((msObj->ZoomDirection != -1) && (msObj->ZoomDirection != 1) && (msObj->ZoomDirection != 0)) {
	msSetError(MS_WEBERR, "Zoom direction must be 1, 0 or -1.", "loadForm()");
	writeError();
      }
      continue;
    }

    if(strcasecmp(msObj->request->ParamNames[i],"zoomsize") == 0) { // absolute zoom magnitude
      ZoomSize = (int) getNumeric(msObj->request->ParamValues[i]);      
      if((ZoomSize > MAXZOOM) || (ZoomSize < 1)) {
	msSetError(MS_WEBERR, "Invalid zoom size.", "loadForm()");
	writeError();
      }	
      continue;
    }
    
    if(strcasecmp(msObj->request->ParamNames[i],"imgext") == 0) { // extent of an existing image in a web application
      tokens = split(msObj->request->ParamValues[i], ' ', &n);

      if(!tokens) {
	msSetError(MS_MEMERR, NULL, "loadForm()");
	writeError();
      }

      if(n != 4) {
	msSetError(MS_WEBERR, "Not enough arguments for imgext.", "loadForm()");
	writeError();
      }

      msObj->ImgExt.minx = getNumeric(tokens[0]);
      msObj->ImgExt.miny = getNumeric(tokens[1]);
      msObj->ImgExt.maxx = getNumeric(tokens[2]);
      msObj->ImgExt.maxy = getNumeric(tokens[3]);

      msFreeCharArray(tokens, 4);
      continue;
    }

    if(strcasecmp(msObj->request->ParamNames[i],"searchmap") == 0) {      
      SearchMap = MS_TRUE;
      continue;
    }

    if(strcasecmp(msObj->request->ParamNames[i],"id") == 0) {
      strncpy(msObj->Id, msObj->request->ParamValues[i], IDSIZE);
      continue;
    }

    if(strcasecmp(msObj->request->ParamNames[i],"mapext") == 0) { // extent of the new map or query

      if(strncasecmp(msObj->request->ParamValues[i],"shape",5) == 0)
        msObj->UseShapes = MS_TRUE;
      else {
	tokens = split(msObj->request->ParamValues[i], ' ', &n);
	
	if(!tokens) {
	  msSetError(MS_MEMERR, NULL, "loadForm()");
	  writeError();
	}
	
	if(n != 4) {
	  msSetError(MS_WEBERR, "Not enough arguments for mapext.", "loadForm()");
	  writeError();
	}
	
	msObj->Map->extent.minx = getNumeric(tokens[0]);
	msObj->Map->extent.miny = getNumeric(tokens[1]);
	msObj->Map->extent.maxx = getNumeric(tokens[2]);
	msObj->Map->extent.maxy = getNumeric(tokens[3]);	
	
	msFreeCharArray(tokens, 4);
	
#ifdef USE_PROJ
	// make sure both coordinates are in range!
	if(msObj->Map->projection.proj && !pj_is_latlong(msObj->Map->projection.proj)
           && (msObj->Map->extent.minx >= -180.0 && msObj->Map->extent.minx <= 180.0) 
           && (msObj->Map->extent.miny >= -90.0 && msObj->Map->extent.miny <= 90.0)
	   && (msObj->Map->extent.maxx >= -180.0 && msObj->Map->extent.maxx <= 180.0) 
           && (msObj->Map->extent.maxy >= -90.0 && msObj->Map->extent.maxy <= 90.0))
	  msProjectRect(&(msObj->Map->latlon), 
                        &(msObj->Map->projection), 
                        &(msObj->Map->extent)); // extent is a in lat/lon
#endif

	if((msObj->Map->extent.minx != msObj->Map->extent.maxx) && (msObj->Map->extent.miny != msObj->Map->extent.maxy)) { // extent seems ok
	  msObj->CoordSource = FROMUSERBOX;
	  QueryCoordSource = FROMUSERBOX;
	}
      }

      continue;
    }

    if(strcasecmp(msObj->request->ParamNames[i],"minx") == 0) { // extent of the new map, in pieces
      msObj->Map->extent.minx = getNumeric(msObj->request->ParamValues[i]);      
      continue;
    }
    if(strcasecmp(msObj->request->ParamNames[i],"maxx") == 0) {      
      msObj->Map->extent.maxx = getNumeric(msObj->request->ParamValues[i]);
      continue;
    }
    if(strcasecmp(msObj->request->ParamNames[i],"miny") == 0) {
      msObj->Map->extent.miny = getNumeric(msObj->request->ParamValues[i]);
      continue;
    }
    if(strcasecmp(msObj->request->ParamNames[i],"maxy") == 0) {
      msObj->Map->extent.maxy = getNumeric(msObj->request->ParamValues[i]);
      msObj->CoordSource = FROMUSERBOX;
      QueryCoordSource = FROMUSERBOX;
      continue;
    } 

    if(strcasecmp(msObj->request->ParamNames[i],"mapxy") == 0) { // user map coordinate
      
      if(strncasecmp(msObj->request->ParamValues[i],"shape",5) == 0) {
        msObj->UseShapes = MS_TRUE;	
      } else {
	tokens = split(msObj->request->ParamValues[i], ' ', &n);

	if(!tokens) {
	  msSetError(MS_MEMERR, NULL, "loadForm()");
	  writeError();
	}
	
	if(n != 2) {
	  msSetError(MS_WEBERR, "Not enough arguments for mapxy.", "loadForm()");
	  writeError();
	}
	
	msObj->MapPnt.x = getNumeric(tokens[0]);
	msObj->MapPnt.y = getNumeric(tokens[1]);
	
	msFreeCharArray(tokens, 2);

#ifdef USE_PROJ
	if(msObj->Map->projection.proj && !pj_is_latlong(msObj->Map->projection.proj)
           && (msObj->MapPnt.x >= -180.0 && msObj->MapPnt.x <= 180.0) 
           && (msObj->MapPnt.y >= -90.0 && msObj->MapPnt.y <= 90.0))
	  msProjectPoint(&(msObj->Map->latlon), 
                         &(msObj->Map->projection), 
                         &msObj->MapPnt); // point is a in lat/lon
#endif

	if(msObj->CoordSource == NONE) { // don't override previous settings (i.e. buffer or scale )
	  msObj->CoordSource = FROMUSERPNT;
	  QueryCoordSource = FROMUSERPNT;
	}
      }
      continue;
    }

    if(strcasecmp(msObj->request->ParamNames[i],"mapshape") == 0) { // query shape
      lineObj line={0,NULL};
      char **tmp=NULL;
      int n, j;
      
      tmp = split(msObj->request->ParamValues[i], ' ', &n);

      if((line.point = (pointObj *)malloc(sizeof(pointObj)*(n/2))) == NULL) {
	msSetError(MS_MEMERR, NULL, "loadForm()");
	writeError();
      }
      line.numpoints = n/2;

      msInitShape(&(msObj->SelectShape));
      msObj->SelectShape.type = MS_SHAPE_POLYGON;

      for(j=0; j<n/2; j++) {
	line.point[j].x = atof(tmp[2*j]);
	line.point[j].y = atof(tmp[2*j+1]);

#ifdef USE_PROJ
	if(msObj->Map->projection.proj && !pj_is_latlong(msObj->Map->projection.proj)
           && (line.point[j].x >= -180.0 && line.point[j].x <= 180.0) 
           && (line.point[j].y >= -90.0 && line.point[j].y <= 90.0))
	  msProjectPoint(&(msObj->Map->latlon), 
                         &(msObj->Map->projection), 
                         &line.point[j]); // point is a in lat/lon
#endif
      }

      if(msAddLine(&msObj->SelectShape, &line) == -1) writeError();

      msFree(line.point);	
      msFreeCharArray(tmp, n);

      QueryCoordSource = FROMUSERSHAPE;
      continue;
    }

    if(strcasecmp(msObj->request->ParamNames[i],"img.x") == 0) { // mouse click, in pieces
      msObj->ImgPnt.x = getNumeric(msObj->request->ParamValues[i]);
      if((msObj->ImgPnt.x > (2*msObj->Map->maxsize)) || (msObj->ImgPnt.x < (-2*msObj->Map->maxsize))) {
	msSetError(MS_WEBERR, "Coordinate out of range.", "loadForm()");
	writeError();
      }
      msObj->CoordSource = FROMIMGPNT;
      QueryCoordSource = FROMIMGPNT;
      continue;
    }
    if(strcasecmp(msObj->request->ParamNames[i],"img.y") == 0) {
      msObj->ImgPnt.y = getNumeric(msObj->request->ParamValues[i]);      
      if((msObj->ImgPnt.y > (2*msObj->Map->maxsize)) || (msObj->ImgPnt.y < (-2*msObj->Map->maxsize))) {
	msSetError(MS_WEBERR, "Coordinate out of range.", "loadForm()");
	writeError();
      }
      msObj->CoordSource = FROMIMGPNT;
      QueryCoordSource = FROMIMGPNT;
      continue;
    }

    if(strcasecmp(msObj->request->ParamNames[i],"imgxy") == 0) { // mouse click, single variable
      if(msObj->CoordSource == FROMIMGPNT)
	    continue;

      tokens = split(msObj->request->ParamValues[i], ' ', &n);

      if(!tokens) {
	    msSetError(MS_MEMERR, NULL, "loadForm()");
	    writeError();
      }

      if(n != 2) {
	    msSetError(MS_WEBERR, "Not enough arguments for imgxy.", "loadForm()");
	    writeError();
      }

      msObj->ImgPnt.x = getNumeric(tokens[0]);
      msObj->ImgPnt.y = getNumeric(tokens[1]);

      msFreeCharArray(tokens, 2);

      if((msObj->ImgPnt.x > (2*msObj->Map->maxsize)) || (msObj->ImgPnt.x < (-2*msObj->Map->maxsize)) || (msObj->ImgPnt.y > (2*msObj->Map->maxsize)) || (msObj->ImgPnt.y < (-2*msObj->Map->maxsize))) {
	    msSetError(MS_WEBERR, "Reference map coordinate out of range.", "loadForm()");
	    writeError();
      }

      if(msObj->CoordSource == NONE) { // override nothing since this parameter is usually used to hold a default value
	    msObj->CoordSource = FROMIMGPNT;
	   QueryCoordSource = FROMIMGPNT;
      }
      continue;
    }

    if(strcasecmp(msObj->request->ParamNames[i],"imgbox") == 0) { // selection box (eg. mouse drag)
      tokens = split(msObj->request->ParamValues[i], ' ', &n);
      
      if(!tokens) {
	    msSetError(MS_MEMERR, NULL, "loadForm()");
	    writeError();
      }
      
      if(n != 4) {
	    msSetError(MS_WEBERR, "Not enough arguments for imgbox.", "loadForm()");
	    writeError();
      }
      
      msObj->ImgBox.minx = getNumeric(tokens[0]);
      msObj->ImgBox.miny = getNumeric(tokens[1]);
      msObj->ImgBox.maxx = getNumeric(tokens[2]);
      msObj->ImgBox.maxy = getNumeric(tokens[3]);
      
      msFreeCharArray(tokens, 4);

      if((msObj->ImgBox.minx != msObj->ImgBox.maxx) && (msObj->ImgBox.miny != msObj->ImgBox.maxy)) { // must not degenerate into a point
        msObj->CoordSource = FROMIMGBOX;
        QueryCoordSource = FROMIMGBOX;
      }
      continue;
    }

    if(strcasecmp(msObj->request->ParamNames[i],"imgshape") == 0) { // shape given in image coordinates
      lineObj line={0,NULL};
      char **tmp=NULL;
      int n, j;
      
      tmp = split(msObj->request->ParamValues[i], ' ', &n);

      if((line.point = (pointObj *)malloc(sizeof(pointObj)*(n/2))) == NULL) {
        msSetError(MS_MEMERR, NULL, "loadForm()");
	    writeError();
      }
      line.numpoints = n/2;

      msInitShape(&msObj->SelectShape);
      msObj->SelectShape.type = MS_SHAPE_POLYGON;

      for(j=0; j<n/2; j++) {
	    line.point[j].x = atof(tmp[2*j]);
	    line.point[j].y = atof(tmp[2*j+1]);
      }

      if(msAddLine(&msObj->SelectShape, &line) == -1) writeError();

      msFree(line.point);
      msFreeCharArray(tmp, n);

      QueryCoordSource = FROMIMGSHAPE;
      continue;
    }

    if(strcasecmp(msObj->request->ParamNames[i],"ref.x") == 0) { // mouse click in reference image, in pieces
      msObj->RefPnt.x = getNumeric(msObj->request->ParamValues[i]);      
      if((msObj->RefPnt.x > (2*msObj->Map->maxsize)) || (msObj->RefPnt.x < (-2*msObj->Map->maxsize))) {
	    msSetError(MS_WEBERR, "Coordinate out of range.", "loadForm()");
	    writeError();
      }
      msObj->CoordSource = FROMREFPNT;
      continue;
    }
    if(strcasecmp(msObj->request->ParamNames[i],"ref.y") == 0) {
      msObj->RefPnt.y = getNumeric(msObj->request->ParamValues[i]); 
      if((msObj->RefPnt.y > (2*msObj->Map->maxsize)) || (msObj->RefPnt.y < (-2*msObj->Map->maxsize))) {
	    msSetError(MS_WEBERR, "Coordinate out of range.", "loadForm()");
	    writeError();
      }
      msObj->CoordSource = FROMREFPNT;
      continue;
    }

    if(strcasecmp(msObj->request->ParamNames[i],"refxy") == 0) { /* mouse click in reference image, single variable */
      tokens = split(msObj->request->ParamValues[i], ' ', &n);

      if(!tokens) {
	    msSetError(MS_MEMERR, NULL, "loadForm()");
	    writeError();
      }

      if(n != 2) {
	    msSetError(MS_WEBERR, "Not enough arguments for imgxy.", "loadForm()");
	    writeError();
      }

      msObj->RefPnt.x = getNumeric(tokens[0]);
      msObj->RefPnt.y = getNumeric(tokens[1]);

      msFreeCharArray(tokens, 2);
      
      if((msObj->RefPnt.x > (2*msObj->Map->maxsize)) || (msObj->RefPnt.x < (-2*msObj->Map->maxsize)) || (msObj->RefPnt.y > (2*msObj->Map->maxsize)) || (msObj->RefPnt.y < (-2*msObj->Map->maxsize))) {
	    msSetError(MS_WEBERR, "Reference map coordinate out of range.", "loadForm()");
	    writeError();
      }
      
      msObj->CoordSource = FROMREFPNT;
      continue;
    }

    if(strcasecmp(msObj->request->ParamNames[i],"buffer") == 0) { // radius (map units), actually 1/2 square side
      msObj->Buffer = getNumeric(msObj->request->ParamValues[i]);      
      msObj->CoordSource = FROMBUF;
      QueryCoordSource = FROMUSERPNT;
      continue;
    }

    if(strcasecmp(msObj->request->ParamNames[i],"scale") == 0) { // scale for new map
      msObj->Scale = getNumeric(msObj->request->ParamValues[i]);      
      if(msObj->Scale <= 0) {
        msSetError(MS_WEBERR, "Scale out of range.", "loadForm()");
        writeError();
      }
      msObj->CoordSource = FROMSCALE;
      QueryCoordSource = FROMUSERPNT;
      continue;
    }
    
    if(strcasecmp(msObj->request->ParamNames[i],"imgsize") == 0) { // size of existing image (pixels)
      tokens = split(msObj->request->ParamValues[i], ' ', &n);

      if(!tokens) {
        msSetError(MS_MEMERR, NULL, "loadForm()");
        writeError();
      }

      if(n != 2) {
        msSetError(MS_WEBERR, "Not enough arguments for imgsize.", "loadForm()");
        writeError();
      }

      msObj->ImgCols = (int)getNumeric(tokens[0]);
      msObj->ImgRows = (int)getNumeric(tokens[1]);

      msFreeCharArray(tokens, 2);
      
      if(msObj->ImgCols > msObj->Map->maxsize || msObj->ImgRows > msObj->Map->maxsize || msObj->ImgCols < 0 || msObj->ImgRows < 0) {
        msSetError(MS_WEBERR, "Image size out of range.", "loadForm()");
        writeError();
      }
 
      continue;
    }

    if(strcasecmp(msObj->request->ParamNames[i],"mapsize") == 0) { // size of new map (pixels)
      tokens = split(msObj->request->ParamValues[i], ' ', &n);

      if(!tokens) {
	    msSetError(MS_MEMERR, NULL, "loadForm()");
	    writeError();
      }

      if(n != 2) {
	    msSetError(MS_WEBERR, "Not enough arguments for mapsize.", "loadForm()");
	    writeError();
      }

      msObj->Map->width = (int)getNumeric(tokens[0]);
      msObj->Map->height = (int)getNumeric(tokens[1]);

      msFreeCharArray(tokens, 2);
      
      if(msObj->Map->width > msObj->Map->maxsize || msObj->Map->height > msObj->Map->maxsize || msObj->Map->width < 0 || msObj->Map->height < 0) {
        msSetError(MS_WEBERR, "Image size out of range.", "loadForm()");
        writeError();
      }
      continue;
    }

    if(strncasecmp(msObj->request->ParamNames[i],"layers", 6) == 0) { // turn a set of layers, delimited by spaces, on

      /* If layers=all then turn on all layers */
      if (strcasecmp(msObj->request->ParamValues[i], "all") == 0 && msObj->Map != NULL)
      {
          int l;

          /* Reset NumLayers=0. If individual layers were already selected then free the previous values.  */
          for(l=0; l<msObj->NumLayers; l++)
              msFree(msObj->Layers[l]);
          msObj->NumLayers=0;

          for(msObj->NumLayers=0; msObj->NumLayers < msObj->Map->numlayers; msObj->NumLayers++)
          {
              if (msObj->Map->layers[msObj->NumLayers].name)
              {
                  msObj->Layers[msObj->NumLayers] = strdup(msObj->Map->layers[msObj->NumLayers].name);
              }
              else
              {
                  msObj->Layers[msObj->NumLayers] = strdup("");
              }
          }
      }
      else
      {
          int num_layers=0, l;
          char **layers=NULL;

          layers = split(msObj->request->ParamValues[i], ' ', &(num_layers));
          for(l=0; l<num_layers; l++)
              msObj->Layers[msObj->NumLayers+l] = strdup(layers[l]);
          msObj->NumLayers += l;

          msFreeCharArray(layers, num_layers);
          num_layers = 0;
      }

      continue;
    }

    if(strncasecmp(msObj->request->ParamNames[i],"layer", 5) == 0) { // turn a single layer/group on
      msObj->Layers[msObj->NumLayers] = strdup(msObj->request->ParamValues[i]);
      msObj->NumLayers++;
      continue;
    }

    if(strcasecmp(msObj->request->ParamNames[i],"qlayer") == 0) { // layer to query (i.e search)
      QueryLayer = strdup(msObj->request->ParamValues[i]);
      continue;
    }

    if(strcasecmp(msObj->request->ParamNames[i],"qitem") == 0) { // attribute to query on (optional)
      QueryItem = strdup(msObj->request->ParamValues[i]);
      continue;
    }

    if(strcasecmp(msObj->request->ParamNames[i],"qstring") == 0) { // attribute query string
      QueryString = strdup(msObj->request->ParamValues[i]);
      continue;
    }

    if(strcasecmp(msObj->request->ParamNames[i],"slayer") == 0) { // layer to select (for feature based search)
      SelectLayer = strdup(msObj->request->ParamValues[i]);
      continue;
    }

    if(strcasecmp(msObj->request->ParamNames[i],"shapeindex") == 0) { // used for index queries
      ShapeIndex = (int)getNumeric(msObj->request->ParamValues[i]);
      continue;
    }
    if(strcasecmp(msObj->request->ParamNames[i],"tileindex") == 0) {
      TileIndex = (int)getNumeric(msObj->request->ParamValues[i]);
      continue;
    }

    if(strcasecmp(msObj->request->ParamNames[i],"mode") == 0) { // set operation mode
      for(j=0; j<numModes; j++) {
	if(strcasecmp(msObj->request->ParamValues[i], modeStrings[j]) == 0) {
	  msObj->Mode = j;

	  if(msObj->Mode == ZOOMIN) {
	    msObj->ZoomDirection = 1;
	    msObj->Mode = BROWSE;
	  }
	  if(msObj->Mode == ZOOMOUT) {
	    msObj->ZoomDirection = -1;
	    msObj->Mode = BROWSE;
	  }

	  break;
	}
      }

      if(j == numModes) {
	msSetError(MS_WEBERR, "Invalid mode.", "loadForm()");
	writeError();
      }

      continue;
    }

    // check to see if there are any additions to the mapfile, this has been moved to loadMap()
    // if(strncasecmp(msObj->request->ParamNames[i],"map_",4) == 0) { 
    //   if(msLoadMapString(msObj->Map, msObj->request->ParamNames[i], msObj->request->ParamValues[i]) == -1)
    //     writeError();
    //   continue;
    // }

/* -------------------------------------------------------------------- */
/*      The following code is used to support the rosa applet (for      */
/*      more information on Rosa, please consult :                      */
/*      http://www2.dmsolutions.ca/webtools/rosa/index.html) .          */
/*      This code was provided by Tim.Mackey@agso.gov.au.               */
/*                                                                      */
/*      For Application using it can be seen at :                       */
/*http://mapserver.gis.umn.edu/wilma/mapserver-users/0011/msg00077.html */
/*   http://www.agso.gov.au/map/pilbara/                                */
/*                                                                      */
/* -------------------------------------------------------------------- */

    if(strcasecmp(msObj->request->ParamNames[i],"INPUT_TYPE") == 0)
    { /* Rosa input type */
        if(strcasecmp(msObj->request->ParamValues[i],"auto_rect") == 0) 
        {
            rosa_type=1; /* rectangle */
            continue;
        }
            
        if(strcasecmp(msObj->request->ParamValues[i],"auto_point") == 0) 
        {
            rosa_type=2; /* point */
            continue;
        }
    }
    if(strcasecmp(msObj->request->ParamNames[i],"INPUT_COORD") == 0) 
    { /* Rosa coordinates */
 
       switch(rosa_type)
       {
         case 1:
             sscanf(msObj->request->ParamValues[i],"%lf,%lf;%lf,%lf",
                    &msObj->ImgBox.minx,&msObj->ImgBox.miny,&msObj->ImgBox.maxx,
                    &msObj->ImgBox.maxy);
             if((msObj->ImgBox.minx != msObj->ImgBox.maxx) && 
                (msObj->ImgBox.miny != msObj->ImgBox.maxy)) 
             {
                 msObj->CoordSource = FROMIMGBOX;
                 QueryCoordSource = FROMIMGBOX;
             }
             else 
             {
                 msObj->CoordSource = FROMIMGPNT;
                 QueryCoordSource = FROMIMGPNT;
                 msObj->ImgPnt.x=msObj->ImgBox.minx;
                 msObj->ImgPnt.y=msObj->ImgBox.miny;
	   }
           break;
         case 2:
           sscanf(msObj->request->ParamValues[i],"%lf,%lf",&msObj->ImgPnt.x,
                   &msObj->ImgPnt.y);
           msObj->CoordSource = FROMIMGPNT;
           QueryCoordSource = FROMIMGPNT;
           break;
         }
       continue;
    }    
/* -------------------------------------------------------------------- */
/*      end of code for Rosa support.                                   */
/* -------------------------------------------------------------------- */


  }

  if(ZoomSize != 0) { // use direction and magnitude to calculate zoom
    if(msObj->ZoomDirection == 0) {
      msObj->fZoom = 1;
    } else {
      msObj->fZoom = ZoomSize*msObj->ZoomDirection;
      if(msObj->fZoom < 0)
	msObj->fZoom = 1.0/MS_ABS(msObj->fZoom);
    }
  } else { // use single value for zoom
    if((msObj->Zoom >= -1) && (msObj->Zoom <= 1)) {
      msObj->fZoom = 1; // pan
    } else {
      if(msObj->Zoom < 0)
	msObj->fZoom = 1.0/MS_ABS(msObj->Zoom);
      else
	msObj->fZoom = msObj->Zoom;
    }
  }

  if(msObj->ImgRows == -1) msObj->ImgRows = msObj->Map->height;
  if(msObj->ImgCols == -1) msObj->ImgCols = msObj->Map->width;  
  if(msObj->Map->height == -1) msObj->Map->height = msObj->ImgRows;
  if(msObj->Map->width == -1) msObj->Map->width = msObj->ImgCols;  
}

void setExtentFromShapes() {
  int found=0;
  double dx, dy, cellsize;

  rectObj tmpext={-1.0,-1.0,-1.0,-1.0};
  pointObj tmppnt={-1.0,-1.0};

  found = msGetQueryResultBounds(msObj->Map, &(tmpext));

  dx = tmpext.maxx - tmpext.minx;
  dy = tmpext.maxy - tmpext.miny;
 
  tmppnt.x = (tmpext.maxx + tmpext.minx)/2;
  tmppnt.y = (tmpext.maxy + tmpext.miny)/2;
  tmpext.minx -= dx*EXTENT_PADDING/2.0;
  tmpext.maxx += dx*EXTENT_PADDING/2.0;
  tmpext.miny -= dy*EXTENT_PADDING/2.0;
  tmpext.maxy += dy*EXTENT_PADDING/2.0;

  if(msObj->Scale != 0) { // apply the scale around the center point (tmppnt)
    cellsize = (msObj->Scale/msObj->Map->resolution)/msInchesPerUnit(msObj->Map->units,0); // user supplied a point and a scale
    tmpext.minx = tmppnt.x - cellsize*msObj->Map->width/2.0;
    tmpext.miny = tmppnt.y - cellsize*msObj->Map->height/2.0;
    tmpext.maxx = tmppnt.x + cellsize*msObj->Map->width/2.0;
    tmpext.maxy = tmppnt.y + cellsize*msObj->Map->height/2.0;
  } else if(msObj->Buffer != 0) { // apply the buffer around the center point (tmppnt)
    tmpext.minx = tmppnt.x - msObj->Buffer;
    tmpext.miny = tmppnt.y - msObj->Buffer;
    tmpext.maxx = tmppnt.x + msObj->Buffer;
    tmpext.maxy = tmppnt.y + msObj->Buffer;
  }

  // in case we don't get  usable extent at this point (i.e. single point result)
  if(!MS_VALID_EXTENT(tmpext)) {
    if(msObj->Map->web.minscale > 0) { // try web object minscale first
      cellsize = (msObj->Map->web.minscale/msObj->Map->resolution)/msInchesPerUnit(msObj->Map->units,0); // user supplied a point and a scale
      tmpext.minx = tmppnt.x - cellsize*msObj->Map->width/2.0;
      tmpext.miny = tmppnt.y - cellsize*msObj->Map->height/2.0;
      tmpext.maxx = tmppnt.x + cellsize*msObj->Map->width/2.0;
      tmpext.maxy = tmppnt.y + cellsize*msObj->Map->height/2.0;
    } else {
      msSetError(MS_WEBERR, "No way to generate a valid map extent from selected shapes.", "mapserv()");
      writeError();
    }
  }

  msObj->MapPnt = tmppnt;
  msObj->Map->extent = msObj->RawExt = tmpext; // save unadjusted extent

  return;
}


/* FIX: NEED ERROR CHECKING HERE FOR IMGPNT or MAPPNT */
void setCoordinate()
{
  double cellx,celly;

  cellx = MS_CELLSIZE(msObj->ImgExt.minx, msObj->ImgExt.maxx, msObj->ImgCols);
  celly = MS_CELLSIZE(msObj->ImgExt.miny, msObj->ImgExt.maxy, msObj->ImgRows);

  msObj->MapPnt.x = MS_IMAGE2MAP_X(msObj->ImgPnt.x, msObj->ImgExt.minx, cellx);
  msObj->MapPnt.y = MS_IMAGE2MAP_Y(msObj->ImgPnt.y, msObj->ImgExt.maxy, celly);

  return;
}

void returnCoordinate()
{
  msSetError(MS_NOERR, 
             "Your \"<i>click</i>\" corresponds to (approximately): (%g, %g).",
             NULL,
             msObj->MapPnt.x, msObj->MapPnt.y);

#ifdef USE_PROJ
  if(msObj->Map->projection.proj != NULL && !pj_is_latlong(msObj->Map->projection.proj) ) {
    pointObj p=msObj->MapPnt;
    msProjectPoint(&(msObj->Map->projection), &(msObj->Map->latlon), &p);
    msSetError( MS_NOERR, 
                "%s Computed lat/lon value is (%g, %g).\n", NULL, p.x, p.y);
  }
#endif

  writeError();
}

#ifndef WIN32
void msCleanupOnSignal( int nInData )

{
    // For some reason, the fastcgi message code does not seem to work
    // from within the signal handler on Unix.  So we force output through
    // normal stdio functions.
    msIO_installHandlers( NULL, NULL, NULL );
    msIO_fprintf( stderr, "In msCleanupOnSignal.\n" );
    msCleanup();
    exit( 3 );
}
#endif

#ifdef WIN32
void msCleanupOnExit( void )
{
    // note that stderr and stdout seem to be non-functional in the
    // fastcgi/win32 case.  If you really want to check functioning do
    // some sort of hack logging like below ... otherwise just trust it!
       
#ifdef notdef
    FILE *fp_out = fopen( "D:\\temp\\mapserv.log", "w" );
    
    fprintf( fp_out, "In msCleanupOnExit\n" );
    fclose( fp_out );
#endif    
    msCleanup();
}
#endif

// FIX: need to consider 5% shape extent expansion

/*
**
** Start of main program
**
*/
int main(int argc, char *argv[]) {
    int i,j;
    char buffer[1024];
    imageObj *img=NULL;
    int status;

    if(argc > 1 && strcmp(argv[1], "-v") == 0) {
      printf("%s\n", msGetVersion());
      fflush(stdout);
      exit(0);
    }
    else if(argc > 2 && strcmp(argv[1], "-t") == 0) 
    {
        char **tokens;
        int numtokens=0;
        if ((tokens=msTokenizeMap(argv[2], &numtokens)) != NULL)
        {
            int i;
            for(i=0; i<numtokens; i++)
                printf("%s\n", tokens[i]);
            msFreeCharArray(tokens, numtokens);
        }
        else
        {
            writeError();
        }

      exit(0);
    }
    else if (argc > 1 && strncmp(argv[1], "QUERY_STRING=", 13) == 0) {
      /* Debugging hook... pass "QUERY_STRING=..." on the command-line */
      char *buf;
      buf = strdup("REQUEST_METHOD=GET");
      putenv(buf);
      buf = strdup(argv[1]);
      putenv(buf);
    }

#ifndef WIN32
    signal( SIGUSR1, msCleanupOnSignal );
    signal( SIGTERM, msCleanupOnSignal );
#endif

#ifdef USE_FASTCGI
    msIO_installFastCGIRedirect();

#ifdef WIN32
    atexit( msCleanupOnExit );
#endif    

    while( FCGI_Accept() >= 0 )
    {
#endif /* def USE_FASTCGI */

    msObj = msAllocMapServObj();

    sprintf(msObj->Id, "%ld%d",(long)time(NULL),(int)getpid()); // asign now so it can be overridden

    msObj->request->ParamNames = (char **) malloc(MAX_PARAMS*sizeof(char*));
    msObj->request->ParamValues = (char **) malloc(MAX_PARAMS*sizeof(char*));
    if (msObj->request->ParamNames==NULL || msObj->request->ParamValues==NULL) {
	msSetError(MS_MEMERR, NULL, "mapserv()");
	writeError();
    }

    msObj->request->NumParams = loadParams(msObj->request);
    msObj->Map = loadMap();

#ifdef USE_FASTCGI
    if( msObj->Map->debug )
    {
        static int nRequestCounter = 1;

        msDebug( "CGI Request %d on process %d\n", 
                 nRequestCounter, getpid() );

        nRequestCounter++;
    }
#endif

    /*
    ** Start by calling the WMS/WFS/WCS Dispatchers.  If they fail then we'll 
    ** process this as a regular MapServer request.
    */
#if defined(USE_WMS_SVR) || defined(USE_WFS_SVR) || defined(USE_WCS_SVR)
    if ((status = msOWSDispatch(msObj->Map, msObj->request)) != MS_DONE  ) 
    {
      /* This was a WMS/WFS request... cleanup and exit 
       * At this point any error has already been handled
       * as an XML exception by the OGC service.
       */
      msFreeMapServObj(msObj);

#ifdef USE_FASTCGI
      /* FCGI_ --- return to top of loop */
      msResetErrorList();
      continue;
#else
      /* normal case, processing is complete */
      msCleanup();
      exit( 0 );
#endif

    }
#endif /* have an OWS service request */

    loadForm();
 
    if(msObj->SaveMap) {
      sprintf(buffer, "%s%s%s.map", msObj->Map->web.imagepath, msObj->Map->name, msObj->Id);
      if(msSaveMap(msObj->Map, buffer) == -1) writeError();
    }

    if((msObj->CoordSource == FROMIMGPNT) || (msObj->CoordSource == FROMIMGBOX)) /* make sure extent of existing image matches shape of image */
      msObj->Map->cellsize = msAdjustExtent(&msObj->ImgExt, msObj->ImgCols, msObj->ImgRows);

    /*
    ** For each layer lets set layer status
    */
        
    for(i=0;i<msObj->Map->numlayers;i++) {
      if((msObj->Map->layers[i].status != MS_DEFAULT)) {
	if(isOn(msObj,  msObj->Map->layers[i].name, msObj->Map->layers[i].group) == MS_TRUE) /* Set layer status */
	  msObj->Map->layers[i].status = MS_ON;
	else
	  msObj->Map->layers[i].status = MS_OFF;
      }     
    }

    if(msObj->CoordSource == FROMREFPNT) /* force browse mode if the reference coords are set */
      msObj->Mode = BROWSE;

    if(msObj->Mode == BROWSE) {

      if(!msObj->Map->web.template) {
	msSetError(MS_WEBERR, "No template provided.", "mapserv()");
	writeError();
      }

      if(QueryFile) {
	status = msLoadQuery(msObj->Map, QueryFile);
	if(status != MS_SUCCESS) writeError();
      }
      
      setExtent(msObj);
      checkWebScale(msObj);
       
/* -------------------------------------------------------------------- */
/*      generate map, legend, scalebar and refernce images.             */
/* -------------------------------------------------------------------- */
      if (msGenerateImages(msObj, QueryFile, MS_TRUE) == MS_FALSE)
        writeError();
      
      if(QueryFile) {
          if (msReturnQuery(msObj, msObj->Map->web.queryformat, NULL) != MS_SUCCESS)
           writeError();
      } else {
	if(TEMPLATE_TYPE(msObj->Map->web.template) == MS_FILE) { /* if thers's an html template, then use it */
	  msIO_printf("Content-type: text/html%c%c", 10, 10); /* write MIME header */
	  msIO_printf("<!-- %s -->\n", msGetVersion());
	  fflush(stdout);
	  if (msReturnPage(msObj, msObj->Map->web.template, BROWSE, NULL) != MS_SUCCESS)
         writeError();
	} else {	
	  if (msReturnURL(msObj, msObj->Map->web.template, BROWSE) != MS_SUCCESS)
         writeError();
	}
      }

    } else if(msObj->Mode == MAP || msObj->Mode == SCALEBAR || msObj->Mode == REFERENCE) { // "image" only modes
      setExtent(msObj);
      checkWebScale(msObj);
      
      switch(msObj->Mode) {
      case MAP:
	if(QueryFile) {
	  status = msLoadQuery(msObj->Map, QueryFile);
	  if(status != MS_SUCCESS) writeError();
	  img = msDrawQueryMap(msObj->Map);
	} else
	  img = msDrawMap(msObj->Map);
	break;
      case REFERENCE:
	msObj->Map->cellsize = msAdjustExtent(&(msObj->Map->extent), msObj->Map->width, msObj->Map->height);
	img = msDrawReferenceMap(msObj->Map);
	break;      
      case SCALEBAR:
	img = msDrawScalebar(msObj->Map);
	break;
      }
      
      if(!img) writeError();

      
      msIO_printf("Content-type: %s%c%c",MS_IMAGE_MIME_TYPE(msObj->Map->outputformat), 10,10);
      if( msObj->Mode == MAP )
          status = msSaveImage(msObj->Map,img, NULL);
      else
          status = msSaveImage(NULL,img, NULL);
          
      if(status != MS_SUCCESS) writeError();
      
      msFreeImage(img);
    } else if (msObj->Mode == LEGEND) {
       if (msObj->Map->legend.template) 
       {
          char *legendTemplate;

          legendTemplate = generateLegendTemplate(msObj);
          if (legendTemplate) {
             msIO_printf("Content-type: text/html\n\n%s", legendTemplate);

             free(legendTemplate);
          }
          else // error already generated by (generateLegendTemplate())
            writeError();;
       }         
       else
       {
          img = msDrawLegend(msObj->Map, MS_FALSE);
          if(!img) writeError();

          msIO_printf("Content-type: %s%c%c",MS_IMAGE_MIME_TYPE(msObj->Map->outputformat), 10,10);
          status = msSaveImage(NULL, img, NULL);
          if(status != MS_SUCCESS) writeError();
          
          msFreeImage(img);            
       }
    } else if(msObj->Mode >= QUERY) { // query modes

      if(QueryFile) { // already got a completed query
	status = msLoadQuery(msObj->Map, QueryFile);
	if(status != MS_SUCCESS) writeError();
      } else {

	if((QueryLayerIndex = msGetLayerIndex(msObj->Map, QueryLayer)) != -1) /* force the query layer on */
	  msObj->Map->layers[QueryLayerIndex].status = MS_ON;

        switch(msObj->Mode) {
	case ITEMFEATUREQUERY:
        case ITEMFEATURENQUERY:
	case ITEMFEATUREQUERYMAP:
        case ITEMFEATURENQUERYMAP:
	  if((SelectLayerIndex = msGetLayerIndex(msObj->Map, SelectLayer)) == -1) { /* force the selection layer on */
	    msSetError(MS_WEBERR, "Selection layer not set or references an invalid layer.", "mapserv()"); 
	    writeError();
	  }
	  msObj->Map->layers[SelectLayerIndex].status = MS_ON;

	  if(QueryCoordSource != NONE && !msObj->UseShapes)
	    setExtent(msObj); /* set user area of interest */

	  if(msObj->Mode == ITEMFEATUREQUERY || msObj->Mode == ITEMFEATUREQUERYMAP) {
	    if((status = msQueryByAttributes(msObj->Map, SelectLayerIndex, QueryItem, QueryString, MS_SINGLE)) != MS_SUCCESS) writeError();
	  } else {
	    if((status = msQueryByAttributes(msObj->Map, SelectLayerIndex, QueryItem, QueryString, MS_MULTIPLE)) != MS_SUCCESS) writeError();
	  }

	  if(msQueryByFeatures(msObj->Map, QueryLayerIndex, SelectLayerIndex) != MS_SUCCESS) writeError();

	  break;
        case FEATUREQUERY:
        case FEATURENQUERY:
	case FEATUREQUERYMAP:
        case FEATURENQUERYMAP:
	  if((SelectLayerIndex = msGetLayerIndex(msObj->Map, SelectLayer)) == -1) { /* force the selection layer on */
	    msSetError(MS_WEBERR, "Selection layer not set or references an invalid layer.", "mapserv()"); 
	    writeError();
	  }
	  msObj->Map->layers[SelectLayerIndex].status = MS_ON;
	  
	  if(msObj->Mode == FEATUREQUERY || msObj->Mode == FEATUREQUERYMAP) {
	    switch(QueryCoordSource) {
	    case FROMIMGPNT:
	      msObj->Map->extent = msObj->ImgExt; /* use the existing map extent */	
	      setCoordinate();
	      if((status = msQueryByPoint(msObj->Map, SelectLayerIndex, MS_SINGLE, msObj->MapPnt, 0)) != MS_SUCCESS) writeError();
	      break;
	    case FROMUSERPNT: /* only a buffer makes sense */
	      if(msObj->Buffer == -1) {
		msSetError(MS_WEBERR, "Point given but no search buffer specified.", "mapserv()");
		writeError();
	      }
	      if((status = msQueryByPoint(msObj->Map, SelectLayerIndex, MS_SINGLE, msObj->MapPnt, msObj->Buffer)) != MS_SUCCESS) writeError();
	      break;
	    default:
	      msSetError(MS_WEBERR, "No way to the initial search, not enough information.", "mapserv()");
	      writeError();
	      break;
	    }	  
	  } else { /* FEATURENQUERY/FEATURENQUERYMAP */
	    switch(QueryCoordSource) {
	    case FROMIMGPNT:
	      msObj->Map->extent = msObj->ImgExt; /* use the existing map extent */	
	      setCoordinate();
	      if((status = msQueryByPoint(msObj->Map, SelectLayerIndex, MS_MULTIPLE, msObj->MapPnt, 0)) != MS_SUCCESS) writeError();
	      break;	 
	    case FROMIMGBOX:
	      break;
	    case FROMUSERPNT: /* only a buffer makes sense */
	      if((status = msQueryByPoint(msObj->Map, SelectLayerIndex, MS_MULTIPLE, msObj->MapPnt, msObj->Buffer)) != MS_SUCCESS) writeError();
	    default:
	      setExtent(msObj);
	      if((status = msQueryByRect(msObj->Map, SelectLayerIndex, msObj->Map->extent)) != MS_SUCCESS) writeError();
	      break;
	    }
	  } /* end switch */
	
	  if(msQueryByFeatures(msObj->Map, QueryLayerIndex, SelectLayerIndex) != MS_SUCCESS) writeError();
      
	  break;
        case ITEMQUERY:
	case ITEMNQUERY:
        case ITEMQUERYMAP:
        case ITEMNQUERYMAP:

	  if(QueryCoordSource != NONE && !msObj->UseShapes)
	    setExtent(msObj); /* set user area of interest */

	  if(msObj->Mode == ITEMQUERY || msObj->Mode == ITEMQUERYMAP) {
	    if((status = msQueryByAttributes(msObj->Map, QueryLayerIndex, QueryItem, QueryString, MS_SINGLE)) != MS_SUCCESS) writeError();
	  } else {
	    if((status = msQueryByAttributes(msObj->Map, QueryLayerIndex, QueryItem, QueryString, MS_MULTIPLE)) != MS_SUCCESS) writeError();
          }

	  break;
        case NQUERY:
        case NQUERYMAP:
	  switch(QueryCoordSource) {
	  case FROMIMGPNT:	  
	    setCoordinate();
	  
	    if(SearchMap) { // compute new extent, pan etc then search that extent
	      setExtent(msObj);
	      msObj->Map->cellsize = msAdjustExtent(&(msObj->Map->extent), msObj->Map->width, msObj->Map->height);
	      if((status = msCalculateScale(msObj->Map->extent, msObj->Map->units, msObj->Map->width, msObj->Map->height, msObj->Map->resolution, &msObj->Map->scale)) != MS_SUCCESS) writeError();
	      if((status = msQueryByRect(msObj->Map, QueryLayerIndex, msObj->Map->extent)) != MS_SUCCESS) writeError();
	    } else {
	      msObj->Map->extent = msObj->ImgExt; // use the existing image parameters
	      msObj->Map->width = msObj->ImgCols;
	      msObj->Map->height = msObj->ImgRows;
	      if((status = msCalculateScale(msObj->Map->extent, msObj->Map->units, msObj->Map->width, msObj->Map->height, msObj->Map->resolution, &msObj->Map->scale)) != MS_SUCCESS) writeError();	 
	      if((status = msQueryByPoint(msObj->Map, QueryLayerIndex, MS_MULTIPLE, msObj->MapPnt, 0)) != MS_SUCCESS) writeError();
	    }
	    break;	  
	  case FROMIMGBOX:	  
	    if(SearchMap) { // compute new extent, pan etc then search that extent
	      setExtent(msObj);
	      if((status = msCalculateScale(msObj->Map->extent, msObj->Map->units, msObj->Map->width, msObj->Map->height, msObj->Map->resolution, &msObj->Map->scale)) != MS_SUCCESS) writeError();
	      msObj->Map->cellsize = msAdjustExtent(&(msObj->Map->extent), msObj->Map->width, msObj->Map->height);
	      if((status = msQueryByRect(msObj->Map, QueryLayerIndex, msObj->Map->extent)) != MS_SUCCESS) writeError();
	    } else {
	      double cellx, celly;
	    
	      msObj->Map->extent = msObj->ImgExt; // use the existing image parameters
	      msObj->Map->width = msObj->ImgCols;
	      msObj->Map->height = msObj->ImgRows;
	      if((status = msCalculateScale(msObj->Map->extent, msObj->Map->units, msObj->Map->width, msObj->Map->height, msObj->Map->resolution, &msObj->Map->scale)) != MS_SUCCESS) writeError();	  
	    
	      cellx = MS_CELLSIZE(msObj->ImgExt.minx, msObj->ImgExt.maxx, msObj->ImgCols); // calculate the new search extent
	      celly = MS_CELLSIZE(msObj->ImgExt.miny, msObj->ImgExt.maxy, msObj->ImgRows);
	      msObj->RawExt.minx = MS_IMAGE2MAP_X(msObj->ImgBox.minx, msObj->ImgExt.minx, cellx);	      
	      msObj->RawExt.maxx = MS_IMAGE2MAP_X(msObj->ImgBox.maxx, msObj->ImgExt.minx, cellx);
	      msObj->RawExt.maxy = MS_IMAGE2MAP_Y(msObj->ImgBox.miny, msObj->ImgExt.maxy, celly); // y's are flip flopped because img/map coordinate systems are
	      msObj->RawExt.miny = MS_IMAGE2MAP_Y(msObj->ImgBox.maxy, msObj->ImgExt.maxy, celly);

	      if((status = msQueryByRect(msObj->Map, QueryLayerIndex, msObj->RawExt)) != MS_SUCCESS) writeError();
	    }
	    break;
	  case FROMIMGSHAPE:
	    msObj->Map->extent = msObj->ImgExt; // use the existing image parameters
	    msObj->Map->width = msObj->ImgCols;
	    msObj->Map->height = msObj->ImgRows;
	    msObj->Map->cellsize = msAdjustExtent(&(msObj->Map->extent), msObj->Map->width, msObj->Map->height);
	    if((status = msCalculateScale(msObj->Map->extent, msObj->Map->units, msObj->Map->width, msObj->Map->height, msObj->Map->resolution, &msObj->Map->scale)) != MS_SUCCESS) writeError();
	  
	    // convert from image to map coordinates here (see setCoordinate)
	    for(i=0; i<msObj->SelectShape.numlines; i++) {
	      for(j=0; j<msObj->SelectShape.line[i].numpoints; j++) {
	        msObj->SelectShape.line[i].point[j].x = MS_IMAGE2MAP_X(msObj->SelectShape.line[i].point[j].x, msObj->Map->extent.minx, msObj->Map->cellsize);
	        msObj->SelectShape.line[i].point[j].y = MS_IMAGE2MAP_Y(msObj->SelectShape.line[i].point[j].y, msObj->Map->extent.maxy, msObj->Map->cellsize);
	      }
	    }
	  
	    if((status = msQueryByShape(msObj->Map, QueryLayerIndex, &msObj->SelectShape)) != MS_SUCCESS) writeError();
	    break;	  
	  case FROMUSERPNT:
	    if(msObj->Buffer == 0) {
	      if((status = msQueryByPoint(msObj->Map, QueryLayerIndex, MS_MULTIPLE, msObj->MapPnt, msObj->Buffer)) != MS_SUCCESS) writeError();
	      setExtent(msObj);
	    } else {
	      setExtent(msObj);
	      if(SearchMap) { // the extent should be tied to a map, so we need to "adjust" it
		if((status = msCalculateScale(msObj->Map->extent, msObj->Map->units, msObj->Map->width, msObj->Map->height, msObj->Map->resolution, &msObj->Map->scale)) != MS_SUCCESS) writeError();
		msObj->Map->cellsize = msAdjustExtent(&(msObj->Map->extent), msObj->Map->width, msObj->Map->height); 
	      }
	      if((status = msQueryByRect(msObj->Map, QueryLayerIndex, msObj->Map->extent)) != MS_SUCCESS) writeError();
	    }
	    break;
	  case FROMUSERSHAPE:
	    setExtent(msObj);	    
	    if((status = msQueryByShape(msObj->Map, QueryLayerIndex, &msObj->SelectShape)) != MS_SUCCESS) writeError();
	    break;	  
	  default: // from an extent of some sort
	    setExtent(msObj);
	    if(SearchMap) { // the extent should be tied to a map, so we need to "adjust" it
	      if((status = msCalculateScale(msObj->Map->extent, msObj->Map->units, msObj->Map->width, msObj->Map->height, msObj->Map->resolution, &msObj->Map->scale)) != MS_SUCCESS) writeError();
	      msObj->Map->cellsize = msAdjustExtent(&(msObj->Map->extent), msObj->Map->width, msObj->Map->height); 
	    }	    
	    if((status = msQueryByRect(msObj->Map, QueryLayerIndex, msObj->Map->extent)) != MS_SUCCESS) writeError();
	    break;
	  }      
	  break;
        case QUERY:
        case QUERYMAP:
	  switch(QueryCoordSource) {
	  case FROMIMGPNT:
	    setCoordinate();
	    msObj->Map->extent = msObj->ImgExt; // use the existing image parameters
	    msObj->Map->width = msObj->ImgCols;
	    msObj->Map->height = msObj->ImgRows;
	    if((status = msCalculateScale(msObj->Map->extent, msObj->Map->units, msObj->Map->width, msObj->Map->height, msObj->Map->resolution, &msObj->Map->scale)) != MS_SUCCESS) writeError();	 	  
	    if((status = msQueryByPoint(msObj->Map, QueryLayerIndex, MS_SINGLE, msObj->MapPnt, 0)) != MS_SUCCESS) writeError();
	    break;
	  
	  case FROMUSERPNT: /* only a buffer makes sense, DOES IT? */	
	    setExtent(msObj);	
	    if((status = msQueryByPoint(msObj->Map, QueryLayerIndex, MS_SINGLE, msObj->MapPnt, msObj->Buffer)) != MS_SUCCESS) writeError();
	    break;
	  
	  default:
	    msSetError(MS_WEBERR, "Query mode needs a point, imgxy and mapxy are not set.", "mapserv()");
	    writeError();
	    break;
	  }
	  break;
        case INDEXQUERY:
        case INDEXQUERYMAP:
	  if((status = msQueryByIndex(msObj->Map, QueryLayerIndex, TileIndex, ShapeIndex)) != MS_SUCCESS) writeError();
	  break;
        } // end mode switch
      }
	  
      if(msObj->Map->querymap.width != -1) msObj->Map->width = msObj->Map->querymap.width; // make sure we use the right size
      if(msObj->Map->querymap.height != -1) msObj->Map->height = msObj->Map->querymap.height;

      if(msObj->UseShapes)
	setExtentFromShapes();

      // just return the image
      if(msObj->Mode == QUERYMAP || msObj->Mode == NQUERYMAP || msObj->Mode == ITEMQUERYMAP || msObj->Mode == ITEMNQUERYMAP || msObj->Mode == FEATUREQUERYMAP || msObj->Mode == FEATURENQUERYMAP || msObj->Mode == ITEMFEATUREQUERYMAP || msObj->Mode == ITEMFEATURENQUERYMAP || msObj->Mode == INDEXQUERYMAP) {

	checkWebScale(msObj);

	img = msDrawQueryMap(msObj->Map);
	if(!img) writeError();

	msIO_printf("Content-type: %s%c%c",MS_IMAGE_MIME_TYPE(msObj->Map->outputformat), 10,10);
	status = msSaveImage(msObj->Map, img, NULL);
	if(status != MS_SUCCESS) writeError();
	msFreeImage(img);

      } 
       else { // process the query through templates
          if (msReturnTemplateQuery(msObj, msObj->Map->web.queryformat, NULL) != MS_SUCCESS) writeError();
          
          if(msObj->SaveQuery) {
             sprintf(buffer, "%s%s%s%s", msObj->Map->web.imagepath, msObj->Map->name, msObj->Id, MS_QUERY_EXTENSION);
             if((status = msSaveQuery(msObj->Map, buffer)) != MS_SUCCESS) return status;
          }
       }

    } else if(msObj->Mode == COORDINATE) {
      setCoordinate(); // mouse click => map coord
      returnCoordinate(msObj->MapPnt);
    }

    writeLog(MS_FALSE);
   
    // Clean-up (the following are not stored as part of the msObj)
    if(QueryItem) free(QueryItem);
    if(QueryString) free(QueryString);
    if(QueryLayer) free(QueryLayer);
    if(SelectLayer) free(SelectLayer);
    if(QueryFile) free(QueryFile);
   
    msFreeMapServObj(msObj);

#ifdef USE_FASTCGI
    msResetErrorList();
    }
#endif

    msCleanup();

    exit(0); // end MapServer
} 
