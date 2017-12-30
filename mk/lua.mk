PKGCONFIG = pkg-config
PKGEXISTS = $(PKGCONFIG) --exists $(1) 2>/dev/null && echo $(1)
PKGFIND = $(shell for P in $(1); do $(call PKGEXISTS, $$P) && break; done)
PKGCFLAGS = $(shell $(PKGCONFIG) --cflags $(1) 2>/dev/null)
PKGLIBS = $(shell $(PKGCONFIG) --libs $(1) 2>/dev/null)
CMDFIND = $(shell for C in $(1); do command -v $$C && break; done)

ifdef STATIC_LUA
  LUA_LDLIBS = lib/lua/src/liblua.a -lm
  LUA_CFLAGS = -I lib/lua/src/
  LUA = lib/lua/src/lua

  $(dte) $(test): | lib/lua/src/liblua.a

  lib/lua/src/liblua.a:
	$(E) MAKE $@
	$(Q) $(MAKE) $(if $(V),,-s) -C lib/lua/ posix CC='$(CC)'
else
  LUA_PC = $(or \
    $(call PKGFIND, lua53 lua5.3 lua-5.3), \
    $(shell $(PKGCONFIG) --exists 'lua >= 5.3; lua < 5.4' && echo lua), \
    $(error Unable to find pkg-config file for Lua 5.3) \
  )

  LUA_LDLIBS = $(call PKGLIBS, $(LUA_PC))
  LUA_CFLAGS = $(call PKGCFLAGS, $(LUA_PC))
  LUA = $(call CMDFIND, $(LUA_PC) lua5.3 lua-5.3 lua53)

  $(foreach V, LUA_PC LUA_LDLIBS LUA_CFLAGS LUA, $(call make-lazy,$(V)))
endif
