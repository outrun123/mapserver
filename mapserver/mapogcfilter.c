/**********************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  OGC Filter Encoding implementation
 * Author:   Y. Assefa, DM Solutions Group (assefa@dmsolutions.ca)
 *
 **********************************************************************
 * Copyright (c) 2003, Y. Assefa, DM Solutions Group Inc
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 ****************************************************************************/


#ifdef USE_OGR
#include "cpl_minixml.h"
#endif

#include "mapogcfilter.h"
#include "mapserver.h"
#include "mapowscommon.h"

MS_CVSID("$Id$")

#ifdef USE_OGR

static int compare_ints( const void * a, const void * b)
{
    return (*(int*)a) - (*(int*)b);
}

static int FLTIsNumeric(char *pszValue)
{
    if (pszValue)
    {
          /*the regex seems to have a problem on windows when mapserver is built using
            PHP regex*/
#if defined(_WIN32) && !defined(__CYGWIN__)
        int i = 0, nLength=0, bString=0;

        nLength = strlen(pszValue);
        for (i=0; i<nLength; i++)
        {
            if (i == 0)
            {
                if (!isdigit(pszValue[i]) &&  pszValue[i] != '-')
                {
                     bString = 1;
                     break;
                }
            }
            else if (!isdigit(pszValue[i]) &&  pszValue[i] != '.')
            {
                bString = 1;
                break;
            }
        }
        if (!bString)
          return MS_TRUE;
#else
        if (msEvalRegex("^[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?$", pszValue) == MS_TRUE)
          return MS_TRUE;
#endif
    }

    return MS_FALSE;
}

int FLTogrConvertGeometry(OGRGeometryH hGeometry, shapeObj *psShape,
                          OGRwkbGeometryType nType) 
{
    return msOGRGeometryToShape(hGeometry, psShape, nType);
}

int FLTShapeFromGMLTree(CPLXMLNode *psTree, shapeObj *psShape , char **ppszSRS)
{
    char *pszSRS = NULL;
    if (psTree && psShape)
    {
        CPLXMLNode *psNext = psTree->psNext;
        OGRGeometryH hGeometry = NULL;

        psTree->psNext = NULL;
        hGeometry = OGR_G_CreateFromGMLTree(psTree );
        psTree->psNext = psNext;

        if (hGeometry)
        {
            FLTogrConvertGeometry(hGeometry, psShape, 
                                  OGR_G_GetGeometryType(hGeometry));
        }

        pszSRS = (char *)CPLGetXMLValue(psTree, "srsName", NULL);
        if (ppszSRS && pszSRS)
          *ppszSRS = strdup(pszSRS);

        return MS_TRUE;
    }

    return MS_FALSE;
}

int FLTGetGeosOperator(char *pszValue)
{
    if (!pszValue)
      return -1;

    if (strcasecmp(pszValue, "Equals") == 0)
      return MS_GEOS_EQUALS;
    else if (strcasecmp(pszValue, "Intersect") == 0 ||
             strcasecmp(pszValue, "Intersects") == 0)
      return MS_GEOS_INTERSECTS;
    else if (strcasecmp(pszValue, "Disjoint") == 0)
      return MS_GEOS_DISJOINT;
     else if (strcasecmp(pszValue, "Touches") == 0)
      return MS_GEOS_TOUCHES;
     else if (strcasecmp(pszValue, "Crosses") == 0)
      return MS_GEOS_CROSSES;
     else if (strcasecmp(pszValue, "Within") == 0)
      return MS_GEOS_WITHIN;
     else if (strcasecmp(pszValue, "Contains") == 0)
      return MS_GEOS_CONTAINS;
     else if (strcasecmp(pszValue, "Overlaps") == 0)
      return MS_GEOS_OVERLAPS;
     else if (strcasecmp(pszValue, "Beyond") == 0)
      return MS_GEOS_BEYOND;
    else if (strcasecmp(pszValue, "DWithin") == 0)
      return MS_GEOS_DWITHIN;

    return -1;
}

int FLTIsGeosNode(char *pszValue)
{
    if (FLTGetGeosOperator(pszValue) == -1)
      return MS_FALSE;

    return MS_TRUE;
}

int FTLParseEpsgString(char *pszEpsg, projectionObj *psProj)
{
    int nStatus = MS_FALSE;
    int nTokens = 0;
    char **tokens = NULL;
    int nEpsgTmp=0;

#ifdef USE_PROJ
    if (pszEpsg && psProj)
    {
        nTokens = 0;
        tokens = msStringSplit(pszEpsg,'#', &nTokens);
        if (tokens && nTokens == 2)
        {
            char szTmp[32];
            sprintf(szTmp, "init=epsg:%s",tokens[1]);
            msInitProjection(psProj);
            if (msLoadProjectionString(psProj, szTmp) == 0)
              nStatus = MS_TRUE;
        }
        else if (tokens &&  nTokens == 1)
        {
            if (tokens)
              msFreeCharArray(tokens, nTokens);
            nTokens = 0;

            tokens = msStringSplit(pszEpsg,':', &nTokens);
            nEpsgTmp = -1;
            if (tokens &&  nTokens == 1)
            {
                nEpsgTmp = atoi(tokens[0]);
                
            }
            else if (tokens &&  nTokens == 2)
            {
                nEpsgTmp = atoi(tokens[1]);
            }
            if (nEpsgTmp > 0)
            {
                char szTmp[32];
                sprintf(szTmp, "init=epsg:%d",nEpsgTmp);
                msInitProjection(psProj);
                if (msLoadProjectionString(psProj, szTmp) == 0)
                  nStatus = MS_TRUE;
            }
        }
        if (tokens)
          msFreeCharArray(tokens, nTokens);
    }
#endif
    return nStatus;
}
    
/************************************************************************/
/*                        FLTGetQueryResultsForNode                     */
/*                                                                      */
/*      Return an array of shape id's selected after the filter has     */
/*      been applied.                                                   */
/*      Assuming here that the node is not a logical node but           */
/*      spatial or comparaison.                                         */
/************************************************************************/
int FLTGetQueryResultsForNode(FilterEncodingNode *psNode, mapObj *map, 
                              int iLayerIndex, int **ppanResults, int *pnResults,
                              int bOnlySpatialFilter)
{
    char *szExpression = NULL;
    int bIsBBoxFilter =0,  i=0;
    char *szEPSG = NULL;
    rectObj sQueryRect = map->extent;
    layerObj *lp = NULL;
    projectionObj sProjTmp;
    int bPointQuery = 0, bShapeQuery=0;
    shapeObj *psQueryShape = NULL;
    double dfDistance = -1;
    double dfCurrentTolerance = 0;
    int nUnit = -1;
    int bUseGeos = 0;
    int geos_operator = -1;
    shapeObj *psTmpShape = NULL;
    int *panResults=NULL;
    int status = MS_SUCCESS;

    if (!psNode || !map || iLayerIndex < 0 ||
        iLayerIndex > map->numlayers-1)
      return status;
  
    lp = GET_LAYER(map, iLayerIndex);

    if (!bOnlySpatialFilter)
      szExpression = FLTGetMapserverExpression(psNode, lp);

    bIsBBoxFilter = FLTIsBBoxFilter(psNode);
    bUseGeos = 1;
    if (strcasecmp(psNode->pszValue, "BBOX") == 0)
      bUseGeos = 0;
    else
      geos_operator = FLTGetGeosOperator(psNode->pszValue);

    if (bIsBBoxFilter)
      szEPSG = FLTGetBBOX(psNode, &sQueryRect);
    else if ((bPointQuery = FLTIsPointFilter(psNode)))
    {
        psQueryShape = FLTGetShape(psNode, &dfDistance, &nUnit);
    }
    else if (FLTIsLineFilter(psNode) || FLTIsPolygonFilter(psNode))
    {
        bShapeQuery = 1;
        psQueryShape = FLTGetShape(psNode, &dfDistance, &nUnit);
        
            
    }

    if (!szExpression && !szEPSG && !bIsBBoxFilter 
        && !bPointQuery && !bShapeQuery && (bOnlySpatialFilter == MS_FALSE))
      return status;


    if (szExpression)
    {
        for(i=0;i<lp->numclasses;i++) 
        {
            if (lp->class[i] != NULL) {
                lp->class[i]->layer=NULL;
                if ( freeClass(lp->class[i]) == MS_SUCCESS ) {
                    msFree(lp->class[i]);
                    lp->class[i] = NULL;
                }
            }
        }
        lp->numclasses = 0;
        if (msGrowLayerClasses(lp) == NULL)
            return MS_FAILURE;
       	initClass(lp->class[0]);

        lp->class[0]->type = lp->type;
        lp->numclasses = 1;
        msLoadExpressionString(&lp->class[0]->expression, 
                               szExpression);

        if (!lp->class[0]->template)
          lp->class[0]->template = strdup("ttt.html");
/* -------------------------------------------------------------------- */
/*      Need to free the template so the all shapes's will not end      */
/*      up being queried. The query is dependent on the class           */
/*      expression.                                                     */
/* -------------------------------------------------------------------- */
        if (lp->template)
        {
            free(lp->template);
            lp->template = NULL;
        }
    }
    else if (!bOnlySpatialFilter)
    {
        /* if there are no expression (so no template set in the classes, */
        /* make sure that the layer is queryable by setting the template  */
        /* parameter */
        if (!lp->template)
          lp->template = strdup("ttt.html");
    }
/* -------------------------------------------------------------------- */
/*      Use the epsg code to reproject the values from the QueryRect    */
/*      to the map projection.                                          */
/*      The srs should be a string like                                 */
/*      srsName="http://www.opengis.net/gml/srs/epsg.xml#4326".         */
/*      We will just extract the value after the # and assume that      */
/*      It corresponds to the epsg code on the system. This syntax      */
/*      is the one descibed in the GML specification.                   */
/*                                                                      */
/*       There is also several servers requesting the box with the      */
/*      srsName using the following syntax : <Box                       */
/*      srsName="EPSG:42304">. So we also support this syntax.          */
/*                                                                      */
/* -------------------------------------------------------------------- */
    if(szEPSG && map->projection.numargs > 0)
    {
        if (FTLParseEpsgString(szEPSG, &sProjTmp))
          msProjectRect(&sProjTmp, &map->projection, &sQueryRect);
    }


    if (szExpression || bIsBBoxFilter || (bOnlySpatialFilter && !bIsBBoxFilter && !bPointQuery && !bShapeQuery)) {
      map->query.type = MS_QUERY_BY_RECT;
      map->query.mode = MS_QUERY_MULTIPLE;
      map->query.layer = lp->index;
      map->query.rect = sQueryRect;
      status = msQueryByRect(map);
    } else if (bPointQuery && psQueryShape && psQueryShape->numlines > 0
             && psQueryShape->line[0].numpoints > 0) /* && dfDistance >=0) */
    {
        if (psNode->pszSRS &&  map->projection.numargs > 0 &&
            FTLParseEpsgString(psNode->pszSRS, &sProjTmp))
           msProjectShape(&sProjTmp, &map->projection, psQueryShape);

      if (bUseGeos)
        {
            if ((strcasecmp(psNode->pszValue, "DWithin") == 0 ||
                 strcasecmp(psNode->pszValue, "Beyond") == 0 ) &&
                dfDistance > 0)
            {
                if (nUnit >=0 && nUnit != map->units)
                  dfDistance *= msInchesPerUnit(nUnit,0)/msInchesPerUnit(map->units,0);

                psTmpShape = msGEOSBuffer(psQueryShape, dfDistance);
                if (psTmpShape)
                {
                    map->query.type = MS_QUERY_BY_OPERATOR;
                    map->query.mode = MS_QUERY_MULTIPLE;
                    map->query.layer = lp->index;
                    map->query.shape = psTmpShape;
                    map->query.op = geos_operator;
                    status = msQueryByOperator(map);
                }
            }
            else 
            {
              map->query.type = MS_QUERY_BY_OPERATOR;
              map->query.mode = MS_QUERY_MULTIPLE;
              map->query.layer = lp->index;
              map->query.shape = (shapeObj *) malloc(sizeof(shapeObj));
              msInitShape(map->query.shape);
              msCopyShape(psQueryShape, map->query.shape);
              map->query.op = geos_operator;
              status = msQueryByOperator(map);
            }
        } 
        else
        {
            /*Set the tolerance to the distance value or 0 is invalid
              set the the unit if unit is valid.
              Bug 1342 for details*/
            lp->tolerance = 0;
            if (dfDistance > 0)
            {
                lp->tolerance = dfDistance;
                if (nUnit >=0)
                  lp->toleranceunits = nUnit;
            }

            map->query.type = MS_QUERY_BY_POINT;
            map->query.mode = MS_QUERY_MULTIPLE;
            map->query.layer = lp->index;
            map->query.point = psQueryShape->line[0].point[0]; 
            status = msQueryByPoint(map);
        }
    }
    else if (bShapeQuery && psQueryShape && psQueryShape->numlines > 0
             && psQueryShape->line[0].numpoints > 0)
    {
        /*reproject shape if epsg was set*/
        if (psNode->pszSRS &&  map->projection.numargs > 0 &&
            FTLParseEpsgString(psNode->pszSRS, &sProjTmp))
           msProjectShape(&sProjTmp, &map->projection, psQueryShape);


        if (bUseGeos)
        {
            if ((strcasecmp(psNode->pszValue, "DWithin") == 0 ||
                 strcasecmp(psNode->pszValue, "Beyond") == 0 ) &&
                dfDistance > 0)
            {
/* -------------------------------------------------------------------- */
/*      if units is set, covert value from unit to map unit.            */
/* -------------------------------------------------------------------- */
                if (nUnit >=0 && nUnit != map->units)
                  dfDistance *= msInchesPerUnit(nUnit,0)/msInchesPerUnit(map->units,0);

                psTmpShape = msGEOSBuffer(psQueryShape, dfDistance);
                if (psTmpShape)
                {
                    map->query.type = MS_QUERY_BY_OPERATOR;
                    map->query.mode = MS_QUERY_MULTIPLE;
                    map->query.layer = lp->index;
                    map->query.shape = psTmpShape;
                    map->query.op = geos_operator;
                    status = msQueryByOperator(map);
                }
            } 
            else
            {
              map->query.type = MS_QUERY_BY_OPERATOR;
              map->query.mode = MS_QUERY_MULTIPLE;
              map->query.layer = lp->index;
              map->query.shape = (shapeObj *) malloc(sizeof(shapeObj));
              msInitShape(map->query.shape);
              msCopyShape(psQueryShape, map->query.shape);
              map->query.op = geos_operator;
              status = msQueryByOperator(map);
            }
        }
        else
        {
            /* disable any tolerance value already set for the layer (Bug 768)
               Set the tolerance to the distance value or 0 is invalid
               set the the unit if unit is valid.
               Bug 1342 for details */

            dfCurrentTolerance = lp->tolerance;
            lp->tolerance = 0;
            if (dfDistance > 0)
            {
                lp->tolerance = dfDistance;
                if (nUnit >=0)
                  lp->toleranceunits = nUnit;
            }
            map->query.type = MS_QUERY_BY_SHAPE;
            map->query.mode = MS_QUERY_MULTIPLE;
            map->query.layer = lp->index;
            map->query.shape = (shapeObj *) malloc(sizeof(shapeObj));
            msInitShape(map->query.shape);
            msCopyShape(psQueryShape, map->query.shape);
            status = msQueryByShape(map);

            lp->tolerance = dfCurrentTolerance;
        }
    }

    if (szExpression)
      free(szExpression);

    if (lp && lp->resultcache && lp->resultcache->numresults > 0)
    {
        panResults = (int *)malloc(sizeof(int)*lp->resultcache->numresults);
        for (i=0; i<lp->resultcache->numresults; i++)
          panResults[i] = lp->resultcache->results[i].shapeindex;

        qsort(panResults, lp->resultcache->numresults, 
              sizeof(int), compare_ints);

        *pnResults = lp->resultcache->numresults;
        *ppanResults = panResults;
        
    }

    return status;

}
 
int FLTGML2Shape_XMLNode(CPLXMLNode *psNode, shapeObj *psShp)
{
    lineObj line={0,NULL};
    CPLXMLNode *psCoordinates = NULL;
    char *pszTmpCoord = NULL;
    char **szCoords = NULL;
    int nCoords = 0;


    if (!psNode || !psShp)
      return MS_FALSE;

    
    if( strcasecmp(psNode->pszValue,"PointType") == 0
        || strcasecmp(psNode->pszValue,"Point") == 0)
    {
        psCoordinates = CPLGetXMLNode(psNode, "coordinates");
        
        if (psCoordinates && psCoordinates->psChild && 
            psCoordinates->psChild->pszValue)
        {
            pszTmpCoord = psCoordinates->psChild->pszValue;
            szCoords = msStringSplit(pszTmpCoord, ',', &nCoords);
            if (szCoords && nCoords >= 2)
            {
                line.numpoints = 1;
                line.point = (pointObj *)malloc(sizeof(pointObj));
                line.point[0].x = atof(szCoords[0]);
                line.point[0].y =  atof(szCoords[1]);
#ifdef USE_POINT_Z_M
                line.point[0].m = 0;
#endif

                psShp->type = MS_SHAPE_POINT;

                msAddLine(psShp, &line);
                free(line.point);

                return MS_TRUE;
            }
        }
    }
    
    return MS_FALSE;
}



static int addResult(resultCacheObj *cache, int classindex, int shapeindex, int tileindex)
{
  int i;

  if(cache->numresults == cache->cachesize) { /* just add it to the end */
    if(cache->cachesize == 0)
      cache->results = (resultCacheMemberObj *) malloc(sizeof(resultCacheMemberObj)*MS_RESULTCACHEINCREMENT);
    else
      cache->results = (resultCacheMemberObj *) realloc(cache->results, sizeof(resultCacheMemberObj)*(cache->cachesize+MS_RESULTCACHEINCREMENT));
    if(!cache->results) {
      msSetError(MS_MEMERR, "Realloc() error.", "addResult()");
      return(MS_FAILURE);
    }   
    cache->cachesize += MS_RESULTCACHEINCREMENT;
  }

  i = cache->numresults;

  cache->results[i].classindex = classindex;
  cache->results[i].tileindex = tileindex;
  cache->results[i].shapeindex = shapeindex;
  cache->numresults++;

  return(MS_SUCCESS);
}
 


