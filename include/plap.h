#ifndef PLAP_H
#define PLAP_H
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PLAP_STRING 1
#define PLAP_INT 2
#define PLAP_DOUBLE 3

typedef struct positional_arg {
    char* name;
    int t;
    union {
        char* str;
        int i;
        double lf;
    };
} PositionalArg;

typedef struct option {
    char* short_name;
    char* long_name;
    int t;
    union {
        char* str;
        int i;
        double lf;
    };
} Option;

typedef struct args_t {
    // positional
    size_t positional_count;
    PositionalArg* positional_args;

    // options
    size_t optional_count;
    Option* optional_args;
} Args;

typedef struct positional_def {
    int parse_as;
    int required;
    char* name;
} PositionalDef;

typedef struct option_def {
    int matched;
    int parse_as;
    int needs_value;
    char* short_name;
    char* long_name;
} OptionDef;

typedef struct args_def_t {
    // positional
    size_t pos_count;
    size_t pos_sz;
    PositionalDef* pos_defs;
    size_t pos_req;

    // options
    size_t opt_count;
    size_t opt_sz;
    OptionDef* opt_defs;
} ArgsDef;

typedef struct args_wrap {
    int args_count;
    int args_pointer;
    char** args;
} ArgsWrap;

void plap_parse_positional(char* value, PositionalArg* parg, PositionalDef* pdef);
void plap_parse_option(const char* value, ArgsWrap* awrap, OptionDef* optdefs, size_t optdefc, Option* res);

ArgsDef plap_args_def();
Args plap_parse_args(ArgsDef def, int argc, char** args);
void plap_print_usage(ArgsDef* def, const char* prog_name);

void plap_positional(ArgsDef* def, const char* name, int parse_as, int required);
void plap_positional_string(ArgsDef* def, const char* name, int required);
void plap_positional_int(ArgsDef* def, const char* name, int required);
void plap_positional_double(ArgsDef* def, const char* name, int required);

void plap_option(ArgsDef* def, const char* sh, const char* l, int parse_as, int needs_value);
void plap_option_string(ArgsDef* def, const char* sh, const char* l, int needs_value);
void plap_option_int(ArgsDef* def, const char* sh, const char* l, int needs_value);
void plap_option_double(ArgsDef* def, const char* sh, const char* l, int needs_value);

void plap_free_option(Option opt);
void plap_free_args_def(ArgsDef def);
void plap_free_args(Args a);

Option* plap_get_option(Args* args, const char* sh, const char* l);
PositionalArg* plap_get_positional(Args* args, size_t pos);

ArgsWrap plap_args_wrap_wrap(int argc, char** args);
char* plap_args_wrap_next(ArgsWrap* aw);

#endif
#ifdef PLAP_IMPLEMENTATION

void plap_print_usage(ArgsDef* def, const char* prog_name)
{
    printf("USAGE \n\t%s ", prog_name);
    for (size_t i = 0; i < def->pos_count; i++) {
        printf("%s ", def->pos_defs[i].name);
    }
    printf("\nOPTIONS:\n");
    for (size_t i = 0; i < def->opt_count; i++) {
        OptionDef* o = &def->opt_defs[i];
        if (o->short_name && o->long_name) {
            printf("\t-%s, --%s \n", o->short_name, o->long_name);
        } else if (o->short_name) {
            printf("\t-%s \n", o->short_name);
        } else if (o->long_name) {
            printf("\t--%s \n", o->long_name);
        }
    }
}

