BIN_TARGETS := \
aa-chroot \
aa-echo \
aa-enable \
aa-incmdline \
aa-kill \
aa-mount \
aa-mvlog \
aa-pivot \
aa-reboot \
aa-reset \
aa-service \
aa-setready \
aa-start \
aa-status \
aa-stop \
aa-sync \
aa-terminate \
aa-test \
aa-tty \
aa-umount

BIN_SCRIPTS_TARGET := \
aa-shutdown

LIBEXEC_SCRIPTS_TARGET := \
aa-stage0 \
aa-stage1 \
aa-stage2 \
aa-stage3 \
aa-stage4

DOC_TARGETS := \
anopa.1 \
aa-chroot.1 \
aa-echo.1 \
aa-enable.1 \
aa-incmdline.1 \
aa-kill.1 \
aa-mount.1 \
aa-mvlog.1 \
aa-pivot.1 \
aa-reboot.1 \
aa-reset.1 \
aa-service.1 \
aa-setready.1 \
aa-shutdown.1 \
aa-stage0.1 \
aa-stage1.1 \
aa-stage2.1 \
aa-stage3.1 \
aa-stage4.1 \
aa-start.1 \
aa-status.1 \
aa-stop.1 \
aa-sync.1 \
aa-terminate.1 \
aa-test.1 \
aa-tty.1 \
aa-umount.1

ifdef DO_ALLSTATIC
LIBANOPA := libanopa.a
else
LIBANOPA := libanopa.so
endif

ifdef DO_SHARED
SHARED_LIBS := libanopa.so
endif

ifdef DO_STATIC
STATIC_LIBS := libanopa.a
endif