/************************************************************************/
/*                         FLTAddToLayerResultCache                     */
/*                                                                      */
/*      Utility function to add shaped to the layer result              */
/*      cache. This function is used with the WFS GetFeature            */
/*      interface and the cache will be used later on to output the     */
/*      result into gml (the only important thing here is the shape     */
/*      id).                                                            */
/************************************************************************/
void FLTAddToLayerResultCache(int *anValues, int nSize, mapObj *map, 
                              int iLayerIndex)
{
    layerObj *psLayer = NULL;
    int i = 0, status = MS_FALSE;
    shapeObj shape;
    int nClassIndex = -1;
    char        annotate=MS_TRUE;

    if (!anValues || nSize <= 0 || !map || iLayerIndex < 0 ||
        iLayerIndex > map->numlayers-1)
      return;

    psLayer = (GET_LAYER(map, iLayerIndex));
    if (psLayer->resultcache)
    {
        if (psLayer->resultcache->results)
          free (psLayer->resultcache->results);
        free(psLayer->resultcache);
    }

    psLayer->resultcache = (resultCacheObj *)malloc(sizeof(resultCacheObj));
    initResultCache(psLayer->resultcache);

    /*this will force the queries to retrive the shapes using
     their unique id #3305*/
    psLayer->resultcache->usegetshape = MS_TRUE;

    status = msLayerOpen(psLayer);
    if (status != MS_SUCCESS) 
      return;
    
    annotate = msEvalContext(map, psLayer, psLayer->labelrequires);
    if(map->scaledenom > 0) 
    {
        if((psLayer->labelmaxscaledenom != -1) && (map->scaledenom >= psLayer->labelmaxscaledenom)) 
          annotate = MS_FALSE;
        if((psLayer->labelminscaledenom != -1) && (map->scaledenom < psLayer->labelminscaledenom)) 
          annotate = MS_FALSE;
    }
    status = msLayerWhichItems(psLayer, MS_TRUE, NULL);
    if (status != MS_SUCCESS) 
      return;

    for (i=0; i<nSize; i++)
    {
        msInitShape(&shape);
        status = msLayerGetShape(psLayer, &shape, -1, anValues[i]);

        if (status != MS_SUCCESS)
          nClassIndex = -1;
        else
          nClassIndex = msShapeGetClass(psLayer, &shape, map->scaledenom, NULL, 0);
        
        addResult(psLayer->resultcache, nClassIndex, anValues[i], shape.tileindex);

#ifdef USE_PROJ
      if(psLayer->project && msProjectionsDiffer(&(psLayer->projection), 
                                                 &(map->projection)))
	msProjectShape(&(psLayer->projection), &(map->projection), &shape);
#endif

        if(psLayer->resultcache->numresults == 1)
          psLayer->resultcache->bounds = shape.bounds;
        else
          msMergeRect(&(psLayer->resultcache->bounds), &shape.bounds);
    }

    /*
      At this point a msQuery was called successfully and the layer is still open.
      No need to reopen and close it. These changes are related to RFC #52 Bug 3069.
      msLayerClose(psLayer);
    */
}

/************************************************************************/
/*                               FLTIsInArray                           */
/*                                                                      */
/*      Verify if a certain value is inside an ordered array.           */
/************************************************************************/
int FLTIsInArray(int *panArray, int nSize, int nValue)
{
    int i = 0;
    if (panArray && nSize > 0)
    {
        for (i=0; i<nSize; i++)
        {
            if (panArray[i] == nValue)
              return MS_TRUE;
            if (panArray[i] > nValue)
              return MS_FALSE;
        }
    }

    return MS_FALSE;
}




/************************************************************************/
/*                               FLTArraysNot                           */
/*                                                                      */
/*      Return the list of all shape id's in a layer that are not in    */
/*      the array.                                                      */
/************************************************************************/
int FLTArraysNot(int *panArray, int nSize, mapObj *map,
                  int iLayerIndex, int **ppanResults, int *pnResult)
{
    layerObj *psLayer = NULL;
    int *panResults = NULL;
    int *panTmp = NULL;
    int i = 0, iResult = 0;
    
    if (!map || iLayerIndex < 0 || iLayerIndex > map->numlayers-1)
      return MS_SUCCESS;

     psLayer = (GET_LAYER(map, iLayerIndex));
     if (psLayer->template == NULL)
       psLayer->template = strdup("ttt.html");

     map->query.type = MS_QUERY_BY_RECT;
     map->query.mode = MS_QUERY_MULTIPLE;
     map->query.layer = psLayer->index;
     map->query.rect = map->extent;
     msQueryByRect(map);

     free(psLayer->template);
     psLayer->template = NULL;

     if (!psLayer->resultcache || psLayer->resultcache->numresults <= 0)
       return MS_SUCCESS;

     panResults = (int *)malloc(sizeof(int)*psLayer->resultcache->numresults);
     
     panTmp = (int *)malloc(sizeof(int)*psLayer->resultcache->numresults);
     for (i=0; i<psLayer->resultcache->numresults; i++)
       panTmp[i] = psLayer->resultcache->results[i].shapeindex;
     qsort(panTmp, psLayer->resultcache->numresults, 
           sizeof(int), compare_ints);
     
     
     iResult = 0;
      for (i=0; i<psLayer->resultcache->numresults; i++)
      {
          if (!FLTIsInArray(panArray, nSize, panTmp[i]) || 
              panArray == NULL)
            panResults[iResult++] = 
              psLayer->resultcache->results[i].shapeindex;
      }

      free(panTmp);

      if (iResult > 0)
      {
          panResults = (int *)realloc(panResults, sizeof(int)*iResult);
          qsort(panResults, iResult, sizeof(int), compare_ints);
          *pnResult = iResult;

          *ppanResults = panResults;
      }

      return MS_SUCCESS;
}
 
/************************************************************************/
/*                               FLTArraysOr                            */
/*                                                                      */
/*      Utility function to do an OR on 2 arrays.                       */
/************************************************************************/
int FLTArraysOr(int *aFirstArray, int nSizeFirst, 
                 int *aSecondArray, int nSizeSecond,
                 int **ppanResults, int *pnResult)
{
    int nResultSize = 0;
    int *panResults = 0;
    int iResult = 0;
    int i, j;
    
    if (aFirstArray == NULL && aSecondArray == NULL)
      return MS_SUCCESS;

    if (aFirstArray == NULL || aSecondArray == NULL)
    {
        if (aFirstArray && nSizeFirst > 0)
        {
            panResults = (int *)malloc(sizeof(int)*nSizeFirst);
            for (i=0; i<nSizeFirst; i++)
              panResults[i] = aFirstArray[i];
            if (pnResult)
              *pnResult = nSizeFirst;
            if (ppanResults)
              *ppanResults = panResults;
            return MS_SUCCESS;
        }
        else if (aSecondArray && nSizeSecond)
        {
            panResults = (int *)malloc(sizeof(int)*nSizeSecond);
            for (i=0; i<nSizeSecond; i++)
              panResults[i] = aSecondArray[i];
            if (pnResult)
              *pnResult = nSizeSecond;
            if (ppanResults)
              *ppanResults = panResults;
            return MS_SUCCESS;
        }
    }
            
    if (aFirstArray && aSecondArray && nSizeFirst > 0 && 
        nSizeSecond > 0)
    {
        nResultSize = nSizeFirst + nSizeSecond;

        panResults = (int *)malloc(sizeof(int)*nResultSize);
        iResult= 0;

        if (nSizeFirst < nSizeSecond)
        {
            for (i=0; i<nSizeFirst; i++)
              panResults[iResult++] = aFirstArray[i];

            for (i=0; i<nSizeSecond; i++)
            {
                for (j=0; j<nSizeFirst; j++)
                {
                    if (aSecondArray[i] ==  aFirstArray[j])
                      break;
                    if (aSecondArray[i] < aFirstArray[j])
                    {
                        panResults[iResult++] = aSecondArray[i];
                        break;
                    }
                }
                if (j == nSizeFirst)
                    panResults[iResult++] = aSecondArray[i];
            }
        }
        else
        {       
            for (i=0; i<nSizeSecond; i++)
              panResults[iResult++] = aSecondArray[i];

            for (i=0; i<nSizeFirst; i++)
            {
                for (j=0; j<nSizeSecond; j++)
                {
                    if (aFirstArray[i] ==  aSecondArray[j])
                      break;
                    if (aFirstArray[i] < aSecondArray[j])
                    {
                        panResults[iResult++] = aFirstArray[i];
                        break;
                    }
                }
                if (j == nSizeSecond)
                    panResults[iResult++] = aFirstArray[i];
            }
        }
          
        if (iResult > 0)
        {
            panResults = (int *)realloc(panResults, sizeof(int)*iResult);
            qsort(panResults, iResult, sizeof(int), compare_ints);
            *pnResult = iResult;
            *ppanResults = panResults;
        }

    }

    return MS_SUCCESS;
}

/************************************************************************/
/*                               FLTArraysAnd                           */
/*                                                                      */
/*      Utility function to do an AND on 2 arrays.                      */
/************************************************************************/
int FLTArraysAnd(int *aFirstArray, int nSizeFirst, 
                  int *aSecondArray, int nSizeSecond,
                  int **ppanResults, int *pnResult)
{
    int *panResults = NULL;
    int nResultSize =0;
    int iResult = 0;
    int i, j;

    /* assuming arrays are sorted. */
    if (aFirstArray && aSecondArray && nSizeFirst > 0 && 
        nSizeSecond > 0)
    {
        if (nSizeFirst < nSizeSecond)
          nResultSize = nSizeFirst;
        else
          nResultSize = nSizeSecond;
 
        panResults = (int *)malloc(sizeof(int)*nResultSize);
        iResult= 0;

        if (nSizeFirst > nSizeSecond)
        {
            for (i=0; i<nSizeFirst; i++)
            {
                for (j=0; j<nSizeSecond; j++)
                {
                    if (aFirstArray[i] == aSecondArray[j])
                    {
                        panResults[iResult++] = aFirstArray[i];
                        break;
                    }
                    /* if (aFirstArray[i] < aSecondArray[j]) */
                    /* break; */
                }
            }
        }
        else
        {
             for (i=0; i<nSizeSecond; i++)
             {
                 for (j=0; j<nSizeFirst; j++)
                 {
                     if (aSecondArray[i] == aFirstArray[j])
                     {
                         panResults[iResult++] = aSecondArray[i];
                         break;
                     }
                     /* if (aSecondArray[i] < aFirstArray[j]) */
                     /* break; */
                }
            }
        }
        
        if (iResult > 0)
        {
            panResults = (int *)realloc(panResults, sizeof(int)*iResult);
            qsort(panResults, iResult, sizeof(int), compare_ints);
            *pnResult = iResult;
            *ppanResults = panResults;
        }

    }

    return MS_SUCCESS;
}

/************************************************************************/
/*                        FLTIsSimpleFilterNoSpatial                    */
/*                                                                      */
/*      Filter encoding with only attribute queries                     */
/************************************************************************/
int FLTIsSimpleFilterNoSpatial(FilterEncodingNode *psNode)
{
    if (FLTIsSimpleFilter(psNode) &&
        FLTNumberOfFilterType(psNode, "BBOX") == 0)
      return MS_TRUE;

    return MS_FALSE;
}


/************************************************************************/
/*                      FLTApplySimpleSQLFilter()                       */
/************************************************************************/

int FLTApplySimpleSQLFilter(FilterEncodingNode *psNode, mapObj *map, 
                             int iLayerIndex)
{
    layerObj *lp = NULL;
    char *szExpression = NULL;
    rectObj sQueryRect = map->extent;
    char *szEPSG = NULL;
    char **tokens = NULL;
    int nTokens = 0, nEpsgTmp = 0;
    projectionObj sProjTmp;
    char *pszBuffer = NULL;
    int bConcatWhere = 0;
    int bHasAWhere =0;
    char *pszTmp = NULL, *pszTmp2 = NULL;
    
    char *tmpfilename = NULL;

    lp = (GET_LAYER(map, iLayerIndex));

    /* if there is a bbox use it */
    szEPSG = FLTGetBBOX(psNode, &sQueryRect);
    if(szEPSG && map->projection.numargs > 0)
    {
#ifdef USE_PROJ
        nTokens = 0;
        tokens = msStringSplit(szEPSG,'#', &nTokens);
        if (tokens && nTokens == 2)
        {
            char szTmp[32];
            sprintf(szTmp, "init=epsg:%s",tokens[1]);
            msInitProjection(&sProjTmp);
            if (msLoadProjectionString(&sProjTmp, szTmp) == 0)
              msProjectRect(&sProjTmp, &map->projection,  &sQueryRect);
        }
        else if (tokens &&  nTokens == 1)
        {
            if (tokens)
              msFreeCharArray(tokens, nTokens);
            nTokens = 0;

            tokens = msStringSplit(szEPSG,':', &nTokens);
            nEpsgTmp = -1;
            if (tokens &&  nTokens == 1)
            {
                nEpsgTmp = atoi(tokens[0]);
                
            }
            else if (tokens &&  nTokens == 2)
            {
                nEpsgTmp = atoi(tokens[1]);
            }
            if (nEpsgTmp > 0)
            {
                char szTmp[32];
                sprintf(szTmp, "init=epsg:%d",nEpsgTmp);
                msInitProjection(&sProjTmp);
                if (msLoadProjectionString(&sProjTmp, szTmp) == 0)
                  msProjectRect(&sProjTmp, &map->projection,  &sQueryRect);
            }
        }
        if (tokens)
          msFreeCharArray(tokens, nTokens);
#endif
    }

    /* make sure that the layer can be queried*/
    if (!lp->template)
      lp->template = strdup("ttt.html");

    /* if there is no class, create at least one, so that query by rect
       would work*/
    if (lp->numclasses == 0)
    {
        if (msGrowLayerClasses(lp) == NULL)
          return MS_FAILURE;
        initClass(lp->class[0]);
    }

    bConcatWhere = 0;
    bHasAWhere = 0;
    if (lp->connectiontype == MS_POSTGIS || lp->connectiontype ==  MS_ORACLESPATIAL ||
        lp->connectiontype == MS_SDE || lp->connectiontype == MS_PLUGIN)
    {
        szExpression = FLTGetSQLExpression(psNode, lp);
        if (szExpression)
        {
            pszTmp = strdup("(");
            pszTmp = msStringConcatenate(pszTmp, szExpression);
            pszTmp = msStringConcatenate(pszTmp, ")");
            msFree(szExpression);
            szExpression = pszTmp;
        }
    }
    /* concatenates the WHERE clause for OGR layers. This only applies if
       the expression was empty or not of an expression string. If there
       is an sql type expression, it is assumed to have the WHERE clause. 
       If it is an expression and does not have a WHERE it is assumed to be a mapserver
       type expression*/
    else if (lp->connectiontype == MS_OGR)
    {
	if (lp->filter.type != MS_EXPRESSION)
	{
	    szExpression = FLTGetSQLExpression(psNode, lp);
	    bConcatWhere = 1;
	}
	else
	{
	    if (lp->filter.string && EQUALN(lp->filter.string,"WHERE ",6))
	    {
		szExpression = FLTGetSQLExpression(psNode, lp);
		bHasAWhere = 1;
		bConcatWhere =1;
	    }
	    else
	    {
              szExpression = FLTGetMapserverExpression(psNode, lp);
	    }
	}
    }
    else
    {
      szExpression = FLTGetMapserverExpression(psNode, lp);
        
    }
   

    if (szExpression)
    {
        if (bConcatWhere)
          pszBuffer = msStringConcatenate(pszBuffer, "WHERE ");

	/* if the filter is set and it's an expression type, concatenate it with
               this filter. If not just free it */
	if (lp->filter.string && lp->filter.type == MS_EXPRESSION)
	{
	    pszBuffer = msStringConcatenate(pszBuffer, "((");
	    if (bHasAWhere)
	      pszBuffer = msStringConcatenate(pszBuffer, lp->filter.string+6);
	    else
	       pszBuffer = msStringConcatenate(pszBuffer, lp->filter.string);
	    pszBuffer = msStringConcatenate(pszBuffer, ") and ");
	}
	else if (lp->filter.string)
	  freeExpression(&lp->filter);

       
	pszBuffer = msStringConcatenate(pszBuffer, szExpression);

	if(lp->filter.string && lp->filter.type == MS_EXPRESSION)
	  pszBuffer = msStringConcatenate(pszBuffer, ")");
        
        
        msLoadExpressionString(&lp->filter, pszBuffer);
        free(szExpression);
    }

    if (pszBuffer)
      free(pszBuffer);

    map->query.type = MS_QUERY_BY_RECT;
    map->query.mode = MS_QUERY_MULTIPLE;
    map->query.layer = lp->index;
    map->query.rect = sQueryRect;

    if(map->debug == MS_DEBUGLEVEL_VVV)
    {
        tmpfilename = msTmpFile(map->mappath, map->web.imagepath, "_filter.map");
        if (tmpfilename == NULL)
        {
#ifndef _WIN32
            tmpfilename = msTmpFile(NULL, "/tmp/", "_filter.map" );
#else
            tmpfilename = msTmpFile(NULL, "C:\\", "_filter.map");
#endif
        }
        if (tmpfilename)
        {
            msSaveMap(map,tmpfilename);
            msDebug("FLTApplySimpleSQLFilter(): Map file after Filter was applied %s", tmpfilename);
            msFree(tmpfilename);
        }
    }

    /*for oracle connection, if we have a simple filter with no spatial constraints 
      we should set the connection function to NONE to have a better performance
      (#2725)*/
      
    if (lp->connectiontype ==  MS_ORACLESPATIAL && FLTIsSimpleFilterNoSpatial(psNode))
    {
        if (msCaseFindSubstring(lp->data, "USING") == 0)
          lp->data = msStringConcatenate(lp->data, " USING NONE"); 
        else if (msCaseFindSubstring(lp->data, "NONE") == 0)
        {
            /*if one of the functions is used, just replace it with NONE*/
            if (msCaseFindSubstring(lp->data, "FILTER"))
              lp->data = msCaseReplaceSubstring(lp->data, "FILTER", "NONE");
            else if (msCaseFindSubstring(lp->data, "GEOMRELATE"))
              lp->data = msCaseReplaceSubstring(lp->data, "GEOMRELATE", "NONE");
            else if (msCaseFindSubstring(lp->data, "RELATE"))
              lp->data = msCaseReplaceSubstring(lp->data, "RELATE", "NONE");
            else if (msCaseFindSubstring(lp->data, "VERSION"))
            {
                /*should add NONE just before the VERSION. Cases are:
                  DATA "ORA_GEOMETRY FROM data USING VERSION 10g
                  DATA "ORA_GEOMETRY FROM data  USING UNIQUE FID VERSION 10g"
                 */
                pszTmp = (char *)msCaseFindSubstring(lp->data, "VERSION");
                pszTmp2 = msStringConcatenate(pszTmp2, " NONE ");
                pszTmp2 = msStringConcatenate(pszTmp2, pszTmp);
                
                lp->data = msCaseReplaceSubstring(lp->data, pszTmp, pszTmp2);

                msFree(pszTmp2);
                
            }
            else if (msCaseFindSubstring(lp->data, "SRID"))
            {
                lp->data = msStringConcatenate(lp->data, " NONE"); 
            }
        }
    }
    
    return msQueryByRect(map);

    /* return MS_SUCCESS; */
}


       


