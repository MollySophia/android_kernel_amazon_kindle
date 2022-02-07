cmd_drivers/mxc/gpu-viv/hal/kernel/gc_hal_kernel_mmu.o := /home/aosp/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-gcc -Wp,-MD,drivers/mxc/gpu-viv/hal/kernel/.gc_hal_kernel_mmu.o.d  -nostdinc -isystem /home/aosp/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/../lib/gcc/arm-eabi/4.6.x-google/include -I/home/aosp/kernel_imx/arch/arm/include -Iarch/arm/include/generated  -Iinclude -I/home/aosp/kernel_imx/arch/arm/include/uapi -Iarch/arm/include/generated/uapi -I/home/aosp/kernel_imx/include/uapi -Iinclude/generated/uapi -include /home/aosp/kernel_imx/include/linux/kconfig.h -D__KERNEL__ -mlittle-endian -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -fno-delete-null-pointer-checks -O2 -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -marm -D__LINUX_ARM_ARCH__=7 -march=armv7-a -msoft-float -Uarm -Wframe-larger-than=1024 -fno-stack-protector -Wno-unused-but-set-variable -fomit-frame-pointer -fno-var-tracking-assignments -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fconserve-stack -DgcdDEFAULT_CONTIGUOUS_SIZE=134217728 -DgcdANDROID_NATIVE_FENCE_SYNC=3 -DLINUX_CMA_FSL=1 -Werror -Idrivers/staging/android -DLINUX -DDRIVER -DDBG=0 -DUSE_PLATFORM_DRIVER=1 -DVIVANTE_PROFILER=1 -DVIVANTE_PROFILER_CONTEXT=1 -DENABLE_GPU_CLOCK_BY_DRIVER=0 -DUSE_NEW_LINUX_SIGNAL=0 -DgcdPAGED_MEMORY_CACHEABLE=0 -DgcdNONPAGED_MEMORY_CACHEABLE=0 -DgcdNONPAGED_MEMORY_BUFFERABLE=1 -DgcdCACHE_FUNCTION_UNIMPLEMENTED=0 -DgcdSMP=0 -DgcdENABLE_3D=1 -DgcdENABLE_2D=1 -DgcdENABLE_VG=1 -DgcdENABLE_OUTER_CACHE_PATCH=1 -DgcdENABLE_BANK_ALIGNMENT=1 -DgcdBANK_BIT_START=13 -DgcdBANK_BIT_END=15 -DgcdBANK_CHANNEL_BIT=12 -DgcdFPGA_BUILD=0 -I/home/aosp/kernel_imx/drivers/mxc/gpu-viv/hal/kernel/inc -I/home/aosp/kernel_imx/drivers/mxc/gpu-viv/hal/kernel -I/home/aosp/kernel_imx/drivers/mxc/gpu-viv/hal/kernel/arch -I/home/aosp/kernel_imx/drivers/mxc/gpu-viv/hal/kernel/inc -I/home/aosp/kernel_imx/drivers/mxc/gpu-viv/hal/os/linux/kernel -I/home/aosp/kernel_imx/drivers/mxc/gpu-viv/hal/os/linux/kernel/allocator/freescale -I/home/aosp/kernel_imx/drivers/mxc/gpu-viv/hal/kernel/archvg    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(gc_hal_kernel_mmu)"  -D"KBUILD_MODNAME=KBUILD_STR(galcore)" -c -o drivers/mxc/gpu-viv/hal/kernel/gc_hal_kernel_mmu.o drivers/mxc/gpu-viv/hal/kernel/gc_hal_kernel_mmu.c

source_drivers/mxc/gpu-viv/hal/kernel/gc_hal_kernel_mmu.o := drivers/mxc/gpu-viv/hal/kernel/gc_hal_kernel_mmu.c

