/*********************************************************************
 * error.h
 *********************************************************************/

#ifndef _KLT_ERROR_H_
#define _KLT_ERROR_H_

#include <stdio.h>
#include <stdarg.h>

void KLTError(char *fmt, ...);
void KLTWarning(char *fmt, ...);

#endif

