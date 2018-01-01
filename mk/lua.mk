AR = ar rc
RANLIB = ranlib
PKGCONFIG = pkg-config
PKGEXISTS = $(PKGCONFIG) --exists $(1) 2>/dev/null && echo $(1)
PKGFIND = $(shell for P in $(1); do $(call PKGEXISTS, $$P) && break; done)
PKGCFLAGS = $(shell $(PKGCONFIG) --cflags $(1) 2>/dev/null)
PKGLIBS = $(shell $(PKGCONFIG) --libs $(1) 2>/dev/null)
CMDFIND = $(shell for C in $(1); do command -v $$C && break; done)

ifdef STATIC_LUA
  LUA_LDLIBS = build/lua/liblua.a -lm
  LUA_CFLAGS = -I lib/lua/
  LUA = build/lua/lua

  $(dte) $(test): | build/lua/liblua.a
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

liblua_objects = $(addprefix build/lua/, \
    lapi.o lcode.o lctype.o ldebug.o ldo.o ldump.o lfunc.o lgc.o llex.o \
    lmem.o lobject.o lopcodes.o lparser.o lstate.o lstring.o ltable.o \
    ltm.o lundump.o lvm.o lzio.o \
    lauxlib.o lbaselib.o lbitlib.o lcorolib.o ldblib.o liolib.o \
    lmathlib.o loslib.o lstrlib.o ltablib.o lutf8lib.o loadlib.o linit.o )

lua_objects = $(liblua_objects) build/lua/lua.o

$(lua_objects): BASIC_CFLAGS_LUA += $(CSTD) -Wall -Wextra -DLUA_COMPAT_5_2 -DLUA_USE_POSIX

build/lua/liblua.a: $(liblua_objects)
	$(E) AR $@
	$(Q) $(AR) $@ $^
	$(Q) $(RANLIB) $@

build/lua/lua: build/lua/lua.o build/lua/liblua.a
	$(E) LINK $@
	$(Q) $(CC) -o $@ $(LDFLAGS) $< $(LUA_LDLIBS)

$(lua_objects): build/lua/%.o: lib/lua/%.c build/lua/all.cflags | build/lua/
	$(E) CC $@
	$(Q) $(CC) $(CPPFLAGS) $(CFLAGS) $(BASIC_CFLAGS_LUA) -c -o $@ $<

build/lua/all.cflags: FORCE | build/lua/
	@$(OPTCHECK) '$(CC) $(CPPFLAGS) $(CFLAGS) $(BASIC_CFLAGS_LUA)' $@

build/lua/: build/
	@mkdir -p $@
