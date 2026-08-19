#include "../include/org_dur.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static void shell_escape(const char *input, char *output, size_t output_size) {
    
    size_t pos = 0;

    if (!input || !output || output_size == 0) {
        return;
    }

    if (pos < output_size - 1) {
        output[pos++] = '\'';
    }


    for (size_t i = 0; input[i] != '\0' && pos < output_size - 1; i++) {

        if (input[i] == '\'') {

            const char *replacement = "'\\''";

            for (size_t j = 0; replacement[j] != '\0' && pos < output_size - 1; j++) {
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


static double get_duration_via_ffprobe(const char *filepath) {
    
    char escaped_path[2048];

    shell_escape(filepath, escaped_path, sizeof(escaped_path));

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

    FILE *fp = popen(cmd, "r");

    if (!fp) {
        perror("[DUR] ERRO: popen");
        printf("[DUR] Nao foi possivel executar ffprobe.\n");
        return -1.0;
    }

    char result[128] = {0};

    if (fgets(result, sizeof(result), fp) != NULL) {
        
        int status = pclose(fp);

        char *endptr = NULL;

        double duration = strtod(result, &endptr);

        if (endptr == result) {
            printf("[DUR] ERRO: ffprobe retornou valor invalido.\n");
            return -1.0;
        }

        return duration;
    }

    int status = pclose(fp);

    printf("[DUR] ffprobe nao retornou duracao.\n");
    printf("[DUR] Status ffprobe: %d\n", status);

    return -1.0;
}


bool match_dur(const char *filepath, const DurConfig *cfg, double *out_dur) {

    if (!cfg->active) {
        return true;
    }

    *out_dur = get_duration_via_ffprobe(filepath);

    if (*out_dur < 0) {
        return false;
    }

    switch (cfg->mode) {

        case MODE_LESS:
            if (*out_dur < cfg->val1) {
                return true;
            }
            return false;

        case MODE_GREATER:
            if (*out_dur > cfg->val1) {
                return true;
            }
            return false;

        case MODE_RANGE:
            if (*out_dur >= cfg->val1 && *out_dur <= cfg->val2) {
                return true;
            }
            return false;

        case MODE_BANDS:
            // Assim como tamanho, band não filtra.
            return true;

        default:
            printf("[DUR] Modo desconhecido (%d) -> MATCH\n", cfg->mode);
            return true;
    }
}


void get_dur_foldername(double dur, const DurConfig *cfg, char *out_name) {
    
    if (!out_name) {
        printf("[DUR] ERRO: out_name == NULL\n");
        return;
    }

    if (cfg->mode == MODE_BANDS) {

        if (cfg->val2 <= 0) {
            printf("[DUR] ERRO: tamanho da banda invalido: %.3f\n", cfg->val2);

            snprintf(out_name, 256, "Erro_Duracao");

            return;
        }

        int band_idx = (int)(dur / cfg->val2);
        double start_seconds = band_idx * cfg->val2;
        double end_seconds = (band_idx + 1) * cfg->val2;
        int start_minutes = (int)(start_seconds / 60.0);
        int end_minutes = (int)(end_seconds / 60.0);

        snprintf(
            out_name,
            256,
            "Duracao_%dmin-%dmin",
            start_minutes,
            end_minutes
        );
        return;
    }

    if (cfg->mode == MODE_RANGE) {

        int start_minutes = (int)(cfg->val1 / 60.0);

        int end_minutes = (int)(cfg->val2 / 60.0);

        snprintf(
            out_name,
            256,
            "Faixa_%dmin-%dmin",
            start_minutes,
            end_minutes
        );
        return;
    }

    snprintf(
        out_name,
        256,
        "Filtrados_Duracao"
    );

}