/************************************************************************/
/*                            FLTIsSimpleFilter                         */
/*                                                                      */
/*      Filter encoding with only attribute queries and only one bbox.  */
/************************************************************************/
int FLTIsSimpleFilter(FilterEncodingNode *psNode)
{
    if (FLTValidForBBoxFilter(psNode))
    {
        if (FLTNumberOfFilterType(psNode, "DWithin") == 0 &&
            FLTNumberOfFilterType(psNode, "Intersect") == 0 &&
            FLTNumberOfFilterType(psNode, "Intersects") == 0 &&
            FLTNumberOfFilterType(psNode, "Equals") == 0 &&
            FLTNumberOfFilterType(psNode, "Disjoint") == 0 &&
            FLTNumberOfFilterType(psNode, "Touches") == 0 &&
            FLTNumberOfFilterType(psNode, "Crosses") == 0 &&
            FLTNumberOfFilterType(psNode, "Within") == 0 &&
            FLTNumberOfFilterType(psNode, "Contains") == 0 &&
            FLTNumberOfFilterType(psNode, "Overlaps") == 0 &&
            FLTNumberOfFilterType(psNode, "Beyond") == 0)
          return TRUE;
    }

    return FALSE;
}


/************************************************************************/
/*                            FLTGetQueryResults                        */
/*                                                                      */
/*      Return an arry of shpe id's after a filetr node was applied     */
/*      on a layer.                                                     */
/************************************************************************/
int FLTGetQueryResults(FilterEncodingNode *psNode, mapObj *map, 
                       int iLayerIndex,  int **ppanResults, int *pnResults,
                       int bOnlySpatialFilter)
{
    int *panLeftResults=NULL, *panRightResults=NULL;
    int nLeftResult=0, nRightResult=0;
    int status = MS_SUCCESS;

    if (psNode->eType == FILTER_NODE_TYPE_LOGICAL)
    {
        if (psNode->psLeftNode)
          status = FLTGetQueryResults(psNode->psLeftNode, map, 
                                      iLayerIndex, &panLeftResults, &nLeftResult,
                                      bOnlySpatialFilter);

        if (psNode->psRightNode)
          status = FLTGetQueryResults(psNode->psRightNode, map,
                                      iLayerIndex, &panRightResults, &nRightResult,
                                      bOnlySpatialFilter);

        if (psNode->pszValue && strcasecmp(psNode->pszValue, "AND") == 0)
          FLTArraysAnd(panLeftResults, nLeftResult, 
                       panRightResults, nRightResult, ppanResults, pnResults);
        else if (psNode->pszValue && strcasecmp(psNode->pszValue, "OR") == 0)
          FLTArraysOr(panLeftResults, nLeftResult, 
                      panRightResults, nRightResult, ppanResults, pnResults);

        else if (psNode->pszValue && strcasecmp(psNode->pszValue, "NOT") == 0)
          FLTArraysNot(panLeftResults, nLeftResult, map, iLayerIndex, ppanResults, 
                       pnResults);
    }
    else
    {
        status = FLTGetQueryResultsForNode(psNode, map, iLayerIndex, 
                                           ppanResults, pnResults , bOnlySpatialFilter);
    }

    return status;
}

int FLTApplySpatialFilterToLayer(FilterEncodingNode *psNode, mapObj *map, 
                                 int iLayerIndex)
{
    return FLTApplyFilterToLayer(psNode, map, iLayerIndex, MS_TRUE);
}

/************************************************************************/
/*                          FLTApplyFilterToLayer                       */
/*                                                                      */
/*      Use the filter encoding node to create mapserver expressions    */
/*      and apply it to the layer.                                      */
/************************************************************************/
int FLTApplyFilterToLayer(FilterEncodingNode *psNode, mapObj *map, 
                          int iLayerIndex, int bOnlySpatialFilter)
{
    layerObj *layer = GET_LAYER(map, iLayerIndex);

    if ( ! layer->vtable) {
        int rv =  msInitializeVirtualTable(layer);
        if (rv != MS_SUCCESS)
            return rv;
    }
    return layer->vtable->LayerApplyFilterToLayer(psNode, map, 
                                                  iLayerIndex, 
                                                  bOnlySpatialFilter);
}

/************************************************************************/
/*               FLTLayerApplyCondSQLFilterToLayer                       */
/*                                                                      */
/* Helper function for layer virtual table architecture                 */
/************************************************************************/
int FLTLayerApplyCondSQLFilterToLayer(FilterEncodingNode *psNode, mapObj *map, 
                                      int iLayerIndex, int bOnlySpatialFilter)
{
/* ==================================================================== */
/*      Check here to see if it is a simple filter and if that is       */
/*      the case, we are going to use the FILTER element on             */
/*      the layer.                                                      */
/* ==================================================================== */
    if (!bOnlySpatialFilter && FLTIsSimpleFilter(psNode))
    {
        return FLTApplySimpleSQLFilter(psNode, map, iLayerIndex);
    }        
    
    return FLTLayerApplyPlainFilterToLayer(psNode, map, iLayerIndex, bOnlySpatialFilter);
}

/************************************************************************/
/*                   FLTLayerApplyPlainFilterToLayer                    */
/*                                                                      */
/* Helper function for layer virtual table architecture                 */
/************************************************************************/
int FLTLayerApplyPlainFilterToLayer(FilterEncodingNode *psNode, mapObj *map, 
                                    int iLayerIndex, int bOnlySpatialFilter)
{
    int *panResults = NULL;
    int nResults = 0;
    layerObj *psLayer = NULL;
    int status;

/* ==================================================================== */
/*      Check here to see if it is a simple filter and if that is       */
/*      the case, we are going to use the FILTER element on             */
/*      the layer.                                                      */
/* ==================================================================== */
    if (!bOnlySpatialFilter && FLTIsSimpleFilter(psNode))
    {
        return FLTApplySimpleSQLFilter(psNode, map, iLayerIndex);
    }        

    
    psLayer = (GET_LAYER(map, iLayerIndex));
    status = FLTGetQueryResults(psNode, map, iLayerIndex,
                                &panResults, &nResults, bOnlySpatialFilter);
    if (panResults) 
        FLTAddToLayerResultCache(panResults, nResults, map, iLayerIndex);
    /* clear the cache if the results is NULL to make sure there aren't */
    /* any left over from intermediate queries. */
    else 
    {
        if (psLayer && psLayer->resultcache)
        {
            if (psLayer->resultcache->results)
                free (psLayer->resultcache->results);
            free(psLayer->resultcache);
            
            psLayer->resultcache = NULL;
        }
    }

    if (panResults)
      free(panResults);
    
    return status;
}



/************************************************************************/
/*            FilterNode *FLTPaserFilterEncoding(char *szXMLString)     */
/*                                                                      */
/*      Parses an Filter Encoding XML string and creates a              */
/*      FilterEncodingNodes corresponding to the string.                */
/*      Returns a pointer to the first node or NULL if                  */
/*      unsuccessfull.                                                  */
/*      Calling function should use FreeFilterEncodingNode function     */
/*      to free memeory.                                                */
/************************************************************************/
FilterEncodingNode *FLTParseFilterEncoding(char *szXMLString)
{
    CPLXMLNode *psRoot = NULL, *psChild=NULL, *psFilter=NULL;
    CPLXMLNode  *psFilterStart = NULL;
    FilterEncodingNode *psFilterNode = NULL;
    
    if (szXMLString == NULL || strlen(szXMLString) <= 0 ||
        (strstr(szXMLString, "Filter") == NULL))
      return NULL;

    psRoot = CPLParseXMLString(szXMLString);
    
    if( psRoot == NULL)
       return NULL;

    /* strip namespaces. We srtip all name spaces (#1350)*/
    CPLStripXMLNamespace(psRoot, NULL, 1);

/* -------------------------------------------------------------------- */
/*      get the root element (Filter).                                  */
/* -------------------------------------------------------------------- */
    psChild = psRoot;
    psFilter = NULL;
     
    while( psChild != NULL )
    {
        if (psChild->eType == CXT_Element &&
            EQUAL(psChild->pszValue,"Filter"))
        {
            psFilter = psChild;
            break;
        }
        else
          psChild = psChild->psNext;
    }

    if (!psFilter)
      return NULL;

    psChild = psFilter->psChild;
    psFilterStart = NULL;
    while (psChild)
    { 
        if (FLTIsSupportedFilterType(psChild))
        {
            psFilterStart = psChild;
            psChild = NULL;
        }
        else
          psChild = psChild->psNext;
    }
              
    if (psFilterStart && FLTIsSupportedFilterType(psFilterStart))
    {
        psFilterNode = FLTCreateFilterEncodingNode();
        
        FLTInsertElementInNode(psFilterNode, psFilterStart);
    }

    CPLDestroyXMLNode( psRoot );

/* -------------------------------------------------------------------- */
/*      validate the node tree to make sure that all the nodes are valid.*/
/* -------------------------------------------------------------------- */
    if (!FLTValidFilterNode(psFilterNode))
      return NULL;


    return psFilterNode;
}


/************************************************************************/
/*      int FLTValidFilterNode(FilterEncodingNode *psFilterNode)        */
/*                                                                      */
/*      Validate that all the nodes are filled properly. We could       */
/*      have parts of the nodes that are correct and part which         */
/*      could be incorrect if the filter string sent is corrupted       */
/*      (eg missing a value :<PropertyName><PropertyName>)              */
/************************************************************************/
int FLTValidFilterNode(FilterEncodingNode *psFilterNode)
{
    int  bReturn = 0;

    if (!psFilterNode)
      return 0;

    if (psFilterNode->eType == FILTER_NODE_TYPE_UNDEFINED)
      return 0;

    if (psFilterNode->psLeftNode)
    {
        bReturn = FLTValidFilterNode(psFilterNode->psLeftNode);
        if (bReturn == 0)
          return 0;
        else if (psFilterNode->psRightNode)
          return FLTValidFilterNode(psFilterNode->psRightNode);
    }

    return 1;
}
/************************************************************************/
/*                          FLTFreeFilterEncodingNode                   */
/*                                                                      */
/*      recursive freeing of Filer Encoding nodes.                      */
/************************************************************************/
void FLTFreeFilterEncodingNode(FilterEncodingNode *psFilterNode)
{
    if (psFilterNode)
    {
        if (psFilterNode->psLeftNode)
        {
            FLTFreeFilterEncodingNode(psFilterNode->psLeftNode);
            psFilterNode->psLeftNode = NULL;
        }
        if (psFilterNode->psRightNode)
        {
            FLTFreeFilterEncodingNode(psFilterNode->psRightNode);
            psFilterNode->psRightNode = NULL;
        }

        if( psFilterNode->pszValue )
            free( psFilterNode->pszValue );

        if (psFilterNode->pszSRS)
          free( psFilterNode->pszSRS);

        if( psFilterNode->pOther )
        {
#ifdef notdef
            /* TODO free pOther special fields */
            if( psFilterNode->pszWildCard )
                free( psFilterNode->pszWildCard );
            if( psFilterNode->pszSingleChar )
                free( psFilterNode->pszSingleChar );
            if( psFilterNode->pszEscapeChar )
                free( psFilterNode->pszEscapeChar );
#endif
            free( psFilterNode->pOther );
        }
        free(psFilterNode);
    }
}


/************************************************************************/
/*                         FLTCreateFilterEncodingNode                  */
/*                                                                      */
/*      return a FilerEncoding node.                                    */
/************************************************************************/
FilterEncodingNode *FLTCreateFilterEncodingNode(void)
{
    FilterEncodingNode *psFilterNode = NULL;

    psFilterNode = 
      (FilterEncodingNode *)malloc(sizeof (FilterEncodingNode));
    psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
    psFilterNode->pszValue = NULL;
    psFilterNode->pOther = NULL;
    psFilterNode->pszSRS = NULL;
    psFilterNode->psLeftNode = NULL;
    psFilterNode->psRightNode = NULL;

    return psFilterNode;
}

FilterEncodingNode *FLTCreateBinaryCompFilterEncodingNode(void)
{
    FilterEncodingNode *psFilterNode = NULL;
    
    psFilterNode = FLTCreateFilterEncodingNode();
    /* used to store case sensitivity flag. Default is 0 meaning the 
       comparing is case snetitive */
    psFilterNode->pOther = (int *)malloc(sizeof(int));
    (*(int *)(psFilterNode->pOther)) = 0;

    return psFilterNode;
}

