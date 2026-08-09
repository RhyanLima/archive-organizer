#include "org_ext.h"
#include <string.h>

static const char* get_extension(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

bool match_ext(const char *filename, const ExtConfig *cfg) {
    if (!cfg->active) return true;
    if (cfg->mode == MODE_ALL) return true;
    
    const char *ext = get_extension(filename);
    return (strcmp(ext, cfg->specific_ext) == 0);
}

void get_ext_foldername(const char *filename, const ExtConfig *cfg, char *out_name) {
    if (cfg->mode == MODE_SPECIFIC) {
        strcpy(out_name, cfg->specific_ext);
    } else {
        const char *ext = get_extension(filename);
        strcpy(out_name, ext[0] ? ext : "sem_extensao");
    }
}