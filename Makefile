CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2
LDFLAGS ?= -ldl

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
LIBDIR ?= $(PREFIX)/lib

TARGET_RUN = timerun
TARGET_EYE = timeeye
TARGET_LIB = libtimerun_inject.so

SRCS_RUN = main.c
SRCS_EYE = eye_main.c
SRCS_LIB = libtimerun_inject.c

all: $(TARGET_LIB) $(TARGET_RUN) $(TARGET_EYE)

$(TARGET_LIB): $(SRCS_LIB)
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $< $(LDFLAGS)

$(TARGET_RUN): $(SRCS_RUN)
	$(CC) $(CFLAGS) -o $@ $<

$(TARGET_EYE): $(SRCS_EYE)
	$(CC) $(CFLAGS) -o $@ $<

install: all
	@echo "Installing TimeRun & TimeEye Suite to $(DESTDIR)$(PREFIX)..."
	install -d $(DESTDIR)$(BINDIR)
	install -d $(DESTDIR)$(LIBDIR)
	install -m 755 $(TARGET_RUN) $(DESTDIR)$(BINDIR)/$(TARGET_RUN)
	install -m 755 $(TARGET_EYE) $(DESTDIR)$(BINDIR)/$(TARGET_EYE)
	install -m 755 $(TARGET_LIB) $(DESTDIR)$(LIBDIR)/$(TARGET_LIB)
	@echo "Installation complete!"

uninstall:
	@echo "Removing Suite from $(DESTDIR)$(PREFIX)..."
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET_RUN)
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET_EYE)
	rm -f $(DESTDIR)$(LIBDIR)/$(TARGET_LIB)
	@echo "Uninstall complete!"

clean:
	rm -f $(TARGET_RUN) $(TARGET_EYE) $(TARGET_LIB)

.PHONY: all install uninstall clean
