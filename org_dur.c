#include "org_dur.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static void shell_escape(
    const char *input,
    char *output,
    size_t output_size
) {
    size_t pos = 0;

    if (!input || !output || output_size == 0) {
        return;
    }

    if (pos < output_size - 1) {
        output[pos++] = '\'';
    }


    for (size_t i = 0;
         input[i] != '\0' && pos < output_size - 1;
         i++) {

        if (input[i] == '\'') {

            const char *replacement = "'\\''";

            for (size_t j = 0;
                 replacement[j] != '\0' &&
                 pos < output_size - 1;
                 j++) {

                output[pos++] = replacement[j];
            }

        } else {

            output[pos++] = input[i];
        }
    }


    if (pos < output_size - 1) {
        output[pos++] = '\'';
    }

    output[pos] = '\0';
}


static double get_duration_via_ffprobe(
    const char *filepath
) {
    char escaped_path[2048];

    shell_escape(
        filepath,
        escaped_path,
        sizeof(escaped_path)
    );


    char cmd[4096];

    snprintf(
        cmd,
        sizeof(cmd),
        "ffprobe "
        "-v error "
        "-show_entries format=duration "
        "-of default=noprint_wrappers=1:nokey=1 "
        "%s "
        "2>/dev/null",
        escaped_path
    );


    printf(
        "[DUR] Executando ffprobe para: '%s'\n",
        filepath
    );

    printf(
        "[DUR] Comando: %s\n",
        cmd
    );


    FILE *fp = popen(cmd, "r");

    if (!fp) {

        perror("[DUR] ERRO: popen");

        printf(
            "[DUR] Nao foi possivel executar ffprobe.\n"
        );

        return -1.0;
    }


    char result[128] = {0};


    if (fgets(
            result,
            sizeof(result),
            fp
        ) != NULL) {

        int status = pclose(fp);

        printf(
            "[DUR] Saida ffprobe: '%s'",
            result
        );

        printf(
            "[DUR] Status ffprobe: %d\n",
            status
        );


        char *endptr = NULL;

        double duration = strtod(
            result,
            &endptr
        );


        if (endptr == result) {

            printf(
                "[DUR] ERRO: ffprobe retornou valor invalido.\n"
            );

            return -1.0;
        }


        printf(
            "[DUR] Duracao encontrada: %.3f segundos\n",
            duration
        );

        return duration;
    }


    int status = pclose(fp);

    printf(
        "[DUR] ffprobe nao retornou duracao.\n"
    );

    printf(
        "[DUR] Status ffprobe: %d\n",
        status
    );

    return -1.0;
}


bool match_dur(
    const char *filepath,
    const DurConfig *cfg,
    double *out_dur
) {
    printf(
        "[DUR] Verificando: '%s'\n",
        filepath
    );


    if (!cfg->active) {

        printf(
            "[DUR] Filtro desativado -> MATCH\n"
        );

        return true;
    }


    printf(
        "[DUR] Filtro ativo\n"
    );

    printf(
        "[DUR] Mode: %d\n",
        cfg->mode
    );

    printf(
        "[DUR] val1: %.3f segundos\n",
        cfg->val1
    );

    printf(
        "[DUR] val2: %.3f segundos\n",
        cfg->val2
    );


    *out_dur = get_duration_via_ffprobe(filepath);


    if (*out_dur < 0) {

        printf(
            "[DUR] Falha ao obter duracao -> NO MATCH\n"
        );

        return false;
    }


    switch (cfg->mode) {

        case MODE_LESS:

            printf(
                "[DUR] Regra: MENOR QUE %.3f segundos\n",
                cfg->val1
            );

            if (*out_dur < cfg->val1) {

                printf(
                    "[DUR] %.3f < %.3f -> MATCH\n",
                    *out_dur,
                    cfg->val1
                );

                return true;
            }

            printf(
                "[DUR] %.3f >= %.3f -> NO MATCH\n",
                *out_dur,
                cfg->val1
            );

            return false;


        case MODE_GREATER:

            printf(
                "[DUR] Regra: MAIOR QUE %.3f segundos\n",
                cfg->val1
            );

            if (*out_dur > cfg->val1) {

                printf(
                    "[DUR] %.3f > %.3f -> MATCH\n",
                    *out_dur,
                    cfg->val1
                );

                return true;
            }

            printf(
                "[DUR] %.3f <= %.3f -> NO MATCH\n",
                *out_dur,
                cfg->val1
            );

            return false;


        case MODE_RANGE:

            printf(
                "[DUR] Regra: FAIXA [%.3f, %.3f]\n",
                cfg->val1,
                cfg->val2
            );

            if (*out_dur >= cfg->val1 &&
                *out_dur <= cfg->val2) {

                printf(
                    "[DUR] %.3f esta dentro da faixa -> MATCH\n",
                    *out_dur
                );

                return true;
            }

            printf(
                "[DUR] %.3f esta fora da faixa -> NO MATCH\n",
                *out_dur
            );

            return false;


        case MODE_BANDS:

            printf(
                "[DUR] Regra: BANDA de %.3f segundos\n",
                cfg->val2
            );

            /*
             * Assim como tamanho, band não filtra.
             */
            printf(
                "[DUR] BAND nao filtra arquivos -> MATCH\n"
            );

            return true;


        default:

            printf(
                "[DUR] Modo desconhecido (%d) -> MATCH\n",
                cfg->mode
            );

            return true;
    }
}


void get_dur_foldername(
    double dur,
    const DurConfig *cfg,
    char *out_name
) {
    if (!out_name) {
        printf(
            "[DUR] ERRO: out_name == NULL\n"
        );

        return;
    }


    if (cfg->mode == MODE_BANDS) {

        if (cfg->val2 <= 0) {

            printf(
                "[DUR] ERRO: tamanho da banda invalido: %.3f\n",
                cfg->val2
            );

            snprintf(
                out_name,
                256,
                "Erro_Duracao"
            );

            return;
        }


        int band_idx =
            (int)(dur / cfg->val2);


        double start_seconds =
            band_idx * cfg->val2;

        double end_seconds =
            (band_idx + 1) * cfg->val2;


        int start_minutes =
            (int)(start_seconds / 60.0);

        int end_minutes =
            (int)(end_seconds / 60.0);


        snprintf(
            out_name,
            256,
            "Duracao_%dmin-%dmin",
            start_minutes,
            end_minutes
        );


        printf(
            "[DUR] Banda calculada:\n"
            "[DUR]   indice : %d\n"
            "[DUR]   inicio : %.3f segundos\n"
            "[DUR]   fim    : %.3f segundos\n"
            "[DUR]   pasta  : '%s'\n",
            band_idx,
            start_seconds,
            end_seconds,
            out_name
        );

        return;
    }


    if (cfg->mode == MODE_RANGE) {

        int start_minutes =
            (int)(cfg->val1 / 60.0);

        int end_minutes =
            (int)(cfg->val2 / 60.0);


        snprintf(
            out_name,
            256,
            "Faixa_%dmin-%dmin",
            start_minutes,
            end_minutes
        );


        printf(
            "[DUR] Pasta para faixa: '%s'\n",
            out_name
        );

        return;
    }


    snprintf(
        out_name,
        256,
        "Filtrados_Duracao"
    );


    printf(
        "[DUR] Pasta padrao: '%s'\n",
        out_name
    );
}