Args plap_parse_args(ArgsDef def, int argc, char** args)
{
    ArgsWrap awrap = plap_args_wrap_wrap(argc, args);
    char* prog_name = plap_args_wrap_next(&awrap);
    if ((argc - 1) < def.pos_req || argc == 0) {
        fprintf(stderr, "Not enough arguments were supplied\n");
        plap_print_usage(&def, prog_name);
        exit(-1);
    }

    Args a = {
        .positional_args = calloc(argc, sizeof(PositionalArg)),
        .optional_args = calloc(argc, sizeof(Option)),
        .positional_count = 0,
        .optional_count = 0,
    };

    char* next = NULL;
    int positional = 0;
    while ((next = plap_args_wrap_next(&awrap))) {
        if (next[0] == '-') {
            plap_parse_option(next + 1, &awrap, def.opt_defs, def.opt_count, &a.optional_args[a.optional_count++]);
        } else {
            PositionalDef* pdef = positional < def.pos_count ? &def.pos_defs[positional++] : NULL;
            PositionalArg* parg = &a.positional_args[a.positional_count++];
            plap_parse_positional(next, parg, pdef);
        }
    }
    if (a.positional_count < def.pos_req) {
        fprintf(stderr, "Not enough positional arguments were supplied\n");
        plap_print_usage(&def, prog_name);
        exit(-1);
    }
    plap_free_args_def(def);
    return a;
}
ArgsDef plap_args_def()
{
    ArgsDef adef = {
        .pos_count = 0,
        .pos_sz = 16,
        .opt_count = 0,
        .opt_sz = 16,
    };
    adef.pos_defs = calloc(adef.pos_sz, sizeof(PositionalDef));
    adef.opt_defs = calloc(adef.opt_sz, sizeof(OptionDef));
    return adef;
}
void plap_parse_positional(char* value, PositionalArg* parg, PositionalDef* pdef)
{
    if (pdef) {
        switch (pdef->parse_as) {
        case PLAP_INT:
            parg->t = PLAP_INT;
            parg->i = atoi(value);
            break;
        case PLAP_DOUBLE:
            parg->t = PLAP_DOUBLE;
            parg->lf = atof(value);
            break;
        case PLAP_STRING:
        default:
            parg->t = PLAP_STRING;
            parg->str = (char*)calloc(strlen(value) + 1, sizeof(char));
            strcpy(parg->str, value);
            break;
        }
        if (pdef->name) {
            parg->name = (char*)calloc(strlen(pdef->name) + 1, sizeof(char));
            strcpy(parg->name, pdef->name);
        }
    } else {
        parg->t = PLAP_STRING;
        parg->str = (char*)calloc(strlen(value) + 1, sizeof(char));
        strcpy(parg->str, value);
    }
}

int streq(const char* a, const char* b)
{
    if (!a || !b)
        return 0;
    return strcmp(a, b) == 0;
}

OptionDef* find_option_def(OptionDef* odefs, size_t ocount, const char* sh, const char* l)
{
    for (size_t i = 0; i < ocount; i++) {
        OptionDef* od = &odefs[i];
        if (streq(od->long_name, l) || streq(od->short_name, sh)) {
            return od;
        }
    }
    return NULL;
}
Option* find_option(Option* opts, size_t ocount, const char* sh, const char* l)
{
    for (size_t i = 0; i < ocount; i++) {
        Option* od = &opts[i];
        if (od->long_name == l || od->short_name == sh) {
            return od;
        }
    }
    return NULL;
}

void plap_parse_option(const char* value, ArgsWrap* awrap, OptionDef* optdefs, size_t optdefc, Option* res)
{
    size_t len = strlen(value);
    if (len == 0 || (value[0] == '-' && len == 1)) {
        fprintf(stderr, "Empty option\n");
        exit(-1);
    }
    OptionDef* optdf = NULL;
    const char* ln = NULL;
    const char* s = NULL;
    if (value[0] == '-') {
        ln = value + 1;
        optdf = find_option_def(optdefs, optdefc, NULL, ln);
    } else {
        s = value;
        optdf = find_option_def(optdefs, optdefc, s, NULL);
    }
    if (!optdf) {
        fprintf(stderr, "Unknown option `%s`\n", value - 1);
        exit(-1);
    }
    if (optdf->matched) {
        fprintf(stderr, "Option `%s` already supplied\n", value - 1);
        exit(-1);
    }
    optdf->matched = 1;
    res->long_name = (char*)calloc(strlen(ln) + 1, sizeof(char));
    strcpy(res->long_name, ln);
    res->short_name = (char*)calloc(strlen(s) + 1, sizeof(char));
    strcpy(res->short_name, s);

    if (!optdf->needs_value) {
        return;
    }
    char* next = plap_args_wrap_next(awrap);
    if (!next) {
        fprintf(stderr, "Option `%s` without value\n", value - 1);
        exit(-1);
    }
    switch (optdf->parse_as) {
    case PLAP_INT:
        res->t = PLAP_INT;
        res->i = atoi(next);
        break;
    case PLAP_DOUBLE:
        res->t = PLAP_DOUBLE;
        res->lf = atof(next);
        break;
    case PLAP_STRING:
    default:
        res->t = PLAP_STRING;
        res->str = (char*)calloc(strlen(next) + 1, sizeof(char));
        strcpy(res->str, next);
        break;
    }
}