/************************************************************************/
/*                           FLTInsertElementInNode                     */
/*                                                                      */
/*      Utility function to parse an XML node and transfter the         */
/*      contennts into the Filer Encoding node structure.               */
/************************************************************************/
void FLTInsertElementInNode(FilterEncodingNode *psFilterNode,
                            CPLXMLNode *psXMLNode)
{
    int nStrLength = 0;
    char *pszTmp = NULL;
    FilterEncodingNode *psCurFilNode= NULL;
    CPLXMLNode *psCurXMLNode = NULL;
    CPLXMLNode *psTmpNode = NULL;
    CPLXMLNode *psFeatureIdNode = NULL;
    char *pszFeatureId=NULL, *pszFeatureIdList=NULL;
    char **tokens = NULL;
    int nTokens = 0;

    if (psFilterNode && psXMLNode && psXMLNode->pszValue)
    {
        psFilterNode->pszValue = strdup(psXMLNode->pszValue);
        psFilterNode->psLeftNode = NULL;
        psFilterNode->psRightNode = NULL;
        
/* -------------------------------------------------------------------- */
/*      Logical filter. AND, OR and NOT are supported. Example of       */
/*      filer using logical filters :                                   */
/*      <Filter>                                                        */
/*        <And>                                                         */
/*          <PropertyIsGreaterThan>                                     */
/*            <PropertyName>Person/Age</PropertyName>                   */
/*            <Literal>50</Literal>                                     */
/*          </PropertyIsGreaterThan>                                    */
/*          <PropertyIsEqualTo>                                         */
/*             <PropertyName>Person/Address/City</PropertyName>         */
/*             <Literal>Toronto</Literal>                               */
/*          </PropertyIsEqualTo>                                        */
/*        </And>                                                        */
/*      </Filter>                                                       */
/* -------------------------------------------------------------------- */
        if (FLTIsLogicalFilterType(psXMLNode->pszValue))
        {
            psFilterNode->eType = FILTER_NODE_TYPE_LOGICAL;
            if (strcasecmp(psFilterNode->pszValue, "AND") == 0 ||
                strcasecmp(psFilterNode->pszValue, "OR") == 0)
            {
                if (psXMLNode->psChild && psXMLNode->psChild->psNext)
                {
                    /*2 operators */
                    if (psXMLNode->psChild->psNext->psNext == NULL)
                    {
                        psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
                        FLTInsertElementInNode(psFilterNode->psLeftNode, 
                                               psXMLNode->psChild);
                        psFilterNode->psRightNode = FLTCreateFilterEncodingNode();
                        FLTInsertElementInNode(psFilterNode->psRightNode, 
                                               psXMLNode->psChild->psNext);
                    }
                    else
                    {
                        psCurXMLNode =  psXMLNode->psChild;
                        psCurFilNode = psFilterNode;
                        while(psCurXMLNode)
                        {
                            if (psCurXMLNode->psNext && psCurXMLNode->psNext->psNext)
                            {
                                psCurFilNode->psLeftNode = 
                                  FLTCreateFilterEncodingNode();
                                FLTInsertElementInNode(psCurFilNode->psLeftNode,
                                                       psCurXMLNode);
                                psCurFilNode->psRightNode = 
                                  FLTCreateFilterEncodingNode();
                                psCurFilNode->psRightNode->eType = 
                                  FILTER_NODE_TYPE_LOGICAL;
                                psCurFilNode->psRightNode->pszValue = 
                                  strdup(psFilterNode->pszValue);
                                
                                psCurFilNode = psCurFilNode->psRightNode;
                                psCurXMLNode = psCurXMLNode->psNext;
                            }
                            else if (psCurXMLNode->psNext)/*last 2 operators*/
                            {
                                psCurFilNode->psLeftNode = 
                                  FLTCreateFilterEncodingNode();
                                FLTInsertElementInNode(psCurFilNode->psLeftNode,
                                                       psCurXMLNode);

                                psCurFilNode->psRightNode = 
                                  FLTCreateFilterEncodingNode();
                                FLTInsertElementInNode(psCurFilNode->psRightNode,
                                                       psCurXMLNode->psNext);
                                psCurXMLNode = psCurXMLNode->psNext->psNext;
                                
                            }
                        }
                    }
                }
            }
            else if (strcasecmp(psFilterNode->pszValue, "NOT") == 0)
            {
                if (psXMLNode->psChild)
                {
                    psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
                    FLTInsertElementInNode(psFilterNode->psLeftNode, 
                                        psXMLNode->psChild); 
                }
            } 
            else
              psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
        }/* end if is logical */
/* -------------------------------------------------------------------- */
/*      Spatial Filter.                                                 */
/*      BBOX :                                                          */
/*      <Filter>                                                        */
/*       <BBOX>                                                         */
/*        <PropertyName>Geometry</PropertyName>                         */
/*        <gml:Box srsName="http://www.opengis.net/gml/srs/epsg.xml#4326">*/
/*          <gml:coordinates>13.0983,31.5899 35.5472,42.8143</gml:coordinates>*/
/*        </gml:Box>                                                    */
/*       </BBOX>                                                        */
/*      </Filter>                                                       */
/*                                                                      */
/*       DWithin                                                        */
/*                                                                      */
/*      <xsd:element name="DWithin"                                     */
/*      type="ogc:DistanceBufferType"                                   */
/*      substitutionGroup="ogc:spatialOps"/>                            */
/*                                                                      */
/*      <xsd:complexType name="DistanceBufferType">                     */
/*         <xsd:complexContent>                                         */
/*            <xsd:extension base="ogc:SpatialOpsType">                 */
/*               <xsd:sequence>                                         */
/*                  <xsd:element ref="ogc:PropertyName"/>               */
/*                  <xsd:element ref="gml:_Geometry"/>                  */
/*                  <xsd:element name="Distance" type="ogc:DistanceType"/>*/
/*               </xsd:sequence>                                        */
/*            </xsd:extension>                                          */
/*         </xsd:complexContent>                                        */
/*      </xsd:complexType>                                              */
/*                                                                      */
/*                                                                      */
/*       <Filter>                                                       */
/*       <DWithin>                                                      */
/*        <PropertyName>Geometry</PropertyName>                         */
/*        <gml:Point>                                                   */
/*          <gml:coordinates>13.0983,31.5899</gml:coordinates>          */
/*        </gml:Point>                                                  */
/*        <Distance units="url#m">10</Distance>                         */
/*       </DWithin>                                                     */
/*      </Filter>                                                       */
/*                                                                      */
/*       Intersect                                                      */
/*                                                                      */
/*       type="ogc:BinarySpatialOpType" substitutionGroup="ogc:spatialOps"/>*/
/*      <xsd:element name="Intersects"                                  */
/*      type="ogc:BinarySpatialOpType"                                  */
/*      substitutionGroup="ogc:spatialOps"/>                            */
/*                                                                      */
/*      <xsd:complexType name="BinarySpatialOpType">                    */
/*      <xsd:complexContent>                                            */
/*      <xsd:extension base="ogc:SpatialOpsType">                       */
/*      <xsd:sequence>                                                  */
/*      <xsd:element ref="ogc:PropertyName"/>                           */
/*      <xsd:choice>                                                    */
/*      <xsd:element ref="gml:_Geometry"/>                              */
/*      <xsd:element ref="gml:Box"/>                                    */
/*      </xsd:sequence>                                                 */
/*      </xsd:extension>                                                */
/*      </xsd:complexContent>                                           */
/*      </xsd:complexType>                                              */
/* -------------------------------------------------------------------- */
        else if (FLTIsSpatialFilterType(psXMLNode->pszValue))
        {
	    psFilterNode->eType = FILTER_NODE_TYPE_SPATIAL;

            if (strcasecmp(psXMLNode->pszValue, "BBOX") == 0)
            {
                char *pszSRS = NULL;
                CPLXMLNode *psPropertyName = NULL;
                CPLXMLNode *psBox = NULL, *psEnvelope=NULL;
                rectObj sBox;
                
                int bCoordinatesValid = 0;

                psPropertyName = CPLGetXMLNode(psXMLNode, "PropertyName");
                psBox = CPLGetXMLNode(psXMLNode, "Box");
                if (!psBox)
                  psBox = CPLGetXMLNode(psXMLNode, "BoxType");

                /*FE 1.0 used box FE1.1 uses envelop*/
                if (psBox)
                  bCoordinatesValid = FLTParseGMLBox(psBox, &sBox, &pszSRS);
                else if ((psEnvelope = CPLGetXMLNode(psXMLNode, "Envelope")))
                  bCoordinatesValid = FLTParseGMLEnvelope(psEnvelope, &sBox, &pszSRS);

                if (psPropertyName == NULL || !bCoordinatesValid)
                  psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;

                if (psPropertyName && bCoordinatesValid)
                {
                    psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
                    /* not really using the property name anywhere ??  */
                    /* Is is always Geometry ?  */
                    if (psPropertyName->psChild && 
                        psPropertyName->psChild->pszValue)
                    {
                        psFilterNode->psLeftNode->eType = 
                          FILTER_NODE_TYPE_PROPERTYNAME;
                        psFilterNode->psLeftNode->pszValue = 
                          strdup(psPropertyName->psChild->pszValue);
                    }
                
                    /* srs and coordinates */
                    psFilterNode->psRightNode = FLTCreateFilterEncodingNode();
                    psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_BBOX;
                    /* srs might be empty */
                    if (pszSRS)
                      psFilterNode->psRightNode->pszValue = pszSRS;

                    psFilterNode->psRightNode->pOther =     
                      (rectObj *)malloc(sizeof(rectObj));
                    ((rectObj *)psFilterNode->psRightNode->pOther)->minx = sBox.minx; 
                    ((rectObj *)psFilterNode->psRightNode->pOther)->miny = sBox.miny; 
                    ((rectObj *)psFilterNode->psRightNode->pOther)->maxx = sBox.maxx; 
                    ((rectObj *)psFilterNode->psRightNode->pOther)->maxy =  sBox.maxy; 
                }
            }
            else if (strcasecmp(psXMLNode->pszValue, "DWithin") == 0 ||
                     strcasecmp(psXMLNode->pszValue, "Beyond") == 0)
 
            {
                shapeObj *psShape = NULL;
                int bPoint = 0, bLine = 0, bPolygon = 0;
                char *pszUnits = NULL;
                char *pszSRS = NULL;
                
                
                CPLXMLNode *psGMLElement = NULL, *psDistance=NULL;


                psGMLElement = CPLGetXMLNode(psXMLNode, "Point");
                if (!psGMLElement)
                  psGMLElement =  CPLGetXMLNode(psXMLNode, "PointType");
                if (psGMLElement)
                  bPoint =1;
                else
                {
                    psGMLElement= CPLGetXMLNode(psXMLNode, "Polygon");
                    if (psGMLElement)
                      bPolygon = 1;
                    else if ((psGMLElement= CPLGetXMLNode(psXMLNode, "MultiPolygon")))
                    {
                          bPolygon = 1;
                    }
                    else
                    {
                        psGMLElement= CPLGetXMLNode(psXMLNode, "LineString");
                        if (psGMLElement)
                          bLine = 1;
                    }		
                }

                psDistance = CPLGetXMLNode(psXMLNode, "Distance");
                if (psGMLElement && psDistance && psDistance->psChild &&  
                    psDistance->psChild->psNext && psDistance->psChild->psNext->pszValue)
                {
                    pszUnits = (char *)CPLGetXMLValue(psDistance, "units", NULL);
                    psShape = (shapeObj *)malloc(sizeof(shapeObj));
                    msInitShape(psShape);
                    if (FLTShapeFromGMLTree(psGMLElement, psShape, &pszSRS))
                      /* if (FLTGML2Shape_XMLNode(psPoint, psShape)) */
                    {
                        /*set the srs if available*/
                        if (pszSRS)
                          psFilterNode->pszSRS = pszSRS;

                        psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
                        /* not really using the property name anywhere ?? Is is always */
                        /* Geometry ?  */
                    
                        psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
                        psFilterNode->psLeftNode->pszValue = strdup("Geometry");

                        psFilterNode->psRightNode = FLTCreateFilterEncodingNode();
                        if (bPoint)
                          psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_GEOMETRY_POINT;
                        else if (bLine)
                          psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_GEOMETRY_LINE;
                        else if (bPolygon)
                          psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_GEOMETRY_POLYGON;
                        psFilterNode->psRightNode->pOther = (shapeObj *)psShape;
                        /*the value will be distance;units*/
                        psFilterNode->psRightNode->pszValue = 
                          strdup(psDistance->psChild->psNext->pszValue);
                        if (pszUnits)
                        {
                            psFilterNode->psRightNode->pszValue= msStringConcatenate(psFilterNode->psRightNode->pszValue, ";");
                            psFilterNode->psRightNode->pszValue= msStringConcatenate(psFilterNode->psRightNode->pszValue, pszUnits);
                        }
                    }
                }
                else
                  psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
            }
            else if (strcasecmp(psXMLNode->pszValue, "Intersect") == 0 ||
                     strcasecmp(psXMLNode->pszValue, "Intersects") == 0 ||
                     strcasecmp(psXMLNode->pszValue, "Equals") == 0 ||
                     strcasecmp(psXMLNode->pszValue, "Disjoint") == 0 ||
                     strcasecmp(psXMLNode->pszValue, "Touches") == 0 ||
                     strcasecmp(psXMLNode->pszValue, "Crosses") == 0 ||
                     strcasecmp(psXMLNode->pszValue, "Within") == 0 ||
                     strcasecmp(psXMLNode->pszValue, "Contains") == 0 ||
                     strcasecmp(psXMLNode->pszValue, "Overlaps") == 0)
            {
                shapeObj *psShape = NULL;
                int  bLine = 0, bPolygon = 0, bPoint=0;
                char *pszSRS = NULL;

                
                CPLXMLNode *psGMLElement = NULL;


                psGMLElement = CPLGetXMLNode(psXMLNode, "Polygon");
                if (psGMLElement)
                  bPolygon = 1;
                else if ((psGMLElement= CPLGetXMLNode(psXMLNode, "MultiPolygon")))
                {
                      bPolygon = 1;
                }
                else if ((psGMLElement= CPLGetXMLNode(psXMLNode, "LineString")))
                {
                    if (psGMLElement)
                      bLine = 1;
                }
		else
                {
                     psGMLElement = CPLGetXMLNode(psXMLNode, "Point");
                     if (!psGMLElement)
                       psGMLElement =  CPLGetXMLNode(psXMLNode, "PointType");
                     if (psGMLElement)
                       bPoint =1;
                }

                if (psGMLElement)
                {
                    psShape = (shapeObj *)malloc(sizeof(shapeObj));
                    msInitShape(psShape);
                    if (FLTShapeFromGMLTree(psGMLElement, psShape, &pszSRS))
                      /* if (FLTGML2Shape_XMLNode(psPoint, psShape)) */
                    {
                        /*set the srs if available*/
                        if (pszSRS)
                          psFilterNode->pszSRS = pszSRS;

                        psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
                        /* not really using the property name anywhere ?? Is is always */
                        /* Geometry ?  */
                    
                        psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
                        psFilterNode->psLeftNode->pszValue = strdup("Geometry");

                        psFilterNode->psRightNode = FLTCreateFilterEncodingNode();
                        if (bPoint)
                          psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_GEOMETRY_POINT;
                        if (bLine)
                          psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_GEOMETRY_LINE;
                        else if (bPolygon)
                          psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_GEOMETRY_POLYGON;
                        psFilterNode->psRightNode->pOther = (shapeObj *)psShape;

                    }
                }
                else
                  psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
            }
            
                
        }/* end of is spatial */


/* -------------------------------------------------------------------- */
/*      Comparison Filter                                               */
/* -------------------------------------------------------------------- */
        else if (FLTIsComparisonFilterType(psXMLNode->pszValue))
        {
            psFilterNode->eType = FILTER_NODE_TYPE_COMPARISON;
/* -------------------------------------------------------------------- */
/*      binary comaparison types. Example :                             */
/*                                                                      */
/*      <Filter>                                                        */
/*        <PropertyIsEqualTo>                                           */
/*          <PropertyName>SomeProperty</PropertyName>                   */
/*          <Literal>100</Literal>                                      */
/*        </PropertyIsEqualTo>                                          */
/*      </Filter>                                                       */
/* -------------------------------------------------------------------- */
            if (FLTIsBinaryComparisonFilterType(psXMLNode->pszValue))
            {
                psTmpNode = CPLSearchXMLNode(psXMLNode,  "PropertyName");
                if (psTmpNode &&  psTmpNode->psChild && 
                    psTmpNode->psChild->pszValue && 
                    strlen(psTmpNode->psChild->pszValue) > 0)
                {
                    psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
                    psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
                    psFilterNode->psLeftNode->pszValue = 
                      strdup(psTmpNode->psChild->pszValue);
                    
                    psTmpNode = CPLSearchXMLNode(psXMLNode,  "Literal");
                    if (psTmpNode)
                    {
                        psFilterNode->psRightNode = FLTCreateBinaryCompFilterEncodingNode();
                        psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_LITERAL;
                    
                        if (psTmpNode && 
                            psTmpNode->psChild && 
                            psTmpNode->psChild->pszValue &&
                            strlen(psTmpNode->psChild->pszValue) > 0)
                        {
                        
                            psFilterNode->psRightNode->pszValue = 
                              strdup(psTmpNode->psChild->pszValue);

                            /*check if the matchCase attribute is set*/
                            if (psXMLNode->psChild && 
                                psXMLNode->psChild->eType == CXT_Attribute  &&
                                psXMLNode->psChild->pszValue && 
                                strcasecmp(psXMLNode->psChild->pszValue, "matchCase") == 0 &&
                                psXMLNode->psChild->psChild &&  
                                psXMLNode->psChild->psChild->pszValue &&
                                strcasecmp( psXMLNode->psChild->psChild->pszValue, "false") == 0)
                            {
                                (*(int *)psFilterNode->psRightNode->pOther) = 1;
                            }
                        
                        }
                        /* special case where the user puts an empty value */
                        /* for the Literal so it can end up as an empty  */
                        /* string query in the expression */
                        else
                          psFilterNode->psRightNode->pszValue = NULL;
                    }
                }
                if (psFilterNode->psLeftNode == NULL || psFilterNode->psRightNode == NULL)
                  psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
            }
 
/* -------------------------------------------------------------------- */
/*      PropertyIsBetween filter : extract property name and boudary    */
/*      values. The boundary  values are stored in the right            */
/*      node. The values are separated by a semi-column (;)             */
/*      Eg of Filter :                                                  */
/*      <PropertyIsBetween>                                             */
/*         <PropertyName>DEPTH</PropertyName>                           */
/*         <LowerBoundary><Literal>400</Literal></LowerBoundary>        */
/*         <UpperBoundary><Literal>800</Literal></UpperBoundary>        */
/*      </PropertyIsBetween>                                            */
/*                                                                      */
/*      Or                                                              */
/*      <PropertyIsBetween>                                             */
/*         <PropertyName>DEPTH</PropertyName>                           */
/*         <LowerBoundary>400</LowerBoundary>                           */
/*         <UpperBoundary>800</UpperBoundary>                           */
/*      </PropertyIsBetween>                                            */
/* -------------------------------------------------------------------- */
            else if (strcasecmp(psXMLNode->pszValue, "PropertyIsBetween") == 0)
            {
                CPLXMLNode *psUpperNode = NULL, *psLowerNode = NULL;
                if (psXMLNode->psChild &&
                    strcasecmp(psXMLNode->psChild->pszValue, 
                               "PropertyName") == 0 &&
                    psXMLNode->psChild->psNext &&  
                      strcasecmp(psXMLNode->psChild->psNext->pszValue, 
                               "LowerBoundary") == 0 &&
                    psXMLNode->psChild->psNext->psNext &&
                    strcasecmp(psXMLNode->psChild->psNext->psNext->pszValue, 
                                "UpperBoundary") == 0)
                {
                    psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();
                    
                    if (psXMLNode->psChild->psChild && 
                        psXMLNode->psChild->psChild->pszValue)
                    {
                        psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
                        psFilterNode->psLeftNode->pszValue = 
                          strdup(psXMLNode->psChild->psChild->pszValue);
                    }

                    psFilterNode->psRightNode = FLTCreateFilterEncodingNode();
                    if (psXMLNode->psChild->psNext->psChild &&
                        psXMLNode->psChild->psNext->psNext->psChild &&
                        psXMLNode->psChild->psNext->psChild->pszValue &&
                        psXMLNode->psChild->psNext->psNext->psChild->pszValue)
                    {
                        /* check if the <Literals> is there */
                        psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_BOUNDARY;
                        if (psXMLNode->psChild->psNext->psChild->psChild)
                          psLowerNode = psXMLNode->psChild->psNext->psChild->psChild;
                        else
                          psLowerNode = psXMLNode->psChild->psNext->psChild;

                        if (psXMLNode->psChild->psNext->psNext->psChild->psChild)
                          psUpperNode = psXMLNode->psChild->psNext->psNext->psChild->psChild;
                        else
                          psUpperNode = psXMLNode->psChild->psNext->psNext->psChild;

                        nStrLength = 
                          strlen(psLowerNode->pszValue);
                        nStrLength +=
                          strlen(psUpperNode->pszValue);

                        nStrLength += 2; /* adding a ; between bounary values */
                         psFilterNode->psRightNode->pszValue = 
                           (char *)malloc(sizeof(char)*(nStrLength));
                         strcpy( psFilterNode->psRightNode->pszValue,
                                psLowerNode->pszValue);
                         strcat(psFilterNode->psRightNode->pszValue, ";");
                         strcat(psFilterNode->psRightNode->pszValue,
                                psUpperNode->pszValue); 
                         psFilterNode->psRightNode->pszValue[nStrLength-1]='\0';
                    }
                         
                  
                }
                else
                  psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;

            }/* end of PropertyIsBetween  */
/* -------------------------------------------------------------------- */
/*      PropertyIsLike                                                  */
/*                                                                      */
/*      <Filter>                                                        */
/*      <PropertyIsLike wildCard="*" singleChar="#" escape="!">         */
/*      <PropertyName>LAST_NAME</PropertyName>                          */
/*      <Literal>JOHN*</Literal>                                        */
/*      </PropertyIsLike>                                               */
/*      </Filter>                                                       */
/* -------------------------------------------------------------------- */
            else if (strcasecmp(psXMLNode->pszValue, "PropertyIsLike") == 0)
            {
                if (CPLSearchXMLNode(psXMLNode,  "Literal") &&
                    CPLSearchXMLNode(psXMLNode,  "PropertyName") &&
                    CPLGetXMLValue(psXMLNode, "wildCard", "") &&
                    CPLGetXMLValue(psXMLNode, "singleChar", "") &&
                    (CPLGetXMLValue(psXMLNode, "escape", "") || 
                     CPLGetXMLValue(psXMLNode, "escapeChar", "")))
                  /*
                    psXMLNode->psChild &&  
                    strcasecmp(psXMLNode->psChild->pszValue, "wildCard") == 0 &&
                    psXMLNode->psChild->psNext &&
                    strcasecmp(psXMLNode->psChild->psNext->pszValue, "singleChar") == 0 &&
                    psXMLNode->psChild->psNext->psNext &&
                    strcasecmp(psXMLNode->psChild->psNext->psNext->pszValue, "escape") == 0 &&
                    psXMLNode->psChild->psNext->psNext->psNext &&
                    strcasecmp(psXMLNode->psChild->psNext->psNext->psNext->pszValue, "PropertyName") == 0 &&
                    psXMLNode->psChild->psNext->psNext->psNext->psNext &&
                    strcasecmp(psXMLNode->psChild->psNext->psNext->psNext->psNext->pszValue, "Literal") == 0)
                  */
                {
/* -------------------------------------------------------------------- */
/*      Get the wildCard, the singleChar and the escapeChar used.       */
/* -------------------------------------------------------------------- */
                    psFilterNode->pOther = (FEPropertyIsLike *)malloc(sizeof(FEPropertyIsLike));
                    /*default is case sensitive*/
                    ((FEPropertyIsLike *)psFilterNode->pOther)->bCaseInsensitive = 0;

                    pszTmp = (char *)CPLGetXMLValue(psXMLNode, "wildCard", "");
                    if (pszTmp)
                      ((FEPropertyIsLike *)psFilterNode->pOther)->pszWildCard = 
                        strdup(pszTmp);
                    pszTmp = (char *)CPLGetXMLValue(psXMLNode, "singleChar", "");
                    if (pszTmp)
                      ((FEPropertyIsLike *)psFilterNode->pOther)->pszSingleChar = 
                        strdup(pszTmp);
                    pszTmp = (char *)CPLGetXMLValue(psXMLNode, "escape", "");
                    if (pszTmp && strlen(pszTmp)>0)
                      ((FEPropertyIsLike *)psFilterNode->pOther)->pszEscapeChar = 
                        strdup(pszTmp);
                    else
                    {
                        pszTmp = (char *)CPLGetXMLValue(psXMLNode, "escapeChar", "");
                        if (pszTmp)
                          ((FEPropertyIsLike *)psFilterNode->pOther)->pszEscapeChar = 
                            strdup(pszTmp);
                    }
                    pszTmp = (char *)CPLGetXMLValue(psXMLNode, "matchCase", "");
                    if (pszTmp && strlen(pszTmp) > 0 && 
                        strcasecmp(pszTmp, "false") == 0)
                    {
                      ((FEPropertyIsLike *)psFilterNode->pOther)->bCaseInsensitive =1;
                    }
/* -------------------------------------------------------------------- */
/*      Create left and right node for the attribute and the value.     */
/* -------------------------------------------------------------------- */
                    psFilterNode->psLeftNode = FLTCreateFilterEncodingNode();

                    psTmpNode = CPLSearchXMLNode(psXMLNode,  "PropertyName");
                    if (psTmpNode &&  psXMLNode->psChild && 
                        psTmpNode->psChild->pszValue && 
                        strlen(psTmpNode->psChild->pszValue) > 0)
                    
                    {
                        /*
                    if (psXMLNode->psChild->psNext->psNext->psNext->psChild &&
                        psXMLNode->psChild->psNext->psNext->psNext->psChild->pszValue)
                    {
                        psFilterNode->psLeftNode->pszValue = 
                          strdup(psXMLNode->psChild->psNext->psNext->psNext->psChild->pszValue);
                        */
                        psFilterNode->psLeftNode->pszValue = 
                          strdup(psTmpNode->psChild->pszValue);
                        psFilterNode->psLeftNode->eType = FILTER_NODE_TYPE_PROPERTYNAME;
                        
                    }

                    psFilterNode->psRightNode = FLTCreateFilterEncodingNode();

                    
                    psTmpNode = CPLSearchXMLNode(psXMLNode,  "Literal");
                    if (psTmpNode &&  psXMLNode->psChild && 
                        psTmpNode->psChild->pszValue && 
                        strlen(psTmpNode->psChild->pszValue) > 0)
                    {
                        /*
                    if (psXMLNode->psChild->psNext->psNext->psNext->psNext->psChild &&
                        psXMLNode->psChild->psNext->psNext->psNext->psNext->psChild->pszValue)
                    {
                        
                        psFilterNode->psRightNode->pszValue = 
                          strdup(psXMLNode->psChild->psNext->psNext->psNext->psNext->psChild->pszValue);        
                        */
                        psFilterNode->psRightNode->pszValue = 
                          strdup(psTmpNode->psChild->pszValue);

                        psFilterNode->psRightNode->eType = FILTER_NODE_TYPE_LITERAL;
                    }
                }
                else
                  psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
                  
            }
        }
/* -------------------------------------------------------------------- */
/*      FeatureId Filter                                                */
/*                                                                      */
/*      <ogc:Filter>                                                    */
/*      <ogc:FeatureId fid="INWATERA_1M.1013"/>                         */
/*      <ogc:FeatureId fid="INWATERA_1M.10"/>                           */
/*      <ogc:FeatureId fid="INWATERA_1M.13"/>                           */
/*      <ogc:FeatureId fid="INWATERA_1M.140"/>                          */
/*      <ogc:FeatureId fid="INWATERA_1M.5001"/>                         */
/*      <ogc:FeatureId fid="INWATERA_1M.2001"/>                         */
/*      </ogc:Filter>                                                   */
/*                                                                      */
/*                                                                      */
/*      Note that for FES1.1.0 the featureid has been depricated in     */
/*      favor of GmlObjectId                                            */
/*      <GmlObjectId gml:id="TREESA_1M.1234"/>                          */
/* -------------------------------------------------------------------- */
        else if (FLTIsFeatureIdFilterType(psXMLNode->pszValue))
        {
            psFilterNode->eType = FILTER_NODE_TYPE_FEATUREID;
            pszFeatureId = (char *)CPLGetXMLValue(psXMLNode, "fid", NULL);
            /*for FE 1.1.0 GmlObjectId */
            if (pszFeatureId == NULL)
              pszFeatureId = (char *)CPLGetXMLValue(psXMLNode, "id", NULL);
            pszFeatureIdList = NULL;
            
            psFeatureIdNode = psXMLNode;
            while (psFeatureIdNode)
            {
                pszFeatureId = (char *)CPLGetXMLValue(psFeatureIdNode, "fid", NULL);
                if (!pszFeatureId)
                   pszFeatureId = (char *)CPLGetXMLValue(psFeatureIdNode, "id", NULL);

                if (pszFeatureId)
                {
                    if (pszFeatureIdList)
                      pszFeatureIdList = msStringConcatenate(pszFeatureIdList, ",");

                    /*typname could be part of the value : INWATERA_1M.1234*/
                    tokens = msStringSplit(pszFeatureId,'.', &nTokens);
                    if (tokens && nTokens == 2)
                      pszFeatureIdList = msStringConcatenate(pszFeatureIdList, tokens[1]);
                    else
                      pszFeatureIdList = msStringConcatenate(pszFeatureIdList, pszFeatureId);

                    if (tokens)
                      msFreeCharArray(tokens, nTokens);
                }
                psFeatureIdNode = psFeatureIdNode->psNext;
            }

            if (pszFeatureIdList)
            {
                psFilterNode->pszValue =  strdup(pszFeatureIdList);
                msFree(pszFeatureIdList);
            }
            else
               psFilterNode->eType = FILTER_NODE_TYPE_UNDEFINED;
        }
            
    }
}

 
/************************************************************************/
/*            int FLTIsLogicalFilterType((char *pszValue)                  */
/*                                                                      */
/*      return TRUE if the value of the node is of logical filter       */
/*       encoding type.                                                 */
/************************************************************************/
int FLTIsLogicalFilterType(char *pszValue)
{
    if (pszValue)
    {
        if (strcasecmp(pszValue, "AND") == 0 ||
            strcasecmp(pszValue, "OR") == 0 ||
             strcasecmp(pszValue, "NOT") == 0)
           return MS_TRUE;
    }

    return MS_FALSE;
}

