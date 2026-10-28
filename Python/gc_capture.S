/* _PyGC_CaptureCaller(_PyGCContext *ctx): the start context of the
 * collector's stack walk (boil/design/11-precise-roots.md, section 3.3):
 * the caller's pc at the call, its stack pointer there, and its
 * callee-saved registers, which this function has not touched, since no
 * prologue runs before it reads them.  The layout is _PyGCContext's
 * (Include/internal/pycore_gc_walk.h).  On other platforms Python/gc_walk.c
 * defines it in C, and the walk is off. */

#if defined(__ELF__) && defined(__x86_64__)
    .text
    .globl  _PyGC_CaptureCaller
    .type   _PyGC_CaptureCaller, @function
    .p2align 4
_PyGC_CaptureCaller:
    .cfi_startproc
#if defined(__CET__) && (__CET__ & 1)
    endbr64
#endif
    movq    (%rsp), %rax        /* pc: the return address */
    movq    %rax, 0(%rdi)
    leaq    8(%rsp), %rax       /* sp: the caller's, at the call */
    movq    %rax, 8(%rdi)
    movq    %rbx, 16(%rdi)
    movq    %rbp, 24(%rdi)
    movq    %r12, 32(%rdi)
    movq    %r13, 40(%rdi)
    movq    %r14, 48(%rdi)
    movq    %r15, 56(%rdi)
    ret
    .cfi_endproc
    .size   _PyGC_CaptureCaller, .-_PyGC_CaptureCaller

#elif defined(__ELF__) && defined(__aarch64__)
    .text
    .globl  _PyGC_CaptureCaller
    .type   _PyGC_CaptureCaller, %function
    .p2align 2
_PyGC_CaptureCaller:
    .cfi_startproc
#if defined(__ARM_FEATURE_BTI_DEFAULT) && __ARM_FEATURE_BTI_DEFAULT
    bti     c
#endif
    mov     x9, sp
    stp     x30, x9, [x0, #0]   /* pc: the link register; sp: unchanged */
    stp     x19, x20, [x0, #16]
    stp     x21, x22, [x0, #32]
    stp     x23, x24, [x0, #48]
    stp     x25, x26, [x0, #64]
    stp     x27, x28, [x0, #80]
    stp     x29, x30, [x0, #96]
    stp     d8, d9, [x0, #112]
    stp     d10, d11, [x0, #128]
    stp     d12, d13, [x0, #144]
    stp     d14, d15, [x0, #160]
    ret
    .cfi_endproc
    .size   _PyGC_CaptureCaller, .-_PyGC_CaptureCaller
#endif

#if defined(__ELF__)
    .section .note.GNU-stack,"",@progbits
#endif

#if defined(__ELF__) && defined(__x86_64__) && defined(__CET__) && (__CET__ & 1)
    .section .note.gnu.property,"a"
    .align 8
    .long   1f - 0f
    .long   4f - 1f
    .long   5
0:
    .string "GNU"
1:
    .align 8
    .long   0xc0000002
    .long   3f - 2f
2:
    .long   0x3
3:
    .align 8
4:
#endif
