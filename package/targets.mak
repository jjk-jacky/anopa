BIN_TARGETS := \
aa-start \
aa-enable

DOC_TARGETS := \
doc/anopa.1 \
doc/aa-enable.1

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
