#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "ogz_types.h"
#include "org_ext.h"
#include "org_size.h"
#include "org_dur.h"

void print_help() {
    printf("Uso: ogz <pasta> [opcoes]\n\n");
    printf("Orquestrador de arquivos. Filtros podem ser combinados.\n\n");
    printf("Opcoes:\n");
    printf("  -x [ext]         Organiza por extensao (ex: -x mp3, ou apenas -x para todas)\n");
    printf("  -s [regra]       Organiza por tamanho (M para MB, G para GB).\n");
    printf("                     >5M, <10M, 5M-15M (faixa fixa), band:2M (pastas de 2 em 2MB)\n");
    printf("  -d [regra]       Organiza por duracao em segundos. Requer ffprobe.\n");
    printf("                     >300, <60, 60-120, band:300\n");
    printf("  -h, --help       Exibe esta ajuda\n");
}

void create_dir_if_not_exists(const char *dir) {
    struct stat st = {0};
    if (stat(dir, &st) == -1) {
        mkdir(dir, 0755);
    }
}

// Uma implementação simples de parser manual
void parse_args(int argc, char **argv, Config *cfg) {
    memset(cfg, 0, sizeof(Config));
    if (argc < 2) return;
    
    strcpy(cfg->target_dir, argv[1]);
    
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-x") == 0) {
            cfg->ext.active = true;
            if (i + 1 < argc && argv[i+1][0] != '-') {
                cfg->ext.mode = MODE_SPECIFIC;
                strcpy(cfg->ext.specific_ext, argv[++i]);
            } else {
                cfg->ext.mode = MODE_ALL;
            }
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            cfg->size.active = true;
            char *rule = argv[++i];
            long long multiplier = 1024 * 1024; // Padrão MB para este exemplo simples
            if (rule[0] == '>') {
                cfg->size.mode = MODE_GREATER;
                cfg->size.val1 = atoi(rule + 1) * multiplier;
            } else if (strncmp(rule, "band:", 5) == 0) {
                cfg->size.mode = MODE_BANDS;
                cfg->size.val2 = atoi(rule + 5) * multiplier;
            }
            // (Lógica adicional para < e faixas fixas iria aqui)
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            cfg->dur.active = true;
            char *rule = argv[++i];
            if (rule[0] == '>') {
                cfg->dur.mode = MODE_GREATER;
                cfg->dur.val1 = atof(rule + 1);
            } else if (strncmp(rule, "band:", 5) == 0) {
                cfg->dur.mode = MODE_BANDS;
                cfg->dur.val2 = atof(rule + 5);
            }
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_help();
        return 0;
    }

    Config cfg;
    parse_args(argc, argv, &cfg);

    DIR *d = opendir(cfg.target_dir);
    if (!d) {
        perror("Erro ao abrir diretorio");
        return 1;
    }

    struct dirent *dir;
    char filepath[PATH_MAX * 2];
    char dest_folder_name[256 * 2];
    char dest_path[PATH_MAX * 2];
    char dest_file[PATH_MAX * 2];

    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type != DT_REG) continue; // Pular diretórios

        snprintf(filepath, sizeof(filepath), "%s/%s", cfg.target_dir, dir->d_name);
        
        long long size_val = 0;
        double dur_val = 0.0;

        // Filtros em Cascata (Acumulativo)
        if (!match_ext(dir->d_name, &cfg.ext)) continue;
        if (!match_size(filepath, &cfg.size, &size_val)) continue;
        if (!match_dur(filepath, &cfg.dur, &dur_val)) continue;

        // Define a pasta de destino com base no organizador prioritário
        strcpy(dest_folder_name, "Organizado"); // Fallback
        
        if (cfg.ext.active && cfg.ext.mode == MODE_ALL) {
            get_ext_foldername(dir->d_name, &cfg.ext, dest_folder_name);
        } else if (cfg.size.active && cfg.size.mode == MODE_BANDS) {
            get_size_foldername(size_val, &cfg.size, dest_folder_name);
        } else if (cfg.dur.active && cfg.dur.mode == MODE_BANDS) {
            get_dur_foldername(dur_val, &cfg.dur, dest_folder_name);
        }

        // Monta os caminhos
        snprintf(dest_path, sizeof(dest_path), "%s/%s", cfg.target_dir, dest_folder_name);
        snprintf(dest_file, sizeof(dest_file), "%s/%s", dest_path, dir->d_name);

        create_dir_if_not_exists(dest_path);
        
        if (rename(filepath, dest_file) == 0) {
            printf("Movido: %s -> %s/\n", dir->d_name, dest_folder_name);
        } else {
            perror("Erro ao mover arquivo");
        }
    }

    closedir(d);
    printf("Organizacao concluida.\n");
    return 0;
}