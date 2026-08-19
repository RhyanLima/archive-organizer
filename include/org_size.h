#ifndef ORG_SIZE_H
#define ORG_SIZE_H
#include "org_types.h"
#include <sys/stat.h>

bool match_size(
    const char *filepath, 
    const SizeConfig *cfg, 
    long long *out_size
);

void get_size_foldername(
    long long file_size, 
    const SizeConfig *cfg, 
    char *out_name
);

#endif