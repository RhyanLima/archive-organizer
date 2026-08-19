#include "../include/org_size.h"

#include <stdio.h>
#include <sys/stat.h>
#include <string.h>


#define BYTES_KB 1024LL
#define BYTES_MB (1024LL * 1024LL)
#define BYTES_GB (1024LL * 1024LL * 1024LL)


bool match_size(const char *filepath, const SizeConfig *cfg, long long *out_size) {
    
    struct stat st;

    if (stat(filepath, &st) != 0) {
        perror("[SIZE] ERRO: stat");
        printf("[SIZE] Falha ao obter tamanho de '%s'\n", filepath);
        return false;
    }

    *out_size = (long long) st.st_size;

    if (!cfg->active) {
        return true;
    }

    switch (cfg->mode) {

        case MODE_LESS:
            if (*out_size < cfg->val1) {
                return true;
            }
            return false;


        case MODE_GREATER:
            if (*out_size > cfg->val1) {
                return true;
            }
            return false;


        case MODE_RANGE:
            if (*out_size >= cfg->val1 &&
                *out_size <= cfg->val2) {
                return true;
            }
            return false;


        case MODE_BANDS:
            return true;

        default:
            printf("[SIZE] Modo desconhecido (%d) -> MATCH\n", cfg->mode);
            return true;
    }
}


void get_size_foldername(long long file_size, const SizeConfig *cfg, char *out_name) {
    
    if (!out_name) {
        printf("[SIZE] ERRO: out_name == NULL\n");
        return;
    }

    if (cfg->mode == MODE_BANDS) {

        if (cfg->val2 <= 0) {

            printf("[SIZE] ERRO: tamanho da banda invalido: %lld\n", cfg->val2);

            snprintf(
                out_name,
                256,
                "Erro_Tamanho"
            );

            return;
        }

        long long band_idx = file_size / cfg->val2;

        long long band_start = band_idx * cfg->val2;
        long long band_end = (band_idx + 1) * cfg->val2;
        
        long long start_mb = band_start / BYTES_MB;
        long long end_mb = band_end / BYTES_MB;

        snprintf(
            out_name,
            256,
            "%lldMB-%lldMB",
            start_mb,
            end_mb
        );

        return;
    }


    if (cfg->mode == MODE_RANGE) {

        long long start_mb = cfg->val1 / BYTES_MB;
        long long end_mb = cfg->val2 / BYTES_MB;

        snprintf(
            out_name,
            256,
            "Faixa_%lldMB-%lldMB",
            start_mb,
            end_mb
        );

        return;
    }


    snprintf(
        out_name,
        256,
        "Filtrados_Tamanho"
    );

}
