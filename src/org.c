#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <ctype.h>

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../include/org_types.h"
#include "../include/org_ext.h"
#include "../include/org_size.h"
#include "../include/org_dur.h"


#define BYTES_KB 1024LL
#define BYTES_MB (1024LL * 1024LL)
#define BYTES_GB (1024LL * 1024LL * 1024LL)

extern unsigned char help_txt[];
extern unsigned int help_txt_len;

void print_help(void) {
    fwrite(help_txt, 1, help_txt_len, stdout);
    printf("\n");
}

void create_dir_if_not_exists(const char *dir) {
    
    struct stat st = {0};

    if (stat(dir, &st) == -1) {

        if (mkdir(dir, 0755) != 0) {
            perror("[DIR] ERRO: mkdir");
            return;
        }    

    } else {
        if (!S_ISDIR(st.st_mode)) {    
            fprintf(stderr, "[DIR] ERRO: caminho '%s' existe mas nao e diretorio.\n", dir);
        }
    }
}

static void trim_spaces(char *str) {
    
    if (!str) {
        return;
    }

    char *start = str;

    while (isspace((unsigned char)*start)) {
        start++;
    }

    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }

    size_t len = strlen(str);

    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[len - 1] = '\0';
        len--;
    }
}

static bool parse_size_value(const char *text, long long *out_bytes) {
    
    if (!text || !out_bytes) {
        return false;
    }

    if (*text == '\0') {
        printf("[PARSER][SIZE] ERRO: valor vazio.\n", text);
        return false;
    }

    errno = 0;

    char *endptr = NULL;

    long long value = strtoll(text, &endptr, 10);

    if (endptr == text) {
        printf("[PARSER][SIZE] ERRO: numero invalido.\n", text);
        return false;
    }


    if (errno == ERANGE) {
        printf("[PARSER][SIZE] ERRO: numero fora do limite.\n");
        return false;
    }

    char unit = '\0';

    if (*endptr != '\0') {
        
        unit = (char)toupper((unsigned char)*endptr);

        endptr++;

        if (unit != 'K' && unit != 'M' && unit != 'G') {
            printf("[PARSER][SIZE] ERRO: unidade invalida '%c'.\n", unit);
            return false;
        }
    }

    if (*endptr != '\0') {
        printf("[PARSER][SIZE] ERRO: caracteres invalidos apos unidade.\n");
        return false;
    }

    long long multiplier = 1;

    switch (unit) {

        case 'K':
            multiplier = BYTES_KB;
            break;

        case 'M':
            multiplier = BYTES_MB;
            break;

        case 'G':
            multiplier = BYTES_GB;
            break;

        default:
            multiplier = 1;
            break;
    }

    if (value < 0) {
        printf("[PARSER][SIZE] ERRO: tamanho negativo.\n");
        return false;
    }

    if (value > LLONG_MAX / multiplier) {
        printf("[PARSER][SIZE] ERRO: overflow.\n");
        return false;
    }

    *out_bytes = value * multiplier;

    

    return true;
}

static bool parse_size_rule(const char *rule, SizeConfig *cfg) {
    
    if (!rule || !cfg) {
        return false;
    }
    
    char buffer[256];

    snprintf(
        buffer,
        sizeof(buffer),
        "%s",
        rule
    );

    trim_spaces(buffer);

    if (buffer[0] == '\0') {
        printf("[PARSER][SIZE] ERRO: regra vazia.\n");
        return false;
    }

    // BAND
    if (strncmp(buffer,"band:",5) == 0) {

        const char *value = buffer + 5;

        long long band_size;

        if (!parse_size_value(value, &band_size)) {
            printf("[PARSER][SIZE] ERRO: banda invalida.\n");
            return false;
        }

        if (band_size <= 0) {
            printf("[PARSER][SIZE] ERRO: banda deve ser > 0.\n");
            return false;
        }

        cfg->mode = MODE_BANDS;
        cfg->val1 = 0;
        cfg->val2 = band_size;

        return true;
    }


    // GREATER
    if (buffer[0] == '>') {

        long long value;

        if (!parse_size_value(buffer + 1, &value)) {
            return false;
        }

        cfg->mode = MODE_GREATER;
        cfg->val1 = value;
        cfg->val2 = 0;

        return true;
    }


    // LESS
    if (buffer[0] == '<') {

        long long value;

        if (!parse_size_value(buffer + 1, &value)) {
            return false;
        }

        cfg->mode = MODE_LESS;
        cfg->val1 = value;
        cfg->val2 = 0;

        return true;
    }


    // RANGE
    char *dash = strchr(buffer, '-');

    if (dash) {

        // Não aceita mais de um '-'.
        if (strchr(dash + 1, '-') ) {
            printf("[PARSER][SIZE] ERRO: faixa invalida.\n");
            return false;
        }

        *dash = '\0';

        const char *first = buffer;
        const char *second = dash + 1;

        long long val1;
        long long val2;

        if (!parse_size_value(first, &val1)) {
            return false;
        }

        if (!parse_size_value(second, &val2)) {
            return false;
        }

        if (val1 > val2) {
            printf("[PARSER][SIZE] ERRO: inicio da faixa maior que fim.\n");
            return false;
        }

        cfg->mode = MODE_RANGE;
        cfg->val1 = val1;
        cfg->val2 = val2;
        
        return true;
    }

    printf("[PARSER][SIZE] ERRO: regra desconhecida '%s'.\n", buffer);

    return false;
}