/************************************************************************/
/*         int FLTIsBinaryComparisonFilterType(char *pszValue)             */
/*                                                                      */
/*      Binary comparison filter type.                                  */
/************************************************************************/
int FLTIsBinaryComparisonFilterType(char *pszValue)
{
    if (pszValue)
    {
         if (strcasecmp(pszValue, "PropertyIsEqualTo") == 0 ||  
             strcasecmp(pszValue, "PropertyIsNotEqualTo") == 0 ||  
             strcasecmp(pszValue, "PropertyIsLessThan") == 0 ||  
             strcasecmp(pszValue, "PropertyIsGreaterThan") == 0 ||  
             strcasecmp(pszValue, "PropertyIsLessThanOrEqualTo") == 0 ||  
             strcasecmp(pszValue, "PropertyIsGreaterThanOrEqualTo") == 0)
           return MS_TRUE;
    }

    return MS_FALSE;
}

/************************************************************************/
/*            int FLTIsComparisonFilterType(char *pszValue)                */
/*                                                                      */
/*      return TRUE if the value of the node is of comparison filter    */
/*      encoding type.                                                  */
/************************************************************************/
int FLTIsComparisonFilterType(char *pszValue)
{
    if (pszValue)
    {
         if (FLTIsBinaryComparisonFilterType(pszValue) ||  
             strcasecmp(pszValue, "PropertyIsLike") == 0 ||  
             strcasecmp(pszValue, "PropertyIsBetween") == 0)
           return MS_TRUE;
    }

    return MS_FALSE;
}

/************************************************************************/
/*            int FLTIsFeatureIdFilterType(char *pszValue)              */
/*                                                                      */
/*      return TRUE if the value of the node is of featureid filter     */
/*      encoding type.                                                  */
/************************************************************************/
int FLTIsFeatureIdFilterType(char *pszValue)
{
    if (pszValue && (strcasecmp(pszValue, "FeatureId") == 0 ||
                     strcasecmp(pszValue, "GmlObjectId") == 0))
                     
      return MS_TRUE;

     return MS_FALSE;
}

/************************************************************************/
/*            int FLTIsSpatialFilterType(char *pszValue)                */
/*                                                                      */
/*      return TRUE if the value of the node is of spatial filter       */
/*      encoding type.                                                  */
/************************************************************************/
int FLTIsSpatialFilterType(char *pszValue)
{
    if (pszValue)
    {
        if ( strcasecmp(pszValue, "BBOX") == 0 ||
             strcasecmp(pszValue, "DWithin") == 0 ||
             strcasecmp(pszValue, "Intersect") == 0 ||
             strcasecmp(pszValue, "Intersects") == 0 ||
             strcasecmp(pszValue, "Equals") == 0 ||
             strcasecmp(pszValue, "Disjoint") == 0 ||
             strcasecmp(pszValue, "Touches") == 0 ||
             strcasecmp(pszValue, "Crosses") == 0 ||
             strcasecmp(pszValue, "Within") == 0 ||
             strcasecmp(pszValue, "Contains") == 0 ||
             strcasecmp(pszValue, "Overlaps") == 0 ||
             strcasecmp(pszValue, "Beyond") == 0)
          return MS_TRUE;
    }

    return MS_FALSE;
}

/************************************************************************/
/*           int FLTIsSupportedFilterType(CPLXMLNode *psXMLNode)           */
/*                                                                      */
/*      Verfify if the value of the node is one of the supported        */
/*      filter type.                                                    */
/************************************************************************/
int FLTIsSupportedFilterType(CPLXMLNode *psXMLNode)
{
    if (psXMLNode)
    {
        if (FLTIsLogicalFilterType(psXMLNode->pszValue) ||
            FLTIsSpatialFilterType(psXMLNode->pszValue) ||
            FLTIsComparisonFilterType(psXMLNode->pszValue) ||
            FLTIsFeatureIdFilterType(psXMLNode->pszValue))
          return MS_TRUE;
    }

    return MS_FALSE;
}
 
/************************************************************************/
/*                          FLTNumberOfFilterType                       */
/*                                                                      */
/*      Loop trhough the nodes and return the number of nodes of        */
/*      specified value.                                                */
/************************************************************************/
int FLTNumberOfFilterType(FilterEncodingNode *psFilterNode, const char *szType)
{
    int nCount = 0;
    int nLeftNode=0 , nRightNode = 0;

    if (!psFilterNode || !szType || !psFilterNode->pszValue)
      return 0;

    if (strcasecmp(psFilterNode->pszValue, (char*)szType) == 0)
      nCount++;

    if (psFilterNode->psLeftNode)
      nLeftNode = FLTNumberOfFilterType(psFilterNode->psLeftNode, szType);

    nCount += nLeftNode;

    if (psFilterNode->psRightNode)
      nRightNode = FLTNumberOfFilterType(psFilterNode->psRightNode, szType);
    nCount += nRightNode;
   
    return nCount;
}
    



/************************************************************************/
/*                          FLTValidForBBoxFilter                       */
/*                                                                      */
/*      Validate if there is only one BBOX filter node. Here is waht    */
/*      is supported (is valid) :                                       */
/*        - one node which is a BBOX                                    */
/*        - a logical AND with a valid BBOX                             */
/*                                                                      */
/*      eg 1: <Filter>                                                  */
/*            <BBOX>                                                    */
/*              <PropertyName>Geometry</PropertyName>                   */
/*              <gml:Box srsName="http://www.opengis.net/gml/srs/epsg.xml#4326">*/
/*                <gml:coordinates>13.0983,31.5899 35.5472,42.8143</gml:coordinates>*/
/*              </gml:Box>                                              */
/*            </BBOX>                                                   */
/*          </Filter>                                                   */
/*                                                                      */
/*      eg 2 :<Filter>                                                  */
/*              <AND>                                                   */
/*               <BBOX>                                                 */
/*                <PropertyName>Geometry</PropertyName>                  */
/*                <gml:Box srsName="http://www.opengis.net/gml/srs/epsg.xml#4326">*/
/*                  <gml:coordinates>13.0983,31.5899 35.5472,42.8143</gml:coordinates>*/
/*                </gml:Box>                                            */
/*               </BBOX>                                                */
/*               <PropertyIsEqualTo>                                    */
/*               <PropertyName>SomeProperty</PropertyName>              */
/*                <Literal>100</Literal>                                */
/*              </PropertyIsEqualTo>                                    */
/*             </AND>                                                   */
/*           </Filter>                                                  */
/*                                                                      */
/************************************************************************/
int FLTValidForBBoxFilter(FilterEncodingNode *psFilterNode)
{
    int nCount = 0;
   
    if (!psFilterNode || !psFilterNode->pszValue)
      return 1;

    nCount = FLTNumberOfFilterType(psFilterNode, "BBOX");

    if (nCount > 1)
      return 0;
    else if (nCount == 0)
      return 1;

    /* nCount ==1  */
    if (strcasecmp(psFilterNode->pszValue, "BBOX") == 0)
      return 1;

    if (strcasecmp(psFilterNode->pszValue, "AND") == 0)
    {
      if (strcasecmp(psFilterNode->psLeftNode->pszValue, "BBOX") ==0 ||
          strcasecmp(psFilterNode->psRightNode->pszValue, "BBOX") ==0)
        return 1;
    }

    return 0;
}    

int FLTIsLineFilter(FilterEncodingNode *psFilterNode)
{
    if (!psFilterNode || !psFilterNode->pszValue)
      return 0;

    if (psFilterNode->eType == FILTER_NODE_TYPE_SPATIAL && 
        psFilterNode->psRightNode &&  
        psFilterNode->psRightNode->eType == FILTER_NODE_TYPE_GEOMETRY_LINE)
      return 1;

    return 0;
}
  
int FLTIsPolygonFilter(FilterEncodingNode *psFilterNode)
{
    if (!psFilterNode || !psFilterNode->pszValue)
      return 0;

    if (psFilterNode->eType == FILTER_NODE_TYPE_SPATIAL && 
        psFilterNode->psRightNode &&  
        psFilterNode->psRightNode->eType == FILTER_NODE_TYPE_GEOMETRY_POLYGON)
      return 1;

    return 0;
}
    
int FLTIsPointFilter(FilterEncodingNode *psFilterNode)
{
    if (!psFilterNode || !psFilterNode->pszValue)
      return 0;

    if (psFilterNode->eType == FILTER_NODE_TYPE_SPATIAL && 
        psFilterNode->psRightNode &&  
        psFilterNode->psRightNode->eType == FILTER_NODE_TYPE_GEOMETRY_POINT)
      return 1;

    return 0;
}

int FLTIsBBoxFilter(FilterEncodingNode *psFilterNode)
{
    if (!psFilterNode || !psFilterNode->pszValue)
      return 0;
    
    if (strcasecmp(psFilterNode->pszValue, "BBOX") == 0)
      return 1;

    /*    if (strcasecmp(psFilterNode->pszValue, "AND") == 0)
    {
      if (strcasecmp(psFilterNode->psLeftNode->pszValue, "BBOX") ==0 ||
          strcasecmp(psFilterNode->psRightNode->pszValue, "BBOX") ==0)
        return 1;
    }
    */
    return 0;
}       

