#include "org_dur.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static double get_duration_via_ffprobe(const char *filepath) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 \"%s\" 2>/dev/null", filepath);
    FILE *fp = popen(cmd, "r");
    if (!fp) return -1.0;
    
    char result[64] = {0};
    if (fgets(result, sizeof(result), fp) != NULL) {
        pclose(fp);
        return atof(result);
    }
    pclose(fp);
    return -1.0;
}

bool match_dur(const char *filepath, const DurConfig *cfg, double *out_dur) {
    if (!cfg->active) return true;
    
    *out_dur = get_duration_via_ffprobe(filepath);
    if (*out_dur < 0) return false; // Falhou ao ler ou não é mídia

    if (cfg->mode == MODE_LESS) return *out_dur < cfg->val1;
    if (cfg->mode == MODE_GREATER) return *out_dur > cfg->val1;
    if (cfg->mode == MODE_RANGE) return (*out_dur >= cfg->val1 && *out_dur <= cfg->val2);
    
    return true;
}

void get_dur_foldername(double dur, const DurConfig *cfg, char *out_name) {
    if (cfg->mode == MODE_BANDS) {
        int band_idx = (int)(dur / cfg->val2);
        sprintf(out_name, "Duracao_%dmin-%dmin", (int)((band_idx * cfg->val2)/60), (int)(((band_idx+1) * cfg->val2)/60));
    } else {
        sprintf(out_name, "Filtrados_Duracao");
    }
}