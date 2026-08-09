#ifndef ORG_TYPES_H
#define ORG_TYPES_H

#include <stdbool.h>
#include <limits.h>

typedef enum { MODE_NONE, MODE_ALL, MODE_SPECIFIC, MODE_GREATER, MODE_LESS, MODE_RANGE, MODE_BANDS } OrgMode;

typedef struct {
    bool active;
    OrgMode mode;
    char specific_ext[32];
} ExtConfig;

typedef struct {
    bool active;
    OrgMode mode;
    long long val1; // Em bytes
    long long val2; // Em bytes (usado para RANGE ou tamanho da BAND)
} SizeConfig;

typedef struct {
    bool active;
    OrgMode mode;
    double val1; // Em segundos
    double val2; // Em segundos
} DurConfig;

typedef struct {
    char target_dir[PATH_MAX];
    ExtConfig ext;
    SizeConfig size;
    DurConfig dur;
} Config;

#endif