shapeObj *FLTGetShape(FilterEncodingNode *psFilterNode, double *pdfDistance,
                      int *pnUnit)
{
    char **tokens = NULL;
    int nTokens = 0;
    FilterEncodingNode *psNode = psFilterNode;
    char *szUnitStr = NULL;
    char *szUnit = NULL;

    if (psNode)
    {
        if (psNode->eType == FILTER_NODE_TYPE_SPATIAL && psNode->psRightNode)
          psNode = psNode->psRightNode;

        if (psNode->eType == FILTER_NODE_TYPE_GEOMETRY_POINT ||
            psNode->eType == FILTER_NODE_TYPE_GEOMETRY_LINE ||
            psNode->eType == FILTER_NODE_TYPE_GEOMETRY_POLYGON)
        {
            
            if (psNode->pszValue && pdfDistance)
            {
                /*
                sytnax expected is "distance;unit" or just "distance"
                if unit is there syntax is "URI#unit" (eg http://..../#m)
                or just "unit"
                */
                tokens = msStringSplit(psNode->pszValue,';', &nTokens);
                if (tokens && nTokens >= 1)
                {
                    *pdfDistance = atof(tokens[0]);
                
                    if (nTokens == 2 && pnUnit)
                    {
                        szUnitStr = strdup(tokens[1]);
                        msFreeCharArray(tokens, nTokens);
                        nTokens = 0;
                        tokens = msStringSplit(szUnitStr,'#', &nTokens);
                        msFree(szUnitStr);
                        if (tokens && nTokens >= 1)
                        {
                            if (nTokens ==1)
                              szUnit = tokens[0];
                            else
                              szUnit = tokens[1];

                            if (strcasecmp(szUnit,"m") == 0 ||
                                strcasecmp(szUnit,"meters") == 0 )
                              *pnUnit = MS_METERS;
                            else if (strcasecmp(szUnit,"km") == 0 || 
                                     strcasecmp(szUnit,"kilometers") == 0)
                              *pnUnit = MS_KILOMETERS;
                            else if (strcasecmp(szUnit,"NM") == 0 || 
                                     strcasecmp(szUnit,"nauticalmiles") == 0)
                              *pnUnit = MS_NAUTICALMILES;
                            else if (strcasecmp(szUnit,"mi") == 0 || 
                                     strcasecmp(szUnit,"miles") == 0)
                              *pnUnit = MS_MILES;
                            else if (strcasecmp(szUnit,"in") == 0 ||
                                    strcasecmp(szUnit,"inches") == 0)
                              *pnUnit = MS_INCHES;
                           else if (strcasecmp(szUnit,"ft") == 0 ||
                                    strcasecmp(szUnit,"feet") == 0)
                             *pnUnit = MS_FEET;
                           else if (strcasecmp(szUnit,"deg") == 0 ||
                                    strcasecmp(szUnit,"dd") == 0)
                              *pnUnit = MS_DD;
                             else if (strcasecmp(szUnit,"px") == 0)
                              *pnUnit = MS_PIXELS;
                            
                             msFreeCharArray(tokens, nTokens);
                        }
                    }
                }       
                           
            }

            return (shapeObj *)psNode->pOther;
        }
    }
    return NULL;   
}

/************************************************************************/
/*                                FLTGetBBOX                            */
/*                                                                      */
/*      Loop through the nodes are return the coordinates of the        */
/*      first bbox node found. The retrun value is the epsg code of     */
/*      the bbox.                                                       */
/************************************************************************/
char *FLTGetBBOX(FilterEncodingNode *psFilterNode, rectObj *psRect)
{
    char *pszReturn = NULL;
    
    if (!psFilterNode || !psRect)
      return NULL;

    if (strcasecmp(psFilterNode->pszValue, "BBOX") == 0)
    {
        if (psFilterNode->psRightNode && psFilterNode->psRightNode->pOther)
        {
            psRect->minx = ((rectObj *)psFilterNode->psRightNode->pOther)->minx;
            psRect->miny = ((rectObj *)psFilterNode->psRightNode->pOther)->miny;
            psRect->maxx = ((rectObj *)psFilterNode->psRightNode->pOther)->maxx;
            psRect->maxy = ((rectObj *)psFilterNode->psRightNode->pOther)->maxy;
            
            return psFilterNode->psRightNode->pszValue;
            
        }
    }
    else
    {
        pszReturn = FLTGetBBOX(psFilterNode->psLeftNode, psRect);
        if (pszReturn)
          return pszReturn;
        else
          return  FLTGetBBOX(psFilterNode->psRightNode, psRect);
    }

    return pszReturn;
}

/************************************************************************/
/*                          GetMapserverExpression                      */
/*                                                                      */
/*      Return a mapserver expression base on the Filer encoding nodes. */
/************************************************************************/
char *FLTGetMapserverExpression(FilterEncodingNode *psFilterNode, layerObj *lp)
{
    char *pszExpression = NULL;
    const char *pszAttribute = NULL;
    char szTmp[256];
    char **tokens = NULL;
    int nTokens = 0, i=0,bString=0;
    char *pszTmp;
    
    if (!psFilterNode)
      return NULL;

    if (psFilterNode->eType == FILTER_NODE_TYPE_COMPARISON)
    {
        if ( psFilterNode->psLeftNode && psFilterNode->psRightNode)
        {
            if (FLTIsBinaryComparisonFilterType(psFilterNode->pszValue))
            {
              pszExpression = FLTGetBinaryComparisonExpresssion(psFilterNode, lp);
            }
            else if (strcasecmp(psFilterNode->pszValue, 
                                "PropertyIsBetween") == 0)
            {
              pszExpression = FLTGetIsBetweenComparisonExpresssion(psFilterNode, lp);
            }
            else if (strcasecmp(psFilterNode->pszValue, 
                                "PropertyIsLike") == 0)
            {
                 pszExpression = FLTGetIsLikeComparisonExpression(psFilterNode);
            }
        }
    }
    else if (psFilterNode->eType == FILTER_NODE_TYPE_LOGICAL)
    {
        if (strcasecmp(psFilterNode->pszValue, "AND") == 0 ||
            strcasecmp(psFilterNode->pszValue, "OR") == 0)
        {
          pszExpression = FLTGetLogicalComparisonExpresssion(psFilterNode, lp);
        }
        else if (strcasecmp(psFilterNode->pszValue, "NOT") == 0)
        {       
          pszExpression = FLTGetLogicalComparisonExpresssion(psFilterNode, lp);
        }
    }
    else if (psFilterNode->eType == FILTER_NODE_TYPE_SPATIAL)
    {
        /* TODO */
    }
    else if (psFilterNode->eType == FILTER_NODE_TYPE_FEATUREID)
    {
#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR) || defined(USE_SOS_SVR)
       if (psFilterNode->pszValue)
       {
            pszAttribute = msOWSLookupMetadata(&(lp->metadata), "OFG", "featureid");
            if (pszAttribute)
            {
                tokens = msStringSplit(psFilterNode->pszValue,',', &nTokens);
                if (tokens && nTokens > 0)
                {
                    for (i=0; i<nTokens; i++)
                    {   
                        if (i == 0)
                        {
                            pszTmp = tokens[0];
                            if(FLTIsNumeric(pszTmp) == MS_FALSE) 
                              bString = 1;
                        }
                        if (bString)
                          sprintf(szTmp, "('[%s]' = '%s')" , pszAttribute, tokens[i]);
                        else
                           sprintf(szTmp, "([%s] = %s)" , pszAttribute, tokens[i]);

                        if (pszExpression != NULL)
                          pszExpression = msStringConcatenate(pszExpression, " OR ");
                        else
                          pszExpression = msStringConcatenate(pszExpression, "(");
                        pszExpression = msStringConcatenate(pszExpression, szTmp);
                    }

                    msFreeCharArray(tokens, nTokens);
                }
            }   
            /*opening and closing brackets are needed for mapserver expressions*/
            if (pszExpression)
               pszExpression = msStringConcatenate(pszExpression, ")");
        }
#else
       msSetError(MS_MISCERR, "OWS support is not available.", 
                   "FLTGetMapserverExpression()");
    return(MS_FAILURE);
#endif

    }
    return pszExpression;
}


/************************************************************************/
/*                           FLTGetSQLExpression                        */
/*                                                                      */
/*      Build SQL expressions from the mapserver nodes.                 */
/************************************************************************/
char *FLTGetSQLExpression(FilterEncodingNode *psFilterNode, layerObj *lp)
{
    char *pszExpression = NULL;
    int connectiontype;
    const char *pszAttribute = NULL;
    char szTmp[256];
    char **tokens = NULL;
    int nTokens = 0, i=0, bString=0;
    char *pszTmp;

    if (psFilterNode == NULL || lp == NULL)
      return NULL;

    connectiontype = lp->connectiontype;
    if (psFilterNode->eType == FILTER_NODE_TYPE_COMPARISON)
    {
        if ( psFilterNode->psLeftNode && psFilterNode->psRightNode)
        {
            if (FLTIsBinaryComparisonFilterType(psFilterNode->pszValue))
            {
                pszExpression = 
                  FLTGetBinaryComparisonSQLExpresssion(psFilterNode, lp);
            }            
            else if (strcasecmp(psFilterNode->pszValue, 
                                "PropertyIsBetween") == 0)
            {
                 pszExpression = 
                   FLTGetIsBetweenComparisonSQLExpresssion(psFilterNode, lp);
            }
            else if (strcasecmp(psFilterNode->pszValue, 
                                "PropertyIsLike") == 0)
            {
                 pszExpression = 
                   FLTGetIsLikeComparisonSQLExpression(psFilterNode,
                                                       connectiontype);
            }
        }
    }
    else if (psFilterNode->eType == FILTER_NODE_TYPE_LOGICAL)
    {
        if (strcasecmp(psFilterNode->pszValue, "AND") == 0 ||
            strcasecmp(psFilterNode->pszValue, "OR") == 0)
        {
            pszExpression =
              FLTGetLogicalComparisonSQLExpresssion(psFilterNode, lp);

        }
        else if (strcasecmp(psFilterNode->pszValue, "NOT") == 0)
        {       
            pszExpression = 
              FLTGetLogicalComparisonSQLExpresssion(psFilterNode, lp);

        }
    }
    
    else if (psFilterNode->eType == FILTER_NODE_TYPE_SPATIAL)
    {
        /* TODO */
    }
    else if (psFilterNode->eType == FILTER_NODE_TYPE_FEATUREID)
    {
#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR) || defined(USE_SOS_SVR)
        if (psFilterNode->pszValue)
        {
            pszAttribute = msOWSLookupMetadata(&(lp->metadata), "OFG", "featureid");
            if (pszAttribute)
            {
                tokens = msStringSplit(psFilterNode->pszValue,',', &nTokens);
                bString = 0;
                if (tokens && nTokens > 0)
                {
                    for (i=0; i<nTokens; i++)
                    {
                        if (i == 0)
                        {
                            pszTmp = tokens[0];
                            if (FLTIsNumeric(pszTmp) == MS_FALSE)    
                               bString = 1;
                        }
                        if (bString)
                          sprintf(szTmp, "(%s = '%s')" , pszAttribute, tokens[i]);
                        else
                           sprintf(szTmp, "(%s = %s)" , pszAttribute, tokens[i]);

                        if (pszExpression != NULL)
                          pszExpression = msStringConcatenate(pszExpression, " OR ");
                        else
                          /*opening and closing brackets*/
                          pszExpression = msStringConcatenate(pszExpression, "(");

                        pszExpression = msStringConcatenate(pszExpression, szTmp);
                    }

                    msFreeCharArray(tokens, nTokens);
                }
            }
            /*opening and closing brackets*/
            if (pszExpression)
               pszExpression = msStringConcatenate(pszExpression, ")");
        }
#else
        msSetError(MS_MISCERR, "OWS support is not available.", 
                   "FLTGetSQLExpression()");
    return(MS_FAILURE);
#endif

    }
    
    return pszExpression;
}
  
/************************************************************************/
/*                            FLTGetNodeExpression                      */
/*                                                                      */
/*      Return the expresion for a specific node.                       */
/************************************************************************/
char *FLTGetNodeExpression(FilterEncodingNode *psFilterNode, layerObj *lp)
{
    char *pszExpression = NULL;
    if (!psFilterNode)
      return NULL;

    if (FLTIsLogicalFilterType(psFilterNode->pszValue))
      pszExpression = FLTGetLogicalComparisonExpresssion(psFilterNode, lp);
    else if (FLTIsComparisonFilterType(psFilterNode->pszValue))
    {
        if (FLTIsBinaryComparisonFilterType(psFilterNode->pszValue))
          pszExpression = FLTGetBinaryComparisonExpresssion(psFilterNode, lp);
        else if (strcasecmp(psFilterNode->pszValue, "PropertyIsBetween") == 0)
          pszExpression = FLTGetIsBetweenComparisonExpresssion(psFilterNode, lp);
        else if (strcasecmp(psFilterNode->pszValue, "PropertyIsLike") == 0)
          pszExpression = FLTGetIsLikeComparisonExpression(psFilterNode);
    }

    return pszExpression;
}


/************************************************************************/
/*                     FLTGetLogicalComparisonSQLExpresssion            */
/*                                                                      */
/*      Return the expression for logical comparison expression.        */
/************************************************************************/
char *FLTGetLogicalComparisonSQLExpresssion(FilterEncodingNode *psFilterNode,
                                            layerObj *lp)
{
    char *pszBuffer = NULL;
    char *pszTmp = NULL;
    int nTmp = 0;
    int connectiontype;

    if (lp == NULL)
      return NULL;

    connectiontype = lp->connectiontype;

/* ==================================================================== */
/*      special case for BBOX node.                                     */
/* ==================================================================== */
    if (psFilterNode->psLeftNode && psFilterNode->psRightNode &&
        ((strcasecmp(psFilterNode->psLeftNode->pszValue, "BBOX") == 0) ||
         (strcasecmp(psFilterNode->psRightNode->pszValue, "BBOX") == 0)))
    {
         if (strcasecmp(psFilterNode->psLeftNode->pszValue, "BBOX") != 0)
           pszTmp = FLTGetSQLExpression(psFilterNode->psLeftNode, lp);
         else
           pszTmp = FLTGetSQLExpression(psFilterNode->psRightNode, lp);
           
         if (!pszTmp)
          return NULL;

         pszBuffer = (char *)malloc(sizeof(char) * (strlen(pszTmp) + 1));
         sprintf(pszBuffer, "%s", pszTmp);
    }

/* -------------------------------------------------------------------- */
/*      OR and AND                                                      */
/* -------------------------------------------------------------------- */
    else if (psFilterNode->psLeftNode && psFilterNode->psRightNode)
    {
        pszTmp = FLTGetSQLExpression(psFilterNode->psLeftNode, lp);
        if (!pszTmp)
          return NULL;

        pszBuffer = (char *)malloc(sizeof(char) * 
                                    (strlen(pszTmp) + 
                                     strlen(psFilterNode->pszValue) + 5));
        pszBuffer[0] = '\0';
        strcat(pszBuffer, " (");
        strcat(pszBuffer, pszTmp);
        strcat(pszBuffer, " ");
        strcat(pszBuffer, psFilterNode->pszValue);
        strcat(pszBuffer, " ");

        free( pszTmp );

        nTmp = strlen(pszBuffer);
        pszTmp = FLTGetSQLExpression(psFilterNode->psRightNode, lp);
        if (!pszTmp)
          return NULL;

        pszBuffer = (char *)realloc(pszBuffer, 
                                    sizeof(char) * (strlen(pszTmp) + nTmp +3));
        strcat(pszBuffer, pszTmp);
        strcat(pszBuffer, ") ");
    }
/* -------------------------------------------------------------------- */
/*      NOT                                                             */
/* -------------------------------------------------------------------- */
    else if (psFilterNode->psLeftNode && 
             strcasecmp(psFilterNode->pszValue, "NOT") == 0)
    {
        pszTmp = FLTGetSQLExpression(psFilterNode->psLeftNode, lp);
        if (!pszTmp)
          return NULL;
        
        pszBuffer = (char *)malloc(sizeof(char) * (strlen(pszTmp) +  9));
        pszBuffer[0] = '\0';

        strcat(pszBuffer, " (NOT ");
        strcat(pszBuffer, pszTmp);
        strcat(pszBuffer, ") ");
    }
    else
      return NULL;

/* -------------------------------------------------------------------- */
/*      Cleanup.                                                        */
/* -------------------------------------------------------------------- */
    if( pszTmp != NULL )
        free( pszTmp );
    return pszBuffer;

}

/************************************************************************/
/*                     FLTGetLogicalComparisonExpresssion               */
/*                                                                      */
/*      Return the expression for logical comparison expression.        */
/************************************************************************/
char *FLTGetLogicalComparisonExpresssion(FilterEncodingNode *psFilterNode, layerObj *lp)
{
    char *pszTmp = NULL;
    char *pszBuffer = NULL;
    int nTmp = 0;

    if (!psFilterNode || !FLTIsLogicalFilterType(psFilterNode->pszValue))
      return NULL;

    
/* ==================================================================== */
/*      special case for BBOX node.                                     */
/* ==================================================================== */
    if (psFilterNode->psLeftNode && psFilterNode->psRightNode &&
        (strcasecmp(psFilterNode->psLeftNode->pszValue, "BBOX") == 0 ||
         strcasecmp(psFilterNode->psRightNode->pszValue, "BBOX") == 0 ||
         FLTIsGeosNode(psFilterNode->psLeftNode->pszValue) ||
         FLTIsGeosNode(psFilterNode->psRightNode->pszValue)))
         
         
    {
        
        /*strcat(szBuffer, " (");*/
        if (strcasecmp(psFilterNode->psLeftNode->pszValue, "BBOX") != 0 &&
            strcasecmp(psFilterNode->psLeftNode->pszValue, "DWithin") != 0 &&
            FLTIsGeosNode(psFilterNode->psLeftNode->pszValue) == MS_FALSE)
          pszTmp = FLTGetNodeExpression(psFilterNode->psLeftNode, lp);
        else
          pszTmp = FLTGetNodeExpression(psFilterNode->psRightNode, lp);

        if (!pszTmp)
          return NULL;
        
        pszBuffer = (char *)malloc(sizeof(char) * (strlen(pszTmp) + 3));
        pszBuffer[0] = '\0';
        /*
        if (strcasecmp(psFilterNode->psLeftNode->pszValue, "PropertyIsLike") == 0 ||
            strcasecmp(psFilterNode->psRightNode->pszValue, "PropertyIsLike") == 0)
          sprintf(pszBuffer, "%s", pszTmp);
        else
        */
        sprintf(pszBuffer, "(%s)", pszTmp);
        
        
        return pszBuffer;
    }

    
/* -------------------------------------------------------------------- */
/*      OR and AND                                                      */
/* -------------------------------------------------------------------- */
    if (psFilterNode->psLeftNode && psFilterNode->psRightNode)
    {
      pszTmp = FLTGetNodeExpression(psFilterNode->psLeftNode, lp);
        if (!pszTmp)
          return NULL;

        pszBuffer = (char *)malloc(sizeof(char) * 
                                   (strlen(pszTmp) + strlen(psFilterNode->pszValue) + 5));
        pszBuffer[0] = '\0';
        strcat(pszBuffer, " (");
        
        strcat(pszBuffer, pszTmp);
        strcat(pszBuffer, " ");
        strcat(pszBuffer, psFilterNode->pszValue);
        strcat(pszBuffer, " ");
        pszTmp = FLTGetNodeExpression(psFilterNode->psRightNode, lp);
        if (!pszTmp)
          return NULL;

        nTmp = strlen(pszBuffer);
        pszBuffer = (char *)realloc(pszBuffer, 
                                    sizeof(char) * (strlen(pszTmp) + nTmp +3));

        strcat(pszBuffer, pszTmp);
        strcat(pszBuffer, ") ");
    }
/* -------------------------------------------------------------------- */
/*      NOT                                                             */
/* -------------------------------------------------------------------- */
    else if (psFilterNode->psLeftNode && 
             strcasecmp(psFilterNode->pszValue, "NOT") == 0)
    {
      pszTmp = FLTGetNodeExpression(psFilterNode->psLeftNode, lp);
        if (!pszTmp)
          return NULL;

         pszBuffer = (char *)malloc(sizeof(char) * 
                                   (strlen(pszTmp) +  9));
         pszBuffer[0] = '\0';
         strcat(pszBuffer, " (NOT ");
         strcat(pszBuffer, pszTmp);
         strcat(pszBuffer, ") ");
    }
    else
      return NULL;
    
    return pszBuffer;
    
}

    
    
