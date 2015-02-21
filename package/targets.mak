BIN_TARGETS := \
aa-start \
aa-stop \
aa-enable \
aa-echo \
aa-kill

DOC_TARGETS := \
anopa.1 \
aa-start.1 \
aa-stop.1 \
aa-enable.1 \
aa-echo.1 \
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
