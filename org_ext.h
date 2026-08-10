#ifndef ORG_EXT_H
#define ORG_EXT_H

#include <stdbool.h>
#include "org_types.h"

bool match_ext(
    const char *filename,
    const ExtConfig *cfg
);

void get_ext_foldername(
    const char *filename,
    const ExtConfig *cfg,
    char *out_name
);

#endif
