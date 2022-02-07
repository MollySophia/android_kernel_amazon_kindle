cmd_arch/arm/boot/compressed/decompress.o := /home/molly/kt3_ll_511_210/ll_511_210_build/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-gcc -Wp,-MD,arch/arm/boot/compressed/.decompress.o.d  -nostdinc -isystem /home/molly/kt3_ll_511_210/ll_511_210_build/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/../lib/gcc/arm-eabi/4.6.x-google/include -I/home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include -Iarch/arm/include/generated  -Iinclude -I/home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/uapi -Iarch/arm/include/generated/uapi -I/home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/include/uapi -Iinclude/generated/uapi -include /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/include/linux/kconfig.h -D__KERNEL__ -mlittle-endian -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -fno-delete-null-pointer-checks -O2 -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -marm -D__LINUX_ARM_ARCH__=7 -march=armv7-a -msoft-float -Uarm -Wframe-larger-than=1024 -fno-stack-protector -Wno-unused-but-set-variable -fomit-frame-pointer -fno-var-tracking-assignments -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack -fpic -mno-single-pic-base -fno-builtin -Iarch/arm/boot/compressed    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(decompress)"  -D"KBUILD_MODNAME=KBUILD_STR(decompress)" -c -o arch/arm/boot/compressed/decompress.o arch/arm/boot/compressed/decompress.c

source_arch/arm/boot/compressed/decompress.o := arch/arm/boot/compressed/decompress.c

deps_arch/arm/boot/compressed/decompress.o := \
    $(wildcard include/config/kernel/gzip.h) \
    $(wildcard include/config/kernel/lzo.h) \
    $(wildcard include/config/kernel/lzma.h) \
    $(wildcard include/config/kernel/xz.h) \
  include/linux/compiler.h \
    $(wildcard include/config/sparse/rcu/pointer.h) \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
    $(wildcard include/config/kprobes.h) \
  include/linux/compiler-gcc.h \
    $(wildcard include/config/arch/supports/optimized/inlining.h) \
    $(wildcard include/config/optimize/inlining.h) \
  include/linux/compiler-gcc4.h \
    $(wildcard include/config/arch/use/builtin/bswap.h) \
  include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/lbdaf.h) \
    $(wildcard include/config/arch/dma/addr/t/64bit.h) \
    $(wildcard include/config/phys/addr/t/64bit.h) \
    $(wildcard include/config/64bit.h) \
  include/uapi/linux/types.h \
  arch/arm/include/generated/asm/types.h \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/include/uapi/asm-generic/types.h \
  include/asm-generic/int-ll64.h \
  include/uapi/asm-generic/int-ll64.h \
  arch/arm/include/generated/asm/bitsperlong.h \
  include/asm-generic/bitsperlong.h \
  include/uapi/asm-generic/bitsperlong.h \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/include/uapi/linux/posix_types.h \
  include/linux/stddef.h \
  include/uapi/linux/stddef.h \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/uapi/asm/posix_types.h \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/include/uapi/asm-generic/posix_types.h \
  include/linux/linkage.h \
  include/linux/stringify.h \
  include/linux/export.h \
    $(wildcard include/config/have/underscore/symbol/prefix.h) \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/unused/symbols.h) \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/asm/linkage.h \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/asm/string.h \
  arch/arm/boot/compressed/../../../../lib/decompress_unlzo.c \
  arch/arm/boot/compressed/../../../../lib/lzo/lzo1x_decompress_safe.c \
    $(wildcard include/config/have/efficient/unaligned/access.h) \
  arch/arm/include/generated/asm/unaligned.h \
  include/asm-generic/unaligned.h \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/uapi/asm/byteorder.h \
  include/linux/byteorder/little_endian.h \
  include/uapi/linux/byteorder/little_endian.h \
  include/linux/swab.h \
  include/uapi/linux/swab.h \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/asm/swab.h \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/uapi/asm/swab.h \
  include/linux/byteorder/generic.h \
  include/linux/unaligned/le_struct.h \
  include/linux/unaligned/packed_struct.h \
  include/linux/kernel.h \
    $(wildcard include/config/preempt/voluntary.h) \
    $(wildcard include/config/debug/atomic/sleep.h) \
    $(wildcard include/config/prove/locking.h) \
    $(wildcard include/config/ring/buffer.h) \
    $(wildcard include/config/tracing.h) \
    $(wildcard include/config/ftrace/mcount/record.h) \
  /home/molly/kt3_ll_511_210/ll_511_210_build/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/../lib/gcc/arm-eabi/4.6.x-google/include/stdarg.h \
  include/linux/bitops.h \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/asm/bitops.h \
    $(wildcard include/config/smp.h) \
  include/linux/irqflags.h \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/irqsoff/tracer.h) \
    $(wildcard include/config/preempt/tracer.h) \
    $(wildcard include/config/trace/irqflags/support.h) \
  include/linux/typecheck.h \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/asm/irqflags.h \
    $(wildcard include/config/iohw/record.h) \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/asm/ptrace.h \
    $(wildcard include/config/arm/thumb.h) \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/uapi/asm/ptrace.h \
    $(wildcard include/config/cpu/endian/be8.h) \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/asm/hwcap.h \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/uapi/asm/hwcap.h \
  include/asm-generic/bitops/non-atomic.h \
  include/asm-generic/bitops/fls64.h \
  include/asm-generic/bitops/sched.h \
  include/asm-generic/bitops/hweight.h \
  include/asm-generic/bitops/arch_hweight.h \
  include/asm-generic/bitops/const_hweight.h \
  include/asm-generic/bitops/lock.h \
  include/asm-generic/bitops/le.h \
  include/asm-generic/bitops/ext2-atomic-setbit.h \
  include/linux/log2.h \
    $(wildcard include/config/arch/has/ilog2/u32.h) \
    $(wildcard include/config/arch/has/ilog2/u64.h) \
  include/linux/printk.h \
    $(wildcard include/config/lab126.h) \
    $(wildcard include/config/early/printk.h) \
    $(wildcard include/config/printk.h) \
    $(wildcard include/config/lab126/printk/buffer.h) \
    $(wildcard include/config/dynamic/debug.h) \
  include/linux/init.h \
    $(wildcard include/config/broken/rodata.h) \
  include/linux/kern_levels.h \
  include/linux/dynamic_debug.h \
  include/linux/string.h \
    $(wildcard include/config/binary/printf.h) \
  include/linux/errno.h \
  include/uapi/linux/errno.h \
  arch/arm/include/generated/asm/errno.h \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/include/uapi/asm-generic/errno.h \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/include/uapi/asm-generic/errno-base.h \
  include/uapi/linux/kernel.h \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/include/uapi/linux/sysinfo.h \
  include/linux/unaligned/be_byteshift.h \
  include/linux/unaligned/generic.h \
  include/linux/lzo.h \
  arch/arm/boot/compressed/../../../../lib/lzo/lzodefs.h \
  include/linux/decompress/mm.h \

arch/arm/boot/compressed/decompress.o: $(deps_arch/arm/boot/compressed/decompress.o)

$(deps_arch/arm/boot/compressed/decompress.o):