deps_drivers/mxc/gpu-viv/hal/kernel/gc_hal_kernel_mmu.o := \
  drivers/mxc/gpu-viv/hal/kernel/gc_hal_kernel_precomp.h \
  /home/aosp/kernel_imx/drivers/mxc/gpu-viv/hal/kernel/inc/gc_hal.h \
  /home/aosp/kernel_imx/drivers/mxc/gpu-viv/hal/kernel/inc/gc_hal_rename.h \
  /home/aosp/kernel_imx/drivers/mxc/gpu-viv/hal/kernel/inc/gc_hal_types.h \
  /home/aosp/kernel_imx/drivers/mxc/gpu-viv/hal/kernel/inc/gc_hal_version.h \
  /home/aosp/kernel_imx/drivers/mxc/gpu-viv/hal/kernel/inc/gc_hal_options.h \
    $(wildcard include/config/smp.h) \
    $(wildcard include/config/sync.h) \
  include/generated/uapi/linux/version.h \
  include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/lbdaf.h) \
    $(wildcard include/config/arch/dma/addr/t/64bit.h) \
    $(wildcard include/config/phys/addr/t/64bit.h) \
    $(wildcard include/config/64bit.h) \
  include/uapi/linux/types.h \
  arch/arm/include/generated/asm/types.h \
  /home/aosp/kernel_imx/include/uapi/asm-generic/types.h \
  include/asm-generic/int-ll64.h \
  include/uapi/asm-generic/int-ll64.h \
  arch/arm/include/generated/asm/bitsperlong.h \
  include/asm-generic/bitsperlong.h \
  include/uapi/asm-generic/bitsperlong.h \
  /home/aosp/kernel_imx/include/uapi/linux/posix_types.h \
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
  /home/aosp/kernel_imx/arch/arm/include/uapi/asm/posix_types.h \
  /home/aosp/kernel_imx/include/uapi/asm-generic/posix_types.h \
  /home/aosp/kernel_imx/drivers/mxc/gpu-viv/hal/kernel/inc/gc_hal_enum.h \
  /home/aosp/kernel_imx/drivers/mxc/gpu-viv/hal/kernel/inc/gc_hal_base.h \
    $(wildcard include/config/format/info.h) \
  /home/aosp/kernel_imx/drivers/mxc/gpu-viv/hal/kernel/inc/gc_hal_dump.h \
  /home/aosp/kernel_imx/drivers/mxc/gpu-viv/hal/kernel/inc/gc_hal_profiler.h \
  /home/aosp/kernel_imx/drivers/mxc/gpu-viv/hal/kernel/inc/gc_hal_driver.h \
    $(wildcard include/config/android/reserved/memory/account.h) \
    $(wildcard include/config/power/management.h) \
  /home/aosp/kernel_imx/drivers/mxc/gpu-viv/hal/kernel/inc/gc_hal_driver_vg.h \
  /home/aosp/kernel_imx/drivers/mxc/gpu-viv/hal/kernel/inc/gc_hal_statistics.h \
  /home/aosp/kernel_imx/drivers/mxc/gpu-viv/hal/kernel/inc/gc_hal_vg.h \
  /home/aosp/kernel_imx/drivers/mxc/gpu-viv/hal/kernel/inc/gc_hal_driver.h \
  drivers/mxc/gpu-viv/hal/kernel/gc_hal_kernel.h \
  /home/aosp/kernel_imx/drivers/mxc/gpu-viv/hal/kernel/arch/gc_hal_kernel_hardware.h \
  /home/aosp/kernel_imx/drivers/mxc/gpu-viv/hal/kernel/archvg/gc_hal_kernel_hardware_vg.h \
  drivers/mxc/gpu-viv/hal/kernel/gc_hal_kernel_vg.h \

drivers/mxc/gpu-viv/hal/kernel/gc_hal_kernel_mmu.o: $(deps_drivers/mxc/gpu-viv/hal/kernel/gc_hal_kernel_mmu.o)

$(deps_drivers/mxc/gpu-viv/hal/kernel/gc_hal_kernel_mmu.o):
