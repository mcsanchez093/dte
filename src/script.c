#include <lua.h>
#include <lauxlib.h>
#include "script.h"
#include "edit.h"

static int insert(lua_State *L) {
    size_t size;
    const char *text = luaL_checklstring(L, 1, &size);
    insert_text(text, size);
    return 0;
}

static const luaL_Reg editorlib[] = {
    {"insert", insert},
    {NULL, NULL}
};

int luaopen_editor(lua_State *L) {
    luaL_newlib(L, editorlib);
    return 1;
}
