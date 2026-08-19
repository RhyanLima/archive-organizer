#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <locale.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ftw.h>
#include <limits.h>

#include <unicode/ustring.h>
#include <unicode/ucasemap.h>
#include <unicode/utypes.h>

static int dry_run = 0;

static void print_escaped(const char *str)
{
    /*
     * UTF-8 é impresso diretamente.
     * O %s não altera os bytes.
     */
    printf("%s", str);
}

/*
 * Converte UTF-8 para maiúsculas usando ICU.
 */
static char *to_upper_utf8(const char *input)
{
    UErrorCode status = U_ZERO_ERROR;

    int32_t src_len = (int32_t)strlen(input);

    /*
     * Normalmente UTF-8 maiúsculo não precisa de muito mais espaço,
     * mas algumas conversões Unicode podem expandir a string.
     *
     * Começamos com 4x o tamanho original + espaço extra.
     */
    int32_t capacity = src_len * 4 + 32;

    char *output = malloc((size_t)capacity);

    if (!output) {
        perror("malloc");
        return NULL;
    }

    UCaseMap *case_map = ucasemap_open("", 0, &status);

    if (U_FAILURE(status)) {
        fprintf(
            stderr,
            "Erro ao inicializar ICU: %s\n",
            u_errorName(status)
        );

        free(output);
        return NULL;
    }

    status = U_ZERO_ERROR;

    int32_t result = ucasemap_utf8ToUpper(
        case_map,
        output,
        capacity,
        input,
        src_len,
        &status
    );

    /*
     * Caso o buffer tenha sido pequeno, aumenta e tenta novamente.
     */
    if (status == U_BUFFER_OVERFLOW_ERROR) {

        capacity = result + 1;

        char *new_output = realloc(
            output,
            (size_t)capacity
        );

        if (!new_output) {
            perror("realloc");

            ucasemap_close(case_map);
            free(output);

            return NULL;
        }

        output = new_output;

        status = U_ZERO_ERROR;

        result = ucasemap_utf8ToUpper(
            case_map,
            output,
            capacity,
            input,
            src_len,
            &status
        );
    }

    ucasemap_close(case_map);

    if (U_FAILURE(status)) {

        fprintf(
            stderr,
            "Erro convertendo '%s': %s\n",
            input,
            u_errorName(status)
        );

        free(output);
        return NULL;
    }

    output[result] = '\0';

    return output;
}

/*
 * Verifica se o destino já existe.
 */
static int target_exists(const char *path)
{
    struct stat st;

    if (lstat(path, &st) == 0)
        return 1;

    if (errno == ENOENT)
        return 0;

    /*
     * Se ocorreu outro erro, tratamos como existente
     * por segurança.
     */
    return 1;
}

static int process_entry(
    const char *path,
    const struct stat *sb,
    int typeflag,
    struct FTW *ftwbuf
)
{
    (void)sb;
    (void)typeflag;
    (void)ftwbuf;

    /*
     * Não mexer no diretório raiz ".".
     */
    if (strcmp(path, ".") == 0)
        return 0;

    const char *slash = strrchr(path, '/');

    const char *name;
    char directory[PATH_MAX];

    if (slash) {
        size_t dir_len = (size_t)(slash - path);

        if (dir_len == 0) {
            strcpy(directory, "/");
        } else {
            if (dir_len >= sizeof(directory)) {
                fprintf(stderr,
                        "Caminho muito longo: %s\n",
                        path);
                return 0;
            }

            memcpy(directory, path, dir_len);
            directory[dir_len] = '\0';
        }

        name = slash + 1;
    } else {
        strcpy(directory, ".");
        name = path;
    }

    char *upper = to_upper_utf8(name);

    if (!upper)
        return 0;

    /*
     * Já está em maiúsculas.
     */
    if (strcmp(name, upper) == 0) {
        free(upper);
        return 0;
    }

    char target[PATH_MAX];

    int written = snprintf(
        target,
        sizeof(target),
        "%s/%s",
        directory,
        upper
    );

    if (written < 0 || (size_t)written >= sizeof(target)) {
        fprintf(stderr,
                "Destino muito longo:\n  %s\n",
                path);

        free(upper);
        return 0;
    }

    /*
     * Nunca sobrescrever.
     */
    if (target_exists(target)) {

        printf("CONFLITO:\n");
        printf("  ");
        print_escaped(path);
        printf("\n");

        printf("  -> ");
        print_escaped(target);
        printf("\n\n");

        free(upper);
        return 0;
    }

    printf("RENOMEAR:\n");
    printf("  ");
    print_escaped(path);
    printf("\n");

    printf("  -> ");
    print_escaped(target);
    printf("\n\n");

    if (!dry_run) {

        if (rename(path, target) != 0) {

            fprintf(
                stderr,
                "ERRO: não foi possível renomear '%s': %s\n",
                path,
                strerror(errno)
            );

        }
    }

    free(upper);

    return 0;
}

static void usage(const char *program)
{
    printf(
        "Uso: %s [opções] [diretório]\n"
        "\n"
        "Converte recursivamente nomes de arquivos e diretórios\n"
        "para caixa alta usando Unicode/UTF-8.\n"
        "\n"
        "Opções:\n"
        "  -n    Dry-run: apenas mostra o que seria alterado\n"
        "  -h    Mostra esta ajuda\n"
        "\n"
        "Exemplos:\n"
        "  %s -n .\n"
        "  %s .\n"
        "  %s -n /home/user/documentos\n",
        program,
        program,
        program,
        program
    );
}

int main(int argc, char **argv)
{
    setlocale(LC_ALL, "");

    int opt;

    while ((opt = getopt(argc, argv, "nh")) != -1) {

        switch (opt) {

        case 'n':
            dry_run = 1;
            break;

        case 'h':
            usage(argv[0]);
            return EXIT_SUCCESS;

        default:
            usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    const char *root = ".";

    if (optind < argc)
        root = argv[optind];

    struct stat st;

    if (lstat(root, &st) != 0) {

        fprintf(
            stderr,
            "Erro ao acessar '%s': %s\n",
            root,
            strerror(errno)
        );

        return EXIT_FAILURE;
    }

    printf(
        "Diretório: %s\n",
        root
    );

    if (dry_run)
        printf("Modo: DRY-RUN\n");
    else
        printf("Modo: ALTERAÇÃO REAL\n");

    printf("\n");

    /*
     * FTW_DEPTH garante processamento de baixo para cima:
     *
     * arquivo.txt
     * pasta/
     *
     * antes de:
     *
     * PASTA/
     */
    int flags = FTW_DEPTH | FTW_PHYS;

    if (nftw(
            root,
            process_entry,
            32,
            flags
        ) != 0) {

        fprintf(
            stderr,
            "Erro durante a varredura: %s\n",
            strerror(errno)
        );

        return EXIT_FAILURE;
    }

    printf("Concluído.\n");

    return EXIT_SUCCESS;
}