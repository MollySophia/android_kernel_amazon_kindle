cmd_arch/arm/boot/dts/imx27-pdk.dtb := /home/molly/kt3_ll_511_210/ll_511_210_build/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin/arm-eabi-gcc -E -Wp,-MD,arch/arm/boot/dts/.imx27-pdk.dtb.d.pre.tmp -nostdinc -I/home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/boot/dts -I/home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/arch/arm/boot/dts/include -undef -D__DTS__ -x assembler-with-cpp -o arch/arm/boot/dts/.imx27-pdk.dtb.dts.tmp arch/arm/boot/dts/imx27-pdk.dts ; /home/molly/kt3_ll_511_210/ll_511_210_build/kernel_imx/scripts/dtc/dtc -O dtb -o arch/arm/boot/dts/imx27-pdk.dtb -b 0 -i arch/arm/boot/dts/  -d arch/arm/boot/dts/.imx27-pdk.dtb.d.dtc.tmp arch/arm/boot/dts/.imx27-pdk.dtb.dts.tmp ; cat arch/arm/boot/dts/.imx27-pdk.dtb.d.pre.tmp arch/arm/boot/dts/.imx27-pdk.dtb.d.dtc.tmp > arch/arm/boot/dts/.imx27-pdk.dtb.d

source_arch/arm/boot/dts/imx27-pdk.dtb := arch/arm/boot/dts/imx27-pdk.dts

deps_arch/arm/boot/dts/imx27-pdk.dtb := \
  arch/arm/boot/dts/imx27.dtsi \
  arch/arm/boot/dts/skeleton.dtsi \

arch/arm/boot/dts/imx27-pdk.dtb: $(deps_arch/arm/boot/dts/imx27-pdk.dtb)

$(deps_arch/arm/boot/dts/imx27-pdk.dtb):
