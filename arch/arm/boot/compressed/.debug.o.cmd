cmd_arch/arm/boot/compressed/debug.o := /home/molly/kt3_ll_511_210/ll_511_210_build/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-gcc -Wp,-MD,arch/arm/boot/compressed/.debug.o.d -nostdinc -isystem /home/molly/kt3_ll_511_210/ll_511_210_build/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/../lib/gcc/arm-eabi/4.6.x-google/include -I/home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include -Iarch/arm/include/generated  -Iinclude -I/home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/uapi -Iarch/arm/include/generated/uapi -I/home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/include/uapi -Iinclude/generated/uapi -include /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/include/linux/kconfig.h -D__KERNEL__ -mlittle-endian -D__ASSEMBLY__ -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -marm -D__LINUX_ARM_ARCH__=7 -march=armv7-a  -include asm/unified.h -msoft-float    -DZIMAGE    -c -o arch/arm/boot/compressed/debug.o arch/arm/boot/compressed/debug.S

source_arch/arm/boot/compressed/debug.o := arch/arm/boot/compressed/debug.S

deps_arch/arm/boot/compressed/debug.o := \
    $(wildcard include/config/debug/semihosting.h) \
    $(wildcard include/config/debug/ll/include.h) \
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
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/asm/assembler.h \
    $(wildcard include/config/cpu/feroceon.h) \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/smp.h) \
    $(wildcard include/config/cpu/use/domains.h) \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/asm/ptrace.h \
    $(wildcard include/config/arm/thumb.h) \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/uapi/asm/ptrace.h \
    $(wildcard include/config/cpu/endian/be8.h) \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/asm/hwcap.h \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/uapi/asm/hwcap.h \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/asm/domain.h \
    $(wildcard include/config/io/36.h) \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/asm/opcodes-virt.h \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/asm/opcodes.h \
    $(wildcard include/config/cpu/endian/be32.h) \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/debug/imx.S \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/include/debug/imx-uart.h \
    $(wildcard include/config/debug/imx/uart/port.h) \
    $(wildcard include/config/debug/imx1/uart.h) \
    $(wildcard include/config/debug/imx21/imx27/uart.h) \
    $(wildcard include/config/debug/imx25/uart.h) \
    $(wildcard include/config/debug/imx31/uart.h) \
    $(wildcard include/config/debug/imx35/uart.h) \
    $(wildcard include/config/debug/imx51/uart.h) \
    $(wildcard include/config/debug/imx53/uart.h) \
    $(wildcard include/config/debug/imx6q/uart.h) \
    $(wildcard include/config/debug/imx6sl/uart.h) \
    $(wildcard include/config/debug/imx6sx/uart.h) \

arch/arm/boot/compressed/debug.o: $(deps_arch/arm/boot/compressed/debug.o)

$(deps_arch/arm/boot/compressed/debug.o):