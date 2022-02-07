cmd_arch/arm/boot/dts/imx6sx-17x17-arm2-mlb.dtb := /home/molly/kt3_ll_511_210/ll_511_210_build/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-gcc -E -Wp,-MD,arch/arm/boot/dts/.imx6sx-17x17-arm2-mlb.dtb.d.pre.tmp -nostdinc -I/home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/boot/dts -I/home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/boot/dts/include -undef -D__DTS__ -x assembler-with-cpp -o arch/arm/boot/dts/.imx6sx-17x17-arm2-mlb.dtb.dts.tmp arch/arm/boot/dts/imx6sx-17x17-arm2-mlb.dts ; /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/scripts/dtc/dtc -O dtb -o arch/arm/boot/dts/imx6sx-17x17-arm2-mlb.dtb -b 0 -i arch/arm/boot/dts/  -d arch/arm/boot/dts/.imx6sx-17x17-arm2-mlb.dtb.d.dtc.tmp arch/arm/boot/dts/.imx6sx-17x17-arm2-mlb.dtb.dts.tmp ; cat arch/arm/boot/dts/.imx6sx-17x17-arm2-mlb.dtb.d.pre.tmp arch/arm/boot/dts/.imx6sx-17x17-arm2-mlb.dtb.d.dtc.tmp > arch/arm/boot/dts/.imx6sx-17x17-arm2-mlb.dtb.d

source_arch/arm/boot/dts/imx6sx-17x17-arm2-mlb.dtb := arch/arm/boot/dts/imx6sx-17x17-arm2-mlb.dts

deps_arch/arm/boot/dts/imx6sx-17x17-arm2-mlb.dtb := \
  arch/arm/boot/dts/imx6sx-17x17-arm2.dts \
  arch/arm/boot/dts/imx6sx.dtsi \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/boot/dts/include/dt-bindings/clock/imx6sx-clock.h \
  /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/boot/dts/include/dt-bindings/gpio/gpio.h \
  arch/arm/boot/dts/imx6sx-pinfunc.h \
  arch/arm/boot/dts/skeleton.dtsi \

arch/arm/boot/dts/imx6sx-17x17-arm2-mlb.dtb: $(deps_arch/arm/boot/dts/imx6sx-17x17-arm2-mlb.dtb)

$(deps_arch/arm/boot/dts/imx6sx-17x17-arm2-mlb.dtb):
