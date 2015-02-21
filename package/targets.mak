SBIN_TARGETS := \
aa-enable \
aa-start \
aa-stop

LIBEXEC_TARGETS := \
aa-chroot \
aa-echo \
aa-kill \
aa-pivot

DOC_TARGETS := \
anopa.1 \
aa-chroot.1 \
aa-echo.1 \
aa-enable.1 \
aa-pivot.1 \
aa-start.1 \
aa-stop.1 \
aa-kill.1 \

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
