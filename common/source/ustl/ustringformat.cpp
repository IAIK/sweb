#include "ustringformat.h"

/* Blatantly stolen from FREEBSD. */

/*
 * Put a NUL-terminated ASCII number (base <= 16) in a buffer in reverse
 * order; return an optional length and a pointer to the last character
 * written in the buffer (i.e., the first character of the string).
 * The buffer pointed to by `nbuf' must have length >= MAXNBUF.
 */
char *
ksprintn(char* nbuf, register u_long ul, register int base, register int* lenp)
{
    register char *p;

    p = nbuf;
    *p = '\0';
    do {
            *++p = hex2ascii(ul % base);
    } while (ul /= base);
    if (lenp)
            *lenp = p - nbuf;
    return (p);
}


/*
 * Scaled down version of printf(3).
 */
int
kvprintf(char const *fmt, void (*func)(int, void*), void *arg, int radix, va_list ap)
{
#define PCHAR(c) {int cc=(c); if (func) (*func)(cc,arg); else *d++ = cc; retval++; }
        char nbuf[MAXNBUF];
        char *p, *q __attribute__((unused)), *d;
        u_char *up __attribute__((unused));
        int ch, n;
        u_long ul;
        int base, lflag, tmp, width, ladjust, sharpflag, neg, sign, dot;
        int dwidth;
        char padc;
        int retval = 0;

        if (!func)
                d = (char *) arg;
        else
                d = NULL;

        if (fmt == NULL)
                fmt = "(fmt null)\n";

        if (radix < 2 || radix > 36)
                radix = 10;

        for (;;) {
                padc = ' ';
                width = 0;
                while ((ch = (u_char)*fmt++) != '%') {
                        if (ch == '\0') 
                                return retval;
                        PCHAR(ch);
                }
                lflag = (sizeof(size_t) == 8); ladjust = 0; sharpflag = 0; neg = 0;
                sign = 0; dot = 0; dwidth = 0;
reswitch:       switch (ch = (u_char)*fmt++) {
                case '.':
                        dot = 1;
                        goto reswitch;
                case '#':
                        sharpflag = 1;
                        goto reswitch;
                case '+':
                        sign = 1;
                        goto reswitch;
                case '-':
                        ladjust = 1;
                        goto reswitch;
                case '%':
                        PCHAR(ch);
                        break;
                case '*':
                        if (!dot) {
                                width = va_arg(ap, int);
                                if (width < 0) {
                                        ladjust = !ladjust;
                                        width = -width;
                                }
                        } else {
                                dwidth = va_arg(ap, int);
                        }
                        goto reswitch;
                case '0':
                        if (!dot) {
                                padc = '0';
                                goto reswitch;
                        }
                case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9':
                                for (n = 0;; ++fmt) {
                                        n = n * 10 + ch - '0';
                                        ch = *fmt;
                                        if (ch < '0' || ch > '9')
                                                break;
                                }
                        if (dot)
                                dwidth = n;
                        else
                                width = n;
                        goto reswitch;
                case 'c':
                        PCHAR(va_arg(ap, int));
                        break;
                case 'd':
                        ul = lflag ? va_arg(ap, long) : va_arg(ap, int);
                        sign = 1;
                        base = 10;
                        goto number;
                case 'l':
                        lflag = 1;
                        goto reswitch;
                case 'o':
                        ul = lflag ? va_arg(ap, u_long) : va_arg(ap, u_int);
                        base = 8;
                        goto nosign;
                case 'p':
                        ul = (uintptr_t)va_arg(ap, void *);
                        base = 16;
                        sharpflag = 1;
                        goto nosign;
                case 'n':
                case 'r':
                        ul = lflag ? va_arg(ap, u_long) :
                            sign ? (u_long)va_arg(ap, int) : va_arg(ap, u_int);
                        base = radix;
                        goto number;
                case 's':
                        p = va_arg(ap, char *);
                        if (p == NULL)
                                p = (char*) "(null)";
                        if (!dot)
                                n = strlen (p);
                        else
                                for (n = 0; n < dwidth && p[n]; n++)
                                        continue;

                        width -= n;

                        if (!ladjust && width > 0)
                                while (width--)
                                        PCHAR(padc);
                        while (n--)
                                PCHAR(*p++);
                        if (ladjust && width > 0)
                                while (width--)
                                        PCHAR(padc);
                        break;
                case 'u':
                        ul = lflag ? va_arg(ap, u_long) : va_arg(ap, u_int);
                        base = 10;
                        goto nosign;
                case 'x':
                        ul = (uintptr_t)va_arg(ap, void *);
                        base = 16;
                        sharpflag = 1;
                        goto nosign;
                case 'z':
                        ul = lflag ? va_arg(ap, u_long) :
                            sign ? (u_long)va_arg(ap, int) : va_arg(ap, u_int);
                        base = 16;
                        goto number;
nosign:                 sign = 0;
number:                 if (sign && (long)ul < 0L) {
                                neg = 1;
                                ul = -(long)ul;
                        }
                        p = ksprintn(nbuf, ul, base, &tmp);
                        if (sharpflag && ul != 0) {
                                if (base == 8)
                                        tmp++;
                                else if (base == 16)
                                        tmp += 2;
                        }
                        if (neg)
                                tmp++;

                        if (!ladjust && width && (width -= tmp) > 0)
                                while (width--)
                                        PCHAR(padc);
                        if (neg)
                                PCHAR('-');
                        if (sharpflag && ul != 0) {
                                if (base == 8) {
                                        PCHAR('0');
                                } else if (base == 16) {
                                        PCHAR('0');
                                        PCHAR('x');
                                }
                        }

                        while (*p)
                                PCHAR(*p--);

                        if (ladjust && width && (width -= tmp) > 0)
                                while (width--)
                                        PCHAR(padc);

                        break;
                default:
                        PCHAR('%');
                        if (lflag)
                                PCHAR('l');
                        PCHAR(ch);
                        break;
                }
        }
#undef PCHAR
}

void
snprintf_func(int ch, void *arg)
{
    snprintf_arg *const info = (snprintf_arg* const)arg;

    if (info->remain >= 2) {
            *info->str++ = ch;
            info->remain--;
    }
}

/*
 * Scaled down version of vsnprintf(3).
 */
int
vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
   snprintf_arg info;
   int retval;

   info.str = str;
   info.remain = size;
   retval = kvprintf(format, snprintf_func, &info, 10, ap);
   if (info.remain >= 1)
           *info.str++ = '\0';
   return retval;
}