void plap_positional_int(ArgsDef* def, const char* name, int required)
{
    plap_positional(def, name, PLAP_INT, required);
}

void plap_positional_double(ArgsDef* def, const char* name, int required)
{
    plap_positional(def, name, PLAP_DOUBLE, required);
}
void plap_positional_string(ArgsDef* def, const char* name, int required)
{
    plap_positional(def, name, PLAP_STRING, required);
}

void plap_positional(ArgsDef* def, const char* name, int parse_as, int required)
{
    PositionalDef pdef = { 0 };
    pdef.name = calloc(strlen(name) + 1, sizeof(char));
    strcpy(pdef.name, name);
    pdef.parse_as = parse_as;
    pdef.required = required;
    def->pos_req += (required ? 1 : 0);
    if (++def->pos_count >= def->pos_sz) {
        def->pos_sz *= 2;
        def->pos_defs = realloc(def->pos_defs, sizeof(PositionalDef) * def->pos_sz);
    }
    def->pos_defs[def->pos_count - 1] = pdef;
}
void plap_option_string(ArgsDef* def, const char* sh, const char* l, int needs_value)
{
    plap_option(def, sh, l, PLAP_STRING, needs_value);
}
void plap_option_int(ArgsDef* def, const char* sh, const char* l, int needs_value)
{
    plap_option(def, sh, l, PLAP_INT, needs_value);
}
void plap_option_double(ArgsDef* def, const char* sh, const char* l, int needs_value)
{
    plap_option(def, sh, l, PLAP_DOUBLE, needs_value);
}
void plap_option(ArgsDef* def, const char* sh, const char* l, int parse_as, int needs_value)
{
    OptionDef odef = { 0 };
    odef.long_name = calloc(strlen(l) + 1, sizeof(char));
    strcpy(odef.long_name, l);
    if (sh) {
        odef.short_name = calloc(strlen(sh) + 1, sizeof(char));
        strcpy(odef.short_name, sh);
    }
    odef.parse_as = parse_as;
    odef.needs_value = needs_value;
    if (++def->opt_count >= def->opt_sz) {
        def->opt_sz *= 2;
        def->opt_defs = realloc(def->opt_defs, sizeof(OptionDef) * def->opt_sz);
    }
    def->opt_defs[def->opt_count - 1] = odef;
}

void plap_free_pos_def(PositionalDef pdef)
{
    if (pdef.name) {
        free(pdef.name);
    }
}
void plap_free_args_def(ArgsDef def)
{
    for (size_t i = 0; i < def.pos_count; i++) {
        plap_free_pos_def(def.pos_defs[i]);
    }
    free(def.pos_defs);
    def.pos_defs = NULL;
}

ArgsWrap plap_args_wrap_wrap(int argc, char** args)
{
    return (ArgsWrap) {
        .args = args,
        .args_count = argc,
        .args_pointer = -1
    };
}

char* plap_args_wrap_next(ArgsWrap* aw)
{
    if (++aw->args_pointer >= aw->args_count) {
        return NULL;
    }
    return aw->args[aw->args_pointer];
}
void plap_free_args(Args a)
{
    for (size_t i = 0; i < a.positional_count; i++) {
        if (a.positional_args[i].t == PLAP_STRING && a.positional_args[i].str) {
            free(a.positional_args[i].str);
            a.positional_args[i].str = NULL;
        }
        if (a.positional_args[i].name) {
            free(a.positional_args[i].name);
            a.positional_args[i].name = NULL;
        }
    }
}
void plap_free_option(Option opt)
{
    if (opt.long_name) {
        free(opt.long_name);
    }
    if (opt.short_name) {
        free(opt.short_name);
    }
    if (opt.t == PLAP_STRING && opt.str) {
        free(opt.str);
    }
}
Option* plap_get_option(Args* args, const char* sh, const char* l)
{
    return find_option(args->optional_args, args->optional_count, sh, l);
}
PositionalArg* plap_get_positional(Args* args, size_t pos)
{
    if (pos < args->positional_count) {
        return &args->positional_args[pos];
    }
    return NULL;
}
#endif