// PAREI A HIGIENIZAÇÃO AQUI ================================
static bool parse_duration_value(
    const char *text,
    double *out_seconds
) {
    if (!text || !out_seconds) {
        return false;
    }


    


    errno = 0;

    char *endptr = NULL;

    double value =
        strtod(
            text,
            &endptr
        );


    if (endptr == text) {

        printf(
            "[PARSER][DUR] ERRO: numero invalido.\n"
        );

        return false;
    }


    if (errno == ERANGE) {

        printf(
            "[PARSER][DUR] ERRO: numero fora do limite.\n"
        );

        return false;
    }


    if (*endptr != '\0') {

        printf(
            "[PARSER][DUR] ERRO: caracteres invalidos: '%s'\n",
            endptr
        );

        return false;
    }


    if (value < 0) {

        printf(
            "[PARSER][DUR] ERRO: duracao negativa.\n"
        );

        return false;
    }


    *out_seconds = value;


    


    return true;
}


/* =========================================================
 * PARSER DE REGRA DE DURACAO
 *
 * >300
 * <60
 * 60-120
 * band:300
 * ========================================================= */

static bool parse_duration_rule(
    const char *rule,
    DurConfig *cfg
) {
    if (!rule || !cfg) {
        return false;
    }


    

    


    char buffer[256];

    snprintf(
        buffer,
        sizeof(buffer),
        "%s",
        rule
    );

    trim_spaces(buffer);


    if (buffer[0] == '\0') {

        printf(
            "[PARSER][DUR] ERRO: regra vazia.\n"
        );

        return false;
    }


    /*
     * BAND
     */
    if (strncmp(
            buffer,
            "band:",
            5
        ) == 0) {

        double band;


        if (!parse_duration_value(
                buffer + 5,
                &band
            )) {

            return false;
        }


        if (band <= 0) {

            printf(
                "[PARSER][DUR] ERRO: banda deve ser > 0.\n"
            );

            return false;
        }


        cfg->mode = MODE_BANDS;
        cfg->val1 = 0;
        cfg->val2 = band;


        


        return true;
    }


    /*
     * GREATER
     */
    if (buffer[0] == '>') {

        double value;


        if (!parse_duration_value(
                buffer + 1,
                &value
            )) {

            return false;
        }


        cfg->mode = MODE_GREATER;
        cfg->val1 = value;
        cfg->val2 = 0;


        


        return true;
    }


    /*
     * LESS
     */
    if (buffer[0] == '<') {

        double value;


        if (!parse_duration_value(
                buffer + 1,
                &value
            )) {

            return false;
        }


        cfg->mode = MODE_LESS;
        cfg->val1 = value;
        cfg->val2 = 0;


        


        return true;
    }


    /*
     * RANGE
     */
    char *dash =
        strchr(
            buffer,
            '-'
        );


    if (dash) {

        if (strchr(
                dash + 1,
                '-'
            )) {

            printf(
                "[PARSER][DUR] ERRO: faixa invalida.\n"
            );

            return false;
        }


        *dash = '\0';


        double val1;
        double val2;


        if (!parse_duration_value(
                buffer,
                &val1
            )) {

            return false;
        }


        if (!parse_duration_value(
                dash + 1,
                &val2
            )) {

            return false;
        }


        if (val1 > val2) {

            printf(
                "[PARSER][DUR] ERRO: inicio maior que fim.\n"
            );

            return false;
        }


        cfg->mode = MODE_RANGE;
        cfg->val1 = val1;
        cfg->val2 = val2;


        


        return true;
    }


    printf(
        "[PARSER][DUR] ERRO: regra desconhecida '%s'.\n",
        buffer
    );


    return false;
}


