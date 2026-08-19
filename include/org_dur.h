#ifndef ORG_DUR_H
#define ORG_DUR_H

#include <stdbool.h>
#include "org_types.h"

bool match_dur(
    const char *filepath,
    const DurConfig *cfg,
    double *out_dur
);

void get_dur_foldername(
    double dur,
    const DurConfig *cfg,
    char *out_name
);

#endif