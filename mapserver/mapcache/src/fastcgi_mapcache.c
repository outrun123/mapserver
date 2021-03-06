/*
 *  Copyright 2010 Thomas Bonfort
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "mapcache.h"
#ifdef USE_FASTCGI
#include <fcgi_stdio.h>
#endif
#include <stdlib.h>
#include <apr_strings.h>
#include <apr_pools.h>
#include <apr_file_io.h>
#include <apr_date.h>

typedef struct mapcache_context_fcgi mapcache_context_fcgi;
typedef struct mapcache_context_fcgi_request mapcache_context_fcgi_request;

static char *err400 = "Bad Request";
static char *err404 = "Not Found";
static char *err500 = "Internal Server Error";
static char *err501 = "Not Implemented";
static char *err502 = "Bad Gateway";
static char *errother = "No Description";
apr_pool_t *global_pool;

static char* err_msg(int code) {
   switch(code) {
      case 400:
         return err400;
      case 404:
         return err404;
      case 500:
         return err500;
      case 501:
         return err501;
      case 502:
         return err502;
      default:
         return errother;
   }
}

struct mapcache_context_fcgi {
   mapcache_context ctx;
   char *mutex_fname;
   apr_file_t *mutex_file;
};

void fcgi_context_log(mapcache_context *c, mapcache_log_level level, char *message, ...) {
   va_list args;
   va_start(args,message);
   fprintf(stderr,"%s\n",apr_pvsprintf(c->pool,message,args));
   va_end(args);
}


void mapcache_fcgi_mutex_aquire(mapcache_context *gctx) {
   mapcache_context_fcgi *ctx = (mapcache_context_fcgi*)gctx;
   int ret;
#ifdef DEBUG
   if(ctx->mutex_file != NULL) {
      gctx->set_error(gctx, 500, "SEVERE: fcgi recursive mutex acquire");
      return; /* BUG ! */
   }
#endif
   if (apr_file_open(&ctx->mutex_file, ctx->mutex_fname,
             APR_FOPEN_CREATE | APR_FOPEN_WRITE | APR_FOPEN_SHARELOCK | APR_FOPEN_BINARY,
             APR_OS_DEFAULT, gctx->pool) != APR_SUCCESS) {
      gctx->set_error(gctx, 500, "failed to create fcgi mutex lockfile %s", ctx->mutex_fname);
      return; /* we could not create the file */
   }
   ret = apr_file_lock(ctx->mutex_file, APR_FLOCK_EXCLUSIVE);
   if (ret != APR_SUCCESS) {
      gctx->set_error(gctx, 500, "failed to lock fcgi mutex file %s", ctx->mutex_fname);
      return;
   }
}

void mapcache_fcgi_mutex_release(mapcache_context *gctx) {
   int ret;
   mapcache_context_fcgi *ctx = (mapcache_context_fcgi*)gctx;
#ifdef DEBUG
   if(ctx->mutex_file == NULL) {
      gctx->set_error(gctx, 500, "SEVERE: fcgi mutex unlock on unlocked file");
      return; /* BUG ! */
   }
#endif
   ret = apr_file_unlock(ctx->mutex_file);
   if(ret != APR_SUCCESS) {
      gctx->set_error(gctx, 500,  "failed to unlock fcgi mutex file%s",ctx->mutex_fname);
   }
   ret = apr_file_close(ctx->mutex_file);
   if(ret != APR_SUCCESS) {
      gctx->set_error(gctx, 500,  "failed to close fcgi mutex file %s",ctx->mutex_fname);
   }
   ctx->mutex_file = NULL;
}

static mapcache_context_fcgi* fcgi_context_create() {
   mapcache_context_fcgi *ctx = apr_pcalloc(global_pool, sizeof(mapcache_context_fcgi));
   if(!ctx) {
      return NULL;
   }
   ctx->ctx.pool = global_pool;
   mapcache_context_init((mapcache_context*)ctx);
   ctx->ctx.log = fcgi_context_log;
   ctx->mutex_fname="/tmp/mapcache.fcgi.lock";
   ctx->ctx.global_lock_aquire = mapcache_fcgi_mutex_aquire;
   ctx->ctx.global_lock_release = mapcache_fcgi_mutex_release;
   return ctx;
}

static void fcgi_write_response(mapcache_context_fcgi *ctx, mapcache_http_response *response) {
   if(response->code != 200) {
      printf("Status: %d %s\r\n",response->code, err_msg(response->code));
   }
   if(response->headers && !apr_is_empty_table(response->headers)) {
      const apr_array_header_t *elts = apr_table_elts(response->headers);
      int i;
      for(i=0;i<elts->nelts;i++) {
         apr_table_entry_t entry = APR_ARRAY_IDX(elts,i,apr_table_entry_t);
         printf("%s: %s\r\n", entry.key, entry.val);
      }
   }
   if(response->mtime) {
      char *if_modified_since = getenv("HTTP_IF_MODIFIED_SINCE");
      if(if_modified_since) {
        apr_time_t ims_time;
        apr_int64_t ims,mtime;


        mtime =  apr_time_sec(response->mtime);
        ims_time = apr_date_parse_http(if_modified_since);
        ims = apr_time_sec(ims_time);
        if(ims >= mtime) {
            printf("Status: 304 Not Modified\r\n");
        }
      }
      char *datestr = apr_palloc(ctx->ctx.pool, APR_RFC822_DATE_LEN);
      apr_rfc822_date(datestr, response->mtime);
      printf("Last-Modified: %s\r\n", datestr);
   }
   if(response->data) {
      printf("Content-Length: %d\r\n\r\n", response->data->size);
      fwrite((char*)response->data->buf, response->data->size,1,stdout);
   }
}


