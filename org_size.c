#include "org_size.h"

#include <stdio.h>
#include <sys/stat.h>
#include <string.h>


#define BYTES_KB 1024LL
#define BYTES_MB (1024LL * 1024LL)
#define BYTES_GB (1024LL * 1024LL * 1024LL)


bool match_size(
    const char *filepath,
    const SizeConfig *cfg,
    long long *out_size
) {
    struct stat st;

    printf("[SIZE] Verificando arquivo: '%s'\n", filepath);

    if (stat(filepath, &st) != 0) {
        perror("[SIZE] ERRO: stat");

        printf("[SIZE] Falha ao obter tamanho de '%s'\n",
               filepath);

        return false;
    }

    *out_size = (long long) st.st_size;

    printf("[SIZE] Tamanho real: %lld bytes\n",
           *out_size);

    if (!cfg->active) {
        printf("[SIZE] Filtro desativado -> MATCH\n");
        return true;
    }

    printf("[SIZE] Filtro ativo\n");
    printf("[SIZE] Mode: %d\n", cfg->mode);
    printf("[SIZE] val1: %lld bytes\n", cfg->val1);
    printf("[SIZE] val2: %lld bytes\n", cfg->val2);


    switch (cfg->mode) {

        case MODE_LESS:

            printf("[SIZE] Regra: MENOR QUE %lld bytes\n",
                   cfg->val1);

            if (*out_size < cfg->val1) {
                printf("[SIZE] %lld < %lld -> MATCH\n",
                       *out_size,
                       cfg->val1);

                return true;
            }

            printf("[SIZE] %lld >= %lld -> NO MATCH\n",
                   *out_size,
                   cfg->val1);

            return false;


        case MODE_GREATER:

            printf("[SIZE] Regra: MAIOR QUE %lld bytes\n",
                   cfg->val1);

            if (*out_size > cfg->val1) {
                printf("[SIZE] %lld > %lld -> MATCH\n",
                       *out_size,
                       cfg->val1);

                return true;
            }

            printf("[SIZE] %lld <= %lld -> NO MATCH\n",
                   *out_size,
                   cfg->val1);

            return false;


        case MODE_RANGE:

            printf(
                "[SIZE] Regra: FAIXA [%lld, %lld] bytes\n",
                cfg->val1,
                cfg->val2
            );

            if (*out_size >= cfg->val1 &&
                *out_size <= cfg->val2) {

                printf(
                    "[SIZE] %lld esta dentro da faixa -> MATCH\n",
                    *out_size
                );

                return true;
            }

            printf(
                "[SIZE] %lld esta fora da faixa -> NO MATCH\n",
                *out_size
            );

            return false;


        case MODE_BANDS:

            printf(
                "[SIZE] Regra: BANDA de %lld bytes\n",
                cfg->val2
            );

            printf(
                "[SIZE] BAND nao filtra arquivos -> MATCH\n"
            );

            return true;


        default:

            printf(
                "[SIZE] Modo desconhecido (%d) -> MATCH\n",
                cfg->mode
            );

            return true;
    }
}


void get_size_foldername(
    long long file_size,
    const SizeConfig *cfg,
    char *out_name
) {
    if (!out_name) {
        printf("[SIZE] ERRO: out_name == NULL\n");
        return;
    }

    printf(
        "[SIZE] Calculando pasta para tamanho: %lld bytes\n",
        file_size
    );


    if (cfg->mode == MODE_BANDS) {

        if (cfg->val2 <= 0) {
            printf(
                "[SIZE] ERRO: tamanho da banda invalido: %lld\n",
                cfg->val2
            );

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

        printf(
            "[SIZE] Banda calculada:\n"
            "[SIZE]   indice : %lld\n"
            "[SIZE]   inicio : %lld bytes\n"
            "[SIZE]   fim    : %lld bytes\n"
            "[SIZE]   pasta  : '%s'\n",
            band_idx,
            band_start,
            band_end,
            out_name
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

        printf(
            "[SIZE] Pasta para faixa: '%s'\n",
            out_name
        );

        return;
    }


    snprintf(
        out_name,
        256,
        "Filtrados_Tamanho"
    );

    printf(
        "[SIZE] Pasta padrao: '%s'\n",
        out_name
    );
}