/* =========================================================
 * PARSER PRINCIPAL
 * ========================================================= */

bool parse_args(
    int argc,
    char **argv,
    Config *cfg
) {
    memset(
        cfg,
        0,
        sizeof(Config)
    );


    printf(
        "\n========== PARSER ==========\n"
    );

    


    for (int i = 0; i < argc; i++) {

        
    }


    if (argc < 2) {

        printf(
            "[PARSER] ERRO: diretorio nao informado.\n"
        );

        return false;
    }


    snprintf(
        cfg->target_dir,
        sizeof(cfg->target_dir),
        "%s",
        argv[1]
    );


    


    for (int i = 2; i < argc; i++) {

        


        /*
         * EXTENSAO
         */
        if (strcmp(
                argv[i],
                "-x"
            ) == 0) {

            cfg->ext.active = true;


            if (i + 1 < argc &&
                argv[i + 1][0] != '-') {

                cfg->ext.mode =
                    MODE_SPECIFIC;


                snprintf(
                    cfg->ext.specific_ext,
                    sizeof(cfg->ext.specific_ext),
                    "%s",
                    argv[++i]
                );


                /*
                 * Remove ponto inicial.
                 *
                 * -x .mp3
                 *
                 * vira:
                 *
                 * mp3
                 */

                if (cfg->ext.specific_ext[0] == '.') {

                    memmove(
                        cfg->ext.specific_ext,
                        cfg->ext.specific_ext + 1,
                        strlen(
                            cfg->ext.specific_ext
                        )
                    );
                }


                

            } else {

                cfg->ext.mode =
                    MODE_ALL;

                
            }


            continue;
        }


        /*
         * TAMANHO
         */
        if (strcmp(
                argv[i],
                "-s"
            ) == 0) {

            if (i + 1 >= argc) {

                printf(
                    "[PARSER] ERRO: -s requer uma regra.\n"
                );

                return false;
            }


            const char *rule =
                argv[++i];


            /*
             * Se for outra opcao, não é uma regra.
             */
            if (rule[0] == '-') {

                printf(
                    "[PARSER] ERRO: regra de tamanho ausente.\n"
                );

                return false;
            }


            cfg->size.active = true;


            if (!parse_size_rule(
                    rule,
                    &cfg->size
                )) {

                printf(
                    "[PARSER] ERRO: regra de tamanho invalida: '%s'\n",
                    rule
                );

                return false;
            }


            continue;
        }


        /*
         * DURACAO
         */
        if (strcmp(
                argv[i],
                "-d"
            ) == 0) {

            if (i + 1 >= argc) {

                printf(
                    "[PARSER] ERRO: -d requer uma regra.\n"
                );

                return false;
            }


            const char *rule =
                argv[++i];


            if (rule[0] == '-') {

                printf(
                    "[PARSER] ERRO: regra de duracao ausente.\n"
                );

                return false;
            }


            cfg->dur.active = true;


            if (!parse_duration_rule(
                    rule,
                    &cfg->dur
                )) {

                printf(
                    "[PARSER] ERRO: regra de duracao invalida: '%s'\n",
                    rule
                );

                return false;
            }


            continue;
        }


        /*
         * Argumento desconhecido
         */
        printf(
            "[PARSER] ERRO: opcao desconhecida: '%s'\n",
            argv[i]
        );

        return false;
    }


    /*
     * CONFIGURACAO FINAL
     */

    printf(
        "\n========== CONFIGURACAO FINAL ==========\n"
    );


    


    

    

    


    

    

    

    


    

    

    

    


    printf(
        "========================================\n\n"
    );


    return true;
}


/* =========================================================
 * MAIN
 * ========================================================= */

