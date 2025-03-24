BL31_SOURCES		+=	opencca/opencca.S
INCLUDES 			+= -Iopencca/include

LDFLAGS += -v

include ${PLAT_DIR}/opencca/opencca-${PLAT}.mk

# TODO: Decide if this includes board specific files or vis versa
# TODO: Move environment vars here for openccas

CFLAGS += -DENABLE_OPENCCA=$(ENABLE_OPENCCA)

ifeq ($(FFH_SUPPORT),1)
# enabling FFH_SUPPORT currently interferes with SCR_EL3 spoofing definitions.
$(error FFH_SUPPORT currently not implemented in opencca)
endif

ifneq ($(ENABLE_RME),1)
$(error "ENABLE_RME must be enabled when building opencca. Set ENABLE_RME=1")
endif

ifeq ($(ENABLE_PAUTH),1)
$(error "ENABLE_PAUTH must be disabled")
endif

ifeq ($(CTX_INCLUDE_PAUTH_REGS),1)
$(error "CTX_INCLUDE_PAUTH_REGS must be disabled")
endif

ifeq ($(ENABLE_FEAT_CSV2_2),1)
$(error "ENABLE_FEAT_CSV2_2 must be disabled")
endif

ifneq ($(ARM_ARCH_MAJOR),8)
$(error "ARM_ARCH_MAJOR must be 8")
endif

ifneq ($(ARM_ARCH_MINOR),2)
$(error "ARM_ARCH_MINOR must be 2")
endif