int main(int argc, char **argv) {
   apr_pool_initialize();
   if(apr_pool_create(&global_pool,NULL) != APR_SUCCESS) {
      return 1;
   }
   mapcache_context_fcgi* globalctx = fcgi_context_create();
   mapcache_context* ctx = (mapcache_context*)globalctx;
   mapcache_cfg *cfg = mapcache_configuration_create(ctx->pool);
   ctx->config = cfg;

   char *conffile  = getenv("MAPCACHE_CONFIG_FILE");
   if(!conffile) {
      ctx->log(ctx,MAPCACHE_ERROR,"no config file found in MAPCACHE_CONFIG_FILE envirronement");
      return 1;
   }
   ctx->log(ctx,MAPCACHE_DEBUG,"mapcache fcgi conf file: %s",conffile);
   mapcache_configuration_parse(ctx,conffile,cfg,1);
   if(GC_HAS_ERROR(ctx)) {
      ctx->log(ctx,500,"failed to parse %s: %s",conffile,ctx->get_error_message(ctx));
      return 1;
   }
   mapcache_configuration_post_config(ctx, cfg);
   if(GC_HAS_ERROR(ctx)) {
      ctx->log(ctx,500,"post-config failed for %s: %s",conffile,ctx->get_error_message(ctx));
      return 1;
   }
   
#ifdef USE_FASTCGI
   while (FCGI_Accept() >= 0) {
#endif
      apr_table_t *params;
      apr_pool_create(&(ctx->pool),global_pool);
      mapcache_request *request = NULL;
      char *pathInfo = getenv("PATH_INFO");
      

      params = mapcache_http_parse_param_string(ctx, getenv("QUERY_STRING"));
      mapcache_service_dispatch_request(ctx,&request,pathInfo,params,cfg);
      if(GC_HAS_ERROR(ctx) || !request) {
         fcgi_write_response(globalctx, mapcache_core_respond_to_error(ctx,(request)?request->service:NULL));
         goto cleanup;
      }
      
      mapcache_http_response *http_response = NULL;
      if(request->type == MAPCACHE_REQUEST_GET_CAPABILITIES) {
         mapcache_request_get_capabilities *req = (mapcache_request_get_capabilities*)request;
         char *host = getenv("SERVER_NAME");
         char *port = getenv("SERVER_PORT");
         char *fullhost;
         char *url;
         if(getenv("HTTPS")) {
            if(!port || !strcmp(port,"443")) {
               fullhost = apr_psprintf(ctx->pool,"https://%s",host);
            } else {
               fullhost = apr_psprintf(ctx->pool,"https://%s:%s",host,port);
            }
         } else {
            if(!port || !strcmp(port,"80")) {
               fullhost = apr_psprintf(ctx->pool,"http://%s",host);
            } else {
               fullhost = apr_psprintf(ctx->pool,"http://%s:%s",host,port);
            }
         }
         url = apr_psprintf(ctx->pool,"%s%s/",
               fullhost,
               getenv("SCRIPT_NAME")
               );
         http_response = mapcache_core_get_capabilities(ctx,request->service,req,url,pathInfo,cfg);
      } else if( request->type == MAPCACHE_REQUEST_GET_TILE) {
         mapcache_request_get_tile *req_tile = (mapcache_request_get_tile*)request;
         http_response = mapcache_core_get_tile(ctx,req_tile);
      } else if( request->type == MAPCACHE_REQUEST_PROXY ) {
         mapcache_request_proxy *req_proxy = (mapcache_request_proxy*)request;
         http_response = mapcache_core_proxy_request(ctx, req_proxy);
      } else if( request->type == MAPCACHE_REQUEST_GET_MAP) {
         mapcache_request_get_map *req_map = (mapcache_request_get_map*)request;
         http_response = mapcache_core_get_map(ctx,req_map);
      } else if( request->type == MAPCACHE_REQUEST_GET_FEATUREINFO) {
         mapcache_request_get_feature_info *req_fi = (mapcache_request_get_feature_info*)request;
         http_response = mapcache_core_get_featureinfo(ctx,req_fi);
#ifdef DEBUG
      } else {
         ctx->set_error(ctx,500,"###BUG### unknown request type");
#endif
      }
      if(GC_HAS_ERROR(ctx)) {
         fcgi_write_response(globalctx, mapcache_core_respond_to_error(ctx,request->service));
         goto cleanup;
      }
#ifdef DEBUG
      if(!http_response) {
         ctx->set_error(ctx,500,"###BUG### NULL response");
         fcgi_write_response(globalctx, mapcache_core_respond_to_error(ctx,request->service));
         goto cleanup;
      }
#endif
      fcgi_write_response(globalctx,http_response);
cleanup:
#ifdef USE_FASTCGI
      apr_pool_destroy(ctx->pool);
      ctx->clear_errors(ctx);
   }
#endif
   apr_pool_destroy(global_pool);
   return 0;

}
/* vim: ai ts=3 sts=3 et sw=3
*/
