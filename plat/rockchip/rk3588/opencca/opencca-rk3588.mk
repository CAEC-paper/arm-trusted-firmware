PLAT_INCLUDES += -I${RK_PLAT_SOC}/opencca

BL31_SOURCES		+=	${RK_PLAT_SOC}/opencca/rk3588_opencca.c         	\
				${RK_PLAT_SOC}/opencca/rk3588_plat_attest_token.c \
				${RK_PLAT_SOC}/opencca/rk3588_realm_attest_key.c 
