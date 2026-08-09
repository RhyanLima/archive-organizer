#include "org_size.h"
#include <stdio.h>

bool match_size(const char *filepath, const SizeConfig *cfg, long long *out_size) {
    struct stat st;
    if (stat(filepath, &st) != 0) return false;
    *out_size = st.st_size;

    if (!cfg->active) return true;

    if (cfg->mode == MODE_LESS) return *out_size < cfg->val1;
    if (cfg->mode == MODE_GREATER) return *out_size > cfg->val1;
    if (cfg->mode == MODE_RANGE) return (*out_size >= cfg->val1 && *out_size <= cfg->val2);
    
    return true; // BANDS processa todos que chegam aqui
}

void get_size_foldername(long long file_size, const SizeConfig *cfg, char *out_name) {
    if (cfg->mode == MODE_BANDS) {
        long long band_idx = file_size / cfg->val2;
        long long mb = 1024 * 1024;
        sprintf(out_name, "%lldMB-%lldMB", (band_idx * cfg->val2)/mb, ((band_idx+1) * cfg->val2)/mb);
    } else if (cfg->mode == MODE_RANGE) {
        long long mb = 1024 * 1024;
        sprintf(out_name, "Faixa_%lldMB-%lldMB", cfg->val1/mb, cfg->val2/mb);
    } else {
        sprintf(out_name, "Filtrados_Tamanho");
    }
}