/*
 * source.c
 *
 *  Created on: Oct 10, 2010
 *      Author: tom
 */

#include "geocache.h"

static int _geocache_source_render_tile(geocache_tile *tile, geocache_context *r) {
   return GEOCACHE_FAILURE;
}

static int _geocache_source_render_metatile(geocache_metatile *tile, geocache_context *r) {
   return GEOCACHE_FAILURE;
}

void geocache_source_init(geocache_source *source, geocache_context *r) {
	source->data_extent[0] =
			source->data_extent[1] =
			source->data_extent[2] =
			source->data_extent[3] = -1;
	source->srs = NULL;
	source->supports_metatiling = 0;
	source->render_metatile = _geocache_source_render_metatile;
	source->render_tile = _geocache_source_render_tile;
}