/************************************************************************/
/*                      FLTGetBinaryComparisonExpresssion               */
/*                                                                      */
/*      Return the expression for a binary comparison filter node.      */
/************************************************************************/
char *FLTGetBinaryComparisonExpresssion(FilterEncodingNode *psFilterNode, layerObj *lp)
{
    char szBuffer[1024];
    int bString=0;
    char szTmp[256];

    szBuffer[0] = '\0';
    if (!psFilterNode || !FLTIsBinaryComparisonFilterType(psFilterNode->pszValue))
      return NULL;

/* -------------------------------------------------------------------- */
/*      check if the value is a numeric value or alphanumeric. If it    */
/*      is alphanumeric, add quotes around attribute and values.        */
/* -------------------------------------------------------------------- */
    bString = 0;
    if (psFilterNode->psRightNode->pszValue)
    {
        sprintf(szTmp, "%s_type",  psFilterNode->psLeftNode->pszValue);
        if (msOWSLookupMetadata(&(lp->metadata), "OFG", szTmp) != NULL &&
            (strcasecmp(msOWSLookupMetadata(&(lp->metadata), "G", szTmp), "Character") == 0))
          bString = 1;
        else if (FLTIsNumeric(psFilterNode->psRightNode->pszValue) == MS_FALSE)    
          bString = 1;
    }
    
    /* specical case to be able to have empty strings in the expression. */
    if (psFilterNode->psRightNode->pszValue == NULL)
      bString = 1;
      

    if (bString)
      strcat(szBuffer, " (\"[");
    else
      strcat(szBuffer, " ([");
    /* attribute */

    strcat(szBuffer, psFilterNode->psLeftNode->pszValue);
    if (bString)
      strcat(szBuffer, "]\" ");
    else
      strcat(szBuffer, "] "); 
    

    /* logical operator */
    if (strcasecmp(psFilterNode->pszValue, 
                   "PropertyIsEqualTo") == 0)
    {
        /*case insensitive set ? */
        if (psFilterNode->psRightNode->pOther && 
            (*(int *)psFilterNode->psRightNode->pOther) == 1)
        {
            strcat(szBuffer, "IEQ");
        }
        else
          strcat(szBuffer, "=");
    }
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsNotEqualTo") == 0)
      strcat(szBuffer, "!="); 
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsLessThan") == 0)
      strcat(szBuffer, "<");
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsGreaterThan") == 0)
      strcat(szBuffer, ">");
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsLessThanOrEqualTo") == 0)
      strcat(szBuffer, "<=");
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsGreaterThanOrEqualTo") == 0)
      strcat(szBuffer, ">=");
    
    strcat(szBuffer, " ");
    
    /* value */
    if (bString)
      strcat(szBuffer, "\"");
    
    if (psFilterNode->psRightNode->pszValue)
      strcat(szBuffer, psFilterNode->psRightNode->pszValue);

    if (bString)
      strcat(szBuffer, "\"");
    
    strcat(szBuffer, ") ");

    return strdup(szBuffer);
}



/************************************************************************/
/*                      FLTGetBinaryComparisonSQLExpresssion            */
/*                                                                      */
/*      Return the expression for a binary comparison filter node.      */
/************************************************************************/
char *FLTGetBinaryComparisonSQLExpresssion(FilterEncodingNode *psFilterNode,
                                           layerObj *lp)
{
    char szBuffer[1024];
    int bString=0;
    char szTmp[256];

    szBuffer[0] = '\0';
    if (!psFilterNode || !
        FLTIsBinaryComparisonFilterType(psFilterNode->pszValue))
      return NULL;

/* -------------------------------------------------------------------- */
/*      check if the value is a numeric value or alphanumeric. If it    */
/*      is alphanumeric, add quotes around attribute and values.        */
/* -------------------------------------------------------------------- */
    bString = 0;
    if (psFilterNode->psRightNode->pszValue)
    {
        sprintf(szTmp, "%s_type",  psFilterNode->psLeftNode->pszValue);
        if (msOWSLookupMetadata(&(lp->metadata), "OFG", szTmp) != NULL &&
            (strcasecmp(msOWSLookupMetadata(&(lp->metadata), "G", szTmp), "Character") == 0))
          bString = 1;

        else if (FLTIsNumeric(psFilterNode->psRightNode->pszValue) == MS_FALSE)    
          bString = 1;
    }
    
    /* specical case to be able to have empty strings in the expression. */
    if (psFilterNode->psRightNode->pszValue == NULL)
      bString = 1;
      

    /*opening bracket*/
    strcat(szBuffer, " (");

    /* attribute */
    /*case insensitive set ? */
    if (bString &&
        strcasecmp(psFilterNode->pszValue, 
                   "PropertyIsEqualTo") == 0 &&
        psFilterNode->psRightNode->pOther && 
        (*(int *)psFilterNode->psRightNode->pOther) == 1)
    {
        sprintf(szTmp, "lower(%s) ",  psFilterNode->psLeftNode->pszValue);
        strcat(szBuffer, szTmp);
    }
    else
      strcat(szBuffer, psFilterNode->psLeftNode->pszValue);

    

    /* logical operator */
    if (strcasecmp(psFilterNode->pszValue, 
                   "PropertyIsEqualTo") == 0)
      strcat(szBuffer, "=");
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsNotEqualTo") == 0)
      strcat(szBuffer, "<>"); 
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsLessThan") == 0)
      strcat(szBuffer, "<");
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsGreaterThan") == 0)
      strcat(szBuffer, ">");
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsLessThanOrEqualTo") == 0)
      strcat(szBuffer, "<=");
    else if (strcasecmp(psFilterNode->pszValue, 
                        "PropertyIsGreaterThanOrEqualTo") == 0)
      strcat(szBuffer, ">=");
    
    strcat(szBuffer, " ");
    
    /* value */

    if (bString && 
        psFilterNode->psRightNode->pszValue &&
        strcasecmp(psFilterNode->pszValue, 
                   "PropertyIsEqualTo") == 0 &&
        psFilterNode->psRightNode->pOther && 
        (*(int *)psFilterNode->psRightNode->pOther) == 1)
    {
        sprintf(szTmp, "lower('%s') ",  psFilterNode->psRightNode->pszValue);
        strcat(szBuffer, szTmp);
    }
    else
    {
        if (bString)
          strcat(szBuffer, "'");
    
        if (psFilterNode->psRightNode->pszValue)
          strcat(szBuffer, psFilterNode->psRightNode->pszValue);

        if (bString)
          strcat(szBuffer, "'");

    }
    /*closing bracket*/
    strcat(szBuffer, ") ");

    return strdup(szBuffer);
}


/************************************************************************/
/*                    FLTGetIsBetweenComparisonSQLExpresssion           */
/*                                                                      */
/*      Build an SQL expresssion for IsBteween Filter.                  */
/************************************************************************/
char *FLTGetIsBetweenComparisonSQLExpresssion(FilterEncodingNode *psFilterNode,
                                              layerObj *lp)
{
    char szBuffer[1024];
    char **aszBounds = NULL;
    int nBounds = 0;
    int bString=0;
    char szTmp[256];


    szBuffer[0] = '\0';
    if (!psFilterNode ||
        !(strcasecmp(psFilterNode->pszValue, "PropertyIsBetween") == 0))
      return NULL;

    if (!psFilterNode->psLeftNode || !psFilterNode->psRightNode )
      return NULL;

/* -------------------------------------------------------------------- */
/*      Get the bounds value which are stored like boundmin;boundmax    */
/* -------------------------------------------------------------------- */
    aszBounds = msStringSplit(psFilterNode->psRightNode->pszValue, ';', &nBounds);
    if (nBounds != 2)
      return NULL;
/* -------------------------------------------------------------------- */
/*      check if the value is a numeric value or alphanumeric. If it    */
/*      is alphanumeric, add quotes around attribute and values.        */
/* -------------------------------------------------------------------- */
    bString = 0;
    if (aszBounds[0])
    {
        sprintf(szTmp, "%s_type",  psFilterNode->psLeftNode->pszValue);
        if (msOWSLookupMetadata(&(lp->metadata), "OFG", szTmp) != NULL &&
            (strcasecmp(msOWSLookupMetadata(&(lp->metadata), "G", szTmp), "Character") == 0))
          bString = 1;
        else if (FLTIsNumeric(aszBounds[0]) == MS_FALSE)    
          bString = 1;
    }
    if (!bString)
    {
        if (aszBounds[1])
        {
            if (FLTIsNumeric(aszBounds[1]) == MS_FALSE)    
              bString = 1;
        }
    }
        

/* -------------------------------------------------------------------- */
/*      build expresssion.                                              */
/* -------------------------------------------------------------------- */
    /*opening paranthesis */
    strcat(szBuffer, " (");

    /* attribute */
    strcat(szBuffer, psFilterNode->psLeftNode->pszValue);

    /*between*/
    strcat(szBuffer, " BETWEEN ");

    /*bound 1*/
    if (bString)
      strcat(szBuffer,"'");
    strcat(szBuffer, aszBounds[0]);
    if (bString)
      strcat(szBuffer,"'");

    strcat(szBuffer, " AND ");

    /*bound 2*/
    if (bString)
      strcat(szBuffer, "'");
    strcat(szBuffer, aszBounds[1]);
    if (bString)
      strcat(szBuffer,"'");

    /*closing paranthesis*/
    strcat(szBuffer, ")");
     
    
    return strdup(szBuffer);
}
  
/************************************************************************/
/*                    FLTGetIsBetweenComparisonExpresssion              */
/*                                                                      */
/*      Build expresssion for IsBteween Filter.                         */
/************************************************************************/
char *FLTGetIsBetweenComparisonExpresssion(FilterEncodingNode *psFilterNode,
                                           layerObj *lp)
{
    char szBuffer[1024];
    char **aszBounds = NULL;
    int nBounds = 0;
    int bString=0;
    char szTmp[256];


    szBuffer[0] = '\0';
    if (!psFilterNode ||
        !(strcasecmp(psFilterNode->pszValue, "PropertyIsBetween") == 0))
      return NULL;

    if (!psFilterNode->psLeftNode || !psFilterNode->psRightNode )
      return NULL;

/* -------------------------------------------------------------------- */
/*      Get the bounds value which are stored like boundmin;boundmax    */
/* -------------------------------------------------------------------- */
    aszBounds = msStringSplit(psFilterNode->psRightNode->pszValue, ';', &nBounds);
    if (nBounds != 2)
      return NULL;
/* -------------------------------------------------------------------- */
/*      check if the value is a numeric value or alphanumeric. If it    */
/*      is alphanumeric, add quotes around attribute and values.        */
/* -------------------------------------------------------------------- */
    bString = 0;
    if (aszBounds[0])
    {
        sprintf(szTmp, "%s_type",  psFilterNode->psLeftNode->pszValue);
        if (msOWSLookupMetadata(&(lp->metadata), "OFG", szTmp) != NULL &&
            (strcasecmp(msOWSLookupMetadata(&(lp->metadata), "G", szTmp), "Character") == 0))
          bString = 1;
        else if (FLTIsNumeric(aszBounds[0]) == MS_FALSE)    
          bString = 1;
    }
    if (!bString)
    {
        if (aszBounds[1])
        {
            if (FLTIsNumeric(aszBounds[1]) == MS_FALSE)    
              bString = 1;  
        }
    }
        

/* -------------------------------------------------------------------- */
/*      build expresssion.                                              */
/* -------------------------------------------------------------------- */
    if (bString)
      strcat(szBuffer, " (\"[");
    else
      strcat(szBuffer, " ([");

    /* attribute */
    strcat(szBuffer, psFilterNode->psLeftNode->pszValue);

    if (bString)
      strcat(szBuffer, "]\" ");
    else
      strcat(szBuffer, "] ");
        
    
    strcat(szBuffer, " >= ");
    if (bString)
      strcat(szBuffer,"\"");
    strcat(szBuffer, aszBounds[0]);
    if (bString)
      strcat(szBuffer,"\"");

    strcat(szBuffer, " AND ");

    if (bString)
      strcat(szBuffer, " \"[");
    else
      strcat(szBuffer, " ["); 

    /* attribute */
    strcat(szBuffer, psFilterNode->psLeftNode->pszValue);
    
    if (bString)
      strcat(szBuffer, "]\" ");
    else
      strcat(szBuffer, "] ");
    
    strcat(szBuffer, " <= ");
    if (bString)
      strcat(szBuffer,"\"");
    strcat(szBuffer, aszBounds[1]);
    if (bString)
      strcat(szBuffer,"\"");
    strcat(szBuffer, ")");
     
    
    return strdup(szBuffer);
}
    
/************************************************************************/
/*                      FLTGetIsLikeComparisonExpression               */
/*                                                                      */
/*      Build expression for IsLike filter.                             */
/************************************************************************/
char *FLTGetIsLikeComparisonExpression(FilterEncodingNode *psFilterNode)
{
    char szBuffer[1024];
    char szTmp[256];
    char *pszValue = NULL;
    
    char *pszWild = NULL;
    char *pszSingle = NULL;
    char *pszEscape = NULL;
    int  bCaseInsensitive = 0;

    int nLength=0, i=0, iTmp=0;


    if (!psFilterNode || !psFilterNode->pOther || !psFilterNode->psLeftNode ||
        !psFilterNode->psRightNode || !psFilterNode->psRightNode->pszValue)
      return NULL;

    pszWild = ((FEPropertyIsLike *)psFilterNode->pOther)->pszWildCard;
    pszSingle = ((FEPropertyIsLike *)psFilterNode->pOther)->pszSingleChar;
    pszEscape = ((FEPropertyIsLike *)psFilterNode->pOther)->pszEscapeChar;
    bCaseInsensitive = ((FEPropertyIsLike *)psFilterNode->pOther)->bCaseInsensitive;

    if (!pszWild || strlen(pszWild) == 0 ||
        !pszSingle || strlen(pszSingle) == 0 || 
        !pszEscape || strlen(pszEscape) == 0)
      return NULL;

 
/* -------------------------------------------------------------------- */
/*      Use operand with regular expressions.                           */
/* -------------------------------------------------------------------- */
    szBuffer[0] = '\0';
    sprintf(szTmp, "%s", " (\"[");
    szTmp[4] = '\0';

    strcat(szBuffer, szTmp);

    /* attribute */
    strcat(szBuffer, psFilterNode->psLeftNode->pszValue);
    szBuffer[strlen(szBuffer)] = '\0';

    sprintf(szTmp, "%s", "]\" =~ /");
    szTmp[7] = '\0';
    strcat(szBuffer, szTmp);
    szBuffer[strlen(szBuffer)] = '\0';


    pszValue = psFilterNode->psRightNode->pszValue;
    nLength = strlen(pszValue);
    
    iTmp =0;
    if (nLength > 0 && pszValue[0] != pszWild[0] && 
        pszValue[0] != pszSingle[0] &&
        pszValue[0] != pszEscape[0])
    {
      szTmp[iTmp]= '^';
      iTmp++;
    }
    for (i=0; i<nLength; i++)
    {
        if (pszValue[i] != pszWild[0] && 
            pszValue[i] != pszSingle[0] &&
            pszValue[i] != pszEscape[0])
        {
            szTmp[iTmp] = pszValue[i];
            iTmp++;
            szTmp[iTmp] = '\0';
        }
        else if  (pszValue[i] == pszSingle[0])
        {
             szTmp[iTmp] = '.';
             iTmp++;
             szTmp[iTmp] = '\0';
        }
        else if  (pszValue[i] == pszEscape[0])
        {
            szTmp[iTmp] = '\\';
            iTmp++;
            szTmp[iTmp] = '\0';

        }
        else if (pszValue[i] == pszWild[0])
        {
            /* strcat(szBuffer, "[0-9,a-z,A-Z,\\s]*"); */
            /* iBuffer+=17; */
            szTmp[iTmp++] = '.';
            szTmp[iTmp++] = '*';
            szTmp[iTmp] = '\0';
        }
    }   
    szTmp[iTmp] = '/';
    if (bCaseInsensitive == 1)
    {
      szTmp[++iTmp] = 'i';
    } 
    szTmp[++iTmp] = '\0';
    
    strcat(szBuffer, szTmp);
    strcat(szBuffer, ")");     
    return strdup(szBuffer);
}

