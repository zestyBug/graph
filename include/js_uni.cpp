#include "quickjs/cutils.h"
#include "quickjs/quickjs.h"
#include "Solver.hpp"
#include "uni.hpp"
#include "ui_components.hpp"
#include "js_uni.hpp"

static JSValue js_uni_new_group(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    (void)this_val;
    if (
        argc < 4 ||
        !JS_IsNumber(argv[1]) ||
        !JS_IsString(argv[0])
        ) return JS_EXCEPTION;
    size_t len;
    uint8_t* str = JS_ToCStringLenRaw(ctx, &len, argv[0]);
    Array::entity grup_id = Group::registery.create();
    Group& group = Group::registery.emplace(grup_id, grup_id, (char*)str);
    group.priority = 1;
    for (size_t i = 2; i < argc; i++)
    {
        int32_t int32_arg;
        Array::entity LESSION;
        if (!JS_IsNumber(argv[i]))
            return JS_EXCEPTION;
        if (JS_ToInt32(ctx, &int32_arg, argv[i]))
            return JS_EXCEPTION;
        LESSION = *(uint32_t*)&int32_arg;
        group.members.emplace_back(LESSION);
    }
    return JS_NewInt32(ctx, *(int32_t*)&grup_id);
}

static JSValue js_uni_new_professor(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    (void)this_val;
    size_t len;
    bool hours[] = {
        false, false, false, false, false,
        false, false, false, false, false,
        false, false, false, false, false,
        false, false, false, false, false,
        false, false, false, false, false,
    };
    if (argc < 2 || !JS_IsString(argv[0]))
        return JS_EXCEPTION;
    uint8_t* str = JS_ToCStringLenRaw(ctx, &len, argv[0]);
    Array::entity prof1_id = Professor::registery.create();
    Professor& prof = Professor::registery.emplace(prof1_id, prof1_id, (char*)str);


    for (size_t i = 1; i < argc; i++)
    {
        int32_t int32_arg;
        if (!JS_IsNumber(argv[i]))
            return JS_EXCEPTION;
        if (JS_ToInt32(ctx, &int32_arg, argv[i]))
            return JS_EXCEPTION;
        if (int32_arg < 1 || int32_arg > 25)
            return JS_EXCEPTION;
        hours[int32_arg - 1] = true;
    }

    memcpy(prof.hours, hours, sizeof(hours));

    return JS_NewInt32(ctx, *(int32_t*)&prof1_id);
}

static JSValue js_uni_new_lession(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    (void)this_val;
    size_t len;
    int32_t int32_arg;
    if (argc < 2 || argc > 6 || !JS_IsString(argv[0]))
        return JS_EXCEPTION;
    uint8_t* str = JS_ToCStringLenRaw(ctx, &len, argv[0]);
    Array::entity less_id = Lession::registery.create();
    Lession& lession = Lession::registery.emplace(less_id, less_id, (char*)str);

    if (!JS_IsNumber(argv[1]))
        return JS_EXCEPTION;
    if (JS_ToInt32(ctx, &int32_arg, argv[1]))
        return JS_EXCEPTION;
    Pair::create(less_id, *(uint32_t*)&int32_arg);

    if (argc > 2) {

        if (!JS_IsNumber(argv[2]))
            return JS_EXCEPTION;
        if (JS_ToInt32(ctx, &int32_arg, argv[2]))
            return JS_EXCEPTION;
        lession.semister = int32_arg ;

        if (argc > 3) {

            if (!JS_IsNumber(argv[3]))
                return JS_EXCEPTION;
            if (JS_ToInt32(ctx, &int32_arg, argv[3]))
                return JS_EXCEPTION;
            if (int32_arg < 2 || int32_arg > 4)
                return JS_EXCEPTION;
            lession.hours = int32_arg - hour_offset;

            if (argc > 4) {

                if (!JS_IsNumber(argv[4]))
                    return JS_EXCEPTION;
                if (JS_ToInt32(ctx, &int32_arg, argv[4]))
                    return JS_EXCEPTION;
                if (int32_arg < 0 || int32_arg > 10)
                    return JS_EXCEPTION;
                lession.weight = *(uint32_t*)&int32_arg;

                if (argc > 5) {

                    if (!JS_IsNumber(argv[5]))
                        return JS_EXCEPTION;
                    if (JS_ToInt32(ctx, &int32_arg, argv[5]))
                        return JS_EXCEPTION;
                    lession.require = *(uint32_t*)&int32_arg;
                }
            }
        }
    }
    return JS_NewInt32(ctx, *(int32_t*)&less_id);
}

static const JSCFunctionListEntry js_uni_funcs[] = {
    // "name", priority = 1, lession1 , lession2, lession3, ...
    JS_CFUNC_DEF("group", 4, js_uni_new_group),
    // "name", day1, day2,...
    JS_CFUNC_DEF("professor", 2, js_uni_new_professor),
    // "name", professor, semister = -1 ,hours = {2,3,4} , weight=0 , require = undefined
    JS_CFUNC_DEF("lession", 2, js_uni_new_lession),
};

static int js_uni_init(JSContext* ctx, JSModuleDef* m)
{
    JS_SetModuleExportList(ctx, m, js_uni_funcs, countof(js_uni_funcs));
    return 0;
}

JSModuleDef* js_init_module_uni(JSContext* ctx, const char* module_name)
{
    JSModuleDef* m;
    m = JS_NewCModule(ctx, module_name, js_uni_init);
    if (!m)
        return NULL;
    JS_AddModuleExportList(ctx, m, js_uni_funcs, countof(js_uni_funcs));
    return m;
}