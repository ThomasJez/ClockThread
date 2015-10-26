dnl $Id$
dnl config.m4 for extension clockthread

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

PHP_ARG_ENABLE(clockthread, for clockthread support,
[  --enable-clockthread             Include clockthread support])

if test "$PHP_CLOCKTHREAD" != "no"; then
  AC_DEFINE(HAVE_CLOCKTHREADLIB,1,[ ])
  PHP_NEW_EXTENSION(clockthread, clockthread.c, $ext_shared)
fi
