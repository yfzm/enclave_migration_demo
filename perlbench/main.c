/*    miniperlmain.c
 *
 *    Copyright (C) 1994, 1995, 1996, 1997, 1999, 2000, 2001, 2002, 2003,
 *    2004, 2005 by Larry Wall and others
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

/*
 * "The Road goes ever on and on, down from the door where it began."
 */

/* This file contains the main() function for the perl interpreter.
 * Note that miniperlmain.c contains main() for the 'miniperl' binary,
 * while perlmain.c contains main() for the 'perl' binary.
 *
 * Miniperl is like perl except that it does not support dynamic loading,
 * and in fact is used to build the dynamic modules needed for the 'real'
 * perl executable.
 */

#ifdef OEMVS
#ifdef MYMALLOC
/* sbrk is limited to first heap segment so make it big */
#pragma runopts(HEAP(8M,500K,ANYWHERE,KEEP,8K,4K) STACK(,,ANY,) ALL31(ON))
#else
#pragma runopts(HEAP(2M,500K,ANYWHERE,KEEP,8K,4K) STACK(,,ANY,) ALL31(ON))
#endif
#endif


#include "EXTERN.h"
#define PERL_IN_MINIPERLMAIN_C
#include "perl.h"

#if 0
#  define YFZM_PUTS(s) puts(s)
#else
#  define YFZM_PUTS(s)
#endif

static void xs_init (pTHX);
static PerlInterpreter *my_perl;

#if defined (__MINT__) || defined (atarist)
/* The Atari operating system doesn't have a dynamic stack.  The
   stack size is determined from this value.  */
long _stksize = 64 * 1024;
#endif

int
main(int argc, char **argv, char **env)
{
    int exitstatus;
#ifndef PERL_USE_SAFE_PUTENV
    PL_use_safe_putenv = 0;
#endif /* PERL_USE_SAFE_PUTENV */

#ifdef PERL_GLOBAL_STRUCT
#define PERLVAR(var,type) /**/
#define PERLVARA(var,type) /**/
#define PERLVARI(var,type,init) PL_Vars.var = init;
#define PERLVARIC(var,type,init) PL_Vars.var = init;
#include "perlvars.h"
#undef PERLVAR
#undef PERLVARA
#undef PERLVARI
#undef PERLVARIC
#endif

    /* if user wants control of gprof profiling off by default */
    /* noop unless Configure is given -Accflags=-DPERL_GPROF_CONTROL */
    PERL_GPROF_MONCONTROL(0);

    YFZM_PUTS("[yfzm debug 0]\n");

    PERL_SYS_INIT3(&argc,&argv,&env);

#if defined(USE_5005THREADS) || defined(USE_ITHREADS)
    /* XXX Ideally, this should really be happening in perl_alloc() or
     * perl_construct() to keep libperl.a transparently fork()-safe.
     * It is currently done here only because Apache/mod_perl have
     * problems due to lack of a call to cancel pthread_atfork()
     * handlers when shared objects that contain the handlers may
     * be dlclose()d.  This forces applications that embed perl to
     * call PTHREAD_ATFORK() explicitly, but if and only if it hasn't
     * been called at least once before in the current process.
     * --GSAR 2001-07-20 */
    PTHREAD_ATFORK(Perl_atfork_lock,
                   Perl_atfork_unlock,
                   Perl_atfork_unlock);
#endif

    YFZM_PUTS("[yfzm debug 1]\n");
    if (!PL_do_undump) {
	    my_perl = perl_alloc();
        YFZM_PUTS("[yfzm debug 2]\n");
	    if (!my_perl)
	        exit(1);
        YFZM_PUTS("[yfzm debug 3]\n");
	    perl_construct(my_perl);
        YFZM_PUTS("[yfzm debug 4]\n");
	    PL_perl_destruct_level = 0;
    }
    YFZM_PUTS("[yfzm debug 5]\n");
    PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
    exitstatus = perl_parse(my_perl, xs_init, argc, argv, (char **)NULL);
    YFZM_PUTS("[yfzm debug 6]\n");
    if (!exitstatus)
        perl_run(my_perl);
      
    YFZM_PUTS("[yfzm debug 7]\n");
    exitstatus = perl_destruct(my_perl);

    YFZM_PUTS("[yfzm debug 8]\n");
    perl_free(my_perl);

    YFZM_PUTS("[yfzm debug 9]\n");
    PERL_SYS_TERM();

    YFZM_PUTS("[yfzm debug 10]\n");
    exit(exitstatus);
    return exitstatus;
}

/* Register any extra external extensions */

/* Do not delete this line--writemain depends on it */
EXTERN_C void boot_DynaLoader (pTHX_ CV* cv);
EXTERN_C void boot_Cwd (pTHX_ CV* cv);
EXTERN_C void boot_Data__Dumper (pTHX_ CV* cv);
EXTERN_C void boot_Devel__Peek (pTHX_ CV* cv);
EXTERN_C void boot_Digest__MD5 (pTHX_ CV* cv);
EXTERN_C void boot_IO (pTHX_ CV* cv);
EXTERN_C void boot_MIME__Base64 (pTHX_ CV* cv);
EXTERN_C void boot_Sys__Hostname (pTHX_ CV* cv);
EXTERN_C void boot_Time__HiRes (pTHX_ CV* cv);
EXTERN_C void boot_attrs (pTHX_ CV* cv);
EXTERN_C void boot_Storable (pTHX_ CV* cv);
EXTERN_C void boot_HTML__Parser (CV* cv);
EXTERN_C void boot_MIME__Base64 (CV* cv);
EXTERN_C void boot_Opcode (CV* cv);

static void
xs_init(pTHX)
{
    char *file = __FILE__;
    dXSUB_SYS;
        newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);
    newXS("Cwd::bootstrap", boot_Cwd, file);
    newXS("Data::Dumper::bootstrap", boot_Data__Dumper, file);
    newXS("Devel::Peek::bootstrap", boot_Devel__Peek, file);
    newXS("Digest::MD5::bootstrap", boot_Digest__MD5, file);
    newXS("IO::bootstrap", boot_IO, file);
    newXS("MIME::Base64::bootstrap", boot_MIME__Base64, file);
    newXS("Sys::Hostname::bootstrap", boot_Sys__Hostname, file);
    newXS("Time::HiRes::bootstrap", boot_Time__HiRes, file);
    newXS("attrs::bootstrap", boot_attrs, file);
    newXS("Storable::bootstrap", boot_Storable, file);
    newXS("HTML::Parser::bootstrap", boot_HTML__Parser, file);
    newXS("MIME::Base64::bootstrap", boot_MIME__Base64, file);
    newXS("Opcode::bootstrap", boot_Opcode, file);

}