int main(
    int argc,
    char **argv
) {
    printf(
        "\n"
        "========================================\n"
        "       ORGANIZADOR DE ARQUIVOS\n"
        "========================================\n"
    );


    if (argc < 2 ||
        strcmp(argv[1], "-h") == 0 ||
        strcmp(argv[1], "--help") == 0) {

        print_help();

        return 0;
    }


    Config cfg;


    if (!parse_args(
            argc,
            argv,
            &cfg
        )) {

        printf(
            "\n[ERRO] Argumentos invalidos.\n"
            "Use -h para ver a ajuda.\n"
        );

        return 1;
    }


    /*
     * Abre diretorio
     */

    printf(
        "\n========== DIRETORIO ==========\n"
    );

    


    DIR *d =
        opendir(
            cfg.target_dir
        );


    if (!d) {

        perror(
            "[MAIN] ERRO: opendir"
        );

        return 1;
    }


    struct dirent *dir;


    char filepath[PATH_MAX * 2];

    char dest_folder_name[256];

    char dest_path[PATH_MAX * 2];

    char dest_file[PATH_MAX * 2 + 256];


    int total_files = 0;
    int processed_files = 0;
    int skipped_files = 0;
    int moved_files = 0;


    /*
     * Leitura
     */

    while (
        (dir = readdir(d)) != NULL
    ) {

        printf(
            "\n----------------------------------------\n"
        );

        


        if (strcmp(
                dir->d_name,
                "."
            ) == 0 ||
            strcmp(
                dir->d_name,
                ".."
            ) == 0) {

            

            continue;
        }


        if (dir->d_type != DT_REG) {

            

            continue;
        }


        total_files++;


        snprintf(
            filepath,
            sizeof(filepath),
            "%s/%s",
            cfg.target_dir,
            dir->d_name
        );


        


        long long size_val = 0;
        double dur_val = 0.0;


        /*
         * EXTENSAO
         */

        


        if (!match_ext(
                dir->d_name,
                &cfg.ext
            )) {

            

            skipped_files++;

            continue;
        }


        /*
         * TAMANHO
         */

        


        if (!match_size(
                filepath,
                &cfg.size,
                &size_val
            )) {

            

            skipped_files++;

            continue;
        }


        /*
         * DURACAO
         */

        


        if (!match_dur(
                filepath,
                &cfg.dur,
                &dur_val
            )) {

            

            skipped_files++;

            continue;
        }


        processed_files++;


        /*
         * ==================================================
         * DESTINO
         * ==================================================
         *
         * Prioridade:
         *
         * 1. Extensao
         * 2. Tamanho
         * 3. Duracao
         * 4. Organizado
         *
         * Os filtros continuam sendo acumulativos.
         *
         * A prioridade aqui define somente a pasta.
         */

        strcpy(
            dest_folder_name,
            "Organizado"
        );


        if (cfg.ext.active) {

            

            get_ext_foldername(
                dir->d_name,
                &cfg.ext,
                dest_folder_name
            );

        } else if (
            cfg.size.active &&
            (
                cfg.size.mode == MODE_BANDS ||
                cfg.size.mode == MODE_RANGE
            )
        ) {

            

            get_size_foldername(
                size_val,
                &cfg.size,
                dest_folder_name
            );

        } else if (
            cfg.dur.active &&
            (
                cfg.dur.mode == MODE_BANDS ||
                cfg.dur.mode == MODE_RANGE
            )
        ) {

            

            get_dur_foldername(
                dur_val,
                &cfg.dur,
                dest_folder_name
            );

        } else {

            
        }


        


        /*
         * Caminhos
         */

        snprintf(
            dest_path,
            sizeof(dest_path),
            "%s/%s",
            cfg.target_dir,
            dest_folder_name
        );


        snprintf(
            dest_file,
            sizeof(dest_file),
            "%s/%s",
            dest_path,
            dir->d_name
        );


        

        


        /*
         * Diretorio
         */

        create_dir_if_not_exists(
            dest_path
        );


        /*
         * Move
         */

        


        if (rename(
                filepath,
                dest_file
            ) == 0) {

            printf(
                "[OK] Movido: %s -> %s/\n",
                dir->d_name,
                dest_folder_name
            );

            moved_files++;

        } else {

            perror(
                "[ERRO] rename"
            );

            printf(
                "[ERRO] Origem : '%s'\n",
                filepath
            );

            printf(
                "[ERRO] Destino: '%s'\n",
                dest_file
            );
        }
    }


    closedir(d);


    /*
     * Resumo
     */

    printf(
        "\n"
        "========================================\n"
        "           RESUMO DA EXECUCAO\n"
        "========================================\n"
    );


    printf(
        "Arquivos encontrados : %d\n",
        total_files
    );

    printf(
        "Arquivos processados : %d\n",
        processed_files
    );

    printf(
        "Arquivos ignorados   : %d\n",
        skipped_files
    );

    printf(
        "Arquivos movidos     : %d\n",
        moved_files
    );


    printf(
        "========================================\n"
        "Organizacao concluida.\n"
    );


    return 0;
}
