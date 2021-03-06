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



void mapcache_source_init(mapcache_context *ctx, mapcache_source *source) {
	source->data_extent[0] =
			source->data_extent[1] =
			source->data_extent[2] =
			source->data_extent[3] = -1;
	source->metadata = apr_table_make(ctx->pool,3);
}
/* vim: ai ts=3 sts=3 et sw=3
*/