/************************************************************************/
/*                      FLTGetIsLikeComparisonSQLExpression             */
/*                                                                      */
/*      Build an sql expression for IsLike filter.                      */
/************************************************************************/
char *FLTGetIsLikeComparisonSQLExpression(FilterEncodingNode *psFilterNode,
                                          int connectiontype)
{
    char szBuffer[1024];
    char *pszValue = NULL;
    
    char *pszWild = NULL;
    char *pszSingle = NULL;
    char *pszEscape = NULL;
    char szTmp[4];

    int nLength=0, i=0, iBuffer = 0;
    int  bCaseInsensitive = 0;

    if (!psFilterNode || !psFilterNode->pOther || !psFilterNode->psLeftNode ||
        !psFilterNode->psRightNode || !psFilterNode->psRightNode->pszValue)
      return NULL;

    pszWild = ((FEPropertyIsLike *)psFilterNode->pOther)->pszWildCard;
    pszSingle = ((FEPropertyIsLike *)psFilterNode->pOther)->pszSingleChar;
    pszEscape = ((FEPropertyIsLike *)psFilterNode->pOther)->pszEscapeChar;
    bCaseInsensitive = ((FEPropertyIsLike *)psFilterNode->pOther)->bCaseInsensitive;

    if (!pszWild || strlen(pszWild) == 0 ||
        !pszSingle || strlen(pszSingle) == 0 || 
        !pszEscape || strlen(pszEscape) == 0)
      return NULL;

 

    szBuffer[0] = '\0';
    /*opening bracket*/
    strcat(szBuffer, " (");

    /* attribute name */
    strcat(szBuffer, psFilterNode->psLeftNode->pszValue);
    if (bCaseInsensitive == 1 && connectiontype == MS_POSTGIS)
      strcat(szBuffer, " ilike '");
    else
      strcat(szBuffer, " like '");
        
   
    pszValue = psFilterNode->psRightNode->pszValue;
    nLength = strlen(pszValue);
    iBuffer = strlen(szBuffer);
    for (i=0; i<nLength; i++)
    {
        if (pszValue[i] != pszWild[0] && 
            pszValue[i] != pszSingle[0] &&
            pszValue[i] != pszEscape[0])
        {
            szBuffer[iBuffer] = pszValue[i];
            iBuffer++;
            szBuffer[iBuffer] = '\0';
        }
        else if  (pszValue[i] == pszSingle[0])
        {
             szBuffer[iBuffer] = '_';
             iBuffer++;
             szBuffer[iBuffer] = '\0';
        }
        else if  (pszValue[i] == pszEscape[0])
        {
            szBuffer[iBuffer] = pszEscape[0];
            iBuffer++;
            szBuffer[iBuffer] = '\0';
            /*if (i<nLength-1)
            {
                szBuffer[iBuffer] = pszValue[i+1];
                iBuffer++;
                szBuffer[iBuffer] = '\0';
            }
            */
        }
        else if (pszValue[i] == pszWild[0])
        {
            strcat(szBuffer, "%");
            iBuffer++;
            szBuffer[iBuffer] = '\0';
        }
    } 

    strcat(szBuffer, "'");
    if (connectiontype != MS_OGR)
    {
      strcat(szBuffer, " escape '");
      szTmp[0] = pszEscape[0];
      if (pszEscape[0] == '\\')
      {
          szTmp[1] = '\\';
          szTmp[2] = '\'';
          szTmp[3] = '\0';
      }
      else
      {
          szTmp[1] = '\'';
          szTmp[2] = '\0';
      }

      strcat(szBuffer,  szTmp);
    }
    strcat(szBuffer,  ") ");
    
    return strdup(szBuffer);
}

/************************************************************************/
/*                           FLTHasSpatialFilter                        */
/*                                                                      */
/*      Utility function to see if a spatial filter is included in      */
/*      the node.                                                       */
/************************************************************************/
int FLTHasSpatialFilter(FilterEncodingNode *psNode)
{
    int bResult = MS_FALSE;

    if (!psNode)
      return MS_FALSE;

    if (psNode->eType == FILTER_NODE_TYPE_LOGICAL)
    {
        if (psNode->psLeftNode)
          bResult = FLTHasSpatialFilter(psNode->psLeftNode);

        if (bResult)
          return MS_TRUE;

        if (psNode->psRightNode)
           bResult = FLTHasSpatialFilter(psNode->psRightNode);

        if (bResult)
          return MS_TRUE;
    }
    else if (FLTIsBBoxFilter(psNode) || FLTIsPointFilter(psNode) ||
             FLTIsLineFilter(psNode) || FLTIsPolygonFilter(psNode))
      return MS_TRUE;


    return MS_FALSE;
}


/************************************************************************/
/*                     FLTCreateFeatureIdFilterEncoding                 */
/*                                                                      */
/*      Utility function to create a filter node of FeatureId type.     */
/************************************************************************/
FilterEncodingNode *FLTCreateFeatureIdFilterEncoding(char *pszString)
{
    FilterEncodingNode *psFilterNode = NULL;
    char **tokens = NULL;
    int nTokens = 0;

    if (pszString)
    {
        psFilterNode = FLTCreateFilterEncodingNode();
        psFilterNode->eType = FILTER_NODE_TYPE_FEATUREID;
        /*split if tyname is included in the string*/
        tokens = msStringSplit(pszString,'.', &nTokens);
        if (tokens && nTokens == 2)
          psFilterNode->pszValue = strdup(tokens[1]);
        else
          psFilterNode->pszValue =  strdup(pszString);

        if (tokens)
          msFreeCharArray(tokens, nTokens); 
        return psFilterNode;
    }
    return NULL;
}


/************************************************************************/
/*                              FLTParseGMLBox                          */
/*                                                                      */
/*      Parse gml box. Used for FE 1.0                                  */
/************************************************************************/
int FLTParseGMLBox(CPLXMLNode *psBox, rectObj *psBbox, char **ppszSRS)
{
    int bCoordinatesValid = 0;
    CPLXMLNode *psCoordinates = NULL, *psCoordChild=NULL;
    CPLXMLNode *psCoord1 = NULL, *psCoord2 = NULL;
    CPLXMLNode *psX = NULL, *psY = NULL;
    char **szCoords=NULL, **szMin=NULL, **szMax = NULL;
    char  *szCoords1=NULL, *szCoords2 = NULL;
    int nCoords = 0;
    char *pszTmpCoord = NULL;
    char *pszSRS = NULL;
    char *pszTS = NULL;
    char *pszCS = NULL;

    if (psBox)
    {
        pszSRS = (char *)CPLGetXMLValue(psBox, "srsName", NULL);
        if (ppszSRS && pszSRS)
          *ppszSRS = strdup(pszSRS);

        psCoordinates = CPLGetXMLNode(psBox, "coordinates");
        if (!psCoordinates)
          return 0;
        pszTS = (char *)CPLGetXMLValue(psCoordinates, "ts", NULL);
        pszCS = (char *)CPLGetXMLValue(psCoordinates, "cs", NULL);

        psCoordChild =  psCoordinates->psChild;
        while (psCoordinates && psCoordChild && psCoordChild->eType != CXT_Text)
        {
            psCoordChild = psCoordChild->psNext;
        }
        if (psCoordChild && psCoordChild->pszValue)
        {
            pszTmpCoord = psCoordChild->pszValue;
            if (pszTS)
              szCoords = msStringSplit(pszTmpCoord, pszTS[0], &nCoords);
            else
              szCoords = msStringSplit(pszTmpCoord, ' ', &nCoords);
            if (szCoords && nCoords == 2)
            {
                szCoords1 = strdup(szCoords[0]);
                szCoords2 = strdup(szCoords[1]);
                if (pszCS)
                  szMin = msStringSplit(szCoords1, pszCS[0], &nCoords);
                else
                  szMin = msStringSplit(szCoords1, ',', &nCoords);
                if (szMin && nCoords == 2)
                {
                    if (pszCS)
                      szMax = msStringSplit(szCoords2, pszCS[0], &nCoords);
                    else
                      szMax = msStringSplit(szCoords2, ',', &nCoords);
                }
                if (szMax && nCoords == 2)
                  bCoordinatesValid =1;

                free(szCoords1);        
                free(szCoords2);
            }
        }
        else
        {
            psCoord1 = CPLGetXMLNode(psBox, "coord");
            if (psCoord1 && psCoord1->psNext && 
                psCoord1->psNext->pszValue && 
                strcmp(psCoord1->psNext->pszValue, "coord") ==0)
            {
                szMin = (char **)malloc(sizeof(char *)*2);
                szMax = (char **)malloc(sizeof(char *)*2);
                psCoord2 = psCoord1->psNext;
                psX =  CPLGetXMLNode(psCoord1, "X");
                psY =  CPLGetXMLNode(psCoord1, "Y");
                if (psX && psY && psX->psChild && psY->psChild &&
                    psX->psChild->pszValue && psY->psChild->pszValue)
                {
                    szMin[0] = psX->psChild->pszValue;
                    szMin[1] = psY->psChild->pszValue;

                    psX =  CPLGetXMLNode(psCoord2, "X");
                    psY =  CPLGetXMLNode(psCoord2, "Y");
                    if (psX && psY && psX->psChild && psY->psChild &&
                        psX->psChild->pszValue && psY->psChild->pszValue)
                    {
                        szMax[0] = psX->psChild->pszValue;
                        szMax[1] = psY->psChild->pszValue;
                        bCoordinatesValid = 1;
                    }
                }
            }
                    
        }
    }

    if (bCoordinatesValid)
    {
        psBbox->minx =  atof(szMin[0]);
        psBbox->miny =  atof(szMin[1]);

        psBbox->maxx =  atof(szMax[0]);
        psBbox->maxy =  atof(szMax[1]);

        if (szMin)
          msFree(szMin);
        if (szMax)
          msFree(szMax);
    }

    return bCoordinatesValid;
}
/************************************************************************/
/*                           FLTParseGMLEnvelope                        */
/*                                                                      */
/*      Utility function to parse a gml:Enevelop (used for SOS and FE1.1)*/
/************************************************************************/
int FLTParseGMLEnvelope(CPLXMLNode *psRoot, rectObj *psBbox, char **ppszSRS)
{
    CPLXMLNode *psChild=NULL; 
    CPLXMLNode *psUpperCorner=NULL, *psLowerCorner=NULL;
    char *pszLowerCorner=NULL, *pszUpperCorner=NULL;
    int bValid = 0;
    char **tokens;
    int n;

    if (psRoot && psBbox && psRoot->eType == CXT_Element && 
        EQUAL(psRoot->pszValue,"Envelope")) 
    {
        /*Get the srs if available*/
        if (ppszSRS)
        {
            psChild = psRoot->psChild;
            while (psChild != NULL) 
            {
               if (psChild->eType == CXT_Attribute && psChild->pszValue &&
                   EQUAL(psChild->pszValue, "srsName") && psChild->psChild &&
                   psChild->psChild->pszValue)
               {
                   *ppszSRS = strdup(psChild->psChild->pszValue);
                   break;
               } 
               psChild = psChild->psNext;
            }
        }
        psLowerCorner = CPLSearchXMLNode(psRoot, "lowerCorner");
        psUpperCorner = CPLSearchXMLNode(psRoot, "upperCorner");

        if (psLowerCorner && psUpperCorner && EQUAL(psLowerCorner->pszValue,"lowerCorner") && 
            EQUAL(psUpperCorner->pszValue,"upperCorner")) 
        {
                /*get the values*/
            psChild = psLowerCorner->psChild;
            while (psChild != NULL) {
                if (psChild->eType != CXT_Text)
                  psChild = psChild->psNext;
                else
                  break;
            }
            if (psChild && psChild->eType == CXT_Text)
              pszLowerCorner = psChild->pszValue;

            psChild = psUpperCorner->psChild;
            while (psChild != NULL) {
                if (psChild->eType != CXT_Text)
                  psChild = psChild->psNext;
                else
                  break;
            }
            if (psChild && psChild->eType == CXT_Text)
              pszUpperCorner = psChild->pszValue;

            if (pszLowerCorner && pszUpperCorner) {
                tokens = msStringSplit(pszLowerCorner, ' ', &n);
                if (tokens && n >= 2) {
                    psBbox->minx = atof(tokens[0]);
                    psBbox->miny = atof(tokens[1]);

                    msFreeCharArray(tokens, n);

                    tokens = msStringSplit(pszUpperCorner, ' ', &n);
                    if (tokens && n >= 2) {
                        psBbox->maxx = atof(tokens[0]);
                        psBbox->maxy = atof(tokens[1]);
                        msFreeCharArray(tokens, n);

                        bValid = 1;
                    }
                }
            }
        }
    }
    return bValid;
}


static void FLTReplacePropertyName(FilterEncodingNode *psFilterNode,
                                   const char *pszOldName, char *pszNewName)
{
    if (psFilterNode && pszOldName && pszNewName)
    {
        if (psFilterNode->eType == FILTER_NODE_TYPE_PROPERTYNAME)
        {
            if (psFilterNode->pszValue && 
                strcasecmp(psFilterNode->pszValue, pszOldName) == 0)
            {
                msFree(psFilterNode->pszValue);
                psFilterNode->pszValue = strdup(pszNewName);
            }
        }
        if (psFilterNode->psLeftNode)
          FLTReplacePropertyName(psFilterNode->psLeftNode, pszOldName,
                                   pszNewName);
        if (psFilterNode->psRightNode)
          FLTReplacePropertyName(psFilterNode->psRightNode, pszOldName,
                                 pszNewName);
    }
}


static void FLTStripNameSpacesFromPropertyName(FilterEncodingNode *psFilterNode)
{
    char **tokens=NULL;
    int n=0;

    if (psFilterNode)
    {
         if (psFilterNode->eType == FILTER_NODE_TYPE_PROPERTYNAME)
         {
             if (psFilterNode->pszValue &&  
                 strstr(psFilterNode->pszValue, ":"))
             {
                 tokens = msStringSplit(psFilterNode->pszValue, ':', &n);
                 if (tokens && n==2)
                 {
                     msFree(psFilterNode->pszValue);
                     psFilterNode->pszValue = strdup(tokens[1]);
                 }
                 if (tokens && n>0)
                   msFreeCharArray(tokens, n);
             }
         }
         if (psFilterNode->psLeftNode)
           FLTStripNameSpacesFromPropertyName(psFilterNode->psLeftNode);
         if (psFilterNode->psRightNode)
           FLTStripNameSpacesFromPropertyName(psFilterNode->psRightNode);
    }

}

/************************************************************************/
/*                        FLTPreParseFilterForAlias                     */
/*                                                                      */
/*      Utility function to replace aliased' attributes with their      */
/*      real name.                                                      */
/************************************************************************/
void FLTPreParseFilterForAlias(FilterEncodingNode *psFilterNode, 
                               mapObj *map, int i, const char *namespaces)
{
    layerObj *lp=NULL;
    char szTmp[256];
    const char *pszFullName = NULL;

#if defined(USE_WMS_SVR) || defined (USE_WFS_SVR) || defined (USE_WCS_SVR) || defined(USE_SOS_SVR)

    if (psFilterNode && map && i>=0 && i<map->numlayers)
    {
        /*strip name speces befor hand*/
        FLTStripNameSpacesFromPropertyName(psFilterNode);

        lp = GET_LAYER(map, i);
        if (msLayerOpen(lp) == MS_SUCCESS && msLayerGetItems(lp) == MS_SUCCESS)
        {
            for(i=0; i<lp->numitems; i++) 
            {
                if (!lp->items[i] || strlen(lp->items[i]) <= 0)
                    continue;
                sprintf(szTmp, "%s_alias", lp->items[i]);
                pszFullName = msOWSLookupMetadata(&(lp->metadata), namespaces, szTmp);
                if (pszFullName)
                {
                    FLTReplacePropertyName(psFilterNode, pszFullName, 
                                           lp->items[i]);
                }
            }
            msLayerClose(lp);
        }
    } 
#else
    msSetError(MS_MISCERR, "OWS support is not available.", 
               "FLTPreParseFilterForAlias()");
    
#endif   
}


#ifdef USE_LIBXML2

xmlNodePtr FLTGetCapabilities(xmlNsPtr psNsParent, xmlNsPtr psNsOgc, int bTemporal)
{
    xmlNodePtr psRootNode = NULL, psNode = NULL, psSubNode = NULL, psSubSubNode = NULL;
    
    psRootNode = xmlNewNode(psNsParent, BAD_CAST "Filter_Capabilities");
    
    psNode = xmlNewChild(psRootNode, psNsOgc, BAD_CAST "Spatial_Capabilities", NULL);

    psSubNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "GeometryOperands", NULL);
    psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "GeometryOperand", BAD_CAST "gml:Point");
    psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "GeometryOperand", BAD_CAST "gml:LineString");
    psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "GeometryOperand", BAD_CAST "gml:Polygon");
    psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "GeometryOperand", BAD_CAST "gml:Envelope");

    psSubNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "SpatialOperators", NULL);
#ifdef USE_GEOS
    psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
    xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "Equals");
    psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
    xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "Disjoint");
    psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
    xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "Touches");
    psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
    xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "Within");
    psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
    xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "Overlaps");
    psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
    xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "Crosses");
    psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
    xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "Intersects");
    psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
    xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "Contains");
    psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
    xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "DWithin");
    psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
    xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "Beyond");
#endif
    psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "SpatialOperator", NULL);
    xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "BBOX");

    if (bTemporal)
    {
        psNode = xmlNewChild(psRootNode, psNsOgc, BAD_CAST "Temporal_Capabilities", NULL);
        psSubNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "TemporalOperands", NULL);
        psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "TemporalOperand", BAD_CAST "gml:TimePeriod");
        psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "TemporalOperand", BAD_CAST "gml:TimeInstant");

        psSubNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "TemporalOperators", NULL);
        psSubSubNode = xmlNewChild(psSubNode, psNsOgc, BAD_CAST "TemporalOperator", NULL);
        xmlNewProp(psSubSubNode, BAD_CAST "name", BAD_CAST "TM_Equals");
    }
    psNode = xmlNewChild(psRootNode, psNsOgc, BAD_CAST "Scalar_Capabilities", NULL);
    xmlNewChild(psNode, psNsOgc, BAD_CAST "LogicalOperators", NULL);
    psNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "ComparisonOperators", NULL);
    psSubNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "ComparisonOperator", BAD_CAST "LessThan");
    psSubNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "ComparisonOperator", BAD_CAST "GreaterThan");
    psSubNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "ComparisonOperator", BAD_CAST "LessThanEqualTo");
    psSubNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "ComparisonOperator", BAD_CAST "GreaterThanEqualTo");
    psSubNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "ComparisonOperator", BAD_CAST "EqualTo");
    psSubNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "ComparisonOperator", BAD_CAST "NotEqualTo");
    psSubNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "ComparisonOperator", BAD_CAST "Like");
    psSubNode = xmlNewChild(psNode, psNsOgc, BAD_CAST "ComparisonOperator", BAD_CAST "Between");
    
    psNode = xmlNewChild(psRootNode, psNsOgc, BAD_CAST "Id_Capabilities", NULL);
    xmlNewChild(psNode, psNsOgc, BAD_CAST "EID", NULL);
    xmlNewChild(psNode, psNsOgc, BAD_CAST "FID", NULL);
    return psRootNode;
}
#endif
#endif
