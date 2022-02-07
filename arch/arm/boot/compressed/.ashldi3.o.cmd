cmd_arch/arm/boot/compressed/ashldi3.o := /home/molly/kt3_ll_511_210/ll_511_210_build/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-gcc -Wp,-MD,arch/arm/boot/compressed/.ashldi3.o.d -nostdinc -isystem /home/molly/kt3_ll_511_210/ll_511_210_build/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/../lib/gcc/arm-eabi/4.6.x-google/include -I/home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include -Iarch/arm/include/generated  -Iinclude -I/home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/uapi -Iarch/arm/include/generated/uapi -I/home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/include/uapi -Iinclude/generated/uapi -include /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/include/linux/kconfig.h -D__KERNEL__ -mlittle-endian -D__ASSEMBLY__ -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -marm -D__LINUX_ARM_ARCH__=7 -march=armv7-a  -include asm/unified.h -msoft-float    -DZIMAGE    -c -o arch/arm/boot/compressed/ashldi3.o arch/arm/boot/compressed/ashldi3.S

source_arch/arm/boot/compressed/ashldi3.o := arch/arm/boot/compressed/ashldi3.S

deps_arch/arm/boot/compressed/ashldi3.o := \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/asm/unified.h \
    $(wildcard include/config/arm/asm/unified.h) \
    $(wildcard include/config/thumb2/kernel.h) \
  include/linux/linkage.h \
  include/linux/compiler.h \
    $(wildcard include/config/sparse/rcu/pointer.h) \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
    $(wildcard include/config/kprobes.h) \
  include/linux/stringify.h \
  include/linux/export.h \
    $(wildcard include/config/have/underscore/symbol/prefix.h) \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/unused/symbols.h) \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/asm/linkage.h \

arch/arm/boot/compressed/ashldi3.o: $(deps_arch/arm/boot/compressed/ashldi3.o)

$(deps_arch/arm/boot/compressed/ashldi3.o):
