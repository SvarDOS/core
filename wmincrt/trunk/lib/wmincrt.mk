SRCS = $(t)startup$(ext) &
       $(t)stkchk$(ext) &
       $(t)alloc$(ext) &
       $(t)u4m$(ext)

ext=.obj
OBJS=$+ $(SRCS) $-
t=+
WLIB_OBJS=$+ $(SRCS) $-

!ifdef DEBUG
ASM_FLAGS+=-DDEBUG
!endif

!ifdef NOARGV
ASM_FLAGS+=-DNOARGV
!endif

!ifdef NOPRGNAME
ASM_FLAGS+=-DNOPRGNAME
!endif

MIN_FLAGS = -DNOARGV
DBG_FLAGS = -DDEBUG -d1

.asm : ../../
