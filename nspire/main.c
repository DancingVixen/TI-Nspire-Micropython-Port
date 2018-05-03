/*
 * This file is part of the Micro Python project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <libndls.h>

#include "mpconfig.h"
#include "nlr.h"
#include "misc.h"
#include "qstr.h"
#include "lexer.h"
#include "parse.h"
#include "obj.h"
#include "parsehelper.h"
#include "compile.h"
#include "runtime0.h"
#include "runtime.h"
#include "repl.h"
#include "gc.h"
#include "pfenv.h"
#include "genhdr/py-version.h"
#include "input.h"
#include "stackctrl.h"

// Command line options, with their defaults
uint mp_verbose_flag = 0;
//uint emit_opt = MP_EMIT_OPT_NATIVE_PYTHON;
uint emit_opt = MP_EMIT_OPT_NONE;

#if MICROPY_ENABLE_GC
// 3 MiB 
long heap_size = 2*1024*1024;
#endif

void nsp_texture_init();
void nsp_texture_deinit();

static bool should_exit = false;
static uint exit_val;

// returns standard error codes: 0 for success, 1 for all other errors
STATIC int execute_from_lexer(mp_lexer_t *lex, mp_parse_input_kind_t input_kind, bool is_repl) {
    if (lex == NULL) {
        return 1;
    }

    mp_parse_error_kind_t parse_error_kind;
    mp_parse_node_t pn = mp_parse(lex, input_kind, &parse_error_kind);

    if (pn == MP_PARSE_NODE_NULL) {
        // parse error
        mp_parse_show_exception(lex, parse_error_kind);
        mp_lexer_free(lex);
        return 1;
    }

    qstr source_name = lex->source_name;
    #if MICROPY_PY___FILE__
    if (input_kind == MP_PARSE_FILE_INPUT) {
        mp_store_global(MP_QSTR___file__, MP_OBJ_NEW_QSTR(source_name));
    }
    #endif
    mp_lexer_free(lex);

    mp_obj_t module_fun = mp_compile(pn, source_name, emit_opt, is_repl);

    if (module_fun == mp_const_none) {
        // compile error
        return 1;
    }

    // execute it
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_call_function_0(module_fun);
        nlr_pop();
        return 0;
    } else {
        // uncaught exception
        // check for SystemExit
        mp_obj_t exc = (mp_obj_t)nlr.ret_val;
        if (mp_obj_is_subclass_fast(mp_obj_get_type(exc), &mp_type_SystemExit)) {
			exit_val = mp_obj_get_int(mp_obj_exception_get_value(exc));
			should_exit = 1;
        }
        else
			mp_obj_print_exception(printf_wrapper, NULL, (mp_obj_t)nlr.ret_val);

        return 1;
    }
}

STATIC char *strjoin(const char *s1, int sep_char, const char *s2) {
    int l1 = strlen(s1);
    int l2 = strlen(s2);
    char *s = malloc(l1 + l2 + 2);
    memcpy(s, s1, l1);
    if (sep_char != 0) {
        s[l1] = sep_char;
        l1 += 1;
    }
    memcpy(s + l1, s2, l2);
    s[l1 + l2] = 0;
    return s;
}

STATIC void do_repl(void) {
    printf("Micro Python " MICROPY_GIT_TAG " on " MICROPY_BUILD_DATE "\n");

    while(!should_exit) {
        char *line = prompt(">>> ");
	if(!line)
		return;

        while (mp_repl_continue_with_input(line)) {
            char *line2 = prompt("... ");
            if (!line2)
                break;

            char *line3 = strjoin(line, '\n', line2);
            free(line);
            free(line2);
            line = line3;
        }

        if(strcmp("quit", line) == 0) {
            should_exit = true;
        } else {
            mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, line, strlen(line), false);
            execute_from_lexer(lex, MP_PARSE_SINGLE_INPUT, true);
        }

        free(line);
    }
}

STATIC int do_file(const char *file) {
    mp_lexer_t *lex = mp_lexer_new_from_file(file);
    return execute_from_lexer(lex, MP_PARSE_FILE_INPUT, false);
}

/*STATIC int do_str(const char *str) {
    mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, str, strlen(str), false);
    return execute_from_lexer(lex, MP_PARSE_SINGLE_INPUT, false);
}*/

int main(int argc, char **argv) {

    //Disable output buffering, otherwise interactive mode becomes useless
    setbuf(stdout, NULL);
  
    cfg_register_fileext("py", "micropython");
  
    mp_stack_set_limit(32768);

#if MICROPY_ENABLE_GC
    char *heap = malloc(heap_size);
    if(!heap)
    {
	_show_msgbox("Micropython", "Heap allocation failed. Please reboot.", 0);
	return 1;
    }
    gc_init(heap, heap + heap_size - 1);
#endif

    nsp_texture_init();

    mp_init();

    uint path_num = 2;
    mp_obj_list_init(mp_sys_path, path_num);
    mp_obj_t *path_items;
    mp_obj_list_get(mp_sys_path, &path_num, &path_items);

    path_items[0] = MP_OBJ_NEW_QSTR(MP_QSTR_);
    path_items[1] = MP_OBJ_NEW_QSTR(qstr_from_str("/documents/ndless"));

    mp_obj_list_init(mp_sys_argv, 0);

    const int NOTHING_EXECUTED = -2;
    int ret = NOTHING_EXECUTED;

    for (int a = 1; a < argc; a++) {
        // Set base dir of the script as first entry in sys.path
        char *p = strrchr(argv[a], '/');
        path_items[0] = MP_OBJ_NEW_QSTR(qstr_from_strn(argv[a], p - argv[a]));

        for (int i = a; i < argc; i++) {
            mp_obj_list_append(mp_sys_argv, MP_OBJ_NEW_QSTR(qstr_from_str(argv[i])));
        }

        ret = do_file(argv[a]);
        break;
    }

    if (ret == NOTHING_EXECUTED) {
        do_repl();
        ret = 0;
    }
    else
    {
        puts("Press any key to exit.");
        wait_key_pressed();
    }

    mp_deinit();

    free(heap);

    nsp_texture_deinit();

    if(should_exit)
        return exit_val;
	
    return ret;
}

mp_import_stat_t mp_import_stat(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return MP_IMPORT_STAT_DIR;
        } else if (S_ISREG(st.st_mode)) {
            return MP_IMPORT_STAT_FILE;
        }
    }
    return MP_IMPORT_STAT_NO_EXIST;
}

int DEBUG_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = vfprintf(stderr, fmt, ap);
    va_end(ap);
    return ret;
}

void nlr_jump_fail(void *val) {
    printf("FATAL: uncaught NLR %p\n", val);
    exit(1);
}
