/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  cgiRequestObj and CGI parsing utility related declarations.
 * Author:   Steve Lime and the MapServer team.
 *
 ******************************************************************************
 * Copyright (c) 1996-2005 Regents of the University of Minnesota.
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
 * Revision 1.18  2006/02/22 05:04:34  sdlime
 * Applied patch for bug 1660 to hide certain structures from Swig-based MapScript.
 *
 * Revision 1.17  2005/06/14 16:03:33  dan
 * Updated copyright date to 2005
 *
 * Revision 1.16  2005/02/18 03:06:44  dan
 * Turned all C++ (//) comments into C comments (bug 1238)
 *
 * Revision 1.15  2004/10/21 04:30:54  frank
 * Added standardized headers.  Added MS_CVSID().
 *
 */

#ifndef CGIUTIL_H
#define CGIUTIL_H

#if defined(_WIN32) && !defined(__CYGWIN__)
#  define MS_DLL_EXPORT     __declspec(dllexport)
#else
#define  MS_DLL_EXPORT
#endif

/*
** Misc. defines
*/
#define MAX_PARAMS 10000

enum MS_REQUEST_TYPE {MS_GET_REQUEST, MS_POST_REQUEST};

/* structure to hold request information */
typedef struct
{
#ifndef SWIG
  char **ParamNames;
  char **ParamValues;
#endif

#ifdef SWIG
%immutable;
#endif
  int NumParams;
#ifdef SWIG
%mutable;
#endif

  enum MS_REQUEST_TYPE type;
  char *contenttype;

  char *postrequest;
} cgiRequestObj;
      

/*
** Function prototypes
*/
#ifndef SWIG
MS_DLL_EXPORT  int loadParams(cgiRequestObj *);
MS_DLL_EXPORT void getword(char *, char *, char);
MS_DLL_EXPORT char *makeword_skip(char *, char, char);
MS_DLL_EXPORT char *makeword(char *, char);
MS_DLL_EXPORT char *fmakeword(FILE *, char, int *);
MS_DLL_EXPORT char x2c(char *);
MS_DLL_EXPORT void unescape_url(char *);
MS_DLL_EXPORT void plustospace(char *);
MS_DLL_EXPORT int rind(char *, char);
MS_DLL_EXPORT int _getline(char *, int, FILE *);
MS_DLL_EXPORT void send_fd(FILE *, FILE *);
MS_DLL_EXPORT int ind(char *, char);
MS_DLL_EXPORT void escape_shell_cmd(char *);

MS_DLL_EXPORT cgiRequestObj *msAllocCgiObj(void);
MS_DLL_EXPORT void msFreeCgiObj(cgiRequestObj *request);
#endif /*SWIG*/

#endif /* CGIUTIL_H */
