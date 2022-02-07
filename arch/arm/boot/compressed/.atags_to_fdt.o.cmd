cmd_arch/arm/boot/compressed/atags_to_fdt.o := /home/molly/kt3_ll_511_210/ll_511_210_build/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-gcc -Wp,-MD,arch/arm/boot/compressed/.atags_to_fdt.o.d  -nostdinc -isystem /home/molly/kt3_ll_511_210/ll_511_210_build/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/../lib/gcc/arm-eabi/4.6.x-google/include -I/home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include -Iarch/arm/include/generated  -Iinclude -I/home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/uapi -Iarch/arm/include/generated/uapi -I/home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/include/uapi -Iinclude/generated/uapi -include /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/include/linux/kconfig.h -D__KERNEL__ -mlittle-endian -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -fno-delete-null-pointer-checks -O2 -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -marm -D__LINUX_ARM_ARCH__=7 -march=armv7-a -msoft-float -Uarm -Wframe-larger-than=1024 -fno-stack-protector -Wno-unused-but-set-variable -fomit-frame-pointer -fno-var-tracking-assignments -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack -fpic -mno-single-pic-base -fno-builtin -Iarch/arm/boot/compressed    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(atags_to_fdt)"  -D"KBUILD_MODNAME=KBUILD_STR(atags_to_fdt)" -c -o arch/arm/boot/compressed/atags_to_fdt.o arch/arm/boot/compressed/atags_to_fdt.c

source_arch/arm/boot/compressed/atags_to_fdt.o := arch/arm/boot/compressed/atags_to_fdt.c

deps_arch/arm/boot/compressed/atags_to_fdt.o := \
    $(wildcard include/config/arm/atag/dtb/compat/cmdline/extend.h) \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/asm/setup.h \
    $(wildcard include/config/arm/nr/banks.h) \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/uapi/asm/setup.h \
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
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/uapi/asm/posix_types.h \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/include/uapi/asm-generic/posix_types.h \
  arch/arm/boot/compressed/libfdt.h \
  arch/arm/boot/compressed/libfdt_env.h \
  include/linux/string.h \
    $(wildcard include/config/binary/printf.h) \
  /home/molly/kt3_ll_511_210/ll_511_210_build/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/../lib/gcc/arm-eabi/4.6.x-google/include/stdarg.h \
  include/uapi/linux/string.h \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/asm/string.h \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/uapi/asm/byteorder.h \
  include/linux/byteorder/little_endian.h \
  include/uapi/linux/byteorder/little_endian.h \
  include/linux/swab.h \
  include/uapi/linux/swab.h \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/asm/swab.h \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/uapi/asm/swab.h \
  include/linux/byteorder/generic.h \
  arch/arm/boot/compressed/fdt.h \

arch/arm/boot/compressed/atags_to_fdt.o: $(deps_arch/arm/boot/compressed/atags_to_fdt.o)

$(deps_arch/arm/boot/compressed/atags_to_fdt.o):
