cmd_arch/arm/boot/dts/imx6q-sabrelite.dtb := /home/molly/kt3_ll_511_210/ll_511_210_build/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-gcc -E -Wp,-MD,arch/arm/boot/dts/.imx6q-sabrelite.dtb.d.pre.tmp -nostdinc -I/home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/boot/dts -I/home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/boot/dts/include -undef -D__DTS__ -x assembler-with-cpp -o arch/arm/boot/dts/.imx6q-sabrelite.dtb.dts.tmp arch/arm/boot/dts/imx6q-sabrelite.dts ; /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/scripts/dtc/dtc -O dtb -o arch/arm/boot/dts/imx6q-sabrelite.dtb -b 0 -i arch/arm/boot/dts/  -d arch/arm/boot/dts/.imx6q-sabrelite.dtb.d.dtc.tmp arch/arm/boot/dts/.imx6q-sabrelite.dtb.dts.tmp ; cat arch/arm/boot/dts/.imx6q-sabrelite.dtb.d.pre.tmp arch/arm/boot/dts/.imx6q-sabrelite.dtb.d.dtc.tmp > arch/arm/boot/dts/.imx6q-sabrelite.dtb.d

source_arch/arm/boot/dts/imx6q-sabrelite.dtb := arch/arm/boot/dts/imx6q-sabrelite.dts

deps_arch/arm/boot/dts/imx6q-sabrelite.dtb := \
  arch/arm/boot/dts/imx6q.dtsi \
  arch/arm/boot/dts/imx6q-pinfunc.h \
  arch/arm/boot/dts/imx6qdl.dtsi \
  arch/arm/boot/dts/skeleton.dtsi \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/boot/dts/include/dt-bindings/gpio/gpio.h \

arch/arm/boot/dts/imx6q-sabrelite.dtb: $(deps_arch/arm/boot/dts/imx6q-sabrelite.dtb)

$(deps_arch/arm/boot/dts/imx6q-sabrelite.dtb):
