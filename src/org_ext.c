#include "../include/org_ext.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>


static const char *get_extension(const char *filename) {
    
    const char *dot = strrchr(filename, '.');

    if (!dot || dot == filename) {
        return "";
    }

    return dot + 1;
}


static void normalize_extension(const char *input, char *output, size_t output_size) {
    
    size_t i;

    if (!input || !output || output_size == 0) {
        return;
    }

    for (i = 0; i < output_size - 1 && input[i] != '\0'; i++) {
        output[i] = (char)tolower((unsigned char)input[i]);
    }

    output[i] = '\0';
}


bool match_ext(const char *filename, const ExtConfig *cfg) {
    
    if (!cfg->active) {
        return true;
    }

    if (cfg->mode == MODE_ALL) {
        return true;
    }

    const char *ext = get_extension(filename);

    char actual[64];
    char expected[64];

    normalize_extension(ext, actual, sizeof(actual));
    normalize_extension(cfg->specific_ext, expected, sizeof(expected));

    if (strcmp(actual, expected) == 0) {
        return true;
    }

    return false;
}


void get_ext_foldername(const char *filename, const ExtConfig *cfg, char *out_name) {
    
    if (!out_name) {
        printf("[EXT] ERRO: out_name == NULL\n");
        return;
    }


    if (cfg->mode == MODE_SPECIFIC) {

        snprintf(
            out_name,
            256,
            "%s",
            cfg->specific_ext
        );

        return;
    }

    const char *ext = get_extension(filename);

    if (ext[0] != '\0') {

        snprintf(
            out_name,
            256,
            "%s",
            ext
        );

    } else {

        snprintf(
            out_name,
            256,
            "sem_extensao"
        );
    }